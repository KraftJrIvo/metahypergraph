#pragma once

#include <vector>
#define _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING

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
        virtual bool isEditing() { return false; }
        bool isChanged() { bool c = _changed; _changed = false; return c; }
        
        virtual void recenter() = 0;

        static std::shared_ptr<Drawer> create(MetaHyperGraph& mhg, Vector2 winSize = { 512, 512 }, std::string winName = "");

    protected:
        static std::vector<Color> COLORS;

        MetaHyperGraph& _mhg;

        Vector2 _baseWinSize;
        Vector2 _winSize;
        std::string _winName;
        std::string _windowName;

        bool _drawing = true;
        bool _changed = true;
    };
}