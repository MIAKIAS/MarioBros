volatile int pixel_buffer_start; // global variable
volatile int character_buffer_start;

void plot_pixel(int x, int y, short int line_color);
void clear_screen();
void draw_line(int x0, int y0, int x1, int y1, short int line_color);
void swap(int* a, int* b);
void wait_for_vsync();
void draw_background();
void draw_image();
void update_location();
void init_location();
void plot_digit(int x, int y, int ascii);

#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include "image.h"

#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 240
#define LOWEST_Y 202

//speed of characters
#define BAD_MUSHROOM_SPEED 5
#define MARIO_RUN_SPEED 2
#define GRAVITY_FALL 20

//Mario's position
int mario_x = 10;
int mario_y = LOWEST_Y - 25;



//location of steps in first background
int steps_1_low_x = 49;
int steps_1_high_x = 152;
int steps_1_y = 131;

//location of pipe in first background
int pipe_1_low_x = 202;
int pipe_1_high_x = 236;
int pipe_1_y = 217;

//up to three bad mushrooms
bool isBadMushroom[3] = {true, true, false};
int badMushroom_x[3] = {179, 152 - 19, -1};
int badMushroom_y[3] = {LOWEST_Y - 19, 131 - 19, -1};

//is the game over
bool isGameOver = false;
//total lives left
int lives = 1;

int main(void)
{
    volatile int * pixel_ctrl_ptr = (int *)0xFF203020;
    volatile int * character_ctrl_ptr = (int *)0xFF203030;
    // declare other variables(not shown)
    // initialize location and direction of rectangles(not shown)

    /* set front pixel buffer to start of FPGA On-chip memory */
    *(pixel_ctrl_ptr + 1) = 0xC8000000; // first store the address in the 
                                        // back buffer
    //*(character_ctrl_ptr + 1) = 0xC9000000;                                    
    /* now, swap the front/back buffers, to set the front buffer location */
    wait_for_vsync();
    /* initialize a pointer to the pixel buffer, used by drawing functions */
    pixel_buffer_start = *pixel_ctrl_ptr;
    character_buffer_start = *character_ctrl_ptr;

    clear_screen(); // pixel_buffer_start points to the pixel buffer
    /* set back pixel buffer to start of SDRAM memory */
    *(pixel_ctrl_ptr + 1) = 0xC0000000;
    //*(character_ctrl_ptr + 1) = 0xC9000000; 

    pixel_buffer_start = *(pixel_ctrl_ptr + 1); // we draw on the back buffer
    //character_buffer_start = *(character_ctrl_ptr + 1);
	character_buffer_start = 0xC9000000; 
	
    while (1)
    {
        /* Erase any boxes and lines that were drawn in the last iteration */
        clear_screen();

        // code for drawing the boxes and lines 
        draw_background();
		
		plot_digit(60, 2, 0x53); //S
        plot_digit(62, 2, 0x43); //C
		plot_digit(64, 2, 0x4F); //O	
		plot_digit(66, 2, 0x52); //R
		plot_digit(68, 2, 0x45); //E
		plot_digit(70, 2, 0x3A); // :
        plot_digit(72, 2, 0x30); //0
		plot_digit(74, 2, 0x30); //0
		
        //check whether the game is over
        if (!isGameOver){
            draw_image(mario_x, mario_y, Mario_stand, 19, 25);
            if (isBadMushroom[0]){
                draw_image(badMushroom_x[0], badMushroom_y[0], bad_mushroom, 19, 19);
            } 
            if (isBadMushroom[1]){
                draw_image(badMushroom_x[1], badMushroom_y[1], bad_mushroom, 19, 19);
            }
            if (isBadMushroom[2]){
                draw_image(badMushroom_x[2], badMushroom_y[2], bad_mushroom, 19, 19);
            }
        }

        
        // code for updating the locations of boxes
        //update_location();
        update_location();
        wait_for_vsync(); // swap front and back buffers on VGA vertical sync
        pixel_buffer_start = *(pixel_ctrl_ptr + 1); // new back buffer
    }
}

//update locations for all characters
void update_location(){
    //bad mushroom move to left automatically
    for (int i = 0; i < 3; i++)
    {
        if (isBadMushroom[i] && badMushroom_x[i] + 19 >= 0){
            badMushroom_x[i] -= BAD_MUSHROOM_SPEED;
        }

        //gravity falling of bad mush room
        if (badMushroom_x[i] + 19 * 0.5 <= steps_1_low_x && badMushroom_y[i] + 19 < LOWEST_Y){
            badMushroom_y[i] += GRAVITY_FALL;
            if (badMushroom_y[i] + GRAVITY_FALL >= LOWEST_Y - 19){
                badMushroom_y[i] = LOWEST_Y - 19;
            }
        }

        //check whether mario dies
        if (badMushroom_x[i] <= mario_x + 19 && badMushroom_x[i] + BAD_MUSHROOM_SPEED >= mario_x + 19  && badMushroom_y[i] + 8 > mario_x){
            lives--;
        }

        //check whether the game is over
        if (lives <= 0){
            isGameOver = true;
        }
    }

    
    
    
}

//helper function to draw any background
void draw_background(){
    for (int x = 0; x < SCREEN_WIDTH; ++x)
    {
        for (int y = 0; y < SCREEN_HEIGHT; ++y)
        {
            if (isGameOver){ //check whether the game is over
                plot_pixel(x, y, game_over[y * SCREEN_WIDTH + x]);
            } else{
                plot_pixel(x, y, background[y * SCREEN_WIDTH + x]);
            }
            
        }
    }
}

//helper function to draw any image
//x,y as the coordinate of right top conor
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

void plot_digit(int x, int y, int ascii){
    *(short int *)(character_buffer_start + (y << 7) + x) = ascii;
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
	