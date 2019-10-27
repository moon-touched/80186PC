#ifndef ATA_ATA_DEMUX_H
#define ATA_ATA_DEMUX_H

#include <ATA/IATADevice.h>

class ATADemux final : public IATADevice, private IATADeviceHost {
public:
	ATADemux(IATADevice* master, IATADevice* slave);
	~ATADemux();

	void write(CS cs, uint8_t address, uint16_t value) override;
	uint16_t read(CS cs, uint8_t address) override;

	void attachToHost(IATADeviceHost* host) override;

	bool isInterruptRequested() const override;

private:
	void interruptRequestedChanged(IATADevice* device) override;

	IATADevice* m_master;
	IATADevice* m_slave;
	bool m_selectedDevice;
	bool m_ien0;
	bool m_ien1;
	IATADeviceHost* m_host;
};

#endif
