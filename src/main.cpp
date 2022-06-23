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
 * bluetooth headphones support
 * 
 * IN_PROGRESS:
 * 
 * WiFi server
 *  - update new tracks
 * 
 * POSSIBLE:
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


String currentFile;

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
ButtonsState buttons;
PlayOrder po;
bool ch;
bool paused;


uint32_t pos;
uint32_t size;

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



void playFile(File file, bool addToList = false){
  if (String(file.name()).endsWith(".mp3")) { 
    source->close();
    if (source->open(file.name())) { 
      currentFile=file.name(); 
      if(addToList){
        po.addToList(file.name());
      }
      po.showOrderSerial();

      Serial.printf_P(PSTR("Playing '%s' from SD card...\n"), file.name());
  
  
  ////////////////
    
    // source = new AudioFileSourceSD(file.name());
    source->open(file.name());
    id3 = new AudioFileSourceID3(source);
    // id3->RegisterMetadataCB(MDCallback, (void*)"ID3TAG");
    // out = new AudioOutputI2SNoDAC();
    // decoder = new AudioGeneratorMP3();
  
    // decoder->begin(id3, out);
  /////////////


      id3->RegisterMetadataCB(MDCallback, (void*)"ID3");
      decoder->begin(id3, out);
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
  if(nextName.length()==0){
    while (!(String(file.name()).endsWith(".mp3"))){
      file.close();
      // delete &file;
      file = dir.openNextFile();  
    }
  }
  else{
    file.close();
    // delete &file;
    file = SD.open(nextName.c_str());

  }

  if (file) {  
    playFile(file,true);
  } else {
    Serial.println(F("Playback form SD card done\n"));
    delay(1000);
  }     
}


AsyncWebServer server(80);

// Replace with your network credentials
const char* ssid = "EidenPearce";
const char* password = "Didim2007";


const char* ap_ssid = "EspPlayer";
const char* ap_password = "EsPlay123";
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

const char state_json[] PROGMEM = R"rawliteral(
{
  "fileNname":"%fileNme%",
  "progress":%progress%,
  "paused":%paused%
}
)rawliteral";


const char file_list_json[] PROGMEM = R"rawliteral(
{
  "files":[%list%]
}
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


String stateProcessor(const String& var){
  if(var == "fileName"){
    return currentFile;
  }
  if(var=="progress"){
    return  (String)((size==0)?1:(float)pos/size);
  }
  
  if(var=="paused"){
    return paused?"true":"false";
  }
  return String();
}

String filesProcessor(const String& var){
  
  String res = "";
  File root = SD.open("/");
 
  File file = root.openNextFile();
 
  while(file){
 
      Serial.print("FILE: ");
      res+=String("{\"name\":\"")+String(file.name())+"\",\"isDir\": "+(file.isDirectory()?"true":"false")+" },";
      Serial.println(file.name());
      
 
      file = root.openNextFile();
  }

  if(var=="list"){
    return res+String("null");
  }
  return String();
}

String resetProcessor(const String& var){
  Serial.println("next track...");
  playNextTrack();

  if(var == "data"){
    String buttons = "";
    buttons += "{\"status\':\"true\"}";
    return buttons;
  }
  return String();
}

File writedFile;

void handleUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
    String logmessage = "Client:" + request->client()->remoteIP().toString() + " " + request->url();
    Serial.println(logmessage);

    if (!index) {

    // File myFile = SD.open("/test.txt",FILE_WRITE);
    // Serial.print("Writing to test.txt...");
    // myFile.println("testing 1, 2, 3.");
    // close the file:
    // myFile.close();

      writedFile = SD.open("/"+String(filename),FILE_WRITE);
      logmessage = "Upload Start: " + String(filename);
      // open the file on first call and store the file handle in the request object
      request->_tempFile = SPIFFS.open("/" + filename, "w");
      Serial.println(logmessage);
    }

    if (len) {
      // stream the incoming chunk to the opened file
      // request->_tempFile.write(data, len);
      writedFile.write(data, len);
      logmessage = "Writing file: " + String(filename) + " index=" + String(index) + " len=" + String(len);
      Serial.println(logmessage);
      vTaskDelay(1);
    }

    if (final) {
      writedFile.close();
      logmessage = "Upload Complete: " + String(filename) + ",size: " + String(index + len);
      // close the file handle as the upload is now done
      request->_tempFile.close();
      Serial.println(logmessage);
      request->redirect("/");
    }
  }

void startServer(){
  
   server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html, processor);
  });
  
  server.on("/next", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", api_json, nextProcessor);
  });

  
  server.on("/state", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/json", state_json, stateProcessor);
  });

  
  server.on("/files", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/json", file_list_json, filesProcessor);
  });

  server.on("/upload",HTTP_POST, [](AsyncWebServerRequest *request){
      AsyncResponseStream *response = request->beginResponseStream("text/html");
      // response->addHeader("Server","ESP Async Web Server");
      response->printf("<!DOCTYPE html><html><head><title>Webpage at %s</title></head><body>", request->url().c_str());

      response->print("<h2>Hello ");
      response->print(request->client()->remoteIP());
      response->print("</h2>");



      response->print("<h3>Parameters</h3>");
      response->print("<ul>");
      int params = request->params();
      Serial.println("Parsing started:");
      for(int i=0;i<params;i++){
        AsyncWebParameter* p = request->getParam(i);
        if(p->isFile()){
          Serial.println("   Parsing file");
          response->printf("<li>FILE[%s]: %s, size: %u</li>", p->name().c_str(), p->value().c_str(), p->size());
        } else if(p->isPost()){
          Serial.println("   Parsing string");
          response->printf("<li>POST[%s]: %s</li>", p->name().c_str(), p->value().c_str());
        } 
      }
      response->print("</ul>");

      response->print("</body></html>");
      Serial.println("Parsing ended");
      //send the response last
      request->send(response);

  });

  


  server.on("/upload-demo",HTTP_POST,  [](AsyncWebServerRequest *request) {
        request->send(200);
      }, handleUpload);

  // calls some bug when uncommented server
  
  // server.on("/reset", HTTP_GET, [](AsyncWebServerRequest *request){
  //   request->send_P(200, "text/json", state_json, resetProcessor);
  //   ESP.restart();
  // });

}

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



void PlayTaskCode(void* p){
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

  #if defined(ESP8266)
    SD.begin(SS, SPI_SPEED);
  #else
    SD.begin( );
  #endif
  dir = SD.open("/"); 
  DisplayHandle = xSemaphoreCreateMutex();
  xTaskCreate(PlayTaskCode, "PlayTask",39000,NULL,1,NULL);
  xTaskCreate(ConnectTaskCode, "ConnecvtTask",20000,NULL,3,NULL);
  startServer();
}

int tick=0;
void loop() {
    ch=false;
    if(tick==10){
      // dis.showName(currentFile);
      pos = source->getPos();
      size = source->getSize();
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