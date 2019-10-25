#ifndef HARDWARE_PIC_H
#define HARDWARE_PIC_H

#include <Infrastructure/IAddressRangeHandler.h>
#include <Infrastructure/InterruptLine.h>
#include <Infrastructure/InterruptController.h>

#include <array>
#include <mutex>

class PIC final : public IAddressRangeHandler, public InterruptController {
public:
	class ELCR final : public IAddressRangeHandler {
	public:
		explicit ELCR(PIC *owner);
		~ELCR();

		void write(uint64_t address, unsigned int accessSize, uint64_t data) override;
		uint64_t read(uint64_t address, unsigned int accessSize) override;

		inline uint8_t value() const {
			return m_value;
		}

	private:
		void write8(uint64_t address, uint8_t mask, uint8_t data);
		uint8_t read8(uint64_t address, uint8_t mask);

		PIC* m_owner;
		uint8_t m_value;
	};

	PIC();
	~PIC();

	ELCR elcr;

	virtual void write(uint64_t address, unsigned int accessSize, uint64_t data) override;
	virtual uint64_t read(uint64_t address, unsigned int accessSize) override;

	inline InterruptLine* line(unsigned int line) {
		return &m_interruptLines[line];
	}

	inline InterruptLine* outputInterruptLine() const {
		return m_outputInterruptLine;
	}

	inline void setOutputInterruptLine(InterruptLine* outputInterruptLine) {
		m_outputInterruptLine = outputInterruptLine;
	}

	inline PIC* secondaryPIC(unsigned int line) const {
		return m_secondaryPICs[line];
	}

	inline void setSecondaryPIC(unsigned int line, PIC *pic) {
		m_secondaryPICs[line] = pic;
	}

	uint8_t processInterruptAcknowledge() override;

private:
	class PICInterruptLine final : public InterruptLine {
	public:
		PICInterruptLine(PIC* owner, unsigned int line);
		~PICInterruptLine();

		void setInterruptAsserted(bool interrupt) override;

	private:
		PIC* m_owner;
		unsigned int m_line;
	};

	void write8(uint64_t address, uint8_t mask, uint8_t data);
	uint8_t read8(uint64_t address, uint8_t mask);

	void setInterruptLineAsserted(unsigned int line, bool asserted);

	void irrUpdated();

	unsigned int internalInterruptAcknowledge(bool& request);

	unsigned int findHighestPriorityRequest(uint8_t mask) const;

	enum class State {
		Idle,
		WaitingForICW2,
		WaitingForICW3,
		WaitingForICW4,
		Poll
	};

	static constexpr unsigned int RegisterCommand = 0;
	static constexpr unsigned int RegisterData = 1;

	/*
	 * ICW1
	 */
	static constexpr uint8_t ICW1_IC4 = 1 << 0;
	static constexpr uint8_t ICW1_SNGL = 1 << 1;
	static constexpr uint8_t ICW1_ADI = 1 << 2;
	static constexpr uint8_t ICW1_LTIM = 1 << 3;
	static constexpr uint8_t CommandICW1 = 1 << 4;
	static constexpr uint8_t ICW1_APos = 5;
	static constexpr uint8_t ICW1_AMask = 7 << 5;

	/*
	 * ICW4
	 */
	static constexpr uint8_t ICW4_UPM = 1 << 0;
	static constexpr uint8_t ICW4_AEOI = 1 << 1;
	static constexpr uint8_t ICW4_MS = 1 << 2;
	static constexpr uint8_t ICW4_BUF = 1 << 3;
	static constexpr uint8_t ICW4_SFNM = 1 << 4;

	/*
	 * OCW2
	 */
	static constexpr uint8_t OCW2_LPos = 0;
	static constexpr uint8_t OCW2_LMask = 7 << 0;
	static constexpr uint8_t OCW2_EOI = 1 << 5;
	static constexpr uint8_t OCW2_SL = 1 << 6;
	static constexpr uint8_t OCW2_R = 1 << 7;

	/*
	 * OCW3
	 */
	static constexpr uint8_t OCW3_RIS = 1 << 0;
	static constexpr uint8_t OCW3_RR = 1 << 1;
	static constexpr uint8_t OCW3_POLL = 1 << 2;
	static constexpr uint8_t CommandOCW3 = 1 << 3;
	static constexpr uint8_t OCW3_SMM = 1 << 5;
	static constexpr uint8_t OCW3_ESMM = 1 << 6;

	std::array<PICInterruptLine, 8> m_interruptLines;
	InterruptLine* m_outputInterruptLine;
	std::array<PIC*, 8> m_secondaryPICs;
	State m_state;
	uint8_t m_edgeSense;
	uint8_t m_mask;
	uint8_t m_icw1;
	uint8_t m_icw2;
	uint8_t m_icw3;
	uint8_t m_icw4;
	uint8_t m_ocw3;
	uint8_t m_isr;
	uint8_t m_irr;
	std::recursive_mutex m_picMutex;
};

#endif
