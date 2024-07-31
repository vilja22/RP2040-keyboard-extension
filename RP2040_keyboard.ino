 
/* Layout
|RE| |A1|A2|A3|A4| |B1|B2|B3|B4|  |C1|C2| |D1|D2|D3| 
|FN|  |A5|A6|A7|A8| |B5|B6|B7|B8| |C3|C4| |D4|D5|D6| *Not actual codes used anywhere

// Default mode
FN + RE -> RGB Settings
Scroll -> Volume
FN + Scroll -> F21 / F22

// RGB Settings
Scroll -> Brightness
FN + RE -> Save & exit
FN + D3 -> Save & exit
FN + D1 -> Exit without saving
D1 -> Raibow RGB
D2 -> Pride RGB
D3 -> Pink and purple RGB
D4 -> Custom colour RGB
    FN + D4 -> Edit RED brightness with scroll
    FN + D5 -> Edit GREEN brightness with scroll
    FN + D6 -> Edit BLUE brightness with scroll
D5 -> White RGB
D6 -> RGB off
*/

#include <SPI.h>
#include <Wire.h>
#include <RotaryEncoder.h>
#include <light_CD74HC4067.h>
#include <Keyboard.h>
#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
#include <avr/power.h> // Required for 16 MHz Adafruit Trinket
#endif
#include <hardware/flash.h>
#include <hardware/sync.h>

#define FLASH_TARGET_OFFSET (PICO_FLASH_SIZE_BYTES - FLASH_SECTOR_SIZE)



// Binds
// You shouldnt use first keys of either row since they are used to control the keyboard
uint8_t bindsProfile1[] = {
  0, KEY_F1,  KEY_F2,  KEY_F3,  KEY_F4,  KEY_F5, KEY_F6,  KEY_F7,  KEY_F8,  KEY_INSERT, KEY_PAGE_UP,   KEY_HOME,       KEY_UP_ARROW,   KEY_END, // Top row
  0, KEY_F13, KEY_F14, KEY_F15, KEY_F16, KEY_F9, KEY_F10, KEY_F11, KEY_F12, KEY_DELETE, KEY_PAGE_DOWN, KEY_LEFT_ARROW, KEY_DOWN_ARROW, KEY_RIGHT_ARROW // Bottom row
};

uint8_t bindsProfile2[] = {
  0, KEY_F13, KEY_F14, KEY_F15, KEY_F16, KEY_F9, KEY_F10, KEY_F11, KEY_F12, KEY_F16, KEY_F19,  KEY_HOME,   KEY_INSERT, KEY_PAGE_UP, // Top row
  0, KEY_F1,  KEY_F2,  KEY_F3,  KEY_F4,  KEY_F5, KEY_F6,  KEY_F7,  KEY_F8,  KEY_F17, KEY_F18,  KEY_DELETE, KEY_END,    KEY_PAGE_DOWN // Bottom row
};

// Rotatory encoder binds are Volume up and down in profiles 1 and 2, And F21, F22 when function Key is pressed down

// Button/Bind mapping
uint8_t outputs[2][12] = {
  {
    bindsProfile1[0], bindsProfile1[1], bindsProfile1[15], bindsProfile1[16], bindsProfile1[3], bindsProfile1[17], 
    bindsProfile1[18], bindsProfile1[5], bindsProfile1[19], bindsProfile1[2], bindsProfile1[4], bindsProfile1[14]
  },{
    bindsProfile2[0], bindsProfile2[1], bindsProfile2[15], bindsProfile2[16], bindsProfile2[3], bindsProfile2[17], 
    bindsProfile2[18], bindsProfile2[5], bindsProfile2[19], bindsProfile2[2], bindsProfile2[4], bindsProfile2[14]
  }
}; 

uint8_t muxoutputs[2][16] = {
  {
    bindsProfile1[6], bindsProfile1[10], bindsProfile1[7], bindsProfile1[20], bindsProfile1[21], bindsProfile1[8], bindsProfile1[22], 
    bindsProfile1[9], bindsProfile1[23], bindsProfile1[24], bindsProfile1[11], bindsProfile1[25], bindsProfile1[12], bindsProfile1[26], 
    bindsProfile1[13], bindsProfile1[27]
  },{
    bindsProfile2[6], bindsProfile2[10], bindsProfile2[7], bindsProfile2[20], bindsProfile2[21], bindsProfile2[8], bindsProfile2[22], 
    bindsProfile2[9], bindsProfile2[23], bindsProfile2[24], bindsProfile2[11], bindsProfile2[25], bindsProfile2[12], bindsProfile2[26], 
    bindsProfile2[13], bindsProfile2[27]
  }
};

