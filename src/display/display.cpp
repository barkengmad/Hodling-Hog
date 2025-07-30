#include "display.h"
#include <Arduino.h>

// Global instance
DisplayManager displayMgr;

DisplayManager::DisplayManager() 
    : display(GxEPD2_154_D67(EPD_CS, EPD_DC, EPD_RST, EPD_BUSY)) {
    currentScreen = ScreenType::BOOT_SCREEN;
    initialized = false;
    fastUpdateMode = false;
    displayBrightness = 128;
    
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
    display.init(115200);
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
        case ScreenType::LIGHTNING_BALANCE:
            drawLightningBalanceScreen();
            break;
        case ScreenType::COLD_BALANCE:
            drawColdBalanceScreen();
            break;
        case ScreenType::COMBINED_BALANCE:
            drawCombinedBalanceScreen();
            break;
        case ScreenType::CONFIG_INFO:
            drawConfigInfoScreen("192.168.4.1");
            break;
        case ScreenType::BOOT_SCREEN:
            drawBootScreen();
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

void DisplayManager::showBootScreen() {
    Serial.println("DisplayManager: Showing boot screen");
    drawBootScreen();
}

void DisplayManager::showErrorScreen(const String& error) {
    Serial.printf("DisplayManager: Showing error: %s\n", error.c_str());
    drawErrorScreen(error);
}

void DisplayManager::showConfigInfo(const String& ipAddress) {
    Serial.printf("DisplayManager: Showing config info - IP: %s\n", ipAddress.c_str());
    drawConfigInfoScreen(ipAddress);
}

void DisplayManager::showWiFiStatus(bool connected, const String& status) {
    Serial.printf("DisplayManager: WiFi %s - %s\n", 
                  connected ? "Connected" : "Disconnected", status.c_str());
    
    // Update status display (simplified)
    display.setPartialWindow(0, 0, DISPLAY_WIDTH, 20);
    display.firstPage();
    do {
        display.fillScreen(GxEPD_WHITE);
        display.setCursor(5, 15);
        display.print(connected ? "WiFi: OK" : "WiFi: --");
        if (status.length() > 0) {
            // Center the IP address or use smaller font if needed
            if (status.length() > 12) {
                // Use smaller positioning for longer IP addresses
                display.setCursor(60, 15);
            } else {
                display.setCursor(80, 15);
            }
            display.print(status);
        }
    } while (display.nextPage());
}

// New method to show IP address with full screen and proper duration
void DisplayManager::showIPAddress(const String& ipAddress, uint32_t displayDurationMs) {
    Serial.printf("DisplayManager: Showing IP address: %s for %dms\n", ipAddress.c_str(), displayDurationMs);
    
    display.setFullWindow();
    display.firstPage();
    do {
        display.fillScreen(GxEPD_WHITE);
        
        // Title
        display.setFont(&FreeMonoBold12pt7b);
        centerText("WiFi Connected!", 60, &FreeMonoBold12pt7b);
        
        // IP Address - use appropriate font size
        display.setFont(&FreeMonoBold9pt7b);
        centerText("Device IP Address:", 100, &FreeMonoBold9pt7b);
        
        // Check if IP fits with current font, if not use smaller positioning
        int16_t ipWidth = getTextWidth(ipAddress, &FreeMonoBold9pt7b);
        if (ipWidth > (DISPLAY_WIDTH - 20)) {
            // IP is too long, center it carefully
            centerText(ipAddress, 130, &FreeMonoBold9pt7b);
        } else {
            // IP fits fine, center it normally  
            centerText(ipAddress, 130, &FreeMonoBold9pt7b);
        }
        
        // Additional info
        display.setFont(&FreeMonoBold9pt7b);
        centerText("Ready to accept", 160, &FreeMonoBold9pt7b);
        centerText("connections", 180, &FreeMonoBold9pt7b);
        
    } while (display.nextPage());
    
    // Keep the IP address on screen for the specified duration
    if (displayDurationMs > 0) {
        delay(displayDurationMs);
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

void DisplayManager::nextScreen() {
    switch (currentScreen) {
        case ScreenType::LIGHTNING_BALANCE:
            showScreen(ScreenType::COLD_BALANCE);
            break;
        case ScreenType::COLD_BALANCE:
            showScreen(ScreenType::COMBINED_BALANCE);
            break;
        case ScreenType::COMBINED_BALANCE:
            showScreen(ScreenType::LIGHTNING_BALANCE);
            break;
        default:
            showScreen(ScreenType::LIGHTNING_BALANCE);
            break;
    }
}

void DisplayManager::previousScreen() {
    switch (currentScreen) {
        case ScreenType::LIGHTNING_BALANCE:
            showScreen(ScreenType::COMBINED_BALANCE);
            break;
        case ScreenType::COLD_BALANCE:
            showScreen(ScreenType::LIGHTNING_BALANCE);
            break;
        case ScreenType::COMBINED_BALANCE:
            showScreen(ScreenType::COLD_BALANCE);
            break;
        default:
            showScreen(ScreenType::LIGHTNING_BALANCE);
            break;
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

// Private drawing methods - simplified implementations
void DisplayManager::drawLightningBalanceScreen() {
    display.setFullWindow();
    display.firstPage();
    do {
        display.fillScreen(GxEPD_WHITE);
        
        // Title
        display.setFont(&FreeMonoBold12pt7b);
        centerText("⚡ LIGHTNING", 30, &FreeMonoBold12pt7b);
        
        // Balance
        display.setFont(&FreeMonoBold18pt7b);
        String balanceStr = formatBalance(balanceData.lightningBalance);
        centerText(balanceStr, 80, &FreeMonoBold18pt7b);
        
        // Status
        display.setFont(&FreeMonoBold9pt7b);
        centerText(balanceData.lightningValid ? "✓ Updated" : "✗ Offline", 120, &FreeMonoBold9pt7b);
        
        // QR placeholder
        display.drawRect(60, 140, 80, 80, GxEPD_BLACK);
        centerText("QR Code", 185, &FreeMonoBold9pt7b);
        
    } while (display.nextPage());
}

void DisplayManager::drawColdBalanceScreen() {
    display.setFullWindow();
    display.firstPage();
    do {
        display.fillScreen(GxEPD_WHITE);
        
        // Title
        display.setFont(&FreeMonoBold12pt7b);
        centerText("❄ COLD STORAGE", 30, &FreeMonoBold12pt7b);
        
        // Balance
        display.setFont(&FreeMonoBold18pt7b);
        String balanceStr = formatBalance(balanceData.coldBalance);
        centerText(balanceStr, 80, &FreeMonoBold18pt7b);
        
        // Status
        display.setFont(&FreeMonoBold9pt7b);
        centerText(balanceData.coldValid ? "✓ Updated" : "✗ Offline", 120, &FreeMonoBold9pt7b);
        
        // QR placeholder
        display.drawRect(60, 140, 80, 80, GxEPD_BLACK);
        centerText("QR Code", 185, &FreeMonoBold9pt7b);
        
    } while (display.nextPage());
}

void DisplayManager::drawCombinedBalanceScreen() {
    display.setFullWindow();
    display.firstPage();
    do {
        display.fillScreen(GxEPD_WHITE);
        
        // Title
        display.setFont(&FreeMonoBold12pt7b);
        centerText("🐷 TOTAL HODL", 30, &FreeMonoBold12pt7b);
        
        // Total balance
        display.setFont(&FreeMonoBold18pt7b);
        String balanceStr = formatBalance(balanceData.totalBalance);
        centerText(balanceStr, 80, &FreeMonoBold18pt7b);
        
        // Breakdown
        display.setFont(&FreeMonoBold9pt7b);
        String lightningStr = "⚡ " + formatBalance(balanceData.lightningBalance, false);
        String coldStr = "❄ " + formatBalance(balanceData.coldBalance, false);
        centerText(lightningStr, 120, &FreeMonoBold9pt7b);
        centerText(coldStr, 140, &FreeMonoBold9pt7b);
        
    } while (display.nextPage());
}

void DisplayManager::drawConfigInfoScreen(const String& ipAddress) {
    display.setFullWindow();
    display.firstPage();
    do {
        display.fillScreen(GxEPD_WHITE);
        
        display.setFont(&FreeMonoBold12pt7b);
        centerText("HODLING HOG", 30, &FreeMonoBold12pt7b);
        centerText("CONFIG MODE", 60, &FreeMonoBold12pt7b);
        
        display.setFont(&FreeMonoBold9pt7b);
        centerText("Connect to WiFi:", 100, &FreeMonoBold9pt7b);
        centerText("HodlingHog-Config", 120, &FreeMonoBold9pt7b);
        centerText("Password: hodling123", 140, &FreeMonoBold9pt7b);
        centerText("Then visit:", 170, &FreeMonoBold9pt7b);
        centerText(ipAddress, 190, &FreeMonoBold9pt7b);
        
    } while (display.nextPage());
}

void DisplayManager::drawBootScreen() {
    display.setFullWindow();
    display.firstPage();
    do {
        display.fillScreen(GxEPD_WHITE);
        
        display.setFont(&FreeMonoBold18pt7b);
        centerText("HODLING", 60, &FreeMonoBold18pt7b);
        centerText("HOG", 90, &FreeMonoBold18pt7b);
        
        display.setFont(&FreeMonoBold9pt7b);
        centerText("🐷⚡", 120, &FreeMonoBold9pt7b);
        centerText("Saving your future,", 150, &FreeMonoBold9pt7b);
        centerText("one oink at a time!", 170, &FreeMonoBold9pt7b);
        
    } while (display.nextPage());
}

void DisplayManager::drawErrorScreen(const String& error) {
    display.setFullWindow();
    display.firstPage();
    do {
        display.fillScreen(GxEPD_WHITE);
        
        display.setFont(&FreeMonoBold12pt7b);
        centerText("ERROR", 40, &FreeMonoBold12pt7b);
        
        display.setFont(&FreeMonoBold9pt7b);
        centerText(error, 80, &FreeMonoBold9pt7b);
        centerText("Press button to restart", 120, &FreeMonoBold9pt7b);
        
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

// Stub implementations for other drawing methods
void DisplayManager::drawBalance(uint64_t satoshis, int16_t x, int16_t y, const GFXfont* font) {}
void DisplayManager::drawQRCode(const String& data, int16_t x, int16_t y, uint8_t scale) {}
void DisplayManager::drawBitcoinSymbol(int16_t x, int16_t y) {}
void DisplayManager::drawLightningSymbol(int16_t x, int16_t y) {}
void DisplayManager::drawWiFiSymbol(int16_t x, int16_t y, bool connected) {}
void DisplayManager::drawBatterySymbol(int16_t x, int16_t y, uint8_t percentage) {}
void DisplayManager::drawStatusBar() {}
void DisplayManager::drawFrame() {} 