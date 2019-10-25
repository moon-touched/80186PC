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

class CPUEmulation;

#define RAM_FILE
class Machine {
public:
	Machine();
	~Machine();

	Machine(const Machine& other) = delete;
	Machine& operator =(const Machine& other) = delete;

private:
	static constexpr uint64_t RAMAreaBase = 0ULL;

	static constexpr uint64_t RAMAreaEnd = 0xA0000ULL;
	static constexpr uint64_t BIOSAreaBase = 0xFE000ULL;
	static constexpr uint64_t BIOSAreaEnd = 0x100000ULL;

	std::unique_ptr<CPUEmulation> m_cpu;
	AddressSpaceDispatcher m_mmioDispatcher;
	AddressSpaceDispatcher m_ioDispatcher;
	PIC m_primaryPIC;
	PIC m_secondaryPIC;
	RTC m_rtc;
	PIT m_pit;
	//IDEControllerChannel m_primaryIDE;
	//IDEControllerChannel m_secondaryIDE;
	//PCIBus m_pci;
	//PCIConfigurationSpaceAccess m_pciConfigAccess;
	//SMBus m_smbus0;
	//SMBus m_smbus1;
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
};

#endif
