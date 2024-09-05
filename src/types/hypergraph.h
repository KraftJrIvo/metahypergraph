#pragma once

#include <cstddef>
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
            size_t nodesCount();
            int nDrawableNodes = 0;
            int _nDrawableNodesCache = -1;
            float _scaleCache;

            void clear();

            NodePtr addNode(HyperGraphPtr self, const std::string& label, const Color& color, bool via = false, bool hyper = false);
            void removeNode(NodePtr node, bool clear = true);
            void transferNode(HyperGraphPtr self, NodePtr node, bool moveEdges = true);
            EdgePtr addEdge(HyperGraphPtr self, EdgeLinkStyle style, NodePtr from, NodePtr to);
            void removeEdge(EdgePtr edge, bool clear = true);
            void transferEdge(HyperGraphPtr self, EdgePtr edge);
            NodePtr addHyperEdge(HyperGraphPtr self, const EdgeLinksBundle& froms, const EdgeLinksBundle& tos);

            void updateScale(int off);
            void recalcTower(NodePtr in, NodePtr from = nullptr);

            void floydWarshall(Matrix& D);
            void kamadaKawai();
            void reposition(unsigned int seed = 0);
            
            Vector2 getCenter();
            void recenter();
            void move(const Vector2 delta);

            void draw(Vector2 origin, Vector2 offset, float scale, const Font& font, bool physics, NodePtr grabbedNode, NodePtr& hoverNode, EdgeLinkHoverPtr& hoverEdgeLink);
            NodePtr getNodeAt(Vector2 pos, const std::set<NodePtr>& except);

        private:
            std::map<size_t, NodePtr> _nodes;
            std::map<size_t, EdgePtr> _edges;

            void _reindex();
    };

}