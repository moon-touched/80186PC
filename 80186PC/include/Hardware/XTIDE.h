#ifndef HARDWARE_XTIDE_H
#define HARDWARE_XTIDE_H

#include <Infrastructure/IAddressRangeHandler.h>
#include <atomic>
#include <ATA/ATADevice.h>

class InterruptLine;
class IATADevice;

class XTIDE : public IAddressRangeHandler, private IATADeviceHost {
public:
	explicit XTIDE(IATADevice *device);
	~XTIDE();

	void write(uint64_t address, unsigned int accessSize, uint64_t data) override;
	uint64_t read(uint64_t address, unsigned int accessSize) override;

	inline InterruptLine* interruptLine() const {
		return m_interruptLine.load();
	}

	inline void setInterruptLine(InterruptLine* interruptLine) {
		m_interruptLine = interruptLine;
	}

private:
	void write8(uint64_t address, uint8_t mask, uint8_t data);
	uint8_t read8(uint64_t address, uint8_t mask);

	void interruptRequestedChanged(IATADevice* device) override;

	IATADevice* m_device;
	std::atomic<InterruptLine*> m_interruptLine;
	uint16_t m_transferBuffer;
};


#endif
