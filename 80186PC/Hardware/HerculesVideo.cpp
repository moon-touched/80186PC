#include <Hardware/HerculesVideo.h>
#include <Utils/AccessSizeUtils.h>

#include <stdio.h>

#include <chrono>

HerculesVideo::HerculesVideo() : m_mode(0), m_graphicsEnable(0), m_crtcAddress(0), m_framebuffer(nullptr) {
	for (auto& reg : m_crtcRegisters) {
		reg = 0;
	}
}

HerculesVideo::~HerculesVideo() = default;

void HerculesVideo::write(uint64_t address, unsigned int accessSize, uint64_t data) {
	splitWriteAccess(address, accessSize, data, &HerculesVideo::write8, this);
}

uint64_t HerculesVideo::read(uint64_t address, unsigned int accessSize) {
	return splitReadAccess(address, accessSize, &HerculesVideo::read8, this);
}

uint8_t HerculesVideo::read8(uint64_t address, uint8_t mask) {
	(void)mask;

	address &= 15;

	if (address & 8) {
		switch (address & 7) {
		case 0:
		{
			std::shared_lock<std::shared_mutex> locker(m_configurationMutex);
			return m_mode;
		}

		case 1:
			printf("HGC: color select read\n");
			break;

		case 2:
		{
			uint8_t status = 0xF0;

			// TODO: bit 3 - video
			// TODO: bit 0 - retrace

			if (hblank()) {
				status |= 1 << 0;
			}

			return status;
		}

		// LPT registers
		case 4:
		case 5:
		case 6:
			break;

		default:
			__debugbreak();
			break;
		}
	}
	else if (address & 1) {
		printf("HGC: CRTC read: [%02X]\n", m_crtcAddress);
		if (m_crtcAddress >= m_crtcRegisters.size())
			throw std::logic_error("CRTC access is out of range");

		{
			std::shared_lock<std::shared_mutex> locker(m_configurationMutex);
			return m_crtcRegisters[m_crtcAddress];
		}
	}
	else {
		return m_crtcAddress;
	}

	return 0xFF;
}

void HerculesVideo::write8(uint64_t address, uint8_t mask, uint8_t data) {
	(void)mask;

	address &= 15;

	if (address & 8) {
		switch (address & 7) {
		case 0:
		{
			std::unique_lock<std::shared_mutex> locker(m_configurationMutex);

			m_mode = data;
		}

			printf("HGC: mode %02X\n", data);
			break;

		case 1:
			printf("HGC: color select: %02X\n", data);
			break;

		case 2:
			// status register
			break;

		// LPT registers
		case 4:
		case 5:
		case 6:
			break;

		case 7:
		{
			std::unique_lock<std::shared_mutex> locker(m_configurationMutex);
			m_graphicsEnable = data;
		}
			break;

		default:
			__debugbreak();
			break;
		}
	}
	else if (address & 1) {
		if(m_crtcAddress != CRTCCursorAddressLSB && m_crtcAddress != CRTCCursorAddressMSB)
			printf("HGC: CRTC write: [%02X] = %02X\n", m_crtcAddress, data);
		if (m_crtcAddress >= m_crtcRegisters.size())
			throw std::logic_error("CRTC access is out of range");

		{
			std::unique_lock<std::shared_mutex> locker(m_configurationMutex);
			m_crtcRegisters[m_crtcAddress] = data;
		}
	}
	else {
		m_crtcAddress = data;
	}
}

bool HerculesVideo::hblank() const {
	std::shared_lock<std::shared_mutex> locker(m_configurationMutex);

	unsigned int pixelsPerCharacter = 9;

	auto nanosecondsPerLine = static_cast<uint64_t>(1e9f / (MDACrystal / pixelsPerCharacter / (m_crtcRegisters[CRTCHTotal] + 1)));
	auto visibleDisplayNanoseconds = static_cast<uint64_t>(1e9f / (MDACrystal / pixelsPerCharacter / (m_crtcRegisters[CRTCHDisplay])));

	return (std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now().time_since_epoch()).count() % nanosecondsPerLine) >= visibleDisplayNanoseconds;
}

void HerculesVideo::acquireAdapterConfiguration(AdapterConfiguration& config) {
	std::shared_lock<std::shared_mutex> locker(m_configurationMutex);

	uint16_t startAddress =
		(static_cast<uint16_t>(m_crtcRegisters[CRTCStartAddressMSB]) << 8) |
		(static_cast<uint16_t>(m_crtcRegisters[CRTCStartAddressLSB]));

	uint16_t cursorAddress =
		(static_cast<uint16_t>(m_crtcRegisters[CRTCCursorAddressMSB]) << 8) |
		(static_cast<uint16_t>(m_crtcRegisters[CRTCCursorAddressLSB]));

	config.videoEnabled = (m_mode & (1 << 3)) != 0;
	config.textMode = (m_mode & (1 << 1)) == 0 || (m_graphicsEnable & (1 << 0)) == 0;
	if (config.textMode) {
		config.widthPixels = m_crtcRegisters[CRTCHDisplay] * 9;
	}
	else {
		config.widthPixels = m_crtcRegisters[CRTCHDisplay] * 16;
	}

	config.heightPixels = m_crtcRegisters[CRTCVDisplay] * (m_crtcRegisters[CRTCMaxScanLine] + 1);

	config.textModeColumns = 80;
	config.textModeRows = 25;
	config.textModeFramebuffer = m_framebuffer + startAddress * 2;
	config.textModeCharacterHeight = m_crtcRegisters[CRTCMaxScanLine] + 1;
	config.textModeCursorAddress = (cursorAddress - startAddress) * 2;
	config.textModeFirstCursorLine = m_crtcRegisters[CRTCCursorStart];
	config.textModeLastCursorLine = m_crtcRegisters[CRTCCursorEnd];
	if (m_mode & (1 << 7))
		config.graphicsModeFramebuffer = m_framebuffer + 0x08000 + startAddress * 2;
	else
		config.graphicsModeFramebuffer = m_framebuffer + startAddress * 2;
}
