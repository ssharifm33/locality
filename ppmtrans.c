/*
 * ppmtrans.c
 * Nick Dresens 
 * Sam Miller
 * 02/22/2023
 *
 * COMP40 HW2 - locality 
 * 
 * Purpose: The implementation of our ppmgtrans Program. This is the 
 *          the implementation for a program that is capable of rotating or
 *          flipping images with various differnt mapping techniques. The 
 *          timing feature of this program gives insight into the different
 *          strengths and weakenss's of the computer cache. 
 *
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

#include "assert.h"
#include "a2methods.h"
#include "a2plain.h"
#include "a2blocked.h"
#include "pnm.h"
#include "cputiming.h"

#define SET_METHODS(METHODS, MAP, WHAT) do {                    \
        methods = (METHODS);                                    \
        assert(methods != NULL);                                \
        map = methods->MAP;                                     \
        if (map == NULL) {                                      \
                fprintf(stderr, "%s does not support "          \
                                WHAT "mapping\n",               \
                                argv[0]);                       \
                exit(1);                                        \
        }                                                       \
} while (false)


static void
usage(const char *progname)
{
        fprintf(stderr, "Usage: %s [-rotate <angle>] "
                        "[-{row,col,block}-major] [filename]\n",
                        progname);
        exit(1);
}

/* Functions */
void applyLogic(A2Methods_mapfun map, Pnm_ppm og_image, int rotation, char *time_file_name);
void apply90(int col, int row, A2Methods_UArray2 array2b, void *elem, void *closure);
void apply180(int col, int row, A2Methods_UArray2 array2b, void *elem, void *closure);
void apply0(int col, int row, A2Methods_T array2b, void *elem, void *finalArray); 
void apply270(int col, int row, A2Methods_UArray2 array2b, void *elem, void *closure);

/* Closure struct for our apply mapping functions */
struct closure{
        A2Methods_UArray2 final;
        A2Methods_T methods;
};

int main(int argc, char *argv[]) 
{
        char *time_file_name = NULL;
        int   rotation       = 0;
        int   i;
        Pnm_ppm original_image;
        /* default to UArray2 methods */
        A2Methods_T methods = uarray2_methods_plain; 
        assert(methods);

        /* default to best map */
        A2Methods_mapfun *map = methods->map_default;
        assert(map);

        /*  */

        for (i = 1; i < argc; i++) {
                if (strcmp(argv[i], "-row-major") == 0) {
                        SET_METHODS(uarray2_methods_plain, map_row_major, 
                                    "row-major");
                } else if (strcmp(argv[i], "-col-major") == 0) {
                        SET_METHODS(uarray2_methods_plain, map_col_major, 
                                    "column-major");
                } else if (strcmp(argv[i], "-block-major") == 0) {
                        SET_METHODS(uarray2_methods_blocked, map_block_major,
                                    "block-major");
                } else if (strcmp(argv[i], "-rotate") == 0) {
                        if (!(i + 1 < argc)) {      /* no rotate value */
                                usage(argv[0]);
                        }
                        char *endptr;
                        rotation = strtol(argv[++i], &endptr, 10);
                        if (!(rotation == 0 || rotation == 90 ||
                            rotation == 180 || rotation == 270)) {
                                fprintf(stderr, 
                                        "Rotation must be 0, 90 180 or 270\n");
                                usage(argv[0]);
                        }
                        if (!(*endptr == '\0')) {    /* Not a number */
                                usage(argv[0]);
                        }


                } else if (strcmp(argv[i], "-time") == 0) {
                        time_file_name = argv[++i];
                } else if (*argv[i] == '-') {
                        fprintf(stderr, "%s: unknown option '%s'\n", argv[0],
                                argv[i]);
                        usage(argv[0]);
                } else if (argc - i > 1) {
                        fprintf(stderr, "Too many arguments\n");
                        usage(argv[0]);
                } else {
                        break;
                }
        }
        
        /* checking if file was provided */
        if (i != argc) {

                FILE *fp = fopen(argv[argc - 1], "rb");
                if (fp == NULL) {
                        fprintf(stderr,
                                "Could not open file for reading\n");
                        exit(EXIT_FAILURE);
                }
                /* read in image */
                original_image = Pnm_ppmread(fp, methods);
                fclose(fp);
        } else {
                original_image = Pnm_ppmread(stdin, methods);
        }

        /* figure out which mapping function to use */
        applyLogic(map, original_image, rotation, time_file_name);

        /* write to standard output */
        Pnm_ppmwrite(stdout, original_image);
        Pnm_ppmfree(&original_image);
        
        exit(EXIT_SUCCESS);
}


/*
 * name:      applyLogic()
 * purpose:   Brings together all the logic needed to perform the transormation
 *            that the user calls for on the image. Makes new image dimensions
 *            depending on the rotation and then calls our apply function to 
 *            copy over the pixels to the right locations. 
 * arguments: (A2Methods_mapfun map,Pnm_ppm og_image,
 *            int rotation, char *time_file_name) 
 * returns:   The final rotated image
 * errors:    the original image must exist, the map must exist, and there must
 *            be space on the heap for the closure struct. 
 */
