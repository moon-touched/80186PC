#include <Hardware/NMIControl.h>
#include <Utils/AccessSizeUtils.h>

NMIControl::NMIControl() : m_allowNMI(false) {

}

NMIControl::~NMIControl() = default;

void NMIControl::write(uint64_t address, unsigned int accessSize, uint64_t data) {
	return splitWriteAccess(address, accessSize, data, &NMIControl::write8, this);
}

uint64_t NMIControl::read(uint64_t address, unsigned int accessSize) {
	return splitReadAccess(address, accessSize, &NMIControl::read8, this);
}

uint8_t NMIControl::read8(uint64_t address, uint8_t mask) {
	(void)address;
	(void)mask;

	return 0xFF;
}

void NMIControl::write8(uint64_t address, uint8_t mask, uint8_t data) {
	(void)address;
	(void)mask;

	m_allowNMI = (data & 0x80) != 0;
}