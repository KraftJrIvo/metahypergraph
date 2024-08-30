#include "node.h"
#include "hypergraph.h"
#include "raylib.h"
#include <cmath>

namespace mhg {

    void Node::predraw(Vector2 origin, Vector2 offset, float scale, const Font& font) {
        Vector2 posmod = origin + pos * scale + offset;
        if (hyper) {
            float thick = std::clamp(2 * EDGE_THICK * scale, 1.0f, EDGE_THICK * 2);
            DrawCircleV(posmod, thick, RED);
        } else {
            float thick = std::clamp(NODE_BORDER * scale, 1.0f, NODE_BORDER);
            float r = (NODE_SZ) * scale + thick;
            DrawCircleV(posmod, r, { 140, 140, 140, 255 });
        }
    }

    bool Node::draw(Vector2 origin, Vector2 offset, float scale, const Font& font) {
        float r = NODE_SZ * scale;
        Vector2 posmod = origin + pos * scale + offset;
        bool drawContent = (content && scale > HIDE_CONTENT_SCALE);
        Color c = (content && !drawContent) ? Color{ 140, 140, 140, 255 } : DARKGRAY;
        DrawCircleV(posmod, r, c);
        if (scale > HIDE_TXT_SCALE) {
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