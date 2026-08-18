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
#include <vector>

class SwitchIO
{
    public:
        SwitchIO();

        static SwitchIO& GetInstance()
        {
            static SwitchIO swio;
            return swio;
        }

        esp_err_t Configure();
        void ConfigurePolarity(bool isNc);
        bool Update();

    private:
        gpio_config_t configGpio;
        gpio_isr_handle_t isrHandle;
        static constexpr gpio_num_t nonPullDownPins[] =
        {
            GPIO_NUM_34,
            GPIO_NUM_35,
            GPIO_NUM_36,
            GPIO_NUM_39
        };
        std::vector<gpio_num_t> switches;
        static constexpr gpio_num_t validPins[] =
        {
            /* A word on forbidden pins:
             *
             * GPIO0 is not allowed as it's also BOOT, and ** will ** cause problems at power-on.
             * GPIO5 is not allowed as it's also SDIO.
             * GPIO6 to GPIO11 are not allowed as they're shared with the flash.
             * GPIO12 is not allowed as it's also VDD_FLASH.
             * GPIO15 is not allowed as it's the LOG bootstrapping pin, but I don't know what it does.
             * GPIO25 to GPIO27 are not allowed as they're the LED PWM output pins.
             * GPIO34, GPIO35, GPIO36 and GPIO39 are allowed, but must be pulled down externally whether used or not. */

            GPIO_NUM_2,
            GPIO_NUM_4,
            GPIO_NUM_13,
            GPIO_NUM_14,
            GPIO_NUM_16,
            GPIO_NUM_17,
            GPIO_NUM_18,
            GPIO_NUM_19,
            GPIO_NUM_21,
            GPIO_NUM_22,
            GPIO_NUM_23,
            GPIO_NUM_32,
            GPIO_NUM_33,
            GPIO_NUM_34,
            GPIO_NUM_35,
            GPIO_NUM_36,
            GPIO_NUM_39
        };

        bool isConfigured(gpio_num_t pin);
        bool isNonPullDown(gpio_num_t pin);
};

#endif