//
//  Config.hpp
//  qad-lights
//
//  Created by michaelobed on 29/07/2026.
//  
//  Copyright © 2026 Michael Obed.

#include "Config.hpp"
#include <cstring>
#include "esp_log.h"

Config::Config()
{
    /* Default values go here. */
    strncpy(hostname, "qad-lights", hostnameMaxLen);
    lightingColour = 0xffffff;
    lightingMode = LightingMode_On;
    networkIsSTA = false;
    strncpy(networkSsid, "YourNetworkHere", MAX_SSID_LEN);
    memset(networkPsk, 0, MAX_PASSPHRASE_LEN);
    shouldRestart = false;
    sleepMode = SleepMode_Never;
    switchPolarity = SwPol_NormallyClosed;
}

int Config::configDataGetIndexOfTag(const char* tag)
{
    for(int i = 0; i < dataMap.size(); i++)
    {
        if(strcmp(dataMap[i].tag, tag) == 0)
            return i;
    }

    return -1;
}

bool Config::ConfigDataIsInt(const char* tag)
{
    int index = configDataGetIndexOfTag(tag);

    if(index != -1)
    {
        switch(dataMap[index].size)
        {
            case 1:
            case 2:
            case 4:
                return true;
    
            default:
                return false;
        }
    }
    else return false;
}

bool Config::ConfigDataIsValidTag(const char* tag)
{
    return (configDataGetIndexOfTag(tag) != -1);
}

void Config::EraseAll()
{
    nvs_erase_all(handle);
    nvs_commit(handle);
}

uint32_t Config::GetConfigData(const char* tag)
{
    int index = configDataGetIndexOfTag(tag);

    if(index != -1)
    {
        switch(dataMap[index].size)
        {
            case 1:
                return *static_cast<uint8_t*>(dataMap[index].data);

            case 2:
                return *static_cast<uint16_t*>(dataMap[index].data);

            case 4:
                return *static_cast<uint32_t*>(dataMap[index].data);

            default:
                return 0;
        }
    }
    else return 0;
}

void Config::GetConfigData(const char* tag, char** out)
{
    int index = configDataGetIndexOfTag(tag);

    if((index != -1) && !ConfigDataIsInt(tag))
        *out = static_cast<char*>(dataMap[index].data);
}

esp_err_t Config::InitStorage()
{
    esp_err_t err = ESP_OK;
    
    /* Init the NVS flash driver. */
    err = nvs_flash_init();
    if(err != ESP_OK)
        return err;

    /* Now get a handle to the NVS. */
    err = nvs_open("KeyStorage", NVS_READWRITE, &handle);

#if defined(CONFIG_CONFIG_ERASEALLONBOOT)
    EraseAll();
#endif

    return err;
}

esp_err_t Config::Load()
{
    esp_err_t err = ESP_OK;
    size_t size = 0;
    union
    {
        uint8_t u8;
        uint16_t u16;
        uint32_t u32;
    } temp;
    
    /* Get the existence number. Do we exist? */
    nvs_get_u32(handle, "existence", &temp.u32);
    if(temp.u32 != existenceNum)
        return ESP_FAIL;

    /* If we do, grab all the config data. */
    for(int i = 0; i < dataMap.size(); i++)
    {
        switch(dataMap[i].size)
        {
            case 1:
                err = nvs_get_u8(handle, dataMap[i].tag, static_cast<uint8_t*>(dataMap[i].data));
                break;

            case 2:
                err = nvs_get_u16(handle, dataMap[i].tag, static_cast<uint16_t*>(dataMap[i].data));
                break;

            case 4:
                err = nvs_get_u32(handle, dataMap[i].tag, static_cast<uint32_t*>(dataMap[i].data));
                break;

            default:
                size = dataMap[i].size;
                err = nvs_get_str(handle, dataMap[i].tag, static_cast<char*>(dataMap[i].data), &size);
                break;
        }

        if(err != ESP_OK)
        {
            ESP_LOGE(__func__, "Could not load config data \"%s\" (%d)!", dataMap[i].tag, err);
            break;
        }
    }

    return err;
}

bool Config::Save()
{
    nvs_set_u32(handle, "existence", existenceNum);

    for(int i = 0; i < dataMap.size(); i++)
    {
        switch(dataMap[i].size)
        {
            case 1:
                nvs_set_u8(handle, dataMap[i].tag, *static_cast<uint8_t*>(dataMap[i].data));
                break;

            case 2:
                nvs_set_u16(handle, dataMap[i].tag, *static_cast<uint16_t*>(dataMap[i].data));
                break;

            case 4:
                nvs_set_u32(handle, dataMap[i].tag, *static_cast<uint32_t*>(dataMap[i].data));
                break;

            default:
                nvs_set_str(handle, dataMap[i].tag, static_cast<char*>(dataMap[i].data));
                break;
        }
    }
    nvs_commit(handle);

    ESP_LOGW(__func__, "Config saved%s.", shouldRestart ? ", will require restart to take effect" : "");

    /* Return whether a restart is required for changes to take effect. */
    return shouldRestart;
}

void Config::SetConfigData(const char* tag, uint32_t value)
{
    int index = configDataGetIndexOfTag(tag);

    if(index != -1)
    {
        if(dataMap[index].willTriggerRestart)
            shouldRestart = true;
        switch(dataMap[index].size)
        {
            case 1:
                *static_cast<uint8_t*>(dataMap[index].data) = value;
                return;

            case 2:
                *static_cast<uint16_t*>(dataMap[index].data) = value;
                return;

            case 4:
                *static_cast<uint32_t*>(dataMap[index].data) = value;
                return;

            default:
                return;
        }
    }
}

void Config::SetConfigData(const char* tag, char* value)
{
    int index = configDataGetIndexOfTag(tag);

    if((index != -1) && !ConfigDataIsInt(tag))
    {
        if(dataMap[index].willTriggerRestart)
            shouldRestart = true;
        strncpy(static_cast<char*>(dataMap[index].data), value, dataMap[index].size);
    }
}