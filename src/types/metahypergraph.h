#pragma once

#include <mutex>

#include "hypergraph.h"
#include "raylib.h"

namespace mhg {
    class DrawerImpl;
    class MetaHyperGraph {
        friend class DrawerImpl;
        public:
            void clear();
            void init();

            NodePtr addNode(const std::string& label, const Color& color, NodePtr parent = nullptr);
            void removeNode(NodePtr node);
            EdgePtr addEdge(EdgeLinkStyle style, NodePtr from, NodePtr to);
            void removeEdge(EdgePtr edge);
            NodePtr addHyperEdge(const EdgeLinksBundle& froms, const EdgeLinksBundle& tos);

            void reposition(unsigned int seed = 0);

            Vector2 getCenter();

            void draw(Vector2 offset, float scale, const Font& font, NodePtr grabbedNode, NodePtr& hoverNode, EdgeLinkHoverPtr& hoverEdgeLink);
            NodePtr getNodeAt(Vector2 pos, const std::set<NodePtr>& except);

        private:
            HyperGraphPtr _root;

            bool physicsEnabled = false;

            std::mutex _lock;
    };
}