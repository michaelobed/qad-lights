//
//  Config.hpp
//  qad-lights
//
//  Created by michaelobed on 29/07/2026.
//  
//  Copyright © 2026 Michael Obed.

#ifndef Config_hpp
#define Config_hpp

#include "esp_wifi_types.h"
#include "nvs_flash.h"

class Config
{
    public:
        enum LEDMode : uint8_t
        {
            LEDMode_Off = 0,            /* Permanently off. */
            LEDMode_OnAny,              /* On if any switches are active. */
            LEDMode_OnAll,              /* On if all switches are active. */
            LEDMode_On                  /* Permanently on. */
        };

        Config();

        static Config& GetInstance()
        {
            static Config c;
            return c;
        }
        
        uint32_t LightingColour;
        int LightingMode;
        static constexpr size_t NameMaxLen = 128;
        char Name[NameMaxLen];
        bool NetworkIsSTA;
        char NetworkPsk[MAX_PASSPHRASE_LEN];
        char NetworkSsid[MAX_SSID_LEN];

        void EraseAll();
        esp_err_t InitStorage();
        esp_err_t Load();
        void Save();

    private:
        static constexpr int tagMaxLen = 32;
        struct ConfigData
        {
            char tag[tagMaxLen];
            void* data;
            size_t size;
        };

        static constexpr uint32_t existenceNum = 0x99ef0b05;
        nvs_handle_t handle;

        static constexpr int dataMaxLen = 6;
        ConfigData data[dataMaxLen] =
        {
            {   "lightingColour", &LightingColour, 4 },
            {   "lightingMode",   &LightingMode,   1 },
            {   "name",           Name,            NameMaxLen },
            {   "networkIsSTA",   &NetworkIsSTA,   1 },
            {   "networkPsk",     &NetworkPsk,     MAX_PASSPHRASE_LEN },
            {   "networkSsid",    &NetworkSsid,    MAX_SSID_LEN }
        };
};

#endif