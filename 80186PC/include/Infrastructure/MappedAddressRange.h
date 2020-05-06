#ifndef MAPPED_ADDRESS_RANGE_H
#define MAPPED_ADDRESS_RANGE_H

#include <Infrastructure/IAddressRangeHandler.h>

class MappedAddressRange final : public IAddressRangeHandler {
public:
	MappedAddressRange(void* base, size_t size, unsigned int permissions);
	~MappedAddressRange();

	void write(uint64_t address, unsigned int accessSize, uint64_t data) override;
	uint64_t read(uint64_t address, unsigned int accessSize) override;

	void* hostMemoryBase() const override;
	size_t hostMemorySize() const override;
	unsigned int hostMemoryPermissions() const override;

	inline void changeBase(void* base) {
		m_base = base;
	}

private:
	void* m_base;
	size_t m_size;
	unsigned int m_permissions;
};

#endif
