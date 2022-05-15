

class ButtonState{
    public:
        int prevCount;
        int count;
        bool isPressed();
        bool isJustPressed();
        bool isLongPressed();
        bool isShortReleased();
        bool isReleased();
        void setRaw(bool);
};