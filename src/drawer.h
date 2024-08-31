#pragma once

#include "types/metahypergraph.h"
#include <memory>
#include <thread>
#include <map>

namespace mhg {
    class Drawer
    {
    public:
        Drawer(MetaHyperGraph& mhg, Vector2 winSize = { 512, 512 }, std::string winName = "") :
            _mhg(mhg), _baseWinSize(winSize), _winSize(winSize), _winName(winName)
        { }

        void stop() { _drawing = false; }
        bool isDrawing() { return _drawing; }
        bool isChanged() { bool c = _changed; _changed = false; return c; }
        
        virtual void recenter() = 0;

        static std::shared_ptr<Drawer> create(MetaHyperGraph& mhg, Vector2 winSize = { 512, 512 }, std::string winName = "");

    protected:
        MetaHyperGraph& _mhg;

        Vector2 _baseWinSize;
        Vector2 _winSize;
        std::string _winName;
        std::string _windowName;

        NodePtr grabbed = nullptr;
        Vector2 grabbedOff = Vector2Zero();

        bool _drawing = true;
        bool _changed = true;
    };
}