//
//  Update.hpp
//  qad-lights
//
//  Created by michaelobed on 20/08/2026.
//  
//  Copyright © 2026 Michael Obed.

#ifndef Update_hpp
#define Update_hpp

#include "esp_app_format.h"
#include "esp_ota_ops.h"

class Update
{
    public:
        Update();

        static Update& GetInstance()
        {
            static Update u;
            return u;
        }

        static constexpr size_t SizeOfFirstChunk = sizeof(esp_image_header_t) + sizeof(esp_image_segment_header_t) + sizeof(esp_app_desc_t);

        esp_err_t Check();
        esp_err_t WriteContinue(const uint8_t* data, size_t size);
        esp_err_t WriteEnd();
        esp_err_t WriteStart(const uint8_t* data, size_t size, bool& shouldContinue);

    private:
        const esp_partition_t* configured;
        esp_ota_handle_t handle;
        const esp_partition_t* running;
        const esp_partition_t* update;
};

#endif