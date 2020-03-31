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
#define ENDING 254

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

//which map 
int map_num = 1;

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

int steps_2_L_low_x = 65;
int steps_2_L_high_x = 130;
int steps_2_L_y = 93;

int steps_2_R_low_x = 170;
int steps_2_R_high_x = 187;
int steps_2_R_y = 132;

//location of pipe in first background
int pipe_1_low_x = 202;
int pipe_1_high_x = 236;
int pipe_1_y = 162;

int pipe_2_L_low_x = 43;
int pipe_2_L_high_x = 77;
int pipe_2_L_y = 162;

int pipe_2_R_low_x = 217;
int pipe_2_R_high_x = 250;
int pipe_2_R_y = 133;

int pipe_3_low_x = 43;
int pipe_3_high_x = 76;
int pipe_3_y = 140;

//up to three bad mushrooms
bool isBadMushroom[3] = {true, true, false};
int badMushroom_x[3] = {179, 152 - 19, OUT_SCREEN};
int badMushroom_y[3] = {LOWEST_Y - 19, 131 - 19, OUT_SCREEN};
bool isBadMushroomMovingRight[3] = {false, false, false};

//up to three moneys
bool isMoney[3] = {false, false, false};
int money_x[3] = {67, OUT_SCREEN, OUT_SCREEN};
int money_y[3] = {112, OUT_SCREEN, OUT_SCREEN};

//Good mushroom
bool isGoodMushroom = false;
int goodMushroom_x = 118;
int goodMushroom_y = 112;

//turtle
bool isTurtle = false;
bool isTurtleMovingRight = true;
int turtle_x = 43;
int turtle_y = 93 - 28;


