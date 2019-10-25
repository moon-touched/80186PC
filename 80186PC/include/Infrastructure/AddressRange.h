#ifndef ADDRESS_RANGE_H
#define ADDRESS_RANGE_H

#include <stdint.h>

class IAddressRangeHandler;

struct AddressRange {
	uint64_t beginAddress;
	uint64_t endAddress;
	uint64_t offset;
	IAddressRangeHandler* handler;

	inline bool operator<(const AddressRange& other) const {
		return beginAddress < other.beginAddress;
	}
};

#endif
