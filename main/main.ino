#include <Wire.h>
#include <Adafruit_GFX.h>
#include <WS2812FX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define ADDRESSED_OPCODE_BIT0_LOW_TO_WRITE 0
#define OLED_RESET -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define OLED_WIRE_SDA 18  // Use GP2 as I2C1 SDA
#define OLED_WIRE_SCL 19  // Use GP3 as I2C1 SCL
#define OLED_ADDRESS 0x3C
#define ICON_HEIGHT   16
#define ICON_WIDTH    16

#define FX_MODE_STATIC 0
#define FX_MODE_BLINK 1
#define FX_MODE_BREATH 2
#define FX_MODE_COLOR_WIPE 3

#define UPDATES_PER_SECOND 100
#define LED_PIN     7
#define NUM_LEDS    3
#define BRIGHTNESS  64
#define LED_TYPE    WS2812
#define COLOR_ORDER GRB

#define PWR_EN 21
#define PWR_SAFE 20
#define MUTE_RLY 14
#define INPUT_RLY 10

#define PRI_V 28
#define NEG_V 29
#define BTN_PWR 24
#define BTN_MUTE 25
#define TRIG_PWR 23

#define SAMPLECOUNT 15

/*
 * Some custom icons for the oled
 */

  static const unsigned char PROGMEM unmute_bmp[] =
{ 0b00000000, 0b00000000,
  0b00000000, 0b00000100,
  0b00000000, 0b00001100,
  0b00000000, 0b00011100,
  0b00001111, 0b11111100,
  0b00011111, 0b11111100,
  0b00011111, 0b11111100,
  0b00011111, 0b11111100,
  0b00011111, 0b11111100,
  0b00011111, 0b11111100,
  0b00011111, 0b11111100,
  0b00001111, 0b11111100,
  0b00000000, 0b00011100,
  0b00000000, 0b00001100,
  0b00000000, 0b00000100,
  0b00000000, 0b00000000 };

  static const unsigned char PROGMEM muted_bmp[] =
{ 0b00000000, 0b00000000,
  0b00010000, 0b00000100,
  0b00001000, 0b00001100,
  0b00000100, 0b00010100,
  0b00001010, 0b11100100,
  0b00010001, 0b01000100,
  0b00010000, 0b10000100,
  0b00010000, 0b01000100,
  0b00010000, 0b00100100,
  0b00010000, 0b00010100,
  0b00010000, 0b01001000,
  0b00001111, 0b11100100,
  0b00000000, 0b00010010,
  0b00000000, 0b00001101,
  0b00000000, 0b00000100,
  0b00000000, 0b00000000 };

WS2812FX ws2812fx = WS2812FX(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);

int c=0;
int crst=100;

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire1, OLED_RESET);

int state_btn = 0;
int state_pwr = 0;
int state_muted = 1;

float primaryRail[SAMPLECOUNT];
float negativeRail[SAMPLECOUNT];
float primaryRailAvg=0;
float negativeRailAvg=0;
float primaryRailAvgOld=0;
float negativeRailAvgOld=0;
int primarySag=0;                 //Used to make sure spurious analog reads trip
int negativeSag=0;                //Used to make sure spurious analog reads trip

int PTO=0;                        //Power TimeOut, used make sure power on sequence does not become stuck in a loop
int btn_press=0;

void setup() {
  //while (!Serial);
  delay(100);
  Serial.println("Starting Program!");
  // put your setup code here, to run once:
  pinMode(PWR_EN, OUTPUT);
  pinMode(PWR_SAFE, OUTPUT);
  pinMode(MUTE_RLY, OUTPUT);
  pinMode(INPUT_RLY, OUTPUT);

  pinMode(BTN_PWR, INPUT);
  pinMode(BTN_MUTE, INPUT);

  /*mute();
  delay(100);
  digitalWrite(INPUT_RLY, LOW);
  delay(100);
  digitalWrite(PWR_SAFE, LOW);
  delay(100);
  digitalWrite(, LOW);*/
  
  // Initialize the strip
  ws2812fx.init();
  // Set the LED’s overall brightness. 0=strip off, 255=strip at full intensity
  ws2812fx.setBrightness(BRIGHTNESS);
  // Set the animation speed. 10=very fast, 5000=very slow
  ws2812fx.setSpeed(200);
  // Set the color of the LEDs
  ws2812fx.setColor(RED);
  ws2812fx.setSegment(0, 0, 0, FX_MODE_BREATH, BLUE, 200, false);
  ws2812fx.setSegment(1, 1, 1, FX_MODE_STATIC, RED, 200, false);
  ws2812fx.setSegment(2, 2, 2, FX_MODE_STATIC, GREEN, 200, false);
  // Start the animation
  ws2812fx.start();

  delay(500);
  
  Wire1.setSDA(OLED_WIRE_SDA);
  Wire1.setSCL(OLED_WIRE_SCL);
  Wire1.begin();

  display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS);
  display.clearDisplay();
  display.display();
  
  //digitalWrite(, !digitalRead());
  //digitalWrite(PWR_SAFE, !digitalRead(PWR_SAFE));
  display.clearDisplay();
  display.setTextSize(2);             // Normal 1:1 pixel scale
  display.setTextColor(SSD1306_WHITE);        // Draw white text

  if (analogRead(PRI_V) < 60) {
    ws2812fx.setSegment(0, 0, 0, FX_MODE_BLINK, RED, 200, false);
    ws2812fx.setSegment(1, 1, 1, FX_MODE_BLINK, RED, 200, false);
    
    display.setCursor(0, 0);     //<pixels from left>, <pixels from top>; 0,0 is top left corner
    display.println("Low");
    display.setCursor(0, 32);     //<pixels from left>, <pixels from top>; 0,0 is top left corner
    display.println("Voltage");
    display.display();
    while (analogRead(PRI_V) < 60) {
      delay(250);
    }
    display.clearDisplay();
    display.display();
  } else {
    display.setCursor(0, 0);     //<pixels from left>, <pixels from top>; 0,0 is top left corner
    //display.println("Hello");
  }

  display.display();
  Serial.println("Program Started");
}

