#include <immintrin.h>
#include <stdio.h>
#include <time.h>

#define N 1024

float a[N][N] __attribute__((aligned(16)));
float b[N][N] __attribute__((aligned(16)));
float c[N][N] __attribute__((aligned(16)));
float d[N][N] __attribute__((aligned(16)));

void randomize(float arr[N][N])
{
    int i, j;
    for (i = 0; i < N; i++)
    {
        for (j = 0; j < N; j++)
        {
            arr[i][j] = (float)(rand() / (RAND_MAX));
        }
    }
}

void naive_mul()
{
    int i, j, k;
    for (i = 0; i < N; i++)
    {
        for (j = 0; j < N; j++)
        {
            for (k = 0; k < N; k++)
            {
                d[i][j] = d[i][j] + a[i][k] * b[k][j];
            }
        }
    }
}

void simd_mul()
{
    for (int i = 0; i < N; i += 1)
    {
        for (int j = 0; j < N; j += 4)
        {
            for (int k = 0; k < N; k += 4)
            {
                __m128 r = _mm_load_ps(&d[i][j]);
                __m128 s = _mm_load_ps(&a[i][k]);
                __m128 b_0 = _mm_load_ps(&b[k][j + 0]);
                __m128 b_1 = _mm_loadu_ps(&b[k][j + 1]);
                __m128 b_2 = _mm_loadu_ps(&b[k][j + 2]);
                __m128 b_3 = _mm_loadu_ps(&b[k][j + 3]);

                r = _mm_add_ps(r, _mm_mul_ps(_mm_shuffle_ps(s, s, 0x00), b_0));
                r = _mm_add_ps(r, _mm_mul_ps(_mm_shuffle_ps(s, s, 0x55), b_1));
                r = _mm_add_ps(r, _mm_mul_ps(_mm_shuffle_ps(s, s, 0xaa), b_2));
                r = _mm_add_ps(r, _mm_mul_ps(_mm_shuffle_ps(s, s, 0xff), b_3));

                _mm_store_ps(&c[i][j], r);
            }
        }
    }
}

int main()
{

    clock_t begin, end;
    double elapsed = 0.0f;

    randomize(a);
    randomize(b);

    begin = clock();
    simd_mul();
    end = clock();
    elapsed = (double)(end - begin) / CLOCKS_PER_SEC;
    printf("SIMD %.6f \n", elapsed);

    begin = clock();
    naive_mul();
    end = clock();
    elapsed = (double)(end - begin) / CLOCKS_PER_SEC;
    printf("Naive %.6f \n", elapsed);

    return 0;
}