#include "hypergraph.h"
#include "raylib.h"
#include <cstddef>

namespace mhg {

    void HyperGraph::clear() {
        _nodes.clear();
        _edges.clear();
    }

    float HyperGraph::localScale() {
        return parent ? std::pow(nDrawableNodes + 1, -lvl) : 1.0f;
    }

    NodePtr HyperGraph::addNode(HyperGraphPtr self, const std::string &label, const Color &color, bool via, bool hyper) {
        size_t idx = _nodes.size();
        auto node = std::make_shared<Node>(self, idx, label, color, via, hyper);
        _nodes[idx] = node;
        if (!via)
            nDrawableNodes++;
        return node;
    }

    EdgePtr HyperGraph::addEdge(HyperGraphPtr self, EdgeType type, NodePtr from, NodePtr to) {
        auto via = addNode(self, "", BLANK, true);
        size_t idx = _edges.size();
        auto edge = std::make_shared<Edge>(idx, type, from, via, to);
        _edges[idx] = edge;
        from->edgesOut.insert(edge);
        to->edgesIn.insert(edge);
        return edge;
    }

    NodePtr HyperGraph::addHyperEdge(HyperGraphPtr self, EdgeType type, NodePtr from, const std::list<std::pair<EdgeType, NodePtr>>& tos) {
        auto hyperVia = addNode(self, "", BLANK, false, true);
        addEdge(self, type, from, hyperVia);
        for (auto& to : tos)
            addEdge(self, to.first, hyperVia, to.second);
        return hyperVia;
    }

    void HyperGraph::reposition(unsigned int seed) {
        if (seed) 
            srand(seed);
        float angle;
        for (auto& n : _nodes) {
            angle = 2.0f * 3.14159f * RAND_FLOAT;
            n.second->pos = NODE_SZ * Vector2{ cos(angle), sin(angle) };
            if (n.second->content)
                n.second->content->reposition(seed);
        }
        kamadaKawai();
        for (auto& e : _edges)
            e.second->reposition();
        recenter();
    }

    void HyperGraph::_reindex() {
        size_t i = 0;
        for (auto n : _nodes)
            n.second->idx = i++;
        i = 0;
        for (auto e : _edges)
            e.second->idx = i++;
    }

    Vector2 HyperGraph::getCenter() {
        Vector2 acc = Vector2Zero();
        for (auto n : _nodes)
            acc += n.second->pos;
        return acc / _nodes.size();
    }            
    
    void HyperGraph::recenter() {
        move(-getCenter());
    }
    
    void HyperGraph::move(const Vector2 delta) {
        for (auto& n : _nodes)
            n.second->pos += delta;
    }

    void HyperGraph::draw(Vector2 origin, Vector2 offset, float scale, float lsAcc, const Font& font, NodePtr& hoverNode) {
        origin += (parent ? parent->pos : Vector2Zero()) * lsAcc;
        float ls = localScale();
        Vector2 scaledOrigin = origin * scale;
        float gsls = scale * ls;
        for (auto& n : _nodes)
            if (!n.second->via)
                n.second->predraw(scaledOrigin, offset, gsls, font);
        for (auto& e : _edges)
            e.second->draw(scaledOrigin, offset, gsls, font);
        for (auto& n : _nodes) {
            if (!n.second->via && !n.second->hyper) {
                bool hover = n.second->draw(scaledOrigin, offset, gsls, font);            
                if (hover)
                    hoverNode = n.second;
            }
        }
        for (auto& n : _nodes)
            if (n.second->content && gsls > HIDE_CONTENT_SCALE)
                n.second->content->draw(origin, offset, scale, lsAcc * ls, font, hoverNode);
    }

}