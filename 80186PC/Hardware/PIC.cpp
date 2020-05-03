#include <Hardware/PIC.h>

#include <Utils/AccessSizeUtils.h>

#include <stdio.h>
#include <assert.h>

#undef TRACE_INTERRUPTS

PIC::ELCR::ELCR(PIC* owner) : m_owner(owner), m_value(0) {

}

PIC::ELCR::~ELCR() = default;

void PIC::ELCR::write(uint64_t address, unsigned int accessSize, uint64_t data) {
	splitWriteAccess(address, accessSize, data, &PIC::ELCR::write8, this);
}

uint64_t PIC::ELCR::read(uint64_t address, unsigned int accessSize) {
	return splitReadAccess(address, accessSize, &PIC::ELCR::read8, this);
}

void PIC::ELCR::write8(uint64_t address, uint8_t mask, uint8_t data) {
	(void)address;
	(void)mask;
	m_value = data;
}

uint8_t PIC::ELCR::read8(uint64_t address, uint8_t mask) {
	(void)address;
	(void)mask;
	return m_value;
}

PIC::PIC() :
	elcr(this),
	m_interruptLines{
		PICInterruptLine(this, 0),
		PICInterruptLine(this, 1),
		PICInterruptLine(this, 2),
		PICInterruptLine(this, 3),
		PICInterruptLine(this, 4),
		PICInterruptLine(this, 5),
		PICInterruptLine(this, 6),
		PICInterruptLine(this, 7)
	},
	m_secondaryPICs{
		nullptr, nullptr, nullptr, nullptr,
		nullptr, nullptr, nullptr, nullptr,
	},
	m_outputInterruptLine(nullptr),
	m_state(State::Idle), m_edgeSense(0), m_mask(0), m_icw1(0), m_icw2(0), m_icw3(0), m_icw4(0), m_ocw3(0), m_isr(0), m_irr(0) {

}

PIC::~PIC() = default;

void PIC::write(uint64_t address, unsigned int accessSize, uint64_t data) {
	splitWriteAccess(address, accessSize, data, &PIC::write8, this);
}

uint64_t PIC::read(uint64_t address, unsigned int accessSize) {
	return splitReadAccess(address, accessSize, &PIC::read8, this);
}

void PIC::write8(uint64_t address, uint8_t mask, uint8_t data) {
	(void)mask;

	std::unique_lock locker(m_picMutex);

#if defined(TRACE_INTERRUPTS)
	printf("PIC: write: %u <- %02X\n", static_cast<uint32_t>(address), data);
#endif

	switch (address & 1) {
	case RegisterCommand:
		if (data & CommandICW1) {
			m_edgeSense = 0;
			m_mask = 0;
			// IR7 input is assigned priority 7
			// The slave mode address is set to 7
			m_ocw3 = 0;
			m_icw1 = data;
			if (!(m_icw1 & ICW1_IC4)) {
				m_icw4 = 0;
			}
			m_state = State::WaitingForICW2;
		}
		else if (data & CommandOCW3) {
			if (data & OCW3_RR) {
				m_ocw3 = (m_ocw3 & ~OCW3_RIS) | (data & OCW3_RIS);
			}

			if (data & OCW3_POLL) {
				m_state = State::Poll;
			}

			if (data & OCW3_ESMM) {
				m_ocw3 = (m_ocw3 & ~OCW3_SMM) | (data & OCW3_SMM);
				assert(!(m_ocw3 & OCW3_SMM));
			}

			m_ocw3 = data;
		}
		else {
			// OCW2
			
			assert(!(data & OCW2_R));

			if (data & OCW2_EOI) {
				unsigned int level;

				if (data & OCW2_SL) {
					level = (data & OCW2_LMask) >> OCW2_LPos;
				}
				else {
					level = findHighestPriorityRequest(m_isr);
				}

#if defined(TRACE_INTERRUPTS)
				printf("PIC: EOI %sspecific, level %u\n", (data & OCW2_SL) ? "" : "non-", level);
#endif

				m_isr &= ~(1 << level);
				irrUpdated();
			}
		}

		break;

	case RegisterData:
		switch (m_state) {
		case State::Idle:
			// OCW1
			m_mask = data;
			irrUpdated();
			break;

		case State::WaitingForICW2:
			m_icw2 = data;

			if (!(m_icw1 & ICW1_SNGL)) {
				m_state = State::WaitingForICW3;
			}
			else {
				if (m_icw1 & ICW1_IC4) {
					m_state = State::WaitingForICW4;
				}
				else {
					m_state = State::Idle;
				}
			}

			break;

		case State::WaitingForICW3:
			m_icw3 = data;

			if (m_icw1 & ICW1_IC4) {
				m_state = State::WaitingForICW4;
			}
			else {
				m_state = State::Idle;
			}
			break;

		case State::WaitingForICW4:
			m_icw4 = data;
			assert(!(m_icw4 & ICW4_SFNM));

			m_state = State::Idle;
		}

		break;
	}

}

