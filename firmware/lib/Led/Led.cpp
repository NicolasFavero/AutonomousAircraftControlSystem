#include "Led.h"


Led::Led(int ledPin_): ledPin(ledPin_){}


void Led::red(){setColor(50, 0, 0);}
void Led::green(){setColor(0, 50, 0);}
void Led::blue(){setColor(0, 0, 50);}
void Led::white(){setColor(50, 50, 50);}
void Led::on(){setColor(50, 50, 50);}
void Led::off(){setColor(0, 0, 0);}

void Led::setColor(int r, int g, int b){rgbLedWrite(ledPin, r, g, b);}
