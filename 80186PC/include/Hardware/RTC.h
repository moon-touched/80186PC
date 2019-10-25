#ifndef RTC_H
#define RTC_H

#include <Infrastructure/IAddressRangeHandler.h>
#include <array>

class InterruptLine;

class RTC : public IAddressRangeHandler {
public:
	RTC();
	~RTC();

	void write(uint64_t address, unsigned int accessSize, uint64_t data) override;
	uint64_t read(uint64_t address, unsigned int accessSize) override;

	inline InterruptLine* interruptLine() const {
		return m_interruptLine;
	}

	inline void setInterruptLine(InterruptLine* interruptLine) {
		m_interruptLine = interruptLine;
	}

private:
	void write8(uint64_t address, uint8_t mask, uint8_t data);
	uint8_t read8(uint64_t address, uint8_t mask);

	void writeReg(uint8_t reg, uint8_t val);
	uint8_t readReg(uint8_t reg);

	static constexpr unsigned int BusRegisterIndex = 0;
	static constexpr unsigned int BusRegisterData = 1;

	InterruptLine* m_interruptLine;
	std::array<uint8_t, 128> m_cmosRAM;
	uint8_t m_index;
};

#endif
