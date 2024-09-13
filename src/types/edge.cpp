#include "edge.h"
#include "base.h"
#include "hypergraph.h"
#include "raylib.h"
#include "raymath.h"
#include <cmath>
#include <cstddef>
#include <memory>
#include <string>

namespace mhg {

    bool operator==(const EdgeLinkPtr& lhs, const EdgeLinkPtr& rhs) {
        return lhs->style->color.r == rhs->style->color.r && 
               lhs->style->color.g == rhs->style->color.g && 
               lhs->style->color.b == rhs->style->color.b && 
               lhs->style->color.a == rhs->style->color.a && 
               lhs->style->label == rhs->style->label; 
    }
    bool operator<(const EdgeLinkPtr& lhs, const EdgeLinkPtr& rhs) {
        size_t c1 = col2num(lhs->style->color);
        size_t c2 = col2num(rhs->style->color);
        return (lhs->style->label == rhs->style->label && c1 < c2) || (lhs->style->label < rhs->style->label);
    }

    EdgePtr EdgeLink::edge() {
        return hg->getEdge(eIdx);
    }

    Texture2D Edge::getArrowHead() {
        static Texture2D tex;
        static bool loaded = false;
        if (loaded)
            return tex;
        tex = LoadTexture("res/arrowhead.png");
        loaded = true;
        return tex;
    }

    Vector2 Edge::getPoint(Vector2 p1, Vector2 c2, Vector2 p3, float t) {
        float x = pow(1 - t, 2) * p1.x + 2 * t * (1 - t) * c2.x + t * t * p3.x;
        float y = pow(1 - t, 2) * p1.y + 2 * t * (1 - t) * c2.y + t * t * p3.y;
        return Vector2{ x, y};
    }

    void Edge::reposition() {
        via->pos = 0.5f * (from->pos + to->pos);
    }

    bool Edge::similar(EdgePtr edge) {
        return ((from == edge->from && to == edge->to) || (to == edge->from && from == edge->to));
    }

    void Edge::fuse(EdgePtr edge) {
        bool inv = edge->from != from;
        for (auto l : edge->links) {
            auto it = links.find(l);
            if (it == links.end()) {
                if (inv) {
                    auto tmp = l->params.backward;
                    l->params.backward = l->params.foreward;
                    l->params.foreward = tmp;
                }
                links.insert(l);
            } else {
                (*it)->params.foreward |= inv ? l->params.backward : l->params.foreward;
                (*it)->params.backward |= inv ? l->params.foreward : l->params.backward;
                (*it)->params.weight += l->params.weight;
            }
        }
    }

    void Edge::reduce(EdgePtr edge) {
        bool inv = edge->from != from;
        auto elinks = edge->links;
        for (auto l : elinks) {
            auto it = links.find(l);
            if (it != links.end()) {
                if ((inv && l->params.backward) || (!inv && l->params.foreward)) (*it)->params.foreward = false;
                if ((inv && l->params.foreward) || (!inv && l->params.backward)) (*it)->params.backward = false;
                if (!(*it)->params.foreward && !(*it)->params.backward)
                    links.erase(l);
            }
        }
    }

