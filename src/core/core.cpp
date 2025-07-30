#include "core.h"
#include <Arduino.h>

// Global instance
CoreManager core;

CoreManager::CoreManager() {
    currentState = SystemState::BOOT;
    previousState = SystemState::BOOT;
    wakeReason = WakeReason::POWER_ON;
    lastUpdateTime = 0;
    lastScreenChange = 0;
    stateStartTime = 0;
    wifiConnected = false;
    updating = false;
}

void CoreManager::init() {
    Serial.println("CoreManager: Initialized");
    stateStartTime = millis();
}

void CoreManager::loop() {
    // Handle current state
    switch (currentState) {
        case SystemState::BOOT:
            handleBootState();
            break;
        case SystemState::WIFI_CONNECTING:
            handleWiFiConnecting();
            break;
        case SystemState::WIFI_CONNECTED:
            handleWiFiConnected();
            break;
        case SystemState::OFFLINE:
            handleOfflineState();
            break;
        case SystemState::DISPLAYING_LIGHTNING:
        case SystemState::DISPLAYING_COLD:
        case SystemState::DISPLAYING_COMBINED:
        case SystemState::DISPLAYING_CONFIG:
            handleDisplayStates();
            break;
        case SystemState::UPDATING_BALANCES:
            handleUpdatingBalances();
            break;
        case SystemState::SLEEPING:
            handleSleepState();
            break;
        case SystemState::CONFIG_MODE:
            handleConfigMode();
            break;
    }
}

void CoreManager::handleStateTransition(SystemState newState) {
    if (newState != currentState) {
        logStateChange(currentState, newState);
        previousState = currentState;
        currentState = newState;
        stateStartTime = millis();
    }
}

void CoreManager::enterSleepMode() {
    Serial.println("CoreManager: Entering sleep mode");
    handleStateTransition(SystemState::SLEEPING);
}

void CoreManager::wakeUp(WakeReason reason) {
    Serial.printf("CoreManager: Wake up (reason: %d)\n", (int)reason);
    wakeReason = reason;
    handleStateTransition(SystemState::WIFI_CONNECTING);
}

void CoreManager::cycleScreen() {
    Serial.println("CoreManager: Cycling screen");
    lastScreenChange = millis();
    
    switch (currentState) {
        case SystemState::DISPLAYING_LIGHTNING:
            handleStateTransition(SystemState::DISPLAYING_COLD);
            break;
        case SystemState::DISPLAYING_COLD:
            handleStateTransition(SystemState::DISPLAYING_COMBINED);
            break;
        case SystemState::DISPLAYING_COMBINED:
            handleStateTransition(SystemState::DISPLAYING_LIGHTNING);
            break;
        default:
            handleStateTransition(SystemState::DISPLAYING_LIGHTNING);
            break;
    }
}

void CoreManager::enterConfigMode() {
    Serial.println("CoreManager: Entering config mode");
    handleStateTransition(SystemState::CONFIG_MODE);
}

void CoreManager::updateBalances() {
    Serial.println("CoreManager: Triggering balance update");
    if (!updating) {
        handleStateTransition(SystemState::UPDATING_BALANCES);
    }
}

// Private methods - stub implementations
void CoreManager::handleBootState() {
    // Boot state handling
}

void CoreManager::handleWiFiConnecting() {
    // WiFi connecting state handling
}

void CoreManager::handleWiFiConnected() {
    // WiFi connected state handling
}

void CoreManager::handleOfflineState() {
    // Offline state handling
}

void CoreManager::handleDisplayStates() {
    // Display states handling
}

void CoreManager::handleUpdatingBalances() {
    // Update balances state handling
    updating = true;
}

void CoreManager::handleSleepState() {
    // Sleep state handling
}

void CoreManager::handleConfigMode() {
    // Config mode handling
}

void CoreManager::checkWiFiStatus() {
    // WiFi status checking
}

void CoreManager::updateSystemTime() {
    // System time update
}

bool CoreManager::shouldSleep() {
    return false; // Stub
}

void CoreManager::logStateChange(SystemState from, SystemState to) {
    Serial.printf("CoreManager: State change %d -> %d\n", (int)from, (int)to);
} 