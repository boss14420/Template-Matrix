/*
 * =====================================================================================
 *
 *       Filename:  matrix.cpp
 *
 *    Description:  class matrix
 *
 *        Version:  1.0
 *        Created:  11/08/2011 12:01:25 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  BOSS14420 (boss14420), boss14420@gmail.com
 *        Company:  
 *
 * =====================================================================================
 */

#ifndef __MATRIX_CPP__
#define __MATRIX_CPP__

#include "matrix.h"
#include <algorithm>
#include <iterator>
#include <boost/thread/thread.hpp>
#include <functional>

#ifdef DEBUG
    #include<iostream>
    #include<ctime>
#endif

template <class elem>
unsigned Matrix<elem>::threads = 2;

template <class elem>
unsigned Matrix<elem>::strassenLowerBound = 128;


template <class elem>
std::istream& operator>> (std::istream& is, Matrix<elem>& mt) {
    if(mt.rows && mt.cols) {
        (mt.data).clear();
    }
    
    is >> mt.rows >> mt.cols;

    (mt.data).assign(mt.rows, std::vector<elem>(mt.cols,0));

    for(unsigned i = 0; i < mt.rows; ++i) {
        for(unsigned j = 0; j < mt.cols; ++j)
            is >> (mt.data)[i][j];
    }

    return is;
}

template <class elem>
std::ifstream& Matrix<elem>::readFromFile(std::ifstream& ifs) {
    if(rows && cols) {
        (data).clear();
    }
    
    ifs.read((char*)&rows, sizeof(unsigned))
        .read((char*)&cols, sizeof(unsigned));

    (data).assign(rows, std::vector<elem>(cols,0));

    for(unsigned i = 0; i < rows; ++i) {
        for(unsigned j = 0; j < cols; ++j)
            ifs.read((char*)&(data[i][j]), sizeof(elem));
    }

    return ifs;
}

template<class elem>
std::ostream& operator<< (std::ostream& os, const Matrix<elem>& mt) {
    if(mt.cols && mt.rows) {
        os << mt.rows << " " <<
            mt.cols << std::endl;

        std::ostream_iterator<elem> oi(os, " ");
        for(unsigned i = 0; i < mt.rows; ++i) {
            copy((mt.data)[i].begin(), (mt.data)[i].end(), oi);
            os << std::endl;
        }
    }
    return os;
}

template<class elem>
std::ofstream& Matrix<elem>::writeToFile(std::ofstream& ofs) const {
    if(cols && rows) {

        ofs.write((char*)&rows, sizeof(unsigned))
            .write((char*)&cols, sizeof(unsigned));

        for(unsigned i = 0; i < rows; ++i) {
            for(unsigned j = 0; j < cols; ++j) {
                ofs.write((char*)&data[i][j], sizeof(elem));
            }
        }
    }
    return ofs;
}

template<class elem>
Matrix<elem>& Matrix<elem>::operator*= (const Matrix<elem> &mt) {
    if(cols == mt.rows) {
        unsigned m = rows, n = cols, p = mt.cols;
        unsigned max = (m >= n) ? m : n;
        max = (max >= p) ? max : p;

        mType res;
        unsigned newSize;

        if(max >= 128) { // Use Straussen's algorithm
            unsigned exponent = 1;
            while(128<<(exponent+1) < max) ++exponent;

            unsigned bound = 128;
            while((bound << exponent) < max ) ++bound;

            newSize = bound << exponent;
        } else {
            for(newSize = 1;newSize < max; newSize<<=1);
        }

        mType tmp1(newSize, std::vector<elem>(newSize, 0));
        mType tmp2(newSize, std::vector<elem>(newSize, 0));
        res.assign(newSize, std::vector<elem>(newSize, 0));

        for(unsigned i = 0; i < m; ++i)
            for(unsigned j = 0; j < n; ++j)
                tmp1[i][j] = data[i][j];

        for(unsigned i = 0; i < n; ++i)
            for(unsigned j = 0; j < p; ++j)
                tmp2[i][j] = mt.data[i][j];

        subMatrix mt1 = {&tmp1, newSize, 0, 0, false, false};
        subMatrix mt2 = {&tmp2, newSize, 0, 0, false, false };
        subMatrix mt3 = {&res, newSize, 0, 0, false, false};

#ifndef DEBUG
        Strassen(&mt1, &mt2, &mt3, m, n, p);
#else
        unsigned count = Strassen(&mt1, &mt2, &mt3, m, n, p);
        std::cout << "Đã thực hiện " << count << " phép nhân\n";
        if(newSize < 9)
            mt3.print();
#endif

        res.resize(m);
        for(unsigned i = 0; i < m; ++i)
            res[i].resize(p);

//        } else { // Use normal algorithm
//            res.assign(m, std::vector<elem>(p,0));
//            
//            subMatrix mt1 = {&tmp1, newSize, 0, 0, false, false};
//            subMatrix mt2 = {&tmp2, newSize, 0, 0, false, false };
//            subMatrix mt3 = {&res, newSize, 0, 0, false, false};
//
//            NormalMatrixMultiply(&mt1, &mt1, &mt3, m, n, p);
//        }

        data = res;
        cols = p;

        return *this;
    }

    return *this;
}

