// **********************************************************************************
// Author: Vilja Seppänen https://github.com/vilja22
// **********************************************************************************
// License
// **********************************************************************************
// This program is free software; you can redistribute it 
// and/or modify it under the terms of the GNU General    
// Public License as published by the Free Software       
// Foundation; either version 3 of the License, or        
// (at your option) any later version.                    
//                                                        
// This program is distributed in the hope that it will   
// be useful, but WITHOUT ANY WARRANTY; without even the  
// implied warranty of MERCHANTABILITY or FITNESS FOR A   
// PARTICULAR PURPOSE. See the GNU General Public        
// License for more details.                              
//                                                        
// Licence can be viewed at                               
// http://www.gnu.org/licenses/gpl-3.0.txt
//
// Please maintain this license information along with authorship
// and copyright notices in any redistribution of this code
// **********************************************************************************


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
#include "hardware/watchdog.h"
#define FLASH_TARGET_OFFSET (PICO_FLASH_SIZE_BYTES - FLASH_SECTOR_SIZE)



// Binds
// You shouldnt use first keys of either row since they are used to control the keyboard
uint8_t bindsProfile1[] = {
  0, KEY_F1,  KEY_F2,  KEY_F3,  KEY_F4,  KEY_F5, KEY_F6,  KEY_F7,  KEY_F8,  KEY_INSERT, KEY_PAGE_UP,   KEY_HOME,       KEY_UP_ARROW,   KEY_END, // Top row
  1, KEY_F13, KEY_F14, KEY_F15, KEY_F16, KEY_F9, KEY_F10, KEY_F11, KEY_F12, KEY_DELETE, KEY_PAGE_DOWN, KEY_LEFT_ARROW, KEY_DOWN_ARROW, KEY_RIGHT_ARROW // Bottom row
};

uint8_t bindsProfile2[] = {
  0, KEY_F13, KEY_F14, KEY_F15, KEY_F16, KEY_F9, KEY_F10, KEY_F11, KEY_F12, KEY_F16, KEY_F19,  KEY_HOME,   KEY_INSERT, KEY_PAGE_UP, // Top row
  1, KEY_F1,  KEY_F2,  KEY_F3,  KEY_F4,  KEY_F5, KEY_F6,  KEY_F7,  KEY_F8,  KEY_F17, KEY_F18,  KEY_DELETE, KEY_END,    KEY_PAGE_DOWN // Bottom row
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
  uint8_t rotBind, rotFnBind;
  uint8_t customRgbValues[3];
};

// Only used if no profiles stored on flash
Profile profiles[2]{
  {0, false, 4, 16, 16, 0 ,1, {200,200,200}},
  {1, true, 2, 16, 16, 0, 1, {200,200,200}}
};

Profile currentProfile = profiles[0];

// Saving data

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
unsigned long restartTimer = 0; //  Used to prevent accidental doubleclicks on rotatory button
unsigned long sleepModeTimer = 0;
int timeToSleep = 2; // Min to go to sleep mode
int timeToDim = 1; // Min
uint8_t sleepModeBrigtness = 0;

bool sleepMode = 0;
bool dimmed = 0;
bool sleepModeInit = 0;
bool bSleepModeAnimation = false;
//uint8_t resentPresenceCount = 0;

bool rgbSettings = 0;
bool bUpdateStaticRgb = true;
uint8_t editCustomRgb = 0;
bool bEditedKeys = false;


uint8_t internalT = 0;
float longestTime = 0;
int loops = 0;
uint16_t hz = 0;

bool functionDown = 0;
bool functionReset = 0;

uint32_t combinedData = 0;
uint32_t combinedDataCustomRgb = 0;


bool bGetPerformance = false;
uint8_t errorCode[8] = {1,1,2,3,5,8,12,21 };


uint8_t recivedData[73];
uint8_t encodedData[73];

