#include "hypergraph.h"
#include "raylib.h"

namespace mhg {

    Texture2D Edge::getArrowHead() {
        static Texture2D tex;
        static bool loaded = false;
        if (loaded)
            return tex;
        tex = LoadTexture("res/arrowhead.png");
        loaded = true;
        return tex;
    }

    Vector2 Edge::getPoint(float t) {
        float x = pow(1 - t, 2) * _pts[0].x + 2 * t * (1 - t) * _pts[1].x + t * t * _pts[2].x;
        float y = pow(1 - t, 2) * _pts[0].y + 2 * t * (1 - t) * _pts[1].y + t * t * _pts[2].y;
        return Vector2{ x, y};
    }

    void Edge::reposition() {
        via->pos = 0.5f * (from->pos + to->pos);
    }

    void Edge::draw(Vector2 origin, Vector2 offset, float gs, float ls, const Font& font) {
        NodePtr minLvlNode = (from->hg->lvl < to->hg->lvl) ? from : to;
        NodePtr maxLvlNode = (from->hg->lvl > to->hg->lvl) ? from : to;
        bool fromSameLvl = (from->hg->lvl == via->hg->lvl);
        bool toSameLvl = (to->hg->lvl == via->hg->lvl);

        _pts[0] = fromSameLvl ? (origin + ls * from->pos + offset) : from->_posCache;
        _pts[2] = toSameLvl ? (origin + ls * to->pos + offset) : to->_posCache;
        _pts[1] = (!fromSameLvl || !toSameLvl) ? (0.5f * (_pts[0] + _pts[2])) : (origin + ls * via->pos + offset);

        float thick = std::clamp(EDGE_THICK * maxLvlNode->_lsCache, 1.0f, EDGE_THICK);

        Vector2 apos; float angle; float t1, t2; 
        findArrowPositionBezier(false, minLvlNode->_lsCache, apos, angle, t1);
        if (!fromSameLvl) {
            Vector2 apos2; float angle2;
            findArrowPositionBezier(true, minLvlNode->_lsCache, apos2, angle2, t2);
        }

        float start = fromSameLvl ? 0.0f : t1;
        float end   = fromSameLvl ? t1 : t2;
        DrawSplineSegmentBezierQuadraticPart(_pts[0], _pts[1], _pts[2], thick, RED, start, end);

        bool drawArrows = (maxLvlNode->_lsCache > HIDE_ARROW_SCALE);
        if (drawArrows) {
            auto arrow = Edge::getArrowHead();
            float arscl = std::clamp(EDGE_THICK * maxLvlNode->_lsCache, 1.0f, EDGE_THICK) * ARROW_SZ;
            Vector2 off = Vector2Rotate(Vector2{(float)arrow.width, (float)arrow.height} * 0.5f, angle);
            angle = 180.0f * angle / PI;
            DrawTextureEx(arrow, apos - off * arscl, angle, arscl, RED);
        }
    }
}