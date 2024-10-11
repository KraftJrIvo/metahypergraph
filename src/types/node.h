#pragma once

#include <algorithm>
#include <cstddef>
#include <memory>
#include <string>
#include <set>

#include "raylib.h"
#include "raymath.h"

#include "base.h"
#include "../util/vec_ops.h"

namespace mhg {

    struct NodeParams {
        std::string label;
        Color color;
    };

    struct NodeDrawParams {
        Vector2 pos = Vector2Zero();
        Vector2 posCache;
        float rCache;
        float rCacheStable;
        float highlight = 0.0f;
        bool editing = false;
        int tmpDrawableNodes = 0;
        NodePtr overNode = nullptr;
        bool overRoot = false;
    };

    struct Node {
        HyperGraphPtr hg = nullptr;
        HyperGraphPtr content = nullptr;
        size_t idx = -1;
        bool via;
        bool hyper;

        std::set<EdgePtr> eIn;
        std::set<EdgePtr> eOut;

        NodeParams p;
        NodeDrawParams dp;

        Node(HyperGraphPtr hg, size_t idx, const NodeParams& params, bool via = false, bool hyper = false) :
            hg(hg), idx(idx), p(params), via(via), hyper(hyper)
        { }

        float coeff();        
        size_t nodesCount();
        float scale();

        size_t getMaxLinks();
        size_t getLinksCount();
        EdgePtr getEdgeTo(NodePtr node);
        EdgePtr getSimilarEdge(EdgePtr edge);

        void predraw(Vector2 origin, Vector2 offset, float scale, const Font& font);
        bool draw(Vector2 orign, Vector2 offset, float scale, const Font& font);
        void resetDraw();

        static NodePtr create(HyperGraphPtr hg, size_t idx, const NodeParams& params, bool via = false, bool hyper = false) {
            return std::make_shared<Node>(hg, idx, params, via, hyper);
        }
    };

}