#include <wiringPi.h>

#include <unistd.h>
#include <stdint.h>
#include <vector>
#include <string>

using namespace std;

enum {
	LCD_RS = 7,
	LCD_E = 8,
	LCD_D4 = 25,
	LCD_D5 = 24,
	LCD_D6 = 23,
	LCD_D7 = 22
};

enum {
	E_DELAY = 500,
	E_PULSE = 500
};

static vector<int> dataPins { LCD_D4, LCD_D5, LCD_D6, LCD_D7 };

inline void toggleEnable() {
	usleep(E_DELAY);
	digitalWrite(LCD_E, HIGH);
	usleep(E_PULSE);
	digitalWrite(LCD_E, LOW);
	usleep(E_DELAY);
}

void lcd_byte(int b, int mode) {

	digitalWrite(LCD_RS, mode);

	for(auto dp : dataPins)
		digitalWrite(dp, LOW);
	for(int i=0; i<4; i++)
		if(b & (0x10<<i))
			digitalWrite(dataPins[i], HIGH);

	toggleEnable();

	for(auto dp : dataPins)
		digitalWrite(dp, LOW);
	
	for(int i=0; i<4; i++)
		if(b & (0x1<<i))
			digitalWrite(dataPins[i], HIGH);

	toggleEnable();
}

enum {
	CLEAR_DISPLAY = 0x01,
	CURSOR_HOME = 0x2,
	ENTRY_MODE = 0x4,
	DISPLAY_CTRL = 0x8,
	CURSOR = 0x10,
	FUNCTION = 0x20,
	SET_CGRAM = 0x40,
	SET_DGRAM = 0x80,

	EM_LEFT_TO_RIGHT = 0x2,
	EM_RIGHT_TO_LEFT = 0x0
};

enum {
	DG_LINE0 = 0x00,
	DG_LINE1 = 0x40,
	DG_LINE2 = 0x14,
	DG_LINE3 = 0x54
};

void lcd_init() {

	wiringPiSetupGpio();

	pinMode(LCD_RS, OUTPUT);
	pinMode(LCD_E, OUTPUT);
	for(auto dp : dataPins)
		pinMode(dp, OUTPUT);

	vector<int> initSequence { FUNCTION | 0x13, FUNCTION | 0x12, FUNCTION | 0x8, DISPLAY_CTRL | 0x4, ENTRY_MODE | EM_LEFT_TO_RIGHT, CLEAR_DISPLAY };
	for(auto b : initSequence) {
		lcd_byte(b, LOW);
	}
}

void lcd_string(const string &text) {
	for(auto c : text) {
		lcd_byte(c, HIGH);
	}
}

void lcd_print(int x, int y, const string &text) {
	static int start [4] = { DG_LINE0, DG_LINE1, DG_LINE2, DG_LINE3 };
	lcd_byte(SET_DGRAM | start[y] + x, LOW);
	lcd_string(text);
}

#ifdef TEST_LCD
int main(int argc, char **argv) {

	lcd_init();
	lcd_byte(SET_DGRAM | DG_LINE0, LOW);
	lcd_string(argv[1]);

	return 0;
}
#endif