template<class elem>
void Matrix<elem>::NormalMatrixMultiply (const typename Matrix<elem>::subMatrix* pMt1,
    const typename Matrix<elem>::subMatrix* pMt2,
    typename Matrix<elem>::subMatrix* pMt3,
    unsigned m, unsigned n, unsigned p) 
{

        std::vector<boost::shared_ptr<boost::thread> > th(threads);

        unsigned slice = m / threads;

        unsigned r1 = pMt1->startRow;
        unsigned r2 = pMt2->startRow;
        unsigned rr = pMt3->startRow;
        unsigned c1 = pMt1->startCol;
        unsigned c2 = pMt2->startCol;
        unsigned cr = pMt3->startCol;

        mType &mt1 = *(pMt1->data);
        mType &mt2 = *(pMt2->data);
        mType &res = *(pMt3->data);

        // Transpose Matrix 2

        mType mt2_transposed(p,std::vector<elem>(n,0));

        for(unsigned i = 0; i < n; ++i)
            for(unsigned j = 0; j < p; ++j)
                mt2_transposed[j][i] = mt2[i+r2][j+c2];
    
        std::function<void(int,int)> func;
        
        func = [&](unsigned m1, unsigned m2) -> void {
            for(unsigned i = m1; i < m2; ++i) {
                for(unsigned j = 0; j < p; ++j) {
                    elem temp = 0;
                    for(unsigned k = 0; k < n; ++k)
                        temp += mt1[i+r1][k+c1] * mt2_transposed[j][k];
                    res[i+rr][j+cr] = temp;
                }
            }
       };



/*         elem v;
 *         auto func = [&](unsigned m1, unsigned m2) -> void {
 *             unsigned kmax = p & ~15;
 *             for(unsigned i = m1; i < m2; ++i) {
 *                 for(unsigned j = 0; j < n; ++j) {
 *                     v = mt1[i+r1][j+c1];
 *                     for(unsigned k = cr; k < kmax;k+=16) {
 * //                        res[i+rr][k] += v*mt2[j+r2][++++k];            
 *                         res[i+rr][k] += v*mt2[j+r2][k];
 *                         res[i+rr][k+1] += v*mt2[j+r2][k+1];
 *                         res[i+rr][k+2] += v*mt2[j+r2][k+2];
 *                         res[i+rr][k+3] += v*mt2[j+r2][k+3];
 *                         res[i+rr][k+4] += v*mt2[j+r2][k+4];
 *                         res[i+rr][k+5] += v*mt2[j+r2][k+5];
 *                         res[i+rr][k+6] += v*mt2[j+r2][k+6];
 *                         res[i+rr][k+7] += v*mt2[j+r2][k+7];
 *                         res[i+rr][k+8] += v*mt2[j+r2][k+8];
 *                         res[i+rr][k+9] += v*mt2[j+r2][k+9];
 *                         res[i+rr][k+10] += v*mt2[j+r2][k+10];
 *                         res[i+rr][k+11] += v*mt2[j+r2][k+11];
 *                         res[i+rr][k+12] += v*mt2[j+r2][k+12];
 *                         res[i+rr][k+13] += v*mt2[j+r2][k+13];
 *                         res[i+rr][k+14] += v*mt2[j+r2][k+14];
 *                         res[i+rr][k+15] += v*mt2[j+r2][k+15];
 *                     }
 *                     for(unsigned k = kmax; k < p; ++k) {
 *                         res[i+rr][k] += v*mt2[j+r2][k];
 *                     }
 *                 }
 *             }
 *         };
 */


/*         elem temp;
 *         auto func = [&](unsigned m1, unsigned m2) -> void {
 *             for(unsigned i = m1; i < m2; ++i) {
 *                 for(unsigned j = 0; j < p; ++j) {
 * //                    v = mt1[i+r1][j+c1];
 *                     temp = 0;
 *                     for(unsigned k = 0; k < n; ++k)
 *                         temp += mt1[i+r1][k+c1] * mt2[k+r2][j+c2];
 * //                        res[i+rr][k+cr] += v*mt2[j+r2][k+c2];
 *                     res[i+rr][j+cr] = temp;
 *                 }
 *             }
 *         };
 */


        for(unsigned id = 0; id < threads; ++id) {
            unsigned startSlice = id*slice;
            unsigned endSlice = (id+1)*slice;
            if(id == threads -1)
                endSlice = m;
            th[id] = boost::shared_ptr<boost::thread> 
                (new boost::thread(func, startSlice, endSlice));
        }

        for(unsigned id =0; id < threads; ++id) {
            th[id]->join();
        }
}

