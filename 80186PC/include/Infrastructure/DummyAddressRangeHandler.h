#ifndef DUMMY_ADDRESS_RANGE_HANDLER_H
#define DUMMY_ADDRESS_RANGE_HANDLER_H

#include <Infrastructure/IAddressRangeHandler.h>

#include <string>

class DummyAddressRangeHandler final : public IAddressRangeHandler {
public:
	explicit DummyAddressRangeHandler(std::string&& description);
	~DummyAddressRangeHandler();

	void write(uint64_t address, unsigned int accessSize, uint64_t data) override;
	uint64_t read(uint64_t address, unsigned int accessSize) override;

private:
	std::string m_description;
};

#endif
