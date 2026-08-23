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
#include "../../main/main.hpp"
#include "Sleep.hpp"
#include "SwitchIO.hpp"
#include "Update.hpp"

extern const char htmlFwUpdate[] asm("_binary_fwupdate_html_start");
extern const char htmlHome[] asm("_binary_home_html_start");
extern const char htmlRestart[] asm("_binary_restart_html_start");
extern const char htmlStyles[] asm("_binary_styles_css_start");
extern const char htmlWifiSearch[] asm("_binary_wifisearch_html_start");

static Config& config = Config::GetInstance();
static Lighting& lighting = Lighting::GetInstance();
static Network& network = Network::GetInstance();
static Sleep& sleep = Sleep::GetInstance();
static SwitchIO& switchIo = SwitchIO::GetInstance();
static Update& update = Update::GetInstance();

static esp_err_t onUriGet(httpd_req_t* request);
static esp_err_t onUriPost(httpd_req_t* request);
static esp_err_t onWs(httpd_req_t* request);

Http::Http()
{
    memset(Buffer, 0, BufferSize);
    fwBytesRemaining = 0;
    handle = nullptr;
    State = State_Normal;

    uriFwUpdate.handler = onUriGet;
    uriHome.handler = onUriGet;
    uriRestart.handler = onUriGet;
    uriWifiSearch.handler = onUriGet;
    uriWifiSubmit.handler = onUriPost;
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
            strncpy(Buffer + tagLocation, toReplaceItWith, toReplaceItWithSize + 1);
            strcpy(Buffer + tagLocation + toReplaceItWithSize, html + tagLocation + toLookForSize);
            tag = Buffer;
        }
    }

    return Buffer;
}

esp_err_t Http::HandleFwUpdate(httpd_req_t* request, uint8_t* data, size_t length)
{
    httpd_ws_frame_t ackPacket =
    {
        .final = false,
        .fragmented = false,
        .type = HTTPD_WS_TYPE_TEXT,
        .payload = nullptr,
        .len = 0

    };
    const char* dataCast = (char*)data;
    uint8_t* dataStart = nullptr;
    esp_err_t err = ESP_OK;
    constexpr char respAckChunk[] = "fwchack";
    constexpr char respAckSize[] = "fwsizeack";
    constexpr char respNack[] = "fwnack";
    constexpr char respNoNeed[] = "fwnoneed";
    constexpr char tagFwChunk[] = "fwchunk: ";
    constexpr char tagFwSize[] = "fwsize: ";
    bool shouldContinue = true;
    char* tag = nullptr;

    /* Find the data we're interested in. */
    if(strstr(dataCast, tagFwSize) != nullptr)
        tag = (char*)tagFwSize;
    else if(strstr(dataCast, tagFwChunk) != nullptr)
        tag = (char*)tagFwChunk;
    if(tag == nullptr)
        return ESP_FAIL;
    dataStart = (uint8_t*)strstr(dataCast, tag) + strlen(tag);
    length -= dataStart - data;

    switch(State)
    {
        /* This should be a "fwsize" tag, so parse that. */
        case State_Normal:
            fwBytesRemaining = atoi((char*)dataStart);
            ESP_LOGI(__func__, "Got file size of %u", fwBytesRemaining);
            ackPacket.payload = (uint8_t*)respAckSize;
            ackPacket.len = strlen(respAckSize);
            httpd_ws_send_frame(request, &ackPacket);
            State = State_FwGotSize;
            return err;

        /* This should be the first chunk of the firmware data. Analyse it to determine whether to continue. */
        case State_FwGotSize:
            err = update.WriteStart(dataStart, length, shouldContinue);
            if((err == ESP_OK) && !shouldContinue)
            {
                ESP_LOGW(__func__, "Firmware data identical to current, ignoring further data.");
                ackPacket.payload = (uint8_t*)respNoNeed;
                ackPacket.len = strlen(respNoNeed);
                State = State_Normal;
                httpd_ws_send_frame(request, &ackPacket);
                return err;
            }
            State = State_FwGotFirstChunk;
            break;

        /* Continuing the write. */
        case State_FwGotFirstChunk:
            err = update.WriteContinue(dataStart, length);
            break;

        /* Invalid! */
        default:
            return ESP_FAIL;
    }

    if(err == ESP_OK)
    {
        ackPacket.payload = (uint8_t*)respAckChunk;
        ackPacket.len = strlen(respAckChunk);
        fwBytesRemaining -= length;
    }
    else
    {
        /* If we're here, something went wrong. */
        ackPacket.payload = (uint8_t*)respNack;
        ackPacket.len = strlen(respNack);
        State = State_Normal;
    }
    httpd_ws_send_frame(request, &ackPacket);

    /* End the write if all bytes have been received. */
    if(fwBytesRemaining == 0)
        err = update.WriteEnd();

    return err;
}

