#ifndef INPUT_H
#define INPUT_H

#include <Arduino.h>

// Pin definitions for input devices (as specified in requirements)
#define BUTTON_PIN    21   // GPIO21 - Safe general purpose pin for button input & boot detection
#define TILT_PIN      2    // GPIO2 - Triggers wake + balance update

// Input configuration
#define DEBOUNCE_DELAY      50     // Debounce delay in milliseconds
#define LONG_PRESS_TIME     2000   // Long press threshold in milliseconds
#define DOUBLE_CLICK_TIME   500    // Double click window in milliseconds
#define TILT_DEBOUNCE_TIME  1000   // Tilt switch debounce in milliseconds

// Input event types
enum class InputEvent {
    NONE,
    BUTTON_SHORT_PRESS,
    BUTTON_LONG_PRESS,
    BUTTON_DOUBLE_CLICK,
    TILT_ACTIVATED,
    TILT_SHAKE,
    WAKE_FROM_SLEEP
};

// Input states for state machine
enum class ButtonState {
    IDLE,
    PRESSED,
    LONG_PRESS_DETECTED,
    RELEASED,
    DOUBLE_CLICK_WAIT
};

class InputManager {
public:
    InputManager();
    void init();
    void loop();
    InputEvent getLastEvent();
    void clearEvents();
    
    // Input status
    bool isButtonPressed() const;
    bool isTiltActivated() const;
    unsigned long getLastInputTime() const { return lastInputTime; }
    
    // Sleep/wake functionality
    void enableWakeOnButton(bool enable);
    void enableWakeOnTilt(bool enable);
    void setupDeepSleepWakeup();
    bool wasWokenByInput() const;
    
    // Calibration and sensitivity
    void calibrateTiltSensor();
    void setTiltSensitivity(uint8_t sensitivity);
    void setButtonHoldTime(unsigned long holdTime);
    
    // Event callbacks
    void setButtonCallback(void (*callback)(InputEvent));
    void setTiltCallback(void (*callback)(InputEvent));
    
private:
    // Button handling
    ButtonState buttonState;
    bool buttonPressed;
    bool buttonLastState;
    unsigned long buttonPressTime;
    unsigned long buttonReleaseTime;
    unsigned long lastButtonDebounce;
    bool longPressDetected;
    bool doubleClickPending;
    
    // Tilt switch handling
    bool tiltActivated;
    bool tiltLastState;
    unsigned long lastTiltTime;
    unsigned long lastTiltDebounce;
    bool tiltCalibrated;
    uint8_t tiltSensitivity;
    
    // General input tracking
    InputEvent lastEvent;
    unsigned long lastInputTime;
    bool wakeOnButton;
    bool wakeOnTilt;
    bool wokenByInput;
    
    // Configuration
    unsigned long longPressThreshold;
    unsigned long doubleClickWindow;
    unsigned long debounceDelay;
    unsigned long tiltDebounceDelay;
    
    // Callbacks
    void (*buttonCallback)(InputEvent);
    void (*tiltCallback)(InputEvent);
    
    // Private methods
    void handleButtonInput();
    void handleTiltInput();
    void updateButtonState();
    void updateTiltState();
    void processButtonPress();
    void processButtonRelease();
    void processTiltActivation();
    void debounceButton();
    void debounceTilt();
    void triggerEvent(InputEvent event);
    
    // Interrupt handlers (must be static)
    static void IRAM_ATTR buttonISR();
    static void IRAM_ATTR tiltISR();
    
    // Static reference for ISR access
    static InputManager* instance;
    
    // Wake detection
    void checkWakeSource();
    void logInputEvent(InputEvent event);
};

// Global instance
extern InputManager inputMgr;

#endif // INPUT_H 