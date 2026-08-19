//
//  Sleep.hpp
//  qad-lights
//
//  Created by michaelobed on 18/08/2026.
//  
//  Copyright © 2026 Michael Obed.

#ifndef Sleep_hpp
#define Sleep_hpp

#include "esp_timer.h"

class Sleep
{
    public:
        Sleep();

        static Sleep& GetInstance()
        {
            static Sleep s;
            return s;
        }

        bool HoldoffExpired;
        
        void TimerStart();
        void TimerStartDefault();
        bool TimerStartDefaultIsActive();
        void TimerStop();
        void Update();
        
    private:
        esp_timer_handle_t holdoffTimer;
        static constexpr int holdoffTimerDefault = 60;
        bool timerWasDefault;

        void doSleep();
};

#endif