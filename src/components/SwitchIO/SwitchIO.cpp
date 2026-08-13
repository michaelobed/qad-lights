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
#include "esp_sleep.h"
#include "SwitchIO.hpp"

static Config& config = Config::GetInstance();

SwitchIO::SwitchIO()
{
    configGpio.pin_bit_mask = 0;
    configGpio.mode = GPIO_MODE_INPUT;
    configGpio.pull_up_en = GPIO_PULLUP_DISABLE;
    configGpio.pull_down_en = GPIO_PULLDOWN_ENABLE;
    configGpio.intr_type = GPIO_INTR_DISABLE;
    isrHandle = nullptr;

    /* TODO: Get Config involved. For now, use these test pins. */
    switches.push_back(GPIO_NUM_2);
    switches.push_back(GPIO_NUM_4);
}

esp_err_t SwitchIO::Configure()
{
    esp_err_t err = ESP_OK;
    bool isNc = (config.GetConfigData("switchPolarity") == Config::SwPol_NormallyClosed);

    for(gpio_num_t sw : switches)
        configGpio.pin_bit_mask |= (0x01 << sw);

    err = gpio_config(&configGpio);
    if(err != ESP_OK)
    {
        ESP_LOGE(__func__, "Could not configure GPIO for switches (%d)!", err);
        return err;
    }

    /* Set up the switches as sleep wakeup sources. */
    for(gpio_num_t sw : switches)
        gpio_wakeup_enable(sw, isNc ? GPIO_INTR_LOW_LEVEL : GPIO_INTR_HIGH_LEVEL);
    esp_sleep_enable_gpio_wakeup();

    /* Set up everything else as a pulled-down input. */
    for(gpio_num_t io : validPins)
    {
        if(!isConfigured(io) && !isNonPullDown(io))
        {
            gpio_set_direction(io, GPIO_MODE_INPUT);
            gpio_pulldown_en(io);
        }
    }

    return err;
}

bool SwitchIO::isConfigured(gpio_num_t pin)
{
    for(gpio_num_t sw : switches)
    {
        if(pin == sw)
            return true;
    }

    return false;
}

bool SwitchIO::isNonPullDown(gpio_num_t pin)
{
    for(gpio_num_t npdp : nonPullDownPins)
    {
        if(pin == npdp)
            return true;
    }

    return false;
}

bool SwitchIO::Update()
{
    bool allOn = true;
    bool anyOn = false;
    bool isNc = (config.GetConfigData("switchPolarity") == Config::SwPol_NormallyClosed);
    int lightingMode = config.GetConfigData("lightingMode");
    bool state = false;

    if(lightingMode == Config::LightingMode_Off)
        return false;
    else if(lightingMode == Config::LightingMode_On)
        return true;

    /* We're here because the config depends on switch states, so read them and perform logic. */
    for(gpio_num_t sw : switches)
    {
        state = (gpio_get_level(sw) == 1);
        state ^= isNc;

        allOn &= state;
        anyOn |= state;
    }

    /* Return what the effective state of the lights should be. */
    if(lightingMode == Config::LightingMode_OnAny)
        return anyOn;
    else return allOn;
}