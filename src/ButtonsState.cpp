#include <ButtonsState.h>
#include <Arduino.h>

void ButtonsState::checkInput(){
  // Serial.printf("prev is '%d' \n",digitalRead(12));
  next.setRaw(digitalRead(14)==0);
  prev.setRaw(digitalRead(12)==0);
  down.setRaw(digitalRead(13)==0);
  up.setRaw(digitalRead(32)==0);
  center.setRaw(digitalRead(33)==0);
  
  // if(digitalRead(12)==0)
  // prev += 1;
  // else prev=0;
  // if(digitalRead(32)==0)
  // up+=1;
  // else up=0;
  // if(digitalRead(13)==0)
  // down+=1;
  // else down=0;
  // if(digitalRead(33)==0)
  // center+=1;
  // else center=0;
}