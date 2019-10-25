#include <Infrastructure/DummyAddressRangeHandler.h>

DummyAddressRangeHandler::DummyAddressRangeHandler(std::string&& description) : m_description(std::move(description)) {

}

DummyAddressRangeHandler::~DummyAddressRangeHandler() = default;

void DummyAddressRangeHandler::write(uint64_t address, unsigned int accessSize, uint64_t data) {
	printf("%s: unhandled write: address %08llX, size %u, data %08llX\n", m_description.c_str(), address, accessSize, data);
}

uint64_t DummyAddressRangeHandler::read(uint64_t address, unsigned int accessSize) {
	printf("%s: unhandled read: address %08llX, size %u\n", m_description.c_str(), address, accessSize);
	return 0;
}

