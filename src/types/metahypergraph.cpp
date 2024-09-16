#include "metahypergraph.h"
#include "edge.h"
#include "raylib.h"
#include "raymath.h"

#include <memory>
#include <string>
#include <utility>

namespace mhg {
    void MetaHyperGraph::clear() {
        _root->clear();
    }

    void MetaHyperGraph::init() {
        _lock.lock();
        _historyRecording = false;
        _history = { MHGaction{.type = MHGactionType::SEP} };
        _histIt = _history.begin();
        if (!_root) {
            _root = std::make_shared<HyperGraph>(*this);
            _root->self = _root;
        }
        clear();

        EdgeLinkStylePtr stl =  EdgeLinkStyle::create(RED, "test");
        EdgeLinkStylePtr stl2 = EdgeLinkStyle::create(YELLOW, "test1");
        EdgeLinkStylePtr stl3 = EdgeLinkStyle::create(GREEN, "test2");

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
        _historyRecording = true;
        _lock.unlock();
    }

    NodePtr MetaHyperGraph::addNode(const std::string &label, const Color &color, NodePtr parent) {
        auto hg = _root;
        if (parent) {
            if (!parent->content) {
                parent->content = std::make_shared<HyperGraph>(*this, parent);
                parent->content->self = parent->content;
            }
            hg = parent->content;
        }
        auto node = hg->addNode(label, color);
        noticeAction({.type = MHGactionType::NODE, .inverse = false, .n = node}, false);
        return node;
    }

    void MetaHyperGraph::_addNode(NodePtr node) {
        node->hg->addNode(node);
    }

    void MetaHyperGraph::removeNode(NodePtr node) {
        auto eIn = node->eIn;
        for (auto& e : eIn)
            noticeAction({.type = MHGactionType::EDGE, .inverse = true, .e = e, .els = (*e->links.begin())->style, .elp = (*e->links.begin())->params}, false);
        auto eOut = node->eOut;
        for (auto& e : eOut)
            noticeAction({.type = MHGactionType::EDGE, .inverse = true, .e = e, .els = (*e->links.begin())->style, .elp = (*e->links.begin())->params}, false);
        node->hg->removeNode(node);
        noticeAction({.type = MHGactionType::NODE, .inverse = true, .n = node, .cur = node->dp.pos});
    }

    void MetaHyperGraph::moveNode(NodePtr node, Vector2 prvPos, Vector2 newPos) {
        noticeAction({.type = MHGactionType::MOVE, .inverse = false, .n = node, .prv = prvPos, .cur = newPos}, false);
        if (Vector2Length(prvPos - newPos) > 10) 
            noticeAction({.type = MHGactionType::SEP}, false);
        node->dp.pos = newPos;
    }

    void MetaHyperGraph::transferNode(HyperGraphPtr to, NodePtr node) {
        noticeAction({.type = MHGactionType::TRANSFER, .inverse = false, .hg = to, .from = node->hg, .n = node}, false);
        to->transferNode(node);
    }

    EdgePtr MetaHyperGraph::addEdge(EdgeLinkStylePtr style, NodePtr from, NodePtr to, const EdgeLinkParams& params) {
        auto hg = (from->hg->lvl > to->hg->lvl) ? from->hg : to->hg;
        auto edge = hg->addEdge(style, from, to, params);
        auto e = Edge::create(hg, edge->idx, from, edge->via, to);
        e->links.insert(EdgeLink::create(e, style, params));
        noticeAction({.type = MHGactionType::EDGE, .inverse = false, .e = (from->getEdgeTo(to)) ? e : edge, .els = style});
        return edge;
    }

    void MetaHyperGraph::_addEdge(EdgePtr edge) {
        edge->hg->addEdge(edge);
    }

    void MetaHyperGraph::removeEdge(EdgePtr edge) {
        noticeAction({.type = MHGactionType::EDGE, .inverse = true, .e = edge, .els = (*edge->links.begin())->style, .elp = (*edge->links.begin())->params});
        edge->hg->removeEdge(edge);
    }

    void MetaHyperGraph::reduceEdge(EdgePtr edge) {
        noticeAction({.type = MHGactionType::EDGE, .inverse = true, .e = edge, .els = (*edge->links.begin())->style, .elp = (*edge->links.begin())->params});
        edge->hg->reduceEdge(edge);
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
        auto hyperNode = hg->addHyperEdge(froms, tos);
        return hyperNode;
    }

