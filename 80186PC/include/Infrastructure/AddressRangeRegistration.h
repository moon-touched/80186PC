#ifndef ADDRESS_RANGE_REGISTRATION_H
#define ADDRESS_RANGE_REGISTRATION_H

#include <set>
#include <Infrastructure/AddressRange.h>

class AddressSpaceDispatcher;

class AddressRangeRegistration {
public:
	AddressRangeRegistration() noexcept;
	AddressRangeRegistration(AddressSpaceDispatcher& dispatcher, std::set<AddressRange>::const_iterator &&it);
	~AddressRangeRegistration();
	
	AddressRangeRegistration(const AddressRangeRegistration &other) = delete;
	AddressRangeRegistration &operator =(const AddressRangeRegistration &other) = delete;

	AddressRangeRegistration(AddressRangeRegistration &&other) noexcept;
	AddressRangeRegistration &operator =(AddressRangeRegistration &&other) noexcept;

	void reset();
	void release();

private:
	AddressSpaceDispatcher* m_dispatcher;
	std::set<AddressRange>::const_iterator m_it;
};

#endif
