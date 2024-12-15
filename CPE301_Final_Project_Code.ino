// Thomas Nelson, Spencer Trudeau, Illya Gavlovski
//12/14/2024
//CPE Final Project

#include <LiquidCrystal.h>
#include <dht.h>
#include <Servo.h>
#include <Wire.h>
#include <DS1307RTC.h>

// Pin Definitions
#define GREEN_LED 13
#define YELLOW_LED 12
#define RED_LED 11
#define BLUE_LED 10
#define FAN_PIN 43
#define RESET_PIN 2
#define STOP_PIN 3
#define START_PIN 18
#define VENT_LEFT_BUTTON 44
#define VENT_RIGHT_BUTTON 46
#define SERVO_PIN 45
#define DHT_PIN 22
#define RS_PIN 4
#define EN_PIN 5
#define D4_PIN 6
#define D5_PIN 7
#define D6_PIN 8
#define D7_PIN 9

// Constants
const float TEMP_THRESHOLD_HIGH = 20.0;
const unsigned long LCD_UPDATE_INTERVAL = 60000;
const int SERVO_MIN_ANGLE = 0;
const int SERVO_MAX_ANGLE = 180;
const int SERVO_STEP = 15;
const int WATER_LEVEL_THRESHOLD = 100;

// Globals
LiquidCrystal lcd(RS_PIN, EN_PIN, D4_PIN, D5_PIN, D6_PIN, D7_PIN);
dht DHT;
Servo ventServo;
String systemState = "DISABLED";
volatile bool startPressed = false;
float currentTemp = 18.0;
float currentHumidity = 0.0;
unsigned long lastUpdateMillis = 0;
int ventPosition = 90; // Default to 90° (center)

// Function Prototypes
void initializeSystem();
void handleStates();
void updateSensors();
void printToLCD();
void logEvent(String message);
void logStartTime();
void adjustVent();
void checkWaterLevel();
void enterDisabledState();
void enterIdleState();
void enterErrorState();
void enterRunningState();
void ISR_StartButton();
void setupADC();
int readADC();

void setup() {
    initializeSystem();
}

void loop() {
    handleStates();
}

void initializeSystem() {
    Serial.begin(9600);

    // LED Pins Setup
    pinMode(GREEN_LED, OUTPUT);
    pinMode(YELLOW_LED, OUTPUT);
    pinMode(RED_LED, OUTPUT);
    pinMode(BLUE_LED, OUTPUT);

    // Buttons and Fan Setup
    pinMode(RESET_PIN, INPUT_PULLUP);
    pinMode(STOP_PIN, INPUT_PULLUP);
    pinMode(START_PIN, INPUT_PULLUP);
    pinMode(VENT_LEFT_BUTTON, INPUT_PULLUP);
    pinMode(VENT_RIGHT_BUTTON, INPUT_PULLUP);
    pinMode(FAN_PIN, OUTPUT);

    // Attach Interrupt for Start Button
    attachInterrupt(digitalPinToInterrupt(START_PIN), ISR_StartButton, RISING);

    // LCD Initialization
    lcd.begin(16, 2);
    lcd.clear();
    lcd.print("Initializing...");

    // Initialize Servo Motor
    ventServo.attach(SERVO_PIN);
    ventServo.write(ventPosition);

    // Initialize DHT Sensor
    DHT.read11(DHT_PIN);
    currentTemp = DHT.temperature;
    currentHumidity = DHT.humidity;

    // Initialize ADC
    setupADC();

    // Start in Disabled State
    enterDisabledState();
}

