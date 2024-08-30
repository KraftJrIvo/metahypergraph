#pragma once

#include "raylib.h"
#include "raymath.h"
#include "types.h"
#include "physics.h"
#include <cstddef>
#include <mutex>
#include <string>

namespace hmg {
    class HyperMetaGraph {
        public:
            void clear();
            void init();

            NodePtr addNode(const std::string& label, const Color& color, NodePtr parent = nullptr);
            EdgePtr addEdge(EdgeType type, NodePtr from, NodePtr to);
            NodePtr addHyperEdge(EdgeType type, NodePtr from, const std::list<std::pair<EdgeType, NodePtr>>& tos);

            void recalcTower(NodePtr in, NodePtr from = nullptr);

            void getDistancesFW(Matrix& D);
            void kamadaKawai();
            void positionInitially(unsigned int seed);

            void doPhysics();

            void grabNode(NodePtr node, const Vector2& off);
            void ungrabNode();
            void dragNode(Vector2 offset, float scale, const Vector2& mpos);
            bool nodeGrabbed() { return (_grabbedNode != nullptr); }

            Vector2 getCenter();

            void draw(Vector2 offset, float scale, const Font& font, NodePtr& hoverNode);

        private:
            std::map<size_t, NodePtr> _nodes;
            std::map<size_t, EdgePtr> _edges;
            int maxLvl = 0;
            PhysicsSolver _physics;

            NodePtr _grabbedNode = nullptr;
            Vector2 _grabOff = Vector2Zero();

            std::mutex _lock;

            void _reindex();
    };
}