void setup() {
  Keyboard.begin();
  
  currentProfile = profiles[0];

  // Cheks if there are saved profiles in flash and loads them
  decodeData();
  updateKeysUsb();
  updateProfileUsb();
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
  Serial.begin(115200);
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
  if (Serial.available() != 0){
    readUsbData();
  }

  
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
  if(loops % 100 == 1){
    if (sleepMode && currentProfile.brightness != sleepModeBrigtness){
      currentProfile.brightness -= currentProfile.defaultBrightness / (currentProfile.defaultBrightness / 0.1);
      refreshStaticRgb();
      if (currentProfile.brightness <= sleepModeBrigtness){
        bSleepModeAnimation = false;
        currentProfile.brightness = sleepModeBrigtness;
      }
    }
    if (!sleepMode && dimmed && currentProfile.brightness != currentProfile.defaultBrightness / 2){
      currentProfile.brightness -= currentProfile.defaultBrightness / (currentProfile.defaultBrightness / 0.1);
      refreshStaticRgb();
      if (currentProfile.brightness <= currentProfile.defaultBrightness / 2){
        bSleepModeAnimation = false;
        currentProfile.brightness = currentProfile.defaultBrightness / 2;
      }
    }
    if (!dimmed && !sleepMode && currentProfile.brightness != currentProfile.defaultBrightness && !functionDown){
      currentProfile.brightness += currentProfile.defaultBrightness / (currentProfile.defaultBrightness / 0.2);
      refreshStaticRgb();
      if (currentProfile.brightness >= currentProfile.defaultBrightness){
        currentProfile.brightness = currentProfile.defaultBrightness;
        bSleepModeAnimation = false;
      }
    }
  }
}

float getBrightnessF(int j) {
  float brightnessf;
  if (j < 20){
    brightnessf = currentProfile.brightness * (((j+1)*1.0) / 40.0) * (currentProfile.brightness / currentProfile.defaultBrightness) * 40.0;
  }
  else {
    brightnessf = currentProfile.brightness * (((40-j)*1.0) / 40.0) * (currentProfile.brightness / currentProfile.defaultBrightness) * 40.0;
  }
  if (brightnessf > currentProfile.defaultBrightness){
    brightnessf = currentProfile.defaultBrightness;
  }
  return brightnessf;
}

