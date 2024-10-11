#include "hypergraph.h"
#include "raylib.h"
#include "../util/vec_ops.h"
#include "raymath.h"
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <string>

namespace mhg {

    float Node::coeff() {
        return 1.0f / ((content ? content->nDrawableNodes : 0) + dp.tmpDrawableNodes + 1);
    }
    
    size_t Node::nodesCount() {
        return dp.tmpDrawableNodes + (content ? content->nodesCount() : 0);
    }

    float Node::scale() {
        return hg->scale() * coeff();
    }

    size_t Node::getMaxLinks() {
        size_t maxLinks = 0;
        for (auto& e : eIn)
            maxLinks = std::max(e->links.size(), maxLinks);
        for (auto& e : eOut)
            maxLinks = std::max(e->links.size(), maxLinks);
        return maxLinks;
    }

    size_t Node::getLinksCount() {
        size_t nLinks = 0;
        for (auto& e : eIn)
            nLinks += e->links.size();
        for (auto& e : eOut)
            nLinks += e->links.size();
        return nLinks;
    }

    EdgePtr Node::getEdgeTo(NodePtr node) {
        for (auto& e : eIn)
            if (e->from == node || e->to == node)
                return e;
        for (auto& e : eOut)
            if (e->from == node || e->to == node)
                return e;
        return nullptr;
    }

    EdgePtr Node::getSimilarEdge(EdgePtr edge) {
        for (auto& e : eIn)
            if (e->similar(edge))
                return e;
        for (auto& e : eOut)
            if (e->similar(edge))
                return e;
        return nullptr;
    }

    void Node::predraw(Vector2 origin, Vector2 offset, float scale, const Font& font) {
        float ls = hg->scale() * scale;
        Vector2 posmod = origin + dp.pos * ls + offset;
        dp.posCache = posmod;
        bool willDrop = dp.highlight > 0 && ((dp.overNode && (!hg->parent || dp.overNode != hg->parent)) || dp.overRoot);
        float ss = willDrop ? (dp.overRoot ? scale : (dp.overNode->scale() * scale)) : ls;
        if (hyper) {
            float r = std::clamp((1 + getMaxLinks()) * EDGE_THICK * ss, 1.0f, (1 + getMaxLinks()) * EDGE_THICK);
            dp.rCache = r;
        } else {
            float thick = std::clamp(NODE_BORDER * ss, 1.0f, NODE_BORDER);
            float r = (NODE_SZ) * ss + thick;
            dp.rCache = (NODE_SZ) * ss;
            DrawCircleV(posmod, r, ColorBrightness({ 140, 140, 140, 255 }, dp.highlight));
        }
        dp.rCacheStable = dp.rCache * (ls / ss);
    }

    bool Node::draw(Vector2 origin, Vector2 offset, float scale, const Font& font) {
        float ls = hg->scale() * scale;
        Vector2 posmod = origin + dp.pos * ls + offset;
        bool hover;
        if (hyper) {
            hover = (dp.rCache * dp.rCache > Vector2DistanceSqr(GetMousePosition(), posmod));
            Vector3 c = Vector3Zero();
            float n = 0;
            for (auto& e : eIn) {
                for (auto& l : e->links) {
                    c = c + Vector3{(float)l->style->color.r, (float)l->style->color.g, (float)l->style->color.b};
                    n++;
                }
            }
            for (auto& e : eOut) {
                for (auto& l : e->links) {
                    c = c + Vector3{(float)l->style->color.r, (float)l->style->color.g, (float)l->style->color.b};
                    n++;
                }
            }
            Color avgColor = (n > 0) ? Color{ uint8_t(c.x / n), uint8_t(c.y / n), uint8_t(c.z / n), 255 } : WHITE;
            DrawCircleV(posmod, dp.rCache, ColorBrightness(avgColor, dp.highlight));
        } else {
            float r = dp.rCache;
            hover = (r * r > Vector2DistanceSqr(GetMousePosition(), posmod));
            bool hasContent = content && content->nodesCount();
            bool drawContent = (hasContent && ls > HIDE_CONTENT_SCALE);
            Color c = dp.editing ? BLUE : ((hasContent && !drawContent) ? Color{ 140, 140, 140, 255 } : DARKGRAY);
            DrawCircleV(posmod, r, c);
            bool drawLabel = dp.editing || ls > HIDE_TXT_SCALE;
            if (drawLabel) {
                auto sz = MeasureTextEx(font, p.label.c_str(), FONT_SZ, 0);
                Vector2 txtpos = posmod - sz * 0.5f;
                float txtsz = sz.y;
                bool labelFits = (0.8f * sz.x < sqrt(2) * r);
                if (drawContent || !labelFits) {
                    float thick = std::clamp(NODE_BORDER * scale, 1.0f, NODE_BORDER);
                    float rr = r + thick;
                    txtpos.y += (rr + txtsz * 0.5f) * ((hg->lvl % 2) ? 1.0f : -1.0f);
                }
                DrawTextEx(font, p.label.c_str(), txtpos, txtsz, 0, WHITE);
            }
        }
        return hover;
    }

    void Node::resetDraw() {
        dp.highlight = 0.0f;
        dp.tmpDrawableNodes = 0;
        dp.overNode = nullptr;
        dp.overRoot = false;
    }
}