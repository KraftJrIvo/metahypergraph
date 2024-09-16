#pragma once

#include <mutex>
#include <deque>
#include <string>

#include "base.h"
#include "edge.h"
#include "hypergraph.h"
#include "raylib.h"

namespace mhg {

    enum class MHGactionType { SEP, NODE, EDGE, MOVE, TRANSFER };  
    struct MHGaction {
        MHGactionType type = MHGactionType::SEP;
        bool inverse = false;
        bool change = false;
        HyperGraphPtr hg, from;
        NodePtr n;
        EdgePtr e;
        EdgeLinkStylePtr els;
        EdgeLinkParams elp = EdgeLinkParams{};
        Vector2 prv, cur;
        std::string prvLabel, curLabel;
        Color prvColor, curColor;
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
            EdgePtr addEdge(EdgeLinkStylePtr style, NodePtr from, NodePtr to, const EdgeLinkParams& params = {});
            void removeEdge(EdgePtr edge);
            void reduceEdge(EdgePtr edge);
            NodePtr addHyperEdge(const EdgeLinksBundle& froms, const EdgeLinksBundle& tos);
            NodePtr makeEdgeHyper(EdgePtr edge);

            void reposition(unsigned int seed = 0);
            Vector2 getCenter();

            void undo();
            void redo();

            void draw(Vector2 offset, float scale, const Font& font, const std::map<NodePtr, std::pair<Vector2, Vector2>>& selectedNodes, NodePtr& hoverNode, EdgeLinkPtr& hoverEdgeLink);
            NodePtr getNodeAt(Vector2 pos, const std::set<NodePtr>& except);

            void noticeAction(const MHGaction& action, bool sep = true);

        private:
            HyperGraphPtr _root;

            std::deque<MHGaction> _history;
            std::deque<MHGaction>::iterator _histIt;
            bool _historyRecording = false;

            bool _physicsEnabled = false;

            std::mutex _lock;

            void _addNode(NodePtr node);
            void _addEdge(EdgePtr edge);
            void _doAction(const MHGaction& action, bool inverse);
    };
}