template<class elem>
Matrix<elem>& Matrix<elem>::operator+= (const Matrix<elem> &mt) {
    if(rows == mt.rows && cols == mt.cols) {
        for(unsigned i = 0; i < rows; ++i)
            for(unsigned j = 0; j < cols; ++j)
                data[i][j] += mt.data[i][j];
    }

    return *this;
}

template<class elem>
Matrix<elem>& Matrix<elem>::operator-= (const Matrix<elem> &mt) {
    if(rows == mt.rows && cols == mt.cols) {
        for(unsigned i = 0; i < rows; ++i)
            for(unsigned j = 0; j < cols; ++j)
                data[i][j] -= mt.data[i][j];
    }

    return *this;
}

#ifndef __GXX_EXPERIMENTAL_CXX0X__
template<class elem>
void Matrix<elem>::MultiplyByCol(const mType* pMt1,
        const mType* pMt2, mType *pRes, 
        unsigned p1, unsigned p2, unsigned m, unsigned n) {
    
    const mType &mt1 = *pMt1;
    const mType &mt2 = *pMt2;
    mType &res = *pRes;

    for(unsigned i = 0; i < m; ++i) {
        for(unsigned j = p1; j < p2; ++j) {
            elem temp = 0;
            for(unsigned k = 0; k < n; ++k)
                temp += mt1[i][k] * mt2[k][j];
            res[i][j] = temp;
        }
    }
}
#endif

#ifndef __GXX_EXPERIMENTAL_CXX0X__
template <class elem>
void Matrix<elem>::MultiplyByRow(const mType* pMt1,
        const mType* pMt2, mType *pRes, 
        unsigned m1, unsigned m2, unsigned n, unsigned p) {

    const mType &mt1 = *pMt1;
    const mType &mt2 = *pMt2;
    mType &res = *pRes;

    for(unsigned i = m1; i < m2; ++i) {
        for(unsigned j = 0; j < p; ++j) {
            elem temp = 0;
            for(unsigned k = 0; k < n; ++k)
                temp += mt1[i][k] * mt2[k][j];
            res[i][j] = temp;
        }
    }
}
#endif

