#include <Arduino.h>
#include <WiFi.h>
#include <esp_sleep.h>
#include <esp_system.h>
#include <LittleFS.h>

// Include all module headers
#include "secrets.h"
#include "core/core.h"
#include "display/display.h"
#include "input/input.h"
#include "wallet/wallet.h"
#include "cold/cold.h"
#include "web/web.h"
#include "settings/settings.h"
#include "utils/utils.h"

// Application constants
#define FIRMWARE_VERSION        "1.0.0"
#define CONFIG_VERSION          1
#define BOOT_TIMEOUT            10000    // 10 seconds boot timeout
#define WIFI_TIMEOUT            5000     // 5 seconds WiFi timeout (non-blocking)
#define UPDATE_INTERVAL         300000   // 5 minutes balance update interval
// Sleep timeout is now configurable in settings (default 3 minutes)
#define CONFIG_MODE_TIMEOUT     600000   // 10 minutes config mode timeout

// Global state variables
SystemState currentState = SystemState::BOOT;
unsigned long lastUpdateTime = 0;
unsigned long lastInputTime = 0;
unsigned long bootStartTime = 0;
bool wifiConnected = false;
bool configModeActive = false;
bool balanceUpdateInProgress = false;

// Forward declarations
void initializeSystem();
void handleWiFiConnection();
void updateBalances();
void handleInputEvents();
void checkPowerManagement();
void handleSystemError(const String& error);
void logSystemStatus();

// Setup function - called once at startup
void setup() {
    Serial.begin(115200);
    
    // Add delay for serial monitor
    delay(1000);
    
    Serial.println();
    Serial.println("========================================");
    Serial.println("     Hodling Hog Bitcoin Piggy Bank");
    Serial.println("     Saving your future, one oink at a time!");
    Serial.println("     Version: " FIRMWARE_VERSION);
    Serial.println("========================================");
    
    bootStartTime = millis();
    
    // Initialize core system
    initializeSystem();
    
    // Initialize all modules
    Serial.println("Initializing modules...");
    
    // Initialize settings first (needed by other modules)
    if (!settings.init()) {
        Serial.println("ERROR: Settings initialization failed");
        handleSystemError("Settings initialization failed");
        return;
    }
    
    // Load configuration
    if (!settings.loadConfig()) {
        Serial.println("WARNING: Using default configuration");
        settings.resetToDefaults();
        settings.saveConfig();
    }
    
    // Initialize utilities
    utils.init();
    utils.enableDebug(true);
    
    // Initialize display
    displayMgr.init();
    displayMgr.showBootScreen();
    Serial.println("Display initialized");
    
    // Initialize input manager
    inputMgr.init();
    inputMgr.setButtonCallback([](InputEvent event) {
        lastInputTime = millis();
        
        switch (event) {
            case InputEvent::BUTTON_SHORT_PRESS:
                core.cycleScreen();
                break;
            case InputEvent::BUTTON_LONG_PRESS:
                core.enterConfigMode();
                break;
            case InputEvent::BUTTON_DOUBLE_CLICK:
                core.updateBalances();
                break;
        }
    });
    
    inputMgr.setTiltCallback([](InputEvent event) {
        lastInputTime = millis();
        if (event == InputEvent::TILT_ACTIVATED) {
            core.wakeUp(WakeReason::TILT_SWITCH);
            // Update balances immediately on tilt to show fresh data
            if (wifiConnected) {
                Serial.println("Triggering balance update on tilt");
                updateBalances();
            }
        }
    });
    Serial.println("Input manager initialized");
    
    // Initialize core state machine
    core.init();
    Serial.println("Core state machine initialized");
    
    // Initialize wallet modules
    lightningWallet.init();
    lightningWallet.setBaseUrl(WOS_API_BASE_URL);
    
    // Initialize or create WoS wallet if needed
    if (lightningWallet.createWalletIfNeeded()) {
        Serial.println("Lightning wallet initialized successfully");
    } else {
        Serial.println("Lightning wallet initialization failed - will retry on login");
    }
    
    coldStorage.init();
    // Load address from settings instead of hardcoded value
    String savedAddress = settings.getConfig().coldStorage.watchAddress;
    if (!savedAddress.isEmpty()) {
        coldStorage.setAddress(savedAddress);
        Serial.printf("Cold storage loaded saved address: %s\n", savedAddress.c_str());
    } else {
        Serial.println("Cold storage: No saved address found");
    }
    coldStorage.setApiEndpoint(BLOCKSTREAM_API);
    Serial.println("Cold storage initialized");
    
    // Initialize web interface
    webInterface.init();
    Serial.println("Web interface initialized");
    
    // Setup Wi-Fi with non-blocking pattern
    WiFi.mode(WIFI_STA);
    WiFi.setHostname(DEVICE_NAME);
    
    // Check if we should enter configuration mode immediately
    if (inputMgr.isButtonPressed()) {
        Serial.println("Button pressed during boot - entering config mode");
        core.handleStateTransition(SystemState::CONFIG_MODE);
    } else {
        // Normal startup - try to connect to WiFi
        core.handleStateTransition(SystemState::WIFI_CONNECTING);
    }
    
    Serial.printf("Boot completed in %lums\n", millis() - bootStartTime);
    Serial.println("========================================");
}

