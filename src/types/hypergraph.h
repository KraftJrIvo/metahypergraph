#pragma once

#include <cstddef>
#include <list>
#include <map>

#include "base.h"
#include "edge.h"
#include "metahypergraph.h"
#include "raylib.h"

namespace mhg {

    class MetaHyperGraph;
    class HyperGraph {
        public:
            HyperGraph(MetaHyperGraph& pmhg, NodePtr parent = nullptr) : 
                pmhg(pmhg), parent(parent), lvl(parent ? (parent->hg->lvl + 1) : 0)
            { }

            MetaHyperGraph& pmhg;
            HyperGraphPtr self = nullptr;
            NodePtr parent = nullptr;
            int lvl = 0;
            float coeff();
            float scale();
            size_t nodesCount();
            int nDrawableNodes = 0;
            int _nDrawableNodesCache = -1;
            float _scaleCache;
            Vector2 _scaledOcache;

            bool isChildOf(HyperGraphPtr hg);
            void clear();
            void removeOuterEdges(HyperGraphPtr hg);
            void checkForTransferEdges(NodePtr node);

            NodePtr addNode(const std::string& label, const Color& color, bool via = false, bool hyper = false);
            void addNode(NodePtr node);
            void removeNode(NodePtr node, bool removeOuterEdges = true);
            void transferNode(NodePtr node, bool moveEdges = true);
            EdgePtr addEdge(EdgeLinkStylePtr style, NodePtr from, NodePtr to, const EdgeLinkParams& params = {});
            void addEdge(EdgePtr edge);
            void removeEdge(EdgePtr edge, bool clear = true);
            void reduceEdge(EdgePtr edge, bool clear = true);
            void transferEdge(EdgePtr edge);
            NodePtr addHyperEdge(const EdgeLinksBundle& froms, const EdgeLinksBundle& tos);
            NodePtr makeEdgeHyper(EdgePtr edge);

            void updateScale(int off);
            void recalcTower(NodePtr in, NodePtr from = nullptr);

            void floydWarshall(Matrix& D);
            void kamadaKawai();
            void reposition(unsigned int seed = 0);
            
            Vector2 getCenter();
            void recenter();
            void move(const Vector2 delta);

            void draw(Vector2 origin, Vector2 offset, float scale, const Font& font, bool physics, const std::map<NodePtr, std::pair<Vector2, Vector2>>& selectedNodes, NodePtr& hoverNode, EdgeLinkPtr& hoverEdgeLink);
            NodePtr getNodeAt(Vector2 pos, const std::set<NodePtr>& except);

            NodePtr getNode(size_t idx) {return _nodes.count(idx) ? _nodes.at(idx) : nullptr;}
            EdgePtr getEdge(size_t idx) {return _edges.count(idx) ? _edges.at(idx) : nullptr;}
    
    private:
            std::map<size_t, NodePtr> _nodes;
            std::map<size_t, EdgePtr> _edges;

            void _reindex();
    };

}