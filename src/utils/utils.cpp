#include "utils.h"
#include <WiFi.h>

// Global instance
Utils utils;

Utils::Utils() {
    ntpInitialized = false;
    timeValid = false;
    timezoneOffset = 0;
    dstOffset = 0;
    ntpServer = NTP_SERVER;
    lastNTPSync = 0;
    lastBatteryUpdate = 0;
    debugEnabled = false;
    
    // Initialize battery status
    batteryStatus.voltage = 0.0;
    batteryStatus.percentage = 100;
    batteryStatus.charging = false;
    batteryStatus.lowBattery = false;
    batteryStatus.lastUpdate = 0;
}

void Utils::init() {
    Serial.println("Utils: Initializing");
    initRandomSeed();
    debugEnabled = true;
}

bool Utils::initNTP(const String& server, int timezoneOffset) {
    Serial.printf("Utils: Initializing NTP - Server: %s, Timezone: %+d\n", 
                  server.c_str(), timezoneOffset);
    
    ntpServer = server;
    this->timezoneOffset = timezoneOffset;
    
    // Configure NTP
    configTime(timezoneOffset * 3600, dstOffset * 3600, server.c_str());
    
    ntpInitialized = true;
    return true;
}

bool Utils::syncTime() {
    if (!ntpInitialized) return false;
    
    Serial.println("Utils: Syncing time with NTP server");
    
    // Wait for time sync with timeout
    unsigned long startTime = millis();
    while (!timeValid && (millis() - startTime < NTP_TIMEOUT)) {
        time_t now = time(nullptr);
        if (now > 1000000000) { // Valid timestamp
            timeValid = true;
            lastNTPSync = millis();
            Serial.printf("Utils: Time synced - %s", ctime(&now));
            return true;
        }
        delay(100);
    }
    
    Serial.println("Utils: NTP sync timeout");
    return false;
}

DateTime Utils::getCurrentTime() {
    DateTime dt;
    dt.valid = false;
    
    if (!timeValid) {
        return dt;
    }
    
    time_t now = time(nullptr);
    struct tm* timeinfo = localtime(&now);
    
    dt.year = timeinfo->tm_year + 1900;
    dt.month = timeinfo->tm_mon + 1;
    dt.day = timeinfo->tm_mday;
    dt.hour = timeinfo->tm_hour;
    dt.minute = timeinfo->tm_min;
    dt.second = timeinfo->tm_sec;
    dt.weekday = timeinfo->tm_wday;
    dt.valid = true;
    
    return dt;
}

String Utils::formatTime(const DateTime& dt, const String& format) {
    if (!dt.valid) return "Invalid Time";
    
    char buffer[64];
    snprintf(buffer, sizeof(buffer), "%04d-%02d-%02d %02d:%02d:%02d",
             dt.year, dt.month, dt.day, dt.hour, dt.minute, dt.second);
    return String(buffer);
}

String Utils::formatTimestamp(unsigned long timestamp, const String& format) {
    DateTime dt = timestampToDateTime(timestamp);
    return formatTime(dt, format);
}

unsigned long Utils::getTimestamp() {
    return timeValid ? time(nullptr) : (millis() / 1000);
}

unsigned long Utils::getTimestamp(const DateTime& dt) {
    return millis() / 1000; // Stub
}

bool Utils::isTimeValid() {
    return timeValid;
}

void Utils::setTimezone(int offset, int dstOffset) {
    timezoneOffset = offset;
    this->dstOffset = dstOffset;
    if (ntpInitialized) {
        configTime(offset * 3600, dstOffset * 3600, ntpServer.c_str());
    }
}

String Utils::getTimezoneString() {
    return "UTC" + String(timezoneOffset >= 0 ? "+" : "") + String(timezoneOffset);
}

// Time formatting helpers
String Utils::formatDuration(unsigned long milliseconds) {
    unsigned long seconds = milliseconds / 1000;
    unsigned long minutes = seconds / 60;
    unsigned long hours = minutes / 60;
    unsigned long days = hours / 24;
    
    if (days > 0) {
        return String(days) + "d " + String(hours % 24) + "h";
    } else if (hours > 0) {
        return String(hours) + "h " + String(minutes % 60) + "m";
    } else if (minutes > 0) {
        return String(minutes) + "m " + String(seconds % 60) + "s";
    } else {
        return String(seconds) + "s";
    }
}

