//
//  Http.cpp
//  qad-lights
//
//  Created by michaelobed on 29/07/2026.
//  
//  Copyright © 2026 Michael Obed.

#include "Config.hpp"
#include "esp_log.h"
#include "Http.hpp"
#include "Lighting.hpp"

extern const char htmlHome[] asm("_binary_home_html_start");
extern const char htmlStyles[] asm("_binary_styles_css_start");

static Config& config = Config::GetInstance();
static Lighting& lighting = Lighting::GetInstance();

static esp_err_t onUriGet(httpd_req_t* request);
static esp_err_t onWs(httpd_req_t* request);

Http::Http()
{
    handle = nullptr;
    memset(Buffer, 0, BufferSize);

    uriHome.handler = onUriGet;
    uriWs.handler = onWs;
}

char* Http::doReplacement(char* html, const char* toLookFor, const char* toReplaceItWith)
{
    char* tag = nullptr;
    int tagLocation = 0;
    int toLookForSize = strlen(toLookFor);
    int toReplaceItWithSize = strlen(toReplaceItWith);
    
    /* Look for the tag. */
    tag = html;
    while(tag != nullptr)
    {
        tag = strstr(tag, toLookFor);
        if(tag != nullptr)
        {
            /* Copy everything until just before the tag, then replace it with the contents of toReplaceItWith. */
            tagLocation = tag - html;
            strncpy(Buffer, html, tagLocation);
            strcpy(Buffer + tagLocation, toReplaceItWith);
            strcpy(Buffer + tagLocation + toReplaceItWithSize, html + tagLocation + toLookForSize);
            tag = Buffer;
        }
    }

    return Buffer;
}

bool Http::HandleWs(char* data, size_t length)
{
    char* dataStart = nullptr;
    const char saveTag[] = "save";

    /* Handle saving first since that isn't dependent on config tags. */
    if(strstr(data, saveTag) != nullptr)
    {
        config.Save();
        ESP_LOGI(__func__, "Config saved.");
        return true;
    }

    /* Otherwise, we are looking for exactly one tag which should be identical to that of the Config data.
     * If it isn't, it's invalid. */
    for(int i = 0; i < Config::DataMapMaxLen; i++)
    {
        if(strstr(data, config.DataMap[i].tag) != nullptr)
        {
            /* Look for a ": ". Anything after that is the data we seek. */
            dataStart = strstr(data, ": ");
            if(dataStart != nullptr)
            {
                dataStart += 2;
                switch(config.DataMap[i].size)
                {
                    case 1:
                        *(uint8_t*)config.DataMap[i].data = atoi(dataStart);
                        break;

                    case 2:
                        *(uint16_t*)config.DataMap[i].data = atoi(dataStart);
                        break;

                    case 4:
                        *(uint32_t*)config.DataMap[i].data = atoi(dataStart);
                        break;

                    /* This'll be a string. */
                    default:
                        strncpy(static_cast<char*>(config.DataMap[i].data), dataStart, config.DataMap[i].size);
                        break;
                }

                /* If it was a lighting change, handle that. */
                if(strstr(data, "lightingColour") != nullptr)
                    lighting.SetColour(atoi(dataStart), false);
                else if(strstr(data, "lightingMode") != nullptr)
                {
                    /* TODO: Handle other modes. */
                    switch(config.LightingMode)
                    {
                        case Config::LEDMode_Off:
                            lighting.Off();
                            break;

                        case Config::LEDMode_On:
                            lighting.On();
                            break;
                    }
                }
            }
            return true;
        }
    }

    return false;
}

esp_err_t Http::Init()
{
    httpd_config_t httpConfig = HTTPD_DEFAULT_CONFIG();

    /* Purge least recently used connection by default. */
    httpConfig.lru_purge_enable = true;

    /* Start httpd and register URIs. */
    return (    httpd_start(&handle, &httpConfig) |
                httpd_register_uri_handler(handle, &uriHome) |
                httpd_register_uri_handler(handle, &uriWs));
}

void Http::onOops(httpd_req_t* request)
{
    httpd_resp_send_err(request, HTTPD_500_INTERNAL_SERVER_ERROR, "Oops.");
}

esp_err_t Http::SendPage(httpd_req_t* request, char* page)
{
    char* newHtml = nullptr;
    constexpr char tagStyles[] = "[[STYLES]]";
    char tempBuf[64] = {};

    /* Import styles.css. */
    newHtml = doReplacement(page, tagStyles, htmlStyles);

    if(newHtml == nullptr)
        onOops(request);
    else
    {
        itoa((config.LightingColour >> 16) & 0xff, tempBuf, 10);
        newHtml = doReplacement(newHtml, "[[CONFIG_LIGHTINGCOLOUR_R]]", tempBuf);

        itoa((config.LightingColour >> 8) & 0xff, tempBuf, 10);
        newHtml = doReplacement(newHtml, "[[CONFIG_LIGHTINGCOLOUR_G]]", tempBuf);

        itoa(config.LightingColour & 0xff, tempBuf, 10);
        newHtml = doReplacement(newHtml, "[[CONFIG_LIGHTINGCOLOUR_B]]", tempBuf);

        httpd_resp_send(request, newHtml, HTTPD_RESP_USE_STRLEN);
    }

    return ESP_OK;
}

esp_err_t onUriGet(httpd_req_t* request)
{
    return Http::GetInstance().SendPage(request, (char*)htmlHome);
}

esp_err_t onWs(httpd_req_t* request)
{
    esp_err_t err = ESP_OK;
    uint8_t* rxBuf = nullptr;
    httpd_ws_frame_t wsFrame;

    memset(&wsFrame, 0, sizeof(httpd_ws_frame_t));
    wsFrame.type = HTTPD_WS_TYPE_TEXT;

    /* Get frame length. Returned value is exclusive of null terminator. */
    err = httpd_ws_recv_frame(request, &wsFrame, 0);
    if(err != ESP_OK)
    {
        ESP_LOGE(__func__, "Could not get WS frame length (%d)!", err);
        return err;
    }
    
    /* Allocate the buffer and receive. */
    rxBuf = new uint8_t[wsFrame.len + 1];
    if(rxBuf == nullptr)
    {
        ESP_LOGE(__func__, "Could not allocate memory!");
        return ESP_ERR_NO_MEM;
    }
    memset(rxBuf, 0, wsFrame.len + 1);

    wsFrame.payload = rxBuf;
    err = httpd_ws_recv_frame(request, &wsFrame, wsFrame.len);
    if(err != ESP_OK)
        ESP_LOGE(__func__, "Could not receive WS frame (%d)!", err);
    else
    {
        /* Handle the data. */
        if(!Http::GetInstance().HandleWs((char*)rxBuf, wsFrame.len))
        {
            ESP_LOGE(__func__, "Invalid payload!");
            err = ESP_ERR_INVALID_RESPONSE;
        }
    }

    delete[] rxBuf;
    return err;
}