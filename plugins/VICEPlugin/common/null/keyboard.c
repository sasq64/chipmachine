void kbd_arch_init(void) {}

signed long kbd_arch_keyname_to_keynum(char *keyname) { return 0; }
const char *kbd_arch_keynum_to_keyname(signed long keynum) { return "NULL"; }
void kbd_initialize_numpad_joykeys(int *joykeys) {}
