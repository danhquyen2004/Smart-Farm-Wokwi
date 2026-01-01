#ifndef MENUSYSTEM_H
#define MENUSYSTEM_H

#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>
#include "KeypadManager.h"
#include "PlantZone.h"

// Define states
enum MenuState {
    LOGIN,
    DASHBOARD,
    ZONE_Select,
    SETTINGS
};

class MenuSystem {
private:
    Adafruit_SSD1306* display;
    KeypadManager* keypad;
    PlantZone* zone1;
    PlantZone* zone2;
    MenuState currentState;
    
    // Login vars
    String inputPassword;
    String correctPassword = "1234";
    char lastDebugKey = ' '; // Variable to store last key for OLED debug
    
    // Dashboard vars
    int selectedZone = 1; // 1 or 2
    
    // Settings vars
    int settingIndex = 0;
    
public:
    MenuSystem(Adafruit_SSD1306* display, KeypadManager* keypad);
    void begin(PlantZone* z1, PlantZone* z2);
    void update();
    void drawLogin();
    void drawDashboard();
    void handleInput(char key);
    bool isLoggedIn();
    void refresh();
};

#endif
