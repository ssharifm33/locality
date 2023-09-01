/*
 * a2plain.c
 * Nick Dresens 
 * Sam Miller
 * 02/22/2023
 *
 * COMP40 HW2 - locality 
 * 
 * Purpose: This is an implementation of the A2Methods_T interface using a 
 *          plain UArray2_T type from the previous homework. The A2Methods_T
 *          interface defines functions that operate on two-dimensional arrays.
 *
 */
#include <string.h>
#include <assert.h>
#include <a2plain.h>
#include "uarray2.h"


/************************************************/
/* Define a private version of each function in */
/* A2Methods_T that we implement.               */
/************************************************/
typedef A2Methods_UArray2 A2;

static A2Methods_UArray2 new(int width, int height, int size)
{ 
        return UArray2_new(width, height, size);
}

static A2Methods_UArray2 new_with_blocksize(int width, int height, int size,
                                            int blocksize)
{
        (void) blocksize;
        return UArray2_new(width, height, size);
        // what do we do with blocksize?
}


/* TODO: ...many more private (static) definitions follow */

static void a2free(A2 * array2p)
{
        assert(array2p);
        UArray2_free((UArray2_T *)array2p);
}

static int width(A2 array2)
{
        assert(array2);
        return UArray2_width(array2);
}
static int height(A2 array2)
{
        assert(array2);
        return UArray2_height(array2);
}
static int size(A2 array2)
{
        assert(array2);
        return UArray2_size(array2);
}
static int blocksize(A2 array2)
{
        assert(array2);
        return 1;
}

static A2Methods_Object *at(A2 array2, int i, int j)
{
        assert(array2);
        assert(i >= 0 && j >= 0);
        return UArray2_at(array2, i, j);
}


static void map_row_major(A2Methods_UArray2 uarray2,
                          A2Methods_applyfun apply,
                          void *cl)
{
        UArray2_map_row_major(uarray2, (UArray2_applyfun*)apply, cl);
}

static void map_col_major(A2Methods_UArray2 uarray2,
                          A2Methods_applyfun apply,
                          void *cl)
{
        UArray2_map_col_major(uarray2, (UArray2_applyfun*)apply, cl);
}

struct small_closure {
        A2Methods_smallapplyfun *apply;
        void                    *cl;
};

static void apply_small(int i, int j, UArray2_T uarray2,
                        void *elem, void *vcl)
{
        (void) i;
        (void) j;
        (void) uarray2;
        struct small_closure *cl = vcl;
        cl->apply(elem, cl->cl);
}

static void small_map_row_major(A2Methods_UArray2        a2,
                                A2Methods_smallapplyfun  apply,
                                void *cl)
{
        struct small_closure mycl = { apply, cl };
        UArray2_map_row_major(a2, apply_small, &mycl);
}

static void small_map_col_major(A2Methods_UArray2        a2,
                                A2Methods_smallapplyfun  apply,
                                void *cl)
{
        struct small_closure mycl = { apply, cl };
        UArray2_map_col_major(a2, apply_small, &mycl);
}
// void UArray2_map_row_major(UArray2_T array2,
//                 void apply(int i, int j, UArray2_T a2, void *elem, void *cl),

static struct A2Methods_T uarray2_methods_plain_struct = {
        new,
        new_with_blocksize,
        a2free,
        width,
        height,
        size,
        blocksize,
        at,
        map_row_major,
        map_col_major,
        NULL,
        map_row_major,
        small_map_row_major,
        small_map_col_major,
        NULL,
        small_map_row_major,
};

// finally the payoff: here is the exported pointer to the struct

A2Methods_T uarray2_methods_plain = &uarray2_methods_plain_struct;
