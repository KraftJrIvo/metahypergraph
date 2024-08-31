#pragma once

#include <mutex>

#include "hypergraph.h"

namespace mhg {
    class DrawerImpl;
    class MetaHyperGraph {
        friend class DrawerImpl;
        public:
            void clear();
            void init();

            NodePtr addNode(const std::string& label, const Color& color, NodePtr parent = nullptr);
            EdgePtr addEdge(EdgeType type, NodePtr from, NodePtr to);
            NodePtr addHyperEdge(EdgeType type, NodePtr from, const std::list<std::pair<EdgeType, NodePtr>>& tos);

            void reposition(unsigned int seed = 0);

            void grabNode(NodePtr node, const Vector2& off);
            void ungrabNode();
            void dragNode(Vector2 offset, float scale, const Vector2& mpos);
            bool nodeGrabbed() { return (_grabbedNode != nullptr); }

            Vector2 getCenter();

            void draw(Vector2 offset, float scale, const Font& font, NodePtr& hoverNode);

        private:
            HyperGraphPtr _root;

            NodePtr _grabbedNode = nullptr;
            Vector2 _grabOff = Vector2Zero();

            bool physicsEnabled = false;

            std::mutex _lock;
    };
}