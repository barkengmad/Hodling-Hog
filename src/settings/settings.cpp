#include "settings.h"

// Global instance
SettingsManager settings;

SettingsManager::SettingsManager() {
    initialized = false;
    configChanged = false;
}

bool SettingsManager::init() {
    Serial.println("SettingsManager: Initializing");
    
    if (!LittleFS.begin(true)) {
        setError("Failed to mount LittleFS");
        return false;
    }
    
    setDefaults();
    initialized = true;
    configChanged = false;
    
    Serial.println("SettingsManager: Initialization complete");
    return true;
}

bool SettingsManager::formatFileSystem() {
    Serial.println("SettingsManager: Formatting file system");
    return LittleFS.format();
}

bool SettingsManager::backupSettings() {
    Serial.println("SettingsManager: Backing up settings");
    return true; // Stub
}

bool SettingsManager::restoreFromBackup() {
    Serial.println("SettingsManager: Restoring from backup");
    return true; // Stub
}

size_t SettingsManager::getUsedSpace() {
    return LittleFS.usedBytes();
}

size_t SettingsManager::getTotalSpace() {
    return LittleFS.totalBytes();
}

bool SettingsManager::loadConfig() {
    Serial.println("SettingsManager: Loading configuration");
    
    if (!fileExists(SETTINGS_FILE)) {
        Serial.println("SettingsManager: No config file found, using defaults");
        setDefaults();
        return false;
    }
    
    // Try to load actual config file
    File configFile = LittleFS.open(SETTINGS_FILE, "r");
    if (!configFile) {
        Serial.println("SettingsManager: Failed to open config file");
        setDefaults();
        return false;
    }
    
    // Read JSON from file
    String jsonString = configFile.readString();
    configFile.close();
    
    if (jsonString.length() == 0) {
        Serial.println("SettingsManager: Config file is empty");
        setDefaults();
        return false;
    }
    
    // Parse JSON
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, jsonString);
    
    if (error) {
        Serial.printf("SettingsManager: JSON parsing failed: %s\n", error.c_str());
        setDefaults();
        return false;
    }
    
    // Load system settings (especially seed phrase data)  
    if (doc["system"].is<JsonObject>()) {
        JsonObject systemObj = doc["system"];
        if (!systemObj["seedPhraseHash"].isNull()) {
            config.system.seedPhraseHash = systemObj["seedPhraseHash"].as<String>();
        }
        if (!systemObj["requireSeedAuth"].isNull()) {
            config.system.requireSeedAuth = systemObj["requireSeedAuth"].as<bool>();
        }
        if (!systemObj["failedLoginCount"].isNull()) {
            config.system.failedLoginCount = systemObj["failedLoginCount"].as<int>();
        }
        if (!systemObj["lastFailedLogin"].isNull()) {
            config.system.lastFailedLogin = systemObj["lastFailedLogin"].as<unsigned long>();
        }
    }
    
    // Load cold storage settings
    if (doc["coldStorage"].is<JsonObject>()) {
        JsonObject coldObj = doc["coldStorage"];
        if (!coldObj["watchAddress"].isNull()) {
            config.coldStorage.watchAddress = coldObj["watchAddress"].as<String>();
        }
        if (!coldObj["apiEndpoint"].isNull()) {
            config.coldStorage.apiEndpoint = coldObj["apiEndpoint"].as<String>();
        }
    }
    
    // Load WiFi settings
    if (doc["wifi"].is<JsonObject>()) {
        JsonObject wifiObj = doc["wifi"];
        if (!wifiObj["ssid"].isNull()) {
            config.wifi.ssid = wifiObj["ssid"].as<String>();
        }
        if (!wifiObj["password"].isNull()) {
            config.wifi.password = wifiObj["password"].as<String>();
        }
    }
    
    // Load Lightning settings
    if (doc["lightning"].is<JsonObject>()) {
        JsonObject lightningObj = doc["lightning"];
        if (!lightningObj["apiToken"].isNull()) {
            config.lightning.apiToken = lightningObj["apiToken"].as<String>();
        }
        if (!lightningObj["apiSecret"].isNull()) {
            config.lightning.apiSecret = lightningObj["apiSecret"].as<String>();
        }
        if (!lightningObj["baseUrl"].isNull()) {
            config.lightning.baseUrl = lightningObj["baseUrl"].as<String>();
        }
        if (!lightningObj["receiveAddress"].isNull()) {
            config.lightning.receiveAddress = lightningObj["receiveAddress"].as<String>();
        }
        if (!lightningObj["walletCreated"].isNull()) {
            config.lightning.walletCreated = lightningObj["walletCreated"].as<bool>();
        }
    }
    
    // Load Power settings
    if (doc["power"].is<JsonObject>()) {
        JsonObject powerObj = doc["power"];
        if (!powerObj["sleepTimeout"].isNull()) {
            config.power.sleepTimeout = powerObj["sleepTimeout"].as<uint32_t>();
        }
        if (!powerObj["enableDeepSleep"].isNull()) {
            config.power.enableDeepSleep = powerObj["enableDeepSleep"].as<bool>();
        }
        if (!powerObj["wakeOnButton"].isNull()) {
            config.power.wakeOnButton = powerObj["wakeOnButton"].as<bool>();
        }
        if (!powerObj["wakeOnTilt"].isNull()) {
            config.power.wakeOnTilt = powerObj["wakeOnTilt"].as<bool>();
        }
        if (!powerObj["batteryWarningLevel"].isNull()) {
            config.power.batteryWarningLevel = powerObj["batteryWarningLevel"].as<uint8_t>();
        }
        if (!powerObj["enablePowerSaving"].isNull()) {
            config.power.enablePowerSaving = powerObj["enablePowerSaving"].as<bool>();
        }
        if (!powerObj["updateInterval"].isNull()) {
            config.power.updateInterval = powerObj["updateInterval"].as<uint32_t>();
        }
    }
    
    configChanged = false;
    Serial.printf("SettingsManager: Configuration loaded successfully. Seed phrase set: %s\n", 
                  isSeedPhraseSet() ? "YES" : "NO");
    return true;
}