esp_err_t Http::HandleWifiSubmit(httpd_req_t* request)
{
    int err = 0;
    int index = -1;
    char indexString[4];
    char* nextToken = nullptr;
    char* pageToSend = (char*)htmlRestart;
    char* tag = nullptr;
    int tagLen = -1;

    /* Again, send back the home page by default unless we really are processing something. */
    pageToSend = (char*)htmlRestart;

    /* Get the POST data.
     * For the record, I really hate that we're sending passwords in plaintext via POST, but whatever. It's quick-and-dirty for a reason... */
    memset(Buffer, 0, BufferSize);
    err = httpd_req_recv(request, Buffer, request->content_len);

    /* If the socket was closed, abort everything. */
    if(err == 0)
        return ESP_FAIL;

    /* Handle timeout. */
    else if(err == HTTPD_SOCK_ERR_TIMEOUT)
        httpd_resp_send_408(request);

    /* Handle SSID. */
    tag = strstr(Buffer, "ssid=");

    /* Find PSK. It may be that we never got one, so handle that case too. */
    nextToken = strchr(tag, '&');
    if(nextToken == nullptr)
        tagLen = strlen(tag);
    else tagLen = nextToken - tag;

    /* Find the index of the selected network, then apply SSID. */
    strncpy(indexString, tag + 5, tagLen);
    index = atoi(indexString);
    config.SetConfigData("networkSsid", NetworkList[index].ssid);

    /* Apply PSK if there is one. */
    if(NetworkList[index].needsPsk)
    {
        tag = strstr(Buffer, "psk=");
        tag += 4;

        /* The PSK arrives percent-encoded, so we have some character replacement to do. */
        nextToken = tag;
        while(nextToken != nullptr)
        {
            /* Turn '+' into spaces. */
            nextToken = strchr(nextToken, '+');
            if(nextToken == nullptr)
                break;
            *nextToken = ' ';
        }

        /* Turn '%' into whatever the hex number after it is. */
        nextToken = tag;
        while(nextToken != nullptr)
        {
            nextToken = strchr(nextToken, '%');
            if(nextToken == nullptr)
                break;
            indexString[0] = *(nextToken + 1);
            indexString[1] = *(nextToken + 2);
            indexString[2] = '\0';
            *nextToken = strtol(indexString, nullptr, 16);
            strncpy(nextToken + 1, nextToken + 3, request->content_len - (tag - nextToken));
        }
        config.SetConfigData("networkPsk", tag);
    }

    config.Save();
    SendPage(request, pageToSend);
    return ESP_OK;
}

