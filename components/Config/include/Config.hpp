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
        struct ConfigData
        {
            static constexpr int tagMaxLen = 32;
            char tag[tagMaxLen];
            void* data;
            size_t size;
        };

        enum LEDMode : uint8_t
        {
            LEDMode_Off = 0,            /* Permanently off. */
            LEDMode_OnAny,              /* On if any switches are active. */
            LEDMode_OnAll,              /* On if all switches are active. */
            LEDMode_On                  /* Permanently on. */
        };

        enum SwPol : uint8_t
        {
            SwPol_NormallyOpen = 0,
            SwPol_NormallyClosed
        };

        Config();

        static Config& GetInstance()
        {
            static Config c;
            return c;
        }
        
        static constexpr int DataMapMaxLen = 7;
        static constexpr size_t HostnameMaxLen = 128;
        char Hostname[HostnameMaxLen];
        uint32_t LightingColour;
        int LightingMode;
        bool NetworkIsSTA;
        char NetworkPsk[MAX_PASSPHRASE_LEN];
        char NetworkSsid[MAX_SSID_LEN];
        int SwitchPolarity;

        ConfigData DataMap[DataMapMaxLen] =
        {
            {   "lightingColour", &LightingColour, 4 },
            {   "lightingMode",   &LightingMode,   1 },
            {   "hostname",       Hostname,        HostnameMaxLen },
            {   "networkIsSTA",   &NetworkIsSTA,   1 },
            {   "networkPsk",     &NetworkPsk,     MAX_PASSPHRASE_LEN },
            {   "networkSsid",    &NetworkSsid,    MAX_SSID_LEN },
            {   "switchPolarity", &SwitchPolarity, 1}
        };

        void EraseAll();
        esp_err_t InitStorage();
        esp_err_t Load();
        void Save();

    private:
        static constexpr uint32_t existenceNum = 0x99ef0b05;
        nvs_handle_t handle;
};

#endif