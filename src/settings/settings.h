#ifndef SETTINGS_H
#define SETTINGS_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <LittleFS.h>
#include <vector>

// Settings file paths
#define SETTINGS_FILE       "/config.json"
#define BACKUP_SETTINGS     "/config_backup.json"
#define WIFI_SETTINGS       "/wifi.json"
#define WALLET_SETTINGS     "/wallet.json"
#define DISPLAY_CONFIG_FILE "/display.json"
#define POWER_SETTINGS      "/power.json"

// Default configuration values
#define DEFAULT_UPDATE_INTERVAL     300000   // 5 minutes
#define DEFAULT_SLEEP_TIMEOUT       180000   // 3 minutes
#define DEFAULT_DISPLAY_BRIGHTNESS  128      // Medium brightness
#define DEFAULT_BUTTON_HOLD_TIME    2000     // 2 seconds
#define DEFAULT_TILT_SENSITIVITY    50       // Medium sensitivity

// Seed phrase authentication defaults
#define DEFAULT_MAX_LOGIN_ATTEMPTS  5        // Maximum failed attempts
#define DEFAULT_LOCKOUT_DURATION    1800     // 30 minutes lockout
#define SEED_PHRASE_WORD_COUNT      4        // Kid-friendly 4-word seed phrase

// Settings categories
enum class SettingsCategory {
    WIFI,
    LIGHTNING,
    COLD_STORAGE,
    DISPLAY_SETTINGS,
    POWER,
    SYSTEM,
    ALL
};

// WiFi settings structure
struct WiFiSettings {
    String ssid;
    String password;
    bool autoConnect;
    uint8_t connectionTimeout;
    String hostname;
    bool enableAP;
    String apSSID;
    String apPassword;
};

// Lightning wallet settings
struct LightningSettings {
    String apiToken;           // WoS API bearer token
    String apiSecret;          // WoS API secret for HMAC signing
    String baseUrl;
    bool autoUpdate;
    uint32_t updateInterval;
    String receiveAddress;     // Lightning address (user@getalby.com)
    bool enableTransfers;
    uint64_t maxTransferAmount;
    bool walletCreated;        // Track if WoS wallet has been created
};

// Cold storage settings
struct ColdStorageSettings {
    String watchAddress;
    String privateKey;          // Encrypted or empty for watch-only
    String apiEndpoint;
    bool autoUpdate;
    uint32_t updateInterval;
    bool enableSigning;
    uint64_t defaultFeeRate;
};

// Display settings
struct DisplaySettings {
    uint8_t brightness;
    bool fastUpdate;
    uint32_t screenTimeout;
    bool showStatusBar;
    bool showQRCodes;
    uint8_t qrCodeSize;
    String defaultScreen;
    bool enableAnimations;
};

// Power management settings
struct PowerSettings {
    uint32_t sleepTimeout;
    bool enableDeepSleep;
    bool wakeOnButton;
    bool wakeOnTilt;
    uint8_t batteryWarningLevel;
    bool enablePowerSaving;
    uint32_t updateInterval;
};

// System settings
struct SystemSettings {
    String deviceName;
    String timezone;
    bool enableLogging;
    uint8_t logLevel;
    bool enableOTA;
    String ntpServer;
    uint32_t heartbeatInterval;
    bool enableWatchdog;
    
    // Seed phrase authentication
    String seedPhraseHash;      // SHA256 hash of the normalized seed phrase
    bool requireSeedAuth;       // Whether seed authentication is required
    uint32_t maxLoginAttempts;  // Maximum failed login attempts before lockout
    uint32_t lockoutDuration;   // Lockout duration in seconds
    unsigned long lastFailedLogin;
    uint8_t failedLoginCount;
};

// Complete configuration structure
struct HodlingHogConfig {
    WiFiSettings wifi;
    LightningSettings lightning;
    ColdStorageSettings coldStorage;
    DisplaySettings display;
    PowerSettings power;
    SystemSettings system;
    
    // Metadata
    String version;
    unsigned long lastModified;
    uint32_t configVersion;
    String deviceId;
};

class SettingsManager {
public:
    SettingsManager();
    bool init();
    
    // File system operations
    bool formatFileSystem();
    bool backupSettings();
    bool restoreFromBackup();
    size_t getUsedSpace();
    size_t getTotalSpace();
    
    // Configuration loading and saving
    bool loadConfig();
    bool saveConfig();
    bool resetToDefaults();
    bool isConfigValid();
    
    // Category-specific operations
    bool loadCategory(SettingsCategory category);
    bool saveCategory(SettingsCategory category);
    bool resetCategory(SettingsCategory category);
    
    // Configuration access
    HodlingHogConfig& getConfig() { return config; }
    const HodlingHogConfig& getConfig() const { return config; }
    
    // Individual setting access
    WiFiSettings& getWiFiSettings() { return config.wifi; }
    LightningSettings& getLightningSettings() { return config.lightning; }
    ColdStorageSettings& getColdStorageSettings() { return config.coldStorage; }
    DisplaySettings& getDisplaySettings() { return config.display; }
    PowerSettings& getPowerSettings() { return config.power; }
    SystemSettings& getSystemSettings() { return config.system; }
    
