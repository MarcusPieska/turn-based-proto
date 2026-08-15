//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "worker_job_imp_index.h"

//================================================================================================================================
//=> - WorkerJobImpIndex -
//================================================================================================================================

WorkerJobImpIndex::WorkerJobImpIndex () : m_idx(nullptr), m_off(nullptr), m_job_n(0), m_imp_n(0) {
}

WorkerJobImpIndex::~WorkerJobImpIndex () {
    clear();
}

void WorkerJobImpIndex::clear () {
    delete[] m_idx;
    delete[] m_off;
    m_idx = nullptr;
    m_off = nullptr;
    m_job_n = 0;
    m_imp_n = 0;
}

void WorkerJobImpIndex::take_ownership () {
    if (m_idx != nullptr && m_imp_n > 0) {
        u16* tmp = m_idx;
        m_idx = new u16[m_imp_n];
        for (u16 i = 0; i < m_imp_n; ++i) {
            m_idx[i] = tmp[i];
        }
        delete[] tmp;
    }
    if (m_off != nullptr && m_job_n > 0) {
        u16* tmp = m_off;
        m_off = new u16[m_job_n + 1u];
        for (u16 i = 0; i <= m_job_n; ++i) {
            m_off[i] = tmp[i];
        }
        delete[] tmp;
    }
}

const u16* WorkerJobImpIndex::imps (u16 job_idx) const {
    if (m_off == nullptr || m_idx == nullptr || job_idx >= m_job_n) {
        return nullptr;
    }
    if (m_off[job_idx] == m_off[job_idx + 1u]) {
        return nullptr;
    }
    return m_idx + m_off[job_idx];
}

u16 WorkerJobImpIndex::imp_n (u16 job_idx) const {
    if (m_off == nullptr || job_idx >= m_job_n) {
        return 0;
    }
    return static_cast<u16>(m_off[job_idx + 1u] - m_off[job_idx]);
}

u16 WorkerJobImpIndex::job_n () const {
    return m_job_n;
}

u16 WorkerJobImpIndex::imp_total () const {
    return m_imp_n;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