// RGB
class Strip {
public:
  uint8_t   effect;
  uint8_t   effects;
  uint16_t  effStep;
  unsigned long effStart;
  Adafruit_NeoPixel strip;
  Strip(uint16_t leds, uint8_t pin, uint8_t toteffects, uint16_t striptype) : strip(leds, pin, striptype) {
    effect = -1;
    effects = toteffects;
    Reset();
  }
  void Reset(){
    effStep = 0;
    effect = (effect + 1) % effects;
    effStart = millis();
  }
};

struct Loop {
  uint8_t currentChild;
  uint8_t childs;
  bool timeBased;
  uint16_t cycles;
  uint16_t currentTime;
  Loop(uint8_t totchilds, bool timebased, uint16_t tottime) {currentTime=0;currentChild=0;childs=totchilds;timeBased=timebased;cycles=tottime;}
};

Strip rgbStrip(40, 20, 40, NEO_GRB + NEO_KHZ800);
struct Loop strip0loop0(2, false, 1);

struct Profile {
  uint8_t num;
  bool animated;
  uint8_t rgbProfile;
  float brightness;
  float defaultBrightness;
};

// Only used if no profiles stored on flash
Profile profiles[2]{
  {0, false, 4, 16, 16},
  {1, true, 2, 16, 16}
};
uint8_t customRgbValues[3] = {200,200,200};
Profile currentProfile = profiles[0];

// Saving data
int buf[FLASH_PAGE_SIZE/sizeof(profiles[2])];  // One page buffer of ints
int *p, addr;
unsigned int page; // prevent comparison of unsigned and signed int
int first_empty_page = -1;
 
// Rotatory encoder
const uint8_t PIN_ROTA = 1;
const uint8_t PIN_ROTB = 0;
RotaryEncoder encoder(PIN_ROTA, PIN_ROTB, RotaryEncoder::LatchMode::FOUR3);
void checkPosition() {  encoder.tick(); } 
int encoder_pos = 0;

// Buttons connected directly
const uint8_t PIN_ROT_SWITCH = 2;
const uint8_t PIN_B1 = 3; 
const uint8_t PIN_B2 = 4;
const uint8_t PIN_B3 = 5; 
const uint8_t PIN_B4 = 11;
const uint8_t PIN_B5 = 12;
const uint8_t PIN_B6 = 13;
const uint8_t PIN_B7 = 14;
const uint8_t PIN_B8 = 16;
const uint8_t PIN_B9 = 17;
const uint8_t PIN_B10 = 18; 
const uint8_t PIN_B11 = 19; 

// Mux
const uint8_t PIN_MUX1 = 6;
const uint8_t PIN_MUX2 = 7;
const uint8_t PIN_MUX3 = 8;
const uint8_t PIN_MUX4 = 9;
const uint8_t PIN_MUX_S = 10; // Singal

uint8_t inputs[] = {PIN_B1,  PIN_B2, PIN_B3, PIN_B4,  PIN_B5, PIN_B6,  PIN_B7, PIN_B8,  PIN_B9, PIN_B10, PIN_B11, PIN_ROT_SWITCH};
uint16_t bstates = 0; // Stores state of switches
uint16_t muxstates = 0; // Stores state of switches
CD74HC4067 mux(PIN_MUX1, PIN_MUX2, PIN_MUX3, PIN_MUX4);

// Presence sensor
const uint8_t SENSOR = 21; 

// Variables
unsigned long StartTime = 0;
unsigned long StartTime2 = 0;
unsigned long deBounceTimer = 0; //  Used to prevent accidental doubleclicks on rotatory button

unsigned long sleepModeTimer = 0;
int timeToSleep = 60; // S to go to sleep mode
uint8_t sleepModeBrigtness = 0;
bool sleepMode = 0;
bool sleepModeInit = 0;
bool bSleepModeAnimation = false;

