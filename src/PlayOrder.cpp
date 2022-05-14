#include <Arduino.h>
#include "PlayOrder.h"

void PlayOrder::addToList(String v){
    num++;
    if(num>maxNum)maxNum=num;
    list[num]=v;
}
String PlayOrder::prev(){
    if(num>0)
        num--;
    String ret = list[num];
    return ret;
}
String PlayOrder::next(){
    if(num<maxNum){
        num++;
        return list[num];
    }
    return "";
}

void PlayOrder::showOrderSerial(){
    Serial.println("");
    Serial.println("Play order");
    for(int i=0; i<=num; i++){
        Serial.print(i+1);
        Serial.print(". ");
        Serial.println(list[i]);
    }
    Serial.println("");
}