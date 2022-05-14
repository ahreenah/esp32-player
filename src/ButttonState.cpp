#include <ButtonState.h>

// кнопку держат
bool ButtonState::isPressed(){
    return count>0;
}

// только что нажали
bool ButtonState::isJustPressed(){
    return count==1;
}

// уже держат и при чем долго
bool ButtonState::isLongPressed(){
    return count>10;
}

// вызывается после отпуксания при коротком нажатии на один такт
bool ButtonState::isShortReleased(){
    return (count==0) && (prevCount>0) && (prevCount<120);
}

// вызывается после отпуксания при коротком нажатии на один такт
bool ButtonState::isReleased(){
    return (count==0) && (prevCount>0);
}

// задание сырого значения извне (для обработчика кнопок)
void ButtonState::setRaw(bool value){
    prevCount=count;
    if(value){
        if(count < 2000)
            count++;
    } else {
        count=0;
    }
}