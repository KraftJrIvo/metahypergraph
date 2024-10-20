#include "drawer.h"
#include "raylib.h"
#include "raymath.h"
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>

#include "types/base.h"
#include "types/edge.h"
#include "types/metahypergraph.h"
#include "types/node.h"
#include "util/utf8.cpp"

#include <codecvt>
#include <locale>

namespace mhg {
    std::vector<Color> Drawer::COLORS = { LIGHTGRAY, GRAY, DARKGRAY, YELLOW, GOLD, ORANGE, PINK, RED, MAROON, GREEN, LIME, DARKGREEN, SKYBLUE, BLUE, DARKBLUE, PURPLE, VIOLET, DARKPURPLE, BEIGE, BROWN, DARKBROWN };

    class DrawerImpl final : public Drawer {
    public:
        DrawerImpl(MetaHyperGraph& mhg, Vector2 winSize, std::string winName);
        ~DrawerImpl() { _redrawer.join(); };

        void recenter() override;

        bool isEditing() override {return _editingNode != nullptr || _editingEdgeLink != nullptr;};
    private:
        std::thread _redrawer;
        Vector2 _offset = Vector2Zero();
        Vector2 _curOffset = Vector2Zero();
        Vector2 _lastMouseGrabPos = Vector2Zero();

        float _scale = 1.0f;
        Font _font;
        const std::string _allChars;

        NodePtr _hoverNode = nullptr;
        EdgeLinkPtr _hoverEdgeLink = nullptr;

        NodePtr _grabbedNode = nullptr;
        Vector2 _grabbedInitPos = Vector2Zero();
        Vector2 _grabOff = Vector2Zero();
        float _grabScale = 1.0f;

        NodePtr _addEdgeFromNode = nullptr;
        NodePtr _addEdgeToNode = nullptr;
        EdgePtr _addEdgeFromEdge = nullptr;
        EdgePtr _addEdgeToEdge = nullptr;
        Vector2 _addEdgeFromMpos = Vector2Zero();

        EdgeLinkStylePtr _lastLinkStyle = nullptr;
        
        NodePtr _editingNode = nullptr;
        EdgeLinkPtr _editingEdgeLink = nullptr;
        uint8_t _editingColorIdx = 0;

        std::map<NodePtr, std::pair<Vector2, Vector2>> _selectedNodes;
        std::map<NodePtr, std::pair<Vector2, Vector2>> _selectedNodesTmp;
        Vector2 _selectionStart = Vector2Zero();
        Rectangle _selectionRect;
        bool _recentlyScaled = false;
        double _lastScaleTs = 0;

        std::string _labelPriorToEdit;
        Color _colorPriorToEdit;


        NodePtr _makeHyperAndMoveToMpos(EdgePtr edge, Vector2 pos);
        void _unmakeHyper(NodePtr node);
        void _grabOrSelectNode(NodePtr node);
        void _dropNode(NodePtr node, Vector2 pos, const std::set<NodePtr>& except = {});
        void _ungrabNode();

        void _selectAll();
        void _deselectAll();
        void _startFrameSelection();
        void _doFrameSelection();
        void _finishFrameSelection();
        void _startMovingSelection();
        void _endMovingSelection();
        void _deleteSelection();
        void _cloneSelection();
        void _scaleSelection();
        void _moveSelection();
        void _checkDoneScaling();
        void _keepSelectedFromOverlapping(std::map<NodePtr, std::pair<Vector2, Vector2>>& selected);
        
        void _addNode();
        void _startAddingEdge();
        void _finishAddingEdge();
        void _addEdge();

        void _startEditingNode(NodePtr node);
        void _startEditingEdgeLink(EdgeLinkPtr el);
        void _edit();
        void _stopEditing();

        void _panAndZoom();

        void _toggleFullscreen();

        void _drawFrameSelection();
        void _drawEdgeAdding();
        void _updateHighlights();
        void _draw();
        void _update();
    };

