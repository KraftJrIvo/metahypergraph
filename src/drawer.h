#pragma once

#include "hypermetagraph.h"
#include <memory>
#include <thread>
#include <map>

namespace hmg {
    class Drawer
    {
    public:
        Drawer(HyperMetaGraph& hmg, Vector2 winSize = { 512, 512 }, std::string winName = "") :
            _hmg(hmg), _baseWinSize(winSize), _winSize(winSize), _winName(winName)
        { }
        ~Drawer() { _redrawer.join(); }

        void stop() { _drawing = false; }
        bool isDrawing() { return _drawing; }
        bool isChanged() { bool c = _changed; _changed = false; return c; }
        
        virtual void recenter() = 0;

        static std::shared_ptr<Drawer> create(HyperMetaGraph& hmg, Vector2 winSize = { 512, 512 }, std::string winName = "");

    protected:
        HyperMetaGraph& _hmg;

        Vector2 _baseWinSize;
        Vector2 _winSize;
        std::string _winName;
        std::thread _redrawer;
        std::string _windowName;

        NodePtr grabbed = nullptr;
        Vector2 grabbedOff = Vector2Zero();

        bool _drawing = true;
        bool _changed = true;
    };
}