#ifndef DISPLAY_H
#define DISPLAY_H

#include <Arduino.h>
#include <GxEPD2_BW.h>
#include <GxEPD2_3C.h>
#include <Fonts/FreeMonoBold9pt7b.h>
#include <Fonts/FreeMonoBold12pt7b.h>
#include <Fonts/FreeMonoBold18pt7b.h>

// Pin definitions for e-paper display (as specified in requirements)
#define EPD_CS    5    // Chip Select
#define EPD_DC    17   // Data/Command
#define EPD_RST   16   // Reset
#define EPD_BUSY  4    // Busy
#define EPD_MOSI  23   // SPI MOSI
#define EPD_SCK   18   // SPI SCK

// Display dimensions (1.54" e-paper, 200x200)
#define DISPLAY_WIDTH  200
#define DISPLAY_HEIGHT 200

// Screen types
enum class ScreenType {
    SETUP_WELCOME,        // Not set up yet - welcome page with IP
    LIGHTNING_BALANCE,    // Lightning wallet balance page
    COLD_BALANCE,         // On-chain cold storage balance page
    TOTAL_BALANCE,        // Total balance page
    ERROR_SCREEN
};

// Balance data structure
struct BalanceData {
    uint64_t lightningBalance;    // Lightning balance in satoshis
    uint64_t coldBalance;         // Cold storage balance in satoshis
    uint64_t totalBalance;        // Combined balance in satoshis
    bool lightningValid;          // Whether lightning data is current
    bool coldValid;               // Whether cold storage data is current
    unsigned long lastUpdate;     // Last update timestamp
};

// QR code data structure
struct QRData {
    String lightningAddress;      // Lightning address for receiving
    String coldAddress;           // Cold storage address for receiving
    String invoiceData;           // Lightning invoice data (if any)
};

class DisplayManager {
public:
    DisplayManager();
    void init();
    void showScreen(ScreenType screen);
    void updateBalances(const BalanceData& balances);
    void updateQRData(const QRData& qrData);
    void showErrorScreen(const String& error);
    void clear();
    void sleep();
    void wake();
    
    // Device setup and status
    void setDeviceSetup(bool isSetup) { deviceSetup = isSetup; }
    bool isDeviceSetup() const { return deviceSetup; }
    void setDeviceName(const String& name) { deviceName = name; }
    void setSetupIP(const String& ip) { setupIP = ip; }
    
    // WiFi status
    void setWiFiStatus(bool connected) { wifiConnected = connected; }
    bool getWiFiStatus() const { return wifiConnected; }
    
    // Screen cycling (only for setup pages)
    void nextSetupScreen();
    void previousSetupScreen();
    ScreenType getCurrentScreen() const { return currentScreen; }
    
    // Status and settings
    bool isDisplayBusy() const;
    void setBrightness(uint8_t brightness);
    void setUpdateMode(bool fastUpdate);
    
private:
    GxEPD2_BW<GxEPD2_154_D67, GxEPD2_154_D67::HEIGHT> display;
    
    ScreenType currentScreen;
    BalanceData balanceData;
    QRData qrData;
    bool initialized;
    bool fastUpdateMode;
    uint8_t displayBrightness;
    
    // Device and status tracking
    bool deviceSetup;
    String deviceName;
    String setupIP;
    bool wifiConnected;
    
    // Screen drawing methods
    void drawSetupWelcomeScreen();
    void drawLightningBalanceScreen();
    void drawColdBalanceScreen();
    void drawTotalBalanceScreen();
    void drawErrorScreen(const String& error);
    
    // Helper drawing methods
    void drawDeviceTitle();                    // Draw "Someone's Hodling Hog" title
    void drawWiFiIndicator();                  // Draw WiFi symbol in top right
    void drawBalance(uint64_t satoshis, int16_t x, int16_t y, const GFXfont* font);
    void drawWiFiSymbol(int16_t x, int16_t y, bool connected);
    void drawBitcoinSymbol(int16_t x, int16_t y);
    void drawLightningSymbol(int16_t x, int16_t y);
    
    // Text and formatting helpers
    String formatBalance(uint64_t satoshis, bool showDecimals = true);
    String formatTime(unsigned long timestamp);
    void centerText(const String& text, int16_t y, const GFXfont* font);
    void rightAlignText(const String& text, int16_t x, int16_t y, const GFXfont* font);
    int16_t getTextWidth(const String& text, const GFXfont* font);
    int16_t getTextHeight(const GFXfont* font);
    
    // Display control
    void fullUpdate();
    void partialUpdate();
    void waitForDisplay();
};

// Global instance
extern DisplayManager displayMgr;

#endif // DISPLAY_H 