#ifndef KEYPADMANAGER_H
#define KEYPADMANAGER_H

#include <Arduino.h>

class KeypadManager {
private:
    unsigned long lastKeyPressTime;
    
public:
    KeypadManager();
    ~KeypadManager();
    
    void begin();
    char getKey(); 
};

#endif
