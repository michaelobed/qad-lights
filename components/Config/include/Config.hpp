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
#include <vector>

class Config
{
    public:
        struct ConfigData
        {
            static constexpr int tagMaxLen = 32;
            char tag[tagMaxLen];
            void* data;
            size_t size;
            bool willTriggerRestart;
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

        bool ConfigDataIsInt(const char* tag);
        bool ConfigDataIsValidTag(const char* tag);
        void EraseAll();
        uint32_t GetConfigData(const char* tag);
        void GetConfigData(const char* tag, char** out);
        esp_err_t InitStorage();
        esp_err_t Load();
        bool Save();
        void SetConfigData(const char* tag, uint32_t value);
        void SetConfigData(const char* tag, char* value);

    private:
        static constexpr uint32_t existenceNum = 0x99ef0b05;
        nvs_handle_t handle;
        static constexpr size_t hostnameMaxLen = 128;
        char hostname[hostnameMaxLen];
        uint32_t lightingColour;
        int lightingMode;
        bool networkIsSTA;
        char networkPsk[MAX_PASSPHRASE_LEN];
        char networkSsid[MAX_SSID_LEN];
        bool shouldRestart;
        int switchPolarity;

        const std::vector<ConfigData> dataMap =
        {
            {   "lightingColour",   &lightingColour,    4,                  false },
            {   "lightingMode",     &lightingMode,      1,                  false },
            {   "networkHostname",  hostname,           hostnameMaxLen,     true },
            {   "networkIsSTA",     &networkIsSTA,      1,                  true },
            {   "networkPsk",       &networkPsk,        MAX_PASSPHRASE_LEN, true },
            {   "networkSsid",      &networkSsid,       MAX_SSID_LEN,       true },
            {   "switchPolarity",   &switchPolarity,    1,                  false }
        };

        int configDataGetIndexOfTag(const char* tag);
};

#endif