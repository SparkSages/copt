#include "copt_fun.h"
#include <stdlib.h>
#include <pthread.h>
#include <omp.h>

/******************************************************************************
 * Usage: copt OP N LOOP
 *
 * copt measures the execution time impact of source level optimizations
 * in C. copt runs and times an unoptimized and optimized version of a given
 * operation and input size.
 *
 * Arguments description:
 * OP is the operation to run for this invocation of copt. There are four
 * possible operations, each of which takes exactly one argument N:
 *   0: initialize a pair of square integer matrices. N is the size of the
 *      matrices.
 *   1: initialize an integer array. N is the length of the array
 *   2: compute factorial with a recursive routine. N is the number for
 *      which the routine computes the factorial
 *   3: multiply two square integer matrices. N is the size of the matrix
 *
 * LOOP is the number of times to run the given operation with the given
 * argument. Timing starts before the first operation begins and ends when
 * the last operation has completed.
 *
 * OP, N, and LOOP all must be integers <= INT_MAX
 ******************************************************************************/

/******************************************************************************
 * Name:          Brendan Dalhover
 ******************************************************************************/

int check(int x, int y)
{
    return x < y;
}

void set(int *mat, int i, int num)
{
    mat[i] = num;
}

void matrix_initialize_unopt(struct fn_args *args)
{
    int i, j, n;
    int *mat1, *mat2;

    n = args->n;
    mat1 = args->mem1;
    mat2 = args->mem2;

    for (i = 0; check(i, n); i++)
    {
        for (j = 0; check(j, n); j++)
        {
            set(mat1, i * n + j, i);
            set(mat2, i * n + j, i + 1);
        }
    }
}
/**
 * 1. Function inlining for check(int, int) in i loop: no speedup (only 200)
 * 2. Function inlining for check(int, int) in j loop: 1.2x speedup (40000 fewer)
 * 3. Function inlining for set(int*, int, int) for both matrices: 2.1x speedup total
 * 4. Common subexpression elimination in j loop (i*n): 2.7x speedup total
 * 5. Common subexpression elimination in j loop (i+1): 2.5x speedup total
 * 6. Loop unrolling of j by a factor of 2: 2.8x speedup total
 * 7. Loop unrolling of j by a factor of 5: 3.0x speedup total
 * 8. Common subexpression elimination in j unroll (i_offset+j): 3.6x speedup total
 * 9. Map offset and sum variables to registers: 6.0x speedup total
 * 10. Map mat1 and mat2 pointers to registers: 6.3x speedup total
 * 11. Map loop control variables to registers: 6.9x speedup total
 * 12. Strength reduction for i_offset (mult -> add): no difference
 */
void matrix_initialize_opt(struct fn_args *args)
{
    register int i, j, n;
    register int *mat1, *mat2;
    register int i_offset = 0, j_offset, i_sum;

    n = args->n;
    mat1 = args->mem1;
    mat2 = args->mem2;

    for (i = 0; i < n; i++)
    {
        // i_offset = i * n;
        i_sum = i + 1;
        for (j = 0; j < n; j += 5)
        {
            j_offset = i_offset + j;
            mat1[j_offset] = i;
            mat1[j_offset + 1] = i;
            mat1[j_offset + 2] = i;
            mat1[j_offset + 3] = i;
            mat1[j_offset + 4] = i;
            mat2[j_offset] = i_sum;
            mat2[j_offset + 1] = i_sum;
            mat2[j_offset + 2] = i_sum;
            mat2[j_offset + 3] = i_sum;
            mat2[j_offset + 4] = i_sum;
        }
        i_offset += n;
    }
}

void array_initialize_unopt(struct fn_args *args)
{
    int i, mod, n, *arr;

    n = args->n;
    arr = args->mem1;
    for (i = 0; i < n; i++)
    {
        mod = X % Y;
        arr[i] = i * mod * Z;
    }
}

/**
 * 1. moved ints to reg (2.3x Speedup)
 * 2. moved mod outside the loop (2.1) - slower
 * 3. Did arithmetic (Mod * Z) outside loop (2.4)
 * 4. unroll loop by a factor of 12 (2.5)
 * 5. parallelized the loop with 4 threads and divided the work into chunks. (7.1)
 * 6. lets try with all 24 threads. (2.5) - slower
 * 7. use 2 threads instead of 4 (15) - Much faster. I think this is caused by the overhead of creating and joining the threads back together
 */

struct ThreadArgs
{
    int start_index;
    int end_index;
    int mod;
    int *arr;
};

void *thread_initialize(void *arg)
{
    struct ThreadArgs *args = (struct ThreadArgs *)arg;

    for (int i = args->start_index; i < args->end_index; i++)
    {
        args->arr[i] = i * args->mod;
    }

    pthread_exit(NULL);
}

void array_initialize_opt(struct fn_args *args)
{
    int num_threads = 2;
    int n = args->n;
    int *arr = args->mem1;
    int mod = (X % Y) * Z;

    // Create an array of threads
    // basically, makes it easier to dynamically change threadcount for tests.
    pthread_t threads[num_threads];

    // Calculate the chunk size for each thread
    // I use this to determine how much work each thread does
    int chunk_size = n / num_threads;

    // Initialize thread arguments
    struct ThreadArgs thread_args[num_threads];
    for (int i = 0; i < num_threads; i++)
    {
        thread_args[i].start_index = i * chunk_size;
        thread_args[i].end_index = (i == num_threads - 1) ? n : (i + 1) * chunk_size;
        thread_args[i].mod = mod;
        thread_args[i].arr = arr;
    }

    // Create and run threads
    for (int i = 0; i < num_threads; i++)
    {
        pthread_create(&threads[i], NULL, thread_initialize, (void *)&thread_args[i]);
    }

    // Join threads when finished.
    for (int i = 0; i < num_threads; i++)
    {
        pthread_join(threads[i], NULL);
    }
}

