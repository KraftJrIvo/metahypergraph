#pragma once

#include <mutex>
#include <deque>

#include "hypergraph.h"
#include "raylib.h"

namespace mhg {

    enum class MHGactionType { SEP, NODE, EDGE, MOVE, TRANSFER };  
    struct MHGaction {
        MHGactionType type;
        bool inverse;
        HyperGraphPtr hg, from;
        NodePtr n;
        EdgePtr e;
        EdgeLinkStyle els;
        Vector2 prv, cur;
    };

    class DrawerImpl;
    class MetaHyperGraph {
        friend class DrawerImpl;
        public:
            void clear();
            void init();

            NodePtr addNode(const std::string& label, const Color& color, NodePtr parent = nullptr);
            void removeNode(NodePtr node);
            void moveNode(NodePtr node, Vector2 prvPos, Vector2 newPos);
            void transferNode(HyperGraphPtr to, NodePtr node);
            EdgePtr addEdge(EdgeLinkStyle style, NodePtr from, NodePtr to);
            void removeEdge(EdgePtr edge);
            void reduceEdge(EdgePtr edge);
            NodePtr addHyperEdge(const EdgeLinksBundle& froms, const EdgeLinksBundle& tos);
            NodePtr makeEdgeHyper(EdgePtr edge);

            void reposition(unsigned int seed = 0);
            Vector2 getCenter();

            void undo();
            void redo();

            void draw(Vector2 offset, float scale, const Font& font, NodePtr grabbedNode, NodePtr& hoverNode, EdgeLinkHoverPtr& hoverEdgeLink);
            NodePtr getNodeAt(Vector2 pos, const std::set<NodePtr>& except);

        private:
            HyperGraphPtr _root;

            std::deque<MHGaction> _history = { MHGaction{.type = MHGactionType::SEP} };
            std::deque<MHGaction>::iterator _histIt = _history.begin();
            bool _historyRecording = false;

            bool _physicsEnabled = false;

            std::mutex _lock;

            void _addNode(NodePtr node);
            void _addEdge(EdgePtr edge);
            void _noticeAction(const MHGaction& action, bool sep = true);
            void _doAction(const MHGaction& action, bool inverse);
    };
}