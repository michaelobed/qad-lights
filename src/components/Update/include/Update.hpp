//
//  Update.hpp
//  qad-lights
//
//  Created by michaelobed on 20/08/2026.
//  
//  Copyright © 2026 Michael Obed.

#ifndef Update_hpp
#define Update_hpp

class Update
{
    public:
        static Update& GetInstance()
        {
            static Update u;
            return u;
        }

        esp_err_t Check();
};

#endif