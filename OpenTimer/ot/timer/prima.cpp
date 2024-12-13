/*
 * Prima.cpp
 *
 *  Created on: Nov 25, 2020
 *      Author: babis
 */

#include <iostream>
#include <fstream>
#include <omp.h>
#include <chrono> // whenever you include chrono library you have to include iostream as well
#include "prima.hpp"

using namespace std;
/* q	: the number of moments */
void Prima(MatrixXd &C_r, MatrixXd &G_r, MatrixXd &B_r, MatrixXd &D_r, MatrixXd &C, MatrixXd &G, MatrixXd &B, MatrixXd &D, int q)
{
	/* !Caution: G has to be positive definite in order to use SimplicialLLT solver.
	 * If it isn't, one option is to use FullPivLU */
	// SparseLU<SpMat> solver;
	// solver.compute(G);

	int N = B.cols();
	int n = G.cols();
	int r = q * N; // the order of the reduced system

	JacobiSVD<MatrixXd> solver(G, ComputeThinU | ComputeThinV);

	/* G*R = B */
	MatrixXd R(n, N);
	for (int j = 0; j < N; j++)
		R.col(j) = solver.solve(B.col(j));

	/* the projection matrix */
	MatrixXd X(n, r);
	MatrixXd Q;

	/* numerically unstable but fast.
	 * When stability is an issue run FullPivHouseholderQR */
	HouseholderQR<MatrixXd> qr(R); // try maybe inplace decomposition (?)
	Q = qr.householderQ();

	X.topLeftCorner(n, N) = Q.topLeftCorner(n, N);

	/* loop until you reach the number of moments */
	for (int q_i = 1; q_i < q; ++q_i) // k = 1, 2, ..., q
	{
		MatrixXd V;
		V = C * X.block(0, (q_i - 1) * N, n, N); // V = C*X(k-1)

		/* Xk = G^(-1)*V */
		for (int j = 0; j < N; j++)
			X.col(q_i * N + j) = solver.solve(V.col(j));

		for (int q_j = 0; q_j < q_i; q_j++) // j = 1, 2, ..., k
		{
			MatrixXd H = X.block(0, (q_i - q_j - 1) * N, n, N).transpose();
			H = H * X.block(0, q_i * N, n, N);																   // H = X(k-j)'*Xk
			X.block(0, q_i * N, n, N) = X.block(0, q_i * N, n, N) - X.block(0, (q_i - q_j - 1) * N, n, N) * H; // Xk = Xk-X(k-j)*H
		}

		HouseholderQR<MatrixXd> qr(X.block(0, q_i * N, n, N));
		Q = qr.householderQ();

		X.block(0, q_i * N, n, N) = Q.topLeftCorner(n, N);
	}

	C_r = X.transpose() * C * X;
	G_r = X.transpose() * G * X;
	B_r = X.transpose() * B;
	D_r = X.transpose() * D;
	// printMatrixXd
}

void rc_tr_sim_d(MatrixXd &C, MatrixXd &G, MatrixXd &B, MatrixXd &D, MatrixXd &u, double h, vector<double>& y,size_t idx)
{
	MatrixXd C_h = C / h;

	MatrixXd A = G + C_h;

	VectorXd v(C.cols());
	v.setZero();

	MatrixXd Dt = D.transpose();

	JacobiSVD<MatrixXd> solver(A, ComputeThinU | ComputeThinV);

	for (int t_i = 0; t_i < u.cols(); ++t_i)
	{
		VectorXd b = (B * u.col(t_i)) + C_h * v;

		v = solver.solve(b);

		y[t_i] = (Dt * v)(idx, 0);
	}
}

/* ************************ aux.cpp ************************** */

/* a routine that returns the i-th col of sparse matrix sm in dense format */
VectorXd spMat_col(SpMat &sm, int col_i)
{
	VectorXd x = VectorXd::Zero(sm.rows(), 1);

	/* loop over the rows of the i-th column of sm */
	for (SpMat::InnerIterator it(sm, col_i); it; ++it)
		x(it.row()) = it.value();

	return x;
}

void printMatrixXd(MatrixXd &X)
{
	for (int row_i = 0; row_i < X.rows(); row_i++)
	{
		cout << endl;
		for (int col_i = 0; col_i < X.cols(); col_i++)
			cout << X(row_i, col_i) << '\t';
	}
	cout << endl
		 << endl;
}

void printVectorXd(VectorXd &X)
{
	for (int row_i = 0; row_i < X.rows(); row_i++)
		cout << X(row_i) << '\t';

	cout << endl
		 << endl;
}

void printSpMat(SpMat &X)
{
	for (int k = 0; k < X.outerSize(); ++k)
		for (SpMat::InnerIterator it(X, k); it; ++it)
			cout << "i = " << it.row() << ", j = " << it.col() << ", x = " << it.value() << endl;

	cout << endl
		 << endl;
}

void printVectorXdToFile(VectorXd &X, string filename)
{
	ofstream myfile;
	myfile.open(filename.c_str());

	for (int row_i = 0; row_i < X.rows(); row_i++)
		myfile << X(row_i) << '\t';

	myfile.close();
}

void printMatrixXdToFile(MatrixXd &X, string filename)
{
	ofstream myfile;
	myfile.open(filename.c_str());

	for (int row_i = 0; row_i < X.rows(); row_i++)
	{
		for (int col_i = 0; col_i < X.cols(); col_i++)
			myfile << X(row_i, col_i) << '\t';

		myfile << endl;
	}

	myfile.close();
}
