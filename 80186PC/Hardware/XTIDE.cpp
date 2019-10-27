#include <Hardware/XTIDE.h>

#include <Utils/AccessSizeUtils.h>

#include <Infrastructure/InterruptLine.h>

#include <ATA/IATADevice.h>

#include <stdio.h>

XTIDE::XTIDE(IATADevice* device) : m_device(device), m_interruptLine(nullptr), m_transferBuffer(0) {
	m_device->attachToHost(this);
}

XTIDE::~XTIDE() = default;


void XTIDE::write(uint64_t address, unsigned int accessSize, uint64_t data) {
	splitWriteAccess(address, accessSize, data, &XTIDE::write8, this);
}

uint64_t XTIDE::read(uint64_t address, unsigned int accessSize) {
	return splitReadAccess(address, accessSize, &XTIDE::read8, this);
}

void XTIDE::write8(uint64_t address, uint8_t mask, uint8_t data) {
	(void)mask;

	address &= 0x1F;

	//printf("XTIDE: write: %08llX, %08X\n", address, data);

	IATADevice::CS cs;
	if ((address & 0x10) == 0) {
		cs = IATADevice::CS::CS0;
	}
	else {
		cs = IATADevice::CS::CS1;
	}

	auto reg = static_cast<uint8_t>((address & 0xE) >> 1);

	if (reg == 0) {
		__debugbreak(); // 16-bit stuff
	}

	return m_device->write(cs, reg, data);
}

uint8_t XTIDE::read8(uint64_t address, uint8_t mask) {
	(void)mask;

	address &= 0x1F;

	//printf("XTIDE: read: %08llX\n", address);

	IATADevice::CS cs;
	if ((address & 0x10) == 0) {
		cs = IATADevice::CS::CS0;
	}
	else {
		cs = IATADevice::CS::CS1;
	}

	auto reg = static_cast<uint8_t>((address & 0xE) >> 1);

	if (reg == 0) {
		if (address & 1) {
			return m_transferBuffer >> 8;
		}
		else {
			m_transferBuffer = m_device->read(cs, reg);
			return m_transferBuffer & 0xFF;
		}
	}

	return static_cast<uint8_t>(m_device->read(cs, reg));
}

void XTIDE::interruptRequestedChanged(IATADevice* device) {
	m_interruptLine.load()->setInterruptAsserted(device->isInterruptRequested());
}