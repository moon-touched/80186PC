#ifndef PIT_H
#define PIT_H

#include <atomic>

#include <Infrastructure/IAddressRangeHandler.h>
#include <Infrastructure/InterruptLine.h>

#include <thread>
#include <mutex>
#include <condition_variable>

class PIT final : public IAddressRangeHandler {
public:
	PIT();
	~PIT();

	void write(uint64_t address, unsigned int accessSize, uint64_t data) override;
	uint64_t read(uint64_t address, unsigned int accessSize) override;

	inline InterruptLine* interruptLine() const {
		return m_interruptLine;
	}

	inline void setInterruptLine(InterruptLine* interruptLine) {
		m_interruptLine = interruptLine;
	}

private:
	void write8(uint64_t address, uint8_t mask, uint8_t data);
	uint8_t read8(uint64_t address, uint8_t mask);

	void pitThread();

	InterruptLine* m_interruptLine;
	bool m_byte;
	uint8_t m_writeLatch;
	uint16_t m_compareValue;
	std::mutex m_pitThreadMutex;
	bool m_runPITThread;
	bool m_pitThreadAlert;
	std::condition_variable m_pitThreadCondvar;
	std::thread m_pitThread;
};

#endif
