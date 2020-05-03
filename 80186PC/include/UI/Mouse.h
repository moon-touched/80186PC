#ifndef UI_MOUSE_H
#define UI_MOUSE_H

class Mouse {
protected:
	Mouse();
	~Mouse();

public:
	virtual void updateButtonState(unsigned int button, bool state) = 0;
	virtual void addDeltas(int dx, int dy) = 0;
};

#endif