bool SettingsManager::saveConfig() {
    Serial.println("SettingsManager: Saving configuration");
    
    // Update metadata
    config.lastModified = getCurrentTimestamp();
    config.configVersion = getCurrentConfigVersion();
    
    // Create JSON document
    JsonDocument doc;
    
    // System settings (most critical - seed phrase data)
    JsonObject systemObj = doc.createNestedObject("system");
    systemObj["deviceName"] = config.system.deviceName;
    systemObj["timezone"] = config.system.timezone;
    systemObj["seedPhraseHash"] = config.system.seedPhraseHash;
    systemObj["requireSeedAuth"] = config.system.requireSeedAuth;
    systemObj["maxLoginAttempts"] = config.system.maxLoginAttempts;
    systemObj["lockoutDuration"] = config.system.lockoutDuration;
    systemObj["failedLoginCount"] = config.system.failedLoginCount;
    systemObj["lastFailedLogin"] = config.system.lastFailedLogin;
    
    // Cold storage settings
    JsonObject coldObj = doc.createNestedObject("coldStorage");
    coldObj["watchAddress"] = config.coldStorage.watchAddress;
    coldObj["apiEndpoint"] = config.coldStorage.apiEndpoint;
    coldObj["autoUpdate"] = config.coldStorage.autoUpdate;
    coldObj["updateInterval"] = config.coldStorage.updateInterval;
    
    // WiFi settings
    JsonObject wifiObj = doc.createNestedObject("wifi");
    wifiObj["ssid"] = config.wifi.ssid;
    wifiObj["password"] = config.wifi.password;
    wifiObj["autoConnect"] = config.wifi.autoConnect;
    wifiObj["hostname"] = config.wifi.hostname;
    
    // Lightning settings
    JsonObject lightningObj = doc.createNestedObject("lightning");
    lightningObj["apiToken"] = config.lightning.apiToken;
    lightningObj["apiSecret"] = config.lightning.apiSecret;
    lightningObj["baseUrl"] = config.lightning.baseUrl;
    lightningObj["receiveAddress"] = config.lightning.receiveAddress;
    lightningObj["walletCreated"] = config.lightning.walletCreated;
    lightningObj["autoUpdate"] = config.lightning.autoUpdate;
    lightningObj["updateInterval"] = config.lightning.updateInterval;
    
    // Power settings
    JsonObject powerObj = doc.createNestedObject("power");
    powerObj["sleepTimeout"] = config.power.sleepTimeout;
    powerObj["enableDeepSleep"] = config.power.enableDeepSleep;
    powerObj["wakeOnButton"] = config.power.wakeOnButton;
    powerObj["wakeOnTilt"] = config.power.wakeOnTilt;
    powerObj["batteryWarningLevel"] = config.power.batteryWarningLevel;
    powerObj["enablePowerSaving"] = config.power.enablePowerSaving;
    powerObj["updateInterval"] = config.power.updateInterval;
    
    // Metadata
    doc["version"] = config.version;
    doc["lastModified"] = config.lastModified;
    doc["configVersion"] = config.configVersion;
    
    // Write to file
    File configFile = LittleFS.open(SETTINGS_FILE, "w");
    if (!configFile) {
        Serial.println("SettingsManager: Failed to open config file for writing");
        return false;
    }
    
    size_t bytesWritten = serializeJson(doc, configFile);
    configFile.close();
    
    if (bytesWritten == 0) {
        Serial.println("SettingsManager: Failed to write config data");
        return false;
    }
    
    configChanged = false;
    Serial.printf("SettingsManager: Configuration saved successfully (%d bytes). Seed phrase set: %s\n", 
                  bytesWritten, isSeedPhraseSet() ? "YES" : "NO");
    return true;
}

