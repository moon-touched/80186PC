#include <Hardware/XTKeyboard.h>
#include <Infrastructure/InterruptLine.h>

XTKeyboard::XTKeyboard() : m_runThread(true), m_thread(&XTKeyboard::xtKeyboardThread, this) {

}

XTKeyboard::~XTKeyboard() {
	{
		std::unique_lock<std::mutex> locker(m_threadMutex);
		m_runThread = false;
	}
	m_threadCondvar.notify_all();
	m_thread.join();
}

void XTKeyboard::pushScancode(uint8_t scancode) {
	{
		std::unique_lock<std::mutex> locker(m_threadMutex);

		m_queue.push_back(scancode);
	}
	m_threadCondvar.notify_one();
}

void XTKeyboard::setReset(bool reset) {
	m_reset = reset;
	if (reset) {
		bool wasWaiting = m_waitingForAck.exchange(false);
		if (wasWaiting) {
			m_threadCondvar.notify_all();
		}
	}
}

void XTKeyboard::xtKeyboardThread() {
	std::unique_lock<std::mutex> locker(m_threadMutex);
	while (m_runThread) {
		m_threadCondvar.wait(locker, [this]() { return !m_runThread || (!m_queue.empty() && !m_hold); });

		if (!m_runThread)
			break;

		locker.unlock();

		std::this_thread::sleep_for(std::chrono::milliseconds(1));

		locker.lock();

		m_scancode = m_queue.front();
		m_waitingForAck = true;

		m_queue.pop_front();

		auto line = m_interruptLine.load();
		if (line) {
			line->setInterruptAsserted(true);
		}

		m_threadCondvar.wait(locker, [this]() { return !m_runThread || !m_waitingForAck; });

		if (m_runThread && line) {
			line->setInterruptAsserted(false);
		}
	}
}

/*

	auto line = m_interruptLine.load();
	if (line) {
		line->setInterruptAsserted(true);
		line->setInterruptAsserted(false);
	}
}*/