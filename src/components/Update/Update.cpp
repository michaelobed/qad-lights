//
//  Update.cpp
//  qad-lights
//
//  Created by michaelobed on 20/08/2026.
//  
//  Copyright © 2026 Michael Obed.

#include "esp_log.h"
#include "Update.hpp"

Update::Update()
{
    configured = esp_ota_get_boot_partition();
    handle = 0;
    running = esp_ota_get_running_partition();
    update = esp_ota_get_next_update_partition(nullptr);
}

esp_err_t Update::Check()
{
    /* Check we're where we meant to be on boot. */
    if(configured != running)
        ESP_LOGW(__func__, "Incorrect boot partition detected! You should probably reflash this device.");

    return ESP_OK;
}

esp_err_t Update::WriteContinue(const uint8_t* data, size_t size)
{
    esp_err_t err = ESP_OK;
    
    /* This should only ever be called after a WriteStart(). */
    err = esp_ota_write(handle, data, size);
    if(err != ESP_OK)
    {
        esp_ota_abort(handle);
        return err;
    }

    return err;
}

esp_err_t Update::WriteEnd()
{
    /* We wrote all the data, so end the write and setup the new partition for next boot. */
    esp_err_t err = esp_ota_end(handle);
    if(err != ESP_OK)
    {
        ESP_LOGE(__func__, "Bad OTA image!");
        return err;
    }
    esp_ota_set_boot_partition(update);
    ESP_LOGI(__func__, "OTA download complete.");
    return err;
}

esp_err_t Update::WriteStart(const uint8_t* data, size_t size, size_t imageSize, bool& shouldContinue)
{
    esp_app_desc_t appInfoCurrent;
    esp_app_desc_t appInfoNew;
    esp_err_t err = ESP_OK;

    /* Compare the version and SHA256 of the current and new images. If they match, don't continue. */
    esp_ota_get_partition_description(running, &appInfoCurrent);
    memcpy(&appInfoNew, data + sizeof(esp_image_header_t) + sizeof(esp_image_segment_header_t), sizeof(esp_app_desc_t));
    if( (memcmp(appInfoCurrent.version, appInfoNew.version, 32) != 0) ||
        (memcmp(appInfoCurrent.app_elf_sha256, appInfoNew.app_elf_sha256, 32) != 0))
    {
        /* Obtain an OTA handle and begin the write. */
        ESP_LOGI(__func__, "Writing to OTA partition starting at address 0x%08x...", update->address);
        err = esp_ota_begin(update, imageSize, &handle);
        if(err != ESP_OK)
            return err;

        err = WriteContinue(data, size);
        shouldContinue = true;
    }
    else
    {
        ESP_LOGI(__func__, "New image matches current. Not updating OTA image.");
        shouldContinue = false;
    }

    return err;
}