bool rgbSettings = 0;
bool bUpdateStaticRgb = true;
uint8_t editCustomRgb = 0;

uint8_t internalT = 0;
float longestTime = 0;
int loops = 0;
int hz = 0;

bool functionDown = 0;
bool functionReset = 0;

uint32_t combinedData = 0;
uint32_t combinedDataCustomRgb = 0;

void setup() {
  Keyboard.begin();

  // Cheks if there are saved profiles in flash and loads them
  decodeData();
  currentProfile = profiles[0];

  // Set rotary encoder inputs and interrupts
  pinMode(PIN_ROTA, INPUT_PULLUP);
  pinMode(PIN_ROTB, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(PIN_ROTA), checkPosition, CHANGE);
  attachInterrupt(digitalPinToInterrupt(PIN_ROTB), checkPosition, CHANGE);  
  pinMode(PIN_MUX_S, INPUT_PULLUP);
  pinMode(SENSOR, INPUT_PULLUP);
  for (int i = 0; i < 12; i++){
    pinMode(inputs[i], INPUT_PULLUP);
  }

  // RGB
  #if defined(__AVR_ATtiny85__) && (F_CPU == 8000000)
  clock_prescale_set(clock_div_1);
  #endif
  rgbStrip.strip.begin();

  if (!currentProfile.animated){
    refreshStaticRgb();
  };

  // Timers
  StartTime = millis();
  StartTime2 = micros();
}

void loop() {
  // Updates rgb
  if (currentProfile.animated){  
    // Updates animation
    strips_loop();
  }
  else {
    // Updates static rgb statusindicators
    updateStaticRgb(); 
  }
  // Precense detection / sleepmode intialisiatio
  precenseDetection();
  // Dims / brightens rgb 
  if (bSleepModeAnimation){
    sleepModeAnimation();
  }
  // Cheks performance / other measurments
  updateInternalData();
  // Rotatory encoder
  updateRotatoryEncoder();
  // Buttons
  updateRotatoryButton();
  updateButtons();
}

// Buttons
void updateButtons(){
  // Buttons connected directly (6 first of both rows including rotatory button & function)
  for (int i = 0; i < 11; i++){
    if (!digitalRead(inputs[i]) && (bstates & (1 << i)) == 0){
      Keyboard.press(outputs[currentProfile.num][i]);
      bstates ^= (1 << i);
      
      // If function key is pressed
      if (i == 0){
        functionDown = 1;
        functionReset = 1;
        currentProfile.brightness = currentProfile.brightness * 1.5;
      }
    }
    else if (digitalRead(inputs[i]) && (bstates & (1 << i)) != 0){
      Keyboard.release(outputs[currentProfile.num][i]);
      bstates ^= (1 << i);

      if (i == 0){
        functionDown = 0;
        currentProfile.brightness = currentProfile.defaultBrightness;
      }
    }
  }

  // Buttons connected via mux (8 last of both rows)
  for (byte i = 0; i < 16; i++) {
    mux.channel(i);
    delayMicroseconds(2);
    if (!digitalRead(PIN_MUX_S) && (muxstates & (1 << i)) == 0){
      if (rgbSettings && i >= 10){ 
        rgbProfileLoader(i);
      }
      else {
        Keyboard.press(muxoutputs[currentProfile.num][i]);
      }
      muxstates ^= (1 << i);
    }
    else if (digitalRead(PIN_MUX_S) && (muxstates & (1 << i)) != 0){
      Keyboard.release(muxoutputs[currentProfile.num][i]);
      muxstates ^= (1 << i);
    }
  }
}

void changeProfile(int newProfile){
  profiles[currentProfile.num] = currentProfile; // saves old profile
  currentProfile = profiles[newProfile];
  if (!currentProfile.animated){
    bUpdateStaticRgb = true;
  }
}