String Utils::formatUptime() {
    return formatDuration(millis());
}

String Utils::formatAge(unsigned long timestamp) {
    if (timestamp == 0) return "Never";
    unsigned long age = millis() - timestamp;
    return formatDuration(age) + " ago";
}

String Utils::getTimeAgo(unsigned long timestamp) {
    return formatAge(timestamp);
}

bool Utils::isAfterTime(unsigned long timestamp, unsigned long hours) {
    return (millis() - timestamp) > (hours * 3600000);
}

bool Utils::isToday(unsigned long timestamp) {
    return !isAfterTime(timestamp, 24);
}

// Bitcoin amount formatting
String Utils::formatSatoshis(uint64_t satoshis, bool showSymbol, uint8_t decimals) {
    if (satoshis == 0) return showSymbol ? "0 sats" : "0";
    
    if (satoshis < 1000) {
        return String(satoshis) + (showSymbol ? " sats" : "");
    } else if (satoshis < 100000) {
        return String(satoshis / 1000.0, 1) + (showSymbol ? "K sats" : "K");
    } else if (satoshis < 100000000) {
        return String(satoshis / 1000000.0, 2) + (showSymbol ? "M sats" : "M");
    } else {
        return String(satoshis / 100000000.0, (unsigned int)decimals) + (showSymbol ? " BTC" : "");
    }
}

String Utils::formatBTC(uint64_t satoshis, bool showSymbol, uint8_t decimals) {
    float btc = satoshis / 100000000.0;
    return String(btc, (unsigned int)decimals) + (showSymbol ? " BTC" : "");
}

String Utils::formatFiat(uint64_t satoshis, const String& currency, float exchangeRate) {
    if (exchangeRate <= 0) return "N/A";
    float btc = satoshis / 100000000.0;
    float fiat = btc * exchangeRate;
    return String(fiat, 2) + " " + currency;
}

uint64_t Utils::parseSatoshis(const String& amount) {
    return amount.toInt();
}

float Utils::satoshisToBTC(uint64_t satoshis) {
    return satoshis / 100000000.0;
}

uint64_t Utils::btcToSatoshis(float btc) {
    return btc * 100000000;
}

// Number formatting
String Utils::formatNumber(uint64_t number, bool useThousandsSeparator) {
    String numStr = String(number);
    if (!useThousandsSeparator || numStr.length() <= 3) {
        return numStr;
    }
    
    String formatted = "";
    int count = 0;
    for (int i = numStr.length() - 1; i >= 0; i--) {
        if (count > 0 && count % 3 == 0) {
            formatted = THOUSANDS_SEPARATOR + formatted;
        }
        formatted = numStr.charAt(i) + formatted;
        count++;
    }
    return formatted;
}

String Utils::formatBytes(size_t bytes) {
    if (bytes < 1024) return String(bytes) + " B";
    if (bytes < 1024 * 1024) return String(bytes / 1024.0, 1) + " KB";
    if (bytes < 1024 * 1024 * 1024) return String(bytes / (1024.0 * 1024), 1) + " MB";
    return String(bytes / (1024.0 * 1024 * 1024), 1) + " GB";
}

String Utils::formatPercentage(float percentage, uint8_t decimals) {
    return String(percentage, (unsigned int)decimals) + "%";
}

String Utils::formatHex(const uint8_t* data, size_t length) {
    String hex = "";
    for (size_t i = 0; i < length; i++) {
        if (data[i] < 16) hex += "0";
        hex += String(data[i], HEX);
    }
    return hex;
}

