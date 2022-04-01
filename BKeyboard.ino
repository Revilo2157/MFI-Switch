#include <bluefruit.h>
#include "NRF52TimerInterrupt.h"
#include "Button.h"

/* BLE */
BLEDis bledis;
BLEHidAdafruit blehid;

/* Button Handlers */
// Pins
#define TOP_BUTTON D0
#define SECOND_BUTTON D1
#define THIRD_BUTTON D2
#define BOTTOM_BUTTON D3

#define BUTTON_COUNT 4
Button buttonArray[BUTTON_COUNT] = {
  Button(TOP_BUTTON, 
    []() -> void {
      uint8_t keys[6] = {HID_KEY_ARROW_UP,0,0,0,0,0};
      blehid.keyboardReport(BLE_CONN_HANDLE_INVALID, 0, keys);
      blehid.keyRelease();
    }, 
    []() -> void {
      uint8_t keys[6] = {HID_KEY_ARROW_RIGHT,0,0,0,0,0};
      blehid.keyboardReport(BLE_CONN_HANDLE_INVALID, 0, keys);
      blehid.keyRelease();
    }), 
  Button(SECOND_BUTTON,     
    []() -> void {
      uint8_t keys[6] = {HID_KEY_SPACE,0,0,0,0,0};
      blehid.keyboardReport(BLE_CONN_HANDLE_INVALID, 0, keys);
      blehid.keyRelease();
    }, 
    []() -> void {
      uint8_t keys[6] = {HID_KEY_TAB,0,0,0,0,0};
      blehid.keyboardReport(BLE_CONN_HANDLE_INVALID, KEYBOARD_MODIFIER_LEFTGUI, keys);
      blehid.keyRelease();
    }), 
  Button(THIRD_BUTTON,
    []() -> void {
      uint8_t keys[6] = {HID_KEY_Z,0,0,0,0,0};
      blehid.keyboardReport(BLE_CONN_HANDLE_INVALID, KEYBOARD_MODIFIER_LEFTGUI, keys);
      blehid.keyRelease();
    }, 
    []() -> void {
      uint8_t keys[6] = {HID_KEY_Z,0,0,0,0,0};
      blehid.keyboardReport(BLE_CONN_HANDLE_INVALID, KEYBOARD_MODIFIER_LEFTSHIFT | KEYBOARD_MODIFIER_LEFTGUI, keys);
      blehid.keyRelease();
    }), 
  Button(BOTTOM_BUTTON,     
    []() -> void {
      uint8_t keys[6] = {HID_KEY_ARROW_DOWN,0,0,0,0,0};
      blehid.keyboardReport(BLE_CONN_HANDLE_INVALID, 0, keys);
      blehid.keyRelease();
    }, 
    []() -> void {
      uint8_t keys[6] = {HID_KEY_ARROW_LEFT,0,0,0,0,0};
      blehid.keyboardReport(BLE_CONN_HANDLE_INVALID, 0, keys);
      blehid.keyRelease();
    })
};

/* Timer Setup */
#define TIMER1_INTERVAL_MS         10
NRF52Timer ButtonTimer(NRF_TIMER_2);

void TimerHandler() {
    for (int i = 0; i < BUTTON_COUNT; i++) {
      buttonArray[i].updateState();
    }
}

/* Battery Service */
#define BATTERY_UPDATE_INTERVAL_MS 10000 // 10 Seconds
#define VREF 3.3 // Assumes 3.3V regulator output is ADC reference voltage
#define ADC_LIMIT 1024 // 10-bit ADC readings 0-1023, so the factor is 1024
#define VMAX 3.7 // Max Voltage 
#define VMIN 3.2 // Off Voltage
#define BATTERY_STATUS_WIDTH (VMAX - VMIN)

unsigned long lastUpdate;

BLEBas blebas;

void updateBatteryStatus() {
  unsigned int adcCount = analogRead(PIN_VBAT);
  double adcVoltage = (adcCount * VREF) / ADC_LIMIT;
  double vBat = adcVoltage*1510.0/(510.0); //true battery voltage 
  uint8_t batteryPercent = (min(vBat, VMAX) - VMIN) / BATTERY_STATUS_WIDTH * 100;
  Serial.print(vBat);  Serial.print(" : "); Serial.println(batteryPercent);
  blebas.notify(batteryPercent);
}



void setup() 
{
  Serial.begin(115200);

  // Button Handlers
  for (int i = 0; i < BUTTON_COUNT; i++) {
    buttonArray[i].begin();
  }

  // Battery Read Enable
  pinMode(VBAT_ENABLE, OUTPUT);
  digitalWrite(VBAT_ENABLE,LOW);

  // BLE
  Bluefruit.begin();
  Bluefruit.setTxPower(4);    // Check bluefruit.h for supported values

  // Configure and Start Device Information Service
  bledis.setManufacturer("Oliver Rodas");
  bledis.setModel("MFI Switch Mk III");
  bledis.begin();

  // Start Battery Service
  blebas.begin();

  // Start BLE HID
  blehid.begin();

  // Set up and start advertising
  startAdv();

  // Start the Timer
  // Interval in microsecs
  if (ButtonTimer.attachInterruptInterval(TIMER1_INTERVAL_MS * 1000, TimerHandler)) {
    Serial.print(F("Starting ButtonTimer OK")); 
  } else
    Serial.println(F("Can't set ButtonTimer. Select another freq. or timer"));

  lastUpdate = millis();
}

void startAdv(void)
{  
  // Advertising packet
  Bluefruit.Advertising.addFlags(BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE);
  Bluefruit.Advertising.addTxPower();
  Bluefruit.Advertising.addAppearance(BLE_APPEARANCE_HID_KEYBOARD);
  
  // Include BLE HID service
  Bluefruit.Advertising.addService(blehid);

  // There is enough room for the dev name in the advertising packet
  Bluefruit.setName("MFI Switch");
  Bluefruit.Advertising.addName();
  
  /* Start Advertising
   * - Enable auto advertising if disconnected
   * - Interval:  fast mode = 20 ms, slow mode = 152.5 ms
   * - Timeout for fast mode is 30 seconds
   * - Start(timeout) with timeout = 0 will advertise forever (until connected)
   * 
   * For recommended advertising interval
   * https://developer.apple.com/library/content/qa/qa1931/_index.html   
   */
  Bluefruit.Advertising.restartOnDisconnect(true);
  Bluefruit.Advertising.setInterval(32, 244);    // in unit of 0.625 ms
  Bluefruit.Advertising.setFastTimeout(30);      // number of seconds in fast mode
  Bluefruit.Advertising.start(0);                // 0 = Don't stop advertising after n seconds
}



void loop() {
  for (int i = 0; i < BUTTON_COUNT; i++) {
    buttonArray[i].sendKeys();
  }

  if ((millis() - lastUpdate) >= BATTERY_UPDATE_INTERVAL_MS) {
    updateBatteryStatus();
    lastUpdate = millis();
  }
}