    DrawerImpl::DrawerImpl(MetaHyperGraph& mhg, Vector2 winSize, std::string winName) :
        Drawer(mhg, winSize, winName),
        _allChars(u8" !\"#$%&\'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~абвгдеёжзийклмнопрстуфхцчшщъыьэюяАБВГДЕЁЖЗИЙКЛМНОПРСТУФХЦЧШЩЪЫЬЭЮЯ")
    {
        setlocale(LC_ALL, "en_US.UTF-8");
        _redrawer = std::thread([&]() {
            SetTraceLogLevel(LOG_ERROR);
            SetConfigFlags(FLAG_MSAA_4X_HINT);
            InitWindow(_winSize.x, _winSize.y, _winName.c_str());
            SetWindowIcon(LoadImage("res/icon.png"));
            int c; auto cdpts = LoadCodepoints(_allChars.c_str(), &c);
            _font = LoadFontEx("res/sofia-sans-extra-condensed.ttf", FONT_SZ, cdpts, c);
            UnloadCodepoints(cdpts);
            SetTargetFPS(60);
            SetExitKey(KEY_F4);
            _offset = _winSize * 0.5f;
            while (!WindowShouldClose() && _drawing) {
                _draw();
                _update();
            }
            CloseWindow();
            _drawing = false;
        });
    }
    
    void DrawerImpl::recenter() {
        Vector2 center = _mhg.getCenter();
        _offset = -center * _scale + _winSize * 0.5f;
        _scale = 1.0f;
    }

    void DrawerImpl::_startEditingNode(NodePtr node) {
        _editingNode = node;
        _editingNode->dp.editing = true;
        _labelPriorToEdit = _editingNode->p.label;
        _colorPriorToEdit = _editingNode->p.color;
        _editingNode->p.label += '_';
        _hoverNode = nullptr;
    }

    void DrawerImpl::_startEditingEdgeLink(EdgeLinkPtr el) {
        _editingEdgeLink = el;
        _editingEdgeLink->editing = true;
        _labelPriorToEdit = _editingEdgeLink->style->label;
        _colorPriorToEdit = _editingEdgeLink->style->color;
        _editingEdgeLink->style->label += '_';
        _hoverEdgeLink = nullptr;
    }

