/* 
 * trans.c - Matrix transpose B = A^T
 *
 * Each transpose function must have a prototype of the form:
 * void trans(int M, int N, int A[N][M], int B[M][N]);
 *
 * A transpose function is evaluated by counting the number of misses
 * on a 1KB direct mapped cache with a block size of 32 bytes.
 */ 
#include <stdio.h>
#include "cachelab.h"

int is_transpose(int M, int N, int A[N][M], int B[M][N]);

/* 
 * transpose_submit - This is the solution transpose function that you
 *     will be graded on for Part B of the assignment. Do not change
 *     the description string "Transpose submission", as the driver
 *     searches for that string to identify the transpose function to
 *     be graded. 
 */
char transpose_submit_desc[] = "Transpose submission";
void transpose_submit(int M, int N, int A[N][M], int B[M][N])
{
    int i, j, k, a1,a2,a3,a4,a5,a6,a7,a8;

    for (i = 0; i < N; i+=8) {
        for (j = 0; j < M; j+=8) {
            for(k = 0; k < 8 ; k++){
                a1=A[i][j+k];a2=A[i+1][j+k];a3=A[i+2][j+k];a4=A[i+3][j+k];
                a5=A[i+4][j+k];a6=A[i+5][j+k];a7=A[i+6][j+k];a8=A[i+7][j+k];

                B[j+k][i]=a1;B[j+k][i+1]=a2;B[j+k][i+2]=a3;B[j+k][i+3]=a4;
                B[j+k][i+4]=a5;B[j+k][i+5]=a6;B[j+k][i+6]=a7;B[j+k][i+7]=a8;
            }
            
        }
    }    
}

/* 
 * You can define additional transpose functions below. We've defined
 * a simple one below to help you get started. 
 */ 
 char transpose_32_32_desc[] = "Transpose transpose_32_32";
void transpose_32_32(int M, int N, int A[N][M], int B[M][N])
{
  int i, j, k, l;
  int block_size = 8;
  int h_v_blocks = M / block_size;
  for(i=0;i<h_v_blocks;i++)
    {
      for(j=0;j<h_v_blocks;j++)
    {
      for(k=block_size*i;k<block_size+block_size*i;k++)
        {
          for(l=block_size*j;l<block_size+block_size*j;l++)
        {
            B[l][k] = A[k][l];
        //   if(k != l)
        //     {
        //       B[l][k] = A[k][l];
        //     }
        //   else
        //     {
        //       temp = A[k][l];
        //       index = k;
        //     }
        }
        //   if(i == j)
        // {
        //   B[index][index] = temp;
        // }
        }
    }
    }
}
/* 
 * trans - A simple baseline transpose function, not optimized for the cache.
 */
char trans_desc[] = "Simple row-wise scan transpose";
void trans(int M, int N, int A[N][M], int B[M][N])
{
    int i, j, tmp;

    for (i = 0; i < N; i++) {
        for (j = 0; j < M; j++) {
            tmp = A[i][j];
            B[j][i] = tmp;
        }
    }    

}

/*
 * registerFunctions - This function registers your transpose
 *     functions with the driver.  At runtime, the driver will
 *     evaluate each of the registered functions and summarize their
 *     performance. This is a handy way to experiment with different
 *     transpose strategies.
 */
void registerFunctions()
{
    /* Register your solution function */
    registerTransFunction(transpose_submit, transpose_submit_desc); 

    /* Register any additional transpose functions */
    registerTransFunction(trans, trans_desc); 
    registerTransFunction(transpose_32_32, transpose_32_32_desc); 

}

/* 
 * is_transpose - This helper function checks if B is the transpose of
 *     A. You can check the correctness of your transpose by calling
 *     it before returning from the transpose function.
 */
int is_transpose(int M, int N, int A[N][M], int B[M][N])
{
    int i, j;

    for (i = 0; i < N; i++) {
        for (j = 0; j < M; ++j) {
            if (A[i][j] != B[j][i]) {
                return 0;
            }
        }
    }
    return 1;
}

