#pragma once

#include <algorithm>
#include <string>
#include <set>

#include "raylib.h"
#include "raymath.h"

#include "base.h"
#include "../util/vec_ops.h"

namespace mhg {

    struct Node {
        HyperGraphPtr hg = nullptr;
        HyperGraphPtr content = nullptr;
        size_t idx = -1;
        std::string label;
        Color color;
        bool via;
        bool hyper;

        std::set<EdgePtr> edgesIn;
        std::set<EdgePtr> edgesOut;

        Vector2 pos = Vector2Zero();
        Vector2 _posCache;
        float _rCache;

        Node(HyperGraphPtr hg, size_t idx, const std::string& label, const Color& color, bool via = false, bool hyper = false) :
            hg(hg), idx(idx), label(label), color(color), via(via), hyper(hyper)
        { }

        float getDistToBorder(float angle, float scale);

        void predraw(Vector2 origin, Vector2 offset, float scale, const Font& font);
        bool draw(Vector2 orign, Vector2 offset, float scale, const Font& font);
    };

}