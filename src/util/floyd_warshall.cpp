#include "../types/hypergraph.h"

void mhg::HyperGraph::floydWarshall(mhg::Matrix& D) {
    size_t N = _nodes.size();
    D = (mhg::Matrix::Ones(N, N) - mhg::Matrix::Identity(N, N)) * 1e9f;
    for (auto& e : _edges) {
        auto fromIdx = e.second->from->idx;
        auto toIdx = e.second->to->idx;
        D(fromIdx, toIdx) = D(toIdx, fromIdx) = 1.0f;
    }
    for (size_t k = 0; k < N; ++k) {
        for (size_t i = 0; i < N - 1; ++i) {
            for (size_t j = i + 1; j < N; ++j) {
                float val = std::min(D.row(i)[j], D.row(i)[k] + D.row(k)[j]);
                D(i, j) = D(j, i) = val;
            }
        }
    }
}