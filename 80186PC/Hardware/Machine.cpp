#include <Hardware/Machine.h>

#include <comdef.h>

#include <Hardware/CPUEmulationFactory.h>
#include <Hardware/CPUEmulation.h>

Machine::Machine() :
	m_cpu(CPUEmulationFactory().createCPUEmulation()),
	m_mmioDispatcher("MMIO"),
	m_ioDispatcher("IO")
#ifndef RAM_FILE
	m_ram(RAMAreaEnd - RAMAreaBase, PAGE_READWRITE),
#endif 
{

	m_cpu->setMMIODispatcher(&m_mmioDispatcher);
	m_cpu->setIODispatcher(&m_ioDispatcher);
	m_mmioDispatcher.establishMappings(m_cpu.get(), 0x00000000ULL, 0x100000ULL);

	auto rawHandle = CreateFile(
		L"C:\\projects\\80186PC\\bios\\pcxtbios.bin",
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

	/*
	 * Legacy (non-PCI) PC hardware
	 */
	m_ioDispatcher.registerAddressRange(0x20, 0x22, &m_primaryPIC).release(); // Primary programmable interrupt controller
	m_ioDispatcher.registerAddressRange(0x40, 0x48, &m_pit).release(); // Programmable interval timer
	m_ioDispatcher.registerAddressRange(0x70, 0x72, &m_rtc).release(); // Real-time 'CMOS' clock
	m_ioDispatcher.registerAddressRange(0xA0, 0xA2, &m_secondaryPIC).release();
	//m_ioDispatcher.registerAddressRange(0x170, 0x178, &m_secondaryIDE).release();
	//m_ioDispatcher.registerAddressRange(0x1F0, 0x1F8, &m_primaryIDE).release();
	//m_ioDispatcher.registerAddressRange(0x376, 0x377, &m_secondaryIDE.control).release();
	//m_ioDispatcher.registerAddressRange(0x3F6, 0x3F7, &m_primaryIDE.control).release();
	//m_ioDispatcher.registerAddressRange(0x3F8, 0x400, &m_serialPort).release();
	m_ioDispatcher.registerAddressRange(0x4D0, 0x4D1, &m_primaryPIC.elcr).release();
	m_ioDispatcher.registerAddressRange(0x4D1, 0x4D2, &m_secondaryPIC.elcr).release();

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
	m_secondaryPIC.setOutputInterruptLine(m_primaryPIC.line(2));
	m_primaryPIC.setSecondaryPIC(2, &m_secondaryPIC);
	m_rtc.setInterruptLine(m_secondaryPIC.line(0));
	//m_primaryIDE.setInterruptLine(m_secondaryPIC.line(6));
	//m_secondaryIDE.setInterruptLine(m_secondaryPIC.line(7));

	m_cpu->start();
}

Machine::~Machine() = default;
