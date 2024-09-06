#include "hypergraph.h"
#include "raylib.h"
#include <cstddef>
#include <iterator>
#include <memory>

namespace mhg {

    void HyperGraph::clear() {
        auto nodes = _nodes;
        for (auto& n : nodes)
            removeNode(n.second);
        _nodes.clear();
        _edges.clear();
    }

    float HyperGraph::coeff() {
        return 1.0f / (nDrawableNodes + 1);
    }
    
    size_t HyperGraph::nodesCount() {
        return _nodes.size();
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
        size_t idx = _nodes.size() ? (_nodes.rbegin()->first + 1) : 0;
        auto node = std::make_shared<Node>(self, idx, label, color, via, hyper);
        if (!via && !hyper)
            updateScale(1);
        _nodes[idx] = node;
        return node;
    }
    
    void HyperGraph::transferNode(HyperGraphPtr self, NodePtr node, bool moveEdges) {
        if (moveEdges) {
            auto edgesIn = node->edgesIn;
            for (auto& e : edgesIn) {
                auto otherHg = (e->to->hg == node->hg) ? e->from->hg : e->to->hg;
                auto maxLvlHg = (lvl > otherHg->lvl) ? self : otherHg;
                if (e->via->hg != maxLvlHg)
                    maxLvlHg->transferEdge(maxLvlHg, e);
            }        
            auto edgesOut = node->edgesOut;
            for (auto& e : edgesOut) {
                auto otherHg = (e->to->hg == node->hg) ? e->from->hg : e->to->hg;
                auto maxLvlHg = (lvl > otherHg->lvl) ? self : otherHg;
                if (e->via->hg != maxLvlHg)
                    maxLvlHg->transferEdge(maxLvlHg, e);
            }
        }
        node->hg->removeNode(node, false);        
        node->hg = self;
        if (!node->via && !node->hyper)
            updateScale(1);
        size_t idx = _nodes.size() ? (_nodes.rbegin()->first + 1) : 0;
        node->idx = idx;
        _nodes[idx] = node;
        if (node->content)
            node->content->lvl = lvl + 1;
    }

    void HyperGraph::removeNode(NodePtr node, bool clear) {
        if (clear) {
            if (node->content)
                node->content->clear();
            auto edgesIn = node->edgesIn;
            for (auto& e : edgesIn) {
                e->via->hg->removeEdge(e);
            }
            auto edgesOut = node->edgesOut;
            for (auto& e : edgesOut) {
                e->via->hg->removeEdge(e);
            }
        }
        _nodes.erase(node->idx);
        if (!node->via && !node->hyper)
            updateScale(-1);
    }

    void HyperGraph::updateScale(int off) {
        float preCoeff = scale();
        nDrawableNodes += off;
        float aftCoeff = scale();
        for (auto& n : _nodes)
            n.second->pos = n.second->pos * preCoeff / aftCoeff;
    }

    EdgePtr HyperGraph::addEdge(HyperGraphPtr self, EdgeLinkStyle style, NodePtr from, NodePtr to) {
        auto sim = from->edgeTo(to);
        if (sim) {
            auto edge = std::make_shared<Edge>(0, style, from, nullptr, to);
            sim->fuse(edge);
            return sim;
        }
        auto via = addNode(self, "", BLANK, true);
        size_t idx = _edges.size() ? (_edges.rbegin()->first + 1) : 0;
        auto edge = std::make_shared<Edge>(idx, style, from, via, to);
        _edges[idx] = edge;
        from->edgesOut.insert(edge);
        to->edgesIn.insert(edge);
        return edge;
    }

    void HyperGraph::removeEdge(EdgePtr edge, bool clear) {
        if (clear) {
            edge->to->edgesIn.erase(edge);
            edge->from->edgesOut.erase(edge);
        }
        _edges.erase(edge->idx);
    }

    void HyperGraph::reduceEdge(EdgePtr edge, bool clear) {
        auto e = _edges[edge->idx];
        e->reduce(edge);
        if (!e->links.size())
            removeEdge(edge, clear);
    }
            
    void HyperGraph::transferEdge(HyperGraphPtr self, EdgePtr edge) {
        edge->via->hg->removeEdge(edge, false);
        size_t idx = _edges.size() ? (_edges.rbegin()->first + 1) : 0;
        edge->idx = idx;
        _edges[idx] = edge;
        self->transferNode(self, edge->via, false);
    }

    NodePtr HyperGraph::addHyperEdge(HyperGraphPtr self, const EdgeLinksBundle& froms, const EdgeLinksBundle& tos) {
        auto hyperVia = addNode(self, "", BLANK, false, true);
        for (auto& from : froms)
            addEdge(self, from.first, from.second, hyperVia);
        for (auto& to : tos)
            addEdge(self, to.first, hyperVia, to.second);
        return hyperVia;
    }

    NodePtr HyperGraph::makeEdgeHyper(HyperGraphPtr self, EdgePtr edge) {
        removeEdge(edge);
        EdgeLinksBundle froms;
        for (auto l : edge->links)
            froms.push_back({l, edge->from});
        EdgeLinksBundle tos;
        for (auto l : edge->links)
            tos.push_back({l, edge->to});
        auto node = addHyperEdge(self, froms, tos);
        node->pos = (edge->from->pos + edge->to->pos) * 0.5f;
        (*node->edgesIn.begin())->reposition();
        (*node->edgesOut.begin())->reposition();
        return node;
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

    void HyperGraph::draw(Vector2 origin, Vector2 offset, float s, const Font& font, bool physics, NodePtr grabbedNode, NodePtr& hoverNode, EdgeLinkHoverPtr& hoverEdgeLink) {
        origin += (parent ? (parent->hg->scale() * parent->pos) : Vector2Zero());
        Vector2 scaledOrigin = origin * s;
        for (auto& n : _nodes)
            if (!n.second->via)
                n.second->predraw(scaledOrigin, offset, s, font);
        for (auto& e : _edges) {
            auto hoverLink = e.second->draw(scaledOrigin, offset, s, font, physics);
            if (hoverLink)
                hoverEdgeLink = std::make_shared<EdgeLinkHover>(e.second, *hoverLink);
        }
        for (auto& n : _nodes) {
            if (!n.second->via) {
                if (n.second->draw(scaledOrigin, offset, s, font))
                    hoverNode = n.second;
            }
        }
        for (auto& n : _nodes)
            if (n.second->content && s * scale() > HIDE_CONTENT_SCALE)
                n.second->content->draw(origin, offset, s, font, physics, grabbedNode, hoverNode, hoverEdgeLink);

        if (grabbedNode && grabbedNode->hg.get() == this) {
            grabbedNode->highlight = HIGHLIGHT_INTENSITY;
            grabbedNode->predraw(scaledOrigin, offset, s, font);
            grabbedNode->draw(scaledOrigin, offset, s, font);
            if (grabbedNode->content && s * scale() > HIDE_CONTENT_SCALE)
                grabbedNode->content->draw(origin, offset, s, font, physics, grabbedNode, hoverNode, hoverEdgeLink);
        }
    }

    NodePtr HyperGraph::getNodeAt(Vector2 pos, const std::set<NodePtr>& except) {
        for (auto& n : _nodes) {
            if (n.second->via || n.second->hyper || except.count(n.second))
                continue;
            NodePtr node;
            if (n.second->content)
                node = n.second->content->getNodeAt(pos, except);
            if (node)
                return node;
            bool hover = (n.second->_rCache * n.second->_rCache > Vector2DistanceSqr(pos, n.second->_posCache));
            if (hover)
                return n.second;
        }
        return nullptr;
    }

}