#ifndef MACHINE_H
#define MACHINE_H

#include <memory>
#include <optional>

#include <Utils/WindowsObjectTypes.h>
#include <Infrastructure/AddressSpaceDispatcher.h>
//#include <PCI/PCIBus.h>
//#include <Hardware/PCIConfigurationSpaceAccess.h>
#include <Infrastructure/MappedAddressRange.h>
#include <Infrastructure/AddressRangeRegistration.h>
#include <Infrastructure/DummyAddressRangeHandler.h>
#include <Hardware/PIC.h>
#include <Hardware/RTC.h>
#include <Hardware/PIT.h>
//#include <SMBus/SMBus.h>
//#include <IDE/IDEControllerChannel.h>
//#include <Hardware/SerialPort.h>
//#include <Hardware/SuperIO.h>
#include <Hardware/HerculesVideo.h>
#include <Hardware/NMIControl.h>
#include <Hardware/PPI.h>
#include <Hardware/PPIConsumer.h>
#include <Hardware/XTIDE.h>
#include <ATA/ATADemux.h>
#include <ATA/ATAHardDisk.h>

class CPUEmulation;

#define RAM_FILE
class Machine final : private PPIConsumer {
public:
	Machine();
	~Machine();

	Machine(const Machine& other) = delete;
	Machine& operator =(const Machine& other) = delete;

	inline VideoAdapter* videoAdapter() {
		return &m_hercules;
	}

private:
	static constexpr uint64_t RAMAreaBase = 0ULL;

	static constexpr uint64_t RAMAreaEnd = 0xC0000ULL;
	static constexpr uint64_t BIOSAreaBase = 0xFC000ULL;
	static constexpr uint64_t BIOSAreaEnd = 0x100000ULL;

	uint8_t readPortA(uint8_t mask) const override;
	void writePortA(uint8_t value, uint8_t mask) override;

	uint8_t readPortB(uint8_t mask) const override;
	void writePortB(uint8_t value, uint8_t mask) override;

	uint8_t readPortC(uint8_t mask) const override;
	void writePortC(uint8_t value, uint8_t mask) override;

	std::unique_ptr<CPUEmulation> m_cpu;
	AddressSpaceDispatcher m_mmioDispatcher;
	AddressSpaceDispatcher m_ioDispatcher;
	PIC m_primaryPIC;
	//PIC m_secondaryPIC;
	RTC m_rtc;
	PIT m_pit;
	//IDEControllerChannel m_primaryIDE;
	//IDEControllerChannel m_secondaryIDE;
	//PCIBus m_pci;
	//PCIConfigurationSpaceAccess m_pciConfigAccess;
	//SMBus m_smbus0;
	//SMBus m_smbus1;
	HerculesVideo m_hercules;
	NMIControl m_nmiControl;
	PPI m_ppi;
	ATAHardDisk m_hdd;
	ATADemux m_ataDemux;
	XTIDE m_xtide;
	WindowsHandle m_biosFile;
	WindowsHandle m_biosMapping;
	WindowsSectionView m_biosBase;
	std::optional<MappedAddressRange> m_biosMainAddressRange;
#if defined(RAM_FILE)
	WindowsHandle m_ramFile;
	WindowsHandle m_ramMapping;
	WindowsSectionView m_ramBase;
#else
	WindowsMemoryRegion m_ram;
#endif
	std::optional<MappedAddressRange> m_ramAddressRange;
	bool m_lowSwitches;
	uint8_t m_switches;
};

#endif
