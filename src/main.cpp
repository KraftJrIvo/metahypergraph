#include "drawer.h"
#include "raylib.h"

#include <thread>

#define W_W 768
#define W_H 768

// TODO
// merge
// duplication

int main(int argc, char *argv[]) {
	
    mhg::MetaHyperGraph mhg;

	mhg.init();

    auto drawer = mhg::Drawer::create(mhg, {W_W, W_H}, "META HYPER GRAPH");

	while (drawer->isDrawing()) {
		//mhg.doPhysics();
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
		if (!drawer->isEditing() && IsKeyPressed(KEY_R)) {
			mhg.init();
			drawer->recenter();
		}
	}

    return 0;
}