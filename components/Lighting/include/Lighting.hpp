//
//  Lighting.hpp
//  qad-lights
//
//  Created by michaelobed on 29/07/2026.
//  
//  Copyright © 2026 Michael Obed.

#ifndef Lighting_hpp
#define Lighting_hpp

#include <cstdint>
#include "driver/ledc.h"

class Lighting
{
    public:
        Lighting();

        static Lighting& GetInstance()
        {
            static Lighting l;
            return l;
        }

        esp_err_t Init();
        void Off();
        void On();
        void SetColour(uint32_t colour);    /* Stored as 0xXXRRGGBB. */

    private:
        uint8_t colour[3];                  /* 0 = r, 1 = g, 2 = b. */
        int fadeTimeMs;
        int fadeTimeMsOff;
        bool on;
        static constexpr int pinB = 4;
        static constexpr int pinG = 0;
        static constexpr int pinR = 2;

        void doChange(uint8_t r, uint8_t g, uint8_t b, int fadeTime);
};

#endif