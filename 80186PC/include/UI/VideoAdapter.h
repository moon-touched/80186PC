#ifndef UI_VIDEO_ADAPTER_H
#define UI_VIDEO_ADAPTER_H

class VideoAdapter {
protected:
	VideoAdapter();
	~VideoAdapter();

public:
	VideoAdapter(const VideoAdapter& other) = delete;
	VideoAdapter &operator =(const VideoAdapter& other) = delete;

	struct AdapterConfiguration {
		bool videoEnabled;
		bool textMode;
		unsigned int widthPixels;
		unsigned int heightPixels;

		// text mode only
		const unsigned char* textModeFramebuffer;
		unsigned int textModeColumns;
		unsigned int textModeRows;
		unsigned int textModeCharacterHeight;
		unsigned int textModeCursorAddress;
		unsigned int textModeFirstCursorLine;
		unsigned int textModeLastCursorLine;
	};

	virtual void acquireAdapterConfiguration(AdapterConfiguration& config) = 0;
};

#endif
