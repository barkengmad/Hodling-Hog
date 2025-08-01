#include "display.h"
#include "../utils/utils.h"
#include <Arduino.h>

// Global instance
DisplayManager displayMgr;

DisplayManager::DisplayManager() 
    : display(GxEPD2_154_D67(EPD_CS, EPD_DC, EPD_RST, EPD_BUSY)) {
    currentScreen = ScreenType::SETUP_WELCOME;
    initialized = false;
    fastUpdateMode = false;
    displayBrightness = 128;
    
    // Device and status initialization
    deviceSetup = false;
    deviceName = "Hodling Hog";
    setupIP = "192.168.4.1";
    wifiConnected = false;
    
    // Initialize balance data
    balanceData.lightningBalance = 0;
    balanceData.coldBalance = 0;
    balanceData.totalBalance = 0;
    balanceData.lightningValid = false;
    balanceData.coldValid = false;
    balanceData.lastUpdate = 0;
}

void DisplayManager::init() {
    Serial.println("DisplayManager: Initializing e-paper display");
    
    // Initialize SPI and display
    display.init();
    display.setRotation(0);
    display.setFont(&FreeMonoBold9pt7b);
    display.setTextColor(GxEPD_BLACK);
    
    initialized = true;
    Serial.println("DisplayManager: Display initialized");
}

void DisplayManager::showScreen(ScreenType screen) {
    if (!initialized) return;
    
    currentScreen = screen;
    Serial.printf("DisplayManager: Showing screen %d\n", (int)screen);
    
    switch (screen) {
        case ScreenType::SETUP_WELCOME:
            drawSetupWelcomeScreen();
            break;
        case ScreenType::LIGHTNING_BALANCE:
            drawLightningBalanceScreen();
            break;
        case ScreenType::COLD_BALANCE:
            drawColdBalanceScreen();
            break;
        case ScreenType::TOTAL_BALANCE:
            drawTotalBalanceScreen();
            break;
        case ScreenType::ERROR_SCREEN:
            drawErrorScreen("System Error");
            break;
    }
}

void DisplayManager::updateBalances(const BalanceData& balances) {
    Serial.printf("DisplayManager: Updating balances - Lightning: %llu, Cold: %llu\n", 
                  balances.lightningBalance, balances.coldBalance);
    balanceData = balances;
    
    // Refresh current screen with new data
    showScreen(currentScreen);
}

void DisplayManager::updateQRData(const QRData& qrData) {
    Serial.println("DisplayManager: Updating QR data");
    this->qrData = qrData;
}

void DisplayManager::showErrorScreen(const String& error) {
    Serial.printf("DisplayManager: Showing error: %s\n", error.c_str());
    drawErrorScreen(error);
}

// Screen cycling for setup pages only
void DisplayManager::nextSetupScreen() {
    if (!deviceSetup) return; // Only cycle when setup is complete
    
    switch (currentScreen) {
        case ScreenType::LIGHTNING_BALANCE:
            showScreen(ScreenType::COLD_BALANCE);
            break;
        case ScreenType::COLD_BALANCE:
            showScreen(ScreenType::TOTAL_BALANCE);
            break;
        case ScreenType::TOTAL_BALANCE:
            showScreen(ScreenType::LIGHTNING_BALANCE);
            break;
        default:
            showScreen(ScreenType::LIGHTNING_BALANCE);
            break;
    }
}

void DisplayManager::previousSetupScreen() {
    if (!deviceSetup) return; // Only cycle when setup is complete
    
    switch (currentScreen) {
        case ScreenType::LIGHTNING_BALANCE:
            showScreen(ScreenType::TOTAL_BALANCE);
            break;
        case ScreenType::COLD_BALANCE:
            showScreen(ScreenType::LIGHTNING_BALANCE);
            break;
        case ScreenType::TOTAL_BALANCE:
            showScreen(ScreenType::COLD_BALANCE);
            break;
        default:
            showScreen(ScreenType::LIGHTNING_BALANCE);
            break;
    }
}





void DisplayManager::clear() {
    if (!initialized) return;
    display.clearScreen();
}

void DisplayManager::sleep() {
    Serial.println("DisplayManager: Entering sleep mode");
    if (initialized) {
        display.hibernate();
    }
}

void DisplayManager::wake() {
    Serial.println("DisplayManager: Waking from sleep");
    if (initialized) {
        display.init();
    }
}



bool DisplayManager::isDisplayBusy() const {
    // Stub implementation - in real implementation would check display busy state
    return false;
}

