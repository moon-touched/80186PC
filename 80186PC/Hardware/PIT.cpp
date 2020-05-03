#include <Hardware/PIT.h>
#include <Utils/AccessSizeUtils.h>

#include <stdio.h>
#include <Windows.h>

PIT::PIT() : m_interruptLine(nullptr), m_byte(false), m_writeLatch(0), m_compareValue(0), m_runPITThread(true), m_pitThreadAlert(false), m_pitThread(&PIT::pitThread, this) {

}

PIT::~PIT() {
	{
		std::unique_lock locker(m_pitThreadMutex);
		m_runPITThread = false;
	}
	m_pitThreadCondvar.notify_all();
	m_pitThread.join();
}

void PIT::write(uint64_t address, unsigned int accessSize, uint64_t data) {
	splitWriteAccess(address, accessSize, data, &PIT::write8, this);
}

uint64_t PIT::read(uint64_t address, unsigned int accessSize) {
	return splitReadAccess(address, accessSize, &PIT::read8, this);
}

void PIT::write8(uint64_t address, uint8_t mask, uint8_t data) {
	(void)mask;

	printf("PIT: write: %02llX, %02X\n", address, data);

	switch (address & 3) {
	case 0:
		if (m_byte) {
			{
				std::unique_lock locker(m_pitThreadMutex);
				m_compareValue = m_writeLatch | (data << 8);
				m_pitThreadAlert = true;
			}

			m_pitThreadCondvar.notify_all();
		}
		else {
			m_writeLatch = data;
		}
		m_byte = !m_byte;
		break;

	case 3:
		if (data == 0x34) {
			m_byte = false;
		}
		else {
			fprintf(stderr, "PIT emulation is very limited at the moment and only supports control byte 0x34\n");
			//if (IsDebuggerPresent())
			//	__debugbreak();
		}
		break;

	default:
		fprintf(stderr, "PIT emulation is very limited at the moment and only supports channel 0\n");
		//if (IsDebuggerPresent())
		//	__debugbreak();
	}
}

uint8_t PIT::read8(uint64_t address, uint8_t mask) {
	(void)mask;

	printf("PIT: read: %02llX\n", address);

	fprintf(stderr, "PIT emulation is very limited at the moment and does not support reads at all\n");

	if (IsDebuggerPresent())
		__debugbreak();

	return 0;
}

void PIT::pitThread() {
	std::unique_lock locker(m_pitThreadMutex);
	while (m_runPITThread) {
		uint64_t freq = m_compareValue;

		if (freq == 0) {
			freq = 65536;
		}

		std::chrono::microseconds delay(freq * 1000000 / 1193182);
		if (m_pitThreadCondvar.wait_for(locker, delay, [this]() { return m_pitThreadAlert || !m_runPITThread; })) {
			m_pitThreadAlert = false;
		}
		else {
			locker.unlock();

			auto line = m_interruptLine.load();
			if (line) {
				line->setInterruptAsserted(true);
				line->setInterruptAsserted(false);
			}

			locker.lock();
		}		
	}
}
