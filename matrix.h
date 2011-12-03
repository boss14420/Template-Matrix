/*
 * =====================================================================================
 *
 *       Filename:  matrix.h
 *
 *    Description:  matrix class's header
 *
 *        Version:  1.0
 *        Created:  11/08/2011 12:01:31 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  BOSS14420 (boss14420), boss14420@gmail.com
 *        Company:  
 *
 * =====================================================================================
 */

#ifndef __MATRIX_H__
#define __MATRIX_H__

#include <vector>
#include <iostream>
#include <fstream>
#include <exception>
#include <iterator>
//#include <algorithm>
//#include <iterator>
//#include <boost/thread/thread.hpp>

//#define elem int

//class MatrixMultiplyThread;

template<class elem> class Matrix;

template<class elem>
std::ostream& operator<< (std::ostream& os, const Matrix<elem>& mt);

template<class elem>
std::istream& operator>> (std::istream& is, Matrix<elem>& mt);

//template<class elem>
//std::ofstream& operator<< (std::ofstream& ofs, const Matrix<elem>& mt);

//template<class elem>
//std::ifstream& operator>> (std::ifstream& ifs, Matrix<elem>& mt);


template<class elem> 
class Matrix {

    typedef std::vector<std::vector<elem> > mType;

    mType data;
    unsigned rows, cols;

    static void NormalMatrixMultiply (const typename Matrix<elem>::subMatrix*,
        const typename Matrix<elem>::subMatrix*,
        typename Matrix<elem>::subMatrix*,
        unsigned, unsigned, unsigned);
#ifndef __GXX_EXPERIMENTAL_CXX0X__
    static void MultiplyByCol(const mType*, const mType*,
            mType*, unsigned, unsigned, unsigned, unsigned);

    static void MultiplyByRow(const mType*, const mType*,
            mType*, unsigned, unsigned, unsigned, unsigned);
#endif

    struct subMatrix {
        mType* data;
        unsigned size;
        unsigned startRow;
        unsigned startCol;
        bool isZero;
        bool isTemp;

        void setSize(unsigned s) {
            size = s;
            startCol = startRow = 0;
            isZero = false;
            isTemp = true;

            data = new mType(s, std::vector<elem>(s,0));
        }

        void reAssign() {
            if(data)
                data->assign(size, std::vector<elem>(size,0));
            else
                data = new mType(size, std::vector<elem>(size,0));
        }

        ~subMatrix() {
            if(isTemp)
                delete data;
        }

        void print() {
            std::cout << size << " x " << size << std::endl;
            for(unsigned i = 0; i < size; ++i) {
                std::copy((*data)[startRow+i].begin()+startCol,
                        (*data)[startRow+i].begin()+startCol+size,
                        std::ostream_iterator<elem>(std::cout, " "));
                std::cout << std::endl;
            }
            std::cout << std::endl;
        }
    };

    static unsigned Strassen(const typename Matrix<elem>::subMatrix*,
        const typename Matrix<elem>::subMatrix*,
        typename Matrix<elem>::subMatrix*, 
        unsigned, unsigned, unsigned);

    static void subMatrixCopy(const typename Matrix<elem>::subMatrix*,
            typename Matrix<elem>::subMatrix*);

    /*
     *--------------------------------------------------------------------------------------
     *       Class:  Matrix<mType>
     *      Method:  Matrix<mType> :: subMatrixAdd
     * Description:  add to a matrix Sum of two other matrixes
     *--------------------------------------------------------------------------------------
     */
    static void subMatrixAdd(const typename Matrix<elem>::subMatrix*,
        const typename Matrix<elem>::subMatrix*,
        typename Matrix<elem>::subMatrix*);

    /*
     *--------------------------------------------------------------------------------------
     *       Class:  Matrix<mType>
     *      Method:  Matrix<mType> :: subMatrixSubtract
     * Description:  add to a matrix difference of two other matrixes
     *--------------------------------------------------------------------------------------
     */
    static void subMatrixSubtract(const typename Matrix<elem>::subMatrix*,
        const typename Matrix<elem>::subMatrix*,
        typename Matrix<elem>::subMatrix*);


public:
    Matrix(/* unsigned r = 0, unsigned c = 0 */) : rows(0), cols(0) {
/*         if( r && c)
 *             data.assign(r, std::vector<mType>(c, 0));
 */
    }

    Matrix(elem **a, unsigned r, unsigned c) : rows(r), cols(c) {
        data.assign(r,std::vector<elem>(c,0));
        for(unsigned i = 0; i < r; ++i) {
            std::copy(a[i], a[i]+c, data[i].begin());
        }
    }
    
    Matrix(const mType &dt) {
        rows = dt.size();
        cols = dt[0].size();

        data = dt;
    }

    Matrix(const mType &dt, unsigned r1, unsigned r2,
            unsigned c1, unsigned c2) {
        rows = r2 - r2;
        cols = c2 - c1;

        data.reserve(rows);
        for(unsigned i = 0; i < rows; ++i) {
            data[i].reserve(cols);
            copy(dt[r1+i].begin(), dt[r1+i].begin() + cols, data[i].begin());
        }
    }

    ~Matrix() {
//        for(unsigned i = 0; i < rows; ++i) {
//            data[i].clear();
//        }
//        data.clear();
    }
    
    friend std::ostream& operator<< <> (std::ostream&, const Matrix<elem>&);
    friend std::istream& operator>> <> (std::istream&, Matrix<elem>&);
    std::ofstream& writeToFile(std::ofstream&) const;
    std::ifstream& readFromFile(std::ifstream&);

//    class MatrixMultiNot
    Matrix<elem>& operator*=(const Matrix<elem>& mt);
    Matrix<elem>& operator+=(const Matrix<elem>& mt);
    Matrix<elem>& operator-=(const Matrix<elem>& mt);

    const Matrix<elem> operator*(const Matrix<elem>& mt) {
        return Matrix<elem>(*this)*=mt;
    }

    unsigned static threads;
    unsigned static strassenLowerBound;
};

#include "matrix.cpp"

#endif
