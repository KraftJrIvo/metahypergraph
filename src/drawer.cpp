#include "drawer.h"
#include "raylib.h"
#include "raymath.h"
#include <string>

#define INPUT_LINE_H 100

namespace hmg {
    class DrawerImpl : public Drawer {
    public:
        DrawerImpl(HyperMetaGraph& hmg, Vector2 winSize, std::string winName);

        void recenter();
    private:
        Vector2 _offset = Vector2Zero();
        Vector2 _curOffset = Vector2Zero();
        Vector2 _lastMouseGrabPos = Vector2Zero();

        float _scale = 1.0f;
        Font _font;

        NodePtr _hoverNode = nullptr;

        void _draw();
        void _update();
    };

    DrawerImpl::DrawerImpl(HyperMetaGraph& hmg, Vector2 winSize, std::string winName) :
        Drawer(hmg, winSize, winName)
    {
        _redrawer = std::thread([&]() {
            SetTraceLogLevel(LOG_ERROR);
            SetConfigFlags(FLAG_MSAA_4X_HINT);
            InitWindow(_winSize.x, _winSize.y + INPUT_LINE_H, _winName.c_str());
            std::string txt = u8" !\"#$%&\'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~абвгдеёжзийклмнопрстуфхцчшщъыьэюяАБВГДЕЁЖЗИЙКЛМНОПРСТУФХЦЧШЩЪЫЬЭЮЯ";
            int c; auto cdpts = LoadCodepoints(txt.c_str(), &c);
            _font = LoadFontEx("res/sofia-sans-extra-condensed.ttf", 128, cdpts, c);
            UnloadCodepoints(cdpts);
            SetTargetFPS(60);
            recenter();
            while (!WindowShouldClose() && _drawing) {
                _draw();
                _update();
            }
            CloseWindow();
            _drawing = false;
        });
    }
    
    void DrawerImpl::recenter() {
        Vector2 center = _hmg.getCenter();
        _offset = -center * _scale + _winSize * 0.5f;
    }

    void DrawerImpl::_update() {
        auto mpos = GetMousePosition();
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && _hoverNode)
            _hmg.grabNode(_hoverNode, (_scale * _hoverNode->pos + _offset) - mpos);
        else if (_hmg.nodeGrabbed()) {
            if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT))
                _hmg.ungrabNode();
            else
                _hmg.dragNode(_offset + _curOffset, _scale, mpos);
        }
        
        // TOGGLE FULLSCREEN
        if (IsKeyPressed(KEY_F)) {
            if (!IsWindowFullscreen()) {
                auto newWinSz = Vector2{(float)GetMonitorWidth(0), (float)GetMonitorHeight(0)};
                _offset += (newWinSz - _winSize) * 0.5f - Vector2{0.0f, 50.0f};
                _winSize = newWinSz;
                SetWindowSize(int(_winSize.x), int(_winSize.y));
            }
            ToggleFullscreen();
            if (!IsWindowFullscreen()) {
                auto newWinSz = _baseWinSize;
                _offset += (newWinSz - _winSize) * 0.5f + Vector2{0.0f, 50.0f};
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
    }

    void DrawerImpl::_draw() {
        BeginDrawing();
        ClearBackground(BLACK);
        _hoverNode = nullptr;
        _hmg.draw(_offset + _curOffset, _scale, _font, _hoverNode);
        EndDrawing();
    }

    std::shared_ptr<Drawer> Drawer::create(HyperMetaGraph& w, Vector2 winSize, std::string winName) {
        return std::make_shared<DrawerImpl>(w, winSize, winName);
    }
}