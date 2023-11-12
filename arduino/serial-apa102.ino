#include <APA102.h>

typedef struct {
  float h;  // Hue (0 - 360)
  float s;  // Saturation (0 - 1)
  float v;  // Value (0 - 1)
} HSV;

typedef struct {
  uint8_t r;
  uint8_t g;
  uint8_t b;
} RGB;

const uint8_t dataPin = 11;
const uint8_t clockPin = 12;
APA102<dataPin, clockPin> ledStrip;
const uint16_t ledCount = 2;
const int animationDelay = 10;  //milliseconds
const int animationStepSize = 10;

String inputString = "";      // a String to hold incoming data
bool stringComplete = false;  // whether the string is complete

uint8_t stripState[ledCount][4];

uint8_t goalState[ledCount][4];

uint8_t lastColor[4];
HSV lastHsv = {0.0, 1.0, 1.0};

unsigned long previousMillis = 0;

float blueFactor = 1;

void setup() {
  // initialize serial:
  Serial.begin(9600);
  // reserve 200 bytes for the inputString:
  inputString.reserve(200);

  pinMode(LED_BUILTIN, OUTPUT);
}

void loop() {
  // print the string when a newline arrives:
  if (stringComplete) {
    inputString.trim();
    Serial.println("Input: " + inputString);

    char commandBuffer[50];  // Buffer to hold the command
    int value = 0;           // Variable to hold the value

    parseInput(inputString.c_str(), commandBuffer, value);

    Serial.println("Command: ");
    Serial.println(commandBuffer);
    Serial.println("Value: ");
    Serial.println(value);

    if (strcmp(commandBuffer, "on") == 0) {
      Serial.println("Switching LED on.");
      digitalWrite(LED_BUILTIN, HIGH);
      if (lastColor[0] == 0 && lastColor && lastColor[1] == 0 && lastColor[2] == 0) {
        setStripWhite();
      }
      if (lastColor[3] == 0) {
        setStripBrightness(50);
      }
    }
    if (strcmp(commandBuffer, "off") == 0) {
      Serial.println("Switching LED off.");
      digitalWrite(LED_BUILTIN, LOW);
      setStripOff();
    }
    if (strcmp(commandBuffer, "brightness") == 0) {
      Serial.print("Setting brightness to ");
      Serial.println(value);
      setStripBrightness(value);
    }
    if (strcmp(commandBuffer, "temperature") == 0) {
      Serial.print("Setting temperature to ");
      Serial.println(value);
      setColorTemperature(value);
    }
    if (strcmp(commandBuffer, "hue") == 0) {
      Serial.print("Setting hue to ");
      Serial.println(value);
      setStripHue(value);
    }
    if (strcmp(commandBuffer, "saturation") == 0) {
      Serial.print("Setting saturation to ");
      Serial.println(value);
      setStripSaturation(value);
    }
    // Reset
    inputString = "";
    stringComplete = false;
  }

  // Animate

  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis >= animationDelay) {
    previousMillis = currentMillis;

    bool changed = incrementStripState();
    if (changed) {
      printStripState();
    }
  }
}

/*
  SerialEvent occurs whenever a new data comes in the hardware serial RX. This
  routine is run between each time loop() runs, so using delay inside loop can
  delay response. Multiple bytes of data may be available.
*/
void serialEvent() {
  while (Serial.available()) {
    // get the new byte:
    char inChar = (char)Serial.read();
    // add it to the inputString:
    inputString += inChar;
    // if the incoming character is a newline, set a flag so the main loop can
    // do something about it:
    if (inChar == '\n') {
      stringComplete = true;
    }
  }
}

void parseInput(char* input, char* command, int& value) {
  // Use strtok instead of strsep, and use a local buffer to store the input string
  char buffer[200];  // Make sure this is large enough for your longest input
  strncpy(buffer, input, 200);
  buffer[199] = '\0';  // Ensure null-termination

  // Tokenize the buffer and get the command
  char* piece = strtok(buffer, " ");
  if (piece != NULL) {
    strcpy(command, piece);  // Copy the command into the provided buffer

    // Get the next piece, which should be the value
    piece = strtok(NULL, " ");
    if (piece != NULL) {
      value = atoi(piece);  // Convert it to an integer
    }
  }
}

void setStripColorBrightness(uint8_t red, uint8_t green, uint8_t blue, uint8_t brightness) {
  setStripColor(red, green, blue);
  setStripBrightness(brightness);
}

void setColorTemperature(uint16_t temp) {
  Serial.println("Last color: ");
  for (int i = 0; i < 4; i++) {
    Serial.println(lastColor[i]);
  }
  blueFactor = constrain((temp - 140.0) / 360.0, 0.0, 1.0);
  Serial.println(temp);
  Serial.println(blueFactor);
  for (uint16_t i = 0; i < ledCount; i++) {
    goalState[i][2] = (uint8_t)lastColor[2] * blueFactor;
  }
}

void setStripHue(uint16_t hueInt) {
  HSV hsv = { (float) hueInt, lastHsv.s, lastHsv.v };
  Serial.println(hsv.h);
  Serial.println(hsv.s);
  Serial.println(hsv.v);
  RGB rgb = hsvToRgb(hsv.h, hsv.s, hsv.v);
  Serial.println(rgb.r);
  Serial.println(rgb.g);
  Serial.println(rgb.b);

  setStripColor(rgb.r, rgb.g, rgb.b);
  lastHsv = hsv;
}

