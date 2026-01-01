#include "KeypadManager.h"

KeypadManager::KeypadManager() {
    lastKeyPressTime = 0;
}

KeypadManager::~KeypadManager() {
}

void KeypadManager::begin() {
    Serial.println("[INFO] KeypadManager initialized (No keypad connected)");
}

char KeypadManager::getKey() {
    // Keypad not connected - always return 0
    return 0;
}
