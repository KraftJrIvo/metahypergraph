#include "hypergraph.h"
#include "base.h"
#include "raylib.h"
#include <cstddef>
#include <iterator>
#include <memory>

namespace mhg {

    bool HyperGraph::isChildOf(HyperGraphPtr hg) {
        if (hg.get() == this)
            return true;
        if (!parent)
            return false;
        return parent->hg->isChildOf(hg);
    }

    void HyperGraph::clear() {
        auto nodes = _nodes;
        for (auto& n : nodes)
            removeNode(n.second);
        _nodes.clear();
        _edges.clear();
    }

    void HyperGraph::removeOuterEdges(HyperGraphPtr hg) {
        for (auto& n : _nodes) {
            if (n.second->content)
                n.second->content->removeOuterEdges(hg);
            auto edgesIn = n.second->edgesIn;
            for (auto& e : edgesIn)
                if (!e->from->hg->isChildOf(hg) || !e->to->hg->isChildOf(hg)) {
                    e->via->hg->removeEdge(e);
                    pmhg.noticeAction(MHGaction{.type = MHGactionType::EDGE, .inverse = true, .e = e, .els = (*e->links.begin())->style, .elp = (*e->links.begin())->params}, false);            
                }
            auto edgesOut = n.second->edgesOut;
            for (auto& e : edgesOut) {
                if (!e->from->hg->isChildOf(hg) || !e->to->hg->isChildOf(hg)) {
                    e->via->hg->removeEdge(e);
                    pmhg.noticeAction(MHGaction{.type = MHGactionType::EDGE, .inverse = true, .e = e, .els = (*e->links.begin())->style, .elp = (*e->links.begin())->params}, false);            
                }
            }
        }
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

    NodePtr HyperGraph::addNode(const std::string &label, const Color &color, bool via, bool hyper) {
        auto node = std::make_shared<Node>(self, _nodes.size() ? (_nodes.rbegin()->first + 1) : 0, label, color, via, hyper);
        addNode(node);
        return node;
    }

    void HyperGraph::addNode(NodePtr node) {
        if (!node->via && !node->hyper)
            updateScale(1);
        _nodes[node->idx] = node;
    }
    
    void HyperGraph::transferNode(NodePtr node, bool moveEdges) {
        if (moveEdges) {
            auto edgesIn = node->edgesIn;
            for (auto& e : edgesIn) {
                auto otherHg = (e->to->hg == node->hg) ? e->from->hg : e->to->hg;
                auto maxLvlHg = (lvl > otherHg->lvl) ? self : otherHg;
                if (e->via->hg != maxLvlHg)
                    maxLvlHg->transferEdge(e);
            }        
            auto edgesOut = node->edgesOut;
            for (auto& e : edgesOut) {
                auto otherHg = (e->to->hg == node->hg) ? e->from->hg : e->to->hg;
                auto maxLvlHg = (lvl > otherHg->lvl) ? self : otherHg;
                if (e->via->hg != maxLvlHg)
                    maxLvlHg->transferEdge(e);
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

    void HyperGraph::removeNode(NodePtr node, bool rmOuterEdges) {
        if (rmOuterEdges) {
            if (node->content)
                node->content->removeOuterEdges(node->content);            
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

    EdgePtr HyperGraph::addEdge(EdgeLinkStylePtr style, NodePtr from, NodePtr to) {
        auto sim = from->edgeTo(to);
        if (sim) {
            auto edge = std::make_shared<Edge>(self, sim->idx, style, from, nullptr, to);
            sim->fuse(edge);
            return sim;
        }
        auto via = std::make_shared<Node>(self, _nodes.size() ? (_nodes.rbegin()->first + 1) : 0, "", BLANK, true, false);
        auto edge = std::make_shared<Edge>(self, _edges.size() ? (_edges.rbegin()->first + 1) : 0, style, from, via, to);
        addEdge(edge);
        return edge;
    }

    void HyperGraph::addEdge(EdgePtr edge) {
        addNode(edge->via);
        _edges[edge->idx] = edge;
        edge->from->edgesOut.insert(edge);
        edge->to->edgesIn.insert(edge);
    }

    void HyperGraph::removeEdge(EdgePtr edge, bool clear) {
        if (clear) {
            edge->to->edgesIn.erase(edge);
            edge->from->edgesOut.erase(edge);
        }
        removeNode(edge->via);
        _edges.erase(edge->idx);
    }

    void HyperGraph::reduceEdge(EdgePtr edge, bool clear) {
        auto e = _edges[edge->idx];
        e->reduce(edge);
        if (!e->links.size())
            removeEdge(e, clear);
    }
            
    void HyperGraph::transferEdge(EdgePtr edge) {
        edge->via->hg->removeEdge(edge, false);
        size_t idx = _edges.size() ? (_edges.rbegin()->first + 1) : 0;
        edge->idx = idx;
        _edges[idx] = edge;
        self->transferNode(edge->via, false);
    }

    NodePtr HyperGraph::addHyperEdge(const EdgeLinksBundle& froms, const EdgeLinksBundle& tos) {
        auto hyperVia = addNode("", BLANK, false, true);
        for (auto& from : froms)
            addEdge(from.first, from.second, hyperVia);
        for (auto& to : tos)
            addEdge(to.first, hyperVia, to.second);
        return hyperVia;
    }

    NodePtr HyperGraph::makeEdgeHyper(EdgePtr edge) {
        removeEdge(edge);
        EdgeLinksBundle froms;
        for (auto l : edge->links)
            froms.push_back({l->style, edge->from});
        EdgeLinksBundle tos;
        for (auto l : edge->links)
            tos.push_back({l->style, edge->to});
        auto node = addHyperEdge(froms, tos);
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

    void HyperGraph::draw(Vector2 origin, Vector2 offset, float s, const Font& font, bool physics, NodePtr grabbedNode, NodePtr& hoverNode, EdgeLinkPtr& hoverEdgeLink) {
        origin += (parent ? (parent->hg->scale() * parent->pos) : Vector2Zero());
        Vector2 scaledOrigin = origin * s;
        for (auto& n : _nodes)
            if (!n.second->via)
                n.second->predraw(scaledOrigin, offset, s, font);
        for (auto& e : _edges) {
            auto hoverLink = e.second->draw(scaledOrigin, offset, s, font, physics);
            if (hoverLink)
                hoverEdgeLink = hoverLink;
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