void DisplayManager::setBrightness(uint8_t brightness) {
    displayBrightness = brightness;
}

void DisplayManager::setUpdateMode(bool fastUpdate) {
    fastUpdateMode = fastUpdate;
}

// New drawing methods implementation
void DisplayManager::drawSetupWelcomeScreen() {
    display.setFullWindow();
    display.firstPage();
    do {
        display.fillScreen(GxEPD_WHITE);
        
        // WiFi indicator in top right
        drawWiFiIndicator();
        
        // Welcome message
        display.setFont(&FreeMonoBold12pt7b);
        centerText("Welcome to your", 40, &FreeMonoBold12pt7b);
        centerText("Hodling Hog", 65, &FreeMonoBold12pt7b);
        
        display.setFont(&FreeMonoBold9pt7b);
        centerText("Saving your future,", 95, &FreeMonoBold9pt7b);
        centerText("one oink at a time!", 115, &FreeMonoBold9pt7b);
        
        // Setup instruction
        display.setFont(&FreeMonoBold9pt7b);
        centerText("Setup at", 150, &FreeMonoBold9pt7b);
        display.setFont(&FreeMonoBold12pt7b);
        centerText(setupIP, 175, &FreeMonoBold12pt7b);
        
    } while (display.nextPage());
}

void DisplayManager::drawLightningBalanceScreen() {
    display.setFullWindow();
    display.firstPage();
    do {
        display.fillScreen(GxEPD_WHITE);
        
        // WiFi indicator and device title
        drawWiFiIndicator();
        drawDeviceTitle();
        
        // Page content
        display.setFont(&FreeMonoBold12pt7b);
        centerText("Lightning Wallet", 80, &FreeMonoBold12pt7b);
        centerText("Balance", 105, &FreeMonoBold12pt7b);
        
        // Balance display with comma formatting
        display.setFont(&FreeMonoBold9pt7b);
        String balanceStr = utils.formatNumber(balanceData.lightningBalance) + " sats";
        centerText(balanceStr, 140, &FreeMonoBold9pt7b);
        
        // Status
        centerText(balanceData.lightningValid ? "✓ Updated" : "✗ Offline", 165, &FreeMonoBold9pt7b);
        
    } while (display.nextPage());
}

void DisplayManager::drawColdBalanceScreen() {
    display.setFullWindow();
    display.firstPage();
    do {
        display.fillScreen(GxEPD_WHITE);
        
        // WiFi indicator and device title
        drawWiFiIndicator();
        drawDeviceTitle();
        
        // Page content
        display.setFont(&FreeMonoBold12pt7b);
        centerText("On-chain Cold", 80, &FreeMonoBold12pt7b);
        centerText("Storage Balance", 105, &FreeMonoBold12pt7b);
        
        // Balance display with comma formatting
        display.setFont(&FreeMonoBold9pt7b);
        String balanceStr = utils.formatNumber(balanceData.coldBalance) + " sats";
        centerText(balanceStr, 140, &FreeMonoBold9pt7b);
        
        // Status
        centerText(balanceData.coldValid ? "✓ Updated" : "✗ Offline", 165, &FreeMonoBold9pt7b);
        
    } while (display.nextPage());
}

void DisplayManager::drawTotalBalanceScreen() {
    display.setFullWindow();
    display.firstPage();
    do {
        display.fillScreen(GxEPD_WHITE);
        
        // WiFi indicator and device title
        drawWiFiIndicator();
        drawDeviceTitle();
        
        // Page content
        display.setFont(&FreeMonoBold12pt7b);
        centerText("Total Balance", 90, &FreeMonoBold12pt7b);
        
        // Calculate and display total balance with comma formatting
        uint64_t total = balanceData.lightningBalance + balanceData.coldBalance;
        display.setFont(&FreeMonoBold9pt7b);
        String totalStr = utils.formatNumber(total) + " sats";
        centerText(totalStr, 130, &FreeMonoBold9pt7b);
        
        // Status indicator
        bool bothValid = balanceData.lightningValid && balanceData.coldValid;
        centerText(bothValid ? "✓ Updated" : "⚠ Partial Data", 155, &FreeMonoBold9pt7b);
        
    } while (display.nextPage());
}



void DisplayManager::drawErrorScreen(const String& error) {
    display.setFullWindow();
    display.firstPage();
    do {
        display.fillScreen(GxEPD_WHITE);
        
        // WiFi indicator
        drawWiFiIndicator();
        
        display.setFont(&FreeMonoBold12pt7b);
        centerText("ERROR", 60, &FreeMonoBold12pt7b);
        
        display.setFont(&FreeMonoBold9pt7b);
        centerText(error, 100, &FreeMonoBold9pt7b);
        centerText("Press button", 140, &FreeMonoBold9pt7b);
        centerText("to restart", 165, &FreeMonoBold9pt7b);
        
    } while (display.nextPage());
}

