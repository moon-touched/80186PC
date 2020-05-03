#include <Hardware/Machine.h>

#include <SDL.h>

#include <UI/SDLUI.h>

int main(int argc, char* argv[]) {
	SDLUI ui;

	Machine machine;

	ui.setVideoAdapter(machine.videoAdapter());
	ui.setKeyboard(machine.keyboard());

	ui.run();

	return 0;
}
