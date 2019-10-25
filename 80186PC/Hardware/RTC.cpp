#include <Hardware/RTC.h>
#include <Utils/AccessSizeUtils.h>

RTC::RTC() : m_interruptLine(nullptr), m_index(0) {
	for (auto& byte : m_cmosRAM) {
		byte = 0;
	}
}

RTC::~RTC() = default;

void RTC::write(uint64_t address, unsigned int accessSize, uint64_t data) {
	splitWriteAccess(address, accessSize, data, &RTC::write8, this);
}

uint64_t RTC::read(uint64_t address, unsigned int accessSize) {
	return splitReadAccess(address, accessSize, &RTC::read8, this);
}

void RTC::write8(uint64_t address, uint8_t mask, uint8_t data) {
	(void)mask;

	switch (address & 1) {
	case BusRegisterIndex:
		m_index = data & 0x7F;
		break;

	case BusRegisterData:
		writeReg(m_index, data);
		break;

	default:
		__assume(0);
	}
}

uint8_t RTC::read8(uint64_t address, uint8_t mask) {
	(void)mask;

	switch (address & 1) {
	case BusRegisterIndex:
		return m_index;

	case BusRegisterData:
		return readReg(m_index);

	default:
		__assume(0);
	}
}

void RTC::writeReg(uint8_t reg, uint8_t val) {
	printf("RTC: write register %02X: %02X\n", reg, val);

	m_cmosRAM[reg] = val;
}

uint8_t RTC::readReg(uint8_t reg) {
	printf("RTC: read register %02X\n", reg);

	return m_cmosRAM[reg];
}