void sleepModeAnimation(){
  if(loops % 500 == 1){
    if (sleepMode && currentProfile.brightness != sleepModeBrigtness){
      currentProfile.brightness -= currentProfile.defaultBrightness / (currentProfile.defaultBrightness / 1.5);
      refreshStaticRgb();
      if (currentProfile.brightness <= sleepModeBrigtness){
        bSleepModeAnimation = false;
        currentProfile.brightness = sleepModeBrigtness;
      }
    }
    if (!sleepMode && currentProfile.brightness != currentProfile.defaultBrightness && !functionDown){
      currentProfile.brightness += currentProfile.defaultBrightness / (currentProfile.defaultBrightness / 1.5);
      refreshStaticRgb();
      if (currentProfile.brightness >= currentProfile.defaultBrightness){
        currentProfile.brightness = currentProfile.defaultBrightness;
        bSleepModeAnimation = false;
      }
    }
  }
}


void updateRotatoryButton(){
  if (!digitalRead(inputs[11])){
    if ((millis() - deBounceTimer) > 50){
      if (functionDown){ // Toggles rgbSettings
        if (rgbSettings){
          // Saves profiles to flash
          editCustomRgb = 0;
          saveData();
        }
        rgbSettings = !rgbSettings;
      }
      else{ // Toggles keyboard profile
        if (currentProfile.num == 0){
          changeProfile(1);
        }
        else{
          changeProfile(0);
        }
      }
    }
    deBounceTimer = millis();
  }
  else if (!digitalRead(inputs[11])){
    if (bstates & (1 << 11) != 0){
      bstates ^= (1 << 11);
    }
  }
}

void updateRotatoryEncoder(){
  encoder.tick();         
  int newPos = encoder.getPosition();
  if (encoder_pos != newPos) {
    // Changes brightenss
    if (rgbSettings){ 
      if (editCustomRgb > 0){
        if (newPos > encoder_pos){
          if (customRgbValues[editCustomRgb - 1] < 3){
            customRgbValues[editCustomRgb - 1] = 0;
          }
          else {
            customRgbValues[editCustomRgb - 1] -= 2;
          }
        }
        else {
          if (customRgbValues[editCustomRgb - 1] > 252){
            customRgbValues[editCustomRgb - 1] = 255;
          }
          else {
            customRgbValues[editCustomRgb - 1] += 2;
          }
        }
      }
      else {
        if (newPos > encoder_pos){
          currentProfile.defaultBrightness -= 2;
          currentProfile.brightness -= 2;
        }
        else{
          currentProfile.defaultBrightness += 2;
          currentProfile.brightness += 2;
        }
        if (currentProfile.defaultBrightness < 5){
          currentProfile.defaultBrightness = 4;
          currentProfile.brightness = 4;
        }
        if (currentProfile.defaultBrightness > 128){
          currentProfile.defaultBrightness = 128;
          currentProfile.brightness = 128;
        }
        currentProfile.defaultBrightness = currentProfile.brightness;
        if (currentProfile.rgbProfile == 4){
          rgbWhite();
        }
      }
    }
    else {
      // Changes volume
      if (!functionDown){
        if (newPos < encoder_pos){
          Keyboard.consumerPress(0x00E9);
          Keyboard.consumerRelease();
        }
        else{
          Keyboard.consumerPress(0x00EA);
          Keyboard.consumerRelease();
        }
      }
      // If function down uses binds
      else{
        if (newPos < encoder_pos){
          Keyboard.press(KEY_F22);
          Keyboard.release(KEY_F22);
        }
        else{
          Keyboard.press(KEY_F21);
          Keyboard.release(KEY_F21);
        }
      }
    }
    encoder_pos = newPos;
  }
}

void updateInternalData(){
  if ((micros() -StartTime2)  > longestTime){
    longestTime = micros() - StartTime2 ;
  }

  StartTime2 = micros();
  loops ++;
  if (5000 < (millis() - StartTime)){
    hz = loops / 5; // avarage updates /s
    /*
    internalT = analogReadTemp();
    Serial.print("Internal temperature: ");
    Serial.print(internalT);
    Serial.println(" C");
    */
    Serial.print("Avarage updates: ");
    Serial.print(hz);
    Serial.println(" / s");
    Serial.print("Slowest update : ");
    Serial.print(longestTime / 1000.0);
    Serial.println(" ms");
    longestTime = 0; // longest update time 
    loops = 0;
    StartTime = millis();
  }
}

