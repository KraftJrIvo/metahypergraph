#include "drawer.h"
#include "raylib.h"
#include "raymath.h"
#include <memory>
#include <string>

#include "util/utf8.cpp"

#include <codecvt>
#include <locale>

#define INPUT_LINE_H 100

namespace mhg {
    class DrawerImpl final : public Drawer {
    public:
        DrawerImpl(MetaHyperGraph& mhg, Vector2 winSize, std::string winName);
        ~DrawerImpl() { _redrawer.join(); };

        void recenter();
    private:
        std::thread _redrawer;
        Vector2 _offset = Vector2Zero();
        Vector2 _curOffset = Vector2Zero();
        Vector2 _lastMouseGrabPos = Vector2Zero();

        float _scale = 1.0f;
        Font _font;
        const std::string _allChars;

        NodePtr _hoverNode = nullptr;
        EdgeLinkHoverPtr _hoverEdgeLink = nullptr;

        NodePtr _grabbedNode = nullptr;
        Vector2 _grabbedInitPos = Vector2Zero();
        Vector2 _grabOff = Vector2Zero();

        NodePtr _addEdgeFromNode = nullptr;
        NodePtr _addEdgeToNode = nullptr;
        EdgePtr _addEdgeFromEdge = nullptr;
        EdgePtr _addEdgeToEdge = nullptr;

        std::string _labelPriorToEdit;

        void _startEditingNode(NodePtr node);
        void _editNode();
        void _stopEditingNode();

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
        _editingNode->editing = true;
        _labelPriorToEdit = _editingNode->label;
        _editingNode->label += '_';
    }

