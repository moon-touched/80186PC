#include <Infrastructure/AddressRangeRegistration.h>
#include <Infrastructure/AddressSpaceDispatcher.h>

#include <algorithm>

AddressRangeRegistration::AddressRangeRegistration() noexcept : m_dispatcher(nullptr) {

}

AddressRangeRegistration::AddressRangeRegistration(AddressSpaceDispatcher &dispatcher, std::set<AddressRange>::const_iterator&& it) : m_dispatcher(&dispatcher), m_it(std::move(it)) {

}

AddressRangeRegistration::AddressRangeRegistration(AddressRangeRegistration&& other) noexcept : AddressRangeRegistration() {
	*this = std::move(other);
}

AddressRangeRegistration& AddressRangeRegistration::operator =(AddressRangeRegistration&& other) noexcept {
	std::swap(m_dispatcher, other.m_dispatcher);
	std::swap(m_it, other.m_it);

	return *this;
}

AddressRangeRegistration::~AddressRangeRegistration() {
	reset();
}

void AddressRangeRegistration::reset() {
	if (m_dispatcher) {
		m_dispatcher->unregisterAddressRange(m_it);
		m_dispatcher = nullptr;
	}
}

void AddressRangeRegistration::release() {
	m_dispatcher = nullptr;
}