    NodePtr MetaHyperGraph::makeEdgeHyper(EdgePtr edge) {
        for (auto& l : edge->links)
            noticeAction({.type = MHGactionType::EDGE, .inverse = true, .e = edge, .els = l->style, .elp = l->params}, false);
        auto hyperNode = edge->hg->makeEdgeHyper(edge);
        if (_historyRecording) {
            noticeAction({.type = MHGactionType::NODE, .inverse = false, .n = hyperNode, .cur = hyperNode->dp.pos}, false);
            for (auto& e : hyperNode->eIn)
                noticeAction({.type = MHGactionType::EDGE, .inverse = false, .e = e, .els = (*e->links.begin())->style, .elp = (*e->links.begin())->params}, false);
            for (auto& e : hyperNode->eOut)
                noticeAction({.type = MHGactionType::EDGE, .inverse = false, .e = e, .els = (*e->links.begin())->style, .elp = (*e->links.begin())->params}, false);
            noticeAction({.type = MHGactionType::SEP}, false);
        }
        return hyperNode;
    }

    void MetaHyperGraph::reposition(unsigned int seed) {
        _root->reposition(seed);
    }

    Vector2 MetaHyperGraph::getCenter() {
        return _root->getCenter();
    }

    void MetaHyperGraph::noticeAction(const MHGaction& action, bool sep) {
        if (!_historyRecording)
            return;
        int n = (_history.end() - _histIt - 1);
        for (int i = 0; i < n; ++i)
            _history.pop_back();
        _history.push_back(action);
        if (sep)
            _history.push_back(MHGaction{.type = MHGactionType::SEP});
        _histIt = _history.end();
    }

    void MetaHyperGraph::_doAction(const MHGaction& action, bool inverse) {
        bool inv = (action.inverse ^ inverse);
        switch (action.type) {
        case MHGactionType::NODE:
            if (action.change) {
                action.n->p.label = inv ? action.prvLabel : action.curLabel;
                action.n->p.color = inv ? action.prvColor : action.curColor;
            } else {
                if (inv) removeNode(action.n); 
                else _addNode(action.n);
            }
            break;
        case MHGactionType::EDGE:
            if (action.change) {
                action.els->label = inv ? action.prvLabel : action.curLabel;
                action.els->color = inv ? action.prvColor : action.curColor;
            } else {
                if (inv) {
                    auto e = Edge::create(action.e->hg, action.e->idx, action.e->from, action.e->via, action.e->to);
                    e->links.insert(EdgeLink::create(e, action.els, action.elp));
                    reduceEdge(e); 
                } else {
                    if (!action.e->hg->getEdge(action.e->idx))
                        _addEdge(action.e);
                    addEdge(action.els, action.e->from, action.e->to, action.elp);
                }
            }
            break;
        case MHGactionType::TRANSFER:
            transferNode(inv ? action.from : action.hg, action.n);
            break;
        case MHGactionType::MOVE:
            action.n->dp.pos = inv ? action.prv : action.cur;
            break;
        default:
            break;
        }
    }

    void MetaHyperGraph::undo() {
        if (_histIt != _history.begin()) {
            _historyRecording = false;
            if (_histIt == _history.end())
                _histIt--;
            if (_histIt != _history.begin() && _histIt->type == MHGactionType::SEP)
                _histIt--;
            while (_histIt->type != MHGactionType::SEP) {
                _doAction(*_histIt, true);
                _histIt--;
            }
            _historyRecording = true;
        }
    }

    void MetaHyperGraph::redo() {
        if (_histIt != _history.end()) {
            _historyRecording = false;
            if (_histIt->type == MHGactionType::SEP)
                _histIt++;
            while (_histIt != _history.end() && _histIt->type != MHGactionType::SEP) {
                _doAction(*_histIt, false);
                _histIt++;
            }
            _historyRecording = true;
        }
    }


    void MetaHyperGraph::draw(Vector2 offset, float scale, const Font& font, const std::map<NodePtr, std::pair<Vector2, Vector2>>& selectedNodes, NodePtr& hoverNode, EdgeLinkPtr& hoverEdgeLink) {
        _lock.lock();
        _root->draw(Vector2Zero(), offset, scale, font, _physicsEnabled, selectedNodes, hoverNode, hoverEdgeLink);
        _lock.unlock();
    }

    NodePtr MetaHyperGraph::getNodeAt(Vector2 pos, const std::set<NodePtr>& except) {
        return _root->getNodeAt(pos, except);
    }
}