void setup1() {
  Serial.begin(115200);
}

void loop() {
  // put your main code here, to run repeatedly:
  //Serial.println(".");
  float primSum=0;
  float negSum=0;
  //float input_v = analogRead(PRI_V)*0.1406923076923077;
  int input_v = analogRead(PRI_V)/10;
  if (input_v < 8.5) {
    
    //do something to indicate brown out condition
  }
  for (int i = 0; i < (SAMPLECOUNT - 1); ++i) {
    primaryRail[i+1]=primaryRail[i];
    primSum=primSum+primaryRail[i+1];
  }
  primaryRail[0]=input_v*1.5;
  primSum=primSum+(input_v*1.5);
  primaryRailAvg=roundToOneDecimalPlace((primSum/SAMPLECOUNT));

  if (state_pwr > 0) {
    //float audio_v = analogRead(NEG_V)*0.0958227848101266;
    int audio_v = analogRead(NEG_V)/10;
    for (int i = 0; i < (SAMPLECOUNT - 1); ++i) {
      negativeRail[i+1]=negativeRail[i];
      negSum=negSum+negativeRail[i+1];
    }
    negativeRail[0]=audio_v;
    negSum=negSum+(audio_v);
    negativeRailAvg=roundToOneDecimalPlace((negSum/SAMPLECOUNT));

    if (negativeRailAvg < 13 || primaryRailAvg < 8 && state_pwr == 2) {
      //power_off();
      updateDisplayDiagnostic();
    }
  }
  
}

void loop1() {
  ++c;
  ws2812fx.service();
/*
  if (!digitalRead(BTN_PWR) && state_btn == 0) {
    state_btn = BTN_PWR;
    if (state_pwr == 0) {
      power_on();
    } else if (state_pwr > 1) {
      power_off();
    }
  } else if (!digitalRead(BTN_MUTE) && state_btn == 0) {
    state_btn = BTN_MUTE;
    if (state_muted == 1) {
      unmute();
    } else {
      mute();
    }
  } else if (digitalRead(BTN_MUTE) && digitalRead(BTN_PWR)) {
    state_btn = 0;
  }
*/
  int key = keyboard_get();
  if (key == BTN_PWR) {
    Serial.println("power button push");
    toggle_pwr();
  } else if (key == BTN_MUTE) {
    Serial.println("mute button push");
    toggle_mute();
  }
  
  if (c > crst) {
    if (primaryRailAvgOld != primaryRailAvg || negativeRailAvgOld != negativeRailAvg) {
      printPowerRailState();
      primaryRailAvgOld=primaryRailAvg;
      negativeRailAvgOld=negativeRailAvg;
      if (state_pwr > 2) {
        updateDisplayDiagnostic();
      }
    }
    if (state_pwr > 0) {
      power_state();
    }
    c=0;
  }
  delay(1000 / UPDATES_PER_SECOND);
  //delay(10);
}

void toggle_pwr() {
  if(state_pwr == 0) {
    power_on();
  } else if (PTO < 1) {
    power_off();
  }
  delay(100);
}

void toggle_mute() {
  if (state_muted == 1) {
    unmute();
  } else {
    mute();
  }
  delay(100);
}

int keyboard_get() {
  int k = 0;
  if (!digitalRead(BTN_PWR)) {
    k = BTN_PWR;
  } else if (!digitalRead(BTN_MUTE)) {
    k = BTN_MUTE;
  } else {
    btn_press = 0;
  }

  if (btn_press == 0) {
    btn_press = k;
  } else {
    k = 0;
  }
  return k;
}

