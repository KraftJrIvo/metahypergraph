#include "edge.h"

namespace mhg {

    void Edge::reposition() {
        via->pos = 0.5f * (from->pos + to->pos);
    }

    void Edge::draw(Vector2 origin, Vector2 offset, float scale, const Font& font) {
        _pts[0] = origin + scale * from->pos + offset;
        _pts[1] = origin + scale * via->pos + offset;
        _pts[2] = origin + scale * to->pos + offset;
        float thick = std::clamp(EDGE_THICK * scale, 1.0f, EDGE_THICK);
        DrawSplineBezierQuadratic(_pts.data(), 3, thick, RED);
    }
}