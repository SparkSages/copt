#include "copt_fun.h"

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
 * Name:          <your name here>
 * Collaboration: <collaborator names here--one per line>
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
 *
 *
 *
 *
 *
 *
 */

void array_initialize_opt(struct fn_args *args)
{
    register int i, mod, n, *arr;

    n = args->n;
    arr = args->mem1;
    for (i = 0; i < n; i++)
    {
        mod = X % Y;
        arr[i] = i * mod * Z;
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

void factorial_opt(struct fn_args *args)
{
    // TODO: optimized implementation goes here
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

void matrix_multiply_opt(struct fn_args *args)
{
    // TODO: optimized implementation goes here
}
