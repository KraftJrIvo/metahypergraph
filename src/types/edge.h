#pragma once

#include <array>
#include <cstddef>
#include <list>
#include <algorithm>
#include <memory>
#include <set>
#include <string>
#include <string_view>

#include "base.h"
#include "node.h"
#include "raylib.h"

#define col2num(color) (256 * 256 * size_t(color.r) + 256 * size_t(color.g) + size_t(color.b))

namespace mhg {

    struct EdgeLinkStyle;
    typedef std::shared_ptr<EdgeLinkStyle> EdgeLinkStylePtr;
    struct EdgeLinkStyle {
        Color color = RED;
        std::string label = "";
        static EdgeLinkStylePtr create() {
            return std::make_shared<EdgeLinkStyle>();
        }
        static EdgeLinkStylePtr create(Color color, std::string label) {
            return std::make_shared<EdgeLinkStyle>(EdgeLinkStyle{color, label});
        }
    };

    struct EdgeLinkParams {
        float weight = 1.0f;
        bool foreward = true;
        bool backward = false;
    };

    struct EdgeLink;
    typedef std::shared_ptr<EdgeLink> EdgeLinkPtr;
    struct EdgeLink {
        EdgePtr edge;
        EdgeLinkStylePtr style;
        mutable EdgeLinkParams params;
        float highlight = 0.0f;
        bool editing = false;
        EdgeLink(EdgePtr edge, EdgeLinkStylePtr style, EdgeLinkParams params = {}) : edge(edge), style(style), params(params) 
        { }
        static EdgeLinkPtr create(EdgePtr edge, EdgeLinkStylePtr style, EdgeLinkParams params = {}) {
            return std::make_shared<EdgeLink>(edge, style, params);
        }
        static EdgeLinkPtr create(EdgeLinkPtr el) {
            return std::make_shared<EdgeLink>(el->edge, el->style, el->params);
        }
    };

    typedef std::list<std::pair<EdgeLinkStylePtr, NodePtr>> EdgeLinksBundle;
    typedef std::set<EdgeLinkPtr> EdgeLinks;

    bool operator==(const EdgeLinkPtr& lhs, const EdgeLinkPtr& rhs);
    bool operator<(const EdgeLinkPtr& lhs, const EdgeLinkPtr& rhs);

    struct Edge {
        HyperGraphPtr hg = nullptr;
        size_t idx;
        EdgeLinks links;
        NodePtr from;
        NodePtr via;
        NodePtr to;

        float highlight = 0.0f;

        static Texture2D getArrowHead();

        Edge(HyperGraphPtr hg, size_t idx, NodePtr from, NodePtr via, NodePtr to) :
            hg(hg), idx(idx), from(from), via(via), to(to)
        { }

        bool similar(EdgePtr edge);
        void fuse(EdgePtr edge);
        void reduce(EdgePtr edge);

        void reindex(HyperGraphPtr hg, size_t idx);

        void findArrowPositionBezier(Vector2 p0, Vector2 c1, Vector2 p2, bool atStart, float scale, Vector2& at, float& angle, float& t);
        bool DrawSplineSegmentBezierQuadraticPart(Vector2 p0, Vector2 c1, Vector2 p2, float thick, Color color, float start, float end, float highlight);
        Vector2 getPoint(Vector2 p0, Vector2 c1, Vector2 p2, float t);
        void reposition();

        EdgeLinkPtr draw(Vector2 origin, Vector2 offset, float s, const Font& font, bool physics);

        static EdgePtr create(HyperGraphPtr hg, size_t idx, NodePtr from, NodePtr via, NodePtr to) {
            return std::make_shared<Edge>(hg, idx, from, via, to);
        }
    };

}