#ifndef ATA_I_ATA_DEVICE_H
#define ATA_I_ATA_DEVICE_H

#include <stdint.h>

class IATADevice;

class IATADeviceHost {
protected:
	IATADeviceHost();
	~IATADeviceHost();

public:
	IATADeviceHost(const IATADeviceHost& other) = delete;
	IATADeviceHost &operator =(const IATADeviceHost& other) = delete;

	virtual void interruptRequestedChanged(IATADevice* device) = 0;
};

class IATADevice {
protected:
	IATADevice();
	~IATADevice();

public:
	IATADevice(const IATADevice& other) = delete;
	IATADevice &operator =(const IATADevice& other) = delete;

	enum class CS {
		CS0,
		CS1
	};

	virtual void write(CS cs, uint8_t address, uint16_t value) = 0;
	virtual uint16_t read(CS cs, uint8_t address) = 0;

	virtual void attachToHost(IATADeviceHost* host) = 0;

	virtual bool isInterruptRequested() const = 0;
};

#endif