    void DrawerImpl::_editNode() {
        if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_ESCAPE)) {
            if (IsKeyPressed(KEY_ESCAPE))
                _editingNode->label = _labelPriorToEdit;
            else
                _editingNode->label = _editingNode->label.substr(0, _editingNode->label.length() - 1);
            _stopEditingNode();
        }
        if (IsKeyPressed(KEY_BACKSPACE) && _editingNode->label.length()) {
            _editingNode->label = _editingNode->label.substr(0, _editingNode->label.length() - 1);
            do { _editingNode->label = _editingNode->label.substr(0, _editingNode->label.length() - 1);
            } while (!validate_utf8(_editingNode->label.c_str()));
            _editingNode->label += '_';
        }
        wchar_t ch = GetCharPressed();
        if (ch) {
            _editingNode->label = _editingNode->label.substr(0, _editingNode->label.length() - 1);
            std::wstring ws(L"a");
            ws.at(0) = ch;
            using convert_typeX = std::codecvt_utf8<wchar_t>;
            std::wstring_convert<convert_typeX, wchar_t> converterX;
            std::string str = converterX.to_bytes(ws);
            if (ch)
                _editingNode->label += str;
            _editingNode->label += '_';
        }
    }

    void DrawerImpl::_stopEditingNode() {
        _editingNode->editing = false;
        _editingNode = nullptr;
    }

    void DrawerImpl::_update() {
        if (_editingNode) {
            _editNode();
        } else {

            if (_hoverNode && !_hoverNode->hyper && IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && IsGestureDetected(GESTURE_DOUBLETAP)) {
                _startEditingNode(_hoverNode);
                _hoverNode = nullptr;
            }

            auto mpos = GetMousePosition();
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && !IsMouseButtonPressed(MOUSE_BUTTON_RIGHT) && _hoverNode) {
                _grabbedNode = _hoverNode;
                _grabbedInitPos = _hoverNode->pos;
                _grabOff = (_scale * _hoverNode->hg->scale() * _hoverNode->pos + _offset) - mpos;
            } else if (_grabbedNode) {
                if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
                    if (!_grabbedNode->via && !_grabbedNode->hyper) {
                        auto dropNode = _mhg.getNodeAt(_grabbedNode->_posCache, {_grabbedNode});
                        auto dropHG = dropNode ? dropNode->content : _mhg._root;
                        if (dropHG != _grabbedNode->hg) {
                            if (!dropHG)
                                dropHG = dropNode->content = std::make_shared<HyperGraph>(dropNode);
                            _mhg.transferNode(dropHG, _grabbedNode);
                            auto o = (dropHG == _mhg._root) ? _offset : dropNode->_posCache;
                            _grabbedNode->pos = (_grabbedNode->_posCache - o) / (dropHG->scale() * _scale);
                        }
                    }
                    _mhg.moveNode(_grabbedNode, _grabbedInitPos, _grabbedNode->pos);
                    _grabbedNode = nullptr;
                } else {
                    float ls = _grabbedNode->hg->scale() * _scale;
                    _grabbedNode->pos = ((mpos - _offset + _curOffset) + _grabOff) / ls;
                }
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
                auto o = _hoverNode ? _hoverNode->_posCache : _offset;
                _mhg.moveNode(node, Vector2Zero(), (mpos - o) / (hg->scale() * _scale));
                if (!node->hyper)
                    _startEditingNode(node);
            } else if (IsKeyPressed(KEY_DELETE)) {
                if (_hoverEdgeLink) {
                    auto e = std::make_shared<Edge>(_hoverEdgeLink->first->idx, _hoverEdgeLink->second, _hoverEdgeLink->first->from, _hoverEdgeLink->first->via, _hoverEdgeLink->first->to);
                    _mhg.reduceEdge(e);
                    _hoverEdgeLink = nullptr;
                } else if (_hoverNode) {
                    _mhg.removeNode(_hoverNode);
                    _hoverNode = nullptr;
                }
            }

            if (IsKeyPressed(KEY_E)) {
                if (_hoverEdgeLink) {
                    _addEdgeFromEdge = _hoverEdgeLink->first;
                } else if (_hoverNode) {
                    _addEdgeFromNode = _hoverNode;
                }
            }
            if (IsKeyDown(KEY_E) && (_addEdgeFromNode || _addEdgeFromEdge)) {
                if (_hoverEdgeLink && _hoverEdgeLink->first != _addEdgeFromEdge) {
                    _addEdgeToEdge = _hoverEdgeLink->first;
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
                    auto from = _addEdgeFromNode ? _addEdgeFromNode : _mhg.makeEdgeHyper(_addEdgeFromEdge);
                    if (_mhg._historyRecording && !_addEdgeFromNode) _mhg._histIt-=2;
                    auto to = _addEdgeToNode ? _addEdgeToNode : _mhg.makeEdgeHyper(_addEdgeToEdge);
                    if (_mhg._historyRecording && !_addEdgeToNode) _mhg._histIt-=2;
                    _mhg.addEdge(EdgeLinkStyle{}, from, to);
                }
                _addEdgeFromNode = nullptr;
                _addEdgeFromEdge = nullptr;
                _addEdgeToNode = nullptr;
                _addEdgeToEdge = nullptr;
            }
            if (IsKeyPressed(KEY_H))
                if (_hoverEdgeLink)
                    _mhg.makeEdgeHyper(_hoverEdgeLink->first);
            
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
        _mhg.draw(_offset + _curOffset, _scale, _font, _grabbedNode, _hoverNode, _hoverEdgeLink);
        if (_addEdgeFromNode || _addEdgeFromEdge) {
            auto from = _addEdgeFromNode ? _addEdgeFromNode->_posCache : (_addEdgeFromEdge->from->_posCache + _addEdgeFromEdge->to->_posCache) * 0.5f;
            auto to = GetMousePosition();
            DrawLineEx(from, to, EDGE_THICK, WHITE);
        }
        if (_hoverEdgeLink) _hoverEdgeLink->first->links.find(_hoverEdgeLink->second)->highlight = HIGHLIGHT_INTENSITY;
        if (_hoverNode) _hoverNode->highlight = HIGHLIGHT_INTENSITY;
        if (_addEdgeFromNode) _addEdgeFromNode->highlight = HIGHLIGHT_INTENSITY_2;
        if (_addEdgeFromEdge) _addEdgeFromEdge->highlight = HIGHLIGHT_INTENSITY_2;
        if (_addEdgeToNode) _addEdgeToNode->highlight = HIGHLIGHT_INTENSITY_2;
        if (_addEdgeToEdge) _addEdgeToEdge->highlight = HIGHLIGHT_INTENSITY_2;
        EndDrawing();
    }

    std::shared_ptr<Drawer> Drawer::create(MetaHyperGraph& w, Vector2 winSize, std::string winName) {
        return std::make_shared<DrawerImpl>(w, winSize, winName);
    }
}