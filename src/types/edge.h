#pragma once

#include <array>
#include <cstddef>
#include <list>
#include <algorithm>
#include <memory>
#include <set>
#include <string_view>

#include "node.h"
#include "raylib.h"

#define col2num(color) (256 * 256 * size_t(color.r) + 256 * size_t(color.g) + size_t(color.b))

namespace mhg {

    struct EdgeLinkStyle {
        Color color = RED;
        std::string label = "";
        mutable float weight = 1.0f;
        mutable bool foreward = true;
        mutable bool backward = false;
        mutable float highlight = 0.0f;

        bool operator==(const EdgeLinkStyle& rhs) const {
            return color.r == rhs.color.r && color.g == rhs.color.g && color.b == rhs.color.b && color.a == rhs.color.a && label == rhs.label; 
        }
        bool operator<(const EdgeLinkStyle& rhs) const {
            size_t c1 = col2num(color);
            size_t c2 = col2num(rhs.color);
            return (label < rhs.label) || (label == rhs.label && c1 < c2) || (label == rhs.label && c1 == c2 && weight < rhs.weight);
        }
    };

    typedef std::list<std::pair<EdgeLinkStyle, NodePtr>> EdgeLinksBundle;

    typedef std::shared_ptr<EdgeLinkStyle> EdgeLinkPtr;
    typedef std::pair<EdgePtr, EdgeLinkStyle> EdgeLinkHover;
    typedef std::shared_ptr<EdgeLinkHover> EdgeLinkHoverPtr;

    struct Edge {
        size_t idx;
        std::set<EdgeLinkStyle> links;
        NodePtr from;
        NodePtr via;
        NodePtr to;

        float highlight = 0.0f;

        static Texture2D getArrowHead();

        Edge(size_t idx, EdgeLinkStyle style, NodePtr from, NodePtr via, NodePtr to) :
            idx(idx), links({style}), from(from), via(via), to(to)
        { }

        bool similar(EdgePtr edge);
        void fuse(EdgePtr edge);
        void reduce(EdgePtr edge);

        void findArrowPositionBezier(Vector2 p0, Vector2 c1, Vector2 p2, bool atStart, float scale, Vector2& at, float& angle, float& t);
        bool DrawSplineSegmentBezierQuadraticPart(Vector2 p0, Vector2 c1, Vector2 p2, float thick, Color color, float start, float end, float highlight);
        Vector2 getPoint(Vector2 p0, Vector2 c1, Vector2 p2, float t);
        void reposition();

        EdgeLinkPtr draw(Vector2 origin, Vector2 offset, float s, const Font& font, bool physics);
    };

}