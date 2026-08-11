#pragma once

#include <Arduino.h>

class Led{

    public:
        Led(int ledPin_ = 48);

        void red();
        void green();
        void blue();
        void white();
        void on();
        void off();
 
    private:
        void setColor(int r = 0, int g = 0, int b = 0);
        int ledPin = 48;
};