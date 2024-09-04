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
        EdgePtr _hoverEdge = nullptr;
        NodePtr _hoverNodePrv = nullptr;
        EdgePtr _hoverEdgePrv = nullptr;

        std::string _labelPriorToEdit;

        void _startEditingNode(NodePtr node);
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
            int c; auto cdpts = LoadCodepoints(_allChars.c_str(), &c);
            _font = LoadFontEx("res/firacode.ttf", FONT_SZ, cdpts, c);
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

    void DrawerImpl::_stopEditingNode() {
        _editingNode->editing = false;
        _editingNode = nullptr;
    }

    void DrawerImpl::_update() {
        if (!_editingNode) {

            if (_hoverNode && IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && IsGestureDetected(GESTURE_DOUBLETAP)) {
                _startEditingNode(_hoverNode);
                _hoverNode = nullptr;
            }

            auto mpos = GetMousePosition();
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && !IsMouseButtonPressed(MOUSE_BUTTON_RIGHT) && _hoverNode)
                _mhg.grabNode(_hoverNode, (_scale * _hoverNode->hg->scale() * _hoverNode->pos + _offset) - mpos);
            else if (_mhg.nodeGrabbed()) {
                if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
                    auto dropNode = _mhg.getNodeAt(_mhg._grabbedNode->_posCache, {_mhg._grabbedNode});
                    auto dropHG = dropNode ? dropNode->content : _mhg._root;
                    if (dropHG != _mhg._grabbedNode->hg) {
                        if (!dropHG)
                            dropHG = dropNode->content = std::make_shared<HyperGraph>(dropNode);
                        dropHG->transferNode(dropHG, _mhg._grabbedNode);
                        auto o = (dropHG == _mhg._root) ? _offset : dropNode->_posCache;
                        _mhg._grabbedNode->pos = (_mhg._grabbedNode->_posCache - o) / (dropHG->scale() * _scale);
                    }
                    _mhg.ungrabNode();
                } else {
                    _mhg.dragNode(_offset + _curOffset, _scale, mpos);
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
                auto hg = _hoverNode ? _hoverNode->content : _mhg._root;
                auto o = _hoverNode ? _hoverNode->_posCache : _offset;
                node->pos = (mpos - o) / (hg->scale() * _scale);
                _startEditingNode(node);
            } else if (IsKeyPressed(KEY_DELETE)) {
                if (_hoverEdge) {
                    _mhg.removeEdge(_hoverEdge);
                    _hoverEdge = nullptr;
                } else if (_hoverNode) {
                    _mhg.removeNode(_hoverNode);
                    _hoverNode = nullptr;
                }
            }
        } else {
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
    }

    void DrawerImpl::_draw() {
        BeginDrawing();
        ClearBackground(BLACK);
        _hoverNode = nullptr;
        _hoverEdge = nullptr;
        if (_hoverEdgePrv) _hoverEdgePrv->highlight = true;
        if (_hoverNodePrv) _hoverNodePrv->highlight = true;
        _mhg.draw(_offset + _curOffset, _scale, _font, _hoverNode, _hoverEdge);
        if (_hoverEdgePrv) _hoverEdgePrv->highlight = false;
        if (_hoverNodePrv) _hoverNodePrv->highlight = false;
        _hoverEdgePrv = _mhg.nodeGrabbed() ? nullptr : _hoverEdge;
        _hoverNodePrv = _mhg.nodeGrabbed() ? _mhg._grabbedNode : _hoverNode;
        EndDrawing();
    }

    std::shared_ptr<Drawer> Drawer::create(MetaHyperGraph& w, Vector2 winSize, std::string winName) {
        return std::make_shared<DrawerImpl>(w, winSize, winName);
    }
}