//
//  Sleep.cpp
//  qad-lights
//
//  Created by michaelobed on 18/08/2026.
//  
//  Copyright © 2026 Michael Obed.

#include "Config.hpp"
#include "esp_log.h"
#include "esp_sleep.h"
#include "Lighting.hpp"
#include "Network.hpp"
#include "Sleep.hpp"

static Config& config = Config::GetInstance();
static Lighting& lighting = Lighting::GetInstance();
static Network& network = Network::GetInstance();

static void onHoldoffTimer(void* arg);

Sleep::Sleep()
{
    esp_timer_create_args_t createArgs =
    {
        .callback = onHoldoffTimer,
        .arg = nullptr,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "holdoffTimer",
        .skip_unhandled_events = false
    };

    HoldoffExpired = false;
    timerWasDefault = false;

    if(esp_timer_init() != ESP_OK)
        ESP_LOGE(__func__, "Could not initialise ESP Timer! Sleep will not work properly.");
    esp_timer_create(&createArgs, &holdoffTimer);
}

void Sleep::TimerStart()
{
    uint64_t holdoffTimerPeriod = 0;
    Config::SleepMode sleepMode = static_cast<Config::SleepMode>(config.GetConfigData("sleepMode"));

    if(esp_timer_is_active(holdoffTimer))
    {
        ESP_LOGI(__func__, "TimerStart() called while holdoff timer is active. Not doing anything.");
        return;
    }
    
    switch(sleepMode)
    {
        /* We don't use the period in this mode. */
        case Config::SleepMode_WhenLEDOff:
            break;

        case Config::SleepMode_WhenLEDOff10:
            holdoffTimerPeriod = 10000000UL;
            break;

        case Config::SleepMode_WhenLEDOff30:
            holdoffTimerPeriod = 30000000UL;
            break;

        case Config::SleepMode_WhenLEDOff60:
            holdoffTimerPeriod = 60000000UL;
            break;

        /* Invalid! */
        default:
            return;
    }

    timerWasDefault = false;
    ESP_LOGW(__func__, "Will sleep in %llu seconds.", holdoffTimerPeriod / 1000000UL);
    if(sleepMode == Config::SleepMode_WhenLEDOff)
        HoldoffExpired = true;
    else esp_timer_start_once(holdoffTimer, holdoffTimerPeriod);
}

void Sleep::TimerStartDefault()
{
    /* Initialise holdoff timer for 60 seconds. */
    timerWasDefault = true;
    esp_timer_start_once(holdoffTimer, holdoffTimerDefault * 1000000UL);
    ESP_LOGW(__func__, "Default holdoff timer started.");
    if(config.GetConfigData("sleepMode") != Config::SleepMode_Never)
        ESP_LOGW(__func__, "Will sleep in %d seconds.", holdoffTimerDefault);
}

bool Sleep::TimerStartDefaultIsActive()
{
    return (esp_timer_is_active(holdoffTimer) && timerWasDefault);
}

void Sleep::TimerStop()
{
    if(esp_timer_is_active(holdoffTimer))
        esp_timer_stop(holdoffTimer);
}

void Sleep::doSleep()
{
    ESP_LOGW(__func__, "Sleeping now... -.-");
    if(network.IsRunning())
        network.DeInit();
    lighting.WaitForLEDsOff();
    esp_light_sleep_start();
    ESP_LOGW(__func__, "I'm awake! ^_^");
}

void Sleep::Update()
{
    if(!esp_timer_is_active(holdoffTimer))
    {
        /* We use the HoldoffExpired flag to differentiate between having no timer because we're
         * never sleeping from having no timer because it has just elapsed. */
        if(HoldoffExpired)
            doSleep();
        HoldoffExpired = false;
    }
}

void onHoldoffTimer(void* arg)
{
    Sleep& sleep = Sleep::GetInstance();

    ESP_LOGW(__func__, "Holdoff expired!");
    if(config.GetConfigData("sleepMode") != Config::SleepMode_Never)
        sleep.HoldoffExpired = true;
}