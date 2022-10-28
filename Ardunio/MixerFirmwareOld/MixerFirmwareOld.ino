
#include "Adafruit_NeoKey_1x4.h"
#include "seesaw_neopixel.h"
#include <Adafruit_NeoPixel.h>

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SS_SWITCH        24
#define SS_NEOPIX        6

#define RING_BRIGHTNESS 25.5

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)

#define WIRE Wire
#define WIRE_SDA 8
#define WIRE_SCL 9

int rotary_addrs[4] = {0x36, 0x37, 0x3A, 0x38};
int key_addrs[2] = {0x30, 0x31};
int display_addrs[4] = {0x3C, 0x3D, 0x3C, 0x3D};

int ring_pixel_order[16] = {5, 4, 3, 2, 1, 0, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6};

char inData[20][255];

String inString = "";

int inputCount = 0;

Adafruit_SSD1306 displays[] = {
  Adafruit_SSD1306(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire1, OLED_RESET),
  Adafruit_SSD1306(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire1, OLED_RESET),
  Adafruit_SSD1306(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET),
  Adafruit_SSD1306(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET)
};


Adafruit_NeoKey_1x4 keys[] = {
  Adafruit_NeoKey_1x4('0', &Wire1),
  Adafruit_NeoKey_1x4('0', &Wire1),
};

// create 4 encoders!
Adafruit_seesaw encoders[4] = {
  Adafruit_seesaw(&Wire1),
  Adafruit_seesaw(&Wire1),
  Adafruit_seesaw(&Wire1),
  Adafruit_seesaw(&Wire1)
};
// create 4 encoder pixels
seesaw_NeoPixel encoder_pixels[4] = {
  seesaw_NeoPixel(1, SS_NEOPIX, NEO_GRB + NEO_KHZ800, &Wire1),
  seesaw_NeoPixel(1, SS_NEOPIX, NEO_GRB + NEO_KHZ800, &Wire1),
  seesaw_NeoPixel(1, SS_NEOPIX, NEO_GRB + NEO_KHZ800, &Wire1),
  seesaw_NeoPixel(1, SS_NEOPIX, NEO_GRB + NEO_KHZ800, &Wire1)
};

Adafruit_NeoPixel rings[4] = {
  Adafruit_NeoPixel(16, A0, NEO_GRB + NEO_KHZ800),
  Adafruit_NeoPixel(16, A1, NEO_GRB + NEO_KHZ800),
  Adafruit_NeoPixel(16, A2, NEO_GRB + NEO_KHZ800),
  Adafruit_NeoPixel(16, A3, NEO_GRB + NEO_KHZ800)
};


int32_t encoder_positions[] = {0, 0, 0, 0};
int32_t encoder_last_positions[] = {0, 0, 0, 0};
int32_t encoder_position_changes[] = {0, 0, 0, 0};
bool button_states[] = {false, false, false, false, false, false, false, false};
bool button_last_states[] = {false, false, false, false, false, false, false, false};
bool encoder_button_states[] = {false, false, false, false};
bool encoder_button_last_states[] = {false, false, false, false};

int ring_levels[4] = {0, 0, 0, 0};
String app_names[4] = {"none", "none", "none", "none"};

long ringColors[4] = {0xFF8000, 0xFF8000, 0xFF8000, 0xFF8000};
long buttonColors[8] = {0xFF8000, 0x0026FF, 0xFF8000, 0x0026FF, 0xFF8000, 0x0026FF, 0xFF8000, 0x0026FF};

bool serialFirstTime = true;

void serialWrite() {
  Serial.print("[");

  for (int i = 0; i < 4; i++) {
    Serial.print(encoder_position_changes[i]);
    Serial.print(",");
  }

  for (int i = 0; i < 8; i++) {
    Serial.print(button_states[i]);
    Serial.print(",");
  };
  for (int i = 0; i < 4; i++) {
    Serial.print(!encoder_button_states[i]);
    Serial.print(",");
  }
  Serial.println("]");
};



void setup() {

  WIRE.setSDA(WIRE_SDA);
  WIRE.setSCL(WIRE_SCL);
  WIRE.begin();

  pinMode(LED_BUILTIN, OUTPUT);

  for (int i = 0; i < 4; i++) {
    // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
    if (!displays[i].begin(SSD1306_SWITCHCAPVCC, display_addrs[i])) {
      //Serial.println(F("SSD1306 allocation failed"));
      for (;;); // Don't proceed, loop forever
    }
    displays[i].display();
    delay(500);
    displays[i].clearDisplay();
    displays[i].setTextSize(2);
    displays[i].setTextColor(SSD1306_WHITE);
    displays[i].setCursor(0, 0);
    displays[i].println("Display: " + String(i) + " @ " + String(display_addrs[i]));
    displays[i].display();

  }
  delay(500);

  for (int i = 0; i < 2; i ++) {
    //Serial.println("Starting NeoKey @ " + String(key_addrs[i]));
    if (! keys[i].begin(key_addrs[i])) {     // begin with I2C address, default is 0x30
      //Serial.println("Could not start NeoKey @ " + String(key_addrs[i]) + " , check wiring?");
      while (1) delay(10);
    }
    //Serial.println("Started NeoKey @ " + String(key_addrs[i]));
  }
  for (int i = 0; i < 4; i++) {

    Adafruit_seesaw ss = encoders[i];
    seesaw_NeoPixel sspixel = encoder_pixels[i];

    if (! ss.begin(rotary_addrs[i]) || ! sspixel.begin(rotary_addrs[i])) {
      //Serial.println("Couldn't find seesaw on: " + String(rotary_addrs[i]));
      while (1) delay(10);
    }
    //Serial.println("seesaw started on: " + String(rotary_addrs[i]));

    uint32_t version = ((ss.getVersion() >> 16) & 0xFFFF);
    if (version  != 4991) {
      //Serial.print("Wrong firmware loaded? ");
      //Serial.println(version);
      while (1) delay(10);
    }
    ///Serial.println("Found Product 4991");

    // set not so bright!
    sspixel.setBrightness(20);
    sspixel.show();

    encoders[i] = ss;
    encoder_pixels[i] = sspixel;

    encoder_positions[i] = encoders[i].getEncoderPosition();
  }

  for (int i = 0; i < 4; i++) {
    rings[i].begin();           // INITIALIZE NeoPixel strip object (REQUIRED)
    rings[i].setBrightness(RING_BRIGHTNESS); // Set BRIGHTNESS to about 1/5 (max = 255)
    rings[i].fill(rings[i].Color(0,   0,   255));
    rings[i].show();            // Turn OFF all pixels ASAP
  }

  for (int i = 0; i < 4; i++) {
    displays[i].clearDisplay();
    displays[i].display();
  }
  displays[0].setCursor(0, 0);
  displays[0].setTextSize(1);
  displays[0].print("No Connection");
  displays[0].display();

  Serial.begin(9600);
  while (! Serial) delay(10);
}

