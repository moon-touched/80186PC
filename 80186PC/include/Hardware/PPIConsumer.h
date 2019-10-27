#ifndef PPI_CONSUMER_H
#define PPI_CONSUMER_H

#include <stdint.h>

class PPIConsumer {
protected:
	PPIConsumer();
	~PPIConsumer();

public:
	PPIConsumer(const PPIConsumer& other) = delete;
	PPIConsumer &operator =(const PPIConsumer& other) = delete;

	virtual uint8_t readPortA(uint8_t mask) const = 0;
	virtual void writePortA(uint8_t value, uint8_t mask) = 0;

	virtual uint8_t readPortB(uint8_t mask) const = 0;
	virtual void writePortB(uint8_t value, uint8_t mask) = 0;

	virtual uint8_t readPortC(uint8_t mask) const = 0;
	virtual void writePortC(uint8_t value, uint8_t mask) = 0;
};

#endif
