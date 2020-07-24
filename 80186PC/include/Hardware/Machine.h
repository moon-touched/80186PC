#ifndef MACHINE_H
#define MACHINE_H

#include <memory>
#include <optional>

#include <Utils/WindowsObjectTypes.h>
#include <Infrastructure/AddressSpaceDispatcher.h>
#include <Infrastructure/MappedAddressRange.h>
#include <Infrastructure/AddressRangeRegistration.h>
#include <Infrastructure/DummyAddressRangeHandler.h>
#include <Hardware/PIC.h>
#include <Hardware/PIT.h>
#include <Hardware/HerculesVideo.h>
#include <Hardware/NMIControl.h>
#include <Hardware/PPI.h>
#include <Hardware/PPIConsumer.h>
#include <Hardware/XTIDE.h>
#include <Hardware/BusMouse.h>
#include <ATA/ATADemux.h>
#include <ATA/ATAHardDisk.h>
#include <Hardware/XTKeyboard.h>
#include <Hardware/AboveBoard.h>

class CPUEmulation;

class Machine final : private PPIConsumer {
public:
	explicit Machine(const std::filesystem::path &hardDiskImage);
	~Machine();

	Machine(const Machine& other) = delete;
	Machine& operator =(const Machine& other) = delete;

	inline VideoAdapter* videoAdapter() {
		return &m_hercules;
	}

	inline Keyboard* keyboard() {
		return &m_xtKeyboard;
	}

	inline Mouse* mouse() {
		return &m_busMouse;
	}

private:
	static constexpr uint64_t RAMAreaBase  = 0ULL;
	static constexpr uint64_t RAMAreaEnd   = 0x80000ULL;
	static constexpr uint64_t VRAMAreaBase = 0xB0000ULL;
	static constexpr uint64_t VRAMAreaEnd  = 0xC0000ULL;
	static constexpr uint64_t BIOSAreaBase = 0xF4000ULL;
	static constexpr uint64_t BIOSAreaEnd  = 0x100000ULL;

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
	PIT m_pit;
	HerculesVideo m_hercules;
	NMIControl m_nmiControl;
	PPI m_ppi;
	ATAHardDisk m_hdd;
	ATADemux m_ataDemux;
	XTIDE m_xtide;
	std::optional<MappedAddressRange> m_biosMainAddressRange;
	WindowsMemoryRegion m_ram;
	std::optional<MappedAddressRange> m_ramAddressRange;
	WindowsMemoryRegion m_vram;
	std::optional<MappedAddressRange> m_vramAddressRange;
	bool m_lowSwitches;
	uint8_t m_switches;
	XTKeyboard m_xtKeyboard;
	BusMouse m_busMouse;
	AboveBoard m_aboveBoard;
};

#endif