void updateRotatoryButton(){
  if (!digitalRead(inputs[11])){
    if ((millis() - deBounceTimer) > 50){
      if (functionDown){ // Toggles rgbSettings
        if (rgbSettings){
          // Saves profiles to flash
          editCustomRgb = 0;
          encodeCurrentSetup();
          saveData(encodedData);
        }
        rgbSettings = !rgbSettings;
      }
      else{ // Toggles keyboard profile
        //restartTimer = millis();
        //watchdog_enable(1, 1);
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
          if (currentProfile.customRgbValues[editCustomRgb - 1] < 3){
            currentProfile.customRgbValues[editCustomRgb - 1] = 0;
          }
          else {
            currentProfile.customRgbValues[editCustomRgb - 1] -= 2;
          }
        }
        else {
          if (currentProfile.customRgbValues[editCustomRgb - 1] > 252){
            currentProfile.customRgbValues[editCustomRgb - 1] = 255;
          }
          else {
            currentProfile.customRgbValues[editCustomRgb - 1] += 2;
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
      if (newPos < encoder_pos){
        rotatoryRotated(true);
      }
      else{
        rotatoryRotated(false);
      }
    }
    encoder_pos = newPos;
  }
}

void rotatoryRotated(bool bPositive){
  uint8_t rotatoryAction = currentProfile.rotBind;
  if (functionDown){
    rotatoryAction = currentProfile.rotFnBind;
  }
  if (rotatoryAction == 0){ // Volume
    if (bPositive){ 
      Keyboard.consumerPress(0x00E9);
      Keyboard.consumerRelease();
    }
    else{
      Keyboard.consumerPress(0x00EA);
      Keyboard.consumerRelease();
    }
  }
  else if (rotatoryAction == 1){ // Function keys
    if (bPositive){
      Keyboard.press(KEY_F22);
      Keyboard.release(KEY_F22);
    }
    else{
      Keyboard.press(KEY_F21);
      Keyboard.release(KEY_F21);
    }
  }
  else if (rotatoryAction == 2){
    if (bPositive){
      Keyboard.press(KEY_RIGHT_ARROW);
      Keyboard.release(KEY_RIGHT_ARROW);
    }
    else{
      Keyboard.press(KEY_LEFT_ARROW);
      Keyboard.release(KEY_LEFT_ARROW);
    }
  }
  else if (rotatoryAction == 3){
    Keyboard.press(KEY_LEFT_CTRL);
    if (bPositive){
      Keyboard.press('y');
      Keyboard.release('y');
    }
    else{
      Keyboard.press('z');
      Keyboard.release('z');
    }
    Keyboard.release(KEY_LEFT_CTRL);
  }
}


void updateInternalData(){
  if ((micros() -StartTime2)  > longestTime){
    longestTime = micros() - StartTime2 ;
  }

  StartTime2 = micros();
  loops ++;
  if (10000 < (millis() - StartTime)){
    hz = loops / 10; // avarage updates /s
    /*
    internalT = analogReadTemp();
    Serial.print("Internal temperature: ");
    Serial.print(internalT);
    Serial.println(" C");
     */
    if (bGetPerformance){
      uint16_t lt = longestTime;
      uint8_t highlt = (lt >> 8) & 0xFF; // Extract the high byte
      uint8_t lowlt = lt & 0xFF; 
      uint8_t highhz = (hz >> 8) & 0xFF; // Extract the high byte
      uint8_t lowhz = hz & 0xFF;     
      Serial.write(highlt);
      Serial.write(lowlt);
      Serial.write(highhz);
      Serial.write(lowhz);
    
      longestTime = 0; // longest update time 
      loops = 0;
      StartTime = millis();
    }

  }
}

void precenseDetection(){
  if (digitalRead(SENSOR) == 0){
    if (!sleepMode){
      if (!sleepModeInit ){
        sleepModeTimer = millis();
        sleepModeInit = true;
      }
      else if (((millis() - sleepModeTimer) > (timeToSleep) * 1000 * 60) && timeToSleep != 0){
        sleepMode = true;
        bSleepModeAnimation = true;
      }
      else if (((millis() - sleepModeTimer) > (timeToDim) * 1000 * 1) && timeToDim != 0){
        dimmed = true;
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
    if (dimmed){
      dimmed = false;
      bSleepModeAnimation = true;
    }
  }
}

void saveData(const uint8_t buf[73]){

  const uint32_t addr = XIP_BASE + FLASH_TARGET_OFFSET;
  const int* p = (int*)addr;

  if(*p != -1){ 
    //Serial.println("Deleting old data");
    uint32_t ints = save_and_disable_interrupts();
    flash_range_erase(FLASH_TARGET_OFFSET, FLASH_SECTOR_SIZE);
    restore_interrupts (ints);
  }
  
  uint32_t ints = save_and_disable_interrupts();
  flash_range_program(FLASH_TARGET_OFFSET, buf, 73);
  restore_interrupts (ints);
}


void decodeData(){
  uint8_t buf[73] = {0};
  const uint32_t addr = XIP_BASE + FLASH_TARGET_OFFSET;
  const int* p = (int*)addr;
  memcpy(buf, p, 73);
  if (buf[0] != -1){
    memcpy(recivedData, buf, 73);
  }
}

void readUsbData(){
  Keyboard.end();
  bEditedKeys = false;
  while (Serial.available() > 0) {
    int o = 0;
    int recived = Serial.read();
    if (recived == 1){
      while (o < 28) {
        if (Serial.available() > 0) {
          recivedData[o] = Serial.read();
          o++;  
        }
      }
      bEditedKeys = true;
      Serial.write('1');
    }
    else if (recived == 2){
      o = 28;
      while (o < 56) {
        if (Serial.available() > 0) {
          recivedData[o] = Serial.read();
          o++;  
        }
      }
      bEditedKeys = true;
      Serial.write('2');
    }
    else if (recived == 3){
      o = 56;
      while (o < 73) {
        if (Serial.available() > 0) {
          recivedData[o] = Serial.read();
          o++;  
        }
      }
      updateProfileUsb();
      Serial.print('3');
    }
    else if (recived == 10){ // Sends current settings to pc
      Serial.write(10);
      encodeCurrentSetup();
      for (int i = 0; i < 73; i++){
        delay(2);
        Serial.write(encodedData[i]);
      }
      Serial.write(10);
    }
    else if (recived == 99){ // Save data to flash
      Serial.write(99);
      encodeCurrentSetup();
      saveData(encodedData);
    }
    else if (recived == 110){ // Save data to flash
      Serial.write(110);
      delay(50);
      watchdog_enable(1, 1);    
    }
    else if (recived == 120){
      Serial.write(120);
      bGetPerformance = false;
    }
    else if (recived == 121){
      Serial.write(121);
      bGetPerformance = true;
    }
  }
  if (bEditedKeys){
    updateKeysUsb();
  }
  Keyboard.begin();
}

void updateProfileUsb(){ // Updates rgb and other settings that were changed by usb

  profiles[0].rotBind = recivedData[56];  
  profiles[0].rotFnBind = recivedData[57];
  profiles[1].rotBind = recivedData[58];   
  profiles[1].rotFnBind = recivedData[59];

  profiles[0].rgbProfile = recivedData[60];
  profiles[1].rgbProfile = recivedData[61];
  profiles[0].customRgbValues[0] = recivedData[62];
  profiles[0].customRgbValues[1] = recivedData[63];
  profiles[0].customRgbValues[2] = recivedData[64];
  profiles[1].customRgbValues[0] = recivedData[65];
  profiles[1].customRgbValues[1] = recivedData[66];
  profiles[1].customRgbValues[2] = recivedData[67];
  profiles[0].defaultBrightness = recivedData[68];
  profiles[1].defaultBrightness = recivedData[69];
  timeToSleep = recivedData[70];
  timeToDim = recivedData[71];
  currentProfile = profiles[recivedData[72]];
  if (currentProfile.rgbProfile == 3){
    currentProfile.animated = false;
  }
  else{
    currentProfile.animated = true;
  }
  currentProfile.brightness = currentProfile.defaultBrightness;
  refreshStaticRgb();
}

void updateKeysUsb(){ // Updates keys that were changed by usb
  for (int i = 0; i < 56; i++){
    // Improve this very hard to read with all the random offsets
    if (i < 28) {
      // Populate bindsProfile1 array
      bindsProfile1[i] = decodeUsbKey(recivedData[i]);
    } 
    else if (i < 56) {
      // Populate bindsProfile2 array
      bindsProfile2[i - 28] = decodeUsbKey(recivedData[i]);
    } 
  }


  outputs[0][0] = bindsProfile1[0];
  outputs[0][1] = bindsProfile1[1];
  outputs[0][2] = bindsProfile1[15];
  outputs[0][3] = bindsProfile1[16];
  outputs[0][4] = bindsProfile1[3];
  outputs[0][5] = bindsProfile1[17];
  outputs[0][6] = bindsProfile1[18];
  outputs[0][7] = bindsProfile1[5];
  outputs[0][8] = bindsProfile1[19];
  outputs[0][9] = bindsProfile1[2];
  outputs[0][10] = bindsProfile1[4];
  outputs[0][11] = bindsProfile1[14];

  outputs[1][0] = bindsProfile2[0];
  outputs[1][1] = bindsProfile2[1];
  outputs[1][2] = bindsProfile2[15];
  outputs[1][3] = bindsProfile2[16];
  outputs[1][4] = bindsProfile2[3];
  outputs[1][5] = bindsProfile2[17];
  outputs[1][6] = bindsProfile2[18];
  outputs[1][7] = bindsProfile2[5];
  outputs[1][8] = bindsProfile2[19];
  outputs[1][9] = bindsProfile2[2];
  outputs[1][10] = bindsProfile2[4];
  outputs[1][11] = bindsProfile2[14];

  muxoutputs[0][0] = bindsProfile1[6];
  muxoutputs[0][1] = bindsProfile1[10];
  muxoutputs[0][2] = bindsProfile1[7];
  muxoutputs[0][3] = bindsProfile1[20];
  muxoutputs[0][4] = bindsProfile1[21];
  muxoutputs[0][5] = bindsProfile1[8];
  muxoutputs[0][6] = bindsProfile1[22];
  muxoutputs[0][7] = bindsProfile1[9];
  muxoutputs[0][8] = bindsProfile1[23];
  muxoutputs[0][9] = bindsProfile1[24];
  muxoutputs[0][10] = bindsProfile1[11];
  muxoutputs[0][11] = bindsProfile1[25];
  muxoutputs[0][12] = bindsProfile1[12];
  muxoutputs[0][13] = bindsProfile1[26];
  muxoutputs[0][14] = bindsProfile1[13];
  muxoutputs[0][15] = bindsProfile1[27];

  muxoutputs[1][0] = bindsProfile2[6];
  muxoutputs[1][1] = bindsProfile2[10];
  muxoutputs[1][2] = bindsProfile2[7];
  muxoutputs[1][3] = bindsProfile2[20];
  muxoutputs[1][4] = bindsProfile2[21];
  muxoutputs[1][5] = bindsProfile2[8];
  muxoutputs[1][6] = bindsProfile2[22];
  muxoutputs[1][7] = bindsProfile2[9];
  muxoutputs[1][8] = bindsProfile2[23];
  muxoutputs[1][9] = bindsProfile2[24];
  muxoutputs[1][10] = bindsProfile2[11];
  muxoutputs[1][11] = bindsProfile2[25];
  muxoutputs[1][12] = bindsProfile2[12];
  muxoutputs[1][13] = bindsProfile2[26];
  muxoutputs[1][14] = bindsProfile2[13];
  muxoutputs[1][15] = bindsProfile2[27];
}

void encodeCurrentSetup(){ // Encode data for transmission
  for (int i = 0; i < 56; i++){
    if (i < 28) {
      encodedData[i] = encodeUsbKey(bindsProfile1[i]);
    }
    else{
      encodedData[i] = encodeUsbKey(bindsProfile2[i-28]);
    }
  }
  encodedData[56] = profiles[0].rotBind;   
  encodedData[57] = profiles[0].rotFnBind; 
  encodedData[58] = profiles[1].rotBind;   
  encodedData[59] = profiles[1].rotFnBind; 
  encodedData[60] = profiles[0].rgbProfile;
  encodedData[61] = profiles[1].rgbProfile;
  encodedData[62] = profiles[0].customRgbValues[0];
  encodedData[63] = profiles[0].customRgbValues[1];
  encodedData[64] = profiles[0].customRgbValues[2];
  encodedData[65] = profiles[1].customRgbValues[0];
  encodedData[66] = profiles[1].customRgbValues[1];
  encodedData[67] = profiles[1].customRgbValues[2];
  encodedData[68] = profiles[0].defaultBrightness;
  encodedData[69] = profiles[1].defaultBrightness;
  encodedData[70] = timeToSleep;
  encodedData[71] = timeToDim;
  encodedData[72] = currentProfile.num;
  delay(5);

}


uint8_t decodeUsbKey(uint8_t key){
  if (key == 0){return KEY_F1;}
  if (key == 1){return KEY_F2;}
  if (key == 2){return KEY_F3;}
  if (key == 3){return KEY_F4;}
  if (key == 4){return KEY_F5;}
  if (key == 5){return KEY_F6;}
  if (key == 6){return KEY_F7;}
  if (key == 7){return KEY_F8;}
  if (key == 8){return KEY_F9;}
  if (key == 9){return KEY_F10;}
  if (key == 10){return KEY_F11;}
  if (key == 11){return KEY_F12;}
  if (key == 12){return KEY_F13;}
  if (key == 13){return KEY_F14;}
  if (key == 14){return KEY_F15;}
  if (key == 15){return KEY_F16;}
  if (key == 16){return KEY_F17;}
  if (key == 17){return KEY_F18;}
  if (key == 18){return KEY_F19;}
  if (key == 19){return KEY_F20;}
  if (key == 20){return KEY_F21;}
  if (key == 21){return KEY_F22;}
  if (key == 22){return KEY_F23;}
  if (key == 23){return KEY_F24;}
  if (key == 24){return 0;}
  if (key == 25){return KEY_DELETE;}
  if (key == 26){return KEY_INSERT;}
  if (key == 27){return KEY_PAGE_UP;}
  if (key == 28){return KEY_PAGE_DOWN;}
  if (key == 29){return KEY_END;}
  if (key == 30){return KEY_HOME;}
  if (key == 31){return KEY_UP_ARROW;}
  if (key == 32){return KEY_DOWN_ARROW;}
  if (key == 33){return KEY_LEFT_ARROW;}
  if (key == 34){return KEY_RIGHT_ARROW;}
  if (key == 35){return 1;}
  return key;
}

uint8_t encodeUsbKey(uint8_t key){
  if (key == KEY_F1){return 0;}
  if (key == KEY_F2){return 1;}
  if (key == KEY_F3){return 2;}
  if (key == KEY_F4){return 3;}
  if (key == KEY_F5){return 4;}
  if (key == KEY_F6){return 5;}
  if (key == KEY_F7){return 6;}
  if (key == KEY_F8){return 7;}
  if (key == KEY_F9){return 8;}
  if (key == KEY_F10){return 9;}
  if (key == KEY_F11){return 10;}
  if (key == KEY_F12){return 11;}
  if (key == KEY_F13){return 12;}
  if (key == KEY_F14){return 13;}
  if (key == KEY_F15){return 14;}
  if (key == KEY_F16){return 15;}
  if (key == KEY_F17){return 16;}
  if (key == KEY_F18){return 17;}
  if (key == KEY_F19){return 18;}
  if (key == KEY_F20){return 19;}
  if (key == KEY_F21){return 20;}
  if (key == KEY_F22){return 21;}
  if (key == KEY_F23){return 22;}
  if (key == KEY_F24){return 23;}
  if (key == 0){return 24;}
  if (key == KEY_DELETE){return 25;}
  if (key == KEY_INSERT){return 26;}
  if (key == KEY_PAGE_UP){return 27;}
  if (key == KEY_PAGE_DOWN){return 28;}
  if (key == KEY_END){return 29;}
  if (key == KEY_HOME){return 30;}
  if (key == KEY_UP_ARROW){return 31;}
  if (key == KEY_DOWN_ARROW){return 32;}
  if (key == KEY_LEFT_ARROW){return 33;}
  if (key == KEY_RIGHT_ARROW){return 34;}
  if (key == 1){return 35;}
  return key;
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
      encodeCurrentSetup();
      saveData(encodedData);
    }
    else {
      currentProfile.rgbProfile = 2;
      currentProfile.animated = true;
      editCustomRgb = 0;
    }
  }
  if (i == 11){
    if (functionDown){
      currentProfile.rgbProfile = 3;
      editCustomRgb = 1;
      currentProfile.animated = false;    
      rgbCustom();       
    }
    else {
      currentProfile.rgbProfile = 3;
      currentProfile.animated = false;
      rgbCustom();
      editCustomRgb = 0;
    }
  }
  if (i == 13){
    if (functionDown){
      currentProfile.rgbProfile = 3;
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
      currentProfile.rgbProfile = 3;
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
  else if (currentProfile.rgbProfile == 6){
    rgbPurple();
  }
  else if (currentProfile.rgbProfile == 5){
    rgbOff();
  }
  else if (currentProfile.rgbProfile == 3){
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
  float brightnessf = currentProfile.brightness;
  for(uint16_t j=0;j<40;j++) {
    ind = rgbStrip.effStep + j * 1.5;
    if (bSleepModeAnimation){
      brightnessf = getBrightnessF(j);
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
    if (bSleepModeAnimation){
      brightnessf = getBrightnessF(j);
    }
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
    if (bSleepModeAnimation){
      brightnessf = getBrightnessF(j);
    }
    ind = rgbStrip.effStep + j * 1.5;
    switch((int)((ind % 60) / 30)) {
      case 0: factor1 = 1.0 - ((float)(ind % 60 - 0 * 30) / 30);
              factor2 = (float)((int)(ind - 0) % 60) / 30;
              rgbStrip.strip.setPixelColor(j, brightnessf * 0.12 * factor1 + brightnessf * 2.38  * factor2, 0 * factor1 + 0 * factor2, brightnessf * 1.65  * factor1 + brightnessf * 1.1  * factor2);
              break;
      case 1: factor1 = 1.0 - ((float)(ind % 60 - 1 * 30) / 30);
              factor2 = (float)((int)(ind - 30) % 60) / 30;
              rgbStrip.strip.setPixelColor(j, brightnessf * 2.38 * factor1 + brightnessf * 0.12 * factor2, 0 * factor1 + 0 * factor2, brightnessf * 1.1 * factor1 + brightnessf * 1.65 * factor2);
              break;
    }
  }
  if(rgbStrip.effStep >= 60) {rgbStrip.Reset(); return 0x03; }
  else rgbStrip.effStep++;
  return 0x01;
}

void rgbWhite(){
  float brightnessf = currentProfile.brightness;
  for(uint16_t j=0;j<40;j++) {
    if (bSleepModeAnimation){
      brightnessf = getBrightnessF(j);
    }
    rgbStrip.strip.setPixelColor(j, brightnessf, brightnessf, brightnessf / 0.85);
  }
  rgbStrip.strip.show();
}

void rgbPurple(){
  float brightnessf = currentProfile.brightness;
  for(uint16_t j=0;j<40;j++) {
    if (bSleepModeAnimation){
      brightnessf = getBrightnessF(j);
    }
    rgbStrip.strip.setPixelColor(j, brightnessf * 0.2, 0, brightnessf);
  }
  rgbStrip.strip.show();
}

void rgbCustom(){
  float brightnessf = currentProfile.brightness;
  for(uint16_t j=0;j<40;j++) {
    if (bSleepModeAnimation){
      brightnessf = getBrightnessF(j);
    }
    rgbStrip.strip.setPixelColor(j, (brightnessf / 256.0) * currentProfile.customRgbValues[0], 
    (brightnessf / 256.0) * currentProfile.customRgbValues[1], (brightnessf / 256.0) * currentProfile.customRgbValues[2]);
  }
  rgbStrip.strip.show();
}

void rgbOff(){
  for(uint16_t j=0;j<40;j++) {
    rgbStrip.strip.setPixelColor(j, 0, 0, 0);
  }
  rgbStrip.strip.show();
}
