/**
 * working sound for internal dac - D25
 * SD : 
 *      CS - D5
 *      MOSI - D23
 *      CLK - D18
 *      MSIO - D19
 * 
 * DISPLAY :
 *      SCL - D22
 *      SDA - D21
 * 
 * CONTROL :
 *      NEXT   - D14
 *      PREV   - D12
 *      UP     - D32
 *      DOWN   - D13
 *      CENTER - D33
 **/

/**
 * Development plan:
 * 
 * PLANNED:
 * volume indication
 * display meta information instead of file name
 * support of other formats, not only mp3
 * external I2S DAC support
 * WiFi server
 *  - update new tracks
 * 
 * POSSIBLE:
 * bluetooth headphones support
 * 
 * READY:
 * play/pause icon in top bar
 * 
 **/


#include <SPIFFS.h>
#include <HTTPClient.h>
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

//wifi
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>

AsyncWebServer server(80);

// Replace with your network credentials
const char* ssid = "243B47";
const char* password = "hr9wuxwkk2";


const char* ap_ssid = "EspPlayer";
const char* ap_password = "EsPlay123";

// void WifiConnect(){
//   // Connect to Wi-Fi
//   WiFi.begin(ssid, password);	
//   while (WiFi.status() != WL_CONNECTED) {
//     Serial.println("Connecting to WiFi..");
//   }
//   Serial.println("WiFi connected");
// }

bool serverRunning = false;

void ConnectTaskCode(void * p){
  Serial.println("Connect task started");
  WiFi.begin(ssid, password);	
  
  // WifiConnect();
  while(true){
                                  // 4 - errors
                                  // 6 - disconnected
    if(WiFi.status()==3){
      if(!serverRunning){
        Serial.println("Connected:"+WiFi.localIP().toString());
        server.begin();
        Serial.println("Server started");
        serverRunning = true;
      }
    }
    else{
      
      Serial.println("Connecting...");
      Serial.println(WiFi.status());// 3-connected
    }
    vTaskDelay(1000);
  }
}

#define SPI_SPEED SD_SCK_MHZ(1)
#define SPDIF_OUT_PIN 27



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
String currentFile;
void MDCallback(void *cbData, const char *type, bool isUnicode, const char *string)
{
  (void)cbData;
  Serial.printf("ID3 callback for: %s = '", type);
  bool isTitle = String(type).equals("Title");

  if (isUnicode) {
    string += 2;
  }
  String s = "";
  
  while (*string) {
    char a = *(string++);
    if (isUnicode) {
      string++;
    }
    Serial.printf("%c", a);
    s+=a;
  }
  if(isTitle){
    dis.showName(String(s));
    Serial.println("title set: "+s);
    title=String(s);
  }
  // else dis.showName(currentFile);
  Serial.printf("'\n");
}



String utf8rus(String source)
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

ButtonsState buttons;
PlayOrder po;
bool ch;
bool paused;

void playFile(File file, bool addToList = false){
  String fileName = String(file.name());
  fileName.toLowerCase();
  if (fileName.endsWith(".mp3")) { 
    source->close();
    if (source->open(file.path())) { 
      currentFile=file.name(); 
      if(addToList){
        po.addToList(file.path());
      }
      po.showOrderSerial();

      Serial.printf_P(PSTR("Playing '%s' from SD card...\n"), file.path());
  
  
  ////////////////
    
    // source = new AudioFileSourceSD(file.name());
    source->open(file.path());
    id3 = new AudioFileSourceID3(source);
    // id3->RegisterMetadataCB(MDCallback, (void*)"ID3TAG");
    // out = new AudioOutputI2SNoDAC();
    // decoder = new AudioGeneratorMP3();
  
    // decoder->begin(id3, out);
  /////////////


      id3->RegisterMetadataCB(MDCallback, (void*)"ID3");
      decoder->begin(id3, out);
      Serial.println("Playback started");
    } else {
      Serial.printf_P(PSTR("Error opening '%s'\n"), file.name());
    }
  }
}

void playPrevTrack(){ 
  ch=true;
  Serial.println("prev");
  File file = SD.open(po.prev().c_str());
  if (file) {
    playFile(file);
  } else {
    Serial.println(F("Playback form SD card done\n"));
    delay(1000);
  }
}

void playNextTrack(){
  ch=true;
  Serial.println("next");
  File file;

  String nextName = po.next();
  Serial.println(nextName);
  if(nextName.length()==0){
    Serial.println("in if");
    String fileName = String(file.name());
    fileName.toLowerCase();
    while (!fileName.endsWith(".mp3")){
      // Serial.print("next one"); 
      fileName = String(file.name());
      fileName.toLowerCase();
      Serial.println(fileName);
      file.close();
      // delete &file;
      file = dir.openNextFile();  
    }
    Serial.println("after loop");
  }
  else{
    Serial.println("in else");
    file.close();
    // delete &file;
    file = SD.open(nextName.c_str());

  }
  Serial.print("Found file:");
  Serial.println(file.path());

  if (file) {  
    playFile(file,true);
  } else {
    Serial.println(F("Playback form SD card done\n"));
    delay(1000);
  }     
}

