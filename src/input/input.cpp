#include "input.h"
#include <Arduino.h>

// Global instance
InputManager inputMgr;

// Static instance pointer for ISR access
InputManager* InputManager::instance = nullptr;

InputManager::InputManager() {
    buttonState = ButtonState::IDLE;
    buttonPressed = false;
    buttonLastState = false;
    buttonPressTime = 0;
    buttonReleaseTime = 0;
    lastButtonDebounce = 0;
    longPressDetected = false;
    doubleClickPending = false;
    
    tiltActivated = false;
    tiltLastState = false;
    lastTiltTime = 0;
    lastTiltDebounce = 0;
    tiltCalibrated = false;
    tiltSensitivity = 50;
    
    lastEvent = InputEvent::NONE;
    lastInputTime = 0;
    wakeOnButton = true;
    wakeOnTilt = true;
    wokenByInput = false;
    
    longPressThreshold = LONG_PRESS_TIME;
    doubleClickWindow = DOUBLE_CLICK_TIME;
    debounceDelay = DEBOUNCE_DELAY;
    tiltDebounceDelay = TILT_DEBOUNCE_TIME;
    
    buttonCallback = nullptr;
    tiltCallback = nullptr;
    
    instance = this;
}

void InputManager::init() {
    Serial.println("InputManager: Initializing input handling");
    
    // Configure button pin
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    
    // Configure tilt switch pin
    pinMode(TILT_PIN, INPUT_PULLUP);
    
    // Attach interrupts for wake functionality
    attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), buttonISR, CHANGE);
    attachInterrupt(digitalPinToInterrupt(TILT_PIN), tiltISR, CHANGE);
    
    lastInputTime = millis();
    Serial.println("InputManager: Input initialization complete");
}

void InputManager::loop() {
    handleButtonInput();
    handleTiltInput();
}

InputEvent InputManager::getLastEvent() {
    return lastEvent;
}

void InputManager::clearEvents() {
    lastEvent = InputEvent::NONE;
}

bool InputManager::isButtonPressed() const {
    return digitalRead(BUTTON_PIN) == LOW; // Active low
}

bool InputManager::isTiltActivated() const {
    return digitalRead(TILT_PIN) == HIGH; // Active high
}

void InputManager::enableWakeOnButton(bool enable) {
    wakeOnButton = enable;
    Serial.printf("InputManager: Wake on button %s\n", enable ? "enabled" : "disabled");
}

void InputManager::enableWakeOnTilt(bool enable) {
    wakeOnTilt = enable;
    Serial.printf("InputManager: Wake on tilt %s\n", enable ? "enabled" : "disabled");
}

void InputManager::setupDeepSleepWakeup() {
    Serial.println("InputManager: Setting up deep sleep wake sources");
    
    if (wakeOnButton) {
        esp_sleep_enable_ext0_wakeup(GPIO_NUM_0, 0); // Button (active low)
    }
    
    if (wakeOnTilt) {
        esp_sleep_enable_ext1_wakeup(1ULL << GPIO_NUM_2, ESP_EXT1_WAKEUP_ANY_HIGH); // Tilt switch
    }
}

bool InputManager::wasWokenByInput() const {
    return wokenByInput;
}

void InputManager::calibrateTiltSensor() {
    Serial.println("InputManager: Calibrating tilt sensor");
    tiltCalibrated = true;
}

void InputManager::setTiltSensitivity(uint8_t sensitivity) {
    tiltSensitivity = sensitivity;
    Serial.printf("InputManager: Tilt sensitivity set to %d\n", sensitivity);
}

void InputManager::setButtonHoldTime(unsigned long holdTime) {
    longPressThreshold = holdTime;
    Serial.printf("InputManager: Button hold time set to %lu ms\n", holdTime);
}

void InputManager::setButtonCallback(void (*callback)(InputEvent)) {
    buttonCallback = callback;
    Serial.println("InputManager: Button callback set");
}

void InputManager::setTiltCallback(void (*callback)(InputEvent)) {
    tiltCallback = callback;
    Serial.println("InputManager: Tilt callback set");
}

