/**
 *
 * @file
 *
 *  PLASMA is a software package provided by:
 *  University of Tennessee, US,
 *  University of Manchester, UK.
 *
 * @generated from core_blas/core_zpotrf.c, normal z -> d, Thu Aug  8 17:24:58 2019
 *
 **/

#include <plasma_core_blas.h>
#include "plasma_types.h"
#include "core_lapack.h"
#include "blasfeo_d_aux.h"

/***************************************************************************//**
 *
 * @ingroup core_potrf
 *
 *  Performs the Cholesky factorization of a symmetric positive definite
 *  matrix A. The factorization has the form
 *
 *    \f[ A = L \times L^T, \f]
 *    or
 *    \f[ A = U^T \times U, \f]
 *
 *  where U is an upper triangular matrix and L is a lower triangular matrix.
 *
 *******************************************************************************
 *
 * @param[in] uplo
 *          - PlasmaUpper: Upper triangle of A is stored;
 *          - PlasmaLower: Lower triangle of A is stored.
 *
 * @param[in] n
 *          The order of the matrix A. n >= 0.
 *
 * @param[in,out] A
 *          On entry, the symmetric positive definite matrix A.
 *          If uplo = PlasmaUpper, the leading N-by-N upper triangular part of A
 *          contains the upper triangular part of the matrix A, and the strictly
 *          lower triangular part of A is not referenced.
 *          If uplo = PlasmaLower, the leading N-by-N lower triangular part of A
 *          contains the lower triangular part of the matrix A, and the strictly
 *          upper triangular part of A is not referenced.
 *          On exit, if return value = 0, the factor U or L from the Cholesky
 *          factorization A = U^T*U or A = L*L^T.
 *
 * @param[in] lda
 *          The leading dimension of the array A. lda >= max(1,n).
 *
 ******************************************************************************/
__attribute__((weak))
int plasma_core_dpotrf_blasfeo(plasma_enum_t uplo,
                 int n,
                 struct blasfeo_dmat *sA, int ai, int aj)
{
    // return LAPACKE_dpotrf_work(LAPACK_COL_MAJOR,
    //                            lapack_const(uplo),
    //                            n,
    //                            A, lda);
    fprintf(stderr, "before blasfeo dpotrf ai: %d aj: %d\n", ai, aj);
    blasfeo_dpotrf_l(n, sA, ai, aj, sA, ai, aj);
    return 0;
}

/******************************************************************************/
void plasma_core_omp_dpotrf_blasfeo(plasma_enum_t uplo,
                     int n,
                     struct blasfeo_dmat *sA, int ai, int aj,
                     int iinfo,
                     plasma_sequence_t *sequence, plasma_request_t *request)
{
    // make a local copy of the structure, such that the orignal one can be safely destroyed when out of scope
	struct blasfeo_dmat sA2;
    sA2 = *sA;

    double *A = sA->pA;
    int sda = sA->cn;

    // #pragma omp task depend(inout:A[0:lda*n])
    #pragma omp task depend(inout:A[0:sda*n])
    {
        if (sequence->status == PlasmaSuccess) {
            fprintf(stderr, "before core dpotrf2\n");
            int info = plasma_core_dpotrf_blasfeo(uplo,
                                   n,
                                   &sA2, ai, aj);
            if (info != 0)
                plasma_request_fail(sequence, request, iinfo+info);
        }
    }
}
