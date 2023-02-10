#include <immintrin.h>
#include <stdio.h>
#include <time.h>

#define N 100

int main()
{
    double a[N][N] __attribute__((aligned(16)));
    double b[N][N] __attribute__((aligned(16)));
    double c[N][N] __attribute__((aligned(16)));
    double d[N][N] __attribute__((aligned(16)));
    for (int i = 0; i < N; ++i)
    {
        for (int j = 0; j < N; ++j)
        {
            a[i][j] = 13;
            b[i][j] = 22;
        }
    }
    
    clock_t begin = clock();

    for (int i = 0; i < N; i += 1)
    {
        for (int j = 0; j < N; j += 4)
        {
            for (int k = 0; k < N; k += 2)
            {

                __m128d result = _mm_load_pd(&d[i][j]);
                __m128d a_line = _mm_load_pd(&a[i][k]);
                __m128d b_line0 = _mm_load_pd(&b[k][j + 0]);
                __m128d b_line1 = _mm_loadu_pd(&b[k][j + 1]);
                __m128d b_line2 = _mm_loadu_pd(&b[k][j + 2]);
                __m128d b_line3 = _mm_loadu_pd(&b[k][j + 3]);
                result = _mm_add_pd(result, _mm_mul_pd(_mm_shuffle_pd(a_line, a_line, 0x00), b_line0));
                result = _mm_add_pd(result, _mm_mul_pd(_mm_shuffle_pd(a_line, a_line, 0x55), b_line1));
                result = _mm_add_pd(result, _mm_mul_pd(_mm_shuffle_pd(a_line, a_line, 0xaa), b_line2));
                result = _mm_add_pd(result, _mm_mul_pd(_mm_shuffle_pd(a_line, a_line, 0xff), b_line3));
                _mm_store_pd(&d[i][j], result);
            }
        }
    }

    clock_t end = clock();

    double TotalTimeSIMD = ((double)end - (double)begin) / CLOCKS_PER_SEC;
    printf(" Time taken by SIMD optimized code is %.7f \n", TotalTimeSIMD);
    return 0;
}