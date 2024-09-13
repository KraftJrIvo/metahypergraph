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

        EdgeLinkStylePtr stl =  std::make_shared<EdgeLinkStyle>(EdgeLinkStyle{.label = "test"});
        EdgeLinkStylePtr stl2 = std::make_shared<EdgeLinkStyle>(EdgeLinkStyle{.color = YELLOW, .label = "test1"});
        EdgeLinkStylePtr stl3 = std::make_shared<EdgeLinkStyle>(EdgeLinkStyle{.color = GREEN, .label = "test2"});

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
        noticeAction(MHGaction{.type = MHGactionType::NODE, .inverse = false, .n = node}, false);
        return node;
    }

    void MetaHyperGraph::_addNode(NodePtr node) {
        node->hg->addNode(node);
    }

    void MetaHyperGraph::removeNode(NodePtr node) {
        auto edgesIn = node->edgesIn;
        for (auto& e : edgesIn)
            noticeAction(MHGaction{.type = MHGactionType::EDGE, .inverse = true, .e = e, .els = (*e->links.begin())->style, .elp = (*e->links.begin())->params}, false);
        auto edgesOut = node->edgesOut;
        for (auto& e : edgesOut)
            noticeAction(MHGaction{.type = MHGactionType::EDGE, .inverse = true, .e = e, .els = (*e->links.begin())->style, .elp = (*e->links.begin())->params}, false);
        node->hg->removeNode(node);
        noticeAction(MHGaction{.type = MHGactionType::NODE, .inverse = true, .n = node, .cur = node->pos});
    }

    void MetaHyperGraph::moveNode(NodePtr node, Vector2 prvPos, Vector2 newPos) {
        if (Vector2Length(prvPos - newPos) > 10) 
            noticeAction(MHGaction{.type = MHGactionType::MOVE, .inverse = false, .n = node, .prv = prvPos, .cur = newPos});
        node->pos = newPos;
    }

    void MetaHyperGraph::transferNode(HyperGraphPtr to, NodePtr node) {
        noticeAction(MHGaction{.type = MHGactionType::TRANSFER, .inverse = false, .hg = to, .from = node->hg, .n = node}, false);
        to->transferNode(node);
    }

    EdgePtr MetaHyperGraph::addEdge(EdgeLinkStylePtr style, NodePtr from, NodePtr to) {
        auto hg = (from->hg->lvl > to->hg->lvl) ? from->hg : to->hg;
        auto edge = hg->addEdge(style, from, to);
        noticeAction(MHGaction{.type = MHGactionType::EDGE, .inverse = false, .e = (from->edgeTo(to)) ? std::make_shared<Edge>(hg, edge->idx, style, from, edge->via, to) : edge, .els = style});
        return edge;
    }

    void MetaHyperGraph::_addEdge(EdgePtr edge) {
        edge->via->hg->addEdge(edge);
    }

    void MetaHyperGraph::removeEdge(EdgePtr edge) {
        noticeAction(MHGaction{.type = MHGactionType::EDGE, .inverse = true, .e = edge, .els = (*edge->links.begin())->style, .elp = (*edge->links.begin())->params});
        edge->via->hg->removeEdge(edge);
    }

    void MetaHyperGraph::reduceEdge(EdgePtr edge) {
        noticeAction(MHGaction{.type = MHGactionType::EDGE, .inverse = true, .e = edge, .els = (*edge->links.begin())->style, .elp = (*edge->links.begin())->params});
        edge->via->hg->reduceEdge(edge);
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
        noticeAction(MHGaction{.type = MHGactionType::EDGE, .inverse = true, .e = edge, .els = (*edge->links.begin())->style, .elp = (*edge->links.begin())->params}, false);
        auto hyperNode = edge->via->hg->makeEdgeHyper(edge);
        if (_historyRecording) {
            noticeAction(MHGaction{.type = MHGactionType::NODE, .inverse = false, .n = hyperNode, .cur = hyperNode->pos}, false);
            for (auto& e : hyperNode->edgesIn)
                noticeAction(MHGaction{.type = MHGactionType::EDGE, .inverse = false, .e = e, .els = (*e->links.begin())->style, .elp = (*e->links.begin())->params}, false);
            for (auto& e : hyperNode->edgesOut)
                noticeAction(MHGaction{.type = MHGactionType::EDGE, .inverse = false, .e = e, .els = (*e->links.begin())->style, .elp = (*e->links.begin())->params}, false);
            noticeAction(MHGaction{.type = MHGactionType::SEP}, false);
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
                action.n->label = inv ? action.prvLabel : action.curLabel;
                action.n->color = inv ? action.prvColor : action.curColor;
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
                    auto e = std::make_shared<Edge>(action.e->hg, action.e->idx, action.els, action.e->from, action.e->via, action.e->to);
                    (*e->links.begin())->params = action.elp;
                    reduceEdge(e); 
                } else {
                    if (!action.e->via->hg->getEdge(action.e->idx))
                        _addEdge(action.e);
                    auto e = addEdge(action.els, action.e->from, action.e->to);
                    (*e->links.begin())->params = action.elp;
                }
            }
            break;
        case MHGactionType::TRANSFER:
            transferNode(inv ? action.from : action.hg, action.n);
            break;
        case MHGactionType::MOVE:
            action.n->pos = inv ? action.prv : action.cur;
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


    void MetaHyperGraph::draw(Vector2 offset, float scale, const Font& font, NodePtr grabbedNode, NodePtr& hoverNode, EdgeLinkPtr& hoverEdgeLink) {
        _lock.lock();
        _root->draw(Vector2Zero(), offset, scale, font, _physicsEnabled, grabbedNode, hoverNode, hoverEdgeLink);
        _lock.unlock();
    }

    NodePtr MetaHyperGraph::getNodeAt(Vector2 pos, const std::set<NodePtr>& except) {
        return _root->getNodeAt(pos, except);
    }
}