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

void handleStates() {
    if (systemState == "DISABLED") {
        enterDisabledState();
    } else if (systemState == "IDLE") {
        enterIdleState();
    } else if (systemState == "ERROR") {
        enterErrorState();
    } else if (systemState == "RUNNING") {
        enterRunningState();
    }
}

void enterDisabledState() {
    digitalWrite(YELLOW_LED, HIGH);
    digitalWrite(GREEN_LED, LOW);
    digitalWrite(RED_LED, LOW);
    digitalWrite(BLUE_LED, LOW);
    digitalWrite(FAN_PIN, LOW);

    lcd.clear();
    lcd.print("System Disabled");

    if (startPressed) {
        systemState = "IDLE";
        lcd.clear();
        lcd.print("System Enabled");
        logEvent("System Enabled");
        logStartTime();
        
    }
}

void enterIdleState() {
    digitalWrite(GREEN_LED, HIGH);
    digitalWrite(YELLOW_LED, LOW);
    digitalWrite(RED_LED, LOW);
    digitalWrite(BLUE_LED, LOW);
    digitalWrite(FAN_PIN, LOW);

    checkWaterLevel();
    updateSensors();
    printToLCD();
    adjustVent();

    if (currentTemp >= TEMP_THRESHOLD_HIGH) {
        systemState = "RUNNING";
        logEvent("Entering RUNNING state");
    }

    if (digitalRead(STOP_PIN) == LOW) {
        systemState = "DISABLED";
        logEvent("System Stopped");
        startPressed = false;
    }
}

void enterErrorState() {
    digitalWrite(RED_LED, HIGH);
    digitalWrite(YELLOW_LED, LOW);
    digitalWrite(GREEN_LED, LOW);
    digitalWrite(BLUE_LED, LOW);
    digitalWrite(FAN_PIN, LOW);

    lcd.clear();
    lcd.print("Water Level Low!");

    if (digitalRead(RESET_PIN) == LOW && readADC() > WATER_LEVEL_THRESHOLD) {
        systemState = "IDLE";
        logEvent("Error Resolved");
    }
}

void enterRunningState() {
    digitalWrite(BLUE_LED, HIGH);
    digitalWrite(YELLOW_LED, LOW);
    digitalWrite(GREEN_LED, LOW);
    digitalWrite(RED_LED, LOW);
    digitalWrite(FAN_PIN, HIGH);

    updateSensors();
    printToLCD();
    adjustVent();
    checkWaterLevel();

    if (currentTemp < TEMP_THRESHOLD_HIGH) {
        systemState = "IDLE";
        logEvent("Temperature Normal");
    }

    if (digitalRead(STOP_PIN) == LOW) {
        systemState = "DISABLED";
        logEvent("System Stopped");
        startPressed = false;
    }
}

void updateSensors() {
    DHT.read11(DHT_PIN);
    currentTemp = DHT.temperature;
    currentHumidity = DHT.humidity;
}

void printToLCD() {
    if (millis() - lastUpdateMillis >= LCD_UPDATE_INTERVAL) {
        lastUpdateMillis = millis();
        lcd.clear(); // Clear the LCD before printing
        lcd.setCursor(0, 0);
        lcd.print("Temp: ");
        lcd.print(currentTemp);
        lcd.print("C");
        lcd.setCursor(0, 1);
        lcd.print("Humidity: ");
        lcd.print(currentHumidity);
        lcd.print("%");
    }
}

void logEvent(String message) {
    tmElements_t tm;
    RTC.read(tm);
    Serial.print(tm.Hour);
    Serial.print(":");
    Serial.print(tm.Minute);
    Serial.print(":");
    Serial.print(tm.Second);
    Serial.print(" ");
    Serial.print(tm.Month);
    Serial.print("/");
    Serial.print(tm.Day);
    Serial.print("/");
    Serial.print(tmYearToCalendar(tm.Year));
    Serial.print(" - ");
    Serial.println(message);
}

void logStartTime() {
    tmElements_t tm;
    RTC.read(tm);
    Serial.print("System Start Time: ");
    Serial.print(tm.Hour);
    Serial.print(":");
    Serial.print(tm.Minute);
    Serial.print(":");
    Serial.print(tm.Second);
    Serial.print(" ");
    Serial.print(tm.Month);
    Serial.print("/");
    Serial.print(tm.Day);
    Serial.print("/");
    Serial.println(tmYearToCalendar(tm.Year));
}

void adjustVent() {
    if (digitalRead(VENT_LEFT_BUTTON) == LOW && ventPosition > SERVO_MIN_ANGLE) {
        ventPosition -= SERVO_STEP;
        ventServo.write(ventPosition);
        logEvent("Vent Moved Left: " + String(ventPosition));
    } else if (digitalRead(VENT_RIGHT_BUTTON) == LOW && ventPosition < SERVO_MAX_ANGLE) {
        ventPosition += SERVO_STEP;
        ventServo.write(ventPosition);
        logEvent("Vent Moved Right: " + String(ventPosition));
    }
}

void checkWaterLevel() {
    if (readADC() < WATER_LEVEL_THRESHOLD) {
        systemState = "ERROR";
        logEvent("Water Level Low");
    }
}

void ISR_StartButton() {
    startPressed = true;
}

void setupADC() {
    ADMUX = (1 << REFS0); // Use AVCC as the reference voltage
    ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1); // Enable ADC and set prescaler to 64
}

int readADC() {
    ADMUX &= 0xF0; // Clear ADC channel selection
    ADCSRA |= (1 << ADSC); // Start conversion
    while (ADCSRA & (1 << ADSC)); // Wait for conversion to finish
    return ADC; // Return ADC value
}
