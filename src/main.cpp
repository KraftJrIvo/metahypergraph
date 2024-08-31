#include "drawer.h"
#include "raylib.h"

#include <thread>

#define W_W 1024
#define W_H 1024

int main(int argc, char *argv[]) {
	
    mhg::MetaHyperGraph mhg;

	mhg.init();

    auto drawer = mhg::Drawer::create(mhg, {W_W, W_H}, "HYPER META GRAPH");

	while (drawer->isDrawing()) {
		//mhg.doPhysics();
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
		if (IsKeyPressed(KEY_R)) {
			mhg.init();
			drawer->recenter();
		}
	}

    return 0;
}