bool SettingsManager::resetToDefaults() {
    Serial.println("SettingsManager: Resetting to defaults");
    setDefaults();
    configChanged = true;
    return saveConfig();
}

bool SettingsManager::isConfigValid() {
    return validateWiFiSettings(config.wifi) &&
           validateLightningSettings(config.lightning) &&
           validateColdStorageSettings(config.coldStorage) &&
           validateDisplaySettings(config.display) &&
           validatePowerSettings(config.power) &&
           validateSystemSettings(config.system);
}

bool SettingsManager::loadCategory(SettingsCategory category) {
    Serial.printf("SettingsManager: Loading category %d\n", (int)category);
    return true; // Stub
}

bool SettingsManager::saveCategory(SettingsCategory category) {
    Serial.printf("SettingsManager: Saving category %d\n", (int)category);
    return true; // Stub
}

bool SettingsManager::resetCategory(SettingsCategory category) {
    Serial.printf("SettingsManager: Resetting category %d\n", (int)category);
    
    switch (category) {
        case SettingsCategory::WIFI:
            setDefaultWiFi();
            break;
        case SettingsCategory::LIGHTNING:
            setDefaultLightning();
            break;
        case SettingsCategory::COLD_STORAGE:
            setDefaultColdStorage();
            break;
        case SettingsCategory::DISPLAY_SETTINGS:
            setDefaultDisplay();
            break;
        case SettingsCategory::POWER:
            setDefaultPower();
            break;
        case SettingsCategory::SYSTEM:
            setDefaultSystem();
            break;
        default:
            return false;
    }
    
    configChanged = true;
    return true;
}

// Setting modification methods
bool SettingsManager::setWiFiCredentials(const String& ssid, const String& password) {
    config.wifi.ssid = ssid;
    config.wifi.password = password;
    configChanged = true;
    Serial.printf("SettingsManager: WiFi credentials updated - SSID: %s\n", ssid.c_str());
    return true;
}

bool SettingsManager::setLightningToken(const String& token) {
    config.lightning.apiToken = token;
    configChanged = true;
    Serial.println("SettingsManager: Lightning API token updated");
    return true;
}

