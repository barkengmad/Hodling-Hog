#ifndef CORE_H
#define CORE_H

#include <Arduino.h>

// System states for the main state machine
enum class SystemState {
    BOOT,
    WIFI_CONNECTING,
    WIFI_CONNECTED,
    OFFLINE,
    DISPLAYING_LIGHTNING,
    DISPLAYING_COLD,
    DISPLAYING_COMBINED,
    DISPLAYING_CONFIG,
    UPDATING_BALANCES,
    SLEEPING,
    CONFIG_MODE
};

// Wake reasons
enum class WakeReason {
    POWER_ON,
    BUTTON_PRESS,
    TILT_SWITCH,
    TIMER,
    UNKNOWN
};

class CoreManager {
public:
    CoreManager();
    void init();
    void loop();
    void handleStateTransition(SystemState newState);
    SystemState getCurrentState() const { return currentState; }
    WakeReason getWakeReason() const { return wakeReason; }
    
    // Public methods for state changes
    void enterSleepMode();
    void wakeUp(WakeReason reason);
    void cycleScreen();
    void enterConfigMode();
    void updateBalances();
    
    // Status indicators
    bool isWiFiConnected() const { return wifiConnected; }
    bool isUpdating() const { return updating; }
    unsigned long getLastUpdateTime() const { return lastUpdateTime; }
    
private:
    SystemState currentState;
    SystemState previousState;
    WakeReason wakeReason;
    
    // Timing and status
    unsigned long lastUpdateTime;
    unsigned long lastScreenChange;
    unsigned long stateStartTime;
    bool wifiConnected;
    bool updating;
    
    // State machine handlers
    void handleBootState();
    void handleWiFiConnecting();
    void handleWiFiConnected();
    void handleOfflineState();
    void handleDisplayStates();
    void handleUpdatingBalances();
    void handleSleepState();
    void handleConfigMode();
    
    // Helper methods
    void checkWiFiStatus();
    void updateSystemTime();
    bool shouldSleep();
    void logStateChange(SystemState from, SystemState to);
};

// Global instance
extern CoreManager core;

#endif // CORE_H 