template<class elem>
void Matrix<elem>::subMatrixCopy(const typename Matrix<elem>::subMatrix* pMt1,
    typename Matrix<elem>::subMatrix* pMt2) {    
        
        unsigned size = pMt1->size;
        mType &mt1 = *(pMt1->data);
        mType &mt2 = *(pMt2->data);

        for(unsigned i = 0; i < size; ++i)
            for(unsigned j = 0; j < size; ++j)
                mt2[i][j] = mt1[i][j];
}


    /*
     *--------------------------------------------------------------------------------------
     *       Class:  Matrix<mType>
     *      Method:  Matrix<mType> :: subMatrixAdd
     * Description:  add to a matrix Sum of two other matrixes
     *--------------------------------------------------------------------------------------
     */
template<class elem>
void Matrix<elem>::subMatrixAdd(const typename Matrix<elem>::subMatrix* pMt1,
    const typename Matrix<elem>::subMatrix* pMt2,
    typename Matrix<elem>::subMatrix* pMt3) {    

    if(!(pMt1->isZero && pMt2->isZero)) {
        unsigned size = pMt1->size;
        unsigned r1 = pMt1->startRow;
        unsigned r2 = pMt2->startRow;
        unsigned rr = pMt3->startRow;
        unsigned c1 = pMt1->startCol;
        unsigned c2 = pMt2->startCol;
        unsigned cr = pMt3->startCol;

        mType &mt1 = *(pMt1->data);
        mType &mt2 = *(pMt2->data);
        mType &res = *(pMt3->data);

        for(unsigned i = 0; i < size; ++i) {
            for(unsigned j = 0; j < size; ++j)
                res[rr+i][cr+j] += mt1[r1+i][c1+j] + mt2[r2+i][c2+j];
        }
    } 
}

    /*
     *--------------------------------------------------------------------------------------
     *       Class:  Matrix<mType>
     *      Method:  Matrix<mType> :: subMatrixSubtract
     * Description:  add to a matrix difference of two other matrixes
     *--------------------------------------------------------------------------------------
     */
template<class elem>
void Matrix<elem>::subMatrixSubtract(const typename Matrix<elem>::subMatrix* pMt1,
    const typename Matrix<elem>::subMatrix* pMt2,
    typename Matrix<elem>::subMatrix* pMt3) {

    unsigned size = pMt1->size;
    unsigned r1 = pMt1->startRow;
    unsigned r2 = pMt2->startRow;
    unsigned rr = pMt3->startRow;
    unsigned c1 = pMt1->startCol;
    unsigned c2 = pMt2->startCol;
    unsigned cr = pMt3->startCol;

    mType &mt1 = *(pMt1->data);
    mType &mt2 = *(pMt2->data);
    mType &res = *(pMt3->data);

    if(!(pMt2->isZero && pMt1->isZero)) {
        for(unsigned i = 0; i < size; ++i) {
            for(unsigned j = 0; j < size; ++j)
                res[rr+i][cr+j] += mt1[r1+i][c1+j] - mt2[r2+i][c2+j];
        }
    } 
}


