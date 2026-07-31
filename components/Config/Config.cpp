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
    LightingColour = 0xffffff;
    LightingMode = LEDMode_On;
    strncpy(Name, "My QAD Lights", NameMaxLen);
    NetworkIsSTA = false;
    strncpy(NetworkSsid, "YourNetworkHere", MAX_SSID_LEN);
    memset(NetworkPsk, 0, MAX_PASSPHRASE_LEN);
}

void Config::EraseAll()
{
    nvs_erase_all(handle);
    nvs_commit(handle);
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
    for(int i = 0; i < dataMaxLen; i++)
    {
        switch(data[i].size)
        {
            case 1:
                err = nvs_get_u8(handle, data[i].tag, static_cast<uint8_t*>(data[i].data));
                break;

            case 2:
                err = nvs_get_u16(handle, data[i].tag, static_cast<uint16_t*>(data[i].data));
                break;

            case 4:
                err = nvs_get_u32(handle, data[i].tag, static_cast<uint32_t*>(data[i].data));
                break;

            default:
                size = data[i].size;
                err = nvs_get_str(handle, data[i].tag, static_cast<char*>(data[i].data), &size);
                break;
        }

        if(err != ESP_OK)
        {
            ESP_LOGE(__func__, "Could not load config data \"%s\" (%d)!", data[i].tag, err);
            break;
        }
    }

    return err;
}

void Config::Save()
{
    nvs_set_u32(handle, "existence", existenceNum);

    for(int i = 0; i < dataMaxLen; i++)
    {
        switch(data[i].size)
        {
            case 1:
                nvs_set_u8(handle, data[i].tag, *static_cast<uint8_t*>(data[i].data));
                break;

            case 2:
                nvs_set_u16(handle, data[i].tag, *static_cast<uint16_t*>(data[i].data));
                break;

            case 4:
                nvs_set_u32(handle, data[i].tag, *static_cast<uint32_t*>(data[i].data));
                break;

            default:
                nvs_set_str(handle, data[i].tag, static_cast<char*>(data[i].data));
                break;
        }
    }

    nvs_commit(handle);
}