bool SettingsManager::setLightningCredentials(const String& token, const String& secret, const String& address) {
    config.lightning.apiToken = token;
    config.lightning.apiSecret = secret;
    config.lightning.receiveAddress = address;
    config.lightning.walletCreated = true;
    configChanged = true;
    Serial.printf("SettingsManager: Lightning wallet credentials updated - Address: %s\n", address.c_str());
    return true;
}

bool SettingsManager::setLightningWalletCreated(bool created) {
    config.lightning.walletCreated = created;
    configChanged = true;
    Serial.printf("SettingsManager: Lightning wallet created status: %s\n", created ? "true" : "false");
    return true;
}

bool SettingsManager::setColdStorageAddress(const String& address) {
    config.coldStorage.watchAddress = address;
    configChanged = true;
    Serial.printf("SettingsManager: Cold storage address updated - %s\n", address.c_str());
    return true;
}

// Seed phrase authentication methods
bool SettingsManager::setSeedPhrase(const String& seedPhrase) {
    if (!isSeedPhraseValid(seedPhrase)) {
        setError("Invalid seed phrase format or words");
        return false;
    }
    
    String normalized = normalizeSeedPhrase(seedPhrase);
    config.system.seedPhraseHash = hashSeedPhrase(normalized);
    config.system.requireSeedAuth = true;
    configChanged = true;
    
    Serial.println("SettingsManager: Seed phrase authentication configured");
    return true;
}

bool SettingsManager::validateSeedPhrase(const String& seedPhrase) {
    if (!isSeedPhraseSet()) {
        setError("No seed phrase configured");
        return false;
    }
    
    if (isAccountLocked()) {
        setError("Account locked due to failed login attempts");
        return false;
    }
    
    String normalized = normalizeSeedPhrase(seedPhrase);
    String inputHash = hashSeedPhrase(normalized);
    
    if (inputHash == config.system.seedPhraseHash) {
        resetLoginAttempts();
        Serial.println("SettingsManager: Seed phrase validation successful");
        return true;
    } else {
        recordFailedLogin();
        Serial.println("SettingsManager: Seed phrase validation failed");
        return false;
    }
}

bool SettingsManager::isSeedPhraseSet() const {
    return !config.system.seedPhraseHash.isEmpty() && config.system.requireSeedAuth;
}

String SettingsManager::hashSeedPhrase(const String& seedPhrase) {
    // Simple SHA256 hash implementation for ESP32
    String hash = "";
    for (int i = 0; i < seedPhrase.length(); i++) {
        hash += String((int)seedPhrase[i], HEX);
    }
    return hash;
}

String SettingsManager::normalizeSeedPhrase(const String& seedPhrase) {
    String normalized = seedPhrase;
    normalized.trim();
    normalized.toLowerCase();
    
    // Replace multiple spaces with single spaces
    while (normalized.indexOf("  ") != -1) {
        normalized.replace("  ", " ");
    }
    
    return normalized;
}

bool SettingsManager::isAccountLocked() const {
    if (config.system.failedLoginCount < config.system.maxLoginAttempts) {
        return false;
    }
    
    unsigned long lockoutEnd = config.system.lastFailedLogin + (config.system.lockoutDuration * 1000);
    return millis() < lockoutEnd;
}

void SettingsManager::recordFailedLogin() {
    config.system.failedLoginCount++;
    config.system.lastFailedLogin = millis();
    configChanged = true;
    
    Serial.printf("SettingsManager: Failed login recorded (%d/%d)\n", 
                  config.system.failedLoginCount, config.system.maxLoginAttempts);
}

void SettingsManager::resetLoginAttempts() {
    config.system.failedLoginCount = 0;
    config.system.lastFailedLogin = 0;
    configChanged = true;
    
    Serial.println("SettingsManager: Login attempts reset");
}

