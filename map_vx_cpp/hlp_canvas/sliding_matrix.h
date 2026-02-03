//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef SLIDING_MATRIX_H
#define SLIDING_MATRIX_H

#include <vector>

//================================================================================================================================
//=> - Class: SlidingMatrix -
//================================================================================================================================

template<typename T>
class SlidingMatrix {
public:
    SlidingMatrix ();

    void addRowHead ();
    void addRowTail ();
    void delRowHead ();
    void delRowTail ();
    void addColumnHead ();
    void addColumnTail ();
    void delColumnHead ();
    void delColumnTail ();

    T get (int row, int col) const;
    void set (int row, int col, T value);
    int getRows () const;
    int getCols () const;

protected:
    std::vector<std::vector<T>> m_data;
    int m_rows;
    int m_cols;
};

//================================================================================================================================
//=> - SlidingMatrix implementation -
//================================================================================================================================

template<typename T>
SlidingMatrix<T>::SlidingMatrix () : m_rows(0), m_cols(0) {
}

template<typename T>
void SlidingMatrix<T>::addRowTail () {
    if (m_cols == 0) {
        m_cols = 1;
    }
    m_data.push_back(std::vector<T>(m_cols, T{}));
    m_rows++;
}

template<typename T>
void SlidingMatrix<T>::delRowTail () {
    if (m_rows == 0) return;
    if (m_rows == 1) {
        m_data.clear();
        m_rows = 0;
        m_cols = 0;
        return;
    }
    m_data.pop_back();
    m_rows--;
}

template<typename T>
void SlidingMatrix<T>::addColumnTail () {
    if (m_rows == 0) {
        m_rows = 1;
        m_cols = 1;
        m_data.push_back(std::vector<T>(1, T{}));
        return;
    }
    for (int r = 0; r < m_rows; r++) {
        m_data[r].push_back(T{});
    }
    m_cols++;
}

template<typename T>
void SlidingMatrix<T>::delColumnTail () {
    if (m_cols == 0) return;
    if (m_cols == 1) {
        m_data.clear();
        m_rows = 0;
        m_cols = 0;
        return;
    }
    for (int r = 0; r < m_rows; r++) {
        m_data[r].pop_back();
    }
    m_cols--;
}

template<typename T>
void SlidingMatrix<T>::addRowHead () {
    if (m_cols == 0) {
        m_cols = 1;
    }
    m_data.insert(m_data.begin(), std::vector<T>(m_cols, T{}));
    m_rows++;
}

template<typename T>
void SlidingMatrix<T>::delRowHead () {
    if (m_rows == 0) return;
    if (m_rows == 1) {
        m_data.clear();
        m_rows = 0;
        m_cols = 0;
        return;
    }
    m_data.erase(m_data.begin());
    m_rows--;
}

template<typename T>
void SlidingMatrix<T>::addColumnHead () {
    if (m_rows == 0) {
        m_rows = 1;
        m_cols = 1;
        m_data.push_back(std::vector<T>(1, T{}));
        return;
    }
    for (int r = 0; r < m_rows; r++) {
        m_data[r].insert(m_data[r].begin(), T{});
    }
    m_cols++;
}

template<typename T>
void SlidingMatrix<T>::delColumnHead () {
    if (m_cols == 0) return;
    if (m_cols == 1) {
        m_data.clear();
        m_rows = 0;
        m_cols = 0;
        return;
    }
    for (int r = 0; r < m_rows; r++) {
        m_data[r].erase(m_data[r].begin());
    }
    m_cols--;
}

template<typename T>
T SlidingMatrix<T>::get (int row, int col) const {
    if (row < 0 || row >= m_rows || col < 0 || col >= m_cols) {
        return T{};
    }
    return m_data[row][col];
}

template<typename T>
void SlidingMatrix<T>::set (int row, int col, T value) {
    if (row < 0 || row >= m_rows || col < 0 || col >= m_cols) {
        return;
    }
    m_data[row][col] = value;
}

template<typename T>
int SlidingMatrix<T>::getRows () const {
    return m_rows;
}

template<typename T>
int SlidingMatrix<T>::getCols () const {
    return m_cols;
}

//================================================================================================================================
//=> - End of implementation -
//================================================================================================================================

#endif

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
