#include "hypergraph.h"
#include "raylib.h"
#include "../util/vec_ops.h"
#include "raymath.h"
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <string>

namespace mhg {

    size_t Node::maxLinks() {
        size_t maxLinks = 0;
        for (auto& e : edgesIn)
            maxLinks = std::max(e->links.size(), maxLinks);
        for (auto& e : edgesOut)
            maxLinks = std::max(e->links.size(), maxLinks);
        return maxLinks;
    }

    size_t Node::nLinks() {
        size_t nLinks = 0;
        for (auto& e : edgesIn)
            nLinks += e->links.size();
        for (auto& e : edgesOut)
            nLinks += e->links.size();
        return nLinks;
    }

    EdgePtr Node::edgeTo(NodePtr node) {
        for (auto& e : edgesIn)
            if (e->from == node || e->to == node)
                return e;
        for (auto& e : edgesOut)
            if (e->from == node || e->to == node)
                return e;
        return nullptr;
    }

    EdgePtr Node::similarEdge(EdgePtr edge) {
        for (auto& e : edgesIn)
            if (e->similar(edge))
                return e;
        for (auto& e : edgesOut)
            if (e->similar(edge))
                return e;
        return nullptr;
    }

    void Node::predraw(Vector2 origin, Vector2 offset, float scale, const Font& font) {
        float ls = hg->scale() * scale;
        Vector2 posmod = origin + pos * ls + offset;
        _posCache = posmod;
        if (hyper) {
            float r = std::clamp((1 + maxLinks()) * EDGE_THICK * ls, 1.0f, (1 + maxLinks()) * EDGE_THICK);
            _rCache = r;
        } else {
            float thick = std::clamp(NODE_BORDER * ls, 1.0f, NODE_BORDER);
            float r = (NODE_SZ) * ls + thick;
            _rCache = (NODE_SZ) * ls;
            DrawCircleV(posmod, r, ColorBrightness({ 140, 140, 140, 255 }, highlight));
        }
    }

    bool Node::draw(Vector2 origin, Vector2 offset, float scale, const Font& font) {
        float ls = hg->scale() * scale;
        Vector2 posmod = origin + pos * ls + offset;
        bool hover;
        if (hyper) {
            hover = (_rCache * _rCache > Vector2DistanceSqr(GetMousePosition(), posmod));
            Vector3 c = Vector3Zero();
            float n = 0;
            for (auto& e : edgesIn) {
                for (auto& l : e->links) {
                    c = c + Vector3{(float)l->style->color.r, (float)l->style->color.g, (float)l->style->color.b};
                    n++;
                }
            }
            for (auto& e : edgesOut) {
                for (auto& l : e->links) {
                    c = c + Vector3{(float)l->style->color.r, (float)l->style->color.g, (float)l->style->color.b};
                    n++;
                }
            }
            Color avgColor = (n > 0) ? Color{ uint8_t(c.x / n), uint8_t(c.y / n), uint8_t(c.z / n), 255 } : WHITE;
            DrawCircleV(posmod, _rCache, ColorBrightness(avgColor, highlight));
        } else {
            float r = NODE_SZ * ls;
            hover = (r * r > Vector2DistanceSqr(GetMousePosition(), posmod));
            bool hasContent = content && content->nodesCount();
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
        highlight = 0.0f;
        return hover;
    }

}