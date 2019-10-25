#ifndef INTERRUPT_LINE_H
#define INTERRUPT_LINE_H

class InterruptLine {
protected:
	InterruptLine();
	~InterruptLine();

public:
	InterruptLine(const InterruptLine& other) = delete;
	InterruptLine &operator =(const InterruptLine& other) = delete;

	virtual void setInterruptAsserted(bool interrupt) = 0;
};

#endif