void precenseDetection(){
  if (digitalRead(SENSOR) == 0){
    if (!sleepMode){
      if (!sleepModeInit){
        sleepModeTimer = millis();
        sleepModeInit = true;
      }
      else if ((millis() - sleepModeTimer) > timeToSleep * 1000){
        sleepMode = true;
        bSleepModeAnimation = true;
      }
    }
  }
  else if (sleepModeInit) {
    sleepModeInit = false;
    if (sleepMode){
      sleepMode = false;
      bSleepModeAnimation = true;
    }
  }
}

void saveData(){
  addr = XIP_BASE + FLASH_TARGET_OFFSET;
  p = (int *)addr;
  if(*p != -1){ 
    //Serial.println("Deleting old data");
    uint32_t ints = save_and_disable_interrupts();
    flash_range_erase(FLASH_TARGET_OFFSET, FLASH_SECTOR_SIZE);
    restore_interrupts (ints);
  }
  encodeData();
  *buf = combinedData;
  uint32_t ints = save_and_disable_interrupts();
  flash_range_program(FLASH_TARGET_OFFSET + (0 * FLASH_PAGE_SIZE), (uint8_t *)buf, FLASH_PAGE_SIZE);
  *buf = combinedDataCustomRgb;
  flash_range_program(FLASH_TARGET_OFFSET + (1 * FLASH_PAGE_SIZE), (uint8_t *)buf, FLASH_PAGE_SIZE);
  restore_interrupts (ints);
}

void encodeData(){
  profiles[currentProfile.num] = currentProfile;
  uint8_t p1 = (static_cast<uint8_t>(profiles[0].animated) << 7) | profiles[0].rgbProfile;
  uint8_t p2 = (static_cast<uint8_t>(profiles[1].animated) << 7) | profiles[1].rgbProfile;
  combinedData = (static_cast<uint32_t>(p1) << 24) |
                (static_cast<uint32_t>(static_cast<uint8_t>(profiles[0].defaultBrightness)) << 16) |
                (static_cast<uint32_t>(p2) << 8) |
                static_cast<uint8_t>(profiles[1].defaultBrightness);
  combinedDataCustomRgb = (static_cast<uint32_t>(customRgbValues[0]) << 24) |
                          (static_cast<uint32_t>(customRgbValues[1]) << 16) |
                          (static_cast<uint32_t>(customRgbValues[2]) << 8);
}

void decodeData(){
  addr = XIP_BASE + FLASH_TARGET_OFFSET;
  p = (int *)addr;
  if(*p != -1){ 
    combinedData = *p;
    profiles[0].animated = (combinedData >> 31) & 1;
    profiles[1].animated = (combinedData >> 15) & 1;
    profiles[0].rgbProfile = (combinedData >> 24) & 0x3F;
    profiles[0].defaultBrightness = (combinedData >> 16) & 0xFF;
    profiles[1].rgbProfile = (combinedData >> 8) & 0x3F;
    profiles[1].defaultBrightness = (combinedData >> 0) & 0xFF;
    profiles[0].brightness = profiles[0].defaultBrightness;
    profiles[1].brightness = profiles[1].defaultBrightness;
  }
  addr = XIP_BASE + FLASH_TARGET_OFFSET + (1 * FLASH_PAGE_SIZE);
  p = (int *)addr;
  if(*p != -1){ 
    combinedDataCustomRgb = *p;
    customRgbValues[0] = (combinedDataCustomRgb >> 24) & 0xFF;
    customRgbValues[1] = (combinedDataCustomRgb >> 16) & 0xFF;
    customRgbValues[2] = (combinedDataCustomRgb >> 8) & 0xFF;
  }
}

// Rgb funcions
void strips_loop() {
  if(rgbLoop()){
    if(rgbSettings && (millis() - StartTime ) % 1000 < 500){
      if (currentProfile.num == 0){
        for (int j = 39; j > 30; j--){
          rgbStrip.strip.setPixelColor(j, currentProfile.brightness, 0, currentProfile.brightness);
        }
      } 
      else {
        for (int j = 39; j > 30; j--){
          rgbStrip.strip.setPixelColor(j, 0, currentProfile.brightness, currentProfile.brightness);
        }
      }
    }
    rgbStrip.strip.show();
  }
}

