#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include "image.h"
#include "interrupt.h"


/*Constant defines here*/
#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 240
#define LOWEST_Y 202
#define OUT_SCREEN -50

//speed of characters
#define BAD_MUSHROOM_SPEED 5
#define MARIO_RUN_SPEED 2
#define MARIO_JUMP_SPEED 1
#define MARIO_JUMP_HIGHT 100
#define GRAVITY_FALL 20

#define MARIO_MID 25 * 0.5

/*global variables defines here*/
volatile int pixel_buffer_start; 
volatile int character_buffer_start;

//Mario's position
int mario_x = 10;
int mario_y = LOWEST_Y - 25;

//whether Mario is moving
bool mario_move_forward = false;
bool mario_move_backward = false;
bool mario_jump = false;
bool mario_fall = false;
bool is_mario_moving_forward = true;
int mario_jumped = 0;

//location of steps in first background
int steps_1_low_x = 49;
int steps_1_high_x = 152;
int steps_1_y = 131;

//location of pipe in first background
int pipe_1_low_x = 202;
int pipe_1_high_x = 236;
int pipe_1_y = 162;

//up to three bad mushrooms
bool isBadMushroom[3] = {true, true, false};
int badMushroom_x[3] = {179, 152 - 19, OUT_SCREEN};
int badMushroom_y[3] = {LOWEST_Y - 19, 131 - 19, OUT_SCREEN};

//up to three moneys
bool isMoney[3] = {false, false, false};
int money_x[3] = {67, 118, OUT_SCREEN};
int money_y[3] = {112, 112, OUT_SCREEN};

//is the game over
bool isGameOver = false;
//total lives left
int lives = 1;

/*Function prototypes*/
void plot_pixel(int x, int y, short int line_color);
void clear_screen();
void draw_line(int x0, int y0, int x1, int y1, short int line_color);
void swap(int* a, int* b);
void wait_for_vsync();
void draw_background();
void draw_image();
void update_location();
void init_location();
void draw_main_canvas();
void bad_mushroom_update_location();
void mario_update_location();
void plot_digit(int x, int y, int ascii);
void beat_mushroom();


int main(void){
    disable_A9_interrupts ();	// disable interrupts in the A9 processor
	set_A9_IRQ_stack ();			// initialize the stack pointer for IRQ mode
	config_GIC ();					// configure the general interrupt controller
	config_KEYs ();				// configure pushbutton KEYs to generate interrupts

	enable_A9_interrupts ();	// enable interrupts in the A9 processor

    //draw things
    draw_main_canvas();
}

