#ifndef HARDWARE_PPI_H
#define HARDWARE_PPI_H

#include <Infrastructure/IAddressRangeHandler.h>

class PPIConsumer;

class PPI final : public IAddressRangeHandler {
public:
	explicit PPI(PPIConsumer *consumer);
	~PPI();

	void write(uint64_t address, unsigned int accessSize, uint64_t data) override;
	uint64_t read(uint64_t address, unsigned int accessSize) override;

private:
	uint8_t read8(uint64_t address, uint8_t mask);
	void write8(uint64_t address, uint8_t mask, uint8_t data);

	uint8_t portAMask() const;
	uint8_t portBMask() const;
	uint8_t portCMask() const;

	PPIConsumer* m_consumer;
	uint8_t m_porta;
	uint8_t m_portb;
	uint8_t m_portc;
	uint8_t m_mode;
};

#endif
