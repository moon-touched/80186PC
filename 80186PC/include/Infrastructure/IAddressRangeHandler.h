#ifndef I_ADDRESS_RANGE_HANDLER_H
#define I_ADDRESS_RANGE_HANDLER_H

#include <stdint.h>

class CPUEmulation;

class IAddressRangeHandler {
protected:
	IAddressRangeHandler();
	~IAddressRangeHandler();

public:
	IAddressRangeHandler(const IAddressRangeHandler &other) = delete;
	IAddressRangeHandler &operator =(const IAddressRangeHandler &other) = delete;

	enum {
		AccessRead = 1,
		AccessWrite = 2,
		AccessExecute = 4
	};

	virtual void write(uint64_t address, unsigned int accessSize, uint64_t data) = 0;
	virtual uint64_t read (uint64_t address, unsigned int accessSize) = 0;

	virtual void* hostMemoryBase() const;
	virtual size_t hostMemorySize() const;
	virtual unsigned int hostMemoryPermissions() const;

	virtual void establishMappings(CPUEmulation* emulation, uint64_t base, uint64_t limit);
	virtual void removeMappings(CPUEmulation* emulation, uint64_t base, uint64_t limit);
};

#endif
