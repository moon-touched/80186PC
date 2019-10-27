#include <Hardware/PPI.h>
#include <Hardware/PPIConsumer.h>

#include <Utils/AccessSizeUtils.h>

#include <stdexcept>

PPI::PPI(PPIConsumer *consumer) : m_consumer(consumer), m_porta(0), m_portb(0), m_portc(0), m_mode(0) {

}

PPI::~PPI() = default;

void PPI::write(uint64_t address, unsigned int accessSize, uint64_t data) {
	return splitWriteAccess(address, accessSize, data, &PPI::write8, this);
}

uint64_t PPI::read(uint64_t address, unsigned int accessSize) {
	return splitReadAccess(address, accessSize, &PPI::read8, this);
}

uint8_t PPI::read8(uint64_t address, uint8_t mask) {
	(void)mask;

	address &= 3;

	switch (address) {
	case 0:
	{
		auto mask = portAMask();
		return (m_consumer->readPortA(mask) & ~mask) | (m_porta & mask);
	}

	case 1:
	{
		auto mask = portBMask();
		return (m_consumer->readPortB(mask) & ~mask) | (m_portb & mask);
	}

	case 2:
	{
		auto mask = portCMask();
		return (m_consumer->readPortC(mask) & ~mask) | (m_portc & mask);
	}

	default:
	case 3:
		return 0xFF;
	}
}

void PPI::write8(uint64_t address, uint8_t mask, uint8_t data) {
	(void)mask;

	address &= 3;

	switch (address) {
	case 0:
		m_porta = data;
		m_consumer->writePortA(m_porta, portAMask());
		break;

	case 1:
		m_portb = data;
		m_consumer->writePortB(m_portb, portBMask());
		break;

	case 2:
		m_portc = data;
		m_consumer->writePortC(m_portc, portCMask());
		break;

	case 3:
		if (data & 0x80) {
			// Mode set
			m_mode = data;

			if (m_mode & 0x64) {
				throw std::logic_error("non-zero modes are not supported");
			}
		}
		else {
			// bit set/reset
			auto mask = 1 << ((data >> 1) & 7);
			if (data & 1) {
				// bit set
				m_portc |= mask;
			}
			else {
				// bit reset
				m_portc &= ~mask;
			}
			m_consumer->writePortC(m_portc, portCMask());
		}
		break;
	}
}

uint8_t PPI::portAMask() const {
	if (m_mode & 16)
		return 0x00;
	else
		return 0xFF;
}

uint8_t PPI::portBMask() const {
	if (m_mode & 2)
		return 0x00;
	else
		return 0xFF;
}

uint8_t PPI::portCMask() const {
	uint8_t mask = 0;

	if (!(m_mode & 1)) {
		mask |= 0x0F;
	}

	if (!(m_mode & 8)) {
		mask |= 0xF0;
	}

	return mask;
}

