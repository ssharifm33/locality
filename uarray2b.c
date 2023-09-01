#include <stdio.h>
#include <stdlib.h>
#include <uarray.h>
#include <assert.h>
#include <math.h>
#include "uarray.h"
#include "uarray2.h"
#include "uarray2b.h"

#define T UArray2b_T

struct UArray2b_T {
        int height;
        int width;
        int size;
        int blocksize;
        UArray2_T outer;
};

/*
* new blocked 2d array
* blocksize = square root of # of cells in block.
* blocksize < 1 is a checked runtime error
*/
/* read in the UArray in row major */
extern T UArray2b_new(int width, int height, int size, int blocksize) {
        assert(width >= 0 && height >= 0 && size > 0 && blocksize > 0);

        int outerWidth = width / blocksize;
        int outerHeight = height / blocksize;
        if (width % blocksize != 0) {
                outerWidth++;
        }
        if (height % blocksize != 0) {
                outerHeight++;
        }
        
        T array = (T) malloc(sizeof(struct T));
        assert(array != NULL);

        array->outer = UArray2_new(outerWidth, outerHeight,
                                   blocksize * blocksize * size);
        array->width = width;
        array->height = height;
        array->size = size;
        array->blocksize = blocksize;
        
        for (int w = 0; w < outerHeight; w++) {
                for (int h = 0; h < outerWidth; h++) {
                        UArray_T *inner = UArray2_at(array->outer, w, h);
                        *inner = UArray_new(blocksize * blocksize, size);
                }
        }

        return array;
}

/* new blocked 2d array: blocksize as large as possible provided
* block occupies at most 64KB (if possible)
*/
extern T UArray2b_new_64K_block(int width, int height, int size) {
        /* size of 64 kilobytes in bytes */
        const int kb_64 = 64 * 1024;
        
        /* if cell size is larger than 64 kb, make blocksize 1 */
        /* else make blocksize as large as possible while less than 64 kb */
        if (size > kb_64) {
                return UArray2b_new(width, height, size, 1);
        } else {
                return UArray2b_new(width, height, size, sqrt(kb_64 / size));
        }
}

extern void UArray2b_free (T *array2b) {
        assert(array2b != NULL);
        for (int w = 0; w < UArray2_height((*array2b)->outer); w++) {
                for (int h = 0; h < UArray2_width((*array2b)->outer); h++) {
                        UArray_T *inner = UArray2_at((*array2b)->outer, w, h);
                        UArray_free(inner);
                }
        }
        UArray2_free(&(*array2b)->outer);
        free(*array2b);
}
extern int UArray2b_width (T array2b) {
        assert(array2b);
        return array2b->width;
}
extern int UArray2b_height (T array2b) {
        assert(array2b);
        return array2b->height;
}
extern int UArray2b_size (T array2b) {
        assert(array2b);
        return array2b->size;
}
extern int UArray2b_blocksize(T array2b) {
        assert(array2b);
        return array2b->blocksize;
}
/* return a pointer to the cell in the given column and row.
* index out of range is a checked run-time error
*/
extern void *UArray2b_at(T array2b, int column, int row) {
        
        assert(array2b);
        // width and height might be switched here
        assert(column >= 0 && column < array2b->width);
        assert(row    >= 0 && row    < array2b->height);
        
        int bs = array2b->blocksize;
        UArray_T *inner = UArray2_at(array2b->outer, column / bs, row / bs);
        return UArray_at(*inner, bs * (column % bs) + (row % bs));
}
/* visits every cell in one block before moving to another block */
extern void UArray2b_map(T array2b,
                         void apply(int col, int row,
                                    T array2b, void *elem, void *cl),
                         void *cl) {
        
        assert(array2b);

        for (int x = 0; x < UArray2_width(array2b->outer); x++) {
                for (int y = 0; y < UArray2_height(array2b->outer); y++) {
                        UArray_T *inner = UArray2_at(array2b->outer, x, y);
                        for (int z = 0; z < UArray_size(*inner); z++) {
                                apply(x, y, array2b, UArray2b_at(array2b, x, y), cl);
                        }
                }
        }
}

/*
* it is a checked run-time error to pass a NULL T
* to any function in this interface
*/

#undef T