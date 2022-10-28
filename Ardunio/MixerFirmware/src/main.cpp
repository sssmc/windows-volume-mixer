#include <Arduino.h>

#include "Adafruit_NeoKey_1x4.h"
#include "seesaw_neopixel.h"
#include <Adafruit_NeoPixel.h>

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

//Pins for encoder seesaw chip
#define SS_SWITCH        24
#define SS_NEOPIX        6

//Max brightness for the neopixel rings
#define RING_BRIGHTNESS 25.5

//OLED screens
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)

//I2C Buses and pins(Wire1 = Stemma Qt connector, Wire = feather wing)
#define WIRE Wire
#define WIRE1 Wire1
#define WIRE_SDA 8
#define WIRE_SCL 9
#define WIRE1_SDA 2
#define WIRE1_SCL 3

//Serial Baud Rate
#define SERIAL_BAUD_RATE 4000000

int inputCount = 0;

//I2C addresses
int rotary_addrs[4] = {0x36, 0x37, 0x3A, 0x38};
int key_addrs[2] = {0x30, 0x31};
int display_addrs[4] = {0x3C, 0x3D, 0x3C, 0x3D};

//Order to turn on the pixels in the neopixel rings
int ring_pixel_order[16] = {5, 4, 3, 2, 1, 0, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6};

//input data from the serial bus(20 char arrays, max 255 chars each)
char inData[20][255];

//OLED display objects
Adafruit_SSD1306 displays[] = {
  Adafruit_SSD1306(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire1, OLED_RESET),
  Adafruit_SSD1306(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire1, OLED_RESET),
  Adafruit_SSD1306(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET),
  Adafruit_SSD1306(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET)
};

//Neo Key objects
Adafruit_NeoKey_1x4 keys[] = {
  Adafruit_NeoKey_1x4('0', &Wire1),
  Adafruit_NeoKey_1x4('0', &Wire1),
};

//Encoder objects
Adafruit_seesaw encoders[4] = {
  Adafruit_seesaw(&Wire1),
  Adafruit_seesaw(&Wire1),
  Adafruit_seesaw(&Wire1),
  Adafruit_seesaw(&Wire1)
};

//Neopixel ring objects(with pins)
Adafruit_NeoPixel rings[4] = {
  Adafruit_NeoPixel(16, A0, NEO_GRB + NEO_KHZ800),
  Adafruit_NeoPixel(16, A1, NEO_GRB + NEO_KHZ800),
  Adafruit_NeoPixel(16, A2, NEO_GRB + NEO_KHZ800),
  Adafruit_NeoPixel(16, A3, NEO_GRB + NEO_KHZ800)
};

//Encoder postion vars
int32_t encoder_positions[] = {0, 0, 0, 0};
int32_t encoder_last_positions[] = {0, 0, 0, 0};
int32_t encoder_position_changes[] = {0, 0, 0, 0};

//Encoder button vars
bool encoder_button_states[] = {false, false, false, false};
bool encoder_button_last_states[] = {false, false, false, false};

//Neokey button state vars
bool button_states[] = {false, false, false, false, false, false, false, false};
bool button_last_states[] = {false, false, false, false, false, false, false, false};

//Colors of the neokey button backlights(hex)
int buttonColors[8] = {0xFF8000, 0x0026FF, 0xFF8000, 0x0026FF, 0xFF8000, 0x0026FF, 0xFF8000, 0x0026FF};

//Neopixel ring levels(0-16)
int ring_levels[4] = {0, 0, 0, 0};

//Color of the neopixel rings(hex)
int ringColors[4] = {0xFF8000, 0xFF8000, 0xFF8000, 0xFF8000};

//No connection neopixel ring color
int NO_CONNECT_COLOR = 0x0000FF;

//No connection display text
String NO_CONNECTION_TEXT = "No Connection";

//OLED display texts
char appNames[4][255];

//Is it the first time going thorugh the loop since the serial was connected
bool serialFirstTime = true;


//--Function Prototypes--//

//Write the data to serial
//Formate: [(encoder positon changes)x4, (neokey button states)x8, (encoder button states)x4]
void serialWrite();

//Read data from serial
void serialRead();

//Get the neokey button states and update the neopixle backlight colors
void getKeys();

//Get encoder position and button data
void getEncoders();

//Set the level and color of the neopixel rings
void setRings();

//Set the text for the OLED displays
void setDisplays();

//Set neopixel rings to the no connection color and set a no connection message on the first screen
//Option so clear all displays
void setNoConnection(bool clearDisplay);

void setup() {

  //Setup and start the I2C busses
  WIRE.setSDA(WIRE_SDA);
  WIRE.setSCL(WIRE_SCL);
  WIRE.begin();

  WIRE1.setSDA(WIRE1_SDA);
  WIRE1.setSCL(WIRE1_SCL);
  WIRE1.begin();

  //Start the OLED displays
  for (int i = 0; i < 4; i++) {
    if (!displays[i].begin(SSD1306_SWITCHCAPVCC, display_addrs[i])) {
      for (;;); // Can't start display, don't proceed, loop forever
    }
    displays[i].display();//Show the default splash screen
  }

  //Start the neokeys
  for (int i = 0; i < 2; i ++) {
    if (! keys[i].begin(key_addrs[i])) {
      while (1) delay(10);
    }
  }
  
  //Start the endoders
  for (int i = 0; i < 4; i++) {

    Adafruit_seesaw ss = encoders[i];

    if (! ss.begin(rotary_addrs[i])) {
      while (1) delay(10); //Can't start, loop forever
    }
    uint32_t version = ((ss.getVersion() >> 16) & 0xFFFF);
    if (version  != 4991) {
      while (1) delay(10); //Incorrect seesaw firware version, loop forever
    }
    encoders[i] = ss;

    //Set intial positions for the encoders
    encoder_positions[i] = encoders[i].getEncoderPosition();
  }

  //Start the neopixel rings
  for (int i = 0; i < 4; i++) {
    rings[i].begin();

    //Set max brightness
    rings[i].setBrightness(RING_BRIGHTNESS);
  }

  //Show a no connection message on the first screen and set rings to no connection color
  setNoConnection(false);

  //Start the serial connection and wait until the pc connect before proceding
  Serial.begin(SERIAL_BAUD_RATE);

}

