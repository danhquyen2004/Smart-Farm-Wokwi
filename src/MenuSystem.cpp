#include "MenuSystem.h"

MenuSystem::MenuSystem(Adafruit_SSD1306* disp) {
    display = disp;
    currentState = LOGIN;
    inputPassword = "";
}

void MenuSystem::begin(PlantZone* z1, PlantZone* z2) {
    zone1 = z1;
    zone2 = z2;
    drawLogin();
}

bool MenuSystem::isLoggedIn() {
    return currentState != LOGIN;
}

void MenuSystem::update() {
    // This method can be used for webapp polling or other updates
    // Keypad input removed - now controlled via webapp
}

void MenuSystem::refresh() {
    if (currentState == LOGIN) drawLogin();
    else if (currentState == DASHBOARD) drawDashboard();
}

void MenuSystem::drawLogin() {
    display->clearDisplay();
    display->setTextColor(SSD1306_WHITE);
    
    // Header
    display->setTextSize(1);
    display->setCursor(0, 0);
    display->println(F("SMART FARM v2.3"));
    display->drawLine(0, 10, 128, 10, SSD1306_WHITE);
    
    // Debug Last Key
    display->setCursor(90, 0);
    display->print(F("["));
    display->print(lastDebugKey);
    display->print(F("]"));
    
    // Prompt
    display->setCursor(10, 20);
    display->println(F("Enter Password:"));
    
    // Input Box
    display->drawRect(14, 35, 100, 15, SSD1306_WHITE);
    display->setCursor(20, 39);
    
    String masked = "";
    for (unsigned int i=0; i<inputPassword.length(); i++) masked += "*";
    display->print(masked);
    
    // Footer
    display->setTextSize(1);
    display->setCursor(10, 55);
    display->print(F("Login via WebApp"));
    
    display->display();
}

void MenuSystem::drawDashboard() {
    display->clearDisplay();
    display->setTextColor(SSD1306_WHITE);
    
    // Header
    display->setTextSize(1);
    display->setCursor(0, 0);
    display->print(F("DASHBOARD   "));
    
    if (selectedZone == 1) display->print(F("[Z1]"));
    else display->print(F("[Z2]"));
    
    display->drawLine(0, 10, 128, 10, SSD1306_WHITE);
    
    // Zone Info
    PlantZone* z = (selectedZone == 1) ? zone1 : zone2;
    
    display->setCursor(0, 15);
    display->print(F("Mode: "));
    display->println(z->getAutoMode() ? "AUTO" : "MANUAL");
    
    display->print(F("Cay: "));
    // Cat ngan ten cay cho vua
    PlantProfile* p = z->getProfile();
    String tenCay = (p != nullptr) ? String(p->ten) : "N/A";
    display->println(tenCay.substring(0, 10));
    
    // Sensor Data (Simplified)
    display->drawLine(0, 35, 128, 35, SSD1306_WHITE);
    display->setCursor(0, 40);
    
    display->print(F("Status: "));
    display->println(z->isAlarmActive() ? "ALARM!" : "OK");
    
    // Footer
    display->setCursor(0, 56);
    display->print(F("Control via WebApp"));
    
    display->display();
}

void MenuSystem::handleInput(char key) {
    lastDebugKey = key; // Store for debug
    
    if (currentState == LOGIN) {
        if (key >= '0' && key <= '9') {
            if (inputPassword.length() < 6) {
                inputPassword += key;
            }
        } else if (key == 'C') { // Clear
            inputPassword = "";
        } else if (key == 'D') { // Enter
            if (inputPassword == correctPassword) {
                currentState = DASHBOARD;
                Serial.println("Login Successful");
            } else {
                display->clearDisplay();
                display->setCursor(10, 25);
                display->print(F("WRONG PASS!"));
                display->display();
                delay(1000);
                inputPassword = "";
            }
        }
        
        if (currentState == LOGIN) drawLogin();
        else drawDashboard();
        
    } else if (currentState == DASHBOARD) {
        if (key == 'A') {
            selectedZone = 1;
        } else if (key == 'B') {
            selectedZone = 2;
        } else if (key == 'D') {
            // Toggle Auto/Manual for selected
            PlantZone* z = (selectedZone == 1) ? zone1 : zone2;
            z->setAutoMode(!z->getAutoMode());
        }
        drawDashboard();
    }
}
