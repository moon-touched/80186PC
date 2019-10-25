#include <Infrastructure/IAddressRangeHandler.h>

IAddressRangeHandler::IAddressRangeHandler() = default;

IAddressRangeHandler::~IAddressRangeHandler() = default;

void* IAddressRangeHandler::hostMemoryBase() const {
	return nullptr;
}

size_t IAddressRangeHandler::hostMemorySize() const {
	return 0;
}

unsigned int IAddressRangeHandler::hostMemoryPermissions() const {
	return 0;
}

void IAddressRangeHandler::establishMappings(CPUEmulation* emulation, uint64_t base, uint64_t limit) {
	(void)emulation;
	(void)base;
	(void)limit;
}

void IAddressRangeHandler::removeMappings(CPUEmulation* emulation, uint64_t base, uint64_t limit) {
	(void)emulation;
	(void)base;
	(void)limit;
}
