#pragma once

#include <array>
#include <cstddef>
#include <list>
#include <algorithm>
#include <memory>
#include <set>
#include <string_view>

#include "base.h"
#include "node.h"
#include "raylib.h"

#define col2num(color) (256 * 256 * size_t(color.r) + 256 * size_t(color.g) + size_t(color.b))

namespace mhg {

    struct EdgeLinkStyle {
        Color color = RED;
        std::string label = "";
    };
    typedef std::shared_ptr<EdgeLinkStyle> EdgeLinkStylePtr;

    struct EdgeLinkParams {
        float weight = 1.0f;
        bool foreward = true;
        bool backward = false;
    };

    struct EdgeLink {
        HyperGraphPtr hg = nullptr;
        size_t eIdx = -1;
        EdgeLinkStylePtr style;
        mutable EdgeLinkParams params;
        float highlight = 0.0f;
        bool editing = false;
        EdgePtr edge();
    };
    typedef std::shared_ptr<EdgeLink> EdgeLinkPtr;

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

        Edge(HyperGraphPtr hg, size_t idx, EdgeLinkStylePtr style, NodePtr from, NodePtr via, NodePtr to) :
            hg(hg), idx(idx), links({std::make_shared<EdgeLink>(EdgeLink{.hg = hg, .eIdx = idx, .style = style})}), from(from), via(via), to(to)
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
    };

}