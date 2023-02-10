#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define N 1024 // 64 .. 2048
#define BS 64  // 32 512

float A[N][N];
float B[N][N];
float C[N][N];
float D[N][N];

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
				D[i][j] = D[i][j] + A[i][k] * B[k][j];
			}
		}
	}
}

void tiled_mul(int blockSize)
{
	int i, j, k, ii, jj, kk;
	for (ii = 0; ii < N; ii += blockSize)
	{
		for (jj = 0; jj < N; jj += blockSize)
		{
			for (kk = 0; kk < N; kk += blockSize)
			{
				for (i = ii; i < ii + blockSize && i < N; i++)
				{
					for (j = jj; j < jj + blockSize && j < N; j++)
					{
						for (k = kk; k < kk + blockSize && k < N; k++)
						{
							C[i][j] = C[i][j] + A[i][k] * B[k][j];
						}
					}
				}
			}
		}
	}
}

int main()
{
	clock_t begin, end;
	srand(time(NULL));

	double elapsed = 0.0f;

	randomize(A);
	randomize(B);

	begin = clock();
	tiled_mul(BS);
	end = clock();
	elapsed = (double)(end - begin) / CLOCKS_PER_SEC;
	printf("Tiled %.6f \n", elapsed);

	begin = clock();
	naive_mul();
	end = clock();
	elapsed = (double)(end - begin) / CLOCKS_PER_SEC;
	printf("Naive %.6f \n", elapsed);

	return EXIT_SUCCESS;
}
