#ifndef PLAYER_DISPLAY
#define PLAYER_DISPLAY
#include "Arduino.h"
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

class PlayerDisplay{
  public:
    void showName(String name);
    void showVolume(float v);
    void showProgress(float);
    void testConnection();
    void showLoadingDisplay();
    void showPlaybackStatus(bool);
};

#endif
