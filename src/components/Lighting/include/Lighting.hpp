//
//  Lighting.hpp
//  qad-lights
//
//  Created by michaelobed on 29/07/2026.
//  
//  Copyright © 2026 Michael Obed.

#ifndef Lighting_hpp
#define Lighting_hpp

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

        void DeInit();
        esp_err_t Init();
        void SetColour(uint32_t colour, bool fade = true);      /* Stored as 0xXXRRGGBB. */
        void SetState(bool isOn);

    private:
        uint8_t colour[3];                                      /* 0 = r, 1 = g, 2 = b. */
        int fadeTimeMsChange;
        int fadeTimeMsOff;
        int fadeTimeMsOn;
        bool on;
        static constexpr int pinB = 27;
        static constexpr int pinG = 26;
        static constexpr int pinR = 25;

        void doChange(uint8_t r, uint8_t g, uint8_t b, int fadeTime, bool fade);
};

#endif