void loop() {
  if (Serial) {
    if (serialFirstTime) {
      delay(100);
      serialWrite();
      serialFirstTime = false;
    }
    for (int i = 0; i < 2; i++) {
      Adafruit_NeoKey_1x4 neokey = keys[i];
      uint8_t buttons = neokey.read();

      // Check each button, if pressed, light the matching neopixel

      if (buttons & (1 << 0)) {
        if (!button_states[4 * i]) {
          button_states[4 * i] = true;
          serialWrite();
        }
      } else {
        if (button_states[4 * i]) {
          button_states[4 * i] = false;
          serialWrite();
        }

      }

      if (buttons & (1 << 1)) {
        if (!button_states[(4 * i) + 1]) {
          button_states[(4 * i) + 1] = true;
          serialWrite();
        }
      } else {
        if (button_states[(4 * i) + 1]) {
          button_states[(4 * i) + 1] = false;
          serialWrite();
        }
      }

      if (buttons & (1 << 2)) {
        if (!button_states[(4 * i) + 2]) {
          button_states[(4 * i) + 2] = true;
          serialWrite();
        }
      } else {
        if (button_states[(4 * i) + 2]) {
          button_states[(4 * i) + 2] = false;
          serialWrite();
        }
      }

      if (buttons & (1 << 3)) {
        if (!button_states[(4 * i) + 3]) {
          button_states[(4 * i) + 3] = true;
          serialWrite();
        }
      } else {
        if (button_states[(4 * i) + 3]) {
          button_states[(4 * i) + 3] = false;
          serialWrite();
        }
      }
      for(int b = 0;b < 4; b++){
        neokey.pixels.setPixelColor(b, buttonColors[(4 * i) + b]);
      }
      neokey.pixels.show();
    }

    for (int i = 0; i < 4; i++) {
      encoder_positions[i] = encoders[i].getEncoderPosition();
      encoder_button_states[i] = encoders[i].digitalRead(SS_SWITCH);
      if (encoder_positions[i] != encoder_last_positions[i]) {
        encoder_position_changes[i] = encoder_positions[i] - encoder_last_positions[i];
        serialWrite();
        encoder_last_positions[i] = encoder_positions[i];
      } else {
        encoder_position_changes[i] = 0;
      }
      if (encoder_button_states[i] != encoder_button_last_states[i]) {
        serialWrite();
        encoder_button_last_states[i] = encoder_button_states[i];
      }
    }

    while (Serial.available() > 0) {
      //Serial.println("Start");
      Serial.readStringUntil(',').toCharArray(inData[inputCount], 255);
      //Serial.println(String(inData[inputCount]));
      inputCount ++;
      if (String(inData[inputCount - 1]) == "") {
        inputCount = 0;
        int displayCount = 0;

        for (int i = 0; i < 4; i++) {
          ringColors[i] = String(inData[i + 8]).toInt();
        }
        for (int i = 0; i < 8; i++) {
          buttonColors[i] = String(inData[i + 12]).toInt();
        }

        for (int i = 0; i <= 6; i += 2) {
          //Serial.println("Display Text: " + String(inData[i]));
          displays[displayCount].clearDisplay();
          displays[displayCount].setTextSize(2);
          displays[displayCount].setTextColor(SSD1306_WHITE);
          displays[displayCount].setCursor(0, 0);
          displays[displayCount].println(String(inData[i]));
          displays[displayCount].display();
          displayCount ++;

        }
        int ringCount = 0;
        for (int i = 1; i <= 7; i += 2)
        {
          ring_levels[ringCount] = String(inData[i]).toInt();
          //Serial.println("Ring count: " + String(ring_l/evels[ringCount] + 100));
          rings[ringCount].clear();
          for (int r = 0; r < ring_levels[ringCount]; r++) {
            rings[ringCount].setPixelColor(ring_pixel_order[r], ringColors[ringCount]);
          }
          rings[ringCount].show();
          ringCount++;

        }
      }
    }

  } else {
    for (int i = 0; i < 4; i++) {
      rings[i].fill(rings[i].Color(0,   0,   255));
      rings[i].show();            // Turn OFF all pixels ASAP
    }
    serialFirstTime = true;

  }
}