float roundToOneDecimalPlace(float value) {
  return round(value * 10) / 10.0;
}

void printPowerRailState() {
  Serial.print("Primary Rail:");
  Serial.print(primaryRailAvg);
  Serial.println("V");
  Serial.print("Negative Rail:-");
  Serial.print(negativeRailAvg);
  Serial.println("V");
}

void power_on() {
  if (state_pwr > 0) {
    return;
  } else {
    Serial.println("Powering On!");
    ws2812fx.setSegment(1, 1, 1, FX_MODE_BLINK, RED, 200, false); 
    display.clearDisplay();
    display.setTextSize(2);             // Normal 1:1 pixel scale
    display.setTextColor(SSD1306_WHITE);        // Draw white text
    display.setCursor(0, 0);     //<pixels from left>, <pixels from top>; 0,0 is top left corner
    display.println("O");
    display.display();
    digitalWrite(PWR_EN, HIGH);
    PTO = 30;
    state_pwr = 1;

  }
}

void power_state() {
  if (PTO > 0) {
    PTO=PTO-1;
    if (negativeRailAvg > 12 && primaryRailAvg > 8) {
      PTO=0;
      state_pwr = 2;
      display.clearDisplay();
      display.setTextSize(2);             // Normal 1:1 pixel scale
      display.setTextColor(SSD1306_WHITE);        // Draw white text
      display.setCursor(32, 0);     //<pixels from left>, <pixels from top>; 0,0 is top left corner
      display.println("Powering");
      display.setCursor(32, 32);     //<pixels from left>, <pixels from top>; 0,0 is top left corner
      display.println("On!");
      display.display();
      digitalWrite(PWR_SAFE, HIGH);
      digitalWrite(INPUT_RLY, HIGH);
      delay(100);
      unmute();
      ws2812fx.setSegment(1, 1, 1, FX_MODE_STATIC, GREEN, 200, false);
      updateDisplayDiagnostic();
    } else {
      Serial.println("Waiting for power supply");
      printPowerRailState();
    }
  } else if (state_pwr == 1) {
    Serial.println("Power On Failed!");
    power_off();
    digitalWrite(PWR_EN, LOW);
    state_pwr = 11;
  }
}

void power_off() {
  Serial.println("Powering Off!");
  display.clearDisplay();
  display.setTextSize(2);             // Normal 1:1 pixel scale
  display.setTextColor(SSD1306_WHITE);        // Draw white text
  display.setCursor(32, 0);     //<pixels from left>, <pixels from top>; 0,0 is top left corner
  display.println("Powering");
  display.setCursor(32, 32);     //<pixels from left>, <pixels from top>; 0,0 is top left corner
  display.println("Off!");
  display.display();
  mute();
  delay(250);
  digitalWrite(INPUT_RLY, LOW);
  delay(250);
  digitalWrite(PWR_SAFE, LOW);
  delay(1000);
  digitalWrite(PWR_EN, LOW);
  display.clearDisplay();
  display.display();
  ws2812fx.setSegment(1, 1, 1, FX_MODE_STATIC, RED, 200, false);
  ws2812fx.setSegment(2, 2, 2, FX_MODE_STATIC, WHITE, 200, false);
  state_pwr=0;
}

void mute() {
  digitalWrite(MUTE_RLY, LOW);
  ws2812fx.setSegment(2, 2, 2, FX_MODE_STATIC, RED, 200, false);
  state_muted = 1;
  drawMuteState();
}


void unmute() {
  digitalWrite(MUTE_RLY, HIGH);
  ws2812fx.setSegment(2, 2, 2, FX_MODE_STATIC, WHITE, 200, false);
  state_muted = 0;
  drawMuteState();
}

void drawMuteState() {
  display.fillRect(0, 2, ICON_WIDTH, ICON_HEIGHT, SSD1306_BLACK);
  if (state_muted == 0) {
     display.drawBitmap(
      (0 ),
      (2 - 1),
      unmute_bmp, ICON_WIDTH, ICON_HEIGHT, 1);
  } else {
    display.drawBitmap(
      (0 ),
      (2 - 1),
      muted_bmp, ICON_WIDTH, ICON_HEIGHT, 1);
  }
  display.display();
}

void updateDisplayDiagnostic() {
  display.clearDisplay();
  display.setTextSize(2);             // Normal 1:1 pixel scale
  display.setTextColor(SSD1306_WHITE);        // Draw white text
  display.setCursor(80, 0);     //<pixels from left>, <pixels from top>; 0,0 is top left corner
  display.print(primaryRailAvg, 1);
  display.print("V");
  display.setCursor(56, 32);     //<pixels from left>, <pixels from top>; 0,0 is top left corner
  display.print("-");
  display.print(negativeRailAvg,1);
  display.print("V");
  drawMuteState();
  display.display();
}
