#include <ATA/ATADevice.h>
#include <ATA/ATATypes.h>

ATADevice::ATADevice() :
	m_stopDriveThread(false),
	m_resetRequest(false),
	m_commandRequest(false),
	m_interruptPending(false),
	m_interruptEnabled(false),
	m_resetAsserted(false),
	m_eightBitPIO(false),
	m_status(0x00),
	m_feature(0x00),
	m_error(0x00),
	m_sectorCount(0x00),
	m_sectorNumber(0x00),
	m_cylinderLow(0x00),
	m_cylinderHigh(0x00),
	m_driveHead(0x00),
	m_command(0x00),
	m_host(nullptr),
	m_transferState(TransferState::Idle),
	m_transferPosition(0),
	m_transferSize(0),
	m_driveThread(&ATADevice::driveThread, this) {

	postReset();
}

ATADevice::~ATADevice() {
	stopDriveThread();
}

void ATADevice::stopDriveThread() {
	{
		std::unique_lock<std::mutex> locker(m_driveThreadMutex);
		m_stopDriveThread = true;
		m_driveThreadCondvar.notify_all();
	}

	if(m_driveThread.joinable())
		m_driveThread.join();
}

void ATADevice::write(CS cs, uint8_t address, uint16_t value) {
	std::unique_lock<std::mutex> locker(m_driveThreadMutex);

	switch (cs) {
	case CS::CS0:
		if (m_status & 0x80) {
			fprintf(stderr, "ATADevice(%p): attempted to write register %02X with %04X while the device is busy\n", this, address, value);
			return;
		}
		switch (address) {
		case ATACS0_DATA:
		{
			if (m_transferState != TransferState::PIOWrite) {
				fprintf(stderr, "ATADevice(%p): DATA write outside of PIO write cycle\n", this);
				return;
			}

			if (!m_eightBitPIO) {
				m_transferBuffer[m_transferPosition++] = value & 0xFF;
				m_transferBuffer[m_transferPosition++] = value >> 8;

				updateDRQLocked(m_transferState, false);
			}
			else {
				m_transferBuffer[m_transferPosition++] = value & 0xFF;

				updateDRQLocked(m_transferState, false);
			}

			break;
		}

		case ATACS0_ERROR_FEATURE:
			m_feature = value;
			break;

		case ATACS0_SECTORCOUNT:
			m_sectorCount = value;
			break;

		case ATACS0_SECTORNUMBER:
			m_sectorNumber = value;
			break;

		case ATACS0_CYLINDERLOW:
			m_cylinderLow = value;
			break;

		case ATACS0_CYLINDERHIGH:
			m_cylinderHigh = value;
			break;

		case ATACS0_DRIVEHEAD:
			m_driveHead = value;
			break;

		case ATACS0_STATUS_COMMAND:
			m_command = value;
			postCommandLocked();
			break;

		default:
			__debugbreak();
			break;
		}
		break;

	case CS::CS1:
		switch (address) {
		case ATACS1_ALTERNATESTATUS_DEVICECONTROL:
		{
			auto wasPending = isInterruptRequestedLocked();
			m_interruptEnabled = (value & 0x02) == 0;
			auto pending = isInterruptRequestedLocked();

			if (pending != wasPending) {
				m_host->interruptRequestedChanged(this);
			}

			auto resetAsserted = (value & 0x04) != 0;
			if (!resetAsserted && m_resetAsserted) {
				postResetLocked();
			}

			m_resetAsserted = resetAsserted;
			break;
		}
		default:
			__debugbreak();
			break;
		}
		break;
	}
}

uint16_t ATADevice::read(CS cs, uint8_t address) {
	std::unique_lock<std::mutex> locker(m_driveThreadMutex);

	switch (cs) {
	case CS::CS0:
		if ((m_status & 0x80) && address != ATACS0_STATUS_COMMAND) {
			fprintf(stderr, "ATADevice(%p): attempted to read register %02X while the device is busy\n", this, address);
			address = ATACS0_STATUS_COMMAND;
		}

		switch (address) {
		case ATACS0_DATA:
		{
			if (m_transferState != TransferState::PIORead) {
				fprintf(stderr, "ATADevice(%p): DATA read outside of PIO read cycle\n", this);
				return 0xFFFF;
			}

			if (!m_eightBitPIO) {
				auto byteLow = m_transferBuffer[m_transferPosition++];
				auto byteHigh = m_transferBuffer[m_transferPosition++];

				updateDRQLocked(m_transferState, false);

				return (byteHigh << 8) | byteLow;
			}
			else {
				auto byteLow = m_transferBuffer[m_transferPosition++];

				updateDRQLocked(m_transferState, false);

				return byteLow;
			}

			break;
		}

		case ATACS0_ERROR_FEATURE:
			return m_error;

		case ATACS0_SECTORCOUNT:
			return m_sectorCount;

		case ATACS0_SECTORNUMBER:
			return m_sectorNumber;

		case ATACS0_CYLINDERLOW:
			return m_cylinderLow;

		case ATACS0_CYLINDERHIGH:
			return m_cylinderHigh;

		case ATACS0_DRIVEHEAD:
			return m_driveHead;

		case ATACS0_STATUS_COMMAND:
			clearInterruptLocked();
			return m_status;

		default:
			__debugbreak();
			return 0xFFFF;
		}
		break;

	case CS::CS1:
		switch (address) {
		case ATACS1_ALTERNATESTATUS_DEVICECONTROL:
			return m_status;

		default:
			__debugbreak();
			return 0xFFFF;
		}
		break;

	default:
		return 0xFFFF;
	}
}