    void DrawerImpl::_edit() {
        auto& label = _editingNode ? _editingNode->p.label : _editingEdgeLink->style->label;
        auto& color = _editingNode ? _editingNode->p.color : _editingEdgeLink->style->color;
        if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_ESCAPE)) {
            if (IsKeyPressed(KEY_ESCAPE)) {
                label = _labelPriorToEdit;
                color = _colorPriorToEdit;
            } else
                label = label.substr(0, label.length() - 1);
            _stopEditing();
        }
        if (IsKeyPressed(KEY_BACKSPACE) && label.length()) {
            label = label.substr(0, label.length() - 1);
            do { label = label.substr(0, label.length() - 1);
            } while (!validate_utf8(label.c_str()));
            label += '_';
        }
        float mwm = GetMouseWheelMove();
        if (mwm < 0)
            _editingColorIdx = (_editingColorIdx == 0) ? (COLORS.size() - 1) : _editingColorIdx - 1;
        if (mwm > 0)
            _editingColorIdx = (_editingColorIdx == COLORS.size() - 1) ? 0 : _editingColorIdx + 1;
        if (mwm != 0)
            color = COLORS[_editingColorIdx];
        wchar_t ch = GetCharPressed();
        if (ch) {
            label = label.substr(0, label.length() - 1);
            std::wstring ws(L"a");
            ws.at(0) = ch;
            using convert_typeX = std::codecvt_utf8<wchar_t>;
            std::wstring_convert<convert_typeX, wchar_t> converterX;
            std::string str = converterX.to_bytes(ws);
            if (ch)
                label += str;
            label += '_';
        }
    }

    void DrawerImpl::_stopEditing() {
        _mhg.noticeAction({.type = _editingNode ? MHGactionType::NODE : MHGactionType::EDGE, .change = true, .n = _editingNode, .els = _editingEdgeLink ? _editingEdgeLink->style : nullptr, 
            .prvLabel = _labelPriorToEdit, .curLabel = _editingNode ? _editingNode->p.label : _editingEdgeLink->style->label,
            .prvColor = _colorPriorToEdit, .curColor = _editingNode ? _editingNode->p.color : _editingEdgeLink->style->color});
        if (_editingNode) {
            _editingNode->dp.editing = false;
            _editingNode = nullptr;
        } else if (_editingEdgeLink) {
            _editingEdgeLink->editing = false;
            _lastLinkStyle = _editingEdgeLink->style;
            _editingEdgeLink = nullptr;
        }
        _editingColorIdx = 0;
        _grabbedNode = nullptr;
    }

    NodePtr DrawerImpl::_makeHyperAndMoveToMpos(EdgePtr edge, Vector2 pos) {
        auto hyperNode = _mhg.makeEdgeHyper(edge);
        if (_mhg._historyRecording) _mhg._histIt-=2;                    
        auto o = hyperNode->hg->parent ? hyperNode->hg->parent->dp.posCache : _offset;
        _mhg.moveNode(hyperNode, Vector2Zero(), (pos - o) / (hyperNode->hg->scale() * _scale));
        _dropNode(hyperNode, pos);
        return hyperNode;
    }

    void DrawerImpl::_unmakeHyper(NodePtr node) {
        node->hyper = !node->hyper;
        _mhg.noticeAction({.type = MHGactionType::HYPER, .inverse = node->hyper, .n = node});
    }

    void DrawerImpl::_dropNode(NodePtr node, Vector2 pos, const std::set<NodePtr>& except) {
        auto dropNode = _mhg.getNodeAt(pos, except);
        auto dropHG = dropNode ? dropNode->content : _mhg._root;
        if (dropHG != node->hg && (!dropHG || !dropHG->parent || !_selectedNodes.count(dropHG->parent))) {
            if (!dropHG) {
                dropHG = dropNode->content = std::make_shared<HyperGraph>(_mhg, dropNode);
                dropHG->self = dropHG;
            }
            _mhg.transferNode(dropHG, node);
            auto o = (dropHG == _mhg._root) ? _offset : dropNode->dp.posCache;
            node->dp.pos = (pos - o) / (dropHG->scale() * _scale);
            auto selectedNodes = _selectedNodes;
            for (auto& sn : selectedNodes)
                if (sn.first->content && _selectedNodes.count(node) && node->hg->isChildOf(sn.first->content))
                    _selectedNodes.erase(node);
        }
    }

    void DrawerImpl::_keepSelectedFromOverlapping(std::map<NodePtr, std::pair<Vector2, Vector2>>& selected) {
        auto selectedNodes = selected;
        for (auto it1 = selectedNodes.begin(); it1  != selectedNodes.end(); ++it1)
            for (auto it2 = std::next(it1); it2 != selectedNodes.end(); ++it2)
                if ((it1->first->content && (it2->first->hg->isChildOf(it1->first->content)))) 
                    selected.erase(it2->first);
                else if (it2->first->content && (it1->first->hg->isChildOf(it2->first->content)))
                    selected.erase(it1->first);
    }

    void DrawerImpl::_startMovingSelection() {
        for (auto& n : _selectedNodes) {
            auto pos = (_scale * n.first->hg->scale() * n.first->dp.pos + _offset);
            n.second.first = n.first->dp.pos;
            n.second.second = pos - GetMousePosition();
        }
    }

    void DrawerImpl::_endMovingSelection() {
        int c = 0;
        for (auto& n : _selectedNodes) {
            _mhg.moveNode(n.first, n.second.first, n.first->dp.pos);
            if (_mhg._historyRecording && c != _selectedNodes.size() - 1) _mhg._histIt-=2;
            c++;
        }
    }

    void DrawerImpl::_grabOrSelectNode(NodePtr node) {
        auto mpos = GetMousePosition();
        bool selecting = (IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_LEFT_SHIFT));
        _grabbedNode = node;
        _grabScale = _scale;
        _grabbedInitPos = node->dp.pos;
        _grabOff = (_scale * node->hg->scale() * node->dp.pos + _offset) - mpos;
        if (!selecting && !_selectedNodes.count(node))
            _selectedNodes.clear();                
        _startMovingSelection();
        if (_selectedNodes.count(_grabbedNode) && selecting) {
            _selectedNodes.erase(_grabbedNode);
        } else {
            auto selectedNodes = _selectedNodes;
            for (auto& sn : selectedNodes)
                if (sn.first->content && _grabbedNode->hg->isChildOf(sn.first->content)) 
                    _selectedNodes.erase(sn.first);
            auto pos = (_scale * _grabbedNode->hg->scale() * _grabbedNode->dp.pos + _offset);
            _selectedNodes[_grabbedNode] = {_grabbedInitPos, pos - mpos};
        }
    }

    void DrawerImpl::_ungrabNode() {
        auto selectedNodes = _selectedNodes;
        for (auto& n : selectedNodes) {
            _mhg.moveNode(n.first, n.second.first, n.first->dp.pos);
            if (_mhg._historyRecording) _mhg._histIt-=2;
        }
        std::set<NodePtr> exceptSelected;
        for (auto& sn : _selectedNodes)
            exceptSelected.insert(sn.first);
        for (auto& n : selectedNodes)
            if (!n.first->via)
                _dropNode(n.first, n.first->dp.posCache, exceptSelected);
        _endMovingSelection();
        _grabbedNode = nullptr;
    }

    void DrawerImpl::_startFrameSelection() {
        _selectionStart = GetMousePosition();
    }

    void DrawerImpl::_doFrameSelection() {
        _selectedNodesTmp.clear();
        std::set<NodePtr> selected;
        _mhg.getNodesIn(_selectionRect, selected);
        for (auto sn : selected)
            _selectedNodesTmp[sn] = {Vector2Zero(), Vector2Zero()};
        _keepSelectedFromOverlapping(_selectedNodesTmp);
    }

    void DrawerImpl::_finishFrameSelection() {
        _selectionStart = Vector2Zero();
        for (auto sn : _selectedNodesTmp)
            _selectedNodes.insert(sn);
        _keepSelectedFromOverlapping(_selectedNodes);
        _selectedNodesTmp.clear();
    }

    void DrawerImpl::_startAddingEdge() {
        auto mpos = GetMousePosition();
        if (_hoverEdgeLink) {
            _addEdgeFromEdge = _hoverEdgeLink->edge;
            _addEdgeFromMpos = mpos;
        } else if (_hoverNode) {
            _addEdgeFromNode = _hoverNode;
        }
    }

    void DrawerImpl::_finishAddingEdge() {
        if (_hoverEdgeLink && _hoverEdgeLink->edge != _addEdgeFromEdge) {
            _addEdgeToEdge = _hoverEdgeLink->edge;
            _addEdgeToNode = nullptr;
        } else if (_hoverNode && _hoverNode != _addEdgeFromNode) {
            _addEdgeToNode = _hoverNode;
            _addEdgeToEdge = nullptr;
        }
        if (!_hoverNode)
            _addEdgeToNode = nullptr;
        if (!_hoverEdgeLink)
            _addEdgeToEdge = nullptr;
    }

    void DrawerImpl::_addNode() {
        auto node = _mhg.addNode("", RED, _hoverNode);
        node->hyper = IsKeyDown(KEY_LEFT_SHIFT);
        auto hg = _hoverNode ? _hoverNode->content : _mhg._root;
        auto o = _hoverNode ? _hoverNode->dp.posCache : _offset;
        _mhg.moveNode(node, Vector2Zero(), (GetMousePosition() - o) / (hg->scale() * _scale));
    }

    void DrawerImpl::_selectAll() {
        for (auto n : _mhg.getAllNodes())
            _selectedNodes[n] = {Vector2Zero(), Vector2Zero()};
    }

    void DrawerImpl::_deselectAll() {
        _selectedNodes.clear();
        _selectedNodesTmp.clear();
    }

    void DrawerImpl::_addEdge() {
        if ((_addEdgeFromNode || _addEdgeFromEdge) && (_addEdgeToNode || _addEdgeToEdge)) {
            auto from = _addEdgeFromNode ? _addEdgeFromNode : _makeHyperAndMoveToMpos(_addEdgeFromEdge, _addEdgeFromMpos);
            if (_mhg._historyRecording && !_addEdgeFromNode) _mhg._histIt-=2;
            auto to = _addEdgeToNode ? _addEdgeToNode : _makeHyperAndMoveToMpos(_addEdgeToEdge, GetMousePosition());
            if (_mhg._historyRecording && !_addEdgeToNode) _mhg._histIt-=2;
            auto style = (!IsKeyDown(KEY_LEFT_SHIFT) && _lastLinkStyle) ? _lastLinkStyle : EdgeLinkStyle::create(COLORS[rand() % COLORS.size()]);
            _mhg.addEdge(style, from, to);
            _lastLinkStyle = style;
        }
        _addEdgeFromNode = nullptr;
        _addEdgeFromEdge = nullptr;
        _addEdgeToNode = nullptr;
        _addEdgeToEdge = nullptr;
    }

    void DrawerImpl::_deleteSelection() {
        if (_selectedNodes.size()) {
            for (auto& sn : _selectedNodes) {                   
                _mhg.removeNode(sn.first);
                if (_mhg._historyRecording) _mhg._histIt-=2;
            }
            _selectedNodes.clear();
        } else {
            if (_hoverEdgeLink) {
                auto& edge = _hoverEdgeLink->edge;
                auto e = Edge::create(edge->hg, edge->idx, edge->from, edge->via, edge->to);
                e->links.insert(EdgeLink::create(_hoverEdgeLink));
                _mhg.reduceEdge(e);
                _hoverEdgeLink = nullptr;
            } else if (_hoverNode) {
                _mhg.removeNode(_hoverNode);
                _hoverNode = nullptr;
            }
        }
    }

    void DrawerImpl::_cloneSelection() {
        std::set<NodePtr> selected;
        for (auto& sn : _selectedNodes)
            selected.insert(sn.first);
        _selectedNodes.clear();
        auto clones = _mhg.cloneNodes(selected);
        for (auto& c : clones) {
            _selectedNodes[c] = {Vector2Zero(), Vector2Zero()};
            _mhg.moveNode(c, Vector2Zero(), c->dp.pos + Vector2{30.0f, 30.0f} / c->hg->scale());
            if (_mhg._historyRecording && _selectedNodes.size() < clones.size()) _mhg._histIt-=2;
        }
    }

    void DrawerImpl::_scaleSelection() {
        auto mpos = GetMousePosition();
        auto mwm = GetMouseWheelMove();
        if (mwm)  {
            float newScale = _scale + mwm * _scale / 10.0f;
            if (!_recentlyScaled)
                _startMovingSelection();
            Vector2 avg = Vector2Zero();
            for (auto& sn : _selectedNodes)
                avg += sn.first->dp.posCache;
            Vector2 center = avg / float(_selectedNodes.size());
            for (auto& sn : _selectedNodes) {
                auto newpospix = center + (sn.first->dp.posCache - center) * (newScale / _scale);
                auto pos = (_scale * sn.first->hg->scale() * sn.first->dp.pos + _offset);
                sn.first->dp.pos = (newpospix - _offset + _curOffset + (pos - sn.first->dp.posCache) * (_scale / _grabScale)) / (sn.first->hg->scale() * _scale);
            }
            _recentlyScaled = true;
            _lastScaleTs = GetTime();
        }
    }

    void DrawerImpl::_moveSelection() {
        std::set<NodePtr> exceptSelected;
        for (auto& sn : _selectedNodes)
            exceptSelected.insert(sn.first);
        for (auto& sn : _selectedNodes) {
            sn.first->dp.overNode = _mhg.getNodeAt(sn.first->dp.posCache, exceptSelected);
            if (sn.first->dp.overNode)
                if (!sn.first->hg->parent || sn.first->dp.overNode != sn.first->hg->parent)
                    sn.first->dp.overNode->dp.tmpDrawableNodes++;
            if (sn.first->hg->parent && sn.first->dp.overNode != sn.first->hg->parent)
                sn.first->hg->parent->dp.tmpDrawableNodes--;
            sn.first->dp.overRoot = !bool(sn.first->dp.overNode);
        }
        for (auto& sn : _selectedNodes)
            sn.first->dp.pos = (GetMousePosition() - _offset + _curOffset + sn.second.second * (_scale / _grabScale)) / (sn.first->hg->scale() * _scale);
    }

    void DrawerImpl::_checkDoneScaling() {
        if (_recentlyScaled && GetTime() - _lastScaleTs > SCALING_EVENT_TIMEOUT) {
            _recentlyScaled = false;
            _lastScaleTs = 0.0;
            _endMovingSelection();
        }
    }

    void DrawerImpl::_panAndZoom() {
        auto mpos = GetMousePosition();
        if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT))
            _lastMouseGrabPos = mpos;
        if (IsMouseButtonReleased(MOUSE_BUTTON_RIGHT))
            _offset = _offset + _curOffset;
        _curOffset = IsMouseButtonDown(MOUSE_BUTTON_RIGHT) ? (mpos - _lastMouseGrabPos) : Vector2Zero();
        float newScale = _scale + GetMouseWheelMove() * _scale / 10.0f;
                        _offset -= ((mpos - _offset) / _scale) * (newScale - _scale);
        _scale = newScale;
    }

    void DrawerImpl::_toggleFullscreen() {
        if (!IsWindowFullscreen()) {
            auto newWinSz = Vector2{(float)GetMonitorWidth(0), (float)GetMonitorHeight(0)};
            _offset += GetWindowPosition();
            _winSize = newWinSz;
            SetWindowSize(int(_winSize.x), int(_winSize.y));
        }
        ToggleFullscreen();
        if (!IsWindowFullscreen()) {
            auto newWinSz = _baseWinSize;
            _offset -= GetWindowPosition();
            _winSize = newWinSz;
            SetWindowSize(int(_winSize.x), int(_winSize.y));
        }
    }

    void DrawerImpl::_update() {
        if (isEditing()) {            
            _edit();        
        } else {
            bool selecting = (IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_LEFT_SHIFT));

            // EDIT
            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && IsGestureDetected(GESTURE_DOUBLETAP)) {
                if (_hoverEdgeLink)
                    _startEditingEdgeLink(_hoverEdgeLink);
                else if (_hoverNode && !_hoverNode->hyper)
                    _startEditingNode(_hoverNode);
            }

            // GRAB / SELECT
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && !IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
                if (_hoverNode) {
                    _grabOrSelectNode(_hoverNode);
                } else {
                    if (selecting)
                        _startFrameSelection();
                    else
                        _deselectAll();
                }
            } else if (_grabbedNode) {
                if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT))
                    _ungrabNode();
            } else if (_selectionStart.x > 0) {
                _doFrameSelection();
                if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON))
                    _finishFrameSelection();
            }     

            // EDIT SELECTON
            if (IsKeyPressed(KEY_D) && selecting && _selectedNodes.size()) {
                _cloneSelection();
            } else if (IsKeyPressed(KEY_A)) {
                if (IsKeyPressed(KEY_LEFT_CONTROL))
                    _selectAll();
                else
                    _addNode();
            } else if (IsKeyPressed(KEY_DELETE)) {
                _deleteSelection();
            }        
            if (_grabbedNode && _selectedNodes.size())
                _moveSelection();

            // ADD EDGE
            if (IsKeyPressed(KEY_E))
                _startAddingEdge();
            if (IsKeyDown(KEY_E) && (_addEdgeFromNode || _addEdgeFromEdge))
                _finishAddingEdge();
            else if (IsKeyReleased(KEY_E))
                _addEdge();

            // TOGGLE HYPER
            if (IsKeyPressed(KEY_H)) {
                if (_hoverEdgeLink)
                    _makeHyperAndMoveToMpos(_hoverEdgeLink->edge, GetMousePosition());
                else if (_hoverNode)
                    _unmakeHyper(_hoverNode);
            }
            
            // HISTORY NAV
            if (IsKeyDown(KEY_LEFT_CONTROL)) {
                if (!IsKeyDown(KEY_LEFT_SHIFT) && IsKeyPressed(KEY_Z))
                    _mhg.undo();
                if (IsKeyPressed(KEY_Y) || (IsKeyDown(KEY_LEFT_SHIFT) && IsKeyPressed(KEY_Z)))
                    _mhg.redo();
            }

            // PAN + ZOOM
            if (selecting)
                _scaleSelection();
            else
                _panAndZoom();
            _checkDoneScaling(); 

            // RECENTER
            if (IsKeyPressed(KEY_C))
                recenter();
            
            // FULLSCREEN
            if (IsKeyPressed(KEY_F) || IsKeyPressed(KEY_F11))
                _toggleFullscreen();
        }
    }

    void DrawerImpl::_drawFrameSelection() {
        auto mpos = GetMousePosition();
        _selectionRect = Rectangle{std::min(_selectionStart.x, mpos.x), std::min(_selectionStart.y, mpos.y), fabs(mpos.x - _selectionStart.x), fabs(mpos.y - _selectionStart.y)};
        DrawRectangleLines(_selectionRect.x, _selectionRect.y, _selectionRect.width, _selectionRect.height, WHITE);
        _hoverNode = nullptr;
        _hoverEdgeLink = nullptr;
    }

    void DrawerImpl::_drawEdgeAdding() {
        auto from = _addEdgeFromNode ? _addEdgeFromNode->dp.posCache : _addEdgeFromMpos;
        auto to = GetMousePosition();
        DrawLineEx(from, to, EDGE_THICK, WHITE);
    }

    void DrawerImpl::_updateHighlights() {
        if (_hoverEdgeLink) _hoverEdgeLink->highlight = HIGHLIGHT_INTENSITY;
        if (_hoverNode) _hoverNode->dp.highlight = HIGHLIGHT_INTENSITY;
        if (_addEdgeFromNode) _addEdgeFromNode->dp.highlight = HIGHLIGHT_INTENSITY_2;
        if (_addEdgeFromEdge) _addEdgeFromEdge->dp.highlight = HIGHLIGHT_INTENSITY_2;
        if (_addEdgeToNode) _addEdgeToNode->dp.highlight = HIGHLIGHT_INTENSITY_2;
        if (_addEdgeToEdge) _addEdgeToEdge->dp.highlight = HIGHLIGHT_INTENSITY_2;
        for (auto& sn : _selectedNodes)
            sn.first->dp.highlight = HIGHLIGHT_INTENSITY_2;
        for (auto& sn : _selectedNodesTmp)
            sn.first->dp.highlight = HIGHLIGHT_INTENSITY_2;
    }

    void DrawerImpl::_draw() {
        BeginDrawing();
        ClearBackground(BLACK);

        _hoverNode = nullptr;
        _hoverEdgeLink = nullptr;

        _mhg.draw(_offset + _curOffset, _scale, _font, _selectedNodes, _hoverNode, _hoverEdgeLink);

        if (_addEdgeFromNode || _addEdgeFromEdge)
            _drawEdgeAdding();

        if (_selectionStart.x > 0)
            _drawFrameSelection();

        _updateHighlights(); 
        
        EndDrawing();
    }

    std::shared_ptr<Drawer> Drawer::create(MetaHyperGraph& w, Vector2 winSize, std::string winName) {
        return std::make_shared<DrawerImpl>(w, winSize, winName);
    }
}