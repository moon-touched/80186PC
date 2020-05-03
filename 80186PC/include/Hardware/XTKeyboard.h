#ifndef HARDWARE_XT_KEYBOARD_H
#define HARDWARE_XT_KEYBOARD_H

#include <atomic>
#include <mutex>
#include <deque>

#include <UI/Keyboard.h>

class InterruptLine;

class XTKeyboard final : public Keyboard {
public:
	XTKeyboard();
	~XTKeyboard();

	XTKeyboard(const XTKeyboard& other) = delete;
	XTKeyboard &operator =(const XTKeyboard& other) = delete;

	inline InterruptLine* interruptLine() const {
		return m_interruptLine;
	}

	inline void setInterruptLine(InterruptLine* interruptLine) {
		m_interruptLine = interruptLine;
	}

	inline bool reset() const {
		return m_reset;
	}

	inline void setReset(bool reset) {
		m_reset = reset;
	}

	inline bool hold() const {
		return m_hold;
	}

	inline void setHold(bool hold) {
		m_hold = hold;
	}

	uint8_t readDataByte();

	void pushScancode(uint8_t scancode) override;

private:
	std::atomic<InterruptLine*> m_interruptLine;
	bool m_reset;
	bool m_hold;
	std::mutex m_queueLock;
	std::deque<uint8_t> m_queue;
};

#endif
