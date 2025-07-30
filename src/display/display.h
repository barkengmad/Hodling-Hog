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
    LIGHTNING_BALANCE,
    COLD_BALANCE,
    COMBINED_BALANCE,
    CONFIG_INFO,
    BOOT_SCREEN,
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
    void showBootScreen();
    void showErrorScreen(const String& error);
    void showConfigInfo(const String& ipAddress);
    void showWiFiStatus(bool connected, const String& status = "");
    void showIPAddress(const String& ipAddress, uint32_t displayDurationMs = 5000);
    void clear();
    void sleep();
    void wake();
    
    // Screen cycling
    void nextScreen();
    void previousScreen();
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
    
    // Screen drawing methods
    void drawLightningBalanceScreen();
    void drawColdBalanceScreen();
    void drawCombinedBalanceScreen();
    void drawConfigInfoScreen(const String& ipAddress);
    void drawBootScreen();
    void drawErrorScreen(const String& error);
    
    // Helper drawing methods
    void drawBalance(uint64_t satoshis, int16_t x, int16_t y, const GFXfont* font);
    void drawQRCode(const String& data, int16_t x, int16_t y, uint8_t scale = 2);
    void drawBitcoinSymbol(int16_t x, int16_t y);
    void drawLightningSymbol(int16_t x, int16_t y);
    void drawWiFiSymbol(int16_t x, int16_t y, bool connected);
    void drawBatterySymbol(int16_t x, int16_t y, uint8_t percentage);
    void drawStatusBar();
    void drawFrame();
    
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