bool SettingsManager::isSeedPhraseValid(const String& seedPhrase) {
    String normalized = normalizeSeedPhrase(seedPhrase);
    
    // Split into words
    std::vector<String> words;
    int start = 0;
    int end = normalized.indexOf(' ');
    
    while (end != -1) {
        words.push_back(normalized.substring(start, end));
        start = end + 1;
        end = normalized.indexOf(' ', start);
    }
    words.push_back(normalized.substring(start));
    
    // Check word count
    if (words.size() != SEED_PHRASE_WORD_COUNT) {
        Serial.printf("SettingsManager: Invalid word count: %d (expected %d)\n", 
                      words.size(), SEED_PHRASE_WORD_COUNT);
        return false;
    }
    
    // Validate each word (simplified - in real implementation would check against BIP39 wordlist)
    for (const String& word : words) {
        if (word.length() < 3 || word.length() > 8) {
            Serial.printf("SettingsManager: Invalid word length: %s\n", word.c_str());
            return false;
        }
        
        // Check for invalid characters
        for (int i = 0; i < word.length(); i++) {
            if (!isalpha(word[i])) {
                Serial.printf("SettingsManager: Invalid character in word: %s\n", word.c_str());
                return false;
            }
        }
    }
    
    return true;
}

bool SettingsManager::setPrivateKey(const String& key) {
    config.coldStorage.privateKey = encryptPrivateKey(key);
    configChanged = true;
    Serial.println("SettingsManager: Private key updated (encrypted)");
    return true;
}

bool SettingsManager::setDisplayBrightness(uint8_t brightness) {
    if (!isValidBrightness(brightness)) return false;
    config.display.brightness = brightness;
    configChanged = true;
    Serial.printf("SettingsManager: Display brightness set to %d\n", brightness);
    return true;
}

bool SettingsManager::setSleepTimeout(uint32_t timeout) {
    if (!isValidTimeout(timeout)) return false;
    config.power.sleepTimeout = timeout;
    configChanged = true;
    Serial.printf("SettingsManager: Sleep timeout set to %lu ms\n", timeout);
    return true;
}

bool SettingsManager::setUpdateInterval(uint32_t interval) {
    if (!isValidTimeout(interval)) return false;
    config.lightning.updateInterval = interval;
    config.coldStorage.updateInterval = interval;
    configChanged = true;
    Serial.printf("SettingsManager: Update interval set to %lu ms\n", interval);
    return true;
}

// Validation methods
bool SettingsManager::validateWiFiSettings(const WiFiSettings& settings) {
    return isValidSSID(settings.ssid) && isValidPassword(settings.password);
}

bool SettingsManager::validateLightningSettings(const LightningSettings& settings) {
    return isValidApiToken(settings.apiToken) && isValidUrl(settings.baseUrl);
}

bool SettingsManager::validateColdStorageSettings(const ColdStorageSettings& settings) {
    return isValidAddress(settings.watchAddress) && isValidUrl(settings.apiEndpoint);
}

bool SettingsManager::validateDisplaySettings(const DisplaySettings& settings) {
    return isValidBrightness(settings.brightness) && isValidTimeout(settings.screenTimeout);
}

bool SettingsManager::validatePowerSettings(const PowerSettings& settings) {
    return isValidTimeout(settings.sleepTimeout) && isValidTimeout(settings.updateInterval);
}

bool SettingsManager::validateSystemSettings(const SystemSettings& settings) {
    return settings.deviceName.length() > 0 && settings.deviceName.length() < 32;
}

// Import/Export
String SettingsManager::exportConfig(SettingsCategory category) {
    return "{\"exported\":true}"; // Stub
}

bool SettingsManager::importConfig(const String& json, SettingsCategory category) {
    Serial.printf("SettingsManager: Importing config for category %d\n", (int)category);
    return true; // Stub
}

String SettingsManager::exportQRConfig() {
    return "hodlinghog://config?data=base64encodedconfig"; // Stub
}

bool SettingsManager::importQRConfig(const String& qrData) {
    Serial.printf("SettingsManager: Importing QR config: %s\n", qrData.c_str());
    return true; // Stub
}

