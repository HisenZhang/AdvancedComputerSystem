#include <immintrin.h>
#include <stdio.h>
#include <time.h>

#define N 1024

int16_t a[N][N] __attribute__((aligned(16)));
int16_t b[N][N] __attribute__((aligned(16)));
int16_t c[N][N] __attribute__((aligned(16)));
int16_t c[N][N] __attribute__((aligned(16)));

void randomize(int16_t arr[N][N])
{
    int i, j;
    for (i = 0; i < N; i++)
    {
        for (j = 0; j < N; j++)
        {
            arr[i][j] = (int16_t)rand();
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
                c[i][j] = c[i][j] + a[i][k] * b[k][j];
            }
        }
    }
}

void simd_mul()
{
    for (int i = 0; i < N; i += 1)
    {
        for (int j = 0; j < N; j += 8)
        {
            for (int k = 0; k < N; k += 8)
            {         
                
                __m128i r = _mm_load_si128((__m128i*)&c[i][j]);
                __m128i s = _mm_load_si128((__m128i*)&a[i][k]);
                __m128i b_0 = _mm_load_si128((__m128i*)&b[k][j+0]);
                __m128i b_1 = _mm_loadu_si128((__m128i*)&b[k][j+1]);
                __m128i b_2 = _mm_loadu_si128((__m128i*)&b[k][j+2]);            
                __m128i b_3 = _mm_loadu_si128((__m128i*)&b[k][j+3]);


                r = _mm_add_epi16(r, _mm_mulhrs_epi16(_mm_shufflelo_epi16(s, 0x00), b_0));
                r = _mm_add_epi16(r, _mm_mulhrs_epi16(_mm_shufflelo_epi16(s, 0x55), b_1));
                r = _mm_add_epi16(r, _mm_mulhrs_epi16(_mm_shufflelo_epi16(s, 0xaa), b_2));
                r = _mm_add_epi16(r, _mm_mulhrs_epi16(_mm_shufflelo_epi16(s, 0xff), b_3));
                
                _mm_store_si128((__m128i*)&c[i][j],r);

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

	return EXIT_SUCCESS;
}