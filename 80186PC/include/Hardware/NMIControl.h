#ifndef HARDWARE_NMI_CONTROL_H
#define HARDWARE_NMI_CONTROL_H

#include <Infrastructure/IAddressRangeHandler.h>

class NMIControl final : public IAddressRangeHandler {
public:
	NMIControl();
	~NMIControl();

	void write(uint64_t address, unsigned int accessSize, uint64_t data) override;
	uint64_t read(uint64_t address, unsigned int accessSize) override;

	inline bool isNMIAllowed() const {
		return m_allowNMI;
	}

private:
	uint8_t read8(uint64_t address, uint8_t mask);
	void write8(uint64_t address, uint8_t mask, uint8_t data);

	bool m_allowNMI;
};

#endif