template <class elem>
unsigned Matrix<elem>::Strassen(const typename Matrix<elem>::subMatrix* pMt1,
        const typename Matrix<elem>::subMatrix* pMt2,
        typename Matrix<elem>::subMatrix* pMt3, 
        unsigned tM, unsigned tN, unsigned tP) {

#ifdef DEBUG
    static unsigned count = 0;
#endif
    /* if Matrix A or B is zero then C is zero */
    if(pMt1->isZero || pMt2->isZero)
        return count;
    else {
        unsigned size = pMt1->size;
        unsigned r1 = pMt1->startRow;
        unsigned c1 = pMt1->startCol;
        unsigned r2 = pMt2->startRow;
        unsigned c2 = pMt2->startCol;
        unsigned rr = pMt3->startRow;
        unsigned cr = pMt3->startCol;
        mType &mt1 = *(pMt1->data);
        mType &mt2 = *(pMt2->data);
        mType &res = *(pMt3->data);

        if(0 //(tM >= strassenLowerBound && (tM >>3) >= size)
                || strassenLowerBound >= size) {
            NormalMatrixMultiply(pMt1, pMt2, pMt3, size, size, size);
            return count += size*size*size;

        } else {
            unsigned newSize = size >> 1;
            subMatrix a11 = {&mt1, newSize, r1, c1, false, false};
/*             subMatrix a12 = {&mt1, newSize, r1, c1+newSize, false, false};
 *             subMatrix a21 = {&mt1, newSize, r1+newSize, c1, false, false};
 *             subMatrix a22 = {&mt1, newSize, r1+newSize, c1+newSize, false, false};
 */

            subMatrix a12 = {&mt1, newSize, r1, c1+newSize, c1+newSize >= tN, false};
            subMatrix a21 = {&mt1, newSize, r1+newSize, c1, r1+newSize >= tM, false};
            subMatrix a22 = {&mt1, newSize, r1+newSize, c1+newSize, a12.isZero || a21.isZero, false};



            subMatrix b11 = {&mt2, newSize, r2, c2, false, false};
/*             subMatrix b12 = {&mt2, newSize, r2, c2+newSize, false, false};
 *             subMatrix b21 = {&mt2, newSize, r2+newSize, c2, false, false};
 *             subMatrix b22 = {&mt2, newSize, r2+newSize, c2+newSize, false, false};
 */

            subMatrix b12 = {&mt2, newSize, r2, c2+newSize, c2+newSize >= tP, false};
            subMatrix b21 = {&mt2, newSize, r2+newSize, c2, r2+newSize >= tN, false};
            subMatrix b22 = {&mt2, newSize, r2+newSize, c2+newSize, b12.isZero || b21.isZero, false};



            subMatrix c11 = {&res, newSize, rr, cr, false, false};
            subMatrix c12 = {&res, newSize, rr, cr+newSize, false, false};
            subMatrix c21 = {&res, newSize, rr+newSize, cr, false, false};
            subMatrix c22 = {&res, newSize, rr+newSize, cr+newSize, false, false};

            subMatrix m[7];

            for(unsigned i = 0; i < 7; ++i)
                m[i].setSize(newSize);


#ifdef __THREADED_STRASSEN_FUNCTION__
            auto calcM1 = [&]() -> void {
                // Calculate M1 = (A11 + A22)(B11 + B22)
#endif

                subMatrix tmp1, tmp2;
                tmp1.setSize(newSize);
                tmp2.setSize(newSize);

                Matrix<elem>::subMatrixAdd(&a11, &a22, &tmp1);
                Matrix<elem>::subMatrixAdd(&b11, &b22, &tmp2);
                Matrix<elem>::Strassen(&tmp1, &tmp2, &m[0], tM, tN, tP);

                
#if (defined __THREADED_STRASSEN_FUNCTION__ && defined __7_THREAD_STRASSEN_FUNCTION__)
            };
            
            auto calcM2 = [&]() -> void {
                // Calculate M2 = (A21 + A22)*B11
                subMatrix tmp1;
                tmp1.setSize(newSize);
#else
                tmp1.reAssign();
#endif
                Matrix<elem>::subMatrixAdd(&a21, &a22, &tmp1);
                Matrix<elem>::Strassen(&tmp1, &b11, &m[1], tM, tN, tP);

#if (defined __THREADED_STRASSEN_FUNCTION__ && defined __7_THREAD_STRASSEN_FUNCTION__)
            };
            
            auto calcM3 = [&]() -> void {
                // Calculate M3 = A11*(B12 - B22)
                subMatrix tmp1;
                tmp1.setSize(newSize);
#else
                tmp1.reAssign();
#endif
                Matrix<elem>::subMatrixSubtract(&b12, &b22, &tmp1);
                Matrix<elem>::Strassen(&a11, &tmp1, &m[2], tM, tN, tP);


#if (defined __THREADED_STRASSEN_FUNCTION__ && defined __7_THREAD_STRASSEN_FUNCTION__)
            };
            
            auto calcM4 = [&]() -> void {
                // Calculate M4 = A22*(B21 - B11)
                subMatrix tmp1;
                tmp1.setSize(newSize);
#else
                tmp1.reAssign();
#endif
                Matrix<elem>::subMatrixSubtract(&b21, &b11, &tmp1);
                Matrix<elem>::Strassen(&a22, &tmp1, &m[3], tM, tN, tP);

#ifdef __THREADED_STRASSEN_FUNCTION__
            };
            
            auto calcM5 = [&]() -> void {
                // Calculate M5 = (A11 + A12)*B22
                subMatrix tmp1, tmp2;
                tmp1.setSize(newSize);
                tmp2.setSize(newSize);
#else
                tmp1.reAssign();
#endif
                Matrix<elem>::subMatrixAdd(&a11, &a12, &tmp1);
                Matrix<elem>::Strassen(&tmp1, &b22, &m[4], tM, tN, tP);


#if (defined __THREADED_STRASSEN_FUNCTION__ && defined __7_THREAD_STRASSEN_FUNCTION__)
            };
            
            auto calcM6 = [&]() -> void {
                // Calculate M6 = (A21 - A11)*(B11 + B12)
                subMatrix tmp1;
                tmp1.setSize(newSize);
#else
                tmp1.reAssign();
                tmp2.reAssign();
#endif
                Matrix<elem>::subMatrixSubtract(&a21, &a11, &tmp1);
                Matrix<elem>::subMatrixAdd(&b11, &b12, &tmp2);
                Matrix<elem>::Strassen(&tmp1, &tmp2, &m[5], tM, tN, tP);


#if (defined __THREADED_STRASSEN_FUNCTION__ && defined __7_THREAD_STRASSEN_FUNCTION__)
            };
            
            auto calcM7 = [&]() -> void {
                // Calculate M7 = (A12 - A22)*(B21 + B22)
                subMatrix tmp1, tmp2;
                tmp1.setSize ( newSize);
                tmp2.setSize (newSize);
#else
                tmp1.reAssign();
                tmp2.reAssign();
#endif
                Matrix<elem>::subMatrixSubtract(&a12, &a22, &tmp1);
                Matrix<elem>::subMatrixAdd(&b21, &b22, &tmp2);
                Matrix<elem>::Strassen(&tmp1, &tmp2, &m[6], tM, tN, tP);

#ifdef __THREADED_STRASSEN_FUNCTION__
            };

            auto calcMatrixC = [&]() -> void {
#endif

                /* Calculate    C11 = M1 + M4 - M5 + M7
                 *              C12 = M3 + M5
                 *              C21 = M2 + M4
                 *              C22 = M1 - M2 + M3 + M6
                 */

                Matrix<elem>::subMatrixAdd(&m[0], &m[6], &c11);
                Matrix<elem>::subMatrixSubtract(&m[3], &m[4], &c11);

                Matrix<elem>::subMatrixAdd(&m[2], &m[4], &c12);  

                Matrix<elem>::subMatrixAdd(&m[1], &m[3], &c21);

                Matrix<elem>::subMatrixSubtract(&m[0], &m[1], &c22);
                Matrix<elem>::subMatrixAdd(&m[2], &m[5], &c22);

#ifdef __THREADED_STRASSEN_FUNCTION__ 
            };

            if(count) {
                calcM1();
#ifdef __7_THREAD_STRASSEN_FUNCTION__
                calcM2();
                calcM3();
                calcM4();
#endif
                calcM5();
#ifdef __7_THREAD_STRASSEN_FUNCTION__
                calcM6();
                calcM7();
#endif
            } else {
                boost::thread th1(calcM1);
#ifdef __7_THREAD_STRASSEN_FUNCTION__
                boost::thread th2(calcM2);
                boost::thread th3(calcM3);
                boost::thread th4(calcM4);
#endif
                boost::thread th5(calcM5);
#ifdef __7_THREAD_STRASSEN_FUNCTION__
                boost::thread th6(calcM6);
                boost::thread th7(calcM7);
#endif

                th1.join();
#ifdef __7_THREAD_STRASSEN_FUNCTION__
                th2.join();
                th3.join();
                th4.join();
#endif
                th5.join();
#ifdef __7_THREAD_STRASSEN_FUNCTION__
                th6.join();
                th7.join();
#endif
            }
            calcMatrixC();
#endif

            return count;
        }
    }
}

#endif
