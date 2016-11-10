/**
 *
 * @file
 *
 *  PLASMA is a software package provided by:
 *  University of Tennessee, US,
 *  University of Manchester, UK.
 *
 * @precisions normal z -> s d c
 *
 **/

#include "plasma_async.h"
#include "plasma_context.h"
#include "plasma_descriptor.h"
#include "plasma_internal.h"
#include "plasma_types.h"
#include "plasma_workspace.h"
#include "plasma_rh_tree.h"
#include "core_blas.h"

#define A(m, n) (plasma_complex64_t*)plasma_tile_addr(A, m, n)
#define B(m, n) (plasma_complex64_t*)plasma_tile_addr(B, m, n)
#define T(m, n) (plasma_complex64_t*)plasma_tile_addr(T, m, n)
#define T2(m, n) (plasma_complex64_t*)plasma_tile_addr(T, m, n+(T.nt/2))
/***************************************************************************//**
 *  Parallel application of Q using tile V based on a tree Householder reduction
 *  algorithm
 * @see plasma_omp_zgeqrs
 **/
void plasma_pzunmqrrh(plasma_enum_t side, plasma_enum_t trans,
                      plasma_desc_t A, plasma_desc_t T, plasma_desc_t B,
                      plasma_workspace_t work,
                      plasma_sequence_t *sequence, plasma_request_t *request)
{
    static const int debug = 0;
    if (debug) printf("executing pzunmqrrh\n");

    // Check sequence status.
    if (sequence->status != PlasmaSuccess) {
        plasma_request_fail(sequence, request, PlasmaErrorSequence);
        return;
    }

    // Precompute order of QR operations.
    int *operations = NULL;
    int noperations;
    plasma_rh_tree_operations(A.mt, A.nt, &operations, &noperations);

    // Set inner blocking from the T tile row-dimension.
    int ib = T.mb;

    if (side == PlasmaLeft) {
        for (int iop = 0; iop < noperations; iop++) {
            int ind_operation;
            // revert the order of Householder reflectors for nontranspose
            if (trans == Plasma_ConjTrans) {
                ind_operation = iop;
            }
            else {
                ind_operation = noperations - 1 - iop;
            }

            int j, k, kpiv;
            plasma_enum_t kernel;
            plasma_rh_tree_operation_get(operations,ind_operation,
                                         &kernel, &j, &k, &kpiv);

            int nvaj = plasma_tile_nview(A, j);
            int mvak = plasma_tile_mview(A, k);
            int ldak = plasma_tile_mmain(A, k);
            int mvbk = plasma_tile_mview(B, k);
            int ldbk = plasma_tile_mmain(B, k);

            if (kernel == PlasmaGEKernel) {
                // triangularization
                for (int n = 0; n < B.nt; n++) {
                    int nvbn = plasma_tile_nview(B, n);

                    if (debug) printf("UNMQR (%d,%d,%d) ", k, j, n);
                    core_omp_zunmqr(side, trans,
                                    mvbk, nvbn, imin(mvak, nvaj), ib,
                                    A(k, j), ldak,
                                    T(k, j), T.mb,
                                    B(k, n), ldbk,
                                    work,
                                    sequence, request);
                }

                if (debug) printf("\n ");
            }
            else if (kernel == PlasmaTTKernel) {
                // elimination of the tile
                int mvakpiv = plasma_tile_mview(A, kpiv);
                int mvbkpiv = plasma_tile_mview(B, kpiv);
                int ldbkpiv = plasma_tile_mmain(B, kpiv);

                for (int n = 0; n < B.nt; n++) {
                    int nvbn = plasma_tile_nview(B, n);

                    if (debug) printf("TTMQR (%d,%d,%d,%d) ", k, kpiv, j, n);
                    core_omp_zttmqr(
                        side, trans,
                        mvbkpiv, nvbn, mvbk, nvbn, imin(mvakpiv+mvak, nvaj), ib,
                        B(kpiv, n), ldbkpiv,
                        B(k,    n), ldbk,
                        A(k,    j), ldak,
                        T2(k,   j), T.mb,
                        work,
                        sequence, request);
                }

                if (debug) printf("\n ");
            }
            else if (kernel == PlasmaTSKernel) {
                // elimination of the tile
                int mvakpiv = plasma_tile_mview(A, kpiv);
                int mvbkpiv = plasma_tile_mview(B, kpiv);
                int ldbkpiv = plasma_tile_mmain(B, kpiv);

                for (int n = 0; n < B.nt; n++) {
                    int nvbn = plasma_tile_nview(B, n);

                    if (debug) printf("TSMQR (%d,%d,%d,%d) ", k, kpiv, j, n);
                    core_omp_ztsmqr(
                        side, trans,
                        mvbkpiv, nvbn, mvbk, nvbn, imin(mvakpiv+mvak, nvaj), ib,
                        B(kpiv, n), ldbkpiv,
                        B(k,    n), ldbk,
                        A(k,    j), ldak,
                        T2(k,   j), T.mb,
                        work,
                        sequence, request);
                }

                if (debug) printf("\n ");
            }
            else {
                plasma_error("illegal kernel");
                plasma_request_fail(sequence, request, PlasmaErrorIllegalValue);
            }
        }
    }
    else {
        //=================================
        // PlasmaRight
        //=================================
        for (int iop = 0; iop < noperations; iop++) {
            int ind_operation;
            // revert the order of Householder reflectors for transpose
            if (trans == Plasma_ConjTrans) {
                ind_operation = noperations - 1 - iop;
            }
            else {
                ind_operation = iop;
            }

            int j, k, kpiv;
            plasma_enum_t kernel;
            plasma_rh_tree_operation_get(operations,ind_operation,
                                         &kernel, &j, &k, &kpiv);

            int nvbk = plasma_tile_nview(B, k);
            int mvak = plasma_tile_mview(A, k);
            int nvaj = plasma_tile_nview(A, j);
            int ldak = plasma_tile_mmain(A, k);

            if (kernel == PlasmaGEKernel) {
                // triangularization
                for (int m = 0; m < B.mt; m++) {
                    int mvbm = plasma_tile_mview(B, m);
                    int ldbm = plasma_tile_mmain(B, m);

                    if (debug) printf("UNMQR (%d,%d,%d) ", k, j, m);
                    core_omp_zunmqr(
                        side, trans,
                        mvbm, nvbk, imin(mvak, nvaj), ib,
                        A(k, j), ldak,
                        T(k, j), T.mb,
                        B(m, k), ldbm,
                        work,
                        sequence, request);
                }

                if (debug) printf("\n ");
            }
            else if (kernel == PlasmaTTKernel) {
                int nvbkpiv = plasma_tile_nview(B, kpiv);
                int mvakpiv = plasma_tile_mview(A, kpiv);
                // elimination of the tile
                for (int m = 0; m < B.mt; m++) {
                    int mvbm = plasma_tile_mview(B, m);
                    int ldbm = plasma_tile_mmain(B, m);

                    if (debug) printf("TTMQR (%d,%d,%d,%d)", k, kpiv, j, m);
                    core_omp_zttmqr(
                        side, trans,
                        mvbm, nvbkpiv, mvbm, nvbk, imin(mvakpiv+mvak,nvaj), ib,
                        B(m, kpiv), ldbm,
                        B(m, k),    ldbm,
                        A(k,  j), ldak,
                        T2(k, j), T.mb,
                        work,
                        sequence, request);
                }

                if (debug) printf("\n ");
            }
            else if (kernel == PlasmaTSKernel) {
                int nvbkpiv = plasma_tile_nview(B, kpiv);
                int mvakpiv = plasma_tile_mview(A, kpiv);
                // elimination of the tile
                for (int m = 0; m < B.mt; m++) {
                    int mvbm = plasma_tile_mview(B, m);
                    int ldbm = plasma_tile_mmain(B, m);

                    if (debug) printf("TSMQR (%d,%d,%d,%d)", k, kpiv, j, m);
                    core_omp_ztsmqr(
                        side, trans,
                        mvbm, nvbkpiv, mvbm, nvbk, imin(mvakpiv+mvak,nvaj), ib,
                        B(m, kpiv), ldbm,
                        B(m, k),    ldbm,
                        A(k,  j), ldak,
                        T2(k, j), T.mb,
                        work,
                        sequence, request);
                }

                if (debug) printf("\n ");
            }
            else {
                plasma_error("illegal kernel");
                plasma_request_fail(sequence, request, PlasmaErrorIllegalValue);
            }
        }
    }

    free(operations);
}