String Utils::formatMAC(const uint8_t* mac) {
    char buffer[18];
    snprintf(buffer, sizeof(buffer), "%02X:%02X:%02X:%02X:%02X:%02X",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    return String(buffer);
}

String Utils::formatIP(IPAddress ip) {
    return ip.toString();
}

// QR code generation
QRCodeData Utils::generateQR(const String& data, uint8_t version) {
    QRCodeData qr;
    qr.valid = false;
    qr.modules = nullptr;
    qr.size = 0;
    qr.version = version;
    
    // Stub implementation
    Serial.printf("Utils: Generating QR code for: %s\n", data.c_str());
    
    return qr;
}

bool Utils::generateQRToBuffer(const String& data, uint8_t* buffer, uint8_t& size, uint8_t version) {
    Serial.printf("Utils: Generating QR to buffer: %s\n", data.c_str());
    size = 25; // Stub size
    return false; // Stub
}

void Utils::freeQRCode(QRCodeData& qr) {
    if (qr.modules) {
        free(qr.modules);
        qr.modules = nullptr;
    }
    qr.valid = false;
}

uint8_t Utils::calculateQRVersion(const String& data) {
    return (data.length() < 100) ? 1 : 3; // Stub
}

String Utils::optimizeQRData(const String& data) {
    return data; // Stub
}

bool Utils::isValidQRData(const String& data) {
    return data.length() > 0 && data.length() < QR_MAX_DATA_SIZE;
}

// Battery monitoring
void Utils::initBatteryMonitor() {
    Serial.println("Utils: Initializing battery monitor");
    pinMode(BATTERY_ADC_PIN, INPUT);
    updateBatteryStatus();
}

BatteryStatus Utils::getBatteryStatus() {
    if (millis() - lastBatteryUpdate > BATTERY_INTERVAL) {
        updateBatteryStatus();
    }
    return batteryStatus;
}

void Utils::updateBatteryStatus() {
    batteryStatus.voltage = readBatteryVoltage();
    batteryStatus.percentage = calculateBatteryPercentage(batteryStatus.voltage);
    batteryStatus.charging = detectCharging();
    batteryStatus.lowBattery = batteryStatus.percentage < 20;
    batteryStatus.lastUpdate = millis();
    lastBatteryUpdate = millis();
}

bool Utils::isBatteryLow() {
    return getBatteryStatus().lowBattery;
}

float Utils::getBatteryVoltage() {
    return getBatteryStatus().voltage;
}

uint8_t Utils::getBatteryPercentage() {
    return getBatteryStatus().percentage;
}

bool Utils::isCharging() {
    return getBatteryStatus().charging;
}

// Network utilities
NetworkDiagnostics Utils::getNetworkDiagnostics() {
    NetworkDiagnostics diag;
    diag.ssid = WiFi.SSID();
    diag.rssi = WiFi.RSSI();
    diag.ipAddress = WiFi.localIP().toString();
    diag.macAddress = WiFi.macAddress();
    diag.uptime = millis();
    diag.internetAccess = checkInternetConnection();
    diag.pingTime = pingHost("google.com");
    return diag;
}

bool Utils::checkInternetConnection(const String& host, uint16_t port) {
    // Stub implementation
    return WiFi.status() == WL_CONNECTED;
}

uint32_t Utils::pingHost(const String& host) {
    return 50; // Stub - 50ms ping
}

String Utils::getLocalIP() {
    return WiFi.localIP().toString();
}

String Utils::getMACAddress() {
    return WiFi.macAddress();
}

int32_t Utils::getWiFiRSSI() {
    return WiFi.RSSI();
}

String Utils::getWiFiSSID() {
    return WiFi.SSID();
}

// System utilities
void Utils::restart() {
    Serial.println("Utils: Restarting system...");
    ESP.restart();
}

void Utils::deepSleep(uint64_t microseconds) {
    Serial.printf("Utils: Entering deep sleep for %llu microseconds\n", microseconds);
    esp_deep_sleep(microseconds);
}

uint32_t Utils::getFreeHeap() {
    return ESP.getFreeHeap();
}

uint32_t Utils::getLargestFreeBlock() {
    return ESP.getMaxAllocHeap();
}

float Utils::getCpuTemperature() {
    return 25.0; // Stub - 25°C
}

uint32_t Utils::getChipId() {
    return ESP.getEfuseMac();
}

String Utils::getChipModel() {
    return ESP.getChipModel();
}

uint32_t Utils::getFlashSize() {
    return ESP.getFlashChipSize();
}

String Utils::getSDKVersion() {
    return ESP.getSdkVersion();
}

// String utilities
String Utils::trim(const String& str) {
    String trimmed = str;
    trimmed.trim();
    return trimmed;
}

String Utils::toUpperCase(const String& str) {
    String upper = str;
    upper.toUpperCase();
    return upper;
}

String Utils::toLowerCase(const String& str) {
    String lower = str;
    lower.toLowerCase();
    return lower;
}

String Utils::sanitizeString(const String& str) {
    String sanitized = str;
    // Remove non-printable characters
    for (int i = 0; i < sanitized.length(); i++) {
        if (!isPrintable(sanitized.charAt(i))) {
            sanitized.setCharAt(i, ' ');
        }
    }
    return sanitized;
}

String Utils::escapeJSON(const String& str) {
    String escaped = str;
    escaped.replace("\\", "\\\\");
    escaped.replace("\"", "\\\"");
    escaped.replace("\n", "\\n");
    escaped.replace("\r", "\\r");
    escaped.replace("\t", "\\t");
    return escaped;
}

String Utils::unescapeJSON(const String& str) {
    String unescaped = str;
    unescaped.replace("\\\"", "\"");
    unescaped.replace("\\\\", "\\");
    unescaped.replace("\\n", "\n");
    unescaped.replace("\\r", "\r");
    unescaped.replace("\\t", "\t");
    return unescaped;
}

String Utils::urlEncode(const String& str) {
    return str; // Stub
}

String Utils::urlDecode(const String& str) {
    return str; // Stub
}

// Debug and logging
void Utils::enableDebug(bool enable) {
    debugEnabled = enable;
    Serial.printf("Utils: Debug %s\n", enable ? "enabled" : "disabled");
}

void Utils::debugPrint(const String& message) {
    if (debugEnabled) {
        Serial.print(message);
    }
}

void Utils::debugPrintln(const String& message) {
    if (debugEnabled) {
        Serial.println(message);
    }
}

void Utils::debugPrintf(const char* format, ...) {
    if (debugEnabled) {
        va_list args;
        va_start(args, format);
        Serial.printf(format, args);
        va_end(args);
    }
}

void Utils::logMessage(const String& level, const String& message) {
    Serial.printf("[%s] %s: %s\n", formatTime(getCurrentTime()).c_str(), level.c_str(), message.c_str());
}

void Utils::dumpHex(const uint8_t* data, size_t length) {
    for (size_t i = 0; i < length; i++) {
        if (i % 16 == 0) Serial.printf("\n%04x: ", i);
        Serial.printf("%02x ", data[i]);
    }
    Serial.println();
}

void Utils::setError(const String& error) {
    lastError = error;
    logMessage("ERROR", error);
}

void Utils::clearError() {
    lastError = "";
}

// Private helper methods
DateTime Utils::timestampToDateTime(unsigned long timestamp) {
    DateTime dt;
    time_t t = timestamp;
    struct tm* timeinfo = localtime(&t);
    
    dt.year = timeinfo->tm_year + 1900;
    dt.month = timeinfo->tm_mon + 1;
    dt.day = timeinfo->tm_mday;
    dt.hour = timeinfo->tm_hour;
    dt.minute = timeinfo->tm_min;
    dt.second = timeinfo->tm_sec;
    dt.weekday = timeinfo->tm_wday;
    dt.valid = true;
    
    return dt;
}

float Utils::readBatteryVoltage() {
    // Stub implementation
    return 3.7; // 3.7V typical LiPo voltage
}

uint8_t Utils::calculateBatteryPercentage(float voltage) {
    // Simple linear mapping for LiPo battery
    if (voltage >= 4.2) return 100;
    if (voltage <= 3.0) return 0;
    return (voltage - 3.0) / (4.2 - 3.0) * 100;
}

bool Utils::detectCharging() {
    return false; // Stub
}

void Utils::initRandomSeed() {
    randomSeed(ESP.getEfuseMac());
}

bool Utils::isPrintable(char c) {
    return c >= 32 && c <= 126;
}

// Stub implementations for remaining methods
bool Utils::isLeapYear(uint16_t year) { return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0); }
uint8_t Utils::getDaysInMonth(uint8_t month, uint16_t year) { return 30; } // Stub
bool Utils::connectToNTP() { return true; }
void Utils::updateLocalTime() {}
void Utils::averageBatteryReading() {}
uint8_t Utils::getQRErrorCorrection() { return 0; }
bool Utils::allocateQRBuffer(QRCodeData& qr, uint8_t size) { return false; }
void Utils::optimizeQRErrorCorrection(const String& data, uint8_t& version) {}
char Utils::hexCharToValue(char c) { return 0; }
char Utils::valueToHexChar(uint8_t value) { return '0'; }
bool Utils::isWhitespace(char c) { return c == ' ' || c == '\t' || c == '\n' || c == '\r'; }
bool Utils::testDNSResolution(const String& hostname) { return true; }
bool Utils::testTCPConnection(const String& host, uint16_t port) { return true; }
uint32_t Utils::measureLatency(const String& host) { return 50; }
void Utils::calibrateADC() {}
void Utils::setupWatchdog() {}
void Utils::feedWatchdog() {}

