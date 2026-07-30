//
//  Lighting.cpp
//  qad-lights
//
//  Created by michaelobed on 29/07/2026.
//  
//  Copyright © 2026 Michael Obed.

#include <cstring>
#include "esp_log.h"
#include "Lighting.hpp"

Lighting::Lighting()
{
    memset(colour, 0xff, 3);
    fadeTimeMs = 500;
    fadeTimeMsOff = 250;
    on = false;
}

void Lighting::doChange(uint8_t r, uint8_t g, uint8_t b, int fadeTime)
{
    ledc_set_fade_with_time(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0, r, fadeTime);
    ledc_set_fade_with_time(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_1, g, fadeTime);
    ledc_set_fade_with_time(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_2, b, fadeTime);

    ledc_fade_start(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0, LEDC_FADE_NO_WAIT);
    ledc_fade_start(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_1, LEDC_FADE_NO_WAIT);
    ledc_fade_start(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_2, LEDC_FADE_NO_WAIT);
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
        .sleep_mode = LEDC_SLEEP_MODE_NO_ALIVE_NO_PD
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

    Off();
    return err;
}

void Lighting::Off()
{
    on = false;
    doChange(0x00, 0x00, 0x00, fadeTimeMsOff);
}

void Lighting::On()
{
    on = true;
    doChange(colour[0], colour[1], colour[2], fadeTimeMs);
}

void Lighting::SetColour(uint8_t r, uint8_t g, uint8_t b)
{
    colour[0] = r;
    colour[1] = g;
    colour[2] = b;

    if(on)
        doChange(r, g, b, fadeTimeMs);
}