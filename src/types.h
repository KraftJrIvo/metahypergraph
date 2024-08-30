#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <list>
#include <sstream>
#include <vector>
#include <set>
#include <map>

#include "raylib.h"
#include "raymath.h"
#include "vec_ops.h"

#define NODE_SZ 25.0f
#define EDGE_THICK 3.0f
#define SPRING_LEN 150.0f
#define SPRING_STR 0.05f

#define RAND_FLOAT static_cast <float> (rand()) / (static_cast <float> (RAND_MAX) + 1.0f)

#include <Eigen/Core>

namespace hmg {
    typedef Eigen::Matrix<float, -1,  1> Vector;
    typedef Eigen::Matrix<float, -1, -1> Matrix;

    struct Node;
    typedef std::shared_ptr<Node> NodePtr;
    struct Edge;
    typedef std::shared_ptr<Edge> EdgePtr;

    struct Node {
        size_t idx = -1;
        std::string label;
        Color color;
        bool via;
        bool hyper;

        NodePtr parent;
        std::set<NodePtr> subNodes;
        std::set<EdgePtr> edgesIn;
        std::set<EdgePtr> edgesOut;

        Vector2 pos = Vector2Zero();

        Node(size_t idx, const std::string& label, const Color& color, NodePtr parent = nullptr, bool via = false, bool hyper = false) :
            idx(idx), label(label), color(color), via(via), hyper(hyper), parent(parent)
        { }

        int lvl = 0;
        float coeff = 0;
        float radius = 0;
        void recalc();
        int getLevel();
        float getCoeff();
        void recalcTower(NodePtr here, NodePtr from = nullptr);

        void move(const Vector2& delta);
        void moveTo(const Vector2& to);

        void predraw(Vector2 offset, float scale, const Font& font);
        bool draw(Vector2 offset, float scale, const Font& font);
    };

    enum EdgeType {
        PROPERTY,
        ACTION,
        PROCESS,
        TARGET,
        ORDER
    };

    struct Edge {
        size_t idx;
        EdgeType type;
        NodePtr from;
        NodePtr via;
        NodePtr to;
        int lvl = 0;

        Edge(size_t idx, EdgeType type, NodePtr from, NodePtr via, NodePtr to) :
            idx(idx), type(type), from(from), via(via), to(to)
        { }
        
        void recalc() { 
            lvl = via->lvl = std::max(from->lvl, to->lvl);
            via->parent = (lvl == from->lvl) ? from->parent : to->parent;
            if (via->parent)
                via->parent->subNodes.insert(via);
        }

        static Color color(EdgeType type);
        void reposition();


        void draw(Vector2 offset, float scale, const Font& font);
    };
}