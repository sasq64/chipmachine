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


void lcd_init() {
	vector<int> initSequence { 0x33, 0x32, 0x28, 0xc, 0x6, 0x1 };
	for(auto b : initSequence) {
		lcd_byte(b, LOW);
	}
}

void lcd_string(const string &text) {

	for(auto c : text) {
		lcd_byte(c, HIGH);
	}
}

int main(int argc, char **argv) {

	wiringPiSetupGpio();

	pinMode(LCD_RS, OUTPUT);
	pinMode(LCD_E, OUTPUT);
	pinMode(LCD_D4, OUTPUT);
	pinMode(LCD_D5, OUTPUT);
	pinMode(LCD_D6, OUTPUT);
	pinMode(LCD_D7, OUTPUT);

	lcd_init();

	lcd_byte(0x80, LOW);

	lcd_string(argv[1]);

	return 0;
}
