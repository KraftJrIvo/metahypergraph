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
#include "util/utf8.cpp"

#include <codecvt>
#include <locale>

#define INPUT_LINE_H 100

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
        Vector2 _selectionStart = Vector2Zero();
        Rectangle _selectionRect;

        std::string _labelPriorToEdit;
        Color _colorPriorToEdit;

        NodePtr _makeHyperAndMoveToMpos(EdgePtr edge, Vector2 pos);
        void _dropNode(NodePtr node, Vector2 pos);

        void _startEditingNode(NodePtr node);
        void _startEditingEdgeLink(EdgeLinkPtr el);
        void _edit();
        void _stopEditing();

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
            InitWindow(_winSize.x, _winSize.y + INPUT_LINE_H, _winName.c_str());
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
    }

    void DrawerImpl::_startEditingEdgeLink(EdgeLinkPtr el) {
        _editingEdgeLink = el;
        _editingEdgeLink->editing = true;
        _labelPriorToEdit = _editingEdgeLink->style->label;
        _colorPriorToEdit = _editingEdgeLink->style->color;
        _editingEdgeLink->style->label += '_';
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

    void DrawerImpl::_dropNode(NodePtr node, Vector2 pos) {
        auto dropNode = _mhg.getNodeAt(pos, {node});
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

    void DrawerImpl::_update() {
        if (isEditing()) {
            _edit();
        } else {
            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && IsGestureDetected(GESTURE_DOUBLETAP)) {
                if (_hoverEdgeLink) {
                    _startEditingEdgeLink(_hoverEdgeLink);
                    _hoverEdgeLink = nullptr;
                } else if (_hoverNode && !_hoverNode->hyper) {
                    _startEditingNode(_hoverNode);
                    _hoverNode = nullptr;
                }
            }

            auto mpos = GetMousePosition();
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && !IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
                if (_hoverNode) {
                    _grabbedNode = _hoverNode;
                    _grabbedInitPos = _hoverNode->dp.pos;
                    _grabOff = (_scale * _hoverNode->hg->scale() * _hoverNode->dp.pos + _offset) - mpos;
                    if (!IsKeyDown(KEY_LEFT_CONTROL) && !_selectedNodes.count(_hoverNode))
                        _selectedNodes.clear();                
                    for (auto& n : _selectedNodes) {
                        auto pos = (_scale * n.first->hg->scale() * n.first->dp.pos + _offset);
                        n.second = {n.first->dp.pos, pos - mpos};
                    }
                    if (_selectedNodes.count(_grabbedNode)) {
                        if (IsKeyDown(KEY_LEFT_CONTROL))
                            _selectedNodes.erase(_grabbedNode);
                            
                    } else {
                        {
                            auto selectedNodes = _selectedNodes;
                            for (auto& sn : selectedNodes)
                                if (sn.first->content && _grabbedNode->hg->isChildOf(sn.first->content)) 
                                    _selectedNodes.erase(sn.first);
                        }
                        auto pos = (_scale * _grabbedNode->hg->scale() * _grabbedNode->dp.pos + _offset);
                        _selectedNodes[_grabbedNode] = {_grabbedInitPos, pos - mpos};
                        if (_grabbedNode->content) {
                            auto selectedNodes = _selectedNodes;
                            for (auto& sn : selectedNodes)
                                if (sn.first->hg->isChildOf(_grabbedNode->content)) 
                                    _selectedNodes.erase(sn.first);
                        }
                    }
                } else {
                    _selectedNodes.clear();
                    if (IsKeyDown(KEY_LEFT_CONTROL))
                        _selectionStart = mpos;
                }
            } else if (_grabbedNode) {
                if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
                    auto selectedNodes = _selectedNodes;
                    for (auto& n : selectedNodes) {
                        _mhg.moveNode(n.first, n.second.first, n.first->dp.pos);
                        if (_mhg._historyRecording) _mhg._histIt-=2;
                    }
                    for (auto& n : selectedNodes)
                        if (!n.first->via)
                            _dropNode(n.first, n.first->dp.posCache);
                    int c = 0;
                    for (auto& n : selectedNodes) {
                        _mhg.moveNode(n.first, n.second.first, n.first->dp.pos);
                        if (_mhg._historyRecording && c != selectedNodes.size() - 1) _mhg._histIt-=2;
                        c++;
                    }
                    _grabbedNode = nullptr;
                } else {
                    for (auto& n : _selectedNodes) {
                        float ls = n.first->hg->scale() * _scale;
                        n.first->dp.pos = ((mpos - _offset + _curOffset) + n.second.second) / ls;
                    }
                }
            } else if (_selectionStart.x > 0) {
                _selectedNodes.clear();
                std::set<NodePtr> selected;
                _mhg.getNodesIn(_selectionRect, selected);
                for (auto sn : selected)
                    _selectedNodes[sn] = {Vector2Zero(), Vector2Zero()};
                if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON))
                    _selectionStart = Vector2Zero();
            }
            
            // TOGGLE FULLSCREEN
            if (IsKeyPressed(KEY_F) || IsKeyPressed(KEY_F11)) {
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

            // PAN + ZOOM
            if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT))
                _lastMouseGrabPos = mpos;
            if (IsMouseButtonReleased(MOUSE_BUTTON_RIGHT))
                _offset = _offset + _curOffset;
            _curOffset = IsMouseButtonDown(MOUSE_BUTTON_RIGHT) ? (mpos - _lastMouseGrabPos) : Vector2Zero();
            float newScale = _scale + GetMouseWheelMove() * _scale / 10.0f;
            _offset -= ((mpos - _offset) / _scale) * (newScale - _scale);
            _scale = newScale;

            // RECENTER
            if (IsKeyPressed(KEY_C))
                recenter();

            // EDIT
            if (IsKeyPressed(KEY_A)) {
                auto node = _mhg.addNode("", RED, _hoverNode);
                node->hyper = IsKeyDown(KEY_LEFT_SHIFT);
                auto hg = _hoverNode ? _hoverNode->content : _mhg._root;
                auto o = _hoverNode ? _hoverNode->dp.posCache : _offset;
                _mhg.moveNode(node, Vector2Zero(), (mpos - o) / (hg->scale() * _scale));
            } else if (IsKeyPressed(KEY_DELETE)) {
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

            if (IsKeyPressed(KEY_E)) {
                if (_hoverEdgeLink) {
                    _addEdgeFromEdge = _hoverEdgeLink->edge;
                    _addEdgeFromMpos = mpos;
                } else if (_hoverNode) {
                    _addEdgeFromNode = _hoverNode;
                }
            }
            if (IsKeyDown(KEY_E) && (_addEdgeFromNode || _addEdgeFromEdge)) {
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
            if (IsKeyReleased(KEY_E)) {
                if ((_addEdgeFromNode || _addEdgeFromEdge) && (_addEdgeToNode || _addEdgeToEdge)) {
                    auto from = _addEdgeFromNode ? _addEdgeFromNode : _makeHyperAndMoveToMpos(_addEdgeFromEdge, _addEdgeFromMpos);
                    if (_mhg._historyRecording && !_addEdgeFromNode) _mhg._histIt-=2;
                    auto to = _addEdgeToNode ? _addEdgeToNode : _makeHyperAndMoveToMpos(_addEdgeToEdge, mpos);
                    if (_mhg._historyRecording && !_addEdgeToNode) _mhg._histIt-=2;
                    auto style = (!IsKeyDown(KEY_LEFT_SHIFT) && _lastLinkStyle) ? _lastLinkStyle : EdgeLinkStyle::create();
                    _mhg.addEdge(style, from, to);
                    _lastLinkStyle = style;
                }
                _addEdgeFromNode = nullptr;
                _addEdgeFromEdge = nullptr;
                _addEdgeToNode = nullptr;
                _addEdgeToEdge = nullptr;
            }
            if (IsKeyPressed(KEY_H)) {
                if (_hoverEdgeLink)
                    _makeHyperAndMoveToMpos(_hoverEdgeLink->edge, mpos);
                else if (_hoverNode) {
                    _hoverNode->hyper = !_hoverNode->hyper;
                    _mhg.noticeAction({.type = MHGactionType::HYPER, .inverse = _hoverNode->hyper, .n = _hoverNode});
                }
            }
            
            // HISTORY
            if (IsKeyDown(KEY_LEFT_CONTROL)) {
                if (!IsKeyDown(KEY_LEFT_SHIFT) && IsKeyPressed(KEY_Z))
                    _mhg.undo();
                if (IsKeyPressed(KEY_Y) || (IsKeyDown(KEY_LEFT_SHIFT) && IsKeyPressed(KEY_Z)))
                    _mhg.redo();
            }
        }
    }

    void DrawerImpl::_draw() {
        BeginDrawing();
        ClearBackground(BLACK);
        _hoverNode = nullptr;
        _hoverEdgeLink = nullptr;
        _mhg.draw(_offset + _curOffset, _scale, _font, _selectedNodes, _hoverNode, _hoverEdgeLink);
        if (_addEdgeFromNode || _addEdgeFromEdge) {
            auto from = _addEdgeFromNode ? _addEdgeFromNode->dp.posCache : _addEdgeFromMpos;
            auto to = GetMousePosition();
            DrawLineEx(from, to, EDGE_THICK, WHITE);
        }
        if (_selectionStart.x > 0) {
            auto mpos = GetMousePosition();
            _selectionRect = Rectangle{std::min(_selectionStart.x, mpos.x), std::min(_selectionStart.y, mpos.y), fabs(mpos.x - _selectionStart.x), fabs(mpos.y - _selectionStart.y)};
            DrawRectangleLines(_selectionRect.x, _selectionRect.y, _selectionRect.width, _selectionRect.height, WHITE);
            _hoverNode = nullptr;
            _hoverEdgeLink = nullptr;
        }
        if (_hoverEdgeLink) _hoverEdgeLink->highlight = HIGHLIGHT_INTENSITY;
        if (_hoverNode) _hoverNode->dp.highlight = HIGHLIGHT_INTENSITY;
        if (_addEdgeFromNode) _addEdgeFromNode->dp.highlight = HIGHLIGHT_INTENSITY_2;
        if (_addEdgeFromEdge) _addEdgeFromEdge->highlight = HIGHLIGHT_INTENSITY_2;
        if (_addEdgeToNode) _addEdgeToNode->dp.highlight = HIGHLIGHT_INTENSITY_2;
        if (_addEdgeToEdge) _addEdgeToEdge->highlight = HIGHLIGHT_INTENSITY_2;
        auto selectedNodes = _selectedNodes;
        for (auto& sn : selectedNodes) {
            if (sn.first->dp.highlight == HIGHLIGHT_INTENSITY_2)
                _selectedNodes.erase(sn.first);
            else
                sn.first->dp.highlight = HIGHLIGHT_INTENSITY_2;
        }
        EndDrawing();
    }

    std::shared_ptr<Drawer> Drawer::create(MetaHyperGraph& w, Vector2 winSize, std::string winName) {
        return std::make_shared<DrawerImpl>(w, winSize, winName);
    }
}