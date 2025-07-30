#ifndef UTILS_H
#define UTILS_H

#include <Arduino.h>
#include <WiFi.h>
#include <time.h>
#include <qrcode.h>
#include <vector>

// Time configuration
#define NTP_TIMEOUT         5000    // NTP sync timeout in milliseconds
#define TIME_ZONE_OFFSET    0       // UTC offset in hours
#define DST_OFFSET          0       // Daylight saving time offset
#define NTP_SERVER          "pool.ntp.org"
#define NTP_UPDATE_INTERVAL 3600000 // 1 hour in milliseconds

// QR code configuration
#define QR_VERSION_MIN      1       // Minimum QR version
#define QR_VERSION_MAX      10      // Maximum QR version
#define QR_ERROR_CORRECT    ECC_LOW // Error correction level
#define QR_MAX_DATA_SIZE    1024    // Maximum data size for QR

// Formatting configuration
#define MAX_DECIMAL_PLACES  8       // Maximum decimal places for BTC
#define THOUSANDS_SEPARATOR ","     // Thousands separator character
#define DECIMAL_SEPARATOR   "."     // Decimal separator character

// Battery monitoring (if applicable)
#define BATTERY_ADC_PIN     A0      // ADC pin for battery monitoring
#define BATTERY_SAMPLES     10      // Number of samples for averaging
#define BATTERY_INTERVAL    30000   // Battery check interval in ms

// Time and date structures
struct DateTime {
    uint16_t year;
    uint8_t month;
    uint8_t day;
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
    uint8_t weekday;
    bool valid;
};

// QR code data structure
struct QRCodeData {
    uint8_t* modules;
    uint8_t size;
    uint8_t version;
    bool valid;
};

// Battery status
struct BatteryStatus {
    float voltage;
    uint8_t percentage;
    bool charging;
    bool lowBattery;
    unsigned long lastUpdate;
};

// Network diagnostic data
struct NetworkDiagnostics {
    String ssid;
    int32_t rssi;
    String ipAddress;
    String macAddress;
    uint32_t uptime;
    bool internetAccess;
    uint32_t pingTime;
};

class Utils {
public:
    Utils();
    void init();
    
    // Time management
    bool initNTP(const String& server = NTP_SERVER, int timezoneOffset = 0);
    bool syncTime();
    DateTime getCurrentTime();
    String formatTime(const DateTime& dt, const String& format = "YYYY-MM-DD HH:MM:SS");
    String formatTimestamp(unsigned long timestamp, const String& format = "YYYY-MM-DD HH:MM:SS");
    unsigned long getTimestamp();
    unsigned long getTimestamp(const DateTime& dt);
    bool isTimeValid();
    void setTimezone(int offset, int dstOffset = 0);
    String getTimezoneString();
    
    // Time formatting helpers
    String formatDuration(unsigned long milliseconds);
    String formatUptime();
    String formatAge(unsigned long timestamp);
    String getTimeAgo(unsigned long timestamp);
    bool isAfterTime(unsigned long timestamp, unsigned long hours);
    bool isToday(unsigned long timestamp);
    
    // Bitcoin amount formatting
    String formatSatoshis(uint64_t satoshis, bool showSymbol = true, uint8_t decimals = 8);
    String formatBTC(uint64_t satoshis, bool showSymbol = true, uint8_t decimals = 8);
    String formatFiat(uint64_t satoshis, const String& currency = "USD", float exchangeRate = 0.0);
    uint64_t parseSatoshis(const String& amount);
    float satoshisToBTC(uint64_t satoshis);
    uint64_t btcToSatoshis(float btc);
    
    // Number formatting
    String formatNumber(uint64_t number, bool useThousandsSeparator = true);
    String formatBytes(size_t bytes);
    String formatPercentage(float percentage, uint8_t decimals = 1);
    String formatHex(const uint8_t* data, size_t length);
    String formatMAC(const uint8_t* mac);
    String formatIP(IPAddress ip);
    
    // QR code generation
    QRCodeData generateQR(const String& data, uint8_t version = 0);
    bool generateQRToBuffer(const String& data, uint8_t* buffer, uint8_t& size, uint8_t version = 0);
    void freeQRCode(QRCodeData& qr);
    uint8_t calculateQRVersion(const String& data);
    String optimizeQRData(const String& data);
    bool isValidQRData(const String& data);
    
    // QR code rendering helpers
    void drawQRCode(const QRCodeData& qr, int16_t x, int16_t y, uint8_t scale = 1);
    String qrToString(const QRCodeData& qr);
    void printQRCode(const QRCodeData& qr);
    