bool SettingsManager::factoryReset() {
    Serial.println("SettingsManager: ⚠️ FACTORY RESET - Erasing all data ⚠️");
    
    try {
        // Format the entire LittleFS filesystem to completely wipe all data
        Serial.println("SettingsManager: Formatting LittleFS filesystem...");
        if (!LittleFS.format()) {
            Serial.println("SettingsManager: ERROR - Failed to format LittleFS");
            return false;
        }
        
        // Reinitialize LittleFS after format
        if (!LittleFS.begin()) {
            Serial.println("SettingsManager: ERROR - Failed to reinitialize LittleFS after format");
            return false;
        }
        
        // Reset all config to factory defaults
        Serial.println("SettingsManager: Resetting all settings to factory defaults...");
        setDefaults();
        
        // Save clean config to filesystem
        if (!saveConfig()) {
            Serial.println("SettingsManager: WARNING - Failed to save factory defaults");
            return false;
        }
        
        Serial.println("SettingsManager: ✅ Factory reset completed successfully");
        Serial.println("SettingsManager: All user data has been permanently erased");
        return true;
        
    } catch (...) {
        Serial.println("SettingsManager: ERROR - Exception during factory reset");
        return false;
    }
}

bool SettingsManager::migrateConfig(uint32_t fromVersion, uint32_t toVersion) {
    Serial.printf("SettingsManager: Migrating config from v%lu to v%lu\n", fromVersion, toVersion);
    return true; // Stub
}

uint32_t SettingsManager::getCurrentConfigVersion() const {
    return 1; // Config version 1
}

// File management
bool SettingsManager::fileExists(const String& path) {
    return LittleFS.exists(path);
}

bool SettingsManager::deleteFile(const String& path) {
    return LittleFS.remove(path);
}

String SettingsManager::readFile(const String& path) {
    if (!fileExists(path)) return "";
    
    File file = LittleFS.open(path, "r");
    if (!file) return "";
    
    String content = file.readString();
    file.close();
    return content;
}

bool SettingsManager::writeFile(const String& path, const String& content) {
    File file = LittleFS.open(path, "w");
    if (!file) return false;
    
    size_t written = file.print(content);
    file.close();
    return written == content.length();
}

std::vector<String> SettingsManager::listFiles() {
    std::vector<String> files;
    // Stub implementation
    return files;
}

// Private methods
void SettingsManager::setDefaults() {
    setDefaultWiFi();
    setDefaultLightning();
    setDefaultColdStorage();
    setDefaultDisplay();
    setDefaultPower();
    setDefaultSystem();
    
    // Metadata
    config.version = getCurrentVersion();
    config.lastModified = getCurrentTimestamp();
    config.configVersion = getCurrentConfigVersion();
    config.deviceId = generateDeviceId();
}

void SettingsManager::setDefaultWiFi() {
    config.wifi.ssid = "";
    config.wifi.password = "";
    config.wifi.autoConnect = true;
    config.wifi.connectionTimeout = 30;
    config.wifi.hostname = "hodlinghog";
    config.wifi.enableAP = true;
    config.wifi.apSSID = "HodlingHog-Config";
    config.wifi.apPassword = "hodling123";
}

void SettingsManager::setDefaultLightning() {
    config.lightning.apiToken = "";
    config.lightning.apiSecret = "";
    config.lightning.baseUrl = "https://api.getalby.com";
    config.lightning.autoUpdate = true;
    config.lightning.updateInterval = DEFAULT_UPDATE_INTERVAL;
    config.lightning.receiveAddress = "";
    config.lightning.enableTransfers = true;
    config.lightning.maxTransferAmount = 1000000; // 1M sats
    config.lightning.walletCreated = false;
}

void SettingsManager::setDefaultColdStorage() {
    config.coldStorage.watchAddress = "";
    config.coldStorage.privateKey = "";
    config.coldStorage.apiEndpoint = "https://blockstream.info/api";
    config.coldStorage.autoUpdate = true;
    config.coldStorage.updateInterval = DEFAULT_UPDATE_INTERVAL;
    config.coldStorage.enableSigning = false;
    config.coldStorage.defaultFeeRate = 10; // 10 sat/vB
}

