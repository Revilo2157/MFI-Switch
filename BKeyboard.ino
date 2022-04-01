/*********************************************************************
 This is an example for our nRF52 based Bluefruit LE modules

 Pick one up today in the adafruit shop!

 Adafruit invests time and resources providing this open source code,
 please support Adafruit and open-source hardware by purchasing
 products from Adafruit!

 MIT license, check LICENSE for more information
 All text above, and the splash screen below must be included in
 any redistribution
*********************************************************************/
#include <bluefruit.h>
#include "NRF52TimerInterrupt.h"

//////////// Button Handlers ///////////
#define TOP_BUTTON D0
#define SECOND_BUTTON D1
#define THIRD_BUTTON D2
#define BOTTOM_BUTTON D3

#define TIMER1_INTERVAL_MS         10
#define BATTERY_UPDATE_INTERVAL_MS 1000 // 10 Seconds


#define VREF 3.3 // Assumes 3.3V regulator output is ADC reference voltage
#define ADC_LIMIT 1024 // 10-bit ADC readings 0-1023, so the factor is 1024
#define VMAX 3.7 // Max Voltage 
#define VMIN 3.2 // Off Voltage
#define BATTERY_STATUS_WIDTH (VMAX - VMIN)

enum ButtonState {
  PRESSED,
  RELEASED, 
  STABLE
};

// Init NRF52 timers NRF_TIMER2 and 3
NRF52Timer ButtonTimer(NRF_TIMER_2);
//NRF52Timer BatteryTimer(NRF_TIMER_4);

bool lastState[4];

ButtonState deBounceButton(int whichButton) {
  bool pressed = digitalRead(whichButton);
  if (pressed == lastState[(whichButton)]) {
    return STABLE;
  }
  lastState[(whichButton)] = pressed;
  return pressed ? PRESSED : RELEASED;
}

ButtonState buttonStates[4] = {STABLE, STABLE, STABLE, STABLE};

void ButtonInterruptHandler() {
    buttonStates[(TOP_BUTTON)] = deBounceButton(TOP_BUTTON);
    buttonStates[(SECOND_BUTTON)] = deBounceButton(SECOND_BUTTON); 
    buttonStates[(THIRD_BUTTON)] = deBounceButton(THIRD_BUTTON);
    buttonStates[(BOTTOM_BUTTON)] = deBounceButton(BOTTOM_BUTTON);
}

String keyCode[4] = {"p", "u", "o", "d"}; 

bool sentKey;


///////////////// BLE /////////////////

BLEDis bledis;
BLEHidAdafruit blehid;
BLEBas blebas;

void updateBatteryStatus() {
  unsigned int adcCount = analogRead(PIN_VBAT);
  double adcVoltage = (adcCount * VREF) / ADC_LIMIT;
  double vBat = adcVoltage*1510.0/(510.0); //true battery voltage 
  uint8_t batteryPercent = (min(vBat, VMAX) - VMIN) / BATTERY_STATUS_WIDTH * 100;
    Serial.print(vBat);  Serial.print(" : "); Serial.println(batteryPercent);
  blebas.notify(batteryPercent);
}


unsigned long lastUpdate;

void setup() 
{
  Serial.begin(115200);
  while ( !Serial ) delay(10);   // for nrf52840 with native usb

  // Button Handlers
  pinMode(TOP_BUTTON, INPUT_PULLDOWN);
  pinMode(SECOND_BUTTON, INPUT_PULLDOWN);
  pinMode(THIRD_BUTTON, INPUT_PULLDOWN);
  pinMode(BOTTOM_BUTTON, INPUT_PULLDOWN);

  // Battery Read Enable
  pinMode(VBAT_ENABLE, OUTPUT);
  digitalWrite(VBAT_ENABLE,LOW);

  // BLE
  Serial.println("Bluefruit52 HID Keyboard Example");
  Serial.println("--------------------------------\n");

  Serial.println();
  Serial.println("Go to your phone's Bluetooth settings to pair your device");
  Serial.println("then open an application that accepts keyboard input");

  Serial.println();
  Serial.println("Enter the character(s) to send:");
  Serial.println();  

  Bluefruit.begin();
  Bluefruit.setTxPower(4);    // Check bluefruit.h for supported values

  // Configure and Start Device Information Service
  bledis.setManufacturer("Oliver Rodas");
  bledis.setModel("MFI Switch Mk III");
  bledis.begin();

  blebas.begin();

  /* Start BLE HID
   * Note: Apple requires BLE device must have min connection interval >= 20m
   * ( The smaller the connection interval the faster we could send data).
   * However for HID and MIDI device, Apple could accept min connection interval 
   * up to 11.25 ms. Therefore BLEHidAdafruit::begin() will try to set the min and max
   * connection interval to 11.25  ms and 15 ms respectively for best performance.
   */
  blehid.begin();

  /* Set connection interval (min, max) to your perferred value.
   * Note: It is already set by BLEHidAdafruit::begin() to 11.25ms - 15ms
   * min = 9*1.25=11.25 ms, max = 12*1.25= 15 ms 
   */
  /* Bluefruit.Periph.setConnInterval(9, 12); */

  // Set up and start advertising
  startAdv();

  // Start the Timer
  // Interval in microsecs
  if (ButtonTimer.attachInterruptInterval(TIMER1_INTERVAL_MS * 1000, ButtonInterruptHandler)) {
    Serial.print(F("Starting ButtonTimer OK, millis() = ")); Serial.println(millis());
  } else
    Serial.println(F("Can't set ButtonTimer. Select another freq. or timer"));

    // Interval in microsecs
//  if (BatteryTimer.attachInterruptInterval(TIMER2_INTERVAL_MS * 1000, BatteryInterruptHandler)) {
//    Serial.print(F("Starting BatteryTimer OK, millis() = ")); Serial.println(millis());
//  } else
//    Serial.println(F("Can't set BatteryTimer. Select another freq. or timer"));

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

void checkButton(int whichButton) {
  ButtonState currentState = buttonStates[(whichButton)];
  buttonStates[(whichButton)] = STABLE;

  if (currentState == PRESSED) {
    sentKey = true;
    String strToSend = keyCode[(whichButton)];
    int numToSend = strToSend.length() + 1;
    char buff[numToSend];
    strToSend.toCharArray(buff, numToSend);
    blehid.keySequence(buff, numToSend);
  
    if (Serial.available()) {
      Serial.print(strToSend);
      Serial.print(" ");
    }
  }
}

void loop() 
{
  // Only send KeyRelease if previously pressed to avoid sending
  // multiple keyRelease reports (that consume memory and bandwidth)
//  if ( sentKey )
//  {
//    sentKey = false;
//    blehid.keyRelease();
//    
//    // Delay a bit after a report
//    delay(5);
//  }

  checkButton(TOP_BUTTON);
  checkButton(SECOND_BUTTON);
  checkButton(THIRD_BUTTON);
  checkButton(BOTTOM_BUTTON);

  if ((millis() - lastUpdate) >= BATTERY_UPDATE_INTERVAL_MS) {
    updateBatteryStatus();
    lastUpdate = millis();
  }


//  if (sentKey) { 
//    Serial.println("");
//
//    // Delay a bit after a report
//    delay(5);
//  }
}
