#ifndef ATA_ATA_DEVICE_H
#define ATA_ATA_DEVICE_H

#include <thread>
#include <mutex>
#include <atomic>
#include <array>

#include <ATA/IATADevice.h>

class ATADevice : public IATADevice {
protected:
	ATADevice();
	~ATADevice();
public:

	void write(CS cs, uint8_t address, uint16_t value) override;
	uint16_t read(CS cs, uint8_t address) override;

	void attachToHost(IATADeviceHost* host) override;

	bool isInterruptRequested() const override;

protected:
	struct ATACommand {
		uint8_t feature;
		uint8_t sectorCount;
		uint8_t sectorNumber;
		uint8_t cylinderLow;
		uint8_t cylinderHigh;
		uint8_t driveHead;
		uint8_t command;
	};

	struct ATACommandResult {
		uint8_t status;
		uint8_t error;
	};

	virtual void resetDevice() = 0;
	virtual void executeCommand(const ATACommand& command, ATACommandResult& result) = 0;

	void stopDriveThread();

	void pioRead(size_t size);

	inline uint8_t* transferBuffer() {
		return m_transferBuffer.data();
	}

	inline void set8BitPIO() {
		m_eightBitPIO = true;
	}

	inline void clear8BitPIO() {
		m_eightBitPIO = false;
	}

private:
	enum class TransferState {
		Idle,
		PIORead
	};

	void driveThread();
	void postReset();
	void postResetLocked();
	void postCommand();
	void postCommandLocked();

	void setInterruptLocked();
	void clearInterruptLocked();
	bool isInterruptRequestedLocked() const;
	
	void updateDRQLocked(TransferState state, bool first);

	mutable std::mutex m_driveThreadMutex;
	std::condition_variable m_driveThreadCondvar;
	bool m_stopDriveThread;
	bool m_resetRequest;
	bool m_commandRequest;
	bool m_interruptPending;
	bool m_interruptEnabled;
	bool m_resetAsserted;
	bool m_eightBitPIO;
	uint8_t m_status;
	uint8_t m_feature;
	uint8_t m_error;
	uint8_t m_sectorCount;
	uint8_t m_sectorNumber;
	uint8_t m_cylinderLow;
	uint8_t m_cylinderHigh;
	uint8_t m_driveHead;
	uint8_t m_command;
	IATADeviceHost* m_host;
	std::array<uint8_t, 131072> m_transferBuffer;
	TransferState m_transferState;
	size_t m_transferPosition;
	size_t m_transferSize;
	std::thread m_driveThread;
};

#endif
