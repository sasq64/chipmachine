
#include <cstdint>

class PiTFT
{
public:
    PiTFT();

    uint16_t* ptr() { return screenPtr; }

private:
    int fbfd;
    int width;
    int height;
    int bpp;

    uint16_t* screenPtr;
};