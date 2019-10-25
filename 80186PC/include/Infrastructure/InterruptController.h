#ifndef INTERRUPT_CONTROLLER_H
#define INTERRUPT_CONTROLLER_H

#include <stdint.h>

class InterruptController {
protected:
	InterruptController();
	~InterruptController();

public:
	virtual uint8_t processInterruptAcknowledge() = 0;
};

#endif
