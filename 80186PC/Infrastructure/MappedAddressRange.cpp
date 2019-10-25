#include <Infrastructure/MappedAddressRange.h>

#include <stdio.h>

MappedAddressRange::MappedAddressRange(void* base, size_t size, unsigned int permissions) : m_base(base), m_size(size), m_permissions(permissions) {

}

MappedAddressRange::~MappedAddressRange() = default;

void MappedAddressRange::write(uint64_t address, unsigned int accessSize, uint64_t data) {
	if (!(m_permissions & AccessWrite)) {
		fprintf(stderr, "MappedAddressRange(%p): disallowed simulated write to %08llX, access size %u, data %08llX\n", this, address, accessSize, data);
		return;
	}

	switch (accessSize) {
	case 1:
		*reinterpret_cast<uint8_t*>(static_cast<uint8_t*>(m_base) + address % m_size) = static_cast<uint8_t>(data);
		break;
		
	case 2:
		*reinterpret_cast<uint16_t*>(static_cast<uint8_t*>(m_base) + address % m_size) = static_cast<uint16_t>(data);
		break;

	case 4:
		*reinterpret_cast<uint32_t*>(static_cast<uint8_t*>(m_base) + address % m_size) = static_cast<uint32_t>(data);
		break;

	case 8:
		*reinterpret_cast<uint64_t*>(static_cast<uint8_t*>(m_base) + address % m_size) = static_cast<uint64_t>(data);
		break;
	}
}

uint64_t MappedAddressRange::read(uint64_t address, unsigned int accessSize) {
	switch (accessSize) {
	case 1:
		return *reinterpret_cast<uint8_t*>(static_cast<uint8_t*>(m_base) + address % m_size);

	case 2:
		return *reinterpret_cast<uint16_t*>(static_cast<uint8_t*>(m_base) + address % m_size);

	case 4:
		return *reinterpret_cast<uint32_t*>(static_cast<uint8_t*>(m_base) + address % m_size);

	case 8:
		return *reinterpret_cast<uint64_t*>(static_cast<uint8_t*>(m_base) + address % m_size);

	default:
		__assume(0);
	}
}

void* MappedAddressRange::hostMemoryBase() const {
	return m_base;
}

size_t MappedAddressRange::hostMemorySize() const {
	return m_size;
}

unsigned int MappedAddressRange::hostMemoryPermissions() const {
	return m_permissions;
}