void updateStaticRgb(){
  if(loops % 1000 == 1){ // Updates status indicators for static rgb profiles (every 1000th loop ~ 100ms)
    if (rgbSettings){ // rgbSettings status indicators
      if ((millis() - StartTime) % 1000 < 500){
        if (editCustomRgb == 1){
          for (int j = 39; j > 30; j--){
            rgbStrip.strip.setPixelColor(j, currentProfile.brightness * 1.1, 0, 0);
          }
        }
        else if (editCustomRgb == 2){
          for (int j = 39; j > 30; j--){
            rgbStrip.strip.setPixelColor(j, 0, currentProfile.brightness * 1.1, 0);
          }
        }
        else if (editCustomRgb == 3){
          for (int j = 39; j > 30; j--){
            rgbStrip.strip.setPixelColor(j, 0, 0, currentProfile.brightness * 1.1);
          }
        }
        else {
          if (currentProfile.num == 0){
            for (int j = 39; j > 30; j--){
              rgbStrip.strip.setPixelColor(j, currentProfile.brightness, 0, currentProfile.brightness);
            }
          } 
          else {
            for (int j = 39; j > 30; j--){
              rgbStrip.strip.setPixelColor(j, 0, currentProfile.brightness, currentProfile.brightness);
            }
          }
        }
        rgbStrip.strip.show();
      }
      else if(currentProfile.rgbProfile == 5){
        for (int j = 39; j > 30; j--){
          rgbStrip.strip.setPixelColor(j, currentProfile.brightness/2, currentProfile.brightness /3, currentProfile.brightness/2);
        }
        rgbStrip.strip.show();
      }
      else{
        refreshStaticRgb();
      }  
    }
    if (functionDown){ // functionDown Status indicators
      if (!rgbSettings) {
        refreshStaticRgb();
      }
      if (currentProfile.rgbProfile == 5){
        rgbStrip.strip.setPixelColor(0, currentProfile.brightness, currentProfile.brightness, currentProfile.brightness);
      }
    }
    if ((functionReset && !functionDown || bUpdateStaticRgb)){
      refreshStaticRgb();
      functionReset = 0;
      bUpdateStaticRgb = false;
    }
  }
}

void rgbProfileLoader(uint8_t i){
  if (i == 10){
    if (functionDown){ // Exit without saving     
      rgbSettings = false;
    }
    else {
      currentProfile.rgbProfile = 0;
      currentProfile.animated = true;
      editCustomRgb = 0;
    }
  }
  if (i == 12){
    currentProfile.rgbProfile = 1;
    currentProfile.animated = true;
    editCustomRgb = 0;
  }
  if (i == 14){ 
    if (functionDown){// Saven and exit
      rgbCustom();       
      rgbSettings = false;
      saveData();
    }
    else {
      currentProfile.rgbProfile = 2;
      currentProfile.animated = true;
      editCustomRgb = 0;
    }
  }
  if (i == 11){
    if (functionDown){
      currentProfile.rgbProfile = 6;
      editCustomRgb = 1;
      currentProfile.animated = false;    
      rgbCustom();       
    }
    else {
      currentProfile.rgbProfile = 6;
      currentProfile.animated = false;
      rgbCustom();
      editCustomRgb = 0;
    }
  }
  if (i == 13){
    if (functionDown){
      currentProfile.rgbProfile = 6;
      editCustomRgb = 2;
      currentProfile.animated = false;
      rgbCustom();  
    }
    else {
      currentProfile.rgbProfile = 4;
      currentProfile.animated = false;
      rgbWhite();
      editCustomRgb = 0;
    }
  }
  if (i == 15){
    if (functionDown){
      currentProfile.rgbProfile = 6;
      editCustomRgb = 3;
      currentProfile.animated = false;
      rgbCustom();
    }
    else {
      currentProfile.rgbProfile = 5;
      currentProfile.animated = false;
      rgbOff();
      editCustomRgb = 0;
    }
  }
}

void refreshStaticRgb(){
  if (currentProfile.rgbProfile == 4){
    rgbWhite();
  }
  else if (currentProfile.rgbProfile == 3){
    rgbPurple();
  }
  else if (currentProfile.rgbProfile == 5){
    rgbOff();
  }
  else if (currentProfile.rgbProfile == 6){
    rgbCustom();
  }
}

