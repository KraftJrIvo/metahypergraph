#include "types.h"
#include "raylib.h"
#include "raymath.h"
#include <array>

namespace hmg {

    void Node::recalc() {
        lvl = getLevel();
        coeff = getCoeff();
        radius = NODE_SZ * coeff;
    }

    int Node::getLevel() {
        if (parent)
            return parent->getLevel() + 1;
        return 0;
    }

    float Node::getCoeff() {
        float coeff = 1.0f;
        if (parent)
            coeff = parent->getCoeff() * 1.0f / (parent->subNodes.size() + 1);
        return coeff;
    }

    void Node::recalcTower(NodePtr here, NodePtr from) {
        here->recalc();
        for (auto& e : edgesOut)
            e->recalc();
        if (parent && parent != from)
            parent->recalcTower(parent, here);
        for (auto& sn : subNodes)
            if (sn != from)
                sn->recalcTower(sn, here);
    }

    Color Edge::color(EdgeType type) {
        switch (type) {
            case PROPERTY:
                return RED;
            case ACTION:
                return LIME;
            case PROCESS:
                return GREEN;
            case TARGET:
                return BLUE;
            case ORDER:
                return GRAY;
            return LIGHTGRAY;
        }
    }

    void Node::move(const Vector2& delta) {
        pos += delta;
        for (auto& sn : subNodes)
            sn->pos += delta;
    }

    void Node::moveTo(const Vector2& to) {
        Vector2 delta = to - pos;
        move(delta);
    }

    void Edge::reposition() {
        via->pos = 0.5f * (from->pos + to->pos);
    }

    bool Node::draw(Vector2 offset, float scale, const Font& font) {
        float r = radius * scale;
        Vector2 posmod = pos * scale + offset;
        DrawCircleV(posmod, r, GRAY);
        DrawCircleLinesV(posmod, r, LIGHTGRAY);
        auto sz = MeasureTextEx(font, label.c_str(), int(2 * r), 0);
        auto ratio = (r * 2) / Vector2Length(sz);
        DrawTextEx(font, label.c_str(), posmod - ratio * sz * 0.5f, ratio * sz.y, 0, WHITE);
        return r * r > Vector2DistanceSqr(GetMousePosition(), posmod);
    }

    void Edge::draw(Vector2 offset, float scale, const Font& font) {
        static std::array<Vector2, 3> pts;
        pts[0] = scale * from->pos + offset;
        pts[1] = scale * via->pos + offset;
        pts[2] = scale * to->pos + offset;
        float thick = std::max(EDGE_THICK * scale, 1.0f);
        DrawSplineBezierQuadratic(pts.data(), 3, thick, DARKGRAY);
    }
}