    EdgeLinkPtr Edge::draw(Vector2 origin, Vector2 offset, float scale, const Font& font, bool physics) {
        float ls = via->hg->scale() * scale;
        float minLvlNodeScale = scale * ((from->hg->scale() > to->hg->scale()) ? from->hg->scale() : to->hg->scale());
        float maxLvlNodeScale = scale * ((from->hg->scale() < to->hg->scale()) ? from->hg->scale() : to->hg->scale());
        bool fromSameHG = (from->hg == via->hg);
        bool toSameHG = (to->hg == via->hg);
        bool notSame = !fromSameHG || !toSameHG;

        Vector2 pt0 = fromSameHG ? (origin + ls * from->pos + offset) : from->_posCache;
        Vector2 pt2 = toSameHG ? (origin + ls * to->pos + offset) : to->_posCache;
        Vector2 pt1 = (notSame || !physics) ? (0.5f * (pt0 + pt2)) : (origin + ls * via->pos + offset);
        Vector2 pt0m, pt1m, pt2m;

        Vector2 apos; float angle; float t1; 
        findArrowPositionBezier(pt0, pt1, pt2, false, minLvlNodeScale, apos, angle, t1);
        Vector2 apos2; float angle2; float t2;
        findArrowPositionBezier(pt0, pt1, pt2, true, minLvlNodeScale, apos2, angle2, t2);

        float fromSpreadStep = (2 * PI / ((from->hyper ? 1 : LINKS_DENSITY) * from->nLinks()));
        float toSpreadStep = (2 * PI / ((to->hyper ? 1 : LINKS_DENSITY) * to->nLinks()));
        float fromSpread = fromSpreadStep * links.size();
        float toSpread = toSpreadStep * links.size();
        float aFromStep = fromSpread / (links.size() + 1);
        float aToStep = toSpread / (links.size() + 1);

        EdgeLinkPtr hoverLink = nullptr;

        float a1 = angle - fromSpread * 0.5 + aFromStep;
        float a2 = angle2 + toSpread * 0.5 - aToStep;

        bool drawArrows = (maxLvlNodeScale > HIDE_ARROW_SCALE);

       for (auto& l : links) {
            if (links.size() > 1) {
                pt0m = pt0 + from->_rCache * Vector2{ cos(a1), sin(a1) };
                pt2m = pt2 + to->_rCache * Vector2{ cos(a2), sin(a2) };
                pt1m = pt1 + (pt0m - pt0 + pt2m - pt2) * 0.5f;

                a1 += aFromStep;
                a2 -= aToStep;
                
                t1 = 0; t2 = 1.0f;
                apos = pt2m;
                apos2 = pt0m;
            } else {
                pt0m = pt0;
                pt1m = pt1;
                pt2m = pt2;
            }

            float start = fromSameHG ? t2 : t1;
            float end   = fromSameHG ? t1 : t2;
            float thick = std::clamp(EDGE_THICK * maxLvlNodeScale, 1.0f, EDGE_THICK);
            bool hover = DrawSplineSegmentBezierQuadraticPart(pt0m, pt1m, pt2m, thick, l->style->color, start, end, std::max(l->highlight, highlight));
            if (hover)
                hoverLink = l;

            if (drawArrows) {
                auto arrow = Edge::getArrowHead();
                float arscl = std::clamp(EDGE_THICK * maxLvlNodeScale, 1.0f, EDGE_THICK) * ARROW_SZ;
                if (l->params.foreward) {
                    Vector2 off = Vector2Rotate(Vector2{(float)arrow.width, (float)arrow.height} * 0.5f, angle);
                    DrawTextureEx(arrow, apos - off * arscl, 180.0f * angle / PI, arscl, ColorBrightness(l->style->color, std::max(l->highlight, highlight)));
                }
                if (l->params.backward) {
                    Vector2 off = Vector2Rotate(Vector2{(float)arrow.width, (float)arrow.height} * 0.5f, angle2);
                    DrawTextureEx(arrow, apos2 - off * arscl, 180.0f * angle2 / PI, arscl, ColorBrightness(l->style->color, std::max(l->highlight, highlight)));
                }
            }
            bool drawLabel = l->editing || l->highlight;
            if (drawLabel) {
                float fntsz = FONT_SZ * EDGE_FONT_COEFF;
                auto sz = MeasureTextEx(font, l->style->label.c_str(), fntsz, 0);
                if (Vector2LengthSqr(pt0m - pt2m) > Vector2LengthSqr(sz)) {
                    float angle = atan2(pt2m.y - pt0m.y, pt2m.x - pt0m.x);
                    if (abs(angle) > PI * 0.5f) angle -= (abs(angle)/angle) * PI;
                    Vector2 pos = (apos + apos2) * 0.5f;
                    Vector2 off = -Vector2Rotate(Vector2{sz.x * 0.5f + EDGE_TXT_SPACING * 1.5f, fntsz * EDGE_TXT_OFFSET }, angle);
                    DrawTextPro(font, l->style->label.c_str(), pos + off, Vector2Zero(), angle * RAD2DEG, fntsz, EDGE_TXT_SPACING, WHITE);
                }
            }
            l->highlight = 0.0f;
        }
        highlight = 0.0f;
        return hoverLink;
    }
}