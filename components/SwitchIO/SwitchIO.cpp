//
//  SwitchIO.cpp
//  qad-lights
//
//  Created by michaelobed on 01/08/2026.
//  
//  Copyright © 2026 Michael Obed.

#include "Config.hpp"
#include <cstring>
#include "esp_log.h"
#include "SwitchIO.hpp"

static Config& config = Config::GetInstance();

SwitchIO::SwitchIO()
{
    isrHandle = nullptr;
    NumSwitches = 0;
    memset(switches, GPIO_NUM_NC, switchesMaxLen * sizeof(gpio_num_t));

    /* TODO: Get Config involved. For now, use these test pins. */
    NumSwitches = 2;
    switches[0] = GPIO_NUM_18;
    switches[1] = GPIO_NUM_19;
}

esp_err_t SwitchIO::Configure()
{
    gpio_config_t config =
    {
        .pin_bit_mask = 0,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_ENABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    esp_err_t err = ESP_OK;

    for(int i = 0; i < NumSwitches; i++)
        config.pin_bit_mask |= (0x01 << switches[i]);

    err = gpio_config(&config);
    if(err != ESP_OK)
        ESP_LOGE(__func__, "Could not configure GPIO for switches (%d)!", err);

    return err;
}

bool SwitchIO::Update()
{
    bool allOn = true;
    bool anyOn = false;
    bool isNc = (config.GetConfigData("switchPolarity") == Config::SwPol_NormallyClosed);
    int_least32_t lightingMode = config.GetConfigData("lightingMode");
    bool state = false;

    if(lightingMode == Config::LEDMode_Off)
        return false;
    else if(lightingMode == Config::LEDMode_On)
        return true;

    /* We're here because the config depends on switch states, so read them and perform logic. */
    for(int i = 0; i < NumSwitches; i++)
    {
        state = (gpio_get_level(switches[i]) == 1);
        state ^= isNc;

        allOn &= state;
        anyOn |= state;
    }

    /* Return what the effective state of the lights should be. */
    if(lightingMode == Config::LEDMode_OnAny)
        return anyOn;
    else return allOn;
}