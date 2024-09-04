#include "hypergraph.h"
#include "raylib.h"
#include <cmath>
#include <string>

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
            float r = std::clamp(2 * EDGE_THICK * ls, 1.0f, EDGE_THICK * 2);
            _rCache = r;
        } else {
            float thick = std::clamp(NODE_BORDER * ls, 1.0f, NODE_BORDER);
            float r = (NODE_SZ) * ls + thick;
            _rCache = (NODE_SZ) * ls;
            DrawCircleV(posmod, r, highlight ? ColorBrightness({ 140, 140, 140, 255 }, 0.25f) : Color{ 140, 140, 140, 255 });
        }
    }

    bool Node::draw(Vector2 origin, Vector2 offset, float scale, const Font& font) {
        float ls = hg->scale() * scale;
        Vector2 posmod = origin + pos * ls + offset;
        bool hover;
        if (hyper) {
            hover = (_rCache * _rCache > Vector2DistanceSqr(GetMousePosition(), posmod));
            DrawCircleV(posmod, _rCache, highlight ? ColorBrightness(RED, 0.25f) : RED);
        } else {
            float r = NODE_SZ * ls;
            hover = (r * r > Vector2DistanceSqr(GetMousePosition(), posmod));
            bool hasContent = content && content->nDrawableNodes;
            bool drawContent = (hasContent && ls > HIDE_CONTENT_SCALE);
            Color c = editing ? BLUE : ((hasContent && !drawContent) ? Color{ 140, 140, 140, 255 } : DARKGRAY);
            DrawCircleV(posmod, r, c);
            bool drawLabel = editing || ls > HIDE_TXT_SCALE;
            if (drawLabel) {
                auto sz = MeasureTextEx(font, label.c_str(), FONT_SZ, 0);
                Vector2 txtpos = posmod - sz * 0.5f;
                float txtsz = sz.y;
                bool labelFits = (0.8f * sz.x < sqrt(2) * r);
                if (drawContent || !labelFits) {
                    float thick = std::clamp(NODE_BORDER * scale, 1.0f, NODE_BORDER);
                    float rr = r + thick;
                    txtpos.y += (rr + txtsz * 0.5f) * ((hg->lvl % 2) ? 1.0f : -1.0f);
                }
                DrawTextEx(font, label.c_str(), txtpos, txtsz, 0, WHITE);
            }
        }
        return hover;
    }

}