bool Http::HandleWs(httpd_req_t* request, char* data, size_t length)
{
    char* dataStart = nullptr;
    esp_err_t err = ESP_OK;
    bool shouldRestart = false;
    char* tag = nullptr;
    constexpr char tagLoadedRestart[] = "loadedrestart";
    constexpr char tagSave[] = "save";
    constexpr char tagWifiSearch[] = "wifiSearch";

    /* Deal with OTA update things first. */
    if(strstr(data, "fw") != nullptr)
    {
        HandleFwUpdate(request, (uint8_t*)data, length);
        return true;
    }

    /* Then saving since that isn't dependent on config tags. */
    else if(strstr(data, tagSave) != nullptr)
    {
        shouldRestart = config.Save();

        /* Deal with the case where changes trigger restarts. */
        if(shouldRestart)
            triggerRestart(request);
        return true;
    }

    /* Then, handle starting a Wi-Fi scan. */
    else if(strstr(data, tagWifiSearch) != nullptr)
    {
        State = State_WifiSearch;
        err = network.StartSTASearch(NetworkList);
        if(err != ESP_OK)
            ESP_LOGE(__func__, "Could not perform WiFi network search (%d)!", err);
        return true;
    }

    /* And restarts. */
    else if(strstr(data, tagLoadedRestart) != nullptr)
    {
        ESP_LOGW(__func__, "Restart triggered via Websocket, beginning restart.");
        Restart();
        while(true);
    }

    /* Otherwise, we are looking for exactly one tag which should be identical to that of the Config data.
     * If it isn't, it's invalid. */
    tag = new char[16];
    strncpy(tag, data, strchr(data, ':') - data);
    
    if(config.ConfigDataIsValidTag(tag))
    {
        /* Look for a ": ". Anything after that is the data we seek. */
        dataStart = strstr(data, ": ");
        if(dataStart != nullptr)
        {
            dataStart += 2;
            if(config.ConfigDataIsInt(tag))
                config.SetConfigData(tag, atoi(dataStart));
            else config.SetConfigData(tag, dataStart);

            /* If it was a lighting change, handle that. */
            if(strstr(data, "lightingColour") != nullptr)
                lighting.SetColour(atoi(dataStart), false);

            /* Handle switch polarity changes. */
            else if(strstr(data, "switchPolarity") != nullptr)
                switchIo.ConfigurePolarity(atoi(dataStart) == Config::SwPol_NormallyClosed);

            /* Handle sleep mode changes. */
            else if(strstr(data, "sleepMode") != nullptr)
            {
                /* Stop the previous timer before starting one, but only if it wasn't the on-startup holdoff timer. */
                if(!sleep.TimerStartDefaultIsActive())
                    sleep.TimerStop();
                sleep.TimerStart();
            }
        }

        delete[] tag;
        return true;
    }

    if(tag != nullptr)
        delete[] tag;
    return false;
}

esp_err_t Http::Init()
{
    httpd_config_t httpConfig = HTTPD_DEFAULT_CONFIG();

    /* Purge least recently used connection by default. */
    httpConfig.lru_purge_enable = true;

    /* Start httpd and register URIs. */
    return (    httpd_start(&handle, &httpConfig) |
                httpd_register_uri_handler(handle, &uriFwUpdate) |
                httpd_register_uri_handler(handle, &uriHome) |
                httpd_register_uri_handler(handle, &uriRestart) |
                httpd_register_uri_handler(handle, &uriWifiSearch) |
                httpd_register_uri_handler(handle, &uriWifiSubmit) |
                httpd_register_uri_handler(handle, &uriWs));
}

void Http::networkListAsSelect(char* html, const char* tagText)
{
    constexpr int outSize = 2048;
    char* out = new char[outSize];
    int outCurrentLen = 0;
    char* tag = nullptr;
    char* tagStart = nullptr;

    /* Prepare the dropdown options and add an emoji when we need a PSK. */
    for(int i = 0; i < NetworkList.size(); i++)
    {
        sprintf(    out + outCurrentLen,
                    "<option value=\"%d\" data-needspsk=\"%s\">%s%s</option>",
                    i, NetworkList[i].needsPsk ? "true" : "false", NetworkList[i].ssid, NetworkList[i].needsPsk ? " &#x1f512;" : "");
        outCurrentLen = strlen(out);
    }

    /* Move everything after the tag to make space, then copy over the list. */
    tag = strstr(html, tagText);
    tagStart = tag;
    tag += strlen(tagText);
    strncpy(out + outCurrentLen, tagStart, outSize - outCurrentLen);
    strncpy(tag, out, outSize);
    State = State_WifiChoice;
}

void Http::onOops(httpd_req_t* request)
{
    httpd_resp_send_err(request, HTTPD_500_INTERNAL_SERVER_ERROR, "Oops.");
}