// Main loop - called repeatedly
void loop() {
    // Update all modules
    core.loop();
    inputMgr.loop();
    webInterface.loop();
    
    // Handle input events
    handleInputEvents();
    
    // Handle WiFi connection management
    handleWiFiConnection();
    
    // Check if we need to update balances
    if (core.getCurrentState() == SystemState::WIFI_CONNECTED && 
        (millis() - lastUpdateTime > UPDATE_INTERVAL || lastUpdateTime == 0)) {
        updateBalances();
    }
    
    // Check power management
    checkPowerManagement();
    
    // Periodic status logging
    static unsigned long lastStatusLog = 0;
    if (millis() - lastStatusLog > 60000) { // Every minute
        logSystemStatus();
        lastStatusLog = millis();
    }
    
    // Yield to other tasks
    yield();
    delay(10);
}

// Initialize core system components
void initializeSystem() {
    // Initialize file system
    if (!LittleFS.begin(true)) {
        Serial.println("ERROR: LittleFS mount failed");
        handleSystemError("File system initialization failed");
        return;
    }
    Serial.println("LittleFS mounted successfully");
    
    // Check wake reason
    esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();
    
    switch (wakeup_reason) {
        case ESP_SLEEP_WAKEUP_EXT0:
            Serial.println("Wake up from button press");
            break;
        case ESP_SLEEP_WAKEUP_EXT1:
            Serial.println("Wake up from tilt switch");
            break;
        case ESP_SLEEP_WAKEUP_TIMER:
            Serial.println("Wake up from timer");
            break;
        case ESP_SLEEP_WAKEUP_TOUCHPAD:
            Serial.println("Wake up from touchpad");
            break;
        case ESP_SLEEP_WAKEUP_ULP:
            Serial.println("Wake up from ULP program");
            break;
        default:
            Serial.printf("Wake up from reset: %d\n", wakeup_reason);
            break;
    }
    
    // Setup deep sleep wake sources
    esp_sleep_enable_ext0_wakeup(GPIO_NUM_0, 0);  // Button (active low)
    esp_sleep_enable_ext1_wakeup(1ULL << GPIO_NUM_2, ESP_EXT1_WAKEUP_ANY_HIGH); // Tilt switch
    
    Serial.println("System initialization completed");
}

