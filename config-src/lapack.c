
#if defined( MKL_ILP64 ) || defined( ILP64 )
    typedef long long myint;
#else
    typedef int myint;
#endif

#if defined( LOWERCASE )
    #define FORTRAN_NAME( lower, UPPER ) lower
#elif defined( UPPERCASE )       
    #define FORTRAN_NAME( lower, UPPER ) UPPER
#else                            
    #define FORTRAN_NAME( lower, UPPER ) lower ## _
#endif

#ifdef __cplusplus
    #define EXTERN_C extern "C"
#else
    #define EXTERN_C
#endif

#include <stdio.h>

#define dpotrf FORTRAN_NAME( dpotrf, DPOTRF )

EXTERN_C
void dpotrf( const char* uplo, const myint* n,
             double* A, const myint* lda,
             myint* info );

int main( int argc, char** argv )
{
    myint i, n = 2, info = 0;
    double A[2*2] = { 16, 4,   -1, 5 };
    double L[2*2] = {  4, 1,   -1, 2 };
    double work[1];
    dpotrf( "lower", &n, A, &n, &info );
    if (info != 0) {
        printf( "dpotrf failed: info %d\n", info );
        return 1;
    }
    for (i = 0; i < n*n; ++i) {
        if (A[i] != L[i]) {
            printf( "dpotrf failed: A[%d] %.2f != L[%d] %.2f\n",
                    i, A[i], i, L[i] );
            return 1;
        }
    }
    printf( "dpotrf ok\n" );
    return 0;
}
