#define INITGUID
#include <ATA/ATAHardDisk.h>

#include <ATA/ATATypes.h>

#include <Windows.h>
#include <virtdisk.h>
#define _NTSCSI_USER_MODE_
#include <scsi.h>
#undef _NTSCSI_USER_MODE_

#include <comdef.h>

const char ATAHardDisk::m_serialNumber[20]{
	'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
};

const char ATAHardDisk::m_firmwareRevision[8]{
	'V', 'E', 'R', ' ', '1', '.', '0', ' '
};

const char ATAHardDisk::m_modelNumber[40]{
	'E', 'M', 'U', 'L', 'A', 'T', 'E', 'D', ' ', 'A' ,'T', 'A', ' ', 'H', 'A', 'R', 'D', ' ', 'D', 'I', 'S', 'K',
	' ', ' ', ' ', ' ', ' ', ' ', ' ',
	' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' '
};

ATAHardDisk::ATAHardDisk(const std::filesystem::path &diskImage) : m_currentAddress(0), m_currentCommand(0), m_sectorsRemaining(0) {
	VIRTUAL_STORAGE_TYPE storage;
	storage.DeviceId = VIRTUAL_STORAGE_TYPE_DEVICE_UNKNOWN;
	storage.VendorId = VIRTUAL_STORAGE_TYPE_VENDOR_UNKNOWN;
	HANDLE rawHandle;
	auto result = OpenVirtualDisk(
		&storage,
		diskImage.wstring().c_str(),
		VIRTUAL_DISK_ACCESS_GET_INFO | VIRTUAL_DISK_ACCESS_READ | VIRTUAL_DISK_ACCESS_WRITABLE | VIRTUAL_DISK_ACCESS_METAOPS,
		OPEN_VIRTUAL_DISK_FLAG_NONE,
		nullptr,
		&rawHandle);
	if (result != ERROR_SUCCESS) {
		_com_raise_error(HRESULT_FROM_NT(result));
	}
	m_disk.reset(rawHandle);

	ATTACH_VIRTUAL_DISK_PARAMETERS attachParameters;
	ZeroMemory(&attachParameters, sizeof(attachParameters));

	attachParameters.Version = ATTACH_VIRTUAL_DISK_VERSION_1;
	result = AttachVirtualDisk(
		m_disk.get(),
		nullptr,
		ATTACH_VIRTUAL_DISK_FLAG_NO_DRIVE_LETTER | ATTACH_VIRTUAL_DISK_FLAG_NO_LOCAL_HOST,
		0,
		&attachParameters,
		nullptr);

	GET_VIRTUAL_DISK_INFO info;
	ULONG infoSize = sizeof(info);
	ULONG sizeUsed;

	info.Version = GET_VIRTUAL_DISK_INFO_SIZE;

	result = GetVirtualDiskInformation(
		m_disk.get(),
		&infoSize,
		&info,
		&sizeUsed);
	if (result != ERROR_SUCCESS) {
		_com_raise_error(HRESULT_FROM_NT(result));
	}
	
	memset(&m_identify, 0, sizeof(m_identify));

	auto sectors = info.Size.VirtualSize / 512;

	printf("ATAHardDisk: presenting as %llu 512-byte blocks\n", sectors);

	m_identify.flags =
		(1 << 2) | // Soft sectored
		(1 << 3) | // Not MFM encoded
		(1 << 6) | // Fixed drive
		(1 << 10); // Transfer rate > 10 MBit/s

	auto cylinders = sectors / (16 * 63);
	if (cylinders > 16383)
		cylinders = 16383;
	else if (cylinders < 2)
		cylinders = 2;
	auto heads = 16;
	auto sectorsPerTrack = 63;

	m_identify.cylinders = cylinders;
	m_identify.heads = heads;
	m_identify.sectorsPerTrack = sectorsPerTrack;
	m_identify.bytesPerSector = 512;
	m_identify.bytesPerTrack = 512 * m_identify.sectorsPerTrack;
	memcpy(m_identify.serialNumber, m_serialNumber, sizeof(m_serialNumber));
	memcpy(m_identify.firmwareRevision, m_firmwareRevision, sizeof(m_firmwareRevision));
	memcpy(m_identify.modelNumber, m_modelNumber, sizeof(m_modelNumber));
	m_identify.readWriteMultipleMaxSectors = 128;
	m_identify.flags2 =
		(1 << 8) | // DMA supported
		(1 << 9); // LBA supported
	m_identify.totalSectors = sectors;
	m_identify.singleWordDMAStatus = 1 << 0;
	m_identify.multiWordDMAStatus = 1 << 0;
}

ATAHardDisk::~ATAHardDisk() {
	stopDriveThread();
}

void ATAHardDisk::resetDevice() {
	printf("ATAHardDisk: reset\n");

	clear8BitPIO();

	m_identify.currentTranslationValid = 0;
	m_identify.multipleSectorConfiguration = 0;

}

