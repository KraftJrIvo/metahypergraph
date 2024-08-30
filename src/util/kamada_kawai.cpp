#include "../types/hypergraph.h"

void mhg::HyperGraph::kamadaKawai() {
    size_t N = _nodes.size();
    mhg::Matrix D; floydWarshall(D);
    mhg::Matrix L   = SPRING_LEN * D;
    mhg::Matrix K   = SPRING_STR * D.array().pow(-2);
    mhg::Matrix Ex  = mhg::Matrix::Zero(N, N);
    mhg::Matrix Ey  = mhg::Matrix::Zero(N, N);
    mhg::Vector Exs = mhg::Vector::Zero(N);
    mhg::Vector Eys = mhg::Vector::Zero(N);
    for (size_t m = 0; m < N; ++m) {
        const auto& pos_m = _nodes[m]->pos;
        for (size_t i = m; i < N; ++i) {
            if (i != m) {
                const auto& pos_i = _nodes[i]->pos;
                const float denom = 1.0f / Vector2Distance(pos_m, pos_i);
                const auto E = K(m, i) * (pos_m - pos_i - L(m, i) * (pos_m - pos_i) * denom);
                Ex(m, i) = Ex(i, m) = E.x;
                Ey(m, i) = Ey(i, m) = E.y;
                Exs(m) += E.x;
                Eys(m) += E.y;
            }
        }
    }

    auto getEnrg = [&](size_t idx, Vector2& dE_dpos, float& dM) {
        dE_dpos = {Exs[idx], Eys[idx]};
        dM = Vector2Length(dE_dpos);
    };

    auto getHighestEnergyNode = [&](size_t& maxEnrgNodeId, float& maxEnrg, Vector2& max_dE_dpos) {
        Vector2 dE_dpos; float dM; float maxE = 0;
        for (size_t m = 0; m < N; ++m) {
            getEnrg(m, dE_dpos, dM);
            if (dM > maxE) {
                maxE = dM;
                maxEnrgNodeId = m;
                max_dE_dpos = dE_dpos;
            }
        }
        maxEnrg = maxE;
    };

    auto updateE = [&](size_t idx) {
        const auto& posM = _nodes[idx]->pos;
        Vector2 dE_dpos = Vector2Zero();
        for (size_t i = 0; i < N; ++i) {
            if (i != idx) {
                const float oldDx = Ex(idx, i);
                const float oldDy = Ey(idx, i);
                const auto& posI = _nodes[i]->pos;
                const float denom = 1.0f / Vector2Distance(posI, posM);
                float dx = K(idx, i) * (posM.x - posI.x - L(idx, i) * (posM.x - posI.x) * denom);
                float dy = K(idx, i) * (posM.y - posI.y - L(idx, i) * (posM.y - posI.y) * denom);
                Ex(idx, i) = dx;
                Ey(idx, i) = dy;
                dE_dpos += {dx, dy};
                Exs(i) += dx - oldDx;
                Eys(i) += dy - oldDy;
            }
        }
        Exs(idx) = dE_dpos.x;
        Eys(idx) = dE_dpos.y;
    };

    const float THRESH = 1e-2f;
    const float INNER_THRESH = 1.0f;
    const int MAX_ITS = std::max(1000, std::min(10 * int(N), 6000));
    const int MAX_INNER_ITS = 5;
    int its = 0;
    int subIts = 0;
    float maxEnrg = 1e9f, dM = 0.0f;
    size_t maxEnrgNodeId; 
    Vector2 dE_dpos;

    auto moveNode = [&](size_t idx, const Vector2& dE_dpos) {
        float d2E_dx2 = 0;
        float d2E_dxdy = 0;
        float d2E_dy2 = 0;
        const auto& posM = _nodes[idx]->pos;
        for (size_t i = 0; i < N; ++i) {
            if (i != idx) {
                const auto& posI = _nodes[i]->pos;
                const float denom = 1.0f / std::pow(Vector2DistanceSqr(posM, posI), 1.5);
                const float kmat = K(idx, i);
                const float lmat = L(idx, i);
                d2E_dx2 += kmat * (1.0f - lmat * (posM.y - posI.y) * (posM.y - posI.y) * denom);
                d2E_dxdy += kmat * (lmat * (posM.x - posI.x) * (posM.y - posI.y) * denom);
                d2E_dy2 += kmat * (1.0f - lmat * (posM.x - posI.x) * (posM.x - posI.x) * denom);
            }
        }
        const auto& A = d2E_dx2;
        const auto& B = d2E_dxdy;
        const auto& C = dE_dpos.x;
        const auto& I = d2E_dy2;
        const auto& J = dE_dpos.y;
        const float dy = (C / A + J / B) / (B / A - I / B);
        const float dx = -(B * dy + C) / A;
        _nodes[idx]->pos += {dx, dy};
        updateE(idx);
    };
    while (maxEnrg > THRESH && its < MAX_ITS) {
        getHighestEnergyNode(maxEnrgNodeId, maxEnrg, dE_dpos);
        dM = maxEnrg;
        subIts = 0;
        while (dM > INNER_THRESH && subIts < MAX_INNER_ITS) {
            moveNode(maxEnrgNodeId, dE_dpos);
            getEnrg(maxEnrgNodeId, dE_dpos, dM);
            subIts++;
        }
        its++;
    }
}