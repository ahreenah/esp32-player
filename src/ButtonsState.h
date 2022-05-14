#include <ButtonState.h>

class ButtonsState{
  public:
    ButtonState next;
    ButtonState prev;
    ButtonState up;
    ButtonState down;
    ButtonState center;

    void checkInput();
};