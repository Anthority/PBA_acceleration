/*
 * prima.hpp
 *
 *  Created on: Nov 25, 2020
 *      Author: babis
 */

#ifndef HPP_PRIMA_HPP_
#define HPP_PRIMA_HPP_

#include <vector>

#include <ot/eigen3/Eigen/Sparse>
#include <ot/eigen3/Eigen/Dense>
#include <ot/eigen3/Eigen/SparseCholesky>
#include <ot/eigen3/Eigen/Eigenvalues>

using namespace Eigen;

/* types.hpp */
enum _bench_type_
{
    RC = 1,
    RLC = 2
};

typedef Matrix<double, Dynamic, Dynamic> MatrixXd;
typedef Matrix<double, Dynamic, 1> VectorXd;
typedef SparseMatrix<double> SpMat;
// typedef Matrix<, Dynamic, Dynamic> MatrixXcd;
typedef Triplet<double> T;

// typedef struct tfm
// {
//     std::complex<double> *poles;
//     std::complex<double> *res;
// } tf;

/* prima.hpp */
void Prima(MatrixXd &, MatrixXd &, MatrixXd &, MatrixXd &, MatrixXd &, MatrixXd &, MatrixXd &, MatrixXd &, int);

VectorXd spMat_col(SpMat &, int);
void rc_tr_sim_d(MatrixXd &C, MatrixXd &G, MatrixXd &B, MatrixXd &D, MatrixXd &u, double h, std::vector<double> &y,size_t idx);
/* aux.hpp */
void printMatrixXd(MatrixXd &);
void printVectorXd(VectorXd &);
void printSpMat(SpMat &);
void printVectorXdToFile(VectorXd &, std::string);
void printMatrixXdToFile(MatrixXd &, std::string);

#endif /* HPP_PRIMA_HPP_ */
