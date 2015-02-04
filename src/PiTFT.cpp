
#include "PiTFT.h"

#include <coreutils/log.h>

#include <fcntl.h>
#include <linux/fb.h>
#include <sys/mman.h>
#include <sys/ioctl.h>



PiTFT::PiTFT() {

	 // Open the file for reading and writing
	fbfd = open("/dev/fb1", O_RDWR);
	if(fbfd >= 0) {
		struct fb_var_screeninfo vinfo;
		struct fb_fix_screeninfo finfo;
	 // Get fixed screen information
		if(ioctl(fbfd, FBIOGET_FSCREENINFO, &finfo)) {
			LOGW("Error reading fixed information");
		}

		// Get variable screen information
		if(ioctl(fbfd, FBIOGET_VSCREENINFO, &vinfo)) {
			LOGW("Error reading variable information");
		}
		LOGI("### SCREEN %dx%d, %d bpp", vinfo.xres, vinfo.yres, vinfo.bits_per_pixel);
		bpp = vinfo.bits_per_pixel;
		width = vinfo.xres;
		height = vinfo.yres;
		// map framebuffer to user memory 
		int screensize = finfo.smem_len;
		screenPtr = (uint16_t*)mmap(0, screensize, PROT_READ | PROT_WRITE, MAP_SHARED, fbfd, 0);
	}
}