void setStripSaturation(uint8_t saturation) {
  HSV hsv = { lastHsv.h, constrain(saturation / 100.0, 0.0, 1.0), lastHsv.v };
  Serial.println(lastHsv.h);
  Serial.println(lastHsv.s);
  Serial.println(lastHsv.v);
  RGB rgb = hsvToRgb(hsv.h, hsv.s, hsv.v);
  Serial.println(rgb.r);
  Serial.println(rgb.g);
  Serial.println(rgb.b);
  setStripColor(rgb.r, rgb.g, rgb.b);
  lastHsv = hsv;
}

void setStripColor(uint8_t red, uint8_t green, uint8_t blue) {
  for (uint16_t i = 0; i < ledCount; i++) {
    goalState[i][0] = red;
    goalState[i][1] = green;
    goalState[i][2] = (uint8_t)blue * blueFactor;
  }
  lastColor[0] = red;
  lastColor[1] = green;
  lastColor[2] = blue;
}

void setStripBrightness(uint8_t brightness) {
  uint8_t fiveBitValue = map(brightness, 0, 100, 0, 31);
  fiveBitValue = constrain(fiveBitValue, 0, 31);
  for (uint16_t i = 0; i < ledCount; i++) {
    goalState[i][3] = fiveBitValue;
  }
  lastColor[3] = brightness;
}

void setStripOff() {
  setStripBrightness(0);
}

void setStripWhite() {
  setStripColor(255, 255, 255);
}

void sendFrame() {
  ledStrip.startFrame();
  for (uint16_t i = 0; i < ledCount; i++) {
    ledStrip.sendColor(stripState[i][0], stripState[i][1], stripState[i][2], stripState[i][3]);
  }
  ledStrip.endFrame(ledCount);
}

bool incrementStripState() {
  bool changed = false;
  for (uint16_t i = 0; i < ledCount; i++) {
    for (uint8_t j = 0; j < 3; j++) {
      changed |= updateOneState(i, j, animationStepSize);
    }
    changed |= updateOneState(i, 3, 1);
  }
  return changed;
}

bool updateOneState(int i, int j, int animationStepSize) {
  if (stripState[i][j] > goalState[i][j]) {
    stripState[i][j] -= calculateStepSize(stripState[i][j], goalState[i][j], animationStepSize);
    return true;
  }
  if (stripState[i][j] < goalState[i][j]) {
    stripState[i][j] += calculateStepSize(stripState[i][j], goalState[i][j], animationStepSize);
    return true;
  }
  return false;
}

uint8_t calculateStepSize(uint8_t state, uint8_t goal, uint8_t animationStepSize) {
  uint8_t difference = abs(state - goal);
  return difference > animationStepSize ? animationStepSize : 1;
}

void printStripState() {
  for (uint16_t i = 0; i < ledCount; i++) {
    Serial.print("| ");
    for (uint8_t j = 0; j < 4; j++) {
      Serial.print(stripState[i][j]);
      Serial.print(" | ");
    }
    Serial.println("");
  }
}

RGB hsvToRgb(float h, float s, float v) {
    float r, g, b;

    int i = (int)h / 60;
    float f = h / 60 - i;
    float p = v * (1 - s);
    float q = v * (1 - s * f);
    float t = v * (1 - s * (1 - f));

    switch (i % 6) {
        case 0: r = v, g = t, b = p; break;
        case 1: r = q, g = v, b = p; break;
        case 2: r = p, g = v, b = t; break;
        case 3: r = p, g = q, b = v; break;
        case 4: r = t, g = p, b = v; break;
        case 5: r = v, g = p, b = q; break;
    }

    RGB rgb = { (uint8_t)(r * 255), (uint8_t)(g * 255), (uint8_t)(b * 255) };
    return rgb;
}

HSV rgbToHsv(int r, int g, int b) {
  HSV hsv;
  float rPrime = r / 255.0;
  float gPrime = g / 255.0;
  float bPrime = b / 255.0;

  float cMax = (rPrime > gPrime) ? rPrime : gPrime;
  if (bPrime > cMax) {
    cMax = bPrime;
  }

  float cMin = (rPrime < gPrime) ? rPrime : gPrime;
  if (bPrime < cMin) {
    cMin = bPrime;
  }

  float delta = cMax - cMin;

  // Hue calculation
  if (delta == 0) {
    hsv.h = 0;
  } else if (cMax == rPrime) {
    hsv.h = 60 * fmod(((gPrime - bPrime) / delta), 6);
  } else if (cMax == gPrime) {
    hsv.h = 60 * (((bPrime - rPrime) / delta) + 2);
  } else {
    hsv.h = 60 * (((rPrime - gPrime) / delta) + 4);
  }

  // Saturation calculation
  hsv.s = (cMax == 0) ? 0 : (delta / cMax);

  // Value calculation
  hsv.v = cMax;

  // Adjust hue to be non-negative
  if (hsv.h < 0) {
    hsv.h += 360;
  }

  return hsv;
}