void loop() {
  
  //If the pc is connected to the serial
  if (Serial) {
    //If there is the first loop since the serial was connected, sent data to get a respose from the pc
    if (serialFirstTime) {
      serialWrite();
      serialFirstTime = false;
    }

    //Neokeys
    getKeys();

    //Encoders
    getEncoders();

    //While they are bytes in the serial buffer, read data and set rings/displays
    while (Serial.available() > 0) {

        serialRead();
      }

  } else {
    //If there is no serial connection, set ring and displays to no connection state

    setNoConnection(true);

    serialFirstTime = true;

  }
}

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
}

void getKeys(){
  for (int i = 0; i < 2; i++) {
      Adafruit_NeoKey_1x4 neokey = keys[i];
      uint8_t buttons = neokey.read();

      // Check each button. Ff the state has changed since last check, write to serial
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
      //Set the button backlight neopixel colors
      for(int b = 0;b < 4; b++){
        neokey.pixels.setPixelColor(b, buttonColors[(4 * i) + b]);
      }
      neokey.pixels.show();
    }
}

void getEncoders(){
  for (int i = 0; i < 4; i++) {
      //Get encoder positons and button states
      encoder_positions[i] = encoders[i].getEncoderPosition();
      encoder_button_states[i] = encoders[i].digitalRead(SS_SWITCH);

      //If the encoder positon has chages since the last check, calculate the position diffrence since last check and write to serial
      if (encoder_positions[i] != encoder_last_positions[i]) {
        encoder_position_changes[i] = encoder_positions[i] - encoder_last_positions[i];
        serialWrite();
        //Reset last position
        encoder_last_positions[i] = encoder_positions[i];
      } else {
        //If there has been no change, set position difference to 0
        encoder_position_changes[i] = 0;
      }

      //If the encoder button has changed since last check, update state and write to serial
      if (encoder_button_states[i] != encoder_button_last_states[i]) {
        serialWrite();
        encoder_button_last_states[i] = encoder_button_states[i];
      }
    }
}

void setDisplays(){

  //Set each display to its app name
  for (int i = 0; i < 4; i ++) {
    displays[i].clearDisplay();
    displays[i].setTextSize(2);
    displays[i].setTextColor(SSD1306_WHITE);
    displays[i].setCursor(0, 0);
    displays[i].println(String(appNames[i]));
    displays[i].display();
  }

}

void setRings(){

  for (int i = 0; i < 4; i++)
    {
      //Clear the ring
      rings[i].clear();

      //Set the number of pixels lit on the ring base on the ring level(0-16)
      for (int r = 0; r < ring_levels[i]; r++) {
        //Use the the custom pixel order to choose the order that the pixels are turned on
        //Use the ring color of that ring to dertimin the color
        rings[i].setPixelColor(ring_pixel_order[r], ringColors[i]);
      }
      //Show the pixels
      rings[i].show();
    }

}

void serialRead(){

  //Read string until a comma is reached(end of value), String is then converted into char array and added to in data array
  Serial.readStringUntil(',').toCharArray(inData[inputCount], 255);

  //Keep track of how many values have been read on the current line
  inputCount ++;

  //If we have reached the end of the line(empty string caused by the ",," at the end of the line), process line
  if (String(inData[inputCount - 1]) == "") {

    //Reset line value count
    inputCount = 0;
    int displayCount = 0;

    //Read ring and button colors(as ints)
    for (int i = 0; i < 4; i++) {
      ringColors[i] = String(inData[i + 8]).toInt();
    }
    for (int i = 0; i < 8; i++) {
      buttonColors[i] = String(inData[i + 12]).toInt();
    }

    //Read the display text to app name array
    for (int i = 0; i <= 6; i += 2) {
      String(inData[i]).toCharArray(appNames[displayCount],255);
      displayCount ++;

    }
    int ringCount = 0;
    for (int i = 1; i <= 7; i += 2)
    {
      ring_levels[ringCount] = String(inData[i]).toInt();
      ringCount++;

    }

    setDisplays();

    setRings();

  }
}

void setNoConnection(bool clearDisplay){
  //Clear right three displays if needed
  if(clearDisplay){
    for(int i = 1; i < 4; i++){
      displays[i].clearDisplay();
      displays[i].display();
    }
  }

  //Add no connection text to the first display
  displays[0].clearDisplay();
  displays[0].setTextColor(SSD1306_WHITE);
  displays[0].setCursor(0, 0);
  displays[0].setTextSize(1);
  displays[0].print(NO_CONNECTION_TEXT);
  displays[0].display();

  //Set the neopixel rings to the no connection color
  for (int i = 0; i < 4; i++) {
      rings[i].fill(NO_CONNECT_COLOR);
      rings[i].show();
  }

}
