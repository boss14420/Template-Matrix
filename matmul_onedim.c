/*
 * Sample program to test runtime of simple matrix multiply
 * with and without OpenMP on gcc-4.3.3-tdm1 (mingw)
 * 
 * (c) 2009, Rajorshi Biswas
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>

#include <omp.h>


int main(int argc, char **argv)
{
    int i,j;
    int n;
    register int idx1, idx2, k, temp;
    double start, end, run;

    printf("Enter dimension ('N' for 'NxN' matrix) (100-7000): ");
    scanf("%d", &n);

    assert( n >= 100 && n <= 7000 );

    int *arr1 = malloc( sizeof(int) * n *n);
    int *arr2 = malloc( sizeof(int) * n *n);
    int *arr3 = malloc( sizeof(int) * n *n);

    printf("Populating array with random values...\n"); 
    srand( time(NULL) );

    for(i=0; i<n*n; ++i) {
        arr1[i] = (rand() % n);
        arr2[i] = (rand() % n);
    }

    printf("Completed array init.\n");
    printf("Crunching without OMP...");
    fflush(stdout);
    start = omp_get_wtime();

    for(i=0; i<n; ++i) {
        idx1 = i * n;
        for(j=0; j<n; ++j) {
            idx2 = j * n;
            temp = 0;
            for(k=0; k<n; ++k) {
                temp += arr1[idx1+k] * arr2[idx2+k];
            }
            arr3[idx1+j] = temp;
        }
    }

    end = omp_get_wtime();
    printf(" took %f seconds with %lld multiplications.\n", end-start, (long long)n * n *n);
    printf("Crunching with OMP...");
    fflush(stdout);
    start = omp_get_wtime();

/*   idx1 = 0;
 *#pragma omp parallel for private(i, j, k, temp, idx1, idx2)
 *     for(i=0 ; i<n; ++i) {
 *         //        idx1 = i * n;
 *         for(idx2=0, j=0; j<n; ++j) {
 *             //            idx2 = j * n;
 *             temp = 0;
 * //            arr3[idx1+j] = 0;
 *             for(k=0, --idx1, --idx2; k<n; k+=16) {
 *                 temp += arr1[++idx1] * arr2[++idx2]
 *                             + arr1[++idx1] * arr2[++idx2]
 *                             + arr1[++idx1] * arr2[++idx2]
 *                             + arr1[++idx1] * arr2[++idx2]
 *                             + arr1[++idx1] * arr2[++idx2]
 *                             + arr1[++idx1] * arr2[++idx2]
 *                             + arr1[++idx1] * arr2[++idx2]
 *                             + arr1[++idx1] * arr2[++idx2]
 *                             + arr1[++idx1] * arr2[++idx2]
 *                             + arr1[++idx1] * arr2[++idx2]
 *                             + arr1[++idx1] * arr2[++idx2]
 *                             + arr1[++idx1] * arr2[++idx2]
 *                             + arr1[++idx1] * arr2[++idx2]
 *                             + arr1[++idx1] * arr2[++idx2]
 *                             + arr1[++idx1] * arr2[++idx2]
 *                             + arr1[++idx1] * arr2[++idx2];
 *             }
 * 
 *             idx1 -= n-1;
 *             arr3[idx1+j] = temp;
 *         }
 *         idx1 += n;
 *     }
 */

#pragma omp parallel for private(i, j, k, temp, idx1, idx2)
    for(i=0; i<n; ++i) {
        idx1 = i * n;
        for(j=0; j<n; ++j) {
            idx2 = j * n;
            temp = 0;
            for(k=0; k<n; ++k) {
                temp += arr1[idx1+k] * arr2[idx2+k];
            }
            arr3[idx1+j] = temp;
        }
    }

    end = omp_get_wtime();
    printf(" took %f seconds.\n", end-start);

    return 0;
}