void ATAHardDisk::executeCommand(const ATACommand& command, ATACommandResult& result) {
	printf("ATAHardDisk: executing command %02X\n", command.command);

	result.status = 0x50;
	result.error = 0;

	auto cmd = command.command;

	if ((cmd & 0xF0) == ATACMD_SEEK || (cmd & 0xF0) == ATACMD_RECALIBRATE) {
		cmd &= 0xF0;
	}

	switch (cmd) {
	case ATACMD_RECALIBRATE:
	case ATACMD_SEEK:
		break;

	case ATACMD_INITIALIZE_DRIVE_PARAMETERS:
		m_identify.currentTranslationValid = 1;
		m_identify.currentSectorsPerTrack = command.sectorCount;
		m_identify.currentHeads = (command.driveHead & 15) + 1;
		m_identify.currentCylinders = m_identify.totalSectors / (m_identify.currentSectorsPerTrack * m_identify.currentHeads);
		m_identify.currentCapacitySectors = m_identify.currentSectorsPerTrack * m_identify.currentHeads * m_identify.currentCylinders;

		printf("ATAHardDisk: driver parameters initialized: C %u, H %u, S %u, maximum addressable: %u sectors, total capacity: %u sectors\n",
			m_identify.currentSectorsPerTrack,
			m_identify.currentHeads,
			m_identify.currentCylinders,
			m_identify.currentCapacitySectors,
			m_identify.totalSectors);

		break;
		
	case ATACMD_READ_SECTORS:
	case ATACMD_READ_SECTORS_NO_RETRY:
	case ATACMD_READ_MULTIPLE:
	case ATACMD_WRITE_MULTIPLY:
	case ATACMD_WRITE_SECTORS:
	case ATACMD_WRITE_SECTORS_NO_RETRY:
		m_currentAddress = translateAddress(command);
		m_currentCommand = command.command;
		m_sectorsRemaining = command.sectorCount;
		if (m_sectorsRemaining == 0)
			m_sectorsRemaining = 256;

		if (m_currentAddress + m_sectorsRemaining > m_identify.totalSectors) {
			m_sectorsRemaining = 0;
			result.error = 0x04; // Aborted
		}
		else {

			pioNext();
		}

		break;

	case ATACMD_SET_MULTIPLE_MODE:
		if (command.sectorCount > m_identify.readWriteMultipleMaxSectors) {
			result.error = 0x04; // Aborted
		}
		else {
			m_identify.multipleSectorConfiguration = command.sectorCount;
		}
		break;

	case ATACMD_IDENTIFY_DRIVE:
		memcpy(transferBuffer(), &m_identify, sizeof(m_identify));
		pioRead(sizeof(m_identify));

		break;

	case ATACMD_SET_FEATURES:
		switch (command.feature) {
		case ATAFEATURE_ENABLE_8BIT_TRANSFERS:
			set8BitPIO();
			break;

		case ATAFEATURE_DISABLE_8BIT_TRANSFERS:
			clear8BitPIO();
			break;

		default:
			fprintf(stderr, "ATAHardDisk: ATACMD_SET_FEATURES: unsupported feature: %02X\n", command.feature);
			result.error = 0x04; // Aborted
			break;
		}
		break;

	default:
		fprintf(stderr, "ATAHardDisk: unsupported command %02X\n", command.command);

		result.error = 0x04; // Aborted
		break;
	}

	if (result.error != 0) {
		result.status |= 0x01;
	}
}

uint64_t ATAHardDisk::translateAddress(const ATACommand& command) const {
	auto c = static_cast<uint16_t>(command.cylinderLow) | (static_cast<uint16_t>(command.cylinderHigh) << 8);
	auto h = (command.driveHead & 0x0F);
	auto s = command.sectorNumber;

	if (command.driveHead & 0x40) {
		// LBA

		return (c << 8) | s;
	}
	else {
		// CHS

		if (m_identify.currentTranslationValid) {
			return (((c * m_identify.currentHeads) + h) * m_identify.currentSectorsPerTrack) + s - 1;
		}
		else {
			return (((c * m_identify.heads) + h) * m_identify.sectorsPerTrack) + s - 1;
		}
	}
}

void ATAHardDisk::pioNext() {
	printf("ATAHardDisk: current address: %llu, current command: %02X, sectors remaining: %u\n",
		m_currentAddress, m_currentCommand, m_sectorsRemaining);

	if (m_sectorsRemaining == 0) {
		printf("ATAHardDisk: operation done\n");
		return;
	}

	unsigned int sectorsInThisChunk = 1;
	if ((m_currentCommand == ATACMD_READ_MULTIPLE || m_currentCommand == ATACMD_WRITE_MULTIPLY) && m_identify.multipleSectorConfiguration != 0) {
		sectorsInThisChunk = m_identify.multipleSectorConfiguration;
	}

	sectorsInThisChunk = std::min<unsigned int>(sectorsInThisChunk, m_sectorsRemaining);

	if (m_currentCommand == ATACMD_READ_MULTIPLE || m_currentCommand == ATACMD_READ_SECTORS || m_currentCommand == ATACMD_READ_SECTORS_NO_RETRY) {
		auto bytesToRead = sectorsInThisChunk << 9;

		OVERLAPPED io;
		ZeroMemory(&io, sizeof(io));
		uint64_t offset = m_currentAddress << 9;

		io.OffsetHigh = static_cast<uint32_t>(offset >> 32);
		io.Offset = static_cast<uint32_t>(offset);

		DWORD bytesRead;

		DWORD result = ERROR_SUCCESS;
		if (!ReadFile(m_disk.get(), transferBuffer(), bytesToRead, &bytesRead, &io)) {
			result = GetLastError();
		}

		if (result == ERROR_IO_PENDING) {
			if (GetOverlappedResult(m_disk.get(), &io, &bytesRead, TRUE))
				result = ERROR_SUCCESS;
			else
				result = GetLastError();
		}

		if(result != ERROR_SUCCESS)
			_com_raise_error(HRESULT_FROM_WIN32(result));

		if (bytesRead != bytesToRead)
			throw std::logic_error("short read");

		m_currentAddress += sectorsInThisChunk;
		m_sectorsRemaining -= sectorsInThisChunk;

		pioRead(bytesToRead);

		if(m_sectorsRemaining != 0)
			__debugbreak();
	}
	else {
		__debugbreak();
	}

}