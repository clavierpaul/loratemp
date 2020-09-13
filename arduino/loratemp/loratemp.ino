#include "TheThingsNetwork.h"
#include "CayenneLPP.h"

#include "conf.h"

// Board Definitions
#define bleSerial Serial1
#define loraSerial Serial2
#define debugSerial SerialUSB
#define SERIAL_TIMEOUT  10000
enum state {WHITE, RED, GREEN, BLUE, CYAN, ORANGE, PURPLE, OFF};    // List of colours for the RGB LED

// For OTAA. Set your AppEUI and AppKey. DevEUI will be serialized by using  HwEUI (in the RN module)
const char *appEui = APP_EUI;
const char *appKey = APP_KEY;

const bool CNF   = true;
const bool UNCNF = false;
const byte MyPort = 3;
byte Payload[51];
byte CNT = 0;                                               // Counter for the main loop, to track packets while prototyping
#define freqPlan TTN_FP_EU868                               // Replace with TTN_FP_EU868 or TTN_FP_US915
#define FSB 0                                               // FSB 0 = enable all channels, 1-8 for private networks
#define SF 10                                               // Initial SF

TheThingsNetwork  ExpLoRer (loraSerial, debugSerial, freqPlan, SF, FSB);    // Create an instance from TheThingsNetwork class
CayenneLPP        CayenneRecord (51);                                       // Create an instance of the Cayenne Low Power Payload

void setup()
{ 
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(LED_RED, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_BLUE, OUTPUT);
  LED(RED);                             // Start with RED
  pinMode(TEMP_SENSOR, INPUT);
  analogReadResolution(10);             //Set ADC resolution to 10 bits
  
  pinMode(LORA_RESET, OUTPUT);          //Reset the LoRa module to a known state
  digitalWrite(LORA_RESET, LOW);
  delay(100);
  digitalWrite(LORA_RESET, HIGH);
  delay(1000);                          // Wait for RN2483 to reset
  LED(ORANGE);                          // Switch to ORANGE after reset
  
  loraSerial.begin(57600);
  debugSerial.begin(57600);

  // Wait a maximum of 10s for Serial Monitor
  while (!debugSerial && millis() < SERIAL_TIMEOUT);

  // Set callback for incoming messages
  ExpLoRer.onMessage(message);

  //Set up LoRa communications
  debugSerial.println("-- STATUS");
  ExpLoRer.showStatus();

  delay(1000);

  debugSerial.println("-- JOIN");

  // Attempt to join 6 times with 5000 ms wait
  if (ExpLoRer.join(appEui, appKey, 6, 5000)) {
    LED(GREEN); // Switch to GREEN if OTAA join is successful
  } else {
    LED(RED); // Switch to RED if OTAA join fails
  }

  delay(1000);
}


void loop()
{
  CNT++;
  // Un-comment during LoRa debugging to see status in Serial Monitor
  // ExpLoRer.showStatus(); 
  
  // Read temperature
  float MyTemp = getTemperature(); // Onboard temperature sensor
  CayenneRecord.addTemperature(3, MyTemp);
  
  // When all measurements are done and the complete Cayenne record created, send it off via LoRa
  
  LED(GREEN); // LED on while transmitting. Green for energy-efficient LoRa
  byte PayloadSize = CayenneRecord.copy(Payload);
  byte response = ExpLoRer.sendBytes(Payload, PayloadSize, MyPort, UNCNF);

  LED(response + 1); // Change LED colour depending on module response to uplink success
  delay(100);
  
  CayenneRecord.reset(); // Clear the record buffer for the next loop
  LED(OFF);

  // Duty cycle wait to avoid getting kicked off
  // Although sampling rate is far below the max
  delay(45000);
}


// Returns temperature in an obscure, hard to read scale known as "Celsius"
float getTemperature()
{
  //10mV per C, 0C is 500mV
  float mVolts = (float)analogRead(TEMP_SENSOR) * 3300.0 / 1023.0;
  float temp = (mVolts - 500.0) / 10.0;
  return temp;
}

void LED(byte state)
{
  switch (state)
  {
  case WHITE:
    digitalWrite(LED_RED, LOW);
    digitalWrite(LED_GREEN, LOW);
    digitalWrite(LED_BLUE, LOW); 
    break;
  case RED:
    digitalWrite(LED_RED, LOW);
    digitalWrite(LED_GREEN, HIGH);
    digitalWrite(LED_BLUE, HIGH); 
    break;
  case ORANGE:
    digitalWrite(LED_RED, LOW);
    digitalWrite(LED_GREEN, LOW);
    digitalWrite(LED_BLUE, HIGH); 
    break;
  case CYAN:
    digitalWrite(LED_RED, HIGH);
    digitalWrite(LED_GREEN, LOW);
    digitalWrite(LED_BLUE, LOW); 
    break;
  case PURPLE:
    digitalWrite(LED_RED, LOW);
    digitalWrite(LED_GREEN, HIGH);
    digitalWrite(LED_BLUE, LOW);
    break;
  case BLUE:
    digitalWrite(LED_RED, HIGH);
    digitalWrite(LED_GREEN, HIGH);
    digitalWrite(LED_BLUE, LOW);
    break;
  case GREEN:
    digitalWrite(LED_RED, HIGH);
    digitalWrite(LED_GREEN, LOW);
    digitalWrite(LED_BLUE, HIGH);
    break;
  default:
    digitalWrite(LED_RED, HIGH);
    digitalWrite(LED_GREEN, HIGH);
    digitalWrite(LED_BLUE, HIGH);
    break;
  }
}

// Callback method for receiving downlink messages. Uses ExpLoRer.onMessage(message) in setup()
// Could be used as an ACK for initial connection test?
void message(const uint8_t *payload, size_t size, port_t port)
{
  debugSerial.println("-- MESSAGE");
  debugSerial.print("Received " + String(size) + " bytes on port " + String(port) + ":");

  for (int i = 0; i < size; i++)
  {
    debugSerial.print(" " + String(payload[i]));
  }

  debugSerial.println();
}
