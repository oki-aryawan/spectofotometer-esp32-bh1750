#include <math.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <BH1750.h>
#include <Adafruit_NeoPixel.h>

#ifdef __AVR__
 #include <avr/power.h> // Required for 16 MHz Adafruit Trinket
#endif

#define PIN 19
#define NUMPIXELS 1
#define BUZZER_PIN 13
#define BUTTON_PIN 34  // Define GPIO pin for the push button

Adafruit_NeoPixel pixels(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);

// Define screen dimensions
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32

// Initialize the OLED display
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// Initialize the BH1750 sensor
BH1750 lightMeter;

// Moving average parameters
const int numReadings = 10;  // Number of readings to average
float readings[numReadings]; // Array to store readings
int readIndex = 0;           // Index of the current reading
float total = 0;             // Sum of the readings
int average = 0;           // Moving average

unsigned long buttonPressTime = 0; // Stores the time button was pressed
bool buttonHeld = false; 


const double constant = 62956.0;
const double exponent = -1.569;

void setup() {
  // Start the I2C communication
  Wire.begin();
  Serial.begin(115200);
  
  // Initialize the BH1750 sensor
  if (!lightMeter.begin()) {
    Serial.println(F("Error initializing BH1750 sensor"));
    while (1);
  }
  
  // Initialize the OLED display
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3C for 128x32
    Serial.println(F("SSD1306 allocation failed"));
    while (1);
  }
  
  // Clear the buffer
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(2, 8);
  display.print(" STARTING ");
  display.display();
  
  // Give some time for the sensor to initialize
  delay(1000);

  // Initialize the NeoPixel
  #if defined(__AVR_ATtiny85__) && (F_CPU == 16000000)
  clock_prescale_set(clock_div_1);
  #endif

  pixels.begin();
  
  // Set NeoPixel to solid white
  pixels.setPixelColor(0, pixels.Color(255, 255, 255));
  pixels.show();

  // Initialize the readings array
  for (int i = 0; i < numReadings; i++) {
    readings[i] = 0;
  }

  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  // Play initialization tone
  playTone(1000, 200);
  display.clearDisplay();
}

void loop() {
  int state = digitalRead(BUTTON_PIN);
  // Subtract the last reading
  total = total - readings[readIndex];
  
  // Read the current light level
  readings[readIndex] = lightMeter.readLightLevel();
  
  // Add the reading to the total
  total = total + readings[readIndex];
  
  // Advance to the next position in the array
  readIndex = readIndex + 1;
  
  // If we're at the end of the array, wrap around to the beginning
  if (readIndex >= numReadings) {
    readIndex = 0;
  }
  
  // Calculate the average
  average = total / numReadings;

  bool isStable = true;
  for (int i = 1; i < numReadings; i++) {
    if (readings[i] != readings[0]) {
      isStable = false;
      break;
    }
  }

  display.setCursor(0, 0); // Adjust the cursor position as needed
  display.print(average);
  
  if (state == LOW){
    if (!buttonHeld){
      buttonPressTime = millis(); // Capture button press time
      buttonHeld = true;
    }
  } else{
    if (buttonHeld && (millis() - buttonPressTime >= 3000)){
      display.clearDisplay();
      double x = average;
      double y = constant * pow(x, exponent);
      //= 62956x^-1.569
      display.setCursor(0, 0);
      display.print(average);
      display.setCursor(0, 16);
      display.print(y);
      display.print("   Mil");
      Serial.println(y);
      Serial.println(" | ");
      Serial.println("HOLD");
      
      playTone(1000, 200);
      unsigned long startTime = millis();
    while (millis() - startTime < 10000) {
        display.display();
        delay(100); // Adjust delay based on desired update frequency
    }
    }
    buttonHeld = false;
  }
  // Update the display with the new data
  display.display();
  Serial.print(state);
  Serial.print(" | ");
  Serial.println(average);
  display.clearDisplay();
  //delay(1000);
}

void playTone(int frequency, int duration) {
  tone(BUZZER_PIN, frequency, duration);
  delay(duration);
  noTone(BUZZER_PIN); // Ensure buzzer is off after the tone
}