void volumeUp(){
  ch=true;
  Serial.println("'up'");
  gain += gainStep;
  if(gain>4)
    gain = 4.0;
  out->SetGain(gain);
  Serial.println("cf:");
  Serial.printf("%f",source->getPos());
  Serial.println(currentFile);
}

void volumeDown(){
  ch=true;
  Serial.println("'down'");
  gain -= gainStep;
  if(gain<=0)
    gain = 0;
  out->SetGain(gain);
}

void togglePause(){
  paused=!paused;
}

portMUX_TYPE  mux;

void processNextButton(){
  playNextTrack();
}

void processPrevButton(){
  playPrevTrack();
}

void processUpButton(){
  volumeUp();
}

void processDownButton(){
  volumeDown();
}

void processCenterButton(){
  togglePause();
}

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <title>ESP Web Server</title>
</head>
<body>
  <h2>ESP Web Server</h2>
  %BUTTONPLACEHOLDER%
</body>
</html>
)rawliteral";

const char api_json[] PROGMEM = R"rawliteral(
%data%"
)rawliteral";

// Replaces placeholder with button section in your web page
String processor(const String& var){
  //Serial.println(var);
  if(var == "BUTTONPLACEHOLDER"){
    String buttons = "";
    buttons += "<h4>currently playing:"+currentFile+"</h4>";
    return buttons;
  }
  return String();
}
String nextProcessor(const String& var){
  Serial.println("next track...");
  playNextTrack();

  if(var == "data"){
    String buttons = "";
    buttons += "<h4>currently playing:"+currentFile+"</h4>";
    return buttons;
  }
  return String();
}


void PlayTaskCode(void* p){
  Serial.println("Play task started");
  for (;;){
    buttons.checkInput();
    // gain
    // Serial.println("buttons: " +String(buttons.prev)+' '+String(buttons.next));
    if(buttons.up.isPressed()){
      processUpButton();
    }
    else if(buttons.down.isPressed()){
      xSemaphoreTake(DisplayHandle, portMAX_DELAY);
      processDownButton();
      xSemaphoreGive(DisplayHandle);
    }
    if ((decoder) && (decoder->isRunning()) ) {
      Serial.println('1');
      // идет воспроизведение, песня не окончилась
      
      if(buttons.prev.isShortReleased()){
        processPrevButton();
      }
      else if (buttons.next.isShortReleased()){
        processNextButton();  
      }
      else if (buttons.center.isShortReleased()){
        processCenterButton();  
      }
      else{
      // кнопки не нажаты
        if(!paused)
          if (!decoder->loop()) decoder->stop();
      }
    } else {
      Serial.println('2');
      // песня окончилась
      playNextTrack();
    }
    vTaskDelay(1);
  }
}

void setup() {
  pinMode(14, INPUT_PULLUP);
  pinMode(12, INPUT_PULLUP);
  pinMode(32, INPUT_PULLUP);
  pinMode(13, INPUT_PULLUP);
  pinMode(33, INPUT_PULLUP);
  Serial.begin(115200);
  Serial.println("Hello from ESPlayer!");
  delay(1000);

  dis.testConnection();

  dis.showLoadingDisplay();


  audioLogger = &Serial;  
  source = new AudioFileSourceSD();
  
  id3 = new AudioFileSourceID3(source);
  
  id3->RegisterMetadataCB(MDCallback, (void*)"ID3");
  out = new AudioOutputI2S(0,1);//uncomment for internal
  decoder = new AudioGeneratorMP3();
  if(
    #if defined(ESP8266)
      SD.begin(SS, SPI_SPEED)
    #else
      SD.begin( )
    #endif
  ){
    Serial.println("sd init successful");
  }else{
    Serial.println("Sd init error");
    while(1){
      delay(1);
    }
  }
  dir = SD.open("/"); 
  
  DisplayHandle = xSemaphoreCreateMutex();
  xTaskCreate(PlayTaskCode, "PlayTask",39000,NULL,1,NULL);
  xTaskCreate(ConnectTaskCode, "ConnecvtTask",20000,NULL,3,NULL);
  // void * pp;
  // PlayTaskCode(pp);
   server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html, processor);
  });
  
  server.on("/next", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", api_json, nextProcessor);
  });

}


int tick=0;
void loop() {
    ch=false;
    if(tick==10){
      // dis.showName(currentFile);
      uint32_t pos = source->getPos();
      uint32_t size = source->getSize();
      float progress = (size==0)?1:(float)pos/size; 
      dis.showPlaybackStatus(!paused);
      dis.showVolume(gain);
      dis.showProgress(progress);
      dis.showName(currentFile);      
        //"\110-\207-\240-\142-\277");
        //currentFile);      
      tick=0;
    }
  tick++;
  vTaskDelay(10);
}