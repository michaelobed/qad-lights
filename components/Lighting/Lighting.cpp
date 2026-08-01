//
//  Lighting.cpp
//  qad-lights
//
//  Created by michaelobed on 29/07/2026.
//  
//  Copyright © 2026 Michael Obed.

#include "Config.hpp"
#include <cstring>
#include "esp_log.h"
#include "Lighting.hpp"

static Config& config = Config::GetInstance();

Lighting::Lighting()
{
    memset(colour, 0xff, 3);
    fadeTimeMsChange = 100;
    fadeTimeMsOn = 500;
    fadeTimeMsOff = 250;
    on = false;
}

void Lighting::doChange(uint8_t r, uint8_t g, uint8_t b, int fadeTime, bool fade)
{
    if(fade)
    {
        ledc_set_fade_with_time(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0, r, fadeTime);
        ledc_set_fade_with_time(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_1, g, fadeTime);
        ledc_set_fade_with_time(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_2, b, fadeTime);

        ledc_fade_start(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0, LEDC_FADE_NO_WAIT);
        ledc_fade_start(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_1, LEDC_FADE_NO_WAIT);
        ledc_fade_start(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_2, LEDC_FADE_NO_WAIT);
    }
    else
    {
        ledc_set_duty_and_update(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0, r, 0);
        ledc_set_duty_and_update(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_1, g, 0);
        ledc_set_duty_and_update(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_2, b, 0);
    }
}

esp_err_t Lighting::Init()
{
    ledc_channel_config_t configChannel =
    {
        .gpio_num = -1,                         /* Don't care - we'll change this later. */
        .speed_mode = LEDC_HIGH_SPEED_MODE,
        .channel = LEDC_CHANNEL_MAX,            /* Ditto. */
        .intr_type = LEDC_INTR_FADE_END,
        .timer_sel = LEDC_TIMER_0,
        .duty = 0,
        .hpoint = 0,
        .sleep_mode = LEDC_SLEEP_MODE_NO_ALIVE_NO_PD,
        .flags = 0,
        .deconfigure = false
    };

    ledc_timer_config_t configTimer =
    {
        .speed_mode = LEDC_HIGH_SPEED_MODE,
        .duty_resolution = LEDC_TIMER_8_BIT,
        .timer_num = LEDC_TIMER_0,
        .freq_hz = 100000,
        .clk_cfg = LEDC_AUTO_CLK,
        .deconfigure = false
    };
    esp_err_t err = ESP_OK;

    /* Configure timer, then configure each channel. */
    err = ledc_timer_config(&configTimer);
    if(err != ESP_OK)
    {
        ESP_LOGE(__func__, "Could not configure lighting timer (%d)!", err);
        return err;
    }

    /* Configure each channel. */
    configChannel.gpio_num = pinR;
    configChannel.channel = LEDC_CHANNEL_0;
    err = ledc_channel_config(&configChannel);

    configChannel.gpio_num = pinG;
    configChannel.channel = LEDC_CHANNEL_1;
    err |= ledc_channel_config(&configChannel);

    configChannel.gpio_num = pinB;
    configChannel.channel = LEDC_CHANNEL_2;
    err |= ledc_channel_config(&configChannel);

    if(err != ESP_OK)
    {
        ESP_LOGE(__func__, "Could not configure one or more lighting channels (%d)!", err);
        return err;
    }

    /* Install fade service. */
    err = ledc_fade_func_install(ESP_INTR_FLAG_LOWMED);
    if(err != ESP_OK)
    {
        ESP_LOGE(__func__, "Could not install fade service (%d)!", err);
        return err;
    }

    SetColour(config.LightingColour);

    /* If permanently on, deal with that now. */
    if(config.LightingMode == Config::LEDMode_On)
        On();
    
    return err;
}

void Lighting::Off()
{
    ESP_LOGI(__func__, "Turning lights off...");
    on = false;
    doChange(0x00, 0x00, 0x00, fadeTimeMsOff, true);
}

void Lighting::On()
{
    ESP_LOGI(__func__, "Turning lights on...");
    on = true;
    doChange(colour[0], colour[1], colour[2], fadeTimeMsOn, true);
}

void Lighting::SetColour(uint32_t rgb, bool fade)
{
    colour[0] = (rgb >> 16) & 0xff;
    colour[1] = (rgb >> 8) & 0xff;;
    colour[2] = rgb & 0xff;

    if(on)
        doChange(colour[0], colour[1], colour[2], fadeTimeMsChange, fade);
}