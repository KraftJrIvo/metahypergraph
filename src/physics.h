#pragma once

#include "types.h"

namespace hmg {
    class PhysicsSolver {
    public:
        void init();
        void step(std::map<size_t, NodePtr>& nodes);
    private:
        bool _inited = false;
    };
}