void draw_main_canvas(){
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
            //draw Mario depends on different movements
            if (mario_jump && is_mario_moving_forward){
                draw_image(mario_x, mario_y, Mario_jump, 26, 25);
            } else if (mario_jump && !is_mario_moving_forward){
                draw_image(mario_x, mario_y, Mario_jump_back, 26, 25);
            } else if (is_mario_moving_forward){
                draw_image(mario_x, mario_y, Mario_run, 25, 25);
            } else if (!is_mario_moving_forward){
                draw_image(mario_x, mario_y, Mario_run_back, 25, 25);
            } 
            
            //draw bad mushrooms
            for (int i = 0; i < 3; i++)
            {
                if (isBadMushroom[i]){
                    draw_image(badMushroom_x[i], badMushroom_y[i], bad_mushroom, 19, 19);
                } 
            }
            

            //draw moneys
            for (int i = 0; i < 3; i++)
            {   
                if (isMoney[i]){
                    draw_image(money_x[i], money_y[i], money, 16, 19);
                }               
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
    mario_update_location();
    bad_mushroom_update_location();

    //check whether the game is over
    if (lives <= 0){
        isGameOver = true;
    }
}


void mario_update_location(){
    //control mario depends on different flags
    if (mario_move_forward && (mario_x + MARIO_MID <= pipe_1_low_x || mario_x + MARIO_MID >= pipe_1_high_x || mario_y + 25 <= pipe_1_y)){
        mario_x += MARIO_RUN_SPEED;
        is_mario_moving_forward = true;
    } 
    if (mario_move_backward && (mario_x + MARIO_MID >= pipe_1_high_x || mario_x + MARIO_MID <= pipe_1_low_x || mario_y + 25 <= pipe_1_y)){
        mario_x -= MARIO_RUN_SPEED;
        is_mario_moving_forward = false;
    }
    if (mario_jump){
        //if under steps
        if (steps_1_low_x <= mario_x + MARIO_MID && mario_x + MARIO_MID <= steps_1_high_x && mario_y > steps_1_y){
            if (!mario_fall && mario_y > steps_1_y + 25){
                mario_y -= MARIO_JUMP_SPEED;
            } else{
                mario_fall = true;
                mario_jumped = 0;
                for (int i = 0; i < 3; i++)
                {
                    if (mario_x + MARIO_MID >= money_x[i] && mario_x + MARIO_MID <= money_x[i] + 16){
                        isMoney[i] = true;
                    }
                }
                
            }
        }else if (!mario_fall && mario_jumped < MARIO_JUMP_HIGHT){
            mario_y -= MARIO_JUMP_SPEED;
            mario_jumped += MARIO_JUMP_SPEED;
        } else{
            mario_fall = true;
            mario_jumped = 0;
        }

        if (mario_fall){
            beat_mushroom();
            mario_y += GRAVITY_FALL;
        }
    }
    //if not landing, cannot jump again
    //if on steps
    if (mario_fall && steps_1_low_x < mario_x + 20 && mario_x + 5 < steps_1_high_x && mario_y + 25 <= steps_1_y + 20){
        if (mario_y + 25 >= steps_1_y){
            mario_y = steps_1_y - 25;
            mario_jump = false;
            mario_fall = false;
        }
    }//if on the pipe 
    else if (mario_fall && pipe_1_low_x < mario_x + 20 && mario_x + 5 < pipe_1_high_x){
        if (mario_y + 25 >= pipe_1_y){
            mario_y = pipe_1_y - 25;
            mario_jump = false;
            mario_fall = false;
        }
    } else if (mario_fall){
        if (mario_y + 25 >= LOWEST_Y){
            mario_y = LOWEST_Y - 25;
            mario_jump = false;
            mario_fall = false;
        }
    }
    mario_move_backward = false;
    mario_move_forward = false;

    //gravity falling of Mario
    if (!mario_jump && (mario_x + 20 < steps_1_low_x || mario_x + 5 > steps_1_high_x) && (mario_x + 20 < pipe_1_low_x || mario_x + 5 > pipe_1_high_x) && mario_y + 25 < LOWEST_Y){
        beat_mushroom();
        mario_y += GRAVITY_FALL;
        if (mario_y + GRAVITY_FALL >= LOWEST_Y - 25){
            mario_y = LOWEST_Y - 25;
        }
    } 

    //whether Mario picks moneys
    for (int i = 0; i < 3; i++)
    {
        if (mario_x + MARIO_MID >= money_x[i] && mario_x + MARIO_MID <= money_x[i] + 16 && money_y[i] + 19 >= mario_y && mario_y + 25 >= money_y[i]){
            isMoney[i] = false;
            money_x[i] = OUT_SCREEN;
            money_y[i] = OUT_SCREEN;
        }
    }
}


void bad_mushroom_update_location(){
    //bad mushroom move to left automatically
    for (int i = 0; i < 3; i++)
    {
        if (isBadMushroom[i] && badMushroom_x[i] + 19 >= 0){
            badMushroom_x[i] -= BAD_MUSHROOM_SPEED;
        }

        //gravity falling of bad mush room
        if (isBadMushroom[i] && badMushroom_x[i] + 19 * 0.5 <= steps_1_low_x && badMushroom_y[i] + 19 < LOWEST_Y){
            badMushroom_y[i] += GRAVITY_FALL;
            if (badMushroom_y[i] + GRAVITY_FALL >= LOWEST_Y - 19){
                badMushroom_y[i] = LOWEST_Y - 19;
            }
        }

        //check whether mario dies
        if (isBadMushroom[i] && badMushroom_x[i] <= mario_x + MARIO_MID && badMushroom_x[i] + 19 >= mario_x + MARIO_MID  && badMushroom_y[i] + 19 >= mario_y && mario_y + 25 >= badMushroom_y[i]){
            lives--;
        }
    }
}

void beat_mushroom(){
    for (int i = 0; i < 3; i++)
    {
        if (isBadMushroom[i] && mario_x + 25 >= badMushroom_x[i] && mario_x <= badMushroom_x[i] + 19 && mario_y + 25 <= badMushroom_y[i]){
            if (mario_y + 25 + GRAVITY_FALL >= badMushroom_y[i]){
                isBadMushroom[i] = false;
                badMushroom_x[i] = OUT_SCREEN;
                badMushroom_y[i] = OUT_SCREEN;
            }
        }
    }
    return;
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
	