/**
 *
 * @file
 *
 *  PLASMA is a software package provided by:
 *  University of Tennessee, US,
 *  University of Manchester, UK.
 *
 * @generated from test/test_zgemm.c, normal z -> d, Thu Aug  8 10:23:26 2019
 *
 **/
#include "test.h"
#include "flops.h"
#include "plasma.h"
#include "core_lapack.h"

#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <omp.h>

#define REAL

/***************************************************************************//**
 *
 * @brief Tests DGEMM.
 *
 * @param[in,out] param - array of parameters
 * @param[in]     run - whether to run test
 *
 * Sets used flags in param indicating parameters that are used.
 * If run is true, also runs test and stores output parameters.
 ******************************************************************************/
void test_dgemm_blasfeo(param_value_t param[], bool run)
{
//omp_set_num_threads(2);
// omp_set_num_threads(4);
//printf("\nnum threads %d\n", omp_get_num_threads());
// printf("\nnum threads %d\n", omp_get_max_threads());

	// our stuff
	// int m0 = 20; // 11;
	// int n0 = 20; // 11;
	// int k0 = 20; // 11;

	// int nb0 = 8;




    //================================================================
    // Mark which parameters are used.
    //================================================================
    param[PARAM_TRANSA ].used = true;
    param[PARAM_TRANSB ].used = true;
    param[PARAM_DIM    ].used = PARAM_USE_M | PARAM_USE_N | PARAM_USE_K;
    param[PARAM_ALPHA  ].used = true;
    param[PARAM_BETA   ].used = true;
    param[PARAM_PADA   ].used = true;
    param[PARAM_PADB   ].used = true;
    param[PARAM_PADC   ].used = true;
    param[PARAM_NB     ].used = true;
    if (! run)
        return;

    //================================================================
    // Set parameters.
    //================================================================
    plasma_enum_t transa = plasma_trans_const(param[PARAM_TRANSA].c);
    plasma_enum_t transb = plasma_trans_const(param[PARAM_TRANSB].c);

#if 1
    int m = param[PARAM_DIM].dim.m;
    int n = param[PARAM_DIM].dim.n;
    int k = param[PARAM_DIM].dim.k;
#else
	int m = m0;
	int n = n0;
	int k = k0;
#endif

    int Am, An;
    int Bm, Bn;
    int Cm, Cn;

    if (transa == PlasmaNoTrans) {
        Am = m;
        An = k;
    }
    else {
        Am = k;
        An = m;
    }
    if (transb == PlasmaNoTrans) {
        Bm = k;
        Bn = n;
    }
    else {
        Bm = n;
        Bn = k;
    }
    Cm = m;
    Cn = n;

    int lda = imax(1, Am + param[PARAM_PADA].i);
    int ldb = imax(1, Bm + param[PARAM_PADB].i);
    int ldc = imax(1, Cm + param[PARAM_PADC].i);

    int test = param[PARAM_TEST].c == 'y';
    double eps = LAPACKE_dlamch('E');

#ifdef COMPLEX
    double alpha = param[PARAM_ALPHA].z;
    double beta  = param[PARAM_BETA].z;
#else
    double alpha = creal(param[PARAM_ALPHA].z);
    double beta  = creal(param[PARAM_BETA].z);
#endif

    //================================================================
    // Set tuning parameters.
    //================================================================
    plasma_set(PlasmaTuning, PlasmaDisabled);
    plasma_set(PlasmaNb, param[PARAM_NB].i);
    // plasma_set(PlasmaNb, nb0);

    //================================================================
    // Allocate and initialize arrays.
    //================================================================
    double *A =
        (double*)malloc((size_t)lda*An*sizeof(double));
    assert(A != NULL);

    double *B =
        (double*)malloc((size_t)ldb*Bn*sizeof(double));
    assert(B != NULL);

    double *C =
        (double*)malloc((size_t)ldc*Cn*sizeof(double));
    assert(C != NULL);

    int seed[] = {0, 0, 0, 1};
    lapack_int retval;
    retval = LAPACKE_dlarnv(1, seed, (size_t)lda*An, A);
    assert(retval == 0);

    retval = LAPACKE_dlarnv(1, seed, (size_t)ldb*Bn, B);
    assert(retval == 0);

    retval = LAPACKE_dlarnv(1, seed, (size_t)ldc*Cn, C);
    assert(retval == 0);

    double *Cref = NULL;
    if (test) {
        Cref = (double*)malloc(
            (size_t)ldc*Cn*sizeof(double));
        assert(Cref != NULL);

        memcpy(Cref, C, (size_t)ldc*Cn*sizeof(double));
    }

    //================================================================
    // Run and time PLASMA.
    //================================================================
    plasma_time_t start = omp_get_wtime();

    plasma_dgemm_blasfeo(
//    plasma_dgemm(
        transa, transb,
        m, n, k,
        alpha, A, lda,
               B, ldb,
         beta, C, ldc);

    plasma_time_t stop = omp_get_wtime();
    plasma_time_t time = stop-start;

    param[PARAM_TIME].d = time;
    param[PARAM_GFLOPS].d = flops_dgemm(m, n, k) / time / 1e9;

    //================================================================
    // Test results by comparing to a reference implementation.
    //================================================================
    if (test) {
        // |R - R_ref|_p < gamma_{k+2} * |alpha| * |A|_p * |B|_p +
        //                 gamma_2 * |beta| * |C|_p
        // holds component-wise or with |.|_p as 1, inf, or Frobenius norm.
        // gamma_k = k*eps / (1 - k*eps), but we use
        // gamma_k = sqrt(k)*eps as a statistical average case.
        // Using 3*eps covers complex arithmetic.
        // See Higham, Accuracy and Stability of Numerical Algorithms, ch 2-3.
        double work[1];
        double Anorm = LAPACKE_dlange_work(
                           LAPACK_COL_MAJOR, 'F', Am, An, A,    lda, work);
        double Bnorm = LAPACKE_dlange_work(
                           LAPACK_COL_MAJOR, 'F', Bm, Bn, B,    ldb, work);
        double Cnorm = LAPACKE_dlange_work(
                           LAPACK_COL_MAJOR, 'F', Cm, Cn, Cref, ldc, work);

        cblas_dgemm(
            CblasColMajor,
            (CBLAS_TRANSPOSE)transa, (CBLAS_TRANSPOSE)transb,
            m, n, k,
            (alpha), A, lda,
                                B, ldb,
             (beta), Cref, ldc);

// d_print_mat(m, n, C, ldc);
// d_print_mat(m, n, Cref, ldc);

        double zmone = -1.0;
        cblas_daxpy((size_t)ldc*Cn, (zmone), Cref, 1, C, 1);

        double error = LAPACKE_dlange_work(
                           LAPACK_COL_MAJOR, 'F', Cm, Cn, C,    ldc, work);
        double normalize = sqrt((double)k+2) * fabs(alpha) * Anorm * Bnorm
                         + 2 * fabs(beta) * Cnorm;
        if (normalize != 0)
            error /= normalize;

        param[PARAM_ERROR].d = error;
        param[PARAM_SUCCESS].i = error < 3*eps;
    }

    //================================================================
    // Free arrays.
    //================================================================
    free(A);
    free(B);
    free(C);
    if (test)
        free(Cref);
}
