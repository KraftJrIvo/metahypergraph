#pragma once

#include <array>

#include "node.h"

namespace mhg {

    enum EdgeType {
        PROPERTY,
        ACTION,
        PROCESS,
        TARGET,
        ORDER
    };

    struct Edge {
        size_t idx;
        EdgeType type;
        NodePtr from;
        NodePtr via;
        NodePtr to;

        Edge(size_t idx, EdgeType type, NodePtr from, NodePtr via, NodePtr to) :
            idx(idx), type(type), from(from), via(via), to(to)
        { }

        void reposition();

        void draw(Vector2 origin, Vector2 offset, float scale, const Font& font);
    private:
        std::array<Vector2, 3> _pts;
    };

}