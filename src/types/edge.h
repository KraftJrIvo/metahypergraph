#pragma once

#include <array>
#include <string_view>

#include "node.h"
#include "raylib.h"

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

        static Texture2D getArrowHead();

        Edge(size_t idx, EdgeType type, NodePtr from, NodePtr via, NodePtr to) :
            idx(idx), type(type), from(from), via(via), to(to)
        { }

        bool highlight = false;

        void findArrowPositionBezier(bool atStart, float scale, Vector2& at, float& angle, float& t);
        bool DrawSplineSegmentBezierQuadraticPart(Vector2 p1, Vector2 c2, Vector2 p3, float thick, Color color, float start, float end);
        Vector2 getPoint(float t);
        void reposition();

        bool draw(Vector2 origin, Vector2 offset, float s, const Font& font, bool physics);
    private:
        std::array<Vector2, 3> _pts;
    };

}