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

        EdgeLinkStyle stl = {};
        EdgeLinkStyle stl2 = {.color = YELLOW};
        EdgeLinkStyle stl3 = {.color = GREEN};

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
        addEdge(stl, a, b);
        addEdge(stl, b, a);
        addEdge(stl2, a, b);
        addEdge(stl3, a, b);
        addEdge(stl, b, c);
        addEdge(stl, c, a);
        addEdge(stl, aa, bb);
        addEdge(stl, bb, cc);
        addEdge(stl, cc, aa);        
        addEdge(stl, aaa, bbb);
        addEdge(stl, bbb, ccc);
        addEdge(stl, ccc, aaa);        
        addEdge(stl, aa2, bb2);
        auto a2 = addNode("A2", RED);
        auto a3 = addNode("A3", RED, a2);
        auto b2 = addNode("B", RED);
        auto c2 = addNode("C", RED);
        EdgeLinksBundle froms0 = {
            {stl, c},
        };
        EdgeLinksBundle tos0 = {
            {stl, a2},
            {stl, b2},
            {stl, c2},
        };
        addHyperEdge(froms0, tos0);
        addEdge(stl, aaa, a3);
        addEdge(stl, a3, bbb);

        auto m1 = addNode("M1", RED, b2);
        auto m2 = addNode("M2", RED, b2);
        auto m3 = addNode("M3", RED, b2);
        auto m4 = addNode("M4", RED, b2);
        auto m5 = addNode("M5", RED, b2);
        addEdge(stl, m1, m2);
        addEdge(stl, m2, m3);
        addEdge(stl, m3, m4);
        addEdge(stl, m4, m5);
        addEdge(stl, m5, m1);

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

    EdgePtr MetaHyperGraph::addEdge(EdgeLinkStyle style, NodePtr from, NodePtr to) {
        auto hg = (from->hg->lvl > to->hg->lvl) ? from->hg : to->hg;
        return hg->addEdge(hg, style, from, to);
    }

    void MetaHyperGraph::removeEdge(EdgePtr edge) {
        edge->via->hg->removeEdge(edge);
    }

    NodePtr MetaHyperGraph::addHyperEdge(const EdgeLinksBundle& froms, const EdgeLinksBundle& tos) {
        int maxLvl = 0;
        NodePtr maxLvlNode = nullptr;
        for (auto& from : froms) {
            if (from.second->hg->lvl > maxLvl) {
                maxLvl = from.second->hg->lvl;
                maxLvlNode = from.second;
            }
        }
        for (auto& to : tos) {
            if (to.second->hg->lvl > maxLvl) {
                maxLvl = to.second->hg->lvl;
                maxLvlNode = to.second;
            }
        }
        auto hg = maxLvlNode ? maxLvlNode->hg : _root;
        auto hyperNode = hg->addHyperEdge(hg, froms, tos);
        return hyperNode;
    }

    void MetaHyperGraph::reposition(unsigned int seed) {
        _root->reposition(seed);
    }

    Vector2 MetaHyperGraph::getCenter() {
        return _root->getCenter();
    }

    void MetaHyperGraph::draw(Vector2 offset, float scale, const Font& font, NodePtr grabbedNode, NodePtr& hoverNode, EdgeLinkHoverPtr& hoverEdgeLink) {
        _lock.lock();
        _root->draw(Vector2Zero(), offset, scale, font, physicsEnabled, grabbedNode, hoverNode, hoverEdgeLink);
        _lock.unlock();
    }

    NodePtr MetaHyperGraph::getNodeAt(Vector2 pos, const std::set<NodePtr>& except) {
        return _root->getNodeAt(pos, except);
    }
}