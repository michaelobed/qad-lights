//
//  Config.hpp
//  qad-lights
//
//  Created by michaelobed on 29/07/2026.
//  
//  Copyright © 2026 Michael Obed.

#include "esp_wifi_types.h"
#include "nvs_flash.h"

class Config
{
    public:
        Config();

        static Config& GetInstance()
        {
            static Config c;
            return c;
        }

        static constexpr size_t NameMaxLen = 128;
        char Name[NameMaxLen];
        bool NetworkIsSTA;
        char NetworkPsk[MAX_PASSPHRASE_LEN];
        char NetworkSsid[MAX_SSID_LEN];

        void EraseAll();
        esp_err_t InitStorage();
        bool Load();
        void Save();

    private:
        static constexpr uint32_t existenceNum = 0x99ef0b05;
        nvs_handle_t handle;
};