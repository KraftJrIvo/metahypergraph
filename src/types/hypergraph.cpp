#include "hypergraph.h"
#include "base.h"
#include "edge.h"
#include "node.h"
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
            auto eIn = n.second->eIn;
            for (auto& e : eIn)
                if (!e->from->hg->isChildOf(hg) || !e->to->hg->isChildOf(hg)) {
                    e->hg->removeEdge(e);
                    pmhg.noticeAction({.type = MHGactionType::EDGE, .inverse = true, .e = e, .els = (*e->links.begin())->style, .elp = (*e->links.begin())->params}, false);            
                }
            auto eOut = n.second->eOut;
            for (auto& e : eOut) {
                if (!e->from->hg->isChildOf(hg) || !e->to->hg->isChildOf(hg)) {
                    e->hg->removeEdge(e);
                    pmhg.noticeAction({.type = MHGactionType::EDGE, .inverse = true, .e = e, .els = (*e->links.begin())->style, .elp = (*e->links.begin())->params}, false);            
                }
            }
        }
    }

    void HyperGraph::checkForTransferEdges(NodePtr node) {
        auto eIn = node->eIn;
        for (auto& e : eIn) {
            auto otherHg = (e->to->hg == node->hg) ? e->from->hg : e->to->hg;
            auto maxLvlHg = (lvl > otherHg->lvl) ? self : otherHg;
            if (e->hg != maxLvlHg)
                maxLvlHg->transferEdge(e);
        }        
        auto eOut = node->eOut;
        for (auto& e : eOut) {
            auto otherHg = (e->to->hg == node->hg) ? e->from->hg : e->to->hg;
            auto maxLvlHg = (lvl > otherHg->lvl) ? self : otherHg;
            if (e->hg != maxLvlHg)
                maxLvlHg->transferEdge(e);
        }
        if (node->content)
            for (auto& n : node->content->_nodes)
                node->content->checkForTransferEdges(n.second);
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
        _nDrawableNodesCache = nDrawableNodes + (parent ? parent->dp.tmpDrawableNodes : 0);
        bool willDrop = (parent->dp.overNode || parent->dp.overRoot);
        return (willDrop ? ((parent->dp.overNode ? (parent->dp.overNode->scale()) : 1.0f) * parent->coeff()) : (parent->hg->scale() * coeff()));
    }

    NodePtr HyperGraph::addNode(const std::string &label, const Color &color, bool via, bool hyper) {
        auto node = Node::create(self, _nodes.size() ? (_nodes.rbegin()->first + 1) : 0, NodeParams{label, color}, via, hyper);
        addNode(node);
        return node;
    }

    void HyperGraph::addNode(NodePtr node) {
        if (!node->via && !node->hyper)
            updateScale(1);
        _nodes[node->idx] = node;
    }
    
    void HyperGraph::transferNode(NodePtr node, bool moveEdges) {
        node->hg->removeNode(node, false);        
        node->hg = self;
        if (!node->via && !node->hyper)
            updateScale(1);
        size_t idx = _nodes.size() ? (_nodes.rbegin()->first + 1) : 0;
        node->idx = idx;
        _nodes[idx] = node;
        if (node->content)
            node->content->lvl = lvl + 1;
        if (moveEdges)
            checkForTransferEdges(node);
    }

    void HyperGraph::removeNode(NodePtr node, bool rmOuterEdges) {
        if (rmOuterEdges) {
            if (node->content)
                node->content->removeOuterEdges(node->content);            
            auto eIn = node->eIn;
            for (auto& e : eIn) {
                e->hg->removeEdge(e);
            }
            auto eOut = node->eOut;
            for (auto& e : eOut) {
                e->hg->removeEdge(e);
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
            n.second->dp.pos = n.second->dp.pos * preCoeff / aftCoeff;
    }

    EdgePtr HyperGraph::addEdge(EdgeLinkStylePtr style, NodePtr from, NodePtr to, const EdgeLinkParams& params) {
        auto sim = from->getEdgeTo(to);
        if (sim) {
            auto edge = Edge::create(self, sim->idx, from, nullptr, to);
            edge->links.insert(EdgeLink::create(sim, style, params));
            sim->fuse(edge);
            return sim;
        }
        auto via = Node::create(self, _nodes.size() ? (_nodes.rbegin()->first + 1) : 0, {}, true, false);
        auto idx = _edges.size() ? (_edges.rbegin()->first + 1) : 0;
        auto edge = Edge::create(self, idx, from, via, to);
        edge->links.insert(EdgeLink::create(edge, style, params));
        addEdge(edge);
        return edge;
    }

    void HyperGraph::addEdge(EdgePtr edge) {
        addNode(edge->via);
        _edges[edge->idx] = edge;
        edge->from->eOut.insert(edge);
        edge->to->eIn.insert(edge);
    }

    void HyperGraph::removeEdge(EdgePtr edge, bool clear) {
        if (clear) {
            edge->to->eIn.erase(edge);
            edge->from->eOut.erase(edge);
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
        edge->hg->removeEdge(edge, false);
        size_t idx = _edges.size() ? (_edges.rbegin()->first + 1) : 0;
        _edges[idx] = edge;
        edge->reindex(self, idx);
        transferNode(edge->via, false);
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
        node->dp.pos = (edge->from->dp.pos + edge->to->dp.pos) * 0.5f;
        (*node->eIn.begin())->reposition();
        (*node->eOut.begin())->reposition();
        return node;
    }

    void HyperGraph::reposition(unsigned int seed) {
        if (seed) 
            srand(seed);
        float angle;
        for (auto& n : _nodes) {
            angle = 2.0f * 3.14159f * RAND_FLOAT;
            n.second->dp.pos = NODE_SZ * Vector2{ cos(angle), sin(angle) };
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
            acc += n.second->dp.pos;
        return acc / _nodes.size();
    }            
    
    void HyperGraph::recenter() {
        move(-getCenter());
    }
    
    void HyperGraph::move(const Vector2 delta) {
        for (auto& n : _nodes)
            n.second->dp.pos += delta;
    }

    void HyperGraph::draw(Vector2 origin, Vector2 offset, float s, const Font& font, bool physics, const std::map<NodePtr, std::pair<Vector2, Vector2>>& selectedNodes, 
        NodePtr& hoverNode, EdgeLinkPtr& hoverEdgeLink) 
    {
        origin += (parent ? (parent->hg->scale() * parent->dp.pos) : Vector2Zero());
        Vector2 scaledOrigin = origin * s;
        _scaledOcache = scaledOrigin;
        for (auto& n : _nodes)
            if (!n.second->via)
                n.second->predraw(scaledOrigin, offset, s, font);
        for (auto& e : _edges) {
            auto hoverLink = e.second->draw(scaledOrigin, offset, s, font, physics, selectedNodes);
            if (hoverLink)
                hoverEdgeLink = hoverLink;
        }
        for (auto& n : _nodes) {
            if (!n.second->via) {
                if (n.second->draw(scaledOrigin, offset, s, font))
                    hoverNode = n.second;
            }
        }
        for (auto& n : _nodes) {
            bool childOrSelected = false;
            for (auto& sn : selectedNodes) {
                if ((sn.first == n.second) || sn.first->content && n.second->hg->isChildOf(sn.first->content)) {
                    childOrSelected = true;
                    break;
                }
            }
            if (childOrSelected) {
                for (auto& e : n.second->eIn)
                    if (e->hg->parent && s * e->hg->parent->hg->scale() > HIDE_CONTENT_SCALE)
                        e->draw(e->hg->_scaledOcache, offset, s, font, physics, selectedNodes);
                for (auto& e : n.second->eOut)
                    if (e->hg->parent && s * e->hg->parent->hg->scale() > HIDE_CONTENT_SCALE)
                        e->draw(e->hg->_scaledOcache, offset, s, font, physics, selectedNodes);
            }
            if (n.second->content) {
                bool parentOfSelected = false;
                for (auto& sn : selectedNodes) {
                    if (n.second->content && sn.first->hg->isChildOf(n.second->content)) {
                        parentOfSelected = true;
                        break;
                    }
                }
                if ((s * scale() > HIDE_CONTENT_SCALE) || parentOfSelected)
                    n.second->content->draw(origin, offset, s, font, physics, selectedNodes, hoverNode, hoverEdgeLink);
            }
        }
    }

    void HyperGraph::redrawSelected(Vector2 origin, Vector2 offset, float s, const Font& font, bool physics, const std::map<NodePtr, std::pair<Vector2, Vector2>>& selectedNodes, 
        NodePtr& hoverNode, EdgeLinkPtr& hoverEdgeLink) 
    {
        origin += (parent ? (parent->hg->scale() * parent->dp.pos) : Vector2Zero());
        Vector2 scaledOrigin = origin * s;
        _scaledOcache = scaledOrigin;
        bool big = s * scale() > HIDE_CONTENT_SCALE;
        for (auto& n : _nodes) {
            if (selectedNodes.count(n.second)) {
                n.second->predraw(scaledOrigin, offset, s, font);
                n.second->draw(scaledOrigin, offset, s, font);
                if (n.second->content && big)
                    n.second->content->draw(origin, offset, s, font, physics, selectedNodes, hoverNode, hoverEdgeLink);
                for (auto& e : n.second->eIn)
                    if (e->hg->parent && s * e->hg->parent->hg->scale() > HIDE_CONTENT_SCALE)
                        e->draw(e->hg->_scaledOcache, offset, s, font, physics, selectedNodes);
                for (auto& e : n.second->eOut)
                    if (e->hg->parent && s * e->hg->parent->hg->scale() > HIDE_CONTENT_SCALE)
                        e->draw(e->hg->_scaledOcache, offset, s, font, physics, selectedNodes);
            } else if (n.second->content && big) {
                n.second->content->redrawSelected(origin, offset, s, font, physics, selectedNodes, hoverNode, hoverEdgeLink);
            }
        }
    }

    void HyperGraph::resetDraw() {
        for (auto& n : _nodes) {
            n.second->resetDraw();
            if (n.second->content)
                n.second->content->resetDraw();
        }
    }

    NodePtr HyperGraph::getNodeAt(Vector2 pos, const std::set<NodePtr>& except) {
        for (auto& n : _nodes) {
            if (n.second->via || n.second->hyper || except.count(n.second))
                continue;
            bool hover = (n.second->dp.rCacheStable * n.second->dp.rCacheStable > Vector2DistanceSqr(pos, n.second->dp.posCache));
            if (hover) {
                NodePtr node;
                if (n.second->content)
                    node = n.second->content->getNodeAt(pos, except);
                if (node)
                    return node;
                return n.second;
            }
        }
        return nullptr;
    }
    
    void HyperGraph::getNodesIn(Rectangle rect, std::set<NodePtr>& result, const std::set<NodePtr>& except) {
        for (auto& n : _nodes) {
            auto r = n.second->dp.rCache;
            Rectangle radiusRect = {rect.x + r, rect.y + r, rect.width - r * 2, rect.height - r * 2};
            if (!except.count(n.second)) {                
                if (CheckCollisionPointRec(n.second->dp.posCache, radiusRect))
                    result.insert(n.second);
                else if (n.second->content)
                    n.second->content->getNodesIn(rect, result, except);
            }
        }
    }
}