unsigned long long factorial_unopt_helper(unsigned long long n)
{
    if (n == 0ull)
        return 1ull;
    return n * factorial_unopt_helper(n - 1);
}

void factorial_unopt(struct fn_args *args)
{
    args->fac = factorial_unopt_helper((unsigned long long)args->n);
}

/**
 *  1. Since 21! is too big for ULL, bruteforce the if logic (7.3x)
 *  2. I dont want that to be the only thing I do for this so ill come back later if I have more time.
 */
void factorial_opt(struct fn_args *args)
{
    // args->fac = factorial_unopt_helper((unsigned long long)args->n);
    int n = args->n;
    unsigned long long result = args->fac;
    if (n == 0 || n == 1)
    {
        result = 1ULL;
    }
    else if (n == 2)
    {
        result = 2ULL;
    }
    else if (n == 3)
    {
        result = 6ULL;
    }
    else if (n == 4)
    {
        result = 24ULL;
    }
    else if (n == 5)
    {
        result = 120ULL;
    }
    else if (n == 6)
    {
        result = 720ULL;
    }
    else if (n == 7)
    {
        result = 5040ULL;
    }
    else if (n == 8)
    {
        result = 40320ULL;
    }
    else if (n == 9)
    {
        result = 362880ULL;
    }
    else if (n == 10)
    {
        result = 3628800ULL;
    }
    else if (n == 11)
    {
        result = 39916800ULL;
    }
    else if (n == 12)
    {
        result = 479001600ULL;
    }
    else if (n == 13)
    {
        result = 6227020800ULL;
    }
    else if (n == 14)
    {
        result = 87178291200ULL;
    }
    else if (n == 15)
    {
        result = 1307674368000ULL;
    }
    else if (n == 16)
    {
        result = 20922789888000ULL;
    }
    else if (n == 17)
    {
        result = 355687428096000ULL;
    }
    else if (n == 18)
    {
        result = 6402373705728000ULL;
    }
    else if (n == 19)
    {
        result = 121645100408832000ULL;
    }
    else if (n == 20)
    {
        result = 2432902008176640000ULL;
    }
    args->fac = result;
}

void matrix_multiply_unopt(struct fn_args *args)
{
    int i, j, k, n;
    int *mat1, *mat2, *res;

    n = args->n;
    mat1 = args->mem1;
    mat2 = args->mem2;
    res = args->mem3;

    for (i = 0; i < n; i++)
    {
        for (j = 0; j < n; j++)
        {
            res[i * n + j] = 0;
            for (k = 0; k < n; k++)
            {
                res[i * n + j] += mat1[i * n + k] * mat2[k * n + j];
            }
        }
    }
}

/**
 * 1. added ints and array to reg (1.3)
 * 2. stored i * n and i * n + j in reg (1.7) - better, these variable name suck
 * 3. unrolled by 8 (2.5) - even better, this is kinda gross though
 * 4. stored k*n+j in an int (2.5) still.. time for tiling
 * 5. Tiling with 32x32 blocks (2.7)
 * 6. throwing more stuff in registers (2.9) so close!!
 *
 * Didn't end up getting to 3x speedup, but after finishing the project I feel i satisfied requirements on this specific method.
 */

#define TILE_SIZE 32
void matrix_multiply_opt(struct fn_args *args)
{
    register int i, j, k, n, ii, jj, kk, ii_end, jj_end, kk_end;
    register int *mat1, *mat2, *res;
    register int iTimesn, inplusJ, knj, temp1, temp2;

    n = args->n;
    mat1 = args->mem1;
    mat2 = args->mem2;
    res = args->mem3;

    for (i = 0; i < n; i += TILE_SIZE)
    {
        for (j = 0; j < n; j += TILE_SIZE)
        {
            for (k = 0; k < n; k += TILE_SIZE)
            {
                ii_end = i + TILE_SIZE < n ? i + TILE_SIZE : n;
                jj_end = j + TILE_SIZE < n ? j + TILE_SIZE : n;
                kk_end = k + TILE_SIZE < n ? k + TILE_SIZE : n;
                {
                    for (ii = i; ii < ii_end; ii++)
                    {
                        iTimesn = ii * n;
                        for (jj = j; jj < jj_end; jj += 8)
                        {
                            inplusJ = iTimesn + jj;

                            for (kk = k; kk < kk_end; kk++)
                            {
                                knj = kk * n + jj;
                                temp1 = mat1[iTimesn + kk];
                                temp2 = mat2[knj];
                                res[inplusJ] += temp1 * temp2;
                                res[inplusJ + 1] += temp1 * mat2[knj + 1];
                                res[inplusJ + 2] += temp1 * mat2[knj + 2];
                                res[inplusJ + 3] += temp1 * mat2[knj + 3];
                                res[inplusJ + 4] += temp1 * mat2[knj + 4];
                                res[inplusJ + 5] += temp1 * mat2[knj + 5];
                                res[inplusJ + 6] += temp1 * mat2[knj + 6];
                                res[inplusJ + 7] += temp1 * mat2[knj + 7];
                            }
                        }
                    }
                }
            }
        }
    }
}
