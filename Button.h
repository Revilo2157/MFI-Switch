#include <Arduino.h>

#define LONG_PRESS_THRESH   25

class Button {

    enum PressState {
        STILL_RELEASED = 0,
        JUST_PRESSED = 1, 
        JUST_RELEASED = 2,
        STILL_PRESSED = 3
    };

    enum SignalType {
        PRESS,
        LONG_PRESS,
        NONE
    };

    private:
        SignalType mySignal;
        bool wasPressedLast;
        int pressDuration;
        void (*myPressHandler)(void);
        void (*myLongPressHandler)(void);
        int myPin;

    public:
        Button(int PIN, void (*pressHandler)(void), void(*longPressHandler)(void));
        void updateState(void);
        void sendKeys(void);
        void begin(void);
};

Button::Button(int PIN, void (*pressHandler)(void), void(*longPressHandler)(void)) {
    myPin = PIN;
    myPressHandler = pressHandler;
    myLongPressHandler = longPressHandler;
}

void Button::begin() {
    pinMode(myPin, INPUT_PULLDOWN);
}

void Button::updateState() {
    bool isPressed = digitalRead(myPin);

    // if state changed, reset
    switch (isPressed | (wasPressedLast << 1)) {
    case STILL_RELEASED: 
        return;
    case JUST_PRESSED:
        wasPressedLast = isPressed;
        pressDuration = 0;
        return;
    case JUST_RELEASED:
        wasPressedLast = isPressed;
        if (pressDuration < LONG_PRESS_THRESH)
        mySignal = PRESS;
        return;
    case STILL_PRESSED:
        pressDuration++;
        if (pressDuration == LONG_PRESS_THRESH) 
        mySignal = LONG_PRESS;
        return;
    }
}
    
void Button::sendKeys() {
    switch (mySignal) {
        case NONE:
            break;
        case PRESS:
            myPressHandler();
            mySignal = NONE;
            break;
        case LONG_PRESS:
            myLongPressHandler();
            mySignal = NONE;
            break;
    }
}
