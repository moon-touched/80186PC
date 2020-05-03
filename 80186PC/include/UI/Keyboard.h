#ifndef UI_KEYBOARD_H
#define UI_KEYBOARD_H

#include <stdint.h>

class Keyboard {
protected:
	Keyboard();
	~Keyboard();

public:
	Keyboard(const Keyboard& other) = delete;
	Keyboard &operator =(const Keyboard& other) = delete;

	virtual void pushScancode(uint8_t scancode) = 0;
};

#endif
