#include "drawer.h"
#include "raylib.h"

#include <thread>

#define W_W 1024
#define W_H 1024

int main(int argc, char *argv[]) {
	
    hmg::HyperMetaGraph hmg;

	hmg.init();

    auto drawer = hmg::Drawer::create(hmg, {W_W, W_H}, "HYPER META GRAPH");
	drawer->recenter();

	while (drawer->isDrawing()) {
		hmg.doPhysics();
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
		if (IsKeyPressed(KEY_R)) {
			hmg.init();
			drawer->recenter();
		}
	}

    return 0;
}