#include "hypergraph.h"
#include "raylib.h"
#include <cmath>

namespace mhg {

    float Node::getDistToBorder(float angle, float scale) {
        if (hyper) {
            float thick = std::clamp(2 * EDGE_THICK, 1.0f, EDGE_THICK * 2) / scale;
            return thick;
        }
        return NODE_SZ;
    }

    void Node::predraw(Vector2 origin, Vector2 offset, float scale, const Font& font) {
        float ls = hg->scale() * scale;
        Vector2 posmod = origin + pos * ls + offset;
        _posCache = posmod;
        if (hyper) {
            float thick = std::clamp(2 * EDGE_THICK * ls, 1.0f, EDGE_THICK * 2);
            DrawCircleV(posmod, thick, RED);
            _rCache = thick;
        } else {
            float thick = std::clamp(NODE_BORDER * ls, 1.0f, NODE_BORDER);
            float r = (NODE_SZ) * ls + thick;
            _rCache = (NODE_SZ) * ls;
            DrawCircleV(posmod, r, { 140, 140, 140, 255 });
        }
    }

    bool Node::draw(Vector2 origin, Vector2 offset, float scale, const Font& font) {
        float ls = hg->scale() * scale;
        float r = NODE_SZ * ls;
        Vector2 posmod = origin + pos * ls + offset;
        bool drawContent = (content && ls > HIDE_CONTENT_SCALE);
        Color c = (content && !drawContent) ? Color{ 140, 140, 140, 255 } : DARKGRAY;
        DrawCircleV(posmod, r, c);
        if (ls > HIDE_TXT_SCALE) {
            auto sz = MeasureTextEx(font, label.c_str(), FONT_SZ, 0);
            Vector2 txtpos = posmod - sz * 0.5f;
            float txtsz = sz.y;
            if (drawContent) {
                float thick = std::clamp(NODE_BORDER * scale, 1.0f, NODE_BORDER);
                float rr = r + thick;
                txtpos.y += (rr + txtsz * 0.5f) * ((hg->lvl % 2) ? 1.0f : -1.0f);
            }
            bool labelFits = 0.8f * sz.x < sqrt(2) * r;
            if (labelFits)
                DrawTextEx(font, label.c_str(), txtpos, txtsz, 0, WHITE);
        }
        return r * r > Vector2DistanceSqr(GetMousePosition(), posmod);
    }

}