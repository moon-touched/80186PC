#ifndef HARDWARE_XT_KEYBOARD_H
#define HARDWARE_XT_KEYBOARD_H

#include <atomic>
#include <mutex>
#include <deque>
#include <thread>

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

	void setReset(bool reset);

	inline bool hold() const {
		return m_hold;
	}

	inline void setHold(bool hold) {
		m_hold = hold;
	}

	inline uint8_t readDataByte() const {
		return m_scancode.load();
	}

	void pushScancode(uint8_t scancode) override;

private:
	void xtKeyboardThread();

	std::atomic<InterruptLine*> m_interruptLine;
	bool m_reset;
	std::atomic<bool> m_waitingForAck;
	std::atomic<bool> m_hold;
	std::deque<uint8_t> m_queue;
	std::atomic<uint8_t> m_scancode;

	std::mutex m_threadMutex;
	std::condition_variable m_threadCondvar;
	bool m_runThread;
	std::thread m_thread;
};

#endif
