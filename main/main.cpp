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
#include "freertos/task.h"

static Config& config = Config::GetInstance();
static Http& http = Http::GetInstance();
static Lighting& lighting = Lighting::GetInstance();
static Network& network = Network::GetInstance();

static void errorHandler();

extern "C" void app_main()
{
    esp_err_t err = ESP_OK;

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
        ESP_LOGW(__func__, "Config did not exist. Saving afresh...");
        config.Save();
    }
    else ESP_LOGI(__func__, "Config loaded successfully.");

    /* Initialise the lighting driver. */
    err = lighting.Init();
    if(err != ESP_OK)
    {
        ESP_LOGE(__func__, "Could not init lighting driver (%d)!", err);
        errorHandler();
    }

    /* We have all the information we need. Start the WiFi! */
    if(config.NetworkIsSTA)
    {
        /* TODO: WiFi STA config. For now, panic. */
        ESP_LOGE(__func__, "WiFi STA not yet supported!");
        errorHandler();
    }
    else
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