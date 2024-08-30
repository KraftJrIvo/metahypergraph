#include "hypermetagraph.h"
#include "raylib.h"
#include "raymath.h"

#include <cmath>
#include <cstddef>
#include <cstdlib>
#include <memory>
#include <string>

namespace hmg {
    void HyperMetaGraph::clear() {
        _lock.lock();
        _nodes.clear();
        _edges.clear();
        _lock.unlock();
    }

    void HyperMetaGraph::init() {
        clear();

        auto a = addNode("КУЗДРА", RED);
        auto aa = addNode("AA", RED, a);
        auto bb = addNode("BB", RED, a);
        auto cc = addNode("CC", RED, a);
        auto b = addNode("B", RED);
        auto c = addNode("C", RED);
        addEdge(hmg::EdgeType::TARGET, a, b);
        addEdge(hmg::EdgeType::TARGET, b, c);
        addEdge(hmg::EdgeType::TARGET, c, a);
        addEdge(hmg::EdgeType::TARGET, aa, bb);
        addEdge(hmg::EdgeType::TARGET, bb, cc);
        addEdge(hmg::EdgeType::TARGET, cc, aa);

        positionInitially(0);
    }

    NodePtr HyperMetaGraph::addNode(const std::string &label, const Color &color, NodePtr parent) {
        _lock.lock();
        size_t idx = _nodes.size();
        auto node = std::make_shared<Node>(idx, label, color, parent);
        _nodes[idx] = node;
        if (parent) {
            parent->subNodes.insert(node);
            for (auto& sn : parent->subNodes)
                sn->recalc();
        } else {
            node->recalc();
        }
        maxLvl = std::max(maxLvl, node->lvl);
        _lock.unlock();
        return node;
    }

    EdgePtr HyperMetaGraph::addEdge(EdgeType type, NodePtr from, NodePtr to) {
        _lock.lock();
        size_t idx = _nodes.size();
        auto via = std::make_shared<Node>(idx, "", BLANK, nullptr, true);
        _nodes[idx] = via;
        idx = _edges.size();
        auto edge = std::make_shared<Edge>(idx, type, from, via, to);
        edge->recalc();
        _edges[idx] = edge;
        from->edgesOut.insert(edge);
        to->edgesIn.insert(edge);
        _lock.unlock();
        return edge;
    }

    NodePtr HyperMetaGraph::addHyperEdge(EdgeType type, NodePtr from, const std::list<std::pair<EdgeType, NodePtr>>& tos) {
        _lock.lock();
        size_t idx = _edges.size();
        auto hyperVia = std::make_shared<Node>(idx, "", Edge::color(type), nullptr, true, true);
        _nodes[idx] = hyperVia;
        addEdge(type, from, hyperVia);
        for (auto& to : tos)
            addEdge(to.first, hyperVia, to.second);
        _lock.unlock();
        return hyperVia;
    }

    void HyperMetaGraph::positionInitially(unsigned int seed) {
        _lock.lock();
        if (seed) 
            srand(seed);
        float angle;
        for (int lvl = 0; lvl <= maxLvl; ++lvl) {
            for (auto& n : _nodes) {
                if (n.second->lvl == lvl) {
                    if (lvl > 0)
                        n.second->pos = n.second->parent->pos;
                    angle = 2.0f * 3.14159f * RAND_FLOAT;
                    n.second->pos = 0.5f * n.second->radius * Vector2{ cos(angle), sin(angle) };
                }
            }
        }
        kamadaKawai();
        for (auto& e : _edges)
            e.second->reposition();
        _lock.unlock();
    }

    void HyperMetaGraph::doPhysics() {
        _physics.step(_nodes);
    }

    void HyperMetaGraph::grabNode(NodePtr node, const Vector2& off) {
        _grabbedNode = node;
        _grabOff = off;
    }

    void HyperMetaGraph::ungrabNode() {
        _grabbedNode = nullptr;
        _grabOff = Vector2Zero();
    }

    void HyperMetaGraph::dragNode(Vector2 offset, float scale, const Vector2& mpos) {
        auto newPos = (mpos - offset) / scale + _grabOff / scale;
        _grabbedNode->moveTo(newPos);
    }

    void HyperMetaGraph::_reindex() {
        size_t i = 0;
        for (auto n : _nodes)
            n.second->idx = i++;
        i = 0;
        for (auto e : _edges)
            e.second->idx = i++;
    }

    Vector2 HyperMetaGraph::getCenter() {
        Vector2 acc = Vector2Zero();
        for (auto n : _nodes)
            acc += n.second->pos;
        return acc / _nodes.size();
    }

    void HyperMetaGraph::draw(Vector2 offset, float scale, const Font& font, NodePtr& hoverNode) {
        _lock.lock();
        for (int lvl = 0; lvl <= maxLvl; ++lvl) {
            for (auto& n : _nodes)
                if (!n.second->via && n.second->lvl == lvl)
                    n.second->predraw(offset, scale, font);
            for (auto& e : _edges)
                if (e.second->lvl == lvl)
                    e.second->draw(offset, scale, font);
            for (auto& n : _nodes) {
                if (!n.second->via && n.second->lvl == lvl) {
                    bool hover = n.second->draw(offset, scale, font);            
                    if (hover)
                        hoverNode = n.second;
                }
            }
        }
        _lock.unlock();
    }
}