//
//  Network.hpp
//  qad-lights
//
//  Created by michaelobed on 29/07/2026.
//  
//  Copyright © 2026 Michael Obed.

#ifndef Network_hpp
#define Network_hpp

#include "esp_wifi.h"
#include <vector>

class Network
{
    public:
        struct WifiInfo
        {
            bool needsPsk;
            char ssid[MAX_SSID_LEN];
        };

        Network();

        static Network& GetInstance()
        {
            static Network n;
            return n;
        }

        void DeInit();
        esp_err_t InitAP();
        esp_err_t InitSTA();
        bool IsRunning();
        esp_err_t StartSTASearch(std::vector<WifiInfo>& list);

    private:
        static constexpr uint32_t deInitDelayUs = 1000000UL;
        static constexpr int maxConnections = 3;
        esp_netif_t* netIfInstance;
        wifi_config_t wifiConfig;

        esp_err_t postInit();
        esp_err_t preInit();
};

#endif