//is the game over
bool isGameOver = false;
bool isWin = false;
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
void reset_characters();
void turtle_update_location();
void beat_turtle();

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
        if (!isGameOver && !isWin){
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
            
            if (isGoodMushroom){
                draw_image(goodMushroom_x, goodMushroom_y, good_mushroom, 19, 19);
            }

            if (isTurtle){
                if (isTurtleMovingRight){
                    draw_image(turtle_x, turtle_y, turtle_right, 19, 28);
                }else{
                    draw_image(turtle_x, turtle_y, turtle, 19, 28);
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
    turtle_update_location();
    //check whether the game is over
    if (lives <= 0){
        isGameOver = true;
    }
}


void mario_update_location(){
    if (map_num == 1){
        //control mario depends on different flags
        if (mario_move_forward && (mario_x + 25 <= pipe_1_low_x || mario_x > pipe_1_low_x || mario_y + 25 <= pipe_1_y)){
            mario_x += MARIO_RUN_SPEED;
            is_mario_moving_forward = true;
        } 
        if (mario_move_backward && (mario_x >= pipe_1_high_x || mario_x < pipe_1_low_x || mario_y + 25 <= pipe_1_y)){
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
                    if (mario_x + MARIO_MID >= goodMushroom_x && mario_x + MARIO_MID <= goodMushroom_x + 19){
                        isGoodMushroom = true;
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

        //gravitational falling of Mario
        if (!mario_jump && (mario_x + 20 < steps_1_low_x || mario_x + 5 > steps_1_high_x) && (mario_x + 20 < pipe_1_low_x || mario_x + 5 > pipe_1_high_x) && mario_y + 25 < LOWEST_Y){
            beat_mushroom();
            mario_y += GRAVITY_FALL;
            if (mario_y + GRAVITY_FALL >= LOWEST_Y - 25){
                mario_y = LOWEST_Y - 25;
            }
        } 
    } else if (map_num == 2){
        //control mario depends on different flags
        if (mario_move_forward && (mario_x + 25 <= pipe_2_L_low_x || mario_x > pipe_2_L_low_x || mario_y + 25 <= pipe_2_L_y)
                               && (mario_x + 25 <= pipe_2_R_low_x || mario_x > pipe_2_R_low_x || mario_y + 25 <= pipe_2_R_y)){
            mario_x += MARIO_RUN_SPEED;
            is_mario_moving_forward = true;
        } 
        if (mario_move_backward && (mario_x >= pipe_2_L_high_x || mario_x < pipe_2_L_high_x || mario_y + 25 <= pipe_2_L_y)
                                && (mario_x >= pipe_2_R_high_x || mario_x < pipe_2_R_high_x || mario_y + 25 <= pipe_2_R_y)){
            mario_x -= MARIO_RUN_SPEED;
            is_mario_moving_forward = false;
        }
        if (mario_jump){
            //if under left steps
            if (steps_2_L_low_x <= mario_x + MARIO_MID && mario_x + MARIO_MID <= steps_2_L_high_x && mario_y > steps_2_L_y){
                if (!mario_fall && mario_y > steps_2_L_y + 25){
                    mario_y -= MARIO_JUMP_SPEED;
                } else{
                    mario_fall = true;
                    mario_jumped = 0;
                }
            } else if (steps_2_R_low_x <= mario_x + MARIO_MID && mario_x + MARIO_MID <= steps_2_R_high_x && mario_y > steps_2_R_y){ //if under right step
                if (!mario_fall && mario_y > steps_2_R_y + 25){
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
                beat_turtle();
                mario_y += GRAVITY_FALL;
            }
        }
        //if not landing, cannot jump again
        //if on left steps
        if (mario_fall && steps_2_L_low_x < mario_x + 20 && mario_x + 5 < steps_2_L_high_x && mario_y + 25 <= steps_2_L_y + 20){
            if (mario_y + 25 >= steps_2_L_y){
                mario_y = steps_2_L_y - 25;
                mario_jump = false;
                mario_fall = false;
            }
        } else if (mario_fall && steps_2_R_low_x < mario_x + 20 && mario_x + 5 < steps_2_R_high_x && mario_y + 25 <= steps_2_R_y + 20){ // on right step
            if (mario_y + 25 >= steps_2_R_y){
                mario_y = steps_2_R_y - 25;
                mario_jump = false;
                mario_fall = false;
            }
        } else if (mario_fall && pipe_2_L_low_x < mario_x + 20 && mario_x + 5 < pipe_2_L_high_x){//if on left pipe 
            if (mario_y + 25 >= pipe_2_L_y){
                mario_y = pipe_2_L_y - 25;
                mario_jump = false;
                mario_fall = false;
            }
        } else if (mario_fall && pipe_2_R_low_x < mario_x + 20 && mario_x + 5 < pipe_2_R_high_x){//if on right pipe 
            if (mario_y + 25 >= pipe_2_R_y){
                mario_y = pipe_2_R_y - 25;
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

        //gravitational falling of Mario
        if (!mario_jump && (mario_x + 20 < steps_2_L_low_x || mario_x + 5 > steps_2_L_high_x) 
                        && (mario_x + 20 < steps_2_R_low_x || mario_x + 5 > steps_2_R_high_x) 
                        && (mario_x + 20 < pipe_2_L_low_x || mario_x + 5 > pipe_2_L_high_x) 
                        && (mario_x + 20 < pipe_2_R_low_x || mario_x + 5 > pipe_2_R_high_x) 
                        && mario_y + 25 < LOWEST_Y){
            beat_mushroom();
            beat_turtle();
            mario_y += GRAVITY_FALL;
            if (mario_y + GRAVITY_FALL >= LOWEST_Y - 25){
                mario_y = LOWEST_Y - 25;
            }
        } 
    } else if (map_num == 3){
        //control mario depends on different flags
        if (mario_move_forward && (mario_x + 25 <= pipe_3_low_x || mario_x > pipe_3_low_x || mario_y + 25 <= pipe_3_y)){
            mario_x += MARIO_RUN_SPEED;
            is_mario_moving_forward = true;
        } 
        if (mario_move_backward && (mario_x >= pipe_3_high_x || mario_x < pipe_3_high_x || mario_y + 25 <= pipe_3_y)){
            mario_x -= MARIO_RUN_SPEED;
            is_mario_moving_forward = false;
        }
        if (mario_jump){
            if (!mario_fall && mario_jumped < MARIO_JUMP_HIGHT){
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
        if (mario_fall && pipe_3_low_x < mario_x + 20 && mario_x + 5 < pipe_3_high_x){//if on right pipe 
            if (mario_y + 25 >= pipe_3_y){
                mario_y = pipe_3_y - 25;
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

        //gravitational falling of Mario
        if (!mario_jump && (mario_x + 20 < pipe_3_low_x || mario_x + 5 > pipe_3_high_x) 
                        && mario_y + 25 < LOWEST_Y){
            beat_mushroom();
            mario_y += GRAVITY_FALL;
            if (mario_y + GRAVITY_FALL >= LOWEST_Y - 25){
                mario_y = LOWEST_Y - 25;
            }
        } 
    }
    
    if (!isGameOver && !isWin){
        //whether Mario picks moneys
        for (int i = 0; i < 3; i++)
        {
            if (mario_x + MARIO_MID >= money_x[i] && mario_x + MARIO_MID <= money_x[i] + 16 && money_y[i] + 19 >= mario_y && mario_y + 25 >= money_y[i]){
                isMoney[i] = false;
                money_x[i] = OUT_SCREEN;
                money_y[i] = OUT_SCREEN;
            }
        }
        //whether Mario picks Good Mushroom
        if (mario_x + MARIO_MID >= goodMushroom_x && mario_x + MARIO_MID <= goodMushroom_x + 19 && goodMushroom_y + 19 >= mario_y && mario_y + 25 >= goodMushroom_y){
            isGoodMushroom = false;
            goodMushroom_x = OUT_SCREEN;
            goodMushroom_y = OUT_SCREEN;
            lives++;
        }
    }
    

    //whether Mario go to next map
    if (map_num != 3 && mario_x + MARIO_MID >= SCREEN_WIDTH){
        map_num++;
        reset_characters();
        //reset Mario's position
        mario_x = 10;
        mario_y = LOWEST_Y - 25;
    } else if (map_num == 3 && mario_x + MARIO_MID >= ENDING){
        isWin = true;
    }
}

void reset_characters(){
    for (int i = 0; i < 3; i++)
    {
        isMoney[i] = false;
    }

    if (map_num == 2){
        for (int i = 0; i < 3; i++)
        {
            isBadMushroom[i] = true;
            badMushroom_y[i] = LOWEST_Y - 19;            
        }
        
        isTurtle = true;

        badMushroom_x[0] = 196;
        badMushroom_x[1] = 152;
        badMushroom_x[2] = 100;

        money_x[0] = steps_2_R_low_x;
        money_y[0] = 113;
        money_x[1] = money_x[2] = money_y[1] = money_y[2] = OUT_SCREEN;
        
    } else if (map_num == 3){
        isBadMushroom[0] = true;
        isBadMushroom[1] = false;
        isBadMushroom[2] = false;

        badMushroom_x[0] = 96;
        badMushroom_y[0] = LOWEST_Y - 19; 
    }
}

void turtle_update_location(){
    if (isTurtle){
        //bouncing 
        if (turtle_x <= steps_2_L_low_x){
            isTurtleMovingRight = true;
        } else if (turtle_x + 19 >= steps_2_L_high_x){
            isTurtleMovingRight = false;
        }

        if (isTurtleMovingRight){
            turtle_x += BAD_MUSHROOM_SPEED;
        } else{
            turtle_x -= BAD_MUSHROOM_SPEED;
        }

        //check whether mario dies
        if (turtle_x <= mario_x + MARIO_MID && turtle_x + 19 >= mario_x + MARIO_MID  && turtle_y + 28 >= mario_y && mario_y + 25 >= turtle_y){
            lives--;
        }
    }
    
}

void beat_turtle(){

    if (isTurtle && mario_x + 25 >= turtle_x && mario_x <= turtle_x + 19 && mario_y + 25 <= turtle_y){
        if (mario_y + 25 + GRAVITY_FALL >= turtle_y){
            isTurtle = false;
            turtle_x = OUT_SCREEN;
            turtle_y = OUT_SCREEN;
            for (int i = 0; i < 3; i++)
            {
                isBadMushroom[i] = false;
            }
            
        }
    }

    return;
}

void bad_mushroom_update_location(){
    //bad mushroom move to left automatically
    for (int i = 0; i < 3; i++)
    {
        if (map_num == 1){
            if (isBadMushroom[i] && badMushroom_x[i] + 19 >= 0){
                badMushroom_x[i] -= BAD_MUSHROOM_SPEED;
            }

            //gravitational falling of bad mush room
            if (isBadMushroom[i] && badMushroom_x[i] + 19 * 0.5 <= steps_1_low_x && badMushroom_y[i] + 19 < LOWEST_Y){
                badMushroom_y[i] += GRAVITY_FALL;
                if (badMushroom_y[i] + GRAVITY_FALL >= LOWEST_Y - 19){
                    badMushroom_y[i] = LOWEST_Y - 19;
                }
            }
        } else if (map_num == 2){
            //bouncing between two pipes
            if (badMushroom_x[i] <= pipe_2_L_high_x){
                isBadMushroomMovingRight[i] = true;
            } else if (badMushroom_x[i] + 19 >= pipe_2_R_low_x){
                isBadMushroomMovingRight[i] = false;
            }

            if (isBadMushroomMovingRight[i]){
                badMushroom_x[i] += BAD_MUSHROOM_SPEED;
            } else{
                badMushroom_x[i] -= BAD_MUSHROOM_SPEED;
            }

        } else{
            //bouncing 
            if (badMushroom_x[i] <= pipe_3_high_x){
                isBadMushroomMovingRight[i] = true;
            } else if (badMushroom_x[i] + 19 >= 224){
                isBadMushroomMovingRight[i] = false;
            }

            if (isBadMushroomMovingRight[i]){
                badMushroom_x[i] += BAD_MUSHROOM_SPEED;
            } else{
                badMushroom_x[i] -= BAD_MUSHROOM_SPEED;
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
        for (int y = 0; y <= SCREEN_HEIGHT; ++y)
        {
            if (isGameOver){ //check whether the game is over
                plot_pixel(x, y, game_over[y * SCREEN_WIDTH + x]);
            } else if (isWin){
                plot_pixel(x, y, win[y * SCREEN_WIDTH + x]);
            } else{    
                if (map_num != 1 && y >= 203){ //to speed up drawing
                    break;
                }           
                if (map_num == 1)
                    plot_pixel(x, y, background[y * SCREEN_WIDTH + x]);
                else if (map_num == 2)
                    plot_pixel(x, y, background2[y * SCREEN_WIDTH + x]);
                else if (map_num == 3)
                    plot_pixel(x, y, end[y * SCREEN_WIDTH + x]);
                else
                    plot_pixel(x, y, 0);
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
	