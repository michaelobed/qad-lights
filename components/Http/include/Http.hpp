//
//  Http.hpp
//  qad-lights
//
//  Created by michaelobed on 29/07/2026.
//  
//  Copyright © 2026 Michael Obed.

#ifndef Http_hpp
#define Http_hpp

#include "esp_http_server.h"

class Http
{
    public:
        Http();

        static Http& GetInstance()
        {
            static Http h;
            return h;
        }

        static constexpr int BufferSize = 8192;
        char Buffer[BufferSize];

        bool HandleWs(char* data, size_t length);
        esp_err_t Init();
        esp_err_t SendPage(httpd_req_t* request, char* page);

    private:
        httpd_handle_t handle;
        static constexpr int wsUriBufSize = 64;
        char wsUriBuf[wsUriBufSize];

        /* URIs. */
        httpd_uri_t uriHome =
        {
            .uri = "/",
            .method = HTTP_GET,
            .handler = nullptr,
            .user_ctx = nullptr,
            .is_websocket = false,
            .handle_ws_control_frames = false,
            .supported_subprotocol = nullptr
        };

        httpd_uri_t uriWs =
        {
            .uri = "/ws",
            .method = HTTP_GET,
            .handler = nullptr,
            .user_ctx = nullptr,
            .is_websocket = true,
            .handle_ws_control_frames = false,
            .supported_subprotocol = nullptr
        };

        char* doReplacement(char* html, const char* toLookFor, const char* toReplaceItWith);
        void onOops(httpd_req_t* request);
};

#endif