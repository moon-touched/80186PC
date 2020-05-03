#include <UI/SDLUI.h>
#include <UI/VideoAdapter.h>
#include <UI/Keyboard.h>

#include <SDL.h>

#include <stdexcept>
#include <string>
#include <unordered_map>

const SDLUI::PaletteSpecification SDLUI::m_paletteSpecifications[PaletteCount]{
	// PaletteIndexAllBlank
	{ 0x00, 0x00, 0x00,   0x00, 0x00, 0x00 },
	// PaletteIndexDimOnBlank
	{ 0x00, 0x00, 0x00,   0xBF, 0x89, 0x00 },
	// PaletteIndexBrightOnBlank
	{ 0x00, 0x00, 0x00,   0xFF, 0xB7, 0x00 },
	// PaletteIndexBlankOnDim
	{ 0xBF, 0x89, 0x00,   0x00, 0x00, 0x00 },
	// PaletteIndexBlankOnBright
	{ 0xFF, 0xB7, 0x0,    0x00, 0x00, 0x00 }
};

void SDLUI::SDLWindowDeleter::operator()(SDL_Window* window) const {
	SDL_DestroyWindow(window);
}

void SDLUI::SDLSurfaceDeleter::operator()(SDL_Surface *surface) const {
	SDL_FreeSurface(surface);
}

void SDLUI::SDLPaletteDeleter::operator()(SDL_Palette* palette) const {
	SDL_FreePalette(palette);
}

SDLUI::SDLUI() : m_videoAdapter(nullptr), m_keyboard(nullptr) {
	auto result = SDL_InitSubSystem(SDL_INIT_VIDEO | SDL_INIT_EVENTS);
	if (result < 0) {
		throw std::runtime_error("SDL_InitSubSystem failed: " + std::string(SDL_GetError()));
	}
	   	
	m_window.reset(SDL_CreateWindow("PC XT", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 1024, 1024, SDL_WINDOW_ALLOW_HIGHDPI));
	if (!m_window) {
		throw std::runtime_error("SDL_CreateWindow failed: " + std::string(SDL_GetError()));;
	}

	for (size_t index = 0; index < sizeof(m_paletteSpecifications) / sizeof(m_paletteSpecifications[0]); index++) {
		const auto& spec = m_paletteSpecifications[index];
		auto& palette = m_screenPalettes[index];
		palette.reset(SDL_AllocPalette(2));
		if (!palette) {
			throw std::runtime_error("SDL_AllocPalette failed: " + std::string(SDL_GetError()));;
		}

		SDL_Color colors[2]{
			{ spec.bgR, spec.bgG, spec.bgB, 0xFF }, // P3 (602 nm)
			{ spec.fgR, spec.fgG, spec.fgB, 0xFF }
		};

		SDL_SetPaletteColors(palette.get(), colors, 0, 2);
	}

	auto font = SDL_LoadBMP("C:\\projects\\80186PC\\bios\\font.bmp");
	if (!font) {
		throw std::runtime_error("SDL_LoadBMP failed: " + std::string(SDL_GetError()));;
	}
	m_textModeFont.reset(font);
}

