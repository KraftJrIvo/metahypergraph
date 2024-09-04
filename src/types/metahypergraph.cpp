#include "metahypergraph.h"
#include "raylib.h"
#include "raymath.h"

#include <memory>
#include <string>

namespace mhg {
    void MetaHyperGraph::clear() {
        _root->clear();
    }

    void MetaHyperGraph::init() {
        _lock.lock();
        if (!_root)
            _root = std::make_shared<HyperGraph>();
        clear();

        auto a = addNode("A1", RED);
        auto aa = addNode("AA", RED, a);
        auto aaa = addNode("AAA", RED, aa);
        auto bbb = addNode("BBB", RED, aa);
        auto ccc = addNode("CCC", RED, aa);
        auto bb = addNode("BB", RED, a);
        auto cc = addNode("CC", RED, a);
        auto b = addNode("B", RED);
        auto aa2 = addNode("AA", RED, b);
        auto bb2 = addNode("BB", RED, b);
        auto c = addNode("C", RED);
        addEdge(mhg::EdgeType::TARGET, a, b);
        addEdge(mhg::EdgeType::TARGET, b, a);
        addEdge(mhg::EdgeType::TARGET, b, c);
        addEdge(mhg::EdgeType::TARGET, c, a);
        addEdge(mhg::EdgeType::TARGET, aa, bb);
        addEdge(mhg::EdgeType::TARGET, bb, cc);
        addEdge(mhg::EdgeType::TARGET, cc, aa);        
        addEdge(mhg::EdgeType::TARGET, aaa, bbb);
        addEdge(mhg::EdgeType::TARGET, bbb, ccc);
        addEdge(mhg::EdgeType::TARGET, ccc, aaa);        
        addEdge(mhg::EdgeType::TARGET, aa2, bb2);
        auto a2 = addNode("A2", RED);
        auto a3 = addNode("A3", RED, a2);
        auto b2 = addNode("B", RED);
        auto c2 = addNode("C", RED);
        std::list<std::pair<EdgeType, NodePtr>> tos0 = {
            {mhg::EdgeType::TARGET, a2},
            {mhg::EdgeType::TARGET, b2},
            {mhg::EdgeType::TARGET, c2},
        };
        addHyperEdge(mhg::EdgeType::TARGET, c, tos0);
        addEdge(mhg::EdgeType::TARGET, aaa, a3);
        addEdge(mhg::EdgeType::TARGET, a3, bbb);

        auto m1 = addNode("M1", RED, b2);
        auto m2 = addNode("M2", RED, b2);
        auto m3 = addNode("M3", RED, b2);
        auto m4 = addNode("M4", RED, b2);
        auto m5 = addNode("M5", RED, b2);
        addEdge(mhg::EdgeType::TARGET, m1, m2);
        addEdge(mhg::EdgeType::TARGET, m2, m3);
        addEdge(mhg::EdgeType::TARGET, m3, m4);
        addEdge(mhg::EdgeType::TARGET, m4, m5);
        addEdge(mhg::EdgeType::TARGET, m5, m1);

        reposition();
        _lock.unlock();
    }

    NodePtr MetaHyperGraph::addNode(const std::string &label, const Color &color, NodePtr parent) {
        auto hg = _root;
        if (parent) {
            if (!parent->content)
                parent->content = std::make_shared<HyperGraph>(parent);
            hg = parent->content;
        }
        return hg->addNode(hg, label, color);
    }

    void MetaHyperGraph::removeNode(NodePtr node) {
        node->hg->removeNode(node);
    }

    EdgePtr MetaHyperGraph::addEdge(EdgeType type, NodePtr from, NodePtr to) {
        auto hg = (from->hg->lvl > to->hg->lvl) ? from->hg : to->hg;
        return hg->addEdge(hg, type, from, to);
    }

    void MetaHyperGraph::removeEdge(EdgePtr edge) {
        edge->via->hg->removeEdge(edge);
    }

    NodePtr MetaHyperGraph::addHyperEdge(EdgeType type, NodePtr from, const std::list<std::pair<EdgeType, NodePtr>>& tos) {
        int maxLvl = 0;
        NodePtr maxLvlToNode = nullptr;
        for (auto& to : tos) {
            if (to.second->hg->lvl > maxLvl) {
                maxLvl = to.second->hg->lvl;
                maxLvlToNode = to.second;
            }
        }
        auto hmg = (!maxLvlToNode || from->hg->lvl > maxLvlToNode->hg->lvl) ? from->hg : maxLvlToNode->hg;
        return hmg->addHyperEdge(hmg, type, from, tos);
    }

    void MetaHyperGraph::reposition(unsigned int seed) {
        _root->reposition(seed);
    }

    void MetaHyperGraph::grabNode(NodePtr node, const Vector2& off) {
        _grabbedNode = node;
        _grabOff = off;
    }

    void MetaHyperGraph::ungrabNode() {
        _grabbedNode = nullptr;
        _grabOff = Vector2Zero();
    }

    void MetaHyperGraph::dragNode(Vector2 offset, float scale, const Vector2& mpos) {
        float ls = _grabbedNode->hg->scale() * scale;
        _grabbedNode->pos = ((mpos - offset) + _grabOff) / ls;
    }

    Vector2 MetaHyperGraph::getCenter() {
        return _root->getCenter();
    }

    void MetaHyperGraph::draw(Vector2 offset, float scale, const Font& font, NodePtr& hoverNode, EdgePtr& hoverEdge) {
        _lock.lock();
        _root->draw(Vector2Zero(), offset, scale, font, physicsEnabled, _grabbedNode, hoverNode, hoverEdge);
        _lock.unlock();
    }

    NodePtr MetaHyperGraph::getNodeAt(Vector2 pos, const std::set<NodePtr>& except) {
        return _root->getNodeAt(pos, except);
    }
}