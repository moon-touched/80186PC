#ifndef SDLUI_H
#define SDLUI_H

#include <SDL.h>

#include <memory>
#include <atomic>
#include <mutex>
#include <deque>
#include <array>
#include <vector>

#include "VideoAdapter.h"

class Keyboard;
class Mouse;

class SDLUI final {
public:
	SDLUI();
	~SDLUI();

	SDLUI(const SDLUI& other) = delete;
	SDLUI &operator =(const SDLUI& other) = delete;

	void run();

	inline void setVideoAdapter(VideoAdapter* adapter) {
		m_videoAdapter = adapter;
	}

	inline void setKeyboard(Keyboard* keyboard) {
		m_keyboard = keyboard;
	}

	inline void setMouse(Mouse *mouse) {
		m_mouse = mouse;
	}

private:
	void pushKeyboardEvent(const SDL_KeyboardEvent& ev);
	void updateMouseButton(const SDL_MouseButtonEvent & ev);

	struct SDLWindowDeleter {
		void operator()(SDL_Window* window) const;
	};
	
	struct SDLSurfaceDeleter {
		void operator()(SDL_Surface* surface) const;
	};

	struct SDLPaletteDeleter {
		void operator()(SDL_Palette *palette) const;
	};

	enum {
		PaletteIndexAllBlank = 0,
		PaletteIndexDimOnBlank,
		PaletteIndexBrightOnBlank,
		PaletteIndexBlankOnDim,
		PaletteIndexBlankOnBright,

		PaletteCount
	};

	struct PaletteSpecification {
		uint8_t bgR;
		uint8_t bgG;
		uint8_t bgB;
		uint8_t fgR;
		uint8_t fgG;
		uint8_t fgB;
	};

	void linearizeHerculesVideo(const VideoAdapter::AdapterConfiguration& config, unsigned char *data);

	std::unique_ptr<SDL_Window, SDLWindowDeleter> m_window;
	std::unique_ptr<SDL_Surface, SDLSurfaceDeleter> m_screenSurface;
	std::atomic<VideoAdapter*> m_videoAdapter;
	std::atomic<Keyboard*> m_keyboard;
	std::atomic<Mouse*> m_mouse;
	std::unique_ptr<SDL_Surface, SDLSurfaceDeleter> m_textModeFont;
	std::array<std::unique_ptr<SDL_Palette, SDLPaletteDeleter>, PaletteCount> m_screenPalettes;
	static const PaletteSpecification m_paletteSpecifications[PaletteCount];
	std::unique_ptr<SDL_Surface, SDLSurfaceDeleter> m_graphicsSurface;
	std::vector<uint8_t> m_graphicsSurfaceData;

	//uint32_t m_dirtyVRAMEvent;
	bool m_mouseCaptured;
	/*std::atomic<int16_t> m_mouseX;
	std::atomic<int16_t> m_mouseY;
	std::atomic<uint8_t> m_mouseButtons;
	std::mutex m_keyboardQueueMutex;
	std::deque<uint8_t> m_keyboardQueue;*/
};

#endif