// Private methods
void InputManager::handleButtonInput() {
    bool currentButtonState = isButtonPressed();
    unsigned long currentTime = millis();
    
    // Debounce
    if (currentButtonState != buttonLastState) {
        lastButtonDebounce = currentTime;
    }
    
    if ((currentTime - lastButtonDebounce) > debounceDelay) {
        if (currentButtonState != buttonPressed) {
            buttonPressed = currentButtonState;
            
            if (buttonPressed) {
                // Button press
                buttonPressTime = currentTime;
                buttonState = ButtonState::PRESSED;
                lastInputTime = currentTime;
                
            } else {
                // Button release
                buttonReleaseTime = currentTime;
                unsigned long pressDuration = buttonReleaseTime - buttonPressTime;
                
                if (pressDuration >= longPressThreshold) {
                    // Long press
                    triggerEvent(InputEvent::BUTTON_LONG_PRESS);
                } else {
                    // Short press (or first click of double click)
                    if (doubleClickPending && (currentTime - buttonReleaseTime) < doubleClickWindow) {
                        // Double click
                        triggerEvent(InputEvent::BUTTON_DOUBLE_CLICK);
                        doubleClickPending = false;
                    } else {
                        // Single click (might become double)
                        doubleClickPending = true;
                        // Trigger short press immediately
                        triggerEvent(InputEvent::BUTTON_SHORT_PRESS);
                    }
                }
                
                buttonState = ButtonState::IDLE;
            }
        }
    }
    
    // Clear double click pending after timeout
    if (doubleClickPending && (currentTime - buttonReleaseTime) > doubleClickWindow) {
        doubleClickPending = false;
    }
    
    buttonLastState = currentButtonState;
}

void InputManager::handleTiltInput() {
    bool currentTiltState = isTiltActivated();
    unsigned long currentTime = millis();
    
    // Debounce
    if (currentTiltState != tiltLastState) {
        lastTiltDebounce = currentTime;
    }
    
    if ((currentTime - lastTiltDebounce) > tiltDebounceDelay) {
        if (currentTiltState != tiltActivated) {
            tiltActivated = currentTiltState;
            
            if (tiltActivated) {
                // Tilt activated
                lastTiltTime = currentTime;
                lastInputTime = currentTime;
                triggerEvent(InputEvent::TILT_ACTIVATED);
            }
        }
    }
    
    tiltLastState = currentTiltState;
}

void InputManager::updateButtonState() {
    // State machine update (handled in handleButtonInput)
}

void InputManager::updateTiltState() {
    // State machine update (handled in handleTiltInput)
}

void InputManager::processButtonPress() {
    // Additional button press processing
}

void InputManager::processButtonRelease() {
    // Additional button release processing
}

void InputManager::processTiltActivation() {
    // Additional tilt activation processing
}

void InputManager::debounceButton() {
    // Handled in handleButtonInput
}

void InputManager::debounceTilt() {
    // Handled in handleTiltInput
}

void InputManager::triggerEvent(InputEvent event) {
    lastEvent = event;
    logInputEvent(event);
    
    // Trigger appropriate callback
    switch (event) {
        case InputEvent::BUTTON_SHORT_PRESS:
        case InputEvent::BUTTON_LONG_PRESS:
        case InputEvent::BUTTON_DOUBLE_CLICK:
            if (buttonCallback) {
                buttonCallback(event);
            }
            break;
            
        case InputEvent::TILT_ACTIVATED:
        case InputEvent::TILT_SHAKE:
            if (tiltCallback) {
                tiltCallback(event);
            }
            break;
            
        default:
            break;
    }
}

// Static ISR handlers
void IRAM_ATTR InputManager::buttonISR() {
    if (instance) {
        instance->lastInputTime = millis();
        instance->wokenByInput = true;
    }
}

void IRAM_ATTR InputManager::tiltISR() {
    if (instance) {
        instance->lastInputTime = millis();
        instance->wokenByInput = true;
    }
}

void InputManager::checkWakeSource() {
    esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();
    
    switch (wakeup_reason) {
        case ESP_SLEEP_WAKEUP_EXT0:
            Serial.println("InputManager: Woken by button");
            wokenByInput = true;
            triggerEvent(InputEvent::WAKE_FROM_SLEEP);
            break;
        case ESP_SLEEP_WAKEUP_EXT1:
            Serial.println("InputManager: Woken by tilt switch");
            wokenByInput = true;
            triggerEvent(InputEvent::WAKE_FROM_SLEEP);
            break;
        default:
            wokenByInput = false;
            break;
    }
}

void InputManager::logInputEvent(InputEvent event) {
    const char* eventNames[] = {
        "NONE", "BUTTON_SHORT_PRESS", "BUTTON_LONG_PRESS", 
        "BUTTON_DOUBLE_CLICK", "TILT_ACTIVATED", "TILT_SHAKE", "WAKE_FROM_SLEEP"
    };
    
    if ((int)event < sizeof(eventNames) / sizeof(eventNames[0])) {
        Serial.printf("InputManager: Event triggered - %s\n", eventNames[(int)event]);
    }
} 