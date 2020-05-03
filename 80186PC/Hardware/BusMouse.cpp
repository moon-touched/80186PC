#include <Hardware/BusMouse.h>
#include <Infrastructure/InterruptLine.h>

#include <stdio.h>

#include <algorithm>

BusMouse::BusMouse() :
	m_uiButtons(0), m_uiDeltaX(0), m_uiDeltaY(0), m_buttons(0), m_deltaX(0), m_deltaY(0),
	m_interruptLine(nullptr), m_mousePPI(this), m_runThread(true), m_interruptEnabled(false), m_busMouseThread(&BusMouse::busMouseThread, this) {

}

BusMouse::~BusMouse() {
	{
		std::unique_lock<std::mutex> locker(m_threadMutex);
		m_runThread = false;
	}
	m_threadCondvar.notify_all();
	m_busMouseThread.join();
}

void BusMouse::write(uint64_t address, unsigned int accessSize, uint64_t data) {
	m_mousePPI.write(address, accessSize, data);
}

uint64_t BusMouse::read(uint64_t address, unsigned int accessSize) {
	return m_mousePPI.read(address, accessSize);
}

uint8_t BusMouse::readPortA(uint8_t mask) const {
	uint8_t selectedPart = 0;

	switch (m_selector.load()) {
	case 0:
		selectedPart = static_cast<uint8_t>(m_deltaX.load()) & 0x0F;
		break;

	case 1:
		selectedPart = static_cast<uint8_t>(m_deltaX.load()) >> 4;
		break;

	case 2:
		selectedPart = static_cast<uint8_t>(m_deltaY.load()) & 0x0F;
		break;

	case 3:
		selectedPart = static_cast<uint8_t>(m_deltaY.load()) >> 4;
		break;
	}

	return selectedPart | (~m_buttons.load() << 5);
}

void BusMouse::writePortA(uint8_t value, uint8_t mask) {
	(void)value;
	(void)mask;
}

uint8_t BusMouse::readPortB(uint8_t mask) const {
	return 0xFF;
}

void BusMouse::writePortB(uint8_t value, uint8_t mask) {
	(void)value;
	(void)mask;
}

uint8_t BusMouse::readPortC(uint8_t mask) const {
	(void)mask;
	if (m_interruptAsserted.load() && m_interruptEnabled.load())
		return 1; // Interrupt line number: IRQ4
	else
		return 0;
}

void BusMouse::writePortC(uint8_t value, uint8_t mask) {
	auto newHold = (value & (1 << 7)) != 0;

	auto oldHold = m_hold.exchange(newHold);
	m_selector = (value >> 5) & 3;
	m_interruptEnabled = (value & (1 << 4)) == 0;

	if (newHold && !oldHold) {
		std::unique_lock<std::mutex> locker(m_uiMutex);
		m_buttons = m_uiButtons;
		m_deltaX = transferDeltaClamped(m_uiDeltaX);
		m_deltaY = transferDeltaClamped(m_uiDeltaY);
	}
}

int8_t BusMouse::transferDeltaClamped(int& delta) {
	if (delta < 0) {
		auto transfer = std::max<int>(delta, -128);
		delta -= transfer;

		return transfer;
	}
	else if (delta > 0) {
		auto transfer = std::min<int>(delta, 127);
		delta -= transfer;

		return transfer;
	}
	else {
		return 0;
	}
}

void BusMouse::busMouseThread() {
	std::unique_lock<std::mutex> locker(m_threadMutex);
	while (m_runThread) {
		if (!m_threadCondvar.wait_for(locker, std::chrono::milliseconds(33), [this] { return !m_runThread; })) {
			auto asserted = !m_interruptAsserted.load();
			m_interruptAsserted = asserted;

			auto line = m_interruptLine.load();
			if (line) {
				line->setInterruptAsserted(asserted && m_interruptEnabled.load());
			}
		}
	}
}

void BusMouse::updateButtonState(unsigned int button, bool state) {
	std::unique_lock<std::mutex> locker(m_uiMutex);
	if (state) {
		m_uiButtons |= (1U << button);
	}
	else {
		m_uiButtons &= ~(1U << button);
	}
}

void BusMouse::addDeltas(int dx, int dy) {
	std::unique_lock<std::mutex> locker(m_uiMutex);
	m_uiDeltaX += dx;
	m_uiDeltaY += dy;
}

