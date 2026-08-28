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
#include "Network.hpp"

class Http
{
    public:
        enum StateType
        {
            State_Normal,
            State_WifiSearch,
            State_WifiChoice,
            State_FwGotSize,
            State_FwIgnoring
        };

        Http();

        static Http& GetInstance()
        {
            static Http h;
            return h;
        }

        static constexpr size_t BufferSize = 20000;
        char Buffer[BufferSize];
        std::vector<Network::WifiInfo> NetworkList;
        StateType State;

        esp_err_t HandleFwUpdate(httpd_req_t* request);
        esp_err_t HandleWifiSubmit(httpd_req_t* request);
        bool HandleWs(httpd_req_t* request, char* data, size_t length);
        esp_err_t Init();
        esp_err_t SendPage(httpd_req_t* request, char* page);

    private:
        size_t fwBytesRemaining;
        uint8_t fwChunkBuffer[BufferSize];
        bool fwIsFirstChunk;
        httpd_handle_t handle;
        static constexpr int restartWaitMs = 3000;
        static constexpr int restartWaitMsNetwork = 30000;
        static constexpr int wsUriBufSize = 64;
        char wsUriBuf[wsUriBufSize];

        /* URIs. */
        httpd_uri_t uriFwSubmit =
        {
            .uri = "/fwsubmit",
            .method = HTTP_POST,
            .handler = nullptr,
            .user_ctx = nullptr,
            .is_websocket = false,
            .handle_ws_control_frames = false,
            .supported_subprotocol = nullptr
        };

        httpd_uri_t uriFwUpdate =
        {
            .uri = "/fwupdate",
            .method = HTTP_GET,
            .handler = nullptr,
            .user_ctx = nullptr,
            .is_websocket = false,
            .handle_ws_control_frames = false,
            .supported_subprotocol = nullptr
        };

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

        httpd_uri_t uriRestart =
        {
            .uri = "/restart",
            .method = HTTP_GET,
            .handler = nullptr,
            .user_ctx = nullptr,
            .is_websocket = false,
            .handle_ws_control_frames = false,
            .supported_subprotocol = nullptr
        };

        httpd_uri_t uriWifiSearch =
        {
            .uri = "/wifisearch",
            .method = HTTP_GET,
            .handler = nullptr,
            .user_ctx = nullptr,
            .is_websocket = false,
            .handle_ws_control_frames = false,
            .supported_subprotocol = nullptr
        };

        httpd_uri_t uriWifiSubmit =
        {
            .uri = "/wifisubmit",
            .method = HTTP_POST,
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
        void networkListAsSelect(char* html, const char* tagText);
        void onOops(httpd_req_t* request);
        void triggerRestart(httpd_req_t* request);
};

#endif