// Animated rgb profiles
uint8_t rgbLoop() {
  uint8_t ret = 0x00;
  if(currentProfile.rgbProfile == 0) {
    strip0loop0.currentChild = 0;
  }
  if(currentProfile.rgbProfile == 1){
    strip0loop0.currentChild = 1;
  }
  if(currentProfile.rgbProfile == 2){
    strip0loop0.currentChild = 2;
  }
  switch(strip0loop0.currentChild) {
    case 0: 
      ret = rgbRainbow();break;
    case 1: 
      ret = rgbPride();break;
    case 2: 
      ret = rgbPinkAndPurple();break;
  }
  return ret;
}

uint8_t rgbRainbow() {
  if(millis() - rgbStrip.effStart < 80 * (rgbStrip.effStep)) return 0x00;
  float factor1, factor2;
  uint16_t ind;
  for(uint16_t j=0;j<40;j++) {
    ind = rgbStrip.effStep + j * 1.5;
    int brightnessf = currentProfile.brightness;
    if (j == 0){
      brightnessf = currentProfile.brightness*2;
    }
    switch((int)((ind % 60) / 20)) {
      case 0: factor1 = 1.0 - ((float)(ind % 60 - 0 * 20) / 20);
              factor2 = (float)((int)(ind - 0) % 60) / 20;
              rgbStrip.strip.setPixelColor(j, brightnessf*2.0 * factor1 + 0 * factor2, 0 * factor1 + brightnessf * factor2, 0 * factor1 + 0 * factor2);
              break;
      case 1: factor1 = 1.0 - ((float)(ind % 60 - 1 * 20) / 20);
              factor2 = (float)((int)(ind - 20) % 60) / 20;
              rgbStrip.strip.setPixelColor(j, 0 * factor1 + 0 * factor2, brightnessf * factor1 + 0 * factor2, 0 * factor1 + brightnessf/1.5 * factor2);
              break;
      case 2: factor1 = 1.0 - ((float)(ind % 60 - 2 * 20) / 20);
              factor2 = (float)((int)(ind - 40) % 60) / 20;
              rgbStrip.strip.setPixelColor(j, 0 * factor1 + brightnessf*2.0 * factor2, 0 * factor1 + 0 * factor2, brightnessf * factor1 + 0 * factor2);
              break;
    }
  }
  if(rgbStrip.effStep >= 60) {rgbStrip.Reset(); return 0x03; }
  else rgbStrip.effStep++;
  return 0x01;
}

uint8_t rgbPride() {
  if(millis() - rgbStrip.effStart < 80 * (rgbStrip.effStep)) return 0x00;
  float factor1, factor2;
  uint16_t ind;
  float brightnessf = currentProfile.brightness;
  for(uint16_t j=0;j<40;j++) {
    ind = rgbStrip.effStep + j * 1.5;
    switch((int)((ind % 60) / 10)) {
      case 0: factor1 = 1.0 - ((float)(ind % 60 - 0 * 10) / 10);
              factor2 = (float)((int)(ind - 0) % 60) / 10;
              rgbStrip.strip.setPixelColor(j, 0 * factor1 + brightnessf * 1.9 * factor2, brightnessf * 1.1 * factor1 + brightnessf * 0.1 * factor2, brightnessf * 1.6 * factor1 + brightnessf * 1.9 * factor2);
              break;
      case 1: factor1 = 1.0 - ((float)(ind % 60 - 1 * 10) / 10);
              factor2 = (float)((int)(ind - 10) % 60) / 10;
              rgbStrip.strip.setPixelColor(j, brightnessf * 1.9 * factor1 + brightnessf * 1.7 * factor2, brightnessf * 0.1 * factor1 + brightnessf * 1.7 * factor2, brightnessf * 1.9 * factor1 + brightnessf * 1.7 * factor2);
              break;
      case 2: factor1 = 1.0 - ((float)(ind % 60 - 2 * 10) / 10);
              factor2 = (float)((int)(ind - 20) % 60) / 10;
              rgbStrip.strip.setPixelColor(j, brightnessf * 1.7 * factor1 + brightnessf * 1.9 * factor2, brightnessf * 1.7 * factor1 + brightnessf * 0.1 * factor2, brightnessf * 1.7 * factor1 + brightnessf * 1.9 * factor2);
              break;
      case 3: factor1 = 1.0 - ((float)(ind % 60 - 3 * 10) / 10);
              factor2 = (float)((int)(ind - 30) % 60) / 10;
              rgbStrip.strip.setPixelColor(j, brightnessf * 1.9 * factor1 + 0 * factor2, 10 * factor1 + brightnessf * 1.1 * factor2, brightnessf * 1.9 * factor1 + brightnessf * 1.6 * factor2);
              break;
      case 4: factor1 = 1.0 - ((float)(ind % 60 - 4 * 10) / 10);
              factor2 = (float)((int)(ind - 40) % 60) / 10;
              rgbStrip.strip.setPixelColor(j, 0 * factor1 + brightnessf * 1.7 * factor2, brightnessf * 1.1 * factor1 + brightnessf * 1.7 * factor2, brightnessf * 1.6 * factor1 + brightnessf * 1.7 * factor2);
              break;
      case 5: factor1 = 1.0 - ((float)(ind % 60 - 5 * 10) / 10);
              factor2 = (float)((int)(ind - 50) % 60) / 10;
              rgbStrip.strip.setPixelColor(j, brightnessf * 1.7 * factor1 + 0 * factor2, brightnessf * 1.7 * factor1 + brightnessf * 1.1 * factor2, brightnessf * 1.7 * factor1 + brightnessf * 1.6 * factor2);
              break;
    }
  }
  if(rgbStrip.effStep >= 60) {rgbStrip.Reset(); return 0x03; }
  else rgbStrip.effStep++;
  return 0x01;
}

