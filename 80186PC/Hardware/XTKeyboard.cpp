#include <Hardware/XTKeyboard.h>
#include <Infrastructure/InterruptLine.h>

XTKeyboard::XTKeyboard() = default;

XTKeyboard::~XTKeyboard() = default;

uint8_t XTKeyboard::readDataByte() {
	bool interrupt = false;
	uint8_t scancode;

	{
		std::unique_lock<std::mutex> locker(m_queueLock);

		if (m_queue.empty()) {
			scancode = 0;
		}
		else {
			scancode = m_queue.front();
			m_queue.pop_front();

			interrupt = !m_queue.empty();
		}
	}

	if (interrupt) {
		auto line = m_interruptLine.load();
		if (line) {
			line->setInterruptAsserted(true);
			line->setInterruptAsserted(false);
		}
	}

	return scancode;
}

void XTKeyboard::pushScancode(uint8_t scancode) {
	{
		std::unique_lock<std::mutex> locker(m_queueLock);

		m_queue.push_back(scancode);
	}

	auto line = m_interruptLine.load();
	if (line) {
		line->setInterruptAsserted(true);
		line->setInterruptAsserted(false);
	}

}