// String validation
bool Utils::isNumeric(const String& str) {
    for (int i = 0; i < str.length(); i++) {
        if (!isdigit(str.charAt(i))) return false;
    }
    return true;
}

bool Utils::isHex(const String& str) {
    for (int i = 0; i < str.length(); i++) {
        char c = str.charAt(i);
        if (!isdigit(c) && !(c >= 'a' && c <= 'f') && !(c >= 'A' && c <= 'F')) {
            return false;
        }
    }
    return true;
}

bool Utils::isValidEmail(const String& email) {
    return email.indexOf('@') > 0 && email.indexOf('.') > 0;
}

bool Utils::isValidURL(const String& url) {
    return url.startsWith("http://") || url.startsWith("https://");
}

bool Utils::isValidJSON(const String& json) {
    return json.startsWith("{") && json.endsWith("}");
}

bool Utils::isValidBitcoinAddress(const String& address) {
    return address.length() > 25 && address.length() < 65;
}

bool Utils::isValidLightningInvoice(const String& invoice) {
    return invoice.startsWith("lnbc") || invoice.startsWith("lntb");
}

String Utils::joinStrings(const std::vector<String>& strings, const String& separator) {
    String result = "";
    for (size_t i = 0; i < strings.size(); i++) {
        if (i > 0) result += separator;
        result += strings[i];
    }
    return result;
}

