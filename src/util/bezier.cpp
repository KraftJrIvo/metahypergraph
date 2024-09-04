#include "../types/edge.h"
#include "raylib.h"
#include "raymath.h"

void mhg::Edge::findArrowPositionBezier(bool atStart, float scale, Vector2& pos, float& angle, float& t) {
    auto node = atStart ? from : to;
    auto nodepos = atStart ? _pts[0] : _pts[2];
    const int maxIterations = 30;
    const float threshold = 0.2;
    float low = 0, high = 1;
    float middle;

    int iteration = 0;
    do {
        middle = (low + high) * 0.5;
        pos = getPoint(middle);
        float distToPt = Vector2Distance(pos, nodepos);
        angle = atan2(nodepos.y - pos.y, nodepos.x - pos.x);
        //float distToBorder = node->getDistToBorder(angle, scale);
        float distToBorder = node->_rCache;
        float diff = distToBorder - distToPt;
        if (abs(diff) < threshold)
            break;
        if (diff < 0)
            if (atStart)
                high = middle;
            else
                low = middle;
        else
            if (atStart)
                low = middle;
            else
                high = middle;
        iteration++;
    } while (low <= high && iteration < maxIterations);

    float t2 = middle + (atStart ? 0.01f : -0.01f);
    Vector2 pos2 = getPoint(t2);
    angle = atan2(pos.y - pos2.y, pos.x - pos2.x);
    t = middle;
  } 

#define SPLINE_SEGMENT_DIVISIONS 24
bool mhg::Edge::DrawSplineSegmentBezierQuadraticPart(Vector2 p1, Vector2 c2, Vector2 p3, float thick, Color color, float start, float end)
{
    const float step = (end - start)/SPLINE_SEGMENT_DIVISIONS;

    Vector2 previous = p1;
    if (start != 0) {
        float a = powf(1.0f - start, 2);
        float b = 2.0f*(1.0f - start)*start;
        float c = powf(start, 2);
        previous.y = a*p1.y + b*c2.y + c*p3.y;
        previous.x = a*p1.x + b*c2.x + c*p3.x;
    }
    bool hover = false;
    Vector2 current = { 0 };
    float t;

    Vector2 points[2*SPLINE_SEGMENT_DIVISIONS + 2];

    for (int i = 1; i <= SPLINE_SEGMENT_DIVISIONS; i++)
    {
        t = start + step * i;

        float a = powf(1.0f - t, 2);
        float b = 2.0f*(1.0f - t)*t;
        float c = powf(t, 2);

        // NOTE: The easing functions aren't suitable here because they don't take a control point
        current.y = a*p1.y + b*c2.y + c*p3.y;
        current.x = a*p1.x + b*c2.x + c*p3.x;

        float dy = current.y - previous.y;
        float dx = current.x - previous.x;
        float size = 0.5f*thick/sqrtf(dx*dx+dy*dy);

        if (i == 1)
        {
            points[0].x = previous.x + dy*size;
            points[0].y = previous.y - dx*size;
            points[1].x = previous.x - dy*size;
            points[1].y = previous.y + dx*size;
        }

        points[2*i + 1].x = current.x - dy*size;
        points[2*i + 1].y = current.y + dx*size;
        points[2*i].x = current.x + dy*size;
        points[2*i].y = current.y - dx*size;

        hover |= CheckCollisionPointLine(GetMousePosition(), previous, current, thick * 2);

        previous = current;
    }

    DrawTriangleStrip(points, 2*SPLINE_SEGMENT_DIVISIONS + 2, highlight ? ColorBrightness(color, 0.25f) : color);
    return hover;
}