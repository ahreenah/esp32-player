// #ifndef GLOBALS_H     // equivalently, #if !defined HEADER_H_
// #define GLOBALS_H

#include <Arduino.h>
#include <AudioFileSourceSD.h>
#include <AudioFileSourceID3.h>
#include <AudioOutputSPDIF.h>
#include <AudioGeneratorMP3.h>
#include <AudioOutputI2SNoDAC.h>
#include <AudioOutputI2S.h>
#include <SD.h>
// #include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include <PlayOrder.h>
#include <ButtonsState.h>
#include <PlayerDisplay.h>

String currentFile;



File dir;
AudioFileSourceID3 *id3;
AudioFileSourceSD *source = NULL;
AudioOutputSPDIF *output = NULL;
AudioGeneratorMP3 *decoder = NULL;

volatile static xSemaphoreHandle DisplayHandle;

AudioOutputI2S *out;
float gain = 1; //step = 1/600;
const float gainStep = (float)1/(float)60;

TaskHandle_t PlayTask;
String title;

PlayerDisplay dis;
ButtonsState buttons;
PlayOrder po;
bool ch;
bool paused;

// #endif