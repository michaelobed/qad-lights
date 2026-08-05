//
//  SwitchIO.hpp
//  qad-lights
//
//  Created by michaelobed on 01/08/2026.
//  
//  Copyright © 2026 Michael Obed.

#ifndef SwitchIO_hpp
#define SwitchIO_hpp

#include "driver/gpio.h"

class SwitchIO
{
    public:
        SwitchIO();

        static SwitchIO& GetInstance()
        {
            static SwitchIO swio;
            return swio;
        }

        int NumSwitches;

        esp_err_t Configure();
        bool Update();

    private:
        gpio_isr_handle_t isrHandle;
        static constexpr int switchesMaxLen = 8;
        gpio_num_t switches[switchesMaxLen];
};

#endif