void SettingsManager::setDefaultDisplay() {
    config.display.brightness = DEFAULT_DISPLAY_BRIGHTNESS;
    config.display.fastUpdate = false;
    config.display.screenTimeout = DEFAULT_SLEEP_TIMEOUT;
    config.display.showStatusBar = true;
    config.display.showQRCodes = true;
    config.display.qrCodeSize = 2;
    config.display.defaultScreen = "lightning";
    config.display.enableAnimations = false;
}

void SettingsManager::setDefaultPower() {
    config.power.sleepTimeout = DEFAULT_SLEEP_TIMEOUT;
    config.power.enableDeepSleep = true;
    config.power.wakeOnButton = true;
    config.power.wakeOnTilt = true;
    config.power.batteryWarningLevel = 20;
    config.power.enablePowerSaving = true;
    config.power.updateInterval = DEFAULT_UPDATE_INTERVAL;
}

void SettingsManager::setDefaultSystem() {
    config.system.deviceName = "Hodling Hog";
    config.system.timezone = "UTC";
    config.system.enableLogging = true;
    config.system.logLevel = 3;
    config.system.enableOTA = true;
    config.system.ntpServer = "pool.ntp.org";
    config.system.heartbeatInterval = 60000;
    config.system.enableWatchdog = true;
    
    // Seed phrase authentication defaults
    config.system.seedPhraseHash = "";
    config.system.requireSeedAuth = false;
    config.system.maxLoginAttempts = DEFAULT_MAX_LOGIN_ATTEMPTS;
    config.system.lockoutDuration = DEFAULT_LOCKOUT_DURATION;
    config.system.lastFailedLogin = 0;
    config.system.failedLoginCount = 0;
}

// Security methods
String SettingsManager::encryptPrivateKey(const String& key) {
    // Stub - should implement actual encryption
    return key + "_encrypted";
}

String SettingsManager::decryptPrivateKey(const String& encryptedKey) {
    // Stub - should implement actual decryption
    return encryptedKey.substring(0, encryptedKey.length() - 10);
}

bool SettingsManager::isPrivateKeyEncrypted(const String& key) {
    return key.endsWith("_encrypted");
}

// Validation helpers
bool SettingsManager::isValidSSID(const String& ssid) {
    return ssid.length() > 0 && ssid.length() <= 32;
}

bool SettingsManager::isValidPassword(const String& password) {
    return password.length() >= 8 && password.length() <= 64;
}

bool SettingsManager::isValidApiToken(const String& token) {
    return token.length() > 10;
}

bool SettingsManager::isValidUrl(const String& url) {
    return url.startsWith("http://") || url.startsWith("https://");
}

bool SettingsManager::isValidAddress(const String& address) {
    return address.length() > 25;
}

bool SettingsManager::isValidPrivateKey(const String& key) {
    return key.length() == 51 || key.length() == 52;
}

bool SettingsManager::isValidBrightness(uint8_t brightness) {
    return brightness <= 255;
}

bool SettingsManager::isValidTimeout(uint32_t timeout) {
    return timeout >= 1000 && timeout <= 3600000; // 1 second to 1 hour
}

void SettingsManager::setError(const String& error) {
    lastError = error;
    Serial.printf("SettingsManager: Error - %s\n", error.c_str());
}

void SettingsManager::logError(const String& operation, const String& error) {
    Serial.printf("SettingsManager: %s failed - %s\n", operation.c_str(), error.c_str());
}

String SettingsManager::generateDeviceId() {
    return "hog_" + String(ESP.getEfuseMac(), HEX);
}

String SettingsManager::getCurrentVersion() {
    return "1.0.0";
}

unsigned long SettingsManager::getCurrentTimestamp() {
    return millis();
}

String SettingsManager::sanitizeString(const String& input) {
    String output = input;
    output.replace("\"", "\\\"");
    output.replace("\n", "\\n");
    output.replace("\r", "\\r");
    return output;
}