// Handle WiFi connection with non-blocking pattern
void handleWiFiConnection() {
    static unsigned long wifiStartTime = 0;
    static bool wifiConnecting = false;
    
    switch (core.getCurrentState()) {
        case SystemState::WIFI_CONNECTING:
            if (!wifiConnecting) {
                Serial.println("Starting WiFi connection...");
                WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
                wifiStartTime = millis();
                wifiConnecting = true;
                displayMgr.showWiFiStatus(false, "Connecting...");
            }
            
            // Check connection status
            if (WiFi.status() == WL_CONNECTED) {
                wifiConnected = true;
                wifiConnecting = false;
                Serial.printf("WiFi connected! IP: %s\n", WiFi.localIP().toString().c_str());
                
                // Initialize NTP
                utils.initNTP();
                
                // Start web interface
                webInterface.start();
                
                core.handleStateTransition(SystemState::WIFI_CONNECTED);
                
                // Show IP address prominently for 5 seconds
                displayMgr.showIPAddress(WiFi.localIP().toString(), 5000);
                
                // Update lastUpdateTime to prevent immediate balance update
                lastUpdateTime = millis();
                
            } else if (millis() - wifiStartTime > WIFI_TIMEOUT) {
                // WiFi timeout - go offline
                Serial.println("WiFi connection timeout - going offline");
                wifiConnected = false;
                wifiConnecting = false;
                WiFi.disconnect();
                core.handleStateTransition(SystemState::OFFLINE);
                displayMgr.showWiFiStatus(false, "Offline");
            }
            break;
            
        case SystemState::WIFI_CONNECTED:
            // Monitor connection status
            if (WiFi.status() != WL_CONNECTED) {
                Serial.println("WiFi connection lost");
                wifiConnected = false;
                core.handleStateTransition(SystemState::WIFI_CONNECTING);
            }
            break;
            
        case SystemState::CONFIG_MODE:
            if (!webInterface.isAPMode()) {
                Serial.println("Starting configuration AP mode");
                webInterface.startAPMode();
                displayMgr.showConfigInfo(webInterface.getAPIP());
            }
            break;
    }
}

// Update wallet balances
void updateBalances() {
    if (balanceUpdateInProgress) {
        return;
    }
    
    Serial.println("Updating balances...");
    balanceUpdateInProgress = true;
    core.handleStateTransition(SystemState::UPDATING_BALANCES);
    
    BalanceData balances = {};
    balances.lastUpdate = millis();
    
    // Update Lightning balance if connected
    if (wifiConnected) {
        if (lightningWallet.updateBalance()) {
            LightningBalance lnBalance = lightningWallet.getBalance();
            balances.lightningBalance = lnBalance.total;
            balances.lightningValid = true;
            Serial.printf("Lightning balance: %llu sats\n", balances.lightningBalance);
        } else {
            Serial.printf("Lightning balance update failed: %s\n", lightningWallet.getLastError().c_str());
            balances.lightningValid = false;
        }
        
        // Update cold storage balance
        if (coldStorage.updateBalance()) {
            ColdBalance coldBalance = coldStorage.getBalance();
            balances.coldBalance = coldBalance.total;
            balances.coldValid = true;
            Serial.printf("Cold storage balance: %llu sats\n", balances.coldBalance);
        } else {
            Serial.printf("Cold storage balance update failed: %s\n", coldStorage.getLastError().c_str());
            balances.coldValid = false;
        }
    }
    
    // Calculate total balance
    balances.totalBalance = balances.lightningBalance + balances.coldBalance;
    
    // Update display
    displayMgr.updateBalances(balances);
    
    // Update QR codes
    QRData qrData = {};
    qrData.lightningAddress = lightningWallet.getReceiveAddress();
    qrData.coldAddress = coldStorage.getWatchAddress();
    displayMgr.updateQRData(qrData);
    
    lastUpdateTime = millis();
    balanceUpdateInProgress = false;
    
    // Return to appropriate display state
    if (wifiConnected) {
        core.handleStateTransition(SystemState::DISPLAYING_LIGHTNING);
    } else {
        core.handleStateTransition(SystemState::OFFLINE);
    }
    
    Serial.printf("Balance update completed. Total: %llu sats\n", balances.totalBalance);
}