uint8_t PIC::read8(uint64_t address, uint8_t mask) {
	(void)mask;
	
	std::unique_lock locker(m_picMutex);

#if defined(TRACE_INTERRUPTS)
	printf("PIC: read: %u\n", static_cast<uint32_t>(address));
#endif

	if(m_state == State::Poll) {
		bool request;
		unsigned int lineNumber = internalInterruptAcknowledge(request);

		m_state = State::Idle;

		return (request << 7) | lineNumber;
	}

	switch (address & 1) {
	case RegisterCommand:
		if (m_ocw3 & OCW3_RIS) {
			return m_isr;
		}
		else {
			return m_irr;
		}

	case RegisterData:
		return m_mask;

	default:
		__assume(0);
	}
}

void PIC::setInterruptLineAsserted(unsigned int line, bool asserted) {
	std::unique_lock locker(m_picMutex);

#if defined(TRACE_INTERRUPTS)
	printf("PIC: line asserted: %u, %u\n", line, static_cast<unsigned int>(asserted));
#endif

	auto lineMask = 1 << line;

	if ((m_icw1 & ICW1_LTIM) || (elcr.value() & (1 << line))) {
		// Level triggered

		if ((m_irr ^ (asserted << line)) & lineMask) {
			m_irr = (m_irr & ~lineMask) | (asserted << line);
			irrUpdated();
		}

	}
	else {
		// Edge triggered

		if (asserted && !(m_edgeSense & lineMask)) {
			m_irr |= lineMask;
			irrUpdated();
		}

		m_edgeSense = (m_edgeSense & ~lineMask) | (asserted << line);
	}
}

void PIC::irrUpdated() {
	auto unserviced = m_irr & ~m_mask & ~m_isr;

#if defined(TRACE_INTERRUPTS)
	printf("PIC: IRR updated, now %02X. Interrupts demanding service: %02X\n", m_irr, unserviced);
#endif

	m_outputInterruptLine->setInterruptAsserted(unserviced != 0);
}

unsigned int PIC::internalInterruptAcknowledge(bool& request) {
	assert(m_icw4 & ICW4_UPM);

	auto unserviced = m_irr & ~m_mask & ~m_isr;
	uint8_t lineNumber;

	if (unserviced == 0) {
#if defined(TRACE_INTERRUPTS)
		printf("PIC: spurious interrupt\n");
#endif
		lineNumber = 7;
		request = false;
	}
	else {
		lineNumber = findHighestPriorityRequest(unserviced);

		if (!((m_icw1 & ICW1_LTIM) || (elcr.value() & (1 << lineNumber)))) {
			m_irr &= ~(1 << lineNumber);
		}

		if (!(m_icw4 & ICW4_AEOI)) {
			m_isr |= 1 << lineNumber;
		}

		irrUpdated();

		request = true;
	}

	return lineNumber;
}

uint8_t PIC::processInterruptAcknowledge() {
	std::unique_lock locker(m_picMutex);

	bool request;
	unsigned int lineNumber = internalInterruptAcknowledge(request);

	uint8_t vector;
	
	if (m_secondaryPICs[lineNumber]) {
		vector = m_secondaryPICs[lineNumber]->processInterruptAcknowledge();
	}
	else {
		vector = (m_icw2 & 0xF8) | lineNumber;
	}

#if defined(TRACE_INTERRUPTS)
	printf("PIC: INTA, vector %02X. IRR is now %02X, ISR is now %02X\n", vector, m_irr, m_isr);
#endif
	
	return vector;
}

unsigned int PIC::findHighestPriorityRequest(uint8_t mask) const {
	for (unsigned int index = 0; index < 6; index++) {
		if (mask & (1 << index))
			return index;
	}

	return 7;
}

PIC::PICInterruptLine::PICInterruptLine(PIC* owner, unsigned int line) : m_owner(owner), m_line(line) {

}

PIC::PICInterruptLine::~PICInterruptLine() = default;

void PIC::PICInterruptLine::setInterruptAsserted(bool interrupt) {
	m_owner->setInterruptLineAsserted(m_line, interrupt);
}