esp_err_t Http::SendPage(httpd_req_t* request, char* page)
{
    char* hostname = nullptr;
    uint32_t lightingColour = config.GetConfigData("lightingColour");
    char* newHtml = nullptr;
    constexpr char tagNetworkList[] = "[[NETWORKLIST]]";
    constexpr char tagRestartWait[] = "[[RESTARTWAIT]]";
    constexpr char tagStyles[] = "[[STYLES]]";
    constexpr int tempBufSize = 64;
    char tempBuf[tempBufSize] = {};

    /* Import styles.css. */
    newHtml = doReplacement(page, tagStyles, htmlStyles);

    if(newHtml == nullptr)
        onOops(request);
    else
    {        
        itoa((lightingColour >> 16) & 0xff, tempBuf, 10);
        newHtml = doReplacement(newHtml, "[[CONFIG_LIGHTINGCOLOUR_R]]", tempBuf);

        itoa((lightingColour >> 8) & 0xff, tempBuf, 10);
        newHtml = doReplacement(newHtml, "[[CONFIG_LIGHTINGCOLOUR_G]]", tempBuf);

        itoa(lightingColour & 0xff, tempBuf, 10);
        newHtml = doReplacement(newHtml, "[[CONFIG_LIGHTINGCOLOUR_B]]", tempBuf);

        itoa(config.GetConfigData("lightingMode"), tempBuf, 10);
        newHtml = doReplacement(newHtml, "[[CONFIG_LIGHTINGMODE]]", tempBuf);

        config.GetConfigData("networkHostname", &hostname);
        newHtml = doReplacement(newHtml, "[[CONFIG_NETWORKHOSTNAME]]", hostname);

        itoa(config.GetConfigData("networkIsSTA") ? 1 : 0, tempBuf, 10);
        newHtml = doReplacement(newHtml, "[[CONFIG_NETWORKISSTA]]", tempBuf);

        itoa(config.GetConfigData("sleepMode"), tempBuf, 10);
        newHtml = doReplacement(newHtml, "[[CONFIG_SLEEPMODE]]", tempBuf);
        
        itoa(config.GetConfigData("switchPolarity"), tempBuf, 10);
        newHtml = doReplacement(newHtml, "[[CONFIG_SWITCHPOLARITY]]", tempBuf);

        if(strstr(newHtml, tagNetworkList) != nullptr)
            networkListAsSelect(newHtml, tagNetworkList);

        /* For tagRestartWait, the value depends on if we're updating network settings. */
        itoa(strstr(request->uri, "wifisearch") != nullptr ? restartWaitMsNetwork : restartWaitMs, tempBuf, 10);
        newHtml = doReplacement(newHtml, tagRestartWait, tempBuf);

        httpd_resp_send(request, newHtml, HTTPD_RESP_USE_STRLEN);
    }

    return ESP_OK;
}

void Http::triggerRestart(httpd_req_t* request)
{
    constexpr char restartString[] = "restart";
    httpd_ws_frame_t packet =
    {
        .final = false,                     /* Don't care (see esp_http_server.h). */
        .fragmented = false,
        .type = HTTPD_WS_TYPE_TEXT,
        .payload = (uint8_t*)restartString,
        .len = strlen(restartString)
    };

    /* Warn the client that a restart is happening. This will trigger a href change to "/restart". */
    httpd_ws_send_frame(request, &packet);
}

esp_err_t onUriGet(httpd_req_t* request)
{
    Http& http = Http::GetInstance();
    char* pageToSend = (char*)htmlHome;

    /* We will send the home page by default, but change it if we need to send something else. */
    if((http.State == Http::State_WifiSearch) && (strstr(request->uri, "wifisearch") != nullptr))
        pageToSend = (char*)htmlWifiSearch;
    else if(strstr(request->uri, "restart") != nullptr)
        pageToSend = (char*)htmlRestart;
    else if(strstr(request->uri, "fwupdate") != nullptr)
        pageToSend = (char*)htmlFwUpdate;

    return http.SendPage(request, pageToSend);
}

esp_err_t onUriPost(httpd_req_t* request)
{
    Http& http = Http::GetInstance();
    
    /* Structure allows for potentially future POST sub-handlers. */
    if((http.State == Http::State_WifiChoice) && strstr(request->uri, "wifisubmit") != nullptr)
        return http.HandleWifiSubmit(request);
    else return ESP_FAIL;
}

esp_err_t onWs(httpd_req_t* request)
{
    esp_err_t err = ESP_OK;
    Http& http = Http::GetInstance();
    uint8_t* rxBuf = nullptr;
    httpd_ws_frame_t wsFrame;

    memset(&wsFrame, 0, sizeof(httpd_ws_frame_t));
    wsFrame.type = ((http.State == Http::State_FwGotSize) || (http.State == Http::State_FwGotFirstChunk)) ? HTTPD_WS_TYPE_BINARY : HTTPD_WS_TYPE_TEXT;

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
        if(!http.HandleWs(request, (char*)rxBuf, wsFrame.len))
        {
            ESP_LOGE(__func__, "Invalid payload!");
            err = ESP_ERR_INVALID_RESPONSE;
        }
    }

    delete[] rxBuf;
    return err;
}