#include <UI/SDLUI.h>
#include <UI/VideoAdapter.h>
#include <UI/Keyboard.h>
#include <UI/Mouse.h>

#include <Utils/WindowsResources.h>

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

SDLUI::SDLUI() : m_videoAdapter(nullptr), m_keyboard(nullptr), m_mouse(nullptr), m_mouseCaptured(false) {
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

	const void* fontBmpData;
	size_t fontBmpDataSize;

	getRCDATA(2, &fontBmpData, &fontBmpDataSize);

	auto fontRW = SDL_RWFromConstMem(fontBmpData, fontBmpDataSize);
	if(fontRW == nullptr)
		throw std::runtime_error("SDL_RWFromMem failed: " + std::string(SDL_GetError()));;

	auto font = SDL_LoadBMP_RW(fontRW, 0);
	SDL_RWclose(fontRW);

	if (!font) {
		throw std::runtime_error("SDL_LoadBMP failed: " + std::string(SDL_GetError()));;
	}
	m_textModeFont.reset(font);
}

SDLUI::~SDLUI() {
	SDL_Quit();
}

void SDLUI::run() {
	SDL_Event ev;

	auto startOfFrame = std::chrono::steady_clock::now();
	auto frameDuration = std::chrono::nanoseconds(16666666); // 60 Hz

	while(true) {
		while(SDL_PollEvent(&ev)) {
			switch(ev.type) {
			case SDL_QUIT:
				return;

			case SDL_MOUSEMOTION:
				if (m_mouseCaptured) {
					auto mouse = m_mouse.load();
					if (mouse)
						mouse->addDeltas(ev.motion.xrel, ev.motion.yrel);
				}
				break;
			case SDL_MOUSEBUTTONDOWN:
				if (!m_mouseCaptured) {
					if (SDL_SetRelativeMouseMode(SDL_TRUE) >= 0) {
						m_mouseCaptured = true;
					}
				} else { 
					updateMouseButton(ev.button);
				}
				break;


			case SDL_MOUSEBUTTONUP:
				if (m_mouseCaptured) {
					updateMouseButton(ev.button);
				}
					
				break;

			case SDL_KEYDOWN:
				if (ev.key.keysym.scancode == SDL_SCANCODE_LALT && m_mouseCaptured) {
					m_mouseCaptured = false;

					SDL_SetRelativeMouseMode(SDL_FALSE);
				}

				pushKeyboardEvent(ev.key);
				break;

			case SDL_KEYUP:
				pushKeyboardEvent(ev.key);
				break;
			}
		}

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
					if (!m_graphicsSurface || m_graphicsSurface->w != config.widthPixels || m_graphicsSurface->h != config.heightPixels) {
						// explicitly allocate framebuffer to ensure tightly packed rows
						m_graphicsSurface.reset();
						m_graphicsSurfaceData.resize(config.widthPixels / 8 * config.heightPixels);
						m_graphicsSurface.reset(SDL_CreateRGBSurfaceWithFormatFrom(m_graphicsSurfaceData.data(), config.widthPixels, config.heightPixels, 1, config.widthPixels / 8, SDL_PIXELFORMAT_INDEX1MSB));
						SDL_SetSurfacePalette(m_graphicsSurface.get(), m_screenPalettes[PaletteIndexDimOnBlank].get());
					}

					linearizeHerculesVideo(config, m_graphicsSurfaceData.data());

					SDL_Rect rect;
					rect.x = 0;
					rect.y = 0;
					rect.w = config.widthPixels;
					rect.h = config.heightPixels;

					SDL_BlitSurface(m_graphicsSurface.get(), &rect, surface, &rect);
				}

				SDL_UpdateWindowSurface(m_window.get());
			}
		}

		auto endOfFrame = std::chrono::steady_clock::now();

		std::this_thread::sleep_until(startOfFrame + frameDuration);
		startOfFrame = endOfFrame;

	} 
}

void SDLUI::linearizeHerculesVideo(const VideoAdapter::AdapterConfiguration& config, unsigned char* data) {
	unsigned int pitch = config.widthPixels / 8;

	for (unsigned int dstrow = 0; dstrow < config.heightPixels; dstrow++) {
		auto srcData = config.graphicsModeFramebuffer + ((dstrow & 3) << 13) + ((dstrow >> 2) * 90);

		memcpy(data + dstrow * pitch, srcData, pitch);
	}
}

void SDLUI::pushKeyboardEvent(const SDL_KeyboardEvent& ev) {
	static const std::unordered_map<SDL_Scancode, uint8_t> keyMap{
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

	if (!keyboard)
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

void SDLUI::updateMouseButton(const SDL_MouseButtonEvent& ev) {
	static const std::unordered_map<uint8_t, uint8_t> buttonMap{
		{ 1, 2 }, // left
		{ 2, 1 }, // middle
		{ 3, 0 } // right
	};

	auto it = buttonMap.find(ev.button);
	if (it == buttonMap.end())
		return;

	auto mouse = m_mouse.load();
	if (mouse)
		mouse->updateButtonState(it->second, ev.state == SDL_PRESSED);
}
