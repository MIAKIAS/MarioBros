volatile int pixel_buffer_start; // global variable

void plot_pixel(int x, int y, short int line_color);
void clear_screen();
void draw_line(int x0, int y0, int x1, int y1, short int line_color);
void swap(int* a, int* b);
void wait_for_vsync();
void draw_background();
void draw_image();
void update_location();
void init_location();

#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include "image.h"

#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 240

int main(void)
{
    volatile int * pixel_ctrl_ptr = (int *)0xFF203020;
    // declare other variables(not shown)
    // initialize location and direction of rectangles(not shown)

    /* set front pixel buffer to start of FPGA On-chip memory */
    *(pixel_ctrl_ptr + 1) = 0xC8000000; // first store the address in the 
                                        // back buffer
    /* now, swap the front/back buffers, to set the front buffer location */
    wait_for_vsync();
    /* initialize a pointer to the pixel buffer, used by drawing functions */
    pixel_buffer_start = *pixel_ctrl_ptr;
    clear_screen(); // pixel_buffer_start points to the pixel buffer
    /* set back pixel buffer to start of SDRAM memory */
    *(pixel_ctrl_ptr + 1) = 0xC0000000;
    pixel_buffer_start = *(pixel_ctrl_ptr + 1); // we draw on the back buffer

    while (1)
    {
        /* Erase any boxes and lines that were drawn in the last iteration */
        clear_screen();

        // code for drawing the boxes and lines 
        draw_background();
        draw_image(100, 100, Mario_stand, 15, 19);
        draw_image(20, 100, Mario_run, 19, 19);
        draw_image(150, 100, Mario_jump, 19, 17);
        // code for updating the locations of boxes
        //update_location();

        wait_for_vsync(); // swap front and back buffers on VGA vertical sync
        pixel_buffer_start = *(pixel_ctrl_ptr + 1); // new back buffer
    }
}

//helper function to draw any background
void draw_background(){
    for (int x = 0; x < SCREEN_WIDTH; ++x)
    {
        for (int y = 0; y < SCREEN_HEIGHT; ++y)
        {
            plot_pixel(x, y, background[y * SCREEN_WIDTH + x]);
        }
    }
}

//helper function to draw any image
void draw_image(int x, int y, int color[], int width, int height){
    for (int i = 0; i < width; i++)
    {
        for (int j = 0; j < height; j++)
        {
            //location of the image drew on the screen
            int plot_x = x + i;
            int plot_y = y + j;

            //filter the transparent color and do not draw when out of screen
            if (color[j * width + i] == 0xf81f || plot_x >= SCREEN_WIDTH || plot_y >= SCREEN_HEIGHT || plot_x < 0 || plot_y < 0){
                continue;
            }

            plot_pixel(plot_x, plot_y, color[j * width + i]);
        }
        
    }
    
}

// code for subroutines (not shown)
void plot_pixel(int x, int y, short int line_color)
{
    *(short int *)(pixel_buffer_start + (y << 10) + (x << 1)) = line_color;
}

//function to clear whole screen
void clear_screen(){
    //set color for every pixel to black
    for (int x = 0; x <= 319; ++x){
        for (int y = 0; y <= 239; ++y){
            plot_pixel(x, y, 0x0);
        }
    }
}

//function to draw a line
void draw_line(int x0, int y0, int x1, int y1, short int line_color){
    bool isSteep = abs(y1 - y0) > abs(x1 - x0);
    if (isSteep){
        swap(&x0, &y0);
        swap(&x1, &y1);
    }
    if (x0 > x1){
        swap(&x0, &x1);
        swap(&y0, &y1);
    }

    int deltax = x1 - x0;
    int deltay = abs(y1 - y0);
    int error = -1 * (deltax * 0.5);
    int y = y0;
    int y_step;
    if (y0 < y1){
        y_step = 1;
    } else{
        y_step = -1;
    }

    for (int x = x0; x <= x1; ++x){
        if (isSteep){
            plot_pixel(y, x, line_color);
        } else{
            plot_pixel(x, y, line_color);
        }
        error = error + deltay;
        if (error >= 0){
            y = y + y_step;
            error = error - deltax;
        }
    }
}

//function to swap two values
void swap(int* a, int* b){
    int temp = *a;
    *a = *b;
    *b = temp;
}

//function to wait for Vsync
void wait_for_vsync(){
     volatile int *pixel_ctrl_ptr = (int *)0xFF203020;
     register int status;

     *pixel_ctrl_ptr = 1;

     status = *(pixel_ctrl_ptr + 3);
     while ((status & 0x01) != 0){
         status = *(pixel_ctrl_ptr + 3);
     }
}