SDLUI::~SDLUI() {
	SDL_Quit();
}
/*
void SDLUI::attachRAM(VideoRAM* vram, const void* base, size_t size) {
	if (size != 128 * 1024)
		throw std::logic_error("expected exactly 128 KiB of VRAM");
	
	m_vram = vram;

	m_screenSurface.reset(SDL_CreateRGBSurfaceWithFormatFrom(
		const_cast<void *>(base), 1024, 1024, 1, 128, SDL_PIXELFORMAT_INDEX1LSB
	));

	if (!m_screenSurface) {
		throw std::runtime_error("SDL_CreateRGBSurfaceWithFormatFrom failed: " + std::string(SDL_GetError()));
	}

	SDL_SetSurfacePalette(m_screenSurface.get(), m_screenPalette.get());
}

void SDLUI::ramDirty(VideoRAM *vram) {
	(void)vram;

	SDL_Event event;
	SDL_zero(event);
	event.type = m_dirtyVRAMEvent;
	SDL_PushEvent(&event);
}

void SDLUI::fetchState(uint16_t& x, uint16_t& y, uint8_t& buttons) {
	x = m_mouseX.load();
	y = m_mouseY.load();

	buttons = m_mouseButtons.load();
}
*/
void SDLUI::run() {
	SDL_Event ev;

	auto startOfFrame = std::chrono::steady_clock::now();
	auto frameDuration = std::chrono::nanoseconds(16666666); // 60 Hz

	while(true) {
		while(SDL_PollEvent(&ev)) {
			//printf("SDL event type: %u\n", ev.type);

			switch(ev.type) {
			case SDL_QUIT:
				return;
#if 0
			case SDL_MOUSEMOTION:
				if (m_mouseCaptured) {
					m_mouseX.fetch_add(ev.motion.xrel);
					m_mouseY.fetch_add(ev.motion.yrel);
				}
				break;

			case SDL_MOUSEBUTTONDOWN:
				if (!m_mouseCaptured) {
					if (SDL_SetRelativeMouseMode(SDL_TRUE) >= 0) {
						m_mouseCaptured = true;
					}
				} else { 
					updateMouseButton(ev.button);
					//m_mouseButtons.fetch_or(1 << ev.button.button);
				}
				break;


			case SDL_MOUSEBUTTONUP:
				if (m_mouseCaptured) {
					updateMouseButton(ev.button);
					//m_mouseButtons.fetch_and(~(1 << ev.button.button));
				}
					
				break;

#endif
			case SDL_KEYDOWN:
#if 0
				if (ev.key.keysym.scancode == SDL_SCANCODE_LALT && m_mouseCaptured) {
					m_mouseCaptured = false;
					m_mouseButtons = 0;

					SDL_SetRelativeMouseMode(SDL_FALSE);
				}
#endif
				pushKeyboardEvent(ev.key);
				break;

			case SDL_KEYUP:
				pushKeyboardEvent(ev.key);
				break;
			}
		}

#if 0
		if (m_vram->markClean()) {

			auto surface = SDL_GetWindowSurface(m_window.get());
			
			SDL_BlitSurface(m_screenSurface.get(), nullptr, surface, nullptr);
			
			SDL_UpdateWindowSurface(m_window.get());
		}
#endif

		auto video = m_videoAdapter.load();
		if (video) {
			VideoAdapter::AdapterConfiguration config;
			video->acquireAdapterConfiguration(config);
			if (config.videoEnabled) {
				int currentWidth, currentHeight;

				SDL_GetWindowSize(m_window.get(), &currentWidth, &currentHeight);

				if (currentWidth != config.widthPixels || currentHeight != config.heightPixels) {
					printf("SDLUI: resizing window from %dx%d to %ux%u\n",
						currentWidth, currentHeight, config.widthPixels, config.heightPixels);

					SDL_SetWindowSize(m_window.get(), config.widthPixels, config.heightPixels);
				}

				auto surface = SDL_GetWindowSurface(m_window.get());

				if (config.textMode) {
					auto begin = config.textModeFramebuffer;
					auto ptr = config.textModeFramebuffer;
					
					for (unsigned int row = 0; row < config.textModeRows; row++) {
						for (unsigned int column = 0; column < config.textModeColumns; column++) {
							auto ch = *ptr++;
							auto attribute = *ptr++;

							auto x = column * 9;
							auto y = row * config.textModeCharacterHeight;

							unsigned int palette;

							if ((attribute & 0x77) == 0x00) {
								// all black/blank

								palette = PaletteIndexAllBlank;
							}
							else if ((attribute & 0x77) == 0x70) {
								// inverse video

								if ((attribute & 0x80)) {
									// bright background

									palette = PaletteIndexBlankOnBright;
								}
								else {
									palette = PaletteIndexBlankOnDim;
								}
							}
							else if (attribute & 0x08) {
								palette = PaletteIndexBrightOnBlank;
							}
							else {
								palette = PaletteIndexDimOnBlank;
							}

							SDL_SetSurfacePalette(m_textModeFont.get(), m_screenPalettes[palette].get());

							SDL_Rect src, dst;

							src.x = 0;
							src.y = config.textModeCharacterHeight * ch;
							src.w = 8;
							src.h = config.textModeCharacterHeight;

							dst.x = x;
							dst.y = y;
							dst.w = 8;
							dst.h = config.textModeCharacterHeight;

							SDL_BlitSurface(
								m_textModeFont.get(),
								&src,
								surface,
								&dst
							);

							src.x = 7;
							src.w = 1;
							dst.x = x + 8;
							dst.w = 1;

							if (ch >= 0xC0 && ch <= 0xDF) {
								SDL_BlitSurface(
									m_textModeFont.get(),
									&src,
									surface,
									&dst
								);
							}
							else {
								auto bg = SDL_MapRGB(surface->format,
									m_screenPalettes[palette]->colors[0].r,
									m_screenPalettes[palette]->colors[0].g,
									m_screenPalettes[palette]->colors[0].b);

								SDL_FillRect(
									surface,
									&dst,
									bg
								);
							}

							auto fg = SDL_MapRGB(surface->format,
								m_screenPalettes[palette]->colors[1].r,
								m_screenPalettes[palette]->colors[1].g,
								m_screenPalettes[palette]->colors[1].b);

							if ((attribute & 0x07) == 0x01) {
								// underlined

								dst.x = x;
								dst.y = y + config.textModeCharacterHeight - 2;
								dst.w = 9;
								dst.h = 1;

								SDL_FillRect(
									surface,
									&dst,
									fg
								);
							}

							if ((config.textModeCursorAddress + 2) == (ptr - begin) && config.textModeFirstCursorLine <= config.textModeLastCursorLine) {
								dst.x = x;
								dst.y = y + config.textModeFirstCursorLine;
								dst.w = 9;
								dst.h = config.textModeLastCursorLine - config.textModeFirstCursorLine + 1;

								SDL_FillRect(
									surface,
									&dst,
									fg
								);
							}
						}
					}
				}
				else {

				}

				SDL_UpdateWindowSurface(m_window.get());
			}
		}

		auto endOfFrame = std::chrono::steady_clock::now();

		std::this_thread::sleep_until(startOfFrame + frameDuration);
		startOfFrame = endOfFrame;

	} 
}
#if 0
bool SDLUI::getKeyboardEvent(uint8_t& ev) {
	std::unique_lock<std::mutex> locker(m_keyboardQueueMutex);

	if (m_keyboardQueue.empty())
		return false;

	ev = m_keyboardQueue.front();
	m_keyboardQueue.pop_front();

	return ev;
}
#endif
void SDLUI::pushKeyboardEvent(const SDL_KeyboardEvent& ev) {
	static const std::unordered_map<SDL_Scancode, uint8_t> keyMap{
#if 0
		{ SDL_SCANCODE_F1,  0x20 },
		{ SDL_SCANCODE_F2,  0x26 },
		{ SDL_SCANCODE_F3,  0x05 },
		{ SDL_SCANCODE_F4,  0x3E },
		{ SDL_SCANCODE_F5,  0x50 },
		{ SDL_SCANCODE_F6,  0x42 },
		{ SDL_SCANCODE_F7,  0x03 },
		{ SDL_SCANCODE_F8,  0x2C },
		{ SDL_SCANCODE_F9,  0x06 },
		{ SDL_SCANCODE_F10, 0x0D },
		{ SDL_SCANCODE_F11, 0x67 },
		{ SDL_SCANCODE_F12, 0x7D },
		{ SDL_SCANCODE_F13, 0x72 },
		{ SDL_SCANCODE_PAGEUP, 0x72 }, // alias for F13 
		{ SDL_SCANCODE_F14, 0x7E },
		{ SDL_SCANCODE_PAGEDOWN, 0x7E }, // alias for F14
		{ SDL_SCANCODE_ESCAPE, 0x62 },
		{ SDL_SCANCODE_1, 0x68 },
		{ SDL_SCANCODE_2, 0x6E },
		{ SDL_SCANCODE_3, 0x22 },
		{ SDL_SCANCODE_4, 0x15 },
		{ SDL_SCANCODE_5, 0x63 },
		{ SDL_SCANCODE_6, 0x74 },
		{ SDL_SCANCODE_7, 0x02 },
		{ SDL_SCANCODE_8, 0x08 },
		{ SDL_SCANCODE_9, 0x24 },
		{ SDL_SCANCODE_0, 0x2A },
		{ SDL_SCANCODE_MINUS, 0x30 },
		{ SDL_SCANCODE_EQUALS, 0x2B },
		{ SDL_SCANCODE_BACKSPACE, 0x64 },
		{ SDL_SCANCODE_DELETE, 0x18 },
		{ SDL_SCANCODE_TAB, 0x4A },
		{ SDL_SCANCODE_Q, 0x56 },
		{ SDL_SCANCODE_W, 0x1C },
		{ SDL_SCANCODE_E, 0x0F },
		{ SDL_SCANCODE_R, 0x5D },
		{ SDL_SCANCODE_T, 0x57 },
		{ SDL_SCANCODE_Y, 0x7A },
		{ SDL_SCANCODE_U, 0x1A },
		{ SDL_SCANCODE_I, 0x0C },
		{ SDL_SCANCODE_O, 0x1F },
		{ SDL_SCANCODE_P, 0x25 },
		{ SDL_SCANCODE_LEFTBRACKET, 0x10 },
		{ SDL_SCANCODE_RIGHTBRACKET, 0x5E },
		{ SDL_SCANCODE_RETURN, 0x6A },
		{ SDL_SCANCODE_GRAVE, 0x55 },
		{ SDL_SCANCODE_CAPSLOCK, 0x71 },
		{ SDL_SCANCODE_LCTRL, 0x44 },
		{ SDL_SCANCODE_A, 0x5C },
		{ SDL_SCANCODE_S, 0x4C },
		{ SDL_SCANCODE_D, 0x09 },
		{ SDL_SCANCODE_F, 0x69 },
		{ SDL_SCANCODE_G, 0x4B },
		{ SDL_SCANCODE_H, 0x3F },
		{ SDL_SCANCODE_J, 0x14 },
		{ SDL_SCANCODE_K, 0x37 },
		{ SDL_SCANCODE_L, 0x19 },
		{ SDL_SCANCODE_SEMICOLON, 0x0A },
		{ SDL_SCANCODE_APOSTROPHE, 0x16 },
		{ SDL_SCANCODE_BACKSLASH, 0x3D },
		{ SDL_SCANCODE_SCROLLLOCK, 0x27 },
		{ SDL_SCANCODE_LSHIFT, 0x41 },
		{ SDL_SCANCODE_Z, 0x01 },
		{ SDL_SCANCODE_X, 0x6F },
		{ SDL_SCANCODE_C, 0x51 },
		{ SDL_SCANCODE_V, 0x45 },
		{ SDL_SCANCODE_B, 0x33 },
		{ SDL_SCANCODE_N, 0x21 },
		{ SDL_SCANCODE_M, 0x28 },
		{ SDL_SCANCODE_COMMA, 0x70 },
		{ SDL_SCANCODE_PERIOD, 0x58 },
		{ SDL_SCANCODE_SLASH, 0x52 },
		{ SDL_SCANCODE_RSHIFT, 0x34 },
		{ SDL_SCANCODE_SPACE, 0x5F },
		{ SDL_SCANCODE_PAUSE, 0x0E }, // Break/Discon
		{ SDL_SCANCODE_DELETE, 0x5B }, // Clear/Reset
		{ SDL_SCANCODE_UP, 0x17 },
		{ SDL_SCANCODE_DOWN, 0x76 },
		{ SDL_SCANCODE_LEFT, 0x13 },
		{ SDL_SCANCODE_RIGHT, 0x4F },

		{ SDL_SCANCODE_NUMLOCKCLEAR, 0x49 }, // Numeric keypad ( =
		{ SDL_SCANCODE_KP_DIVIDE, 0x61 },
		{ SDL_SCANCODE_KP_MULTIPLY, 0x11 }, // Numeric keypad ) X
		{ SDL_SCANCODE_KP_PLUS, 0x7F },
		{ SDL_SCANCODE_KP_7, 0x32 },
		{ SDL_SCANCODE_KP_8, 0x12 },
		{ SDL_SCANCODE_KP_9, 0x2F },
		{ SDL_SCANCODE_KP_MINUS, 0x73 },
		{ SDL_SCANCODE_KP_4, 0x1B },
		{ SDL_SCANCODE_KP_5, 0x75 },
		{ SDL_SCANCODE_KP_6, 0x23 },
		{ SDL_SCANCODE_KP_1, 0x6D },
		{ SDL_SCANCODE_KP_2, 0x6B },
		{ SDL_SCANCODE_KP_3, 0x78 },
		{ SDL_SCANCODE_KP_0, 0x53 },
		{ SDL_SCANCODE_KP_PERIOD, 0x2E },
		{ SDL_SCANCODE_KP_ENTER, 0x00 },
#endif

		{ SDL_SCANCODE_ESCAPE,			0x01 },
		{ SDL_SCANCODE_1,				0x02 },
		{ SDL_SCANCODE_2,				0x03 },
		{ SDL_SCANCODE_3,				0x04 },
		{ SDL_SCANCODE_4,				0x05 },
		{ SDL_SCANCODE_5,				0x06 },
		{ SDL_SCANCODE_6,				0x07 },
		{ SDL_SCANCODE_7,				0x08 },
		{ SDL_SCANCODE_8,				0x09 },
		{ SDL_SCANCODE_9,				0x0A },
		{ SDL_SCANCODE_0,				0x0B },
		{ SDL_SCANCODE_MINUS,			0x0C },
		{ SDL_SCANCODE_EQUALS,			0x0D },
		{ SDL_SCANCODE_BACKSPACE,		0x0E },
		{ SDL_SCANCODE_TAB,				0x0F },
		{ SDL_SCANCODE_Q,				0x10 },
		{ SDL_SCANCODE_W,				0x11 },
		{ SDL_SCANCODE_E,				0x12 },
		{ SDL_SCANCODE_R,				0x13 },
		{ SDL_SCANCODE_T,				0x14 },
		{ SDL_SCANCODE_Y,				0x15 },
		{ SDL_SCANCODE_U,				0x16 },
		{ SDL_SCANCODE_I,				0x17 },
		{ SDL_SCANCODE_O,				0x18 },
		{ SDL_SCANCODE_P,				0x19 },
		{ SDL_SCANCODE_LEFTBRACKET,		0x1A },
		{ SDL_SCANCODE_RIGHTBRACKET,	0x1B },
		{ SDL_SCANCODE_RETURN,			0x1C },
		{ SDL_SCANCODE_LCTRL,			0x1D },
		{ SDL_SCANCODE_A,				0x1E },
		{ SDL_SCANCODE_S,				0x1F },
		{ SDL_SCANCODE_D,				0x20 },
		{ SDL_SCANCODE_F,				0x21 },
		{ SDL_SCANCODE_G,				0x22 },
		{ SDL_SCANCODE_H,				0x23 },
		{ SDL_SCANCODE_J,				0x24 },
		{ SDL_SCANCODE_K,				0x25 },
		{ SDL_SCANCODE_L,				0x26 },
		{ SDL_SCANCODE_SEMICOLON,		0x27 },
		{ SDL_SCANCODE_APOSTROPHE,		0x28 },
		{ SDL_SCANCODE_GRAVE,			0x29 },
		{ SDL_SCANCODE_LSHIFT,			0x2A },
		{ SDL_SCANCODE_BACKSLASH,		0x2B },
		{ SDL_SCANCODE_Z,               0x2C },
		{ SDL_SCANCODE_X,               0x2D },
		{ SDL_SCANCODE_C,               0x2E },
		{ SDL_SCANCODE_V,               0x2F },
		{ SDL_SCANCODE_B,               0x30 },
		{ SDL_SCANCODE_N,               0x31 },
		{ SDL_SCANCODE_M,               0x32 },
		{ SDL_SCANCODE_COMMA,			0x33 },
		{ SDL_SCANCODE_PERIOD,			0x34 },
		{ SDL_SCANCODE_SLASH,			0x35 },
		{ SDL_SCANCODE_RSHIFT,			0x36 },
		{ SDL_SCANCODE_PRINTSCREEN,		0x37 },
		{ SDL_SCANCODE_LALT,			0x38 },
		{ SDL_SCANCODE_SPACE,			0x39 },
		{ SDL_SCANCODE_CAPSLOCK,		0x3A },
		{ SDL_SCANCODE_F1,              0x3B },
		{ SDL_SCANCODE_F2,              0x3C },
		{ SDL_SCANCODE_F3,              0x3D },
		{ SDL_SCANCODE_F4,              0x3E },
		{ SDL_SCANCODE_F5,              0x3F },
		{ SDL_SCANCODE_F6,              0x40 },
		{ SDL_SCANCODE_F7,              0x41 },
		{ SDL_SCANCODE_F8,              0x42 },
		{ SDL_SCANCODE_F9,              0x43 },
		{ SDL_SCANCODE_F10,             0x44 },
		{ SDL_SCANCODE_NUMLOCKCLEAR,	0x45 },
		{ SDL_SCANCODE_SCROLLLOCK,      0x46 },
		{ SDL_SCANCODE_KP_7,			0x47 },	
		{ SDL_SCANCODE_KP_8,			0x48 },
		{ SDL_SCANCODE_KP_9,			0x49 },
		{ SDL_SCANCODE_KP_MINUS,        0x4A },
		{ SDL_SCANCODE_KP_4,			0x4B },
		{ SDL_SCANCODE_KP_5,            0x4C },
		{ SDL_SCANCODE_KP_6,			0x4D },
		{ SDL_SCANCODE_KP_PLUS,         0x4E },
		{ SDL_SCANCODE_KP_1,			0x4F },
		{ SDL_SCANCODE_KP_2,			0x50 },
		{ SDL_SCANCODE_KP_3,			0x51 },
		{ SDL_SCANCODE_KP_0,			0x52 },
		{ SDL_SCANCODE_KP_PERIOD,		0x53 },
		
		// extended codes
		{ SDL_SCANCODE_RALT,			0x80 | 0x38 },
		{ SDL_SCANCODE_RCTRL,			0x80 | 0x1D },
		{ SDL_SCANCODE_INSERT,			0x80 | 0x52 },
		{ SDL_SCANCODE_DELETE,			0x80 | 0x53 },
		{ SDL_SCANCODE_HOME,			0x80 | 0x47 },
		{ SDL_SCANCODE_END,				0x80 | 0x4F },
		{ SDL_SCANCODE_PAGEUP,			0x80 | 0x49 },
		{ SDL_SCANCODE_PAGEDOWN,		0x80 | 0x51 },
		{ SDL_SCANCODE_LEFT,			0x80 | 0x4B },
		{ SDL_SCANCODE_UP,				0x80 | 0x48 },
		{ SDL_SCANCODE_DOWN,			0x80 | 0x50 },
		{ SDL_SCANCODE_RIGHT,			0x80 | 0x4D },
		{ SDL_SCANCODE_KP_DIVIDE,		0x80 | 0x35 },
		{ SDL_SCANCODE_KP_ENTER,		0x80 | 0x1C },
		{ SDL_SCANCODE_F11,				0x57 },
		{ SDL_SCANCODE_F12,				0x58 },
		{ SDL_SCANCODE_SYSREQ,			0x54 },
	};

	auto keyboard = m_keyboard.load();

	if (ev.repeat || !keyboard)
		return;

	auto it = keyMap.find(ev.keysym.scancode);
	if (it == keyMap.end())
		return;

	uint8_t code = it->second;

	if (code & 0x80) {
		keyboard->pushScancode(0xE0);
	}

	if (ev.state == SDL_PRESSED) {
		keyboard->pushScancode(code & 0x7F);
	}
	else {
		keyboard->pushScancode(code | 0x80);
	}
}
#if 0
void SDLUI::updateMouseButton(const SDL_MouseButtonEvent& ev) {
	static const std::unordered_map<uint8_t, uint8_t> buttonMap{
		{ 1, (1 << 2) }, // left
		{ 2, (1 << 1) }, // middle
		{ 3, (1 << 0) } // right
	};

	auto it = buttonMap.find(ev.button);
	if (it == buttonMap.end())
		return;

	if (ev.state == SDL_PRESSED)
		m_mouseButtons.fetch_or(it->second);
	else
		m_mouseButtons.fetch_and(~it->second);
}
#endif