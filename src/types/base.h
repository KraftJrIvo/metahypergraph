#pragma once

#include "config.h"

#include <memory>

#define RAND_FLOAT static_cast <float> (rand()) / (static_cast <float> (RAND_MAX) + 1.0f)

#include <Eigen/Core>

namespace mhg {
    struct Node;
    typedef std::shared_ptr<Node> NodePtr;    
    struct Edge;
    typedef std::shared_ptr<Edge> EdgePtr;
    class HyperGraph;
    typedef std::shared_ptr<HyperGraph> HyperGraphPtr;

    typedef Eigen::Matrix<float, -1,  1> Vector;
    typedef Eigen::Matrix<float, -1, -1> Matrix;
}