// Helper methods
String DisplayManager::formatBalance(uint64_t satoshis, bool showDecimals) {
    if (satoshis == 0) return "0 sats";
    
    if (satoshis < 1000) {
        return String(satoshis) + " sats";
    } else if (satoshis < 100000) {
        return String(satoshis / 1000.0, 1) + "K sats";
    } else if (satoshis < 100000000) {
        return String(satoshis / 1000000.0, 2) + "M sats";
    } else {
        return String(satoshis / 100000000.0, showDecimals ? 8 : 2) + " BTC";
    }
}

String DisplayManager::formatTime(unsigned long timestamp) {
    return "00:00"; // Stub
}

void DisplayManager::centerText(const String& text, int16_t y, const GFXfont* font) {
    display.setFont(font);
    int16_t tbx, tby; 
    uint16_t tbw, tbh;
    display.getTextBounds(text, 0, 0, &tbx, &tby, &tbw, &tbh);
    uint16_t x = (DISPLAY_WIDTH - tbw) / 2;
    display.setCursor(x, y);
    display.print(text);
}

void DisplayManager::rightAlignText(const String& text, int16_t x, int16_t y, const GFXfont* font) {
    display.setFont(font);
    int16_t tbx, tby; 
    uint16_t tbw, tbh;
    display.getTextBounds(text, 0, 0, &tbx, &tby, &tbw, &tbh);
    display.setCursor(x - tbw, y);
    display.print(text);
}

int16_t DisplayManager::getTextWidth(const String& text, const GFXfont* font) {
    display.setFont(font);
    int16_t tbx, tby; 
    uint16_t tbw, tbh;
    display.getTextBounds(text, 0, 0, &tbx, &tby, &tbw, &tbh);
    return tbw;
}

int16_t DisplayManager::getTextHeight(const GFXfont* font) {
    return font ? font->yAdvance : 8;
}

void DisplayManager::fullUpdate() {
    // Trigger full e-paper update
}

void DisplayManager::partialUpdate() {
    // Trigger partial e-paper update
}

void DisplayManager::waitForDisplay() {
    while (isDisplayBusy()) {
        delay(10);
    }
}

// Helper drawing method implementations
void DisplayManager::drawDeviceTitle() {
    // Create title text: "Someone's Hodling Hog" or "My Hodling Hog"
    String title;
    if (deviceName == "Hodling Hog" || deviceName.isEmpty()) {
        title = "My Hodling Hog";
    } else {
        title = deviceName + "'s Hodling Hog";
    }
    
    display.setFont(&FreeMonoBold9pt7b);
    centerText(title, 25, &FreeMonoBold9pt7b);
    
    // Draw separator line
    display.drawLine(20, 35, 180, 35, GxEPD_BLACK);
}

void DisplayManager::drawWiFiIndicator() {
    // Draw WiFi symbol in top right corner (185, 15)
    drawWiFiSymbol(170, 15, wifiConnected);
}

void DisplayManager::drawWiFiSymbol(int16_t x, int16_t y, bool connected) {
    if (connected) {
        // Draw connected WiFi symbol - simple arcs
        display.drawCircle(x + 15, y + 8, 3, GxEPD_BLACK);   // Center dot
        display.drawCircle(x + 15, y + 8, 6, GxEPD_BLACK);   // Inner arc
        display.drawCircle(x + 15, y + 8, 9, GxEPD_BLACK);   // Outer arc
    } else {
        // Draw X for disconnected
        display.drawLine(x + 10, y + 4, x + 20, y + 12, GxEPD_BLACK);
        display.drawLine(x + 20, y + 4, x + 10, y + 12, GxEPD_BLACK);
    }
}

void DisplayManager::drawBalance(uint64_t satoshis, int16_t x, int16_t y, const GFXfont* font) {
    display.setFont(font);
    display.setCursor(x, y);
    display.print(String(satoshis) + " sats");
}

void DisplayManager::drawBitcoinSymbol(int16_t x, int16_t y) {
    // Simple Bitcoin symbol - ₿
    display.setCursor(x, y);
    display.print("₿");
}

void DisplayManager::drawLightningSymbol(int16_t x, int16_t y) {
    // Simple Lightning symbol - ⚡
    display.setCursor(x, y);
    display.print("⚡");
} 