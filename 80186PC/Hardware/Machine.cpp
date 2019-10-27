#include <Hardware/Machine.h>

#include <comdef.h>

#include <Hardware/CPUEmulationFactory.h>
#include <Hardware/CPUEmulation.h>

Machine::Machine() :
	m_cpu(CPUEmulationFactory().createCPUEmulation()),
	m_mmioDispatcher("MMIO"),
	m_ioDispatcher("IO"),
#ifndef RAM_FILE
	m_ram(RAMAreaEnd - RAMAreaBase, PAGE_READWRITE),
#endif 
	m_ppi(this),
	m_hdd("C:\\projects\\80186PC\\hdd.vhd"),
	m_ataDemux(&m_hdd, nullptr),
	m_xtide(&m_ataDemux),
	m_lowSwitches(false),
	m_switches(0x3C)
{

	m_cpu->setMMIODispatcher(&m_mmioDispatcher);
	m_cpu->setIODispatcher(&m_ioDispatcher);
	m_mmioDispatcher.establishMappings(m_cpu.get(), 0x00000000ULL, 0x100000ULL);

	auto rawHandle = CreateFile(
		L"C:\\projects\\80186PC\\bios\\bios.bin",
		GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr);
	if (rawHandle == INVALID_HANDLE_VALUE) {
		auto error = HRESULT_FROM_WIN32(GetLastError());
		_com_raise_error(error);
	}
	m_biosFile.reset(rawHandle);

	auto mapping = CreateFileMapping(m_biosFile.get(), nullptr, PAGE_READONLY, 0, BIOSAreaEnd - BIOSAreaBase, nullptr);
	if (mapping == nullptr) {
		auto error = HRESULT_FROM_WIN32(GetLastError());
		_com_raise_error(error);
	}
	m_biosMapping.reset(mapping);

	auto biosBase = MapViewOfFile(m_biosMapping.get(), FILE_MAP_READ, 0, 0, BIOSAreaEnd - BIOSAreaBase);
	if (!biosBase) {
		auto error = HRESULT_FROM_WIN32(GetLastError());
		_com_raise_error(error);
	}
	m_biosBase.reset(biosBase);

	#ifdef RAM_FILE	
		rawHandle = CreateFile(L"C:\\projects\\80186PC\\ram.bin", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
		if (rawHandle == INVALID_HANDLE_VALUE) {
			auto error = HRESULT_FROM_WIN32(GetLastError());
			_com_raise_error(error);
		}
		m_ramFile.reset(rawHandle);
		mapping = CreateFileMapping(m_ramFile.get(), nullptr, PAGE_READWRITE, 0, RAMAreaEnd - RAMAreaBase, nullptr);
		if (mapping == nullptr) {
			auto error = HRESULT_FROM_WIN32(GetLastError());
			_com_raise_error(error);
		}
		m_ramMapping.reset(mapping);
		auto ramBase = MapViewOfFile(m_ramMapping.get(), FILE_MAP_READ | FILE_MAP_WRITE, 0, 0, RAMAreaEnd - RAMAreaBase);
		if (!ramBase) {
			auto error = HRESULT_FROM_WIN32(GetLastError());
			_com_raise_error(error);
		}
		m_ramBase.reset(ramBase);
		m_ramAddressRange.emplace(m_ramBase.get(), RAMAreaEnd - RAMAreaBase, MappedAddressRange::AccessRead | MappedAddressRange::AccessWrite | MappedAddressRange::AccessExecute);

	#else
		m_ramAddressRange.emplace(m_ram.base(), RAMAreaEnd - RAMAreaBase, MappedAddressRange::AccessRead | MappedAddressRange::AccessWrite | MappedAddressRange::AccessWrite);
	#endif

	m_mmioDispatcher.registerAddressRange(
		RAMAreaBase, RAMAreaEnd - RAMAreaBase, &*m_ramAddressRange
	).release();

	m_biosMainAddressRange.emplace(
		m_biosBase.get(), BIOSAreaEnd - BIOSAreaBase, MappedAddressRange::AccessRead | MappedAddressRange::AccessWrite | MappedAddressRange::AccessExecute
	);
	m_mmioDispatcher.registerAddressRange(BIOSAreaBase, BIOSAreaEnd, &*m_biosMainAddressRange).release();

	m_hercules.setFramebuffer(static_cast<unsigned char*>(m_ramAddressRange->hostMemoryBase()) + 0xB0000);

	/*
	 * Legacy (non-PCI) PC hardware
	 */
	m_ioDispatcher.registerAddressRange(0x20, 0x40, &m_primaryPIC).release(); // Primary programmable interrupt controller
	m_ioDispatcher.registerAddressRange(0x40, 0x60, &m_pit).release(); // Programmable interval timer
	m_ioDispatcher.registerAddressRange(0x60, 0x70, &m_ppi).release(); 
	//m_ioDispatcher.registerAddressRange(0x70, 0x72, &m_rtc).release(); // Real-time 'CMOS' clock
	// 80-9F - DMA page registers
	m_ioDispatcher.registerAddressRange(0xA0, 0xB0, &m_nmiControl).release(); // NMI mask register

	//m_ioDispatcher.registerAddressRange(0x170, 0x178, &m_secondaryIDE).release();
	//m_ioDispatcher.registerAddressRange(0x1F0, 0x1F8, &m_primaryIDE).release();
	m_ioDispatcher.registerAddressRange(0x300, 0x320, &m_xtide).release();
	//m_ioDispatcher.registerAddressRange(0x376, 0x377, &m_secondaryIDE.control).release();
	m_ioDispatcher.registerAddressRange(0x3B0, 0x3BB, &m_hercules).release();
	//m_ioDispatcher.registerAddressRange(0x3F6, 0x3F7, &m_primaryIDE.control).release();
	//m_ioDispatcher.registerAddressRange(0x3F8, 0x400, &m_serialPort).release();
	m_ioDispatcher.registerAddressRange(0x4D0, 0x4D1, &m_primaryPIC.elcr).release();
	
	/*
	 * Interrupt routing:
	 * 0 - PIT
	 * 1 - PCI: OHCI 0:2.0
	 * 2 - PIC to PIC
	 * 3 - PCI: NV2A
	 * 4 - PCI: NIC
	 * 5 - PCI: APU
	 * 6 - PCI: AC97
	 * 7 - ?
	 * 8 - RTC
	 * 9 - PCI: OHCI 0:3.0
	 * 10 - ?
	 * 11 - PCI: SMBus
	 * 12 - ?
	 * 13 - ?
	 * 14 - Primary IDE
	 * 15 - Secondary IDE
	 */
	m_cpu->setInterruptController(&m_primaryPIC);
	m_primaryPIC.setOutputInterruptLine(m_cpu.get());
	m_pit.setInterruptLine(m_primaryPIC.line(0));
	//m_secondaryPIC.setOutputInterruptLine(m_primaryPIC.line(2));
	//m_primaryPIC.setSecondaryPIC(2, &m_secondaryPIC);
	m_xtide.setInterruptLine(m_primaryPIC.line(7));
	//m_rtc.setInterruptLine(m_secondaryPIC.line(0));
	//m_primaryIDE.setInterruptLine(m_secondaryPIC.line(6));
	//m_secondaryIDE.setInterruptLine(m_secondaryPIC.line(7));
	
	m_cpu->start();
}

Machine::~Machine() = default;

uint8_t Machine::readPortA(uint8_t mask) const {
	__debugbreak();
	return 0xFF;
}

void Machine::writePortA(uint8_t value, uint8_t mask) {
	(void)value;
	(void)mask;
	__debugbreak();
}

uint8_t Machine::readPortB(uint8_t mask) const {
	(void)mask;
	return 0xFF;
}

void Machine::writePortB(uint8_t value, uint8_t mask) {
	value |= ~mask;

	// TODO: 0: speaker gate
	// TODO: 1: speaker data
	// 2 - turbo
	m_lowSwitches = (value & (1 << 3)) == 0;
	// 4 - RAM parity check
	// 5 - I/O channel check
	// 6 - hold keyboard clock low, if low
	// 7 - reset keyboard, if high

	printf("PPI: port B: %02X\n", value);
}

uint8_t Machine::readPortC(uint8_t mask) const {
	(void)mask;

	uint8_t result = 0;

	if (m_lowSwitches) {
		result |= m_switches & 0x0F;
	}
	else {
		result |= (m_switches & 0xF0) >> 4;
	}

	// TODO: bit 5: timer channel 2 out
	// TODO: bit 6: I/O channel check
	// TODO: bit 7: RAM parity check

	return result;
}

void Machine::writePortC(uint8_t value, uint8_t mask) {
	(void)value;
	(void)mask;
}