    // String utilities
    String trim(const String& str);
    String toUpperCase(const String& str);
    String toLowerCase(const String& str);
    String sanitizeString(const String& str);
    String escapeJSON(const String& str);
    String unescapeJSON(const String& str);
    String urlEncode(const String& str);
    String urlDecode(const String& str);
    
    // String validation and parsing
    bool isNumeric(const String& str);
    bool isHex(const String& str);
    bool isValidEmail(const String& email);
    bool isValidURL(const String& url);
    bool isValidJSON(const String& json);
    bool isValidBitcoinAddress(const String& address);
    bool isValidLightningInvoice(const String& invoice);
    
    // Array and list utilities
    template<typename T>
    T* resizeArray(T* array, size_t oldSize, size_t newSize);
    String joinStrings(const std::vector<String>& strings, const String& separator);
    std::vector<String> splitString(const String& str, const String& separator);
    
    // Cryptographic utilities
    String sha256Hash(const String& input);
    String sha256Hash(const uint8_t* data, size_t length);
    String generateRandomString(size_t length);
    uint32_t generateRandomNumber(uint32_t min, uint32_t max);
    String generateUUID();
    uint32_t crc32(const uint8_t* data, size_t length);
    
    // Base encoding/decoding
    String base64Encode(const uint8_t* data, size_t length);
    String base64Encode(const String& str);
    String base64Decode(const String& encoded);
    String base58Encode(const uint8_t* data, size_t length);
    String base58Decode(const String& encoded);
    
    // Battery monitoring
    void initBatteryMonitor();
    BatteryStatus getBatteryStatus();
    void updateBatteryStatus();
    bool isBatteryLow();
    float getBatteryVoltage();
    uint8_t getBatteryPercentage();
    bool isCharging();
    
    // Network utilities
    NetworkDiagnostics getNetworkDiagnostics();
    bool checkInternetConnection(const String& host = "google.com", uint16_t port = 80);
    uint32_t pingHost(const String& host);
    String getLocalIP();
    String getMACAddress();
    int32_t getWiFiRSSI();
    String getWiFiSSID();
    
    // System utilities
    void restart();
    void deepSleep(uint64_t microseconds);
    uint32_t getFreeHeap();
    uint32_t getLargestFreeBlock();
    float getCpuTemperature();
    uint32_t getChipId();
    String getChipModel();
    uint32_t getFlashSize();
    String getSDKVersion();
    
    // Memory management
    void* safeMalloc(size_t size);
    void* safeRealloc(void* ptr, size_t size);
    void safeFree(void* ptr);
    bool checkMemoryLeaks();
    size_t getMemoryUsage();
    
    // Debug and logging
    void enableDebug(bool enable);
    void debugPrint(const String& message);
    void debugPrintln(const String& message);
    void debugPrintf(const char* format, ...);
    void logMessage(const String& level, const String& message);
    void dumpHex(const uint8_t* data, size_t length);
    
    // Error handling
    String getLastError() const { return lastError; }
    void setError(const String& error);
    void clearError();
    
private:
    bool ntpInitialized;
    bool timeValid;
    int timezoneOffset;
    int dstOffset;
    String ntpServer;
    unsigned long lastNTPSync;
    unsigned long lastBatteryUpdate;
    BatteryStatus batteryStatus;
    String lastError;
    bool debugEnabled;
    
    // Time helpers
    bool connectToNTP();
    void updateLocalTime();
    DateTime timestampToDateTime(unsigned long timestamp);
    bool isLeapYear(uint16_t year);
    uint8_t getDaysInMonth(uint8_t month, uint16_t year);
    
    // Battery monitoring helpers
    float readBatteryVoltage();
    uint8_t calculateBatteryPercentage(float voltage);
    bool detectCharging();
    void averageBatteryReading();
    
    // QR code helpers
    uint8_t getQRErrorCorrection();
    bool allocateQRBuffer(QRCodeData& qr, uint8_t size);
    void optimizeQRErrorCorrection(const String& data, uint8_t& version);
    
    // String processing helpers
    char hexCharToValue(char c);
    char valueToHexChar(uint8_t value);
    bool isWhitespace(char c);
    bool isPrintable(char c);
    
    // Network helpers
    bool testDNSResolution(const String& hostname);
    bool testTCPConnection(const String& host, uint16_t port);
    uint32_t measureLatency(const String& host);
    
    // System helpers
    void initRandomSeed();
    void calibrateADC();
    void setupWatchdog();
    void feedWatchdog();
};

// Global instance
extern Utils utils;

#endif // UTILS_H 