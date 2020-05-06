#include <Hardware/AboveBoard.h>
#include <Infrastructure/AddressSpaceDispatcher.h>
#include <Infrastructure/AddressRangeRegistration.h>
#include <Utils/AccessSizeUtils.h>

AboveBoard::AboveBoard() :
	m_expandedMemory(8 * 1024 * 1024, PAGE_READWRITE),
	m_memoryDispatcher(nullptr),
	m_reg7Field0(0) {

}

AboveBoard::~AboveBoard() = default;

void AboveBoard::install(AddressSpaceDispatcher * ioDispatcher, unsigned int baseAddress, AddressSpaceDispatcher *memoryDispatcher) {
	m_memoryDispatcher = memoryDispatcher;

	for (unsigned int highAddress = 0; highAddress <= 0xF000; highAddress += 0x1000) {
		ioDispatcher->registerAddressRange(baseAddress + highAddress - 0x08, baseAddress + highAddress + 0x08, this, highAddress).release();
	}

	for (unsigned int page = 0; page < 8; page++) {
		map(page + 16, true, page);
	}
}

void AboveBoard::write(uint64_t address, unsigned int accessSize, uint64_t data) {
	return splitWriteAccess(address, accessSize, data, &AboveBoard::write8, this);
}

uint64_t AboveBoard::read(uint64_t address, unsigned int accessSize) {
	return splitReadAccess(address, accessSize, &AboveBoard::read8, this);
}

uint8_t AboveBoard::read8(uint64_t address, uint8_t mask) {
	(void)mask;

	uint8_t repackedAddress = repackAddress(address);

	printf("AboveBoard: read: %02X\n", repackedAddress);

	switch (repackedAddress) {
	case 0x09:
		//return 0x20; detects as 'Board for AT'
		return 0;

	case 0x0F:
		return (m_reg7Field0 + 3) & 7;

	default:
		if ((repackedAddress & 0x08) == 0) {
			// read page register

			unsigned int logicalPage = ((repackedAddress >> 6) & 3) | ((repackedAddress & 7) << 2);

			auto& logicalPageState = m_logicalPages[logicalPage];

			if (logicalPageState.bank == ((repackedAddress >> 4) & 3)) {
				return logicalPageState.value;
			}
			else {
				return 0x00;
			}
		}
	}
	
	return 0xFF;
}

void AboveBoard::write8(uint64_t address, uint8_t mask, uint8_t data) {
	(void)mask;

	uint8_t repackedAddress = repackAddress(address);

	printf("AboveBoard: write: %02X <- %02X\n", repackedAddress, data);

	switch (repackedAddress) {
	case 0x0F:
		m_reg7Field0 = data & 7;
		break;

	default:
		if ((repackedAddress & 0x08) == 0) {
			// write page register
			unsigned int logicalPage = ((repackedAddress >> 6) & 3) | ((repackedAddress & 7) << 2);
			bool present = (data & 0x80) != 0;
			unsigned int physicalPage = (data & 0x7F) | (((repackedAddress >> 4) & 3) << 7);

			map(logicalPage, present, physicalPage);
		}
		break;
	}
}

void AboveBoard::map(unsigned int logicalPage, bool present, unsigned int physicalPage) {
	auto& logicalPageState = m_logicalPages[logicalPage];

	logicalPageState.bank = (physicalPage >> 7);
	logicalPageState.value = (present << 7) | (physicalPage & 0x7F);

	printf("AboveBoard: mapping logical page %u; present: %d, physical page: %u\n", logicalPage, present, physicalPage);

	logicalPageState.registration.reset();

	if (present) {
		unsigned int offset = 0;
		if (logicalPage < 24) {
			offset = 0x40000;
		}
		else {
			offset = 0x60000;
		}

		unsigned int logicalPageBase =  offset + (logicalPage << 14);
		unsigned int logicalPageLimit = offset + ((logicalPage + 1) << 14);

		logicalPageState.addressRange.changeBase(static_cast<uint8_t*>(m_expandedMemory.base()) + physicalPage * 16 * 1024);

		logicalPageState.registration = m_memoryDispatcher->registerAddressRange(logicalPageBase, logicalPageLimit, &logicalPageState.addressRange, 0, true);
	}
}

uint8_t AboveBoard::repackAddress(uint64_t address) {
	return (address & 15) | ((address & 0xF000) >> 8);
}

AboveBoard::LogicalPageState::LogicalPageState() : addressRange(nullptr, 128 * 1024, IAddressRangeHandler::AccessRead | IAddressRangeHandler::AccessWrite | IAddressRangeHandler::AccessExecute) {

}

AboveBoard::LogicalPageState::~LogicalPageState() = default;