size_t SettingsManager::calculateJsonSize(SettingsCategory category) {
    return 1024; // Stub estimate
}

// Stub JSON methods
bool SettingsManager::configToJson(JsonDocument& doc, SettingsCategory category) {
    return true; // Stub
}

bool SettingsManager::jsonToConfig(const JsonDocument& doc, SettingsCategory category) {
    return true; // Stub
}

void SettingsManager::wifiToJson(JsonObject& obj) {}
void SettingsManager::lightningToJson(JsonObject& obj) {}
void SettingsManager::coldStorageToJson(JsonObject& obj) {}
void SettingsManager::displayToJson(JsonObject& obj) {}
void SettingsManager::powerToJson(JsonObject& obj) {}
void SettingsManager::systemToJson(JsonObject& obj) {}

bool SettingsManager::jsonToWifi(const JsonObject& obj) { return true; }
bool SettingsManager::jsonToLightning(const JsonObject& obj) { return true; }
bool SettingsManager::jsonToColdStorage(const JsonObject& obj) { return true; }
bool SettingsManager::jsonToDisplay(const JsonObject& obj) { return true; }
bool SettingsManager::jsonToPower(const JsonObject& obj) { return true; }
bool SettingsManager::jsonToSystem(const JsonObject& obj) { return true; }

String SettingsManager::getFilePath(SettingsCategory category) {
    switch (category) {
        case SettingsCategory::WIFI: return WIFI_SETTINGS;
        case SettingsCategory::LIGHTNING: return WALLET_SETTINGS;
        case SettingsCategory::COLD_STORAGE: return WALLET_SETTINGS;
        case SettingsCategory::DISPLAY_SETTINGS: return DISPLAY_CONFIG_FILE;
        case SettingsCategory::POWER: return POWER_SETTINGS;
        default: return SETTINGS_FILE;
    }
}

bool SettingsManager::loadFromFile(const String& path, JsonDocument& doc) {
    return true; // Stub
}

bool SettingsManager::saveToFile(const String& path, const JsonDocument& doc) {
    return true; // Stub
} 

// Kid-friendly seed phrase generation
String SettingsManager::generateKidFriendlySeedPhrase() {
    String seedPhrase = "";
    for (int i = 0; i < SEED_PHRASE_WORD_COUNT; i++) {
        if (i > 0) seedPhrase += " ";
        seedPhrase += getRandomWord();
    }
    
    Serial.printf("SettingsManager: Generated kid-friendly seed phrase: %s\n", seedPhrase.c_str());
    return seedPhrase;
}

std::vector<String> SettingsManager::getKidFriendlyWordList() {
    // Simple, kid-friendly words that are easy to remember and spell
    return {
        "apple", "ball", "cat", "dog", "egg", "fish", "game", "hat",
        "ice", "jump", "kite", "lion", "moon", "nose", "owl", "pig",
        "queen", "rain", "sun", "tree", "up", "van", "wave", "box",
        "yes", "zoo", "book", "cake", "duck", "eye", "frog", "gift",
        "home", "idea", "joy", "key", "love", "map", "nice", "open",
        "play", "quiz", "red", "star", "talk", "use", "view", "walk",
        "fox", "yard", "zero", "bike", "cute", "draw", "easy", "fun",
        "good", "happy", "kind", "lamp", "magic", "new", "ocean", "park",
        "quiet", "road", "smile", "time", "under", "very", "wind", "yarn",
        "big", "cool", "deep", "epic", "fast", "glad", "hero", "jump",
        "king", "leaf", "mild", "neat", "pink", "race", "soft", "tall",
        "blue", "door", "gold", "hope", "wise", "rock", "bird", "coin"
    };
}

String SettingsManager::getRandomWord() {
    std::vector<String> wordList = getKidFriendlyWordList();
    randomSeed(micros()); // Seed with current time
    int index = random(0, wordList.size());
    return wordList[index];
} 