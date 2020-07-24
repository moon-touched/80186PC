#include <Hardware/Machine.h>

#include <comdef.h>

#include <Hardware/CPUEmulationFactory.h>
#include <Hardware/CPUEmulation.h>

#include <Utils/WindowsResources.h>

Machine::Machine(const std::filesystem::path &hardDiskImage) :
	m_mmioDispatcher("MMIO"),
	m_ioDispatcher("IO"),
	m_ram(RAMAreaEnd - RAMAreaBase, PAGE_READWRITE),
	m_vram(VRAMAreaEnd - VRAMAreaBase, PAGE_READWRITE),
	m_ppi(this),
	m_hdd(hardDiskImage),
	m_ataDemux(&m_hdd, nullptr),
	m_xtide(&m_ataDemux),
	m_lowSwitches(false),
	m_switches(0x3C),
	m_cpu(CPUEmulationFactory().createCPUEmulation())
{

	m_cpu->setMMIODispatcher(&m_mmioDispatcher);
	m_cpu->setIODispatcher(&m_ioDispatcher);
	m_mmioDispatcher.establishMappings(m_cpu.get(), 0x00000000ULL, 0x100000ULL);

	const void* biosImage;
	size_t biosImageSize;
	getRCDATA(1, &biosImage, &biosImageSize);

	if (biosImageSize != BIOSAreaEnd - BIOSAreaBase)
		throw std::runtime_error("unexpected BIOS image size");

	m_ramAddressRange.emplace(m_ram.base(), RAMAreaEnd - RAMAreaBase, MappedAddressRange::AccessRead | MappedAddressRange::AccessWrite | MappedAddressRange::AccessExecute);
		
	m_mmioDispatcher.registerAddressRange(
		RAMAreaBase, RAMAreaEnd, &*m_ramAddressRange
	).release();

	m_vramAddressRange.emplace(m_vram.base(), VRAMAreaEnd - VRAMAreaBase, MappedAddressRange::AccessRead | MappedAddressRange::AccessWrite | MappedAddressRange::AccessExecute);

	m_mmioDispatcher.registerAddressRange(
		VRAMAreaBase, VRAMAreaEnd, &*m_vramAddressRange
	).release();

	m_biosMainAddressRange.emplace(
		const_cast<void *>(biosImage), BIOSAreaEnd - BIOSAreaBase, MappedAddressRange::AccessRead | MappedAddressRange::AccessExecute
	);
	m_mmioDispatcher.registerAddressRange(BIOSAreaBase, BIOSAreaEnd, &*m_biosMainAddressRange).release();

	m_hercules.setFramebuffer(static_cast<unsigned char*>(m_vramAddressRange->hostMemoryBase()));

	m_ioDispatcher.registerAddressRange(0x20, 0x22, &m_primaryPIC).release(); // Primary programmable interrupt controller
	m_ioDispatcher.registerAddressRange(0x40, 0x60, &m_pit).release(); // Programmable interval timer
	m_ioDispatcher.registerAddressRange(0x60, 0x70, &m_ppi).release(); 
	m_ioDispatcher.registerAddressRange(0xA0, 0xB0, &m_nmiControl).release(); // NMI mask register

	m_aboveBoard.install(&m_ioDispatcher, 0x258, &m_mmioDispatcher);
	m_ioDispatcher.registerAddressRange(0x23C, 0x240, &m_busMouse).release();
	m_ioDispatcher.registerAddressRange(0x300, 0x320, &m_xtide).release();
	m_ioDispatcher.registerAddressRange(0x3B0, 0x3C0, &m_hercules).release();
	m_ioDispatcher.registerAddressRange(0x4D0, 0x4D1, &m_primaryPIC.elcr).release();
	

	/*
	 * Interrupt routing:
	 * 0 - PIT
	 * 1 - Keyboard
	 * 2 - 
	 * 3 - 
	 * 4 -
	 * 5 -  Bus mouse
	 * 6 - 
	 * 7 - IDE
	 */
	m_cpu->setInterruptController(&m_primaryPIC);
	m_primaryPIC.setOutputInterruptLine(m_cpu.get());
	m_pit.setInterruptLine(m_primaryPIC.line(0));
	m_xtKeyboard.setInterruptLine(m_primaryPIC.line(1));
	m_busMouse.setInterruptLine(m_primaryPIC.line(5));
	m_xtide.setInterruptLine(m_primaryPIC.line(7));
	
	m_cpu->start();
}

Machine::~Machine() {
	m_cpu->stop();
}

uint8_t Machine::readPortA(uint8_t mask) const {
	(void)mask;

	return m_xtKeyboard.readDataByte();
}

void Machine::writePortA(uint8_t value, uint8_t mask) {
	(void)value;
	(void)mask;
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

	m_xtKeyboard.setHold((value & (1 << 6)) == 0);
	m_xtKeyboard.setReset((value & (1 << 7)) != 0);

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