// Handle input events
void handleInputEvents() {
    InputEvent event = inputMgr.getLastEvent();
    if (event != InputEvent::NONE) {
        lastInputTime = millis();
        
        switch (event) {
            case InputEvent::WAKE_FROM_SLEEP:
                Serial.println("Device woke from sleep");
                core.wakeUp(WakeReason::BUTTON_PRESS);
                // Update balances on wake up to show fresh data
                if (wifiConnected) {
                    Serial.println("Triggering balance update on wake up");
                    updateBalances();
                }
                break;
                
            default:
                // Button and tilt events are handled in callbacks
                break;
        }
        
        inputMgr.clearEvents();
    }
}

// Check power management and sleep conditions
void checkPowerManagement() {
    static unsigned long lastActivity = 0;
    
    // Update last activity time
    if (lastInputTime > lastActivity) {
        lastActivity = lastInputTime;
    }
    
    // Check if we should enter sleep mode
    bool shouldSleep = false;
    
    switch (core.getCurrentState()) {
        case SystemState::DISPLAYING_LIGHTNING:
        case SystemState::DISPLAYING_COLD:
        case SystemState::DISPLAYING_COMBINED:
        case SystemState::OFFLINE:
            // Use configurable sleep timeout from settings
            if (millis() - lastActivity > settings.getConfig().power.sleepTimeout) {
                shouldSleep = true;
            }
            break;
            
        case SystemState::CONFIG_MODE:
            if (millis() - lastActivity > CONFIG_MODE_TIMEOUT) {
                Serial.println("Config mode timeout - exiting");
                webInterface.stopAPMode();
                core.handleStateTransition(SystemState::WIFI_CONNECTING);
            }
            break;
            
        default:
            // Don't sleep in other states
            break;
    }
    
    if (shouldSleep) {
        Serial.println("Entering sleep mode");
        core.enterSleepMode();
        
        // Prepare for deep sleep
        displayMgr.sleep();
        WiFi.disconnect();
        webInterface.stop();
        
        // Configure wake sources
        inputMgr.setupDeepSleepWakeup();
        
        Serial.println("Going to deep sleep...");
        Serial.flush();
        
        // Enter deep sleep
        esp_deep_sleep_start();
    }
}

// Handle system errors
void handleSystemError(const String& error) {
    Serial.printf("SYSTEM ERROR: %s\n", error.c_str());
    
    // Show error on display
    displayMgr.showErrorScreen(error);
    
    // Log error to settings if possible
    if (settings.init()) {
        utils.logMessage("ERROR", error);
    }
    
    // Wait for user input or restart
    unsigned long errorStart = millis();
    while (millis() - errorStart < 30000) { // 30 second timeout
        inputMgr.loop();
        if (inputMgr.getLastEvent() != InputEvent::NONE) {
            // User pressed button - restart
            Serial.println("User input detected - restarting");
            utils.restart();
        }
        delay(100);
    }
    
    // Auto-restart after timeout
    Serial.println("Error timeout - restarting");
    utils.restart();
}

// Log system status for debugging
void logSystemStatus() {
    Serial.println("--- System Status ---");
    Serial.printf("State: %d\n", (int)core.getCurrentState());
    Serial.printf("WiFi: %s\n", wifiConnected ? "Connected" : "Disconnected");
    Serial.printf("Free heap: %u bytes\n", ESP.getFreeHeap());
    Serial.printf("Uptime: %s\n", utils.formatUptime().c_str());
    Serial.printf("Last update: %s ago\n", utils.getTimeAgo(lastUpdateTime).c_str());
    Serial.printf("Last input: %s ago\n", utils.getTimeAgo(lastInputTime).c_str());
    
    if (wifiConnected) {
        Serial.printf("IP: %s\n", WiFi.localIP().toString().c_str());
        Serial.printf("RSSI: %d dBm\n", WiFi.RSSI());
    }
    
    // Battery status if available
    BatteryStatus battery = utils.getBatteryStatus();
    if (battery.voltage > 0) {
        Serial.printf("Battery: %.2fV (%d%%)\n", battery.voltage, battery.percentage);
    }
    
    Serial.println("--------------------");
}