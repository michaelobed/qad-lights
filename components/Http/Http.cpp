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
extern const char htmlWifiSearch[] asm("_binary_wifisearch_html_start");

static Config& config = Config::GetInstance();
static Lighting& lighting = Lighting::GetInstance();
static Network& network = Network::GetInstance();

static esp_err_t onUriGet(httpd_req_t* request);
static esp_err_t onWs(httpd_req_t* request);

Http::Http()
{
    handle = nullptr;
    memset(Buffer, 0, BufferSize);

    uriHome.handler = onUriGet;
    uriWifiSearch.handler = onUriGet;
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
    esp_err_t err = ESP_OK;
    const char tagSave[] = "save";
    const char tagWifiSearch[] = "wifiSearch";

    /* Handle saving first since that isn't dependent on config tags. */
    if(strstr(data, tagSave) != nullptr)
    {
        config.Save();
        ESP_LOGI(__func__, "Config saved.");
        return true;
    }

    /* Then, handle starting a Wi-Fi scan. */
    else if(strstr(data, tagWifiSearch) != nullptr)
    {
        err = network.StartSTASearch(networkList);
        if(err != ESP_OK)
            ESP_LOGE(__func__, "Could not perform WiFi network search (%d)!", err);
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
                httpd_register_uri_handler(handle, &uriWifiSearch) |
                httpd_register_uri_handler(handle, &uriWs));
}

void Http::networkListAsTable(char* html, const char* tagText)
{
    constexpr int outSize = 2048;
    char* out = new char[outSize];
    int outCurrentLen = 0;
    char* tag = nullptr;
    char* tagStart = nullptr;

    /* Add table header. */
    sprintf(out, "<table><tr><th></th><th>Name</th><th>Needs password?</th></tr>");
    outCurrentLen = strlen(out);

    /* Prepare buttons and add an emoji when we need a PSK. */
    for(int i = 0; i < networkList.size(); i++)
    {
        sprintf(    out + outCurrentLen,
                    "<tr><td>%d.</td><td><input type=\"button\" class=\"buttonNetwork\" value=\"%s\" /></td><td>%s</td></tr>",
                    i + 1, networkList[i].ssid, networkList[i].needsPsk ? "&#x1f512;" : "");
        outCurrentLen = strlen(out);
    }

    sprintf(out + outCurrentLen, "</table>");
    outCurrentLen = strlen(out);

    /* Move everything after the tag to make space, then copy over the list.
     * Remember to use strncpy() to exclude the null terminator so things after the table don't get missed out. */
    tag = strstr(html, tagText);
    tagStart = tag;
    tag += strlen(tagText);
    strcpy(tagStart + outCurrentLen, tag);
    strncpy(tagStart, out, outCurrentLen);
}

void Http::onOops(httpd_req_t* request)
{
    httpd_resp_send_err(request, HTTPD_500_INTERNAL_SERVER_ERROR, "Oops.");
}

esp_err_t Http::SendPage(httpd_req_t* request, char* page)
{
    char* newHtml = nullptr;
    constexpr char tagNetworkList[] = "[[NETWORKLIST]]";
    constexpr char tagStyles[] = "[[STYLES]]";
    constexpr int tempBufSize = 64;
    char tempBuf[tempBufSize] = {};

    /* Import styles.css. */
    newHtml = doReplacement(page, tagStyles, htmlStyles);

    if(newHtml == nullptr)
        onOops(request);
    else
    {
        newHtml = doReplacement(newHtml, "[[CONFIG_HOSTNAME]]", config.Hostname);
        
        itoa((config.LightingColour >> 16) & 0xff, tempBuf, 10);
        newHtml = doReplacement(newHtml, "[[CONFIG_LIGHTINGCOLOUR_R]]", tempBuf);

        itoa((config.LightingColour >> 8) & 0xff, tempBuf, 10);
        newHtml = doReplacement(newHtml, "[[CONFIG_LIGHTINGCOLOUR_G]]", tempBuf);

        itoa(config.LightingColour & 0xff, tempBuf, 10);
        newHtml = doReplacement(newHtml, "[[CONFIG_LIGHTINGCOLOUR_B]]", tempBuf);

        itoa(config.LightingMode, tempBuf, 10);
        newHtml = doReplacement(newHtml, "[[CONFIG_LIGHTINGMODE]]", tempBuf);
        
        itoa(config.SwitchPolarity, tempBuf, 10);
        newHtml = doReplacement(newHtml, "[[CONFIG_SWITCHPOLARITY]]", tempBuf);

        itoa(config.NetworkIsSTA ? 1 : 0, tempBuf, 10);
        newHtml = doReplacement(newHtml, "[[CONFIG_WIFIMODE]]", tempBuf);

        if(strstr(newHtml, tagNetworkList) != nullptr)
            networkListAsTable(newHtml, tagNetworkList);

        httpd_resp_send(request, newHtml, HTTPD_RESP_USE_STRLEN);
    }

    return ESP_OK;
}

esp_err_t onUriGet(httpd_req_t* request)
{
    char* pageToSend = (char*)htmlHome;

    /* We will send the home page by default, but change it if we need to send something else. */
    if(strstr(request->uri, "wifisearch") != nullptr)
        pageToSend = (char*)htmlWifiSearch;

    return Http::GetInstance().SendPage(request, pageToSend);
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