#include "hypergraph.h"
#include "raylib.h"
#include <cstddef>

namespace mhg {

    void HyperGraph::clear() {
        _nodes.clear();
        _edges.clear();
    }

    float HyperGraph::coeff() {
        return 1.0f / (nDrawableNodes + 1);
    }

    float HyperGraph::scale() {
        if (!parent)
            return 1.0f;
        if (_nDrawableNodesCache == nDrawableNodes)
            return parent->hg->scale() * _scaleCache;
        _nDrawableNodesCache = nDrawableNodes;
        _scaleCache = coeff();
        return parent->hg->scale() * _scaleCache;
    }

    NodePtr HyperGraph::addNode(HyperGraphPtr self, const std::string &label, const Color &color, bool via, bool hyper) {
        size_t idx = _nodes.size();
        auto node = std::make_shared<Node>(self, idx, label, color, via, hyper);
        if (!via) {
            float preCoeff = scale();
            nDrawableNodes++;
            float aftCoeff = scale();
            for (auto& n : _nodes)
                n.second->pos = n.second->pos * preCoeff / aftCoeff;
        }
        _nodes[idx] = node;
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

    void HyperGraph::draw(Vector2 origin, Vector2 offset, float s, const Font& font, bool physics, NodePtr& hoverNode) {
        origin += (parent ? (parent->hg->scale() * parent->pos) : Vector2Zero());
        Vector2 scaledOrigin = origin * s;
        for (auto& n : _nodes)
            if (!n.second->via)
                n.second->predraw(scaledOrigin, offset, s, font);
        for (auto& e : _edges)
            e.second->draw(scaledOrigin, offset, s, font, physics);
        for (auto& n : _nodes) {
            if (!n.second->via && !n.second->hyper) {
                bool hover = n.second->draw(scaledOrigin, offset, s, font);            
                if (hover)
                    hoverNode = n.second;
            }
        }
        for (auto& n : _nodes)
            if (n.second->content && s * scale() > HIDE_CONTENT_SCALE)
                n.second->content->draw(origin, offset, s, font, physics, hoverNode);
    }

}