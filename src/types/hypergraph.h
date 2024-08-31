#pragma once

#include <list>
#include <map>

#include "edge.h"
#include "raylib.h"

namespace mhg {

    class HyperGraph {
        public:
            HyperGraph(NodePtr parent = nullptr) : 
                parent(parent), lvl(parent ? (parent->hg->lvl + 1) : 0)
            { }

            NodePtr parent = nullptr;
            int lvl = 0;
            float coeff();
            float scale();
            int nDrawableNodes = 0;
            int _nDrawableNodesCache = -1;
            float _scaleCache;

            void clear();

            NodePtr addNode(HyperGraphPtr self, const std::string& label, const Color& color, bool via = false, bool hyper = false);
            EdgePtr addEdge(HyperGraphPtr self, EdgeType type, NodePtr from, NodePtr to);
            NodePtr addHyperEdge(HyperGraphPtr self, EdgeType type, NodePtr from, const std::list<std::pair<EdgeType, NodePtr>>& tos);

            void recalcTower(NodePtr in, NodePtr from = nullptr);

            void floydWarshall(Matrix& D);
            void kamadaKawai();
            void reposition(unsigned int seed = 0);
            
            Vector2 getCenter();
            void recenter();
            void move(const Vector2 delta);

            void draw(Vector2 origin, Vector2 offset, float scale, const Font& font, bool physics, NodePtr& hoverNode);

        private:
            std::map<size_t, NodePtr> _nodes;
            std::map<size_t, EdgePtr> _edges;

            void _reindex();
    };

}