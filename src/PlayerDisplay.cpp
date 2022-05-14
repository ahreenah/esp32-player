#include <PlayerDisplay.h>
Adafruit_SSD1306 display_a(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

String utf8rusd(String source)
{
  int i,k;
  String target;
  unsigned char n;
  char m[2] = { '0', '\0' };
  k = source.length(); i = 0;
  while (i < k) {
    n = source[i]; i++;
    if (n >= 0xC0) {
      switch (n) {
        case 0xD0: {
          n = source[i]; i++;
          if (n == 0x81) { n = 0xA8; break; }
          if (n >= 0x90 && n <= 0xBF) n = n + 0x30;
          break;
        }
        case 0xD1: {
          n = source[i]; i++;
          if (n == 0x91) { n = 0xB8; break; }
          if (n >= 0x80 && n <= 0x8F) n = n + 0x70;
          break;
        }
      }
    }
    m[0] = n; target = target + String(m);
  }
return target;
}


void PlayerDisplay::showName(String name){
  display_a.cp437(true);
  display_a.clearDisplay();
//   display.fillRect(0,0,255*gain/4,5,WHITE);
  display_a.setTextSize(1);
  display_a.setTextColor(WHITE);
  display_a.setCursor(10, 20);
  display_a.println(utf8rusd(name));
  // display_a.display(); 
}
void PlayerDisplay::showVolume(float v){
  display_a.setCursor(104,1);
  display_a.println(String((int)(v*100))+'%');
}

void PlayerDisplay::showLoadingDisplay(){
  
  display_a.clearDisplay();

  display_a.setTextSize(1);
  display_a.setTextColor(WHITE);
  display_a.setCursor(0, 10);
  // Display static text
  display_a.println("Starting...");
  display_a.display(); 
}

void PlayerDisplay::testConnection(){
  if(!display_a.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3D for 128x64
    Serial.println(F("SSD1306 allocation failed"));
  }
}

void PlayerDisplay::showPlaybackStatus(bool isPlay){
  if(isPlay){
    display_a.fillTriangle(95, 0, 99, 4, 95, 9, WHITE);
  }
  else{
    display_a.fillRect(95,0,3,10, WHITE);  
    display_a.fillRect(99,0,3,10, WHITE);  
  }
}

void PlayerDisplay::showProgress(float progress){
  display_a.drawRect(0,0,92,10,WHITE);
  display_a.fillRect(2,2,(int)(progress*92),6, WHITE);
  display_a.display(); 
}