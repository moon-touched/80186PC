#include <Hardware/Machine.h>

#include <SDL.h>

#include <UI/SDLUI.h>

int main(int argc, char* argv[]) {
	if(argc < 2) {
		fprintf(stderr, "Usage: %s <HARD DISK IMAGE IN VHD FORMAT>\n", argv[0]);
		return 1;
	}

	SDLUI ui;

	Machine machine(argv[1]);

	ui.setVideoAdapter(machine.videoAdapter());
	ui.setKeyboard(machine.keyboard());
	ui.setMouse(machine.mouse());

	ui.run();

	return 0;
}
