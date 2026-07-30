//
//  Http.cpp
//  qad-lights
//
//  Created by michaelobed on 29/07/2026.
//  
//  Copyright © 2026 Michael Obed.

#include "Http.hpp"
#include "Lighting.hpp"

extern const char htmlHome[] asm("_binary_home_html_start");
extern const char htmlStyles[] asm("_binary_styles_css_start");

static Lighting& lighting = Lighting::GetInstance();

static esp_err_t onUriGet(httpd_req_t* request);

Http::Http()
{
    handle = nullptr;
    memset(Buffer, 0, BufferSize);

    uriIndexHome.handler = onUriGet;
}

char* Http::doReplacement(char* html, const char* toLookFor, const char* toReplaceItWith)
{
    char* tag = nullptr;
    int tagLocation = 0;
    int toLookForSize = strlen(toLookFor);
    int toReplaceItWithSize = strlen(toReplaceItWith);
    
    /* Look for the tag. */
    tag = strstr(html, toLookFor);
    if(tag != nullptr)
    {
        /* Copy everything until just before the tag, then replace it with the contents of toReplaceItWith. */
        tagLocation = tag - html;
        strncpy(Buffer, html, tagLocation);
        strcpy(Buffer + tagLocation, toReplaceItWith);
        strcpy(Buffer + tagLocation + toReplaceItWithSize, html + tagLocation + toLookForSize);
    }

    return Buffer;
}

esp_err_t Http::Init()
{
    httpd_config_t httpConfig = HTTPD_DEFAULT_CONFIG();

    /* Purge least recently used connection by default. */
    httpConfig.lru_purge_enable = true;

    /* Start httpd and register URIs. */
    return (    httpd_start(&handle, &httpConfig) |
                httpd_register_uri_handler(handle, &uriIndexHome));
}

void Http::onOops(httpd_req_t* request)
{
    httpd_resp_send_err(request, HTTPD_500_INTERNAL_SERVER_ERROR, "Oops.");
}

esp_err_t Http::SendPage(httpd_req_t* request, char* page)
{
    char* newHtml = nullptr;
    constexpr char tagStyles[] = "[[STYLES]]";

    /* Import styles.css. */
    newHtml = doReplacement(page, tagStyles, htmlStyles);

    if(newHtml == nullptr)
        onOops(request);
    else
    {
        /* TODO: Replace other things as necessary. */

        httpd_resp_send(request, newHtml, HTTPD_RESP_USE_STRLEN);
    }

    return ESP_OK;
}

esp_err_t onUriGet(httpd_req_t* request)
{
    Http& http = Http::GetInstance();

    return http.SendPage(request, (char*)htmlHome);
}