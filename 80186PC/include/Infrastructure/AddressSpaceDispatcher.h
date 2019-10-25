#ifndef ADDRESS_SPACE_DISPATCHER_H
#define ADDRESS_SPACE_DISPATCHER_H

#include <set>
#include <string>

#include <Infrastructure/AddressRange.h>
#include <Infrastructure/IAddressRangeHandler.h>

class IAddressRangeHandler;
class AddressRangeRegistration;
class CPUEmulation;

class AddressSpaceDispatcher final : public IAddressRangeHandler {
public:
	explicit AddressSpaceDispatcher(std::string &&name);
	~AddressSpaceDispatcher();

	AddressSpaceDispatcher(const AddressSpaceDispatcher &other) = delete;
	AddressSpaceDispatcher &operator =(const AddressSpaceDispatcher &other) = delete;

	void write(uint64_t address, unsigned int accessSize, uint64_t data) override;
	uint64_t read(uint64_t address, unsigned int accessSize) override;

	void establishMappings(CPUEmulation* emulation, uint64_t base, uint64_t limit) override;
	void removeMappings(CPUEmulation* emulation, uint64_t base, uint64_t limit) override;

	AddressRangeRegistration registerAddressRange(uint64_t base, uint64_t limit, IAddressRangeHandler *handler, uint64_t offset = 0);
	void unregisterAddressRange(std::set<AddressRange>::const_iterator it);

	IAddressRangeHandler* findHandlerForRange(uint32_t base);

private:
	IAddressRangeHandler* resolve(uint64_t &address);

	void establishAddressRangeMappings(const AddressRange& registration);
	void removeAddressRangeMappings(const AddressRange& registration);

	std::string m_name;
	CPUEmulation* m_cpuEmulation;
	uint64_t m_addressSpaceBegin;
	uint64_t m_addressSpaceLimit;
	std::set<AddressRange> m_ranges;
};

#endif

