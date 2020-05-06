#ifndef HARDWARE_ABOVE_BOARD_H
#define HARDWARE_ABOVE_BOARD_H

#include <Infrastructure/IAddressRangeHandler.h>
#include <Infrastructure/AddressRangeRegistration.h>
#include <Infrastructure/MappedAddressRange.h>
#include <Utils/WindowsObjectTypes.h>

#include <stdint.h>

#include <array>

class AddressSpaceDispatcher;

class AboveBoard final : public IAddressRangeHandler {
public:
	AboveBoard();
	~AboveBoard();

	void install(AddressSpaceDispatcher* ioDispatcher, unsigned int baseAddress, AddressSpaceDispatcher *memoryDispatcher);
	
	void write(uint64_t address, unsigned int accessSize, uint64_t data) override;
	uint64_t read(uint64_t address, unsigned int accessSize) override;

private:
	uint8_t read8(uint64_t address, uint8_t mask);
	void write8(uint64_t address, uint8_t mask, uint8_t data);

	static uint8_t repackAddress(uint64_t address);

	void map(unsigned int logicalPage, bool present, unsigned int physicalPage);

	struct LogicalPageState {
		LogicalPageState();
		~LogicalPageState();

		uint8_t bank;
		uint8_t value;

		MappedAddressRange addressRange;
		AddressRangeRegistration registration;
	};

	WindowsMemoryRegion m_expandedMemory;
	AddressSpaceDispatcher* m_memoryDispatcher;
	std::array<LogicalPageState, 32> m_logicalPages;

	uint8_t m_reg7Field0;
};

#endif