std::vector<String> Utils::splitString(const String& str, const String& separator) {
    std::vector<String> parts;
    // Stub implementation
    parts.push_back(str);
    return parts;
}

String Utils::sha256Hash(const String& input) {
    return "hash_" + input; // Stub
}

String Utils::sha256Hash(const uint8_t* data, size_t length) {
    return "hash_data"; // Stub
}

String Utils::generateRandomString(size_t length) {
    String chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
    String result = "";
    for (size_t i = 0; i < length; i++) {
        result += chars.charAt(random(chars.length()));
    }
    return result;
}

uint32_t Utils::generateRandomNumber(uint32_t min, uint32_t max) {
    return random(min, max + 1);
}

String Utils::generateUUID() {
    return generateRandomString(8) + "-" + generateRandomString(4) + "-" + 
           generateRandomString(4) + "-" + generateRandomString(4) + "-" + generateRandomString(12);
}

uint32_t Utils::crc32(const uint8_t* data, size_t length) {
    return 0x12345678; // Stub
}

String Utils::base64Encode(const uint8_t* data, size_t length) {
    return "base64data"; // Stub
}

String Utils::base64Encode(const String& str) {
    return base64Encode((uint8_t*)str.c_str(), str.length());
}

String Utils::base64Decode(const String& encoded) {
    return "decoded"; // Stub
}

String Utils::base58Encode(const uint8_t* data, size_t length) {
    return "base58data"; // Stub
}

String Utils::base58Decode(const String& encoded) {
    return "decoded"; // Stub
}

void* Utils::safeMalloc(size_t size) {
    return malloc(size);
}

void* Utils::safeRealloc(void* ptr, size_t size) {
    return realloc(ptr, size);
}

void Utils::safeFree(void* ptr) {
    if (ptr) free(ptr);
}

bool Utils::checkMemoryLeaks() {
    return false; // Stub
}

size_t Utils::getMemoryUsage() {
    return ESP.getFreeHeap();
}

void Utils::drawQRCode(const QRCodeData& qr, int16_t x, int16_t y, uint8_t scale) {
    // Stub - would integrate with display
}

String Utils::qrToString(const QRCodeData& qr) {
    return "QR_CODE_STRING"; // Stub
}

void Utils::printQRCode(const QRCodeData& qr) {
    Serial.println("QR Code (stub)");
} 