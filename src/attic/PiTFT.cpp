
#include "PiTFT.h"

#include <coreutils/log.h>

#include <fcntl.h>
#include <linux/fb.h>
#include <sys/ioctl.h>
#include <sys/mman.h>

std::atomic<bool> hasFrame;
std::mutex frameMutex;
std::thread frameThread;
uint8_t frameData[320 * 240 * 4];

#ifdef RASPBERRYPI
PiTFT tft;
#endif

extTexture = Texture(320, 240);

frameThread = std::thread([=]() {
    while (true) {
        if (hasFrame) {
            frameMutex.lock();
            uint16_t* ptr = tft.ptr();
            for (int y = 0; y < 240; y++) {
                for (int x = 0; x < 320; x++) {
                    int i = x + y * 320;
                    // BGRA
                    ptr[(240 - y) * 320 + x] =
                        ((frameData[i * 4 + 2] & 0x1f) << 11) |
                        ((frameData[i * 4 + 1] & 0x2f) << 5) |
                        (frameData[i * 4 + 2] & 0x1f);
                }
            }
            hasFrame = false;
            frameMutex.unlock();
        } else
            sleepms(20);
    }
});
frameThread.detach();

#ifdef EXTERNAL_SCREEN
if (!hasFrame) {
    glBindTexture(GL_TEXTURE_2D, extTexture.id());
    extTexture.clear(0xff33cc33);
    extTexture.text(font, currentInfo.title, 10, 10);
    extTexture.text(font, currentInfo.composer, 10, 60);

    for (int i = 0; i < (int)eq.size(); i++) {
        extTexture.rectangle(10 + 10 * i, 100, 8, 100 - (50 * eq[i] / 256),
                             0xffff00ff);
    }
    frameMutex.lock();
    hasFrame = true;
    extTexture.get_pixels(frameData);
    frameMutex.unlock();
}
#endif

PiTFT::PiTFT()
{

    // Open the file for reading and writing
    fbfd = open("/dev/fb1", O_RDWR);
    if (fbfd >= 0) {
        struct fb_var_screeninfo vinfo;
        struct fb_fix_screeninfo finfo;
        // Get fixed screen information
        if (ioctl(fbfd, FBIOGET_FSCREENINFO, &finfo)) {
            LOGW("Error reading fixed information");
        }

        // Get variable screen information
        if (ioctl(fbfd, FBIOGET_VSCREENINFO, &vinfo)) {
            LOGW("Error reading variable information");
        }
        LOGI("### SCREEN %dx%d, %d bpp", vinfo.xres, vinfo.yres,
             vinfo.bits_per_pixel);
        bpp = vinfo.bits_per_pixel;
        width = vinfo.xres;
        height = vinfo.yres;
        // map framebuffer to user memory
        int screensize = finfo.smem_len;
        screenPtr = (uint16_t*)mmap(0, screensize, PROT_READ | PROT_WRITE,
                                    MAP_SHARED, fbfd, 0);
    }
}