void applyLogic(A2Methods_mapfun map,
                Pnm_ppm og_image,
                int rotation, char *time_file_name) {
                        
        assert(og_image != NULL);
        assert(map);

        struct closure *final_str = malloc(sizeof(struct closure));
        assert(final_str != NULL);
        final_str->methods = (A2Methods_T)og_image->methods;
        /* Start timer */
        CPUTime_T timer;
	double time_used;
        timer = CPUTime_New();
        CPUTime_Start(timer);

        //CPUTime_Start(timer);
        if (rotation == 90) {
                /* set up final dimensions of image */
                final_str->final = final_str->methods->new(
                                        og_image->methods->height(og_image->pixels),
                                        og_image->methods->width(og_image->pixels),
                                        og_image->methods->size(og_image->pixels));

                og_image->width = og_image->methods->height(og_image->pixels);
                og_image->height = og_image->methods->width(og_image->pixels);

                map(og_image->pixels, apply90, final_str);

        } else if (rotation == 180) {
                /* set up final dimensions of image */
                final_str->final = final_str->methods->new(og_image->methods->width(og_image->pixels),
                                               og_image->methods->height(og_image->pixels),
                                               og_image->methods->size(og_image->pixels));
                map(og_image->pixels, apply180, final_str);
        } else if (rotation == 270) {
                /* set up final dimensions of image */
                final_str->final = final_str->methods->new(
                                        og_image->methods->height(og_image->pixels),
                                        og_image->methods->width(og_image->pixels),
                                        og_image->methods->size(og_image->pixels));
                map(og_image->pixels, apply270, final_str);
        }
                // unimplemented
                // rotate90();
        //  else if (roation == flipped)  {
        //         /* set up final dimensions of image */
        //         final_str->final = final_str->methods->new(
        //                                 og_image->methods->height(og_image->pixels),
        //                                 og_image->methods->width(og_image->pixels),
        //                                 og_image->methods->size(og_image->pixels));
        //         printf("before\n");
        //         map(og_image->pixels, apply90, final_str);
        //         printf("after\n");
        //}

        if (rotation != 0) {
                og_image->methods->free(&(og_image->pixels));
                og_image->pixels = final_str->final;
        }

        time_used = CPUTime_Stop(timer);
        if (time_file_name != NULL) {
                FILE *fp = fopen(time_file_name, "w");
                if (fp == NULL) {
                        fprintf(stderr,
                                "Could not open file for writing\n");
                        exit(EXIT_FAILURE);
                }



                int numPixels = og_image->width * og_image->height;
                int average   = time_used / numPixels;
                fprintf(fp, "Average pixel speed: %d", average);

                //fprintf(*time_file_name, "Average pixel speed: %n", average);


                fclose(fp);
        }
        (void) time_used;
        (void) time_file_name;



        CPUTime_Free(&timer);
        free(final_str);
}


/*
 * name:      apply90()
 * purpose:   provides the translation calculations that our apply mapping
 *            needs to copy over each pixel into the correct index of the
 *            final image rotated 90 degrees. Then copies each pixel. 
 * arguments: (int col, int row, A2Methods_UArray2 array2b,
 *            void *elem, void *closure) 
 * returns:   Nothing - updates the pixel of the new image
 * errors:    This program will throw an error if the underlying array doesn't
 *            exist, row or col is less than 0, or closure is NULL. 
 */
void apply90(int col, int row, A2Methods_UArray2 array2b, void *elem, void *closure) {
        
        assert(col >= 0 && row >= 0);
        assert(array2b);
        assert(closure != NULL);

        int newCol, newRow;
        (void) elem;
        int height = ((struct closure *)closure)->methods->height(array2b);
        newCol = height - row - 1;
        newRow = col;
        Pnm_rgb new = ((struct closure *)closure)->methods->at(((struct closure *)closure)->final, newCol, newRow);
        Pnm_rgb old = ((struct closure *)closure)->methods->at(array2b, col, row);

        *new = *old;
}

/*
 * name:      apply180()
 * purpose:   provides the translation calculations that our apply mapping
 *            needs to copy over each pixel into the correct index of the
 *            final image rotated 180 degrees. Then copies each pixel. 
 * arguments: (int col, int row, A2Methods_UArray2 array2b,
 *            void *elem, void *closure) 
 * returns:   Nothing - updates the pixel of the new image
 * errors:    This program will throw an error if the underlying array doesn't
 *            exist, row or col is less than 0, or closure is NULL. 
 */
void apply180(int col, int row, A2Methods_UArray2 array2b, void *elem, void *closure) {
        
        int newCol, newRow;
        (void) elem;
        int height = ((struct closure *)closure)->methods->height(array2b);
        int width = ((struct closure *)closure)->methods->width(array2b);
        newCol = width - col - 1;
        newRow = height - row - 1;
        Pnm_rgb new = ((struct closure *)closure)->methods->at(((struct closure *)closure)->final, newCol, newRow);
        Pnm_rgb old = ((struct closure *)closure)->methods->at(array2b, col, row);

        *new = *old;
}

/*
 * name:      apply270()
 * purpose:   provides the translation calculations that our apply mapping
 *            needs to copy over each pixel into the correct index of the
 *            final image rotated 270 degrees. Then copies each pixel. 
 * arguments: (int col, int row, A2Methods_UArray2 array2b,
 *            void *elem, void *closure) 
 * returns:   Nothing - updates the pixel of the new image
 * errors:    This program will throw an error if the underlying array doesn't
 *            exist, row or col is less than 0, or closure is NULL. 
 */
void apply270(int col, int row, A2Methods_UArray2 array2b, void *elem, void *closure) {
        
        int newCol, newRow;
        (void) elem;
        int width = ((struct closure *)closure)->methods->width(array2b);
        newRow = width - col - 1;
        newCol = row;
        Pnm_rgb new = ((struct closure *)closure)->methods->at(((struct closure *)closure)->final, newCol, newRow);
        Pnm_rgb old = ((struct closure *)closure)->methods->at(array2b, col, row);

        *new = *old;
}
