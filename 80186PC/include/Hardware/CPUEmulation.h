#ifndef HARDWARE_CPU_EMULATION_H
#define HARDWARE_CPU_EMULATION_H

#include <stdint.h>
#include <Infrastructure/InterruptLine.h>

class IAddressRangeHandler;
class InterruptController;

class CPUEmulation : public InterruptLine {
protected:
	CPUEmulation();

public:
	virtual ~CPUEmulation();

	CPUEmulation(const CPUEmulation& other) = delete;
	CPUEmulation& operator =(const CPUEmulation& other) = delete;

	inline IAddressRangeHandler* mmioDispatcher() const {
		return m_mmioDispatcher;
	}

	inline void setMMIODispatcher(IAddressRangeHandler* mmioDispatcher) {
		m_mmioDispatcher = mmioDispatcher;
	}

	inline IAddressRangeHandler* ioDispatcher() const {
		return m_ioDispatcher;
	}

	inline void setIODispatcher(IAddressRangeHandler* ioDispatcher) {
		m_ioDispatcher = ioDispatcher;
	}

	inline InterruptController* interruptController() const {
		return m_interruptController;
	}

	inline void setInterruptController(InterruptController* interruptController) {
		m_interruptController = interruptController;
	}

	virtual void start() = 0;
	virtual void stop() = 0;

	virtual void mapMemory(uint64_t base, uint64_t limit, void* hostMemory, unsigned int permissions) = 0;
	virtual void unmapMemory(uint64_t base, uint64_t limit) = 0;

private:
	IAddressRangeHandler* m_mmioDispatcher = nullptr;
	IAddressRangeHandler* m_ioDispatcher = nullptr;
	InterruptController* m_interruptController = nullptr;
};

#endif