uint8_t rgbPinkAndPurple() {
  if(millis() - rgbStrip.effStart < 80 * (rgbStrip.effStep)) return 0x00;
  float factor1, factor2;
  uint16_t ind;
  float brightnessf = currentProfile.brightness;
  for(uint16_t j=0;j<40;j++) {
    ind = rgbStrip.effStep + j * 1.5;
    switch((int)((ind % 60) / 30)) {
      case 0: factor1 = 1.0 - ((float)(ind % 60 - 0 * 30) / 30);
              factor2 = (float)((int)(ind - 0) % 60) / 30;
              rgbStrip.strip.setPixelColor(j, brightnessf * 0.2 * factor1 + brightnessf * 2.38  * factor2, 0 * factor1 + 0 * factor2, brightnessf * 1.92  * factor1 + brightnessf * 1.25  * factor2);
              break;
      case 1: factor1 = 1.0 - ((float)(ind % 60 - 1 * 30) / 30);
              factor2 = (float)((int)(ind - 30) % 60) / 30;
              rgbStrip.strip.setPixelColor(j, brightnessf * 2.38 * factor1 + brightnessf * 0.2 * factor2, 0 * factor1 + 0 * factor2, brightnessf * 1.25 * factor1 + brightnessf * 1.92 * factor2);
              break;
    }
  }
  if(rgbStrip.effStep >= 60) {rgbStrip.Reset(); return 0x03; }
  else rgbStrip.effStep++;
  return 0x01;
}

void rgbWhite(){
  for(uint16_t j=0;j<40;j++) {
    rgbStrip.strip.setPixelColor(j, currentProfile.brightness, currentProfile.brightness, currentProfile.brightness / 0.85);
  }
  rgbStrip.strip.show();
}

void rgbPurple(){
  for(uint16_t j=0;j<40;j++) {
    rgbStrip.strip.setPixelColor(j, currentProfile.brightness * 0.2, 0, currentProfile.brightness);
  }
  rgbStrip.strip.show();
}

void rgbCustom(){
  for(uint16_t j=0;j<40;j++) {
    rgbStrip.strip.setPixelColor(j, (currentProfile.brightness / 256.0) * customRgbValues[0], 
    (currentProfile.brightness / 256.0) * customRgbValues[1], (currentProfile.brightness / 256.0) * customRgbValues[2]);
  }
  rgbStrip.strip.show();
}

void rgbOff(){
  for(uint16_t j=0;j<40;j++) {
    rgbStrip.strip.setPixelColor(j, 0, 0, 0);
  }
  rgbStrip.strip.show();
}