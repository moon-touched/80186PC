#include <ATA/ATADemux.h>

#include <ATA/ATATypes.h>

ATADemux::ATADemux(IATADevice* master, IATADevice* slave) : m_master(master), m_slave(slave), m_selectedDevice(false),
	m_ien0(false), m_ien1(false), m_host(nullptr) {

}

ATADemux::~ATADemux() = default;

void ATADemux::write(CS cs, uint8_t address, uint16_t value) {
	if (cs == CS::CS0 && address == ATACS0_DRIVEHEAD) {
		bool requested = isInterruptRequested();
		m_selectedDevice = (value & (1 << 4)) != 0;
		bool nowRequested = isInterruptRequested();

		if (requested != nowRequested) {
			m_host->interruptRequestedChanged(this);
		}
	}

	if (cs == CS::CS1 && address == ATACS1_ALTERNATESTATUS_DEVICECONTROL) {
		if (m_selectedDevice) {
			m_ien1 = (value & (1 << 1)) == 0;
		}
		else {
			m_ien0 = (value & (1 << 1)) == 0;
		}

		if (m_master) {
			m_master->write(cs, address, 0x08 | (value & 0x04) | (m_ien0 ? 0x00 : 0x02));
		}

		if (m_slave) {
			m_slave->write(cs, address, 0x08 | (value & 0x04) | (m_ien1 ? 0x00 : 0x02));
		}

		return;
	}
	else {
		auto device = m_selectedDevice ? m_slave : m_master;
		if (device) {
			device->write(cs, address, value);
		}
	}
}

uint16_t ATADemux::read(CS cs, uint8_t address) {
	auto device = m_selectedDevice ? m_slave : m_master;

	uint16_t result;

	if (device) {
		result = device->read(cs, address);
	}
	else {
		result = 0xFFFF;
	}

	if (cs == CS::CS1 && address == ATACS1_DRIVEADDRESS) {
		result &= 0xFFFC;

		if (m_selectedDevice) {
			result |= 0x01;
		}
		else {
			result |= 0x02;
		}
	}

	return result;
}

void ATADemux::attachToHost(IATADeviceHost* host) {
	m_host = host;
	if (m_master) {
		m_master->attachToHost(this);
	}

	if (m_slave) {
		m_slave->attachToHost(this);
	}
}

bool ATADemux::isInterruptRequested() const {
	auto device = m_selectedDevice ? m_slave : m_master;

	if (device) {
		return device->isInterruptRequested();
	}
	else {
		return false;
	}
}

void ATADemux::interruptRequestedChanged(IATADevice* device) {
	auto selectedDevice = m_selectedDevice ? m_slave : m_master;

	if (device && device == selectedDevice) {
		m_host->interruptRequestedChanged(this);
	}
}