    // Setting modification
    bool setWiFiCredentials(const String& ssid, const String& password);
    bool setLightningToken(const String& token);
    bool setLightningCredentials(const String& token, const String& secret, const String& address);
    bool setLightningWalletCreated(bool created);
    bool setColdStorageAddress(const String& address);
    bool setPrivateKey(const String& key);
    bool setDisplayBrightness(uint8_t brightness);
    bool setSleepTimeout(uint32_t timeout);
    bool setUpdateInterval(uint32_t interval);
    
    // Seed phrase authentication
    bool setSeedPhrase(const String& seedPhrase);
    bool validateSeedPhrase(const String& seedPhrase);
    bool isSeedPhraseSet() const;
    String hashSeedPhrase(const String& seedPhrase);
    String normalizeSeedPhrase(const String& seedPhrase);
    bool isAccountLocked() const;
    void recordFailedLogin();
    void resetLoginAttempts();
    std::vector<String> getBIP39WordList();
    bool isValidBIP39Word(const String& word);
    bool isSeedPhraseValid(const String& seedPhrase);
    
    // Kid-friendly seed phrase generation
    String generateKidFriendlySeedPhrase();
    std::vector<String> getKidFriendlyWordList();
    String getRandomWord();
    
    // Validation
    bool validateWiFiSettings(const WiFiSettings& settings);
    bool validateLightningSettings(const LightningSettings& settings);
    bool validateColdStorageSettings(const ColdStorageSettings& settings);
    bool validateDisplaySettings(const DisplaySettings& settings);
    bool validatePowerSettings(const PowerSettings& settings);
    bool validateSystemSettings(const SystemSettings& settings);
    
    // Import/Export
    String exportConfig(SettingsCategory category = SettingsCategory::ALL);
    bool importConfig(const String& json, SettingsCategory category = SettingsCategory::ALL);
    String exportQRConfig();
    bool importQRConfig(const String& qrData);
    
    // Factory reset and migration
    bool factoryReset();
    bool migrateConfig(uint32_t fromVersion, uint32_t toVersion);
    uint32_t getCurrentConfigVersion() const;
    
    // Change tracking
    bool hasChanges() const { return configChanged; }
    void markChanged() { configChanged = true; }
    void markSaved() { configChanged = false; }
    unsigned long getLastModified() const { return config.lastModified; }
    
    // File management
    bool fileExists(const String& path);
    bool deleteFile(const String& path);
    String readFile(const String& path);
    bool writeFile(const String& path, const String& content);
    std::vector<String> listFiles();
    
    // Error handling
    String getLastError() const { return lastError; }
    void clearError() { lastError = ""; }
    
private:
    HodlingHogConfig config;
    bool initialized;
    bool configChanged;
    String lastError;
    
    // Default configuration
    void setDefaults();
    void setDefaultWiFi();
    void setDefaultLightning();
    void setDefaultColdStorage();
    void setDefaultDisplay();
    void setDefaultPower();
    void setDefaultSystem();
    
    // JSON serialization
    bool configToJson(JsonDocument& doc, SettingsCategory category = SettingsCategory::ALL);
    bool jsonToConfig(const JsonDocument& doc, SettingsCategory category = SettingsCategory::ALL);
    
    // Category-specific JSON handling
    void wifiToJson(JsonObject& obj);
    void lightningToJson(JsonObject& obj);
    void coldStorageToJson(JsonObject& obj);
    void displayToJson(JsonObject& obj);
    void powerToJson(JsonObject& obj);
    void systemToJson(JsonObject& obj);
    
    bool jsonToWifi(const JsonObject& obj);
    bool jsonToLightning(const JsonObject& obj);
    bool jsonToColdStorage(const JsonObject& obj);
    bool jsonToDisplay(const JsonObject& obj);
    bool jsonToPower(const JsonObject& obj);
    bool jsonToSystem(const JsonObject& obj);
    
    // File operations
    String getFilePath(SettingsCategory category);
    bool loadFromFile(const String& path, JsonDocument& doc);
    bool saveToFile(const String& path, const JsonDocument& doc);
    
    // Security and encryption
    String encryptPrivateKey(const String& key);
    String decryptPrivateKey(const String& encryptedKey);
    bool isPrivateKeyEncrypted(const String& key);
    
    // Validation helpers
    bool isValidSSID(const String& ssid);
    bool isValidPassword(const String& password);
    bool isValidApiToken(const String& token);
    bool isValidUrl(const String& url);
    bool isValidAddress(const String& address);
    bool isValidPrivateKey(const String& key);
    bool isValidBrightness(uint8_t brightness);
    bool isValidTimeout(uint32_t timeout);
    
    // Error handling
    void setError(const String& error);
    void logError(const String& operation, const String& error);
    
    // Utility methods
    String generateDeviceId();
    String getCurrentVersion();
    unsigned long getCurrentTimestamp();
    String sanitizeString(const String& input);
    size_t calculateJsonSize(SettingsCategory category);
};

// Global instance
extern SettingsManager settings;

#endif // SETTINGS_H 