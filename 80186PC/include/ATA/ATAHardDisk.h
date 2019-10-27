#ifndef ATA_HARD_DISK_H
#define ATA_HARD_DISK_H

#include <ATA/ATADevice.h>

#include <Utils/WindowsObjectTypes.h>

#include <filesystem>

class ATAHardDisk final : public ATADevice {
public:
	explicit ATAHardDisk(const std::filesystem::path& diskImage);
	~ATAHardDisk();

protected:
	void resetDevice() override;
	void executeCommand(const ATACommand& command, ATACommandResult& result) override;

private:
#pragma pack(push, 1)
	struct IdentifyDriveResponse {
		uint16_t flags;
		uint16_t cylinders;
		uint16_t reserved1;
		uint16_t heads;
		uint16_t bytesPerTrack;
		uint16_t bytesPerSector;
		uint16_t sectorsPerTrack;
		uint16_t reserved3[3];
		char serialNumber[20];
		uint16_t bufferType;
		uint16_t bufferSize;
		uint16_t eccBytes;
		char firmwareRevision[8];
		char modelNumber[40];
		uint16_t readWriteMultipleMaxSectors;
		uint16_t doublewordSupported;
		uint16_t flags2;
		uint16_t reserved5;
		uint16_t pioTimingMode;
		uint16_t dmaTimingMode;
		uint16_t currentTranslationValid;
		uint16_t currentCylinders;
		uint16_t currentHeads;
		uint16_t currentSectorsPerTrack;
		uint32_t currentCapacitySectors;
		uint16_t multipleSectorConfiguration;
		uint32_t totalSectors;
		uint16_t singleWordDMAStatus;
		uint16_t multiWordDMAStatus;
		uint16_t reserved6[192];
	};
#pragma pack(pop)
	static_assert(sizeof(IdentifyDriveResponse) == 512, "Unexpected length of IdentifyDriveResponse");

	uint64_t translateAddress(const ATACommand& command) const;

	void pioNext();

	WindowsHandle m_disk;
	IdentifyDriveResponse m_identify;
	static const char m_serialNumber[20];
	static const char m_firmwareRevision[8];
	static const char m_modelNumber[40];

	uint64_t m_currentAddress;
	uint8_t m_currentCommand;
	uint32_t m_sectorsRemaining;
};

#endif
