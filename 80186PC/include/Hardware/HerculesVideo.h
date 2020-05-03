#ifndef HERCULES_VIDEO_H
#define HERCULES_VIDEO_H

#include <Infrastructure/IAddressRangeHandler.h>
#include <UI/VideoAdapter.h>

#include <array>
#include <shared_mutex>

class HerculesVideo final : public IAddressRangeHandler, public VideoAdapter {
public:
	HerculesVideo();
	~HerculesVideo();

	void write(uint64_t address, unsigned int accessSize, uint64_t data) override;
	uint64_t read(uint64_t address, unsigned int accessSize) override;

	void acquireAdapterConfiguration(AdapterConfiguration& config) override;

	inline void setFramebuffer(unsigned char* framebuffer) {
		m_framebuffer = framebuffer;
	}

private:
	uint8_t read8(uint64_t address, uint8_t mask);
	void write8(uint64_t address, uint8_t mask, uint8_t data);

	static constexpr float MDACrystal = 16.257e6f;

	bool hblank() const;

	enum {
		CRTCHTotal			 = 0x00,
		CRTCHDisplay		 = 0x01,
		CRTCVDisplay		 = 0x06,
		CRTCMaxScanLine		 = 0x09,
		CRTCCursorStart      = 0x0A,
		CRTCCursorEnd        = 0x0B,
		CRTCStartAddressMSB	 = 0x0C,
		CRTCStartAddressLSB  = 0x0D,
		CRTCCursorAddressMSB = 0x0E,
		CRTCCursorAddressLSB = 0x0F,
	};

	mutable std::shared_mutex m_configurationMutex;
	uint8_t m_mode;
	uint8_t m_crtcAddress;
	uint8_t m_graphicsEnable;
	std::array<uint8_t, 18> m_crtcRegisters;
	unsigned char* m_framebuffer;
};

#endif
