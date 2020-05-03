#ifndef HARDWARE_BUS_MOUSE_H
#define HARDWARE_BUS_MOUSE_H

#include <Infrastructure/IAddressRangeHandler.h>
#include <Hardware/PPI.h>
#include <Hardware/PPIConsumer.h>
#include <UI/Mouse.h>

#include <atomic>
#include <mutex>
#include <condition_variable>
#include <thread>

class InterruptLine;

class BusMouse final : public IAddressRangeHandler, private PPIConsumer, public Mouse {
public:
	BusMouse();
	~BusMouse();	

	inline InterruptLine* interruptLine() const {
		return m_interruptLine;
	}

	inline void setInterruptLine(InterruptLine* interruptLine) {
		m_interruptLine = interruptLine;
	}

	void write(uint64_t address, unsigned int accessSize, uint64_t data) override;
	uint64_t read(uint64_t address, unsigned int accessSize) override;
	
	void updateButtonState(unsigned int button, bool state) override;
	void addDeltas(int dx, int dy) override;

private:
	uint8_t readPortA(uint8_t mask) const override;
	void writePortA(uint8_t value, uint8_t mask) override;

	uint8_t readPortB(uint8_t mask) const override;
	void writePortB(uint8_t value, uint8_t mask) override;

	uint8_t readPortC(uint8_t mask) const override;
	void writePortC(uint8_t value, uint8_t mask) override;

	void busMouseThread();

	int8_t transferDeltaClamped(int& delta);

	std::mutex m_uiMutex;
	unsigned int m_uiButtons;
	int m_uiDeltaX;
	int m_uiDeltaY;

	std::atomic<unsigned int> m_buttons;
	std::atomic<int8_t> m_deltaX;
	std::atomic<int8_t> m_deltaY;
	
	PPI m_mousePPI;
	std::atomic<InterruptLine*> m_interruptLine;
	std::mutex m_threadMutex;
	bool m_runThread;
	std::condition_variable m_threadCondvar;
	std::atomic<bool> m_interruptEnabled;
	std::atomic<bool> m_interruptAsserted;
	std::atomic<bool> m_hold;
	std::atomic<unsigned int> m_selector;
	std::thread m_busMouseThread;
};

#endif
