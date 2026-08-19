//
//  main.cpp
//  qad-lights
//
//  Created by michaelobed on 29/07/2026.
//  
//  Copyright © 2026 Michael Obed.

#include "Config.hpp"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "Http.hpp"
#include "Lighting.hpp"
#include "Network.hpp"
#include "Sleep.hpp"
#include "SwitchIO.hpp"
#include "freertos/task.h"

static Config& config = Config::GetInstance();
static Http& http = Http::GetInstance();
static Lighting& lighting = Lighting::GetInstance();
static volatile bool lightingState = false;
static volatile bool lightingStateLast = false;
static Network& network = Network::GetInstance();
static Sleep& sleep = Sleep::GetInstance();
static SwitchIO& switchIo = SwitchIO::GetInstance();
static TaskHandle_t switchIOTaskHandle = nullptr;

static void errorHandler();
static void periodicTask(void* arg);

extern "C" void app_main()
{
    esp_err_t err = ESP_OK;
    bool isSTA = false;

    /* Initialise a default event loop. */
    err = esp_event_loop_create_default();
    if(err != ESP_OK)
    {
        /* We couldn't initialise the default event loop. This is even worse. Freak out! */
        ESP_LOGE(__func__, "Could not init event loop (%d)!", err);
        errorHandler();
    }

    /* Initialise NVS storage. */
    err = config.InitStorage();
    if(err != ESP_OK)
    {
        ESP_LOGE(__func__, "Could not init config storage (%d)!", err);
        errorHandler();
    }

    /* Load the config (or create one if it doesn't exist). */
    err = config.Load();
    if(err != ESP_OK)
    {
        ESP_LOGW(__func__, "Config did not exist or was corrupted. Saving afresh...");
        config.Save();
    }
    else ESP_LOGI(__func__, "Config loaded successfully.");

    /* Initialise switches and create background task. */
    err = switchIo.Configure();
    if(err != ESP_OK)
    {
        ESP_LOGE(__func__, "Could not configure switches (%d)!", err);
        errorHandler();
    }
    xTaskCreate(periodicTask, "SwitchIO", 2048, nullptr, tskIDLE_PRIORITY, &switchIOTaskHandle);

    /* Initialise the lighting driver. */
    err = lighting.Init();
    if(err != ESP_OK)
    {
        ESP_LOGE(__func__, "Could not init lighting driver (%d)!", err);
        errorHandler();
    }

    /* Start the WiFi! */
    isSTA = (config.GetConfigData("networkIsSTA") == 1);
    if(isSTA)
    {
        err = network.InitSTA();
        if(err != ESP_OK)
        {
            ESP_LOGW(__func__, "Could not start WiFi connection to network (%d)! Reverting to access point mode.", err);
            isSTA = false;
        }
        else ESP_LOGI(__func__, "WiFi connection to network started successfully.");
    }

    if(!isSTA)
    {
        err = network.InitAP();
        if(err != ESP_OK)
        {
            ESP_LOGE(__func__, "Could not start WiFi access point (%d)!", err);
            errorHandler();
        }
        else ESP_LOGI(__func__, "WiFi access point started.");
    }

    /* Get HTTP server going. */
    err = http.Init();
    if(err != ESP_OK)
    {
        ESP_LOGE(__func__, "Could not start HTTP server (%d)!", err);
        errorHandler();
    }
    else ESP_LOGI(__func__, "HTTP server started.");

    /* Start a default sleep timer. This won't take effect if we're configured to never sleep. */
    sleep.TimerStartDefault();

    ESP_LOGI(__func__, "Init done! Running main loop...");

    while(true)
    {
        /* Delay so main loop doesn't choke for watchdog reasons. */
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}

void errorHandler()
{
    /* Do nothing forever. */
    while(true);
}

void Restart()
{
    constexpr uint32_t delayUs = 3000000UL;

    esp_rom_delay_us(delayUs);
    ESP_LOGW(__func__, "*** RESET!!! ***");
    vTaskDelete(switchIOTaskHandle);
    lighting.DeInit();
    if(network.IsRunning())
        network.DeInit();

    esp_restart();
}

void periodicTask(void* arg)
{
    bool lightingStateChanged = false;

    /* Fire every 100ms. */
    while(true)
    {
        /* Handle changes in lighting state based on switch state. */
        lightingState = switchIo.Update();
        lightingStateChanged = (lightingState != lightingStateLast);
        if(lightingStateChanged)
        {
            ESP_LOGI(__func__, "Lighting change detected!");
            lighting.SetState(lightingState);
            lightingStateLast = lightingState;

            /* Start/stop the sleep timer according to the lighting state. */
            if(lightingState)
                sleep.TimerStop();
            else sleep.TimerStart();
        }

        /* Finally, check if we should be sleeping (and sleep if true). */
        sleep.Update();

        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}