void ATADevice::postReset() {
	std::unique_lock<std::mutex> locker(m_driveThreadMutex);

	postResetLocked();
}

void ATADevice::postResetLocked() {
	m_resetRequest = true;
	m_status |= 0x80;

	m_driveThreadCondvar.notify_all();
}

void ATADevice::postCommand() {
	std::unique_lock<std::mutex> locker(m_driveThreadMutex);

	postCommandLocked();
}

void ATADevice::postCommandLocked() {
	m_commandRequest = true;
	m_status |= 0x80;

	m_driveThreadCondvar.notify_all();
}

void ATADevice::setInterruptLocked() {
	if (!m_interruptPending) {
		m_interruptPending = true;

		if(m_interruptEnabled)
			m_host->interruptRequestedChanged(this);
	}
}

void ATADevice::clearInterruptLocked() {
	if (m_interruptPending) {
		m_interruptPending = false;

		if(m_interruptEnabled)
			m_host->interruptRequestedChanged(this);
	}
}

void ATADevice::attachToHost(IATADeviceHost* host) {
	m_host = host;
}

bool ATADevice::isInterruptRequested() const {
	std::unique_lock<std::mutex> locker(m_driveThreadMutex);

	return isInterruptRequestedLocked();
}

bool ATADevice::isInterruptRequestedLocked() const {
	return m_interruptPending && m_interruptEnabled;
}

void ATADevice::driveThread() {
	std::unique_lock<std::mutex> locker(m_driveThreadMutex);
	while (!m_stopDriveThread) {
		m_driveThreadCondvar.wait(locker, [this]() { return m_stopDriveThread || m_resetRequest || m_commandRequest; });

		if (m_resetRequest) {
			locker.unlock();

			resetDevice();

			locker.lock();

			m_resetRequest = false;
			m_status = 0x50; // DRDY, DSC
			m_feature = 0x00;
			m_error = 0x01; // Diagnostic code: no error detected
			m_sectorCount = 0x00;
			m_sectorNumber = 0x00;
			m_cylinderLow = 0;
			m_cylinderHigh = 0x00;
			m_driveHead = 0x00;
			m_command = 0x00;
			m_commandRequest = false;
		}

		if (m_commandRequest) {
			ATACommand command;
			command.feature = m_feature;
			command.sectorCount = m_sectorCount;
			command.sectorNumber = m_sectorNumber;
			command.cylinderLow = m_cylinderLow;
			command.cylinderHigh = m_cylinderHigh;
			command.driveHead = m_driveHead;
			command.command = m_command;

			locker.unlock();

			ATACommandResult result;

			executeCommand(command, result);

			locker.lock();

			m_status = (m_status & 0x08) | (result.status & 0x77);
			m_error = result.error;
			m_commandRequest = false;

			if (!(m_status & 1))
				setInterruptLocked();
		}
	}
}

void ATADevice::pioRead(size_t size) {
	std::unique_lock<std::mutex> locker(m_driveThreadMutex);

	if (size > m_transferBuffer.size()) {
		throw std::logic_error("PIO read transfer is too long");
	}

	m_transferSize = size;
	m_transferPosition = 0;

	updateDRQLocked(TransferState::PIORead, true);
}

void ATADevice::pioWrite(size_t size) {
	std::unique_lock<std::mutex> locker(m_driveThreadMutex);

	if (size > m_transferBuffer.size()) {
		throw std::logic_error("PIO read transfer is too long");
	}

	m_transferSize = size;
	m_transferPosition = 0;

	updateDRQLocked(TransferState::PIOWrite, true);
}

void ATADevice::updateDRQLocked(TransferState state, bool first) {
	//printf("UpdateDRQ: current state %d, new state %d, first transfer %d, position %u, length %u\n",
	//	m_transferState, state, first, m_transferPosition, m_transferSize);
	/*
	if (m_transferPosition == 0) {
		m_status |= 0x08;

		if (!first)
			setInterruptLocked();
	}
	else {
		m_status &= ~0x08;
	}
	*/
	if (m_transferSize > m_transferPosition) {
		m_transferState = state;
		m_status |= 0x08;
		if (!first)
			setInterruptLocked();
	}
	else {
		m_status &= ~0x08;

		bool writeFinished = m_transferState == TransferState::PIOWrite;
		m_transferState = TransferState::Idle;
		printf("PIO transfer complete\n");

		if (writeFinished) {
			pioWriteFinished();
		}
	}
}
