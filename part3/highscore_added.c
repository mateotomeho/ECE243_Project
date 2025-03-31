//Part3 of Project ECE243: Towers of Hanoi

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h> //to use randomness ran() function
#include <string.h>

#define LEDR_BASE			0xFF200000
#define SW_BASE				0xFF200040
#define KEY_BASE			0xFF200050
#define TIMER_BASE			0xFF202000
#define AUDIO_BASE			0xFF203040
#define PS2_BASE			0xFF200100
#define HEX3_HEX0_BASE	    0xFF200020
#define HEX5_HEX4_BASE		0xFF200030
#define COUNTER_DELAY 100000000 //number that we'll count simce 100MHz



volatile int pixel_buffer_start; // global variable
short int Buffer1[240][512]; // 240 rows, 512 (320 + padding) columns
short int Buffer2[240][512];
/////////////////////////////////////////////////////////////////////////////////////////
//Struct for the timer registers
struct timer_t {
       volatile unsigned int status;
       volatile unsigned int control;
       volatile unsigned int periodlo;
       volatile unsigned int periodhi;
       volatile unsigned int snaplo;
       volatile unsigned int snaphi;
};

struct timer_t * const timer = (struct timer_t *) TIMER_BASE;
int time = 90;

/////////////////////////////////////////////////////////////////////////////////////////
//Struct for the audio
struct audio_t {
	volatile unsigned int control;  // The control/status register
	volatile unsigned char rarc;	// the 8 bit RARC register
	volatile unsigned char ralc;	// the 8 bit RALC register
	volatile unsigned char wsrc;	// the 8 bit WSRC register
	volatile unsigned char wslc;	// the 8 bit WSLC register
    volatile unsigned int ldata;	// the 32 bit (really 24) left data register
	volatile unsigned int rdata;	// the 32 bit (really 24) right data register
};
struct audio_t *const audiop = ((struct audio_t *)AUDIO_BASE);

/*
int victory[39010] = {
    0x00000000};
int numVictory = 39010;

int sad[36966] = {
    0x00000000};
int numSad = 36966;
*/

/////////////////////////////////////////////////////////////////////////////////////////
//Struct for each disks : store location (x,y), direction (x, y), colour
struct disk_info {
    int x; //Location of each disk in x
    int y;  //Location of each disk in y
    int dx; //Direction in x of disk
    int dy; //Direction in y of disk
    short int colour; //Colour of each box
    int x_old1; //OLD Location of each disk in x BUFFER 1
    int y_old1; //OLD Location of each disk in y BUFFER 1
    int x_old2; //OLD Location of each disk in x BUFFER 2
    int y_old2; //OLD Location of each disk in y BUFFER 2
    int size; //Size of the disk
    int column; //The column where the disk is located: (0, 1, 2)
};

//Easy mode column
int column0[3] = {0};
int column1[3] = {0};
int column2[3] = {0};

//Medium mode column
int column0_medium[4] = {0};
int column1_medium[4] = {0};
int column2_medium[4] = {0};

//Hard mode column
int column0_hard[5] = {0};
int column1_hard[5] = {0};
int column2_hard[5] = {0};

int rod_positions[3] = {55, 159, 263};

int N = 3; //number of Disks
int num_move = 0; //number of Moves

//Keep track of the best move of the user
int best_move_easy = 0; 
int best_move_medium = 0;
int best_move_hard = 0;

//Boolean for winning or losing
bool winning = false;
bool losing = false;
bool start_screen = true;
bool end_screen = false;
bool restart = false;
int once = 0;


/////////////////////////////////////////////////////////////////////////////////////////
//Function declaration
void plot_pixel(int x, int y, short int line_color);
void clear_screen();
void wait_for_vsync();
void swap(int *a, int *b);
void draw_line(int x0, int y0, int x1, int y1, short int color);
void update_direction(struct disk_info disks[]);
void draw_disk(struct disk_info disk);
void drawBars();
void draw(struct disk_info disks[], volatile int *KEY_ptr, volatile int *SW_ptr );
void update_disk_position(struct disk_info disks[], int index);
void direction_rods(struct disk_info disks[], int index, int direction);
bool add_disk_column(struct disk_info disk, int direction);
void delete_disk_column(struct disk_info disk);
void read_keyboard(unsigned char *pressedKey);
int num_move_tracker(int num_move);
void display_hex_10(int num);
void display_hex_54(int num);
void drawLetter(int x, int y, char c, short int color);
void draw_text(int x, int y, const char *s, short int color);
void draw_start_screen();
void restart_game(struct disk_info disks[]);
void best_move_tracker(struct disk_info disks[]);
void setup_timer();
bool delay_sec();
void no_more_time();
void play_audio(int *samples, int numSamples);
void draw_end_screen();


int main(void){
    volatile int * pixel_ctrl_ptr = (int *)0xFF203020;
    volatile int * ps2_ptr = (volatile int *) PS2_BASE;
    volatile int * KEY_ptr = (volatile int *) KEY_BASE;
	volatile int * KEY_edge_ptr = (volatile int *)(KEY_BASE + 0xC);
    volatile int * SW_ptr = (volatile int *) SW_BASE;
	
	*KEY_edge_ptr = 0xF; //make sure edge capture is turned off

    int mode =0;
	N=3;

    struct disk_info disks[5]; //Create an array of disks of struct disk_info
	
                                  //White,  Red,   Green, Blue,  Cyan,   Magenta, Yellow, Orange, Pink, 
    short int colour_array[9] = {0xFFFF, 0xF800, 0x07E0, 0x001F, 0x07FF, 0xF81F, 0xFFE0, 0xFC60, 0xF81F}; 
  
     // initialize location and direction of rectangles(not shown)
    for (int i = 0; i < N; i++) {
    	disks[i].dx = (((rand() % 2) * 2) - 1); //generate a random value: -1 or 1
    	disks[i].dy = (((rand() % 2) * 2) - 1); //generate a random value: -1 or 1
    	//disks[i].colour = colour_array[rand() % 9];   //Create a random initial colour for the boxes
    	disks[i].x_old1 = 0;
    	disks[i].y_old1 = 0;
    	disks[i].x_old2 = 0;
    	disks[i].y_old2 = 0;
    	disks[i].column = 0;
    }

	//Intialize the size on easy mode
    disks[0].size = 50;  // Small (Blue)
    disks[1].size = 70;  // Medium (Green)
    disks[2].size = 90;  // Large (Red)

    //Initialize the colour
    disks[0].colour = colour_array[4];  // Small (Blue)
    disks[1].colour = colour_array[2];  // Medium (Green)
    disks[2].colour = colour_array[1];  // Large (Red)

    //Initialize the column
    for (int i = 0; i < N; i++) {
        column0[i] = disks[i].size;
    }

	//Intialize the location of the disks
    for (int i = 0; i < N; i++) {
        //Small at (30,10), Medium at (20,50), Large at (10,90)
        //Remember each rectangle is 20 pixels in the y-axis
        //disks[i].x = 30 - i*10;  
		disks[i].x = rod_positions[0] - ((disks[i].size) / 2);
        disks[i].y = 20 + i*40;
        //printf("Disk %d: x = %d, y = %d\n", i, disks[i].x, disks[i].y);
    }
	
	//Set up timer
    setup_timer();

    /* set front pixel buffer to Buffer 1 */
    *(pixel_ctrl_ptr + 1) = (int) &Buffer1; // first store the address in the  back buffer
    /* now, swap the front/back buffers, to set the front buffer location */
    wait_for_vsync();
    /* initialize a pointer to the pixel buffer, used by drawing functions */
    pixel_buffer_start = *pixel_ctrl_ptr;
    clear_screen(); // pixel_buffer_start points to the pixel buffer

    /* set back pixel buffer to Buffer 2 */
    *(pixel_ctrl_ptr + 1) = (int) &Buffer2;
    pixel_buffer_start = *(pixel_ctrl_ptr + 1); // we draw on the back buffer
    clear_screen(); // pixel_buffer_start points to the pixel buffer
	
    while (1)
    {
		if (start_screen) {
        	clear_screen();
        	draw_start_screen();
        	wait_for_vsync();
        	pixel_buffer_start = *(pixel_ctrl_ptr + 1);
			
			int ps2_data = *ps2_ptr;
			if (ps2_data & 0x8000){
				unsigned char key = ps2_data & 0xFF;
				
				if (key == 0x24){ //Check if user press e
					N=3;
					mode=0;
					num_move = 0;
					winning = false;
    				losing = false;
    				once = 0;
					
					disks[0].size = 50;  // Small (Blue)
        			disks[1].size = 70;  // Medium (Green)
        			disks[2].size = 90;  // Large (Red)

        			//Initialize the colour
        			disks[0].colour = colour_array[4];  // Small (Blue)
        			disks[1].colour = colour_array[2];  // Medium (Green)
        			disks[2].colour = colour_array[1];  // Large (Red)
					
					// Reset columns
                    for (int i = 0; i < 5; i++){
                        column0[i] = column1[i] = column2[i] = 0;
                        column0_medium[i] = column1_medium[i] = column2_medium[i] = 0;
                        column0_hard[i] = column1_hard[i] = column2_hard[i] = 0;
                    }
					
                    for (int i = 0; i < N; i++){
                        column0[i] = disks[i].size;
                    }
					
                    // Reset positions
                    for (int i = 0; i < N; i++){
                        disks[i].x = rod_positions[0] - ((disks[i].size) / 2);
                        disks[i].y = 20 + i*40;
                        disks[i].column = 0;
                    }

                	start_screen = false;
                	clear_screen();
					
            	}else if (key == 0x3A){ //Check if user press m
					N=4;
					mode=1;
					num_move = 0;
					winning = false;
    				losing = false;
    				once = 0;
					
					disks[0].size = 30;  // Very Small (Yellow)
       		 		disks[1].size = 50;  // Small (Blue)
        			disks[2].size = 70;  // Medium (Green)
        			disks[3].size = 90;  // Large (Red)

        			//Initialize the colour
        			disks[0].colour = colour_array[6];  // Very Small (Yellow)
        			disks[1].colour = colour_array[4];  // Small (Blue)
        			disks[2].colour = colour_array[2];  // Medium (Green)
        			disks[3].colour = colour_array[1];  // Large (Red)
				
					// Reset columns
                	for (int i = 0; i < 5; i++){
                    	column0[i] = column1[i] = column2[i] = 0;
                    	column0_medium[i] = column1_medium[i] = column2_medium[i] = 0;
                    	column0_hard[i] = column1_hard[i] = column2_hard[i] = 0;
                	}
				
                	for (int i = 0; i < N; i++){
                    	column0_medium[i] = disks[i].size;
                	}
				
                	// Reset positions
                	for (int i = 0; i < N; i++){
                    	disks[i].x = rod_positions[0] - ((disks[i].size) / 2);
                    	disks[i].y = 20 + i*40;
                    	disks[i].column = 0;
                	}
					
                	start_screen = false;
                	clear_screen();
                	continue;
				}else if (key == 0x33){ //Check if user press h
					N=5;
					mode = 2;
					num_move = 0;
					winning = false;
    				losing = false;
    				once = 0;
					
					//Intialize the size
        			disks[0].size = 20;  // Extra Small (Pink)
        			disks[1].size = 30;  // Very Small (Yellow)
        			disks[2].size = 50;  // Small (Blue)
        			disks[3].size = 70;  // Medium (Green)
        			disks[4].size = 90;  // Large (Red)

        			//Initialize the colour
        			disks[0].colour = colour_array[8];  // Extra Small (Pink)
        			disks[1].colour = colour_array[6];  // Very Small (Yellow)
        			disks[2].colour = colour_array[4];  // Small (Blue)
        			disks[3].colour = colour_array[2];  // Medium (Green)
        			disks[4].colour = colour_array[1];  // Large (Red)
					
					// Reset columns
                    for (int i = 0; i < 5; i++){
                        column0[i] = column1[i] = column2[i] = 0;
                        column0_medium[i] = column1_medium[i] = column2_medium[i] = 0;
                        column0_hard[i] = column1_hard[i] = column2_hard[i] = 0;
                    }
					
                    for (int i = 0; i < N; i++){
                        column0_hard[i] = disks[i].size;
                    }
					
                    // Reset positions
                    for (int i = 0; i < N; i++){
                        disks[i].x = rod_positions[0] - ((disks[i].size) / 2);
                        disks[i].y = 20 + i*40;
                        disks[i].column = 0;
                    }
				
                	start_screen = false;
                	clear_screen();
				}
			}
			continue;
		}else if (end_screen){
			clear_screen();
            draw_end_screen();
            wait_for_vsync();
            pixel_buffer_start = *(pixel_ctrl_ptr + 1);
			
			unsigned char restart_input = 0;
			read_keyboard(&restart_input);
			
			if(restart_input == 0x2D){
				end_screen = false;
            	start_screen = true;
            	restart = true;
            	restart_game(disks);
        	}
			continue;
		}else{
		
        	/* Erase any disk and lines that were drawn in the last iteration */
			draw(disks, KEY_ptr, SW_ptr);
		
        	// code for drawing the disk and lines (not shown)
        	// code for updating the locations of disk (not shown)

         	//Verify if we need to restart the game
        	restart_game(disks);

        	wait_for_vsync(); // swap front and back buffers on VGA vertical sync
        
        	//If the game is done wait until restart
        	if (once == 1){
            	while(restart == false){
            	restart_game(disks);
            	}
        	} 
        	pixel_buffer_start = *(pixel_ctrl_ptr + 1); // new back buffer
		
		/*
        //Music for winning or losing
        if (once == 0){
            if (N == 3){
                if ((winning && disks[0].y >= 160)|| losing){
                    if (winning){
                        play_audio(victory, numVictory);
                        winning = false;
                    } else if (losing){
                        play_audio(sad, numSad);
                        losing = false;
                    }
                    once = 1;
                }   
            } else if (N == 4){
                if ((winning && disks[0].y >= 140)|| losing){
                    if (winning){
                        play_audio(victory, numVictory);
                        winning = false;
                    } else if (losing){
                        play_audio(sad, numSad);
                        losing = false;
                    }
                    once = 1;
                }                 
            } else if (N == 5){
                if ((winning && disks[0].y >= 120)|| losing){
                    if (winning){
                        play_audio(victory, numVictory);
                        winning = false;
                    } else if (losing){
                        play_audio(sad, numSad);
                        losing = false;
                    }
                    once = 1;
                } 
            }  
        }*/
		}
    }
}


void draw(struct disk_info disks[], volatile int *KEY_ptr, volatile int *SW_ptr) {
    // should erase the previous content of the back buffer, two possible ways:
    //draw black pixels everywhere (slow)‣
    //METHOD 1:
    clear_screen();
    //faster, but harder: erase only what you drew - 2 frames ago!
    //METHOD 2:
    //Erase the old location of boxes:

    ///////////DRAW VGA///////////
    //Draw the 3 rods for the game
	drawBars();
	
	//draw the text for during game
	int title = strlen("tower of hanoi") * 10;
	int start_Xcoord = (320 - title) / 2;
	draw_text(start_Xcoord, 20, "tower of hanoi", 0xFFFF);
	
	int score = strlen("SCORE:") * 10;
	int score_Xcoord = (320 - (score+20))/2;
	draw_text(score_Xcoord, 60, "SCORE:", 0xFFFF);
	//draw the two‑digit move count
	int tens = num_move/10;
	int ones = num_move%10;
	drawLetter(score_Xcoord+score+2, 60,'0'+tens, 0xFFFF);
	drawLetter(score_Xcoord+score+12, 60,'0'+ones, 0xFFFF);
	
	//draw the minimum score achievable
	int min_score = strlen("MINIMUM SCORE:07") * 10;
	int min_score_x = (320 - min_score)/2;
	
	if (N == 3){ //easy
		draw_text(min_score_x, 40, "MINIMUM SCORE:07", 0xFFFF);
	}else if (N == 4){ //med
    	draw_text(min_score_x, 40, "MINIMUM SCORE:15", 0xFFFF);
	}else{ //hard
    	draw_text(min_score_x, 40, "MINIMUM SCORE:31", 0xFFFF);
	}
	
	//draw arrows for user instructions
	draw_text(51, 90, "#", 0xFFFF); //left arrow
	draw_text(155, 90, "$", 0xFFFF); //down arrow
	draw_text(259, 90, "@", 0xFFFF); //right arrow
	
    //draw each disk
    for (int index = 0; index < N; index++){
        draw_disk(disks[index]);
    }

    ///////////GAME LOGIC///////////
    //update_direction(disks) by creating the gravity affect
    for (int index = 0; index < N; index++){
        update_disk_position(disks, index);
    }

    //Use the PS/2 Keyboard to control the disks 
    unsigned char key_input = 0;
    int bit_3210 = 0;
    int SW_value = 0;

    //SELECT THE DISK TO MOVE//
    //Read key_input from the keyboard
    /*
    read_keyboard(&key_input);
    if (key_input == 0x16) { // Check if '1' is pressed
        SW_value = 0b00001;
    } else if (key_input == 0x1E) { // Check if '2' is pressed
        SW_value = 0b00010;
    } else if (key_input == 0x26) { // Check if '3' is pressed
        SW_value = 0b00100;
    } else if (key_input == 0x25) { // Check if '4' is pressed
        SW_value = 0b01000;
    } else if (key_input == 0x2E) { // Check if '5' is pressed
        SW_value = 0b10000;
    }*/
    //SELECT THE DIRECTION TO MOVE//
    //The key_input from the keyboard
    read_keyboard(&key_input);

    if (key_input == 0x6B){ //Check if left arrow (go left)
        bit_3210 = 0b0100;
    } else if (key_input == 0x72){ //Check if down arrow (go center)
        bit_3210 = 0b0010;
    } else if (key_input == 0x74){ //Check if right arrow (go right)
        bit_3210 = 0b0001;
    }

    if (bit_3210 != 0){
        //Get which disks to move by having the SW

        SW_value = (*SW_ptr) & 0b11111; //Get the value of the first 5 SW
        direction_rods(disks, SW_value, bit_3210);
    }

    //Display the num of move
    display_hex_10(num_move);
	
	//Check if the user won to display there stats if not already winning
	if (!winning) {
        best_move_tracker(disks);
    }

    display_hex_54(time);

    //Check if the user don't have any time
    no_more_time(); //will set losing to true
    
    //Show the time
    if (delay_sec() == true){
        time --;

        //Reset time if reach 0
        if (time < 0){
        time = 90;
        }
    }

    /*
    //Music for winning or losing
    if (once == 0){
        if ((winning && disks[0].y == 160)|| losing){
            if (winning){
                play_audio(victory, numVictory);
                winning = false;
            } else if (losing){
                play_audio(sad, numSad);
                losing = false;
            }
            once = 1;
        }     
    }*/

    
}

//////////////////////////////////////////////////////////////////////////////////
void draw_start_screen(){
    clear_screen();
    int title_length = strlen("tower of hanoi") * 10;
	int menu_length = strlen("MAIN MENU") * 10;
	int objective_length = strlen("OBJECTIVE: MOVE ALL") * 10;
	int objective2_length = strlen("DISKS TO TOWER 3") * 10;
	int easy = strlen("EASY MODE: PRESS E") * 10;
	int medium = strlen("MEDIUM MODE: PRESS M") * 10;
	int hard = strlen("HARD MODE: PRESS H") * 10;
	
    int title_x = (320 - title_length)/2;
	int objective_x = (320 - objective_length)/2;
	int objective2_x = (320 - objective2_length)/2;
	int menu_x = (320 - menu_length)/2;
	int easy_x = (320 - easy)/2;
	int medium_x = (320 - medium)/2;
	int hard_x = (320 - hard)/2;
	
    draw_text(title_x, 50, "tower of hanoi", 0xFFFF);
	draw_text(objective_x, 80, "OBJECTIVE: MOVE ALL", 0xFFFF);
	draw_text(objective2_x, 95, "DISKS TO TOWER 3", 0xFFFF);
	draw_text(menu_x, 130, "MAIN MENU", 0xFFFF);
	draw_text(easy_x, 150, "EASY MODE: PRESS E", 0xFFFF);
	draw_text(medium_x, 170, "MEDIUM MODE: PRESS M", 0xFFFF);
	draw_text(hard_x, 190, "HARD MODE: PRESS H", 0xFFFF);
}

//////////////////////////////////////////////////////////
void draw_end_screen(){
	int title_length = strlen("tower of hanoi") * 10;
    int message = strlen("GAME COMPLETE!") * 10;
	int score_length = strlen("FINAL SCORE: ") * 10;
	int restart_length = strlen("RETURN TO MAIN MENU: PRESS R") * 10;
	
	int best_easy_length = strlen("BEST SCORE <EASY>: ") * 10;
    int best_medium_length = strlen("BEST SCORE <MEDIUM>: ") * 10;
    int best_hard_length = strlen("BEST SCORE <HARD>: ") * 10;
	
    int title_x = (320 - title_length)/2;
    int message_x = (320 - message)/2;
	int score_x = (320 - score_length)/2;
	int restart_x = (320 - restart_length)/2;
	
	int score_num_x = score_x + score_length + 2;
	
    draw_text(title_x, 60, "tower of hanoi", 0xFFFF);
    draw_text(message_x, 110, "GAME COMPLETE!", 0xFFFF);
	draw_text(score_x, 130, "FINAL SCORE: ", 0xFFFF);
	draw_text(restart_x, 190, "RETURN TO MAIN MENU: PRESS R", 0xFFFF);
	
	//draw the score
	int tens = num_move/10;
    int ones = num_move%10;
    drawLetter(score_num_x, 130, '0'+tens, 0xFFFF);
    drawLetter(score_num_x+10, 130, '0'+ones, 0xFFFF);
	
	//draw the best score
	int best_score = 0;
	int best_text_x;
	int best_num_x;
	
    if (N == 3){
		best_score = best_move_easy;
		best_text_x = (320 - best_easy_length)/2;
        draw_text(best_text_x, 150, "BEST SCORE <EASY>: ", 0xFFFF);
        best_num_x = best_text_x + best_easy_length + 2;
	}else if (N == 4){
		best_score = best_move_medium;
		best_text_x = (320 - best_medium_length)/2;
        draw_text(best_text_x, 150, "BEST SCORE <MEDIUM>: ", 0xFFFF);
        best_num_x = best_text_x + best_medium_length + 2;
	}else if (N == 5){
		best_score = best_move_hard;
		best_text_x = (320 - best_hard_length)/2;
        draw_text(best_text_x, 150, "BEST SCORE <HARD>: ", 0xFFFF);
        best_num_x = best_text_x + best_hard_length + 2;
	}
	
	tens = best_score/10;
    ones = best_score%10;
    drawLetter(best_num_x, 150, '0'+tens, 0xFFFF);
    drawLetter(best_num_x+10, 150, '0'+ones, 0xFFFF);
}
	
	
/////////////////////////////////////////////////////////////////////////////////////////
void display_hex_10(int num){
    unsigned char hex_num[10] = { //8 bits values
    0x3F,  // 0
    0x06,  // 1
    0x5B,  // 2
    0x4F,  // 3
    0x66,  // 4
    0x6D,  // 5
    0x7D,  // 6
    0x07,  // 7
    0x7F,  // 8
    0x6F   // 9
    };

    if (num < 0 || num > 99) {
        return;  // Make sure the num is in the range
    }
    
    volatile int * HEX_ptr = (volatile int *) HEX3_HEX0_BASE;
    int ones = num%10; //get the first digit
    int tens = (num/10)%10; //get the second digit

    // Combine 2 digits into one 32-bit value
    unsigned int hex_value = (hex_num[tens] << 8) | hex_num[ones];

    // Write the combined value to HEX1 and HEX0
    *HEX_ptr = hex_value;
}


void display_hex_54(int num){
    unsigned char hex_num[10] = { //8 bits values
    0x3F,  // 0
    0x06,  // 1
    0x5B,  // 2
    0x4F,  // 3
    0x66,  // 4
    0x6D,  // 5
    0x7D,  // 6
    0x07,  // 7
    0x7F,  // 8
    0x6F   // 9
    };

    if (num < 0 || num > 99) {
        return;  // Make sure the num is in the range
    }
    
    volatile int * HEX_ptr = (volatile int *) HEX5_HEX4_BASE;
    int ones = num%10; //get the first digit
    int tens = (num/10)%10; //get the second digit

    // Combine 2 digits into one 32-bit value
    unsigned int hex_value = (hex_num[tens] << 8) | hex_num[ones];

    // Write the combined value to HEX1 and HEX0
    *HEX_ptr = hex_value;
}

/////////////////////////////////////////////////////////////////////////////////////////
void read_keyboard(unsigned char *pressedKey) {
    volatile int *ps2_ptr = (int *) 0xFF200100; //Get the PS/2 data register

    //while (1) {  //Loop until a valid key is received
        int data = *ps2_ptr;  

        if (data & 0x8000) {  //Check if RVALID (bit 15) is set
            *pressedKey = data & 0xFF;  // Get data == lower 8 bits
            return;
        }
    //}
}

/////////////////////////////////////////////////////////////////////////////////////////
//Or Draw rectangular shape function (just change the size)
void draw_disk(struct disk_info disk) {
    for (int x_axis = 0; x_axis < disk.size; x_axis++) {
        for (int y_axis = 0; y_axis < 20; y_axis++) {
            plot_pixel(disk.x + x_axis, disk.y + y_axis, disk.colour);
        }
    }
}

/////////////////////////////////////////////////////////////
void drawBars(){
	
	for(int row = 220; row<=225; row++){
		draw_line(10, row, 309, row, 0xFFFF);
	}
	for(int row = 110; row<=225; row++){
		draw_line(53, row, 57, row, 0xFFFF);
		draw_line(157, row, 161, row, 0xFFFF);
		draw_line(261, row, 265, row, 0xFFFF);
	}
}
////////////////////////////////////////////////////////////////////////////////
void drawLetter(int x, int y, char c, short int color){
	static const unsigned char T_array[8] = {0xFF,0x18,0x18,0x18,0x18,0x18,0x18,0x00};
	static const unsigned char O_array[8] = {0x7E,0x81,0x81,0x81,0x81,0x81,0x7E,0x00};
	static const unsigned char W_array[8] = {0x81,0x81,0x81,0x81,0x99,0xA5,0x42,0x00};
	static const unsigned char E_array[8] = {0xFF,0x80,0x80,0xFE,0x80,0x80,0xFF,0x00};
	static const unsigned char R_array[8] = {0xFE,0x81,0x81,0xFE,0x90,0x88,0x87,0x00};
	static const unsigned char F_array[8] = {0xFF,0x80,0x80,0xFE,0x80,0x80,0x80,0x00};
	static const unsigned char H_array[8] = {0x81,0x81,0x81,0xFF,0x81,0x81,0x81,0x00};
	static const unsigned char A_array[8] = {0x7E,0x81,0x81,0xFF,0x81,0x81,0x81,0x00};
	static const unsigned char N_array[8] = {0x81,0xC1,0xA1,0x91,0x89,0x85,0x81,0x00};
	static const unsigned char I_array[8] = {0xFF,0x18,0x18,0x18,0x18,0x18,0xFF,0x00};
	static const unsigned char P_array[8] = {0xFE,0x81,0x81,0xFE,0x80,0x80,0x80,0x00};
	static const unsigned char S_array[8] = {0x7E,0x80,0x80,0x7E,0x01,0x01,0x7E,0x00};
	static const unsigned char K_array[8] = {0x81,0x82,0x84,0xF8,0x84,0x82,0x81,0x00};
	static const unsigned char Y_array[8] = {0x81,0x42,0x24,0x18,0x18,0x18,0x18,0x00};
	static const unsigned char G_array[8] = {0x7E,0x81,0x80,0x9E,0x81,0x81,0x7E,0x00};
	static const unsigned char U_array[8] = {0x81,0x81,0x81,0x81,0x81,0x81,0x7E,0x00};
	static const unsigned char L_array[8] = {0x80,0x80,0x80,0x80,0x80,0x80,0xFF,0x00};
	static const unsigned char M_array[8] = {0x81,0xC3,0xA5,0x99,0x81,0x81,0x81,0x00};
	static const unsigned char D_array[8] = {0xFE,0x81,0x81,0x81,0x81,0x81,0xFE,0x00};
	static const unsigned char C_array[8] = {0x3E,0x40,0x80,0x80,0x80,0x40,0x3E,0x00};
	static const unsigned char B_array[8] = {0xFE,0x82,0x82,0xFC,0x82,0x82,0xFE,0x00};
	static const unsigned char J_array[8] = {0x7F,0x04,0x04,0x04,0x04,0x44,0x64,0x38};
	static const unsigned char V_array[8] = {0x41,0x41,0x41,0x41,0x22,0x22,0x14,0x08};
	static const unsigned char zero_array[8] = {0x7E,0x81,0x81,0x81,0x81,0x81,0x7E,0x00};
	static const unsigned char one_array[8] = {0x18,0x38,0x18,0x18,0x18,0x18,0x7E,0x00};
	static const unsigned char two_array[8] = {0x7E,0x81,0x01,0x3E,0x40,0x80,0xFF,0x00};
	static const unsigned char three_array[8]= {0x7E,0x81,0x01,0x3E,0x01,0x81,0x7E,0x00};
	static const unsigned char four_array[8] = {0x04,0x0C,0x14,0x24,0x44,0xFF,0x04,0x00};
	static const unsigned char five_array[8] = {0xFF,0x80,0x80,0xFE,0x01,0x81,0x7E,0x00};
	static const unsigned char six_array[8] = {0x7E,0x80,0x80,0xFE,0x81,0x81,0x7E,0x00};
	static const unsigned char seven_array[8]= {0xFF,0x01,0x02,0x04,0x08,0x10,0x10,0x00};
	static const unsigned char eight_array[8]= {0x7E,0x81,0x81,0x7E,0x81,0x81,0x7E,0x00};
	static const unsigned char nine_array[8] = {0x7E,0x81,0x81,0x7F,0x01,0x81,0x7E,0x00};
	static const unsigned char space_array[8]= {0,0,0,0,0,0,0,0};
	static const unsigned char colon_array[8] = {0x00,0x18,0x18,0x00,0x00,0x18,0x18,0x00};
	static const unsigned char exclamation_array[9] = {0x18,0x18,0x18,0x18,0x18,0x18,0x00,0x18,0x00};
	static const unsigned char less_than_array[8] = {0x02,0x04,0x08,0x10,0x08,0x04,0x02,0x00};
	static const unsigned char greater_than_array[8] = {0x40,0x20,0x10,0x08,0x10,0x20,0x40,0x00};
	static const unsigned char right_arrow_array[8] = {0x08,0x0C,0x0E,0xFF,0x0E,0x0C,0x08,0x00};
	static const unsigned char left_arrow_array[8] = {0x10,0x30,0x70,0xFF,0x70,0x30,0x10,0x00};
	static const unsigned char down_arrow_array[8]  = {0x18,0x18,0x18,0x18,0xFF,0x7E,0x3C,0x18};
	
	static const unsigned char T_wide[8] = {0xFF,0xFF,0x18,0x18,0x18,0x18,0x18,0x18};
    static const unsigned char O_wide[8] = {0x7E,0xFF,0xC3,0xC3,0xC3,0xC3,0xFF,0x7E};
    static const unsigned char W_wide[8] = {0xC3,0xC3,0xC3,0xC3,0xDB,0xDB,0x66,0x66};
    static const unsigned char E_wide[8] = {0xFF,0xFF,0xC0,0xFE,0xFE,0xC0,0xFF,0xFF};
    static const unsigned char R_wide[8] = {0xFC,0xFE,0xC6,0xFC,0xF8,0xCC,0xC6,0xC3};
    static const unsigned char F_wide[8] = {0xFF,0xFF,0xC0,0xFC,0xFC,0xC0,0xC0,0xC0};
    static const unsigned char H_wide[8] = {0xC3,0xC3,0xC3,0xFF,0xFF,0xC3,0xC3,0xC3};
    static const unsigned char A_wide[8] = {0x3C,0x7E,0xC3,0xC3,0xFF,0xFF,0xC3,0xC3};
    static const unsigned char N_wide[8] = {0xC3,0xE3,0xF3,0xDB,0xCF,0xC7,0xC3,0xC3};
    static const unsigned char I_wide[8] = {0xFF,0xFF,0x18,0x18,0x18,0x18,0xFF,0xFF};
	const unsigned char *letter;
	
    switch(c) {
        case 'T': letter = T_array; break;
        case 'O': letter = O_array; break;
        case 'W': letter = W_array; break;
        case 'E': letter = E_array; break;
        case 'R': letter = R_array; break;
        case 'F': letter = F_array; break;
        case 'H': letter = H_array; break;
        case 'A': letter = A_array; break;
        case 'N': letter = N_array; break;
        case 'I': letter = I_array; break;
		case 'P': letter = P_array; break;
		case 'S': letter = S_array; break;
		case 'K': letter = K_array; break;
		case 'Y': letter = Y_array; break;
		case 'G': letter = G_array; break;
    	case 'U': letter = U_array; break;
    	case 'L': letter = L_array; break;
    	case 'M': letter = M_array; break;
    	case 'D': letter = D_array; break;
		case 'C': letter = C_array; break;
		case 'B': letter = B_array; break;
		case 'J': letter = J_array; break;
		case 'V': letter = V_array; break;
		case '0': letter = zero_array; break;
		case '1': letter = one_array; break;
		case '2': letter = two_array; break;
		case '3': letter = three_array; break;
		case '4': letter = four_array; break;
		case '5': letter = five_array; break;
		case '6': letter = six_array; break;
		case '7': letter = seven_array; break;
		case '8': letter = eight_array; break;
		case '9': letter = nine_array; break;
        case ' ': letter = space_array; break;
		case ':': letter = colon_array; break;
		case '!': letter = exclamation_array; break;
		case '<': letter = less_than_array; break;
    	case '>': letter = greater_than_array; break;
		case '@': letter = right_arrow_array; break;
		case '#': letter = left_arrow_array; break;
		case '$': letter = down_arrow_array; break;
			
		case 't': letter = T_wide; break;
        case 'o': letter = O_wide; break;
        case 'w': letter = W_wide; break;
        case 'e': letter = E_wide; break;
        case 'r': letter = R_wide; break;
        case 'f': letter = F_wide; break;
        case 'h': letter = H_wide; break;
        case 'a': letter = A_wide; break;
        case 'n': letter = N_wide; break;
        case 'i': letter = I_wide; break;
		
        default: return;
	}
			
	for(int i = 0; i < 8; i++) {
        for(int j = 0; j < 8; j++) {
            if(letter[i] & (1 << (7-j))) {
                plot_pixel(x+j, y+i, color);
			}
        }
    }
}
/////////////////////////////////////////////////////////////////////////////////////////
void draw_text(int x, int y, const char *s, short int color) {
    while(*s) {
        drawLetter(x, y, *s++, color);
        x += 10; //space between letters
    }
}
/////////////////////////////////////////////////////////////////////////////////////////
//Check if we can add the disk at the column
bool add_disk_column(struct disk_info disk, int direction){
    struct disk_info *current = &disk;

    if (N == 3) {//Easy mode
        //Verify that the disk is the highest element on the column right now
        int i = 0;  // start from the top

        //Get the correct column array
        int *column = NULL;
        if (current->column == 0) {
            column = column0;
        } else if (current->column == 1) {
            column = column1;
        } else if (current->column == 2) {
            column = column2;
        }

        //Find the topmost nonzero disk
        while (i < 3 && column[i] == 0) {
            i++;
        }

        //If i = 3 then the entire column is full of 0 == empty
        //If i < 3 then there is a item in the column
        //Ensure it's the disk we want to move
        if ((i < 3) && (column[i] != current->size)) {
            return false;
        }

        bool left = false;
        bool center = false;
        bool right = false;

        //Get the direction to go
        //KEY2 == LEFT // KEY1 == CENTER // KEY0 == RIGHT

        if (direction == 0b0001){
            right = true;
        } else if (direction == 0b0010){
            center = true;
        } else if (direction == 0b0100){
            left = true;
        }

        //Start from the end and goes to the begin
        for (int i = N-1 ; i >= 0; i--){
            //Check LEFT
            if(left){
                //Check if there is a smaller disk already here
                if ((column0[i] != 0) && (current->size > column0[i])) {
                    return false;
                } else if (column0[i] == 0){ //Check if space is available
                    column0[i] = current->size;
                    return true;
                }
            } 

            //Check CENTER
            if(center){
                //Check if there is a smaller disk already here
                if ((column1[i] != 0) && (current->size > column1[i])) {
                    return false;
                } else if (column1[i] == 0){ //Check if space is available
                    column1[i] = current->size;
                    return true;
                }
            }

            //Check RIGHT
            if(right){
                //Check if there is a smaller disk already here
                if ((column2[i] != 0) && (current->size > column2[i])) {
                    return false;
                } else if (column2[i] == 0){ //Check if space is available
                    column2[i] = current->size;
                    return true;
                }
            }

        }
    } else if (N == 4) { //Medium mode
        //Verify that the disk is the highest element on the column right now
        int i = 0;  // start from the top

        //Get the correct column array
        int *column = NULL;
        if (current->column == 0) {
            column = column0_medium;
        } else if (current->column == 1) {
            column = column1_medium;
        } else if (current->column == 2) {
            column = column2_medium;
        }

        //Find the topmost nonzero disk
        while (i < 4 && column[i] == 0) {
            i++;
        }

        //If i = 4 then the entire column is full of 0 == empty
        //If i < 4 then there is a item in the column
        //Ensure it's the disk we want to move
        if ((i < 4) && (column[i] != current->size)) {
            return false;
        }

        bool left = false;
        bool center = false;
        bool right = false;

        //Get the direction to go
        //KEY2 == LEFT // KEY1 == CENTER // KEY0 == RIGHT

        if (direction == 0b0001){
            right = true;
        } else if (direction == 0b0010){
            center = true;
        } else if (direction == 0b0100){
            left = true;
        }

        //Start from the end and goes to the begin
        for (int i = N-1 ; i >= 0; i--){
            //Check LEFT
            if(left){
                //Check if there is a smaller disk already here
                if ((column0_medium[i] != 0) && (current->size > column0_medium[i])) {
                    return false;
                } else if (column0_medium[i] == 0){ //Check if space is available
                    column0_medium[i] = current->size;
                    return true;
                }
            } 

            //Check CENTER
            if(center){
                //Check if there is a smaller disk already here
                if ((column1_medium[i] != 0) && (current->size > column1_medium[i])) {
                    return false;
                } else if (column1_medium[i] == 0){ //Check if space is available
                    column1_medium[i] = current->size;
                    return true;
                }
            }

            //Check RIGHT
            if(right){
                //Check if there is a smaller disk already here
                if ((column2_medium[i] != 0) && (current->size > column2_medium[i])) {
                    return false;
                } else if (column2_medium[i] == 0){ //Check if space is available
                    column2_medium[i] = current->size;
                    return true;
                }
            }

        }
    
    } else if (N == 5) { //Hard mode
        //Verify that the disk is the highest element on the column right now
        int i = 0;  // start from the top

        //Get the correct column array
        int *column = NULL;
        if (current->column == 0) {
            column = column0_hard;
        } else if (current->column == 1) {
            column = column1_hard;
        } else if (current->column == 2) {
            column = column2_hard;
        }

        //Find the topmost nonzero disk
        while (i < 5 && column[i] == 0) {
            i++;
        }

        //If i = 5 then the entire column is full of 0 == empty
        //If i < 5 then there is a item in the column
        //Ensure it's the disk we want to move
        if ((i < 5) && (column[i] != current->size)) {
            return false;
        }

        bool left = false;
        bool center = false;
        bool right = false;

        //Get the direction to go
        //KEY2 == LEFT // KEY1 == CENTER // KEY0 == RIGHT

        if (direction == 0b0001){
            right = true;
        } else if (direction == 0b0010){
            center = true;
        } else if (direction == 0b0100){
            left = true;
        }

        //Start from the end and goes to the begin
        for (int i = N-1 ; i >= 0; i--){
            //Check LEFT
            if(left){
                //Check if there is a smaller disk already here
                if ((column0_hard[i] != 0) && (current->size > column0_hard[i])) {
                    return false;
                } else if (column0_hard[i] == 0){ //Check if space is available
                    column0_hard[i] = current->size;
                    return true;
                }
            } 

            //Check CENTER
            if(center){
                //Check if there is a smaller disk already here
                if ((column1_hard[i] != 0) && (current->size > column1_hard[i])) {
                    return false;
                } else if (column1_hard[i] == 0){ //Check if space is available
                    column1_hard[i] = current->size;
                    return true;
                }
            }

            //Check RIGHT
            if(right){
                //Check if there is a smaller disk already here
                if ((column2_hard[i] != 0) && (current->size > column2_hard[i])) {
                    return false;
                } else if (column2_hard[i] == 0){ //Check if space is available
                    column2_hard[i] = current->size;
                    return true;
                }
            }

        }
    } 

    // If none of the conditions matched, return false to prevent warning
    return false;
}

/////////////////////////////////////////////////////////////////////////////////////////
void delete_disk_column(struct disk_info disk){
    struct disk_info *current = &disk; 

    if (N == 3) { //Easy mode
        //Go through the column and remove the disk size with 0
        for (int i = 0; i < N; i++){
            if (current->column == 0){
                if (column0[i] == current->size){
                    column0[i] = 0;
                    return;
                }
            }
            if (current->column == 1){
                if (column1[i] == current->size){
                    column1[i] = 0;
                    return;
                }
            }

            if (current->column == 2){
                if (column2[i] == current->size){
                    column2[i] = 0;
                    return;
                }
            }
        }
    } else if (N == 4) { //Medium mode
        //Go through the column and remove the disk size with 0
        for (int i = 0; i < N; i++){
            if (current->column == 0){
                if (column0_medium[i] == current->size){
                    column0_medium[i] = 0;
                    return;
                }
            }
            if (current->column == 1){
                if (column1_medium[i] == current->size){
                    column1_medium[i] = 0;
                    return;
                }
            }

            if (current->column == 2){
                if (column2_medium[i] == current->size){
                    column2_medium[i] = 0;
                    return;
                }
            }
        }
    }  else if (N == 5) { //Hard mode
        //Go through the column and remove the disk size with 0
        for (int i = 0; i < N; i++){
            if (current->column == 0){
                if (column0_hard[i] == current->size){
                    column0_hard[i] = 0;
                    return;
                }
            }
            if (current->column == 1){
                if (column1_hard[i] == current->size){
                    column1_hard[i] = 0;
                    return;
                }
            }

            if (current->column == 2){
                if (column2_hard[i] == current->size){
                    column2_hard[i] = 0;
                    return;
                }
            }
        }
    }
}
/////////////////////////////////////////////////////////////////////////////////////
//Create the direction pattern to go left, center, right

void direction_rods(struct disk_info disks[], int index, int direction) {
    if (N == 3){ //easy mode
        if (index == 0b001){
            index = 0;
        } else if (index == 0b010){
            index = 1;
        } else if (index == 0b100){
            index = 2;
        } else {
            return;
        }
    } else if (N == 4) { //medium mode
        if (index == 0b001){
            index = 0;
        } else if (index == 0b010){
            index = 1;
        } else if (index == 0b100){
            index = 2;
        } else if (index == 0b1000){
            index = 3;
        } else {
            return;
        }
    } else if (N == 5) { //hard mode
        if (index == 0b00001){
            index = 0;
        } else if (index == 0b00010){
            index = 1;
        } else if (index == 0b00100){
            index = 2;
        } else if (index == 0b01000){
            index = 3;
        } else if (index == 0b10000){
            index = 4;
        } else {
            return;
        }
    }

    // Get the disk to move
    struct disk_info *current = &disks[index]; //index is the SW where 0,1,2 represent S, M, L Disks

    bool left = false;
    bool center = false;
    bool right = false;

    //Get the direction to go
    //KEY2 == LEFT // KEY1 == CENTER // KEY0 == RIGHT

    if (direction == 0b0001){
        right = true;
    } else if (direction == 0b0010){
        center = true;
    } else if (direction == 0b0100){
        left = true;
    } else {
        return;
    }


    //Check for invalid move (the disk is already at the column where the move want to be executed)
    if ((current->column == 0 && left) || (current->column == 1 && center) || (current->column == 2 && right)){
        return; 
    }

    //If LEFT: Change the x and y coordinate for left side
    //If CENTER: Change the x and y coordinate for center side
    //If RIGHT: Change the x and y coordinate for right side
    //UPDATE COLUMN

    if (left){
        if (add_disk_column(*current, direction)) {
            if (current->size == 50){
                current->x = 30;
            } else if (current->size == 70){
                current->x = 20;
            } else if (current->size == 90){
                current->x = 10;
            } else if (current->size == 30){ //for medium
                current->x = rod_positions[0] - (current->size / 2);;
            } else if (current->size == 20){// for hard
                current->x = rod_positions[0] - (current->size / 2);;
            }
            current->y = 80;

            delete_disk_column(*current); //Delete the disk from column array before update
            //Delete use the position of current in the column before the update
            current->column = 0;
            num_move = num_move_tracker(num_move);
        }
    } else if (center){
        if (add_disk_column(*current, direction)) {
            /*if (current->size == 50){
                current->x = 140; 
            } else if (current->size == 70){
                current->x = 130;
            } else if (current->size == 90){
                current->x = 120;
            }
			*/
			
			current->x = rod_positions[1] - (current->size / 2);
            current->y = 80;
			
            delete_disk_column(*current); //Delete the disk from column array before update
            //Delete use the position of current in the column before the update
            current->column = 1;
            num_move = num_move_tracker(num_move);
        }
    }  else if (right){
        if (add_disk_column(*current, direction)){
            if (current->size == 50){
                current->x = 240; 
            } else if (current->size == 70){
                current->x = 230;
            } else if (current->size == 90){
                current->x = 220;
            } else if (current->size == 30){
                current->x = rod_positions[2] - (current->size / 2);;
            } else if (current->size == 20){// for hard
                current->x = rod_positions[2] - (current->size / 2);;
            }
            current->y = 60;
            delete_disk_column(*current); //Delete the disk from column array before update
            //Delete use the position of current in the column before the update
            current->column = 2;
            num_move = num_move_tracker(num_move);
        }
    }
}

/////////////////////////////////////////////////////////////////////////////////////////
//Create direction pattern to always want to go down until collision occurs with limit from rods or another disks

void update_disk_position(struct disk_info disks[], int index) {
    // Get the current disk
    struct disk_info *current = &disks[index];

    // Check if the disk has reached the bottom of the screen
    if (current->y >= 200) {
        current->y = 200; //Max y of a disk
        return;
    }

    // Check collision with other disks below
    for (int i = 0; i < N; i++) {
        if (i != index && disks[i].column == current->column){
            if (disks[i].y > current->y) {  // Only check disks below
                if (current->y + 20 >= disks[i].y) {
                    current->y = current->y; //Means we do not need to move the disk down
                    return;
                }
            }
        }
    }

    // Move the disk down by 1 pixel (gravity)
    current->y += 4;
}
/////////////////////////////////////////////////////////////////////////////////////////
//SCORE//

//Increment the number of move by one + display it 
int num_move_tracker(int num_move){
    num_move = num_move + 1;
    return num_move;

}


//Check the best score
void best_move_tracker(struct disk_info disks[]){
    //Check the mode and if we are at the winning state
    int potential_move = num_move;
    bool win = false;
	static bool animation_complete = false;

    if (N == 3){
        //Check if winning state
        if (column2[0] == 50 && column2[1] == 70 && column2[2] == 90) {
			//if last disk has stopped moving
			if (disks[0].y >= 200 || (disks[0].y + 20 >= disks[1].y)){
            	win = true;
				animation_complete = true;
			}
			
            //Update the best num of move if the numver is less or if best was initially 0
            if (potential_move < best_move_easy || best_move_easy == 0){
                best_move_easy = potential_move;               
            }
        }
    } else if (N == 4){
        //Check if winning state
        if (column2_medium[0] == 30 && column2_medium[1] == 50 && column2_medium[2] == 70 && column2_medium[3] == 90) {
			//if last disk has stopped moving
			if (disks[0].y >= 200 || (disks[0].y + 20 >= disks[1].y)){
            	win = true;
				animation_complete = true;
			}
            //Update the best num of move if the numver is less or if best was initially 0
            if (potential_move < best_move_medium || best_move_medium == 0){
                best_move_medium = potential_move;
            }
        }
    } else if (N == 5){
        //Check if winning state
        if (column2_hard[0] == 20 && column2_hard[1] == 30 && column2_hard[2] == 50 && column2_hard[3] == 70 && column2_hard[4] == 90) {
			//if last disk has stopped moving
			if (disks[0].y >= 200 || (disks[0].y + 20 >= disks[1].y)){
            	win = true;
				animation_complete = true;
			}
            //Update the best num of move if the numver is less or if best was initially 0
            if (potential_move < best_move_hard || best_move_hard == 0){
                best_move_hard = potential_move;
            }
        }
    }
    
    if (win && animation_complete){
        winning = true;
		end_screen=true;
		animation_complete = false; //reset for next game
        //stop the time
        timer->control = 0b1000; //Press on STOP

        // Print the results
        printf("Best Move Easy: %d \nBest Move Medium: %d \nBest Move Hard: %d\n", 
            best_move_easy, best_move_medium, best_move_hard);
        win = false;
    }
}

/////////////////////////////////////////////////////////////////////////////////////////
void restart_game(struct disk_info disks[]){
    //The key_input from the keyboard
    unsigned char restart_input = 0;
    read_keyboard(&restart_input);
    restart = false;
	winning = false;
	losing = false;
	once = 0;

    if (restart_input == 0x2D){ //Check if user press R
        restart = true;
    }

    //If restart, reinitalize everything : column, disk position, score
    if (restart){
		end_screen = false;
		start_screen = true;
		num_move = 0;
        //Get the mode
        volatile int * SW_ptr = (volatile int *) SW_BASE;
        int mode = (*SW_ptr) & 0b1100000000; //Get SW[9]
        if (mode == 0){
            N = 3; //easy mode
        } else if (mode == 0b1000000000){
            N = 4; //medium mode
            mode = 1;
        } else if (mode == 0b1100000000){
            N = 5; //hard mode
            mode = 2;
        }

        //Reset each column
        for (int i = 0; i < N; i++){
            column0[i] = 0;
            column1[i] = 0;
            column2[i] = 0;
            column0_medium[i] = 0;
            column1_medium[i] = 0;
            column2_medium[i] = 0;
            column0_hard[i] = 0;
            column1_hard[i] = 0;
            column2_hard[i] = 0;
        }

        //Reinitialize location and direction of rectangles(not shown)
        for (int i = 0; i < N; i++) {
            disks[i].dx = (((rand() % 2) * 2) - 1); //generate a random value: -1 or 1
            disks[i].dy = (((rand() % 2) * 2) - 1); //generate a random value: -1 or 1
            //disks[i].colour = colour_array[rand() % 9];   //Create a random initial colour for the boxes
            disks[i].x_old1 = 0;
            disks[i].y_old1 = 0;
            disks[i].x_old2 = 0;
            disks[i].y_old2 = 0;
            disks[i].column = 0; //Need to reset each column location to 0
        }

        //White,  Red,   Green, Blue,  Cyan,   Magenta, Yellow, Orange, Pink, 
        short int colour_array[9] = {0xFFFF, 0xF800, 0x07E0, 0x001F, 0x07FF, 0xF81F, 0xFFE0, 0xFC60, 0xF81F}; 
        
        if (mode == 0) {
                //Reinitialize the size
                disks[0].size = 50;  // Small (Blue)
                disks[1].size = 70;  // Medium (Green)
                disks[2].size = 90;  // Large (Red)

                //Reinitialize the colour
                disks[0].colour = colour_array[4];  // Small (Blue)
                disks[1].colour = colour_array[2];  // Medium (Green)
                disks[2].colour = colour_array[1];  // Large (Red)

                //Reinitialize the column
                for (int i = 0; i < N; i++) {
                    column0[i] = disks[i].size;
                }
        } else if (mode == 1) {
            //Reinitialize the size
            disks[0].size = 30;  // Very Small (Yellow)
            disks[1].size = 50;  // Small (Blue)
            disks[2].size = 70;  // Medium (Green)
            disks[3].size = 90;  // Large (Red)

            //Reinitialize the colour
            disks[0].colour = colour_array[6];  // Very Small (Yellow)
            disks[1].colour = colour_array[4];  // Small (Blue)
            disks[2].colour = colour_array[2];  // Medium (Green)
            disks[3].colour = colour_array[1];  // Large (Red)

            //Reinitialize the column
            for (int i = 0; i < N; i++) {
                column0_medium[i] = disks[i].size;
            }
        } else if (mode == 2){
            //Reinitialize the size
            disks[0].size = 20;  // Extra Small (pink)
            disks[1].size = 30;  // Very Small (Yellow)
            disks[2].size = 50;  // Small (Blue)
            disks[3].size = 70;  // Medium (Green)
            disks[4].size = 90;  // Large (Red)

            //Reinitialize the colour
            disks[0].colour = colour_array[8];  // Extra Small (Pink)
            disks[1].colour = colour_array[6];  // Very Small (Yellow)
            disks[2].colour = colour_array[4];  // Small (Blue)
            disks[3].colour = colour_array[2];  // Medium (Green)
            disks[4].colour = colour_array[1];  // Large (Red)

            //Reinitialize the column
            for (int i = 0; i < N; i++) {
                column0_hard[i] = disks[i].size;
            }

        }

        //Reinitialize the location of the disks
        for (int i = 0; i < N; i++) {
            //Small at (30,10), Medium at (20,50), Large at (10,90)
            //Remember each rectangle is 20 pixels in the y-axis
            //disks[i].x = 30 - i*10;  
            disks[i].x = rod_positions[0] - ((disks[i].size) / 2);
            disks[i].y = 20 + i*40;
            //printf("Disk %d: x = %d, y = %d\n", i, disks[i].x, disks[i].y);
        }

        //Restart num_move
        num_move = 0;

        //Restart the time
        time = 90;
        timer->control = 0b0110; //Start the timer + CONT

        //Restart the bool for win or losing
        winning = false;
        losing = false;
        once = 0;

    }
}

/////////////////////////////////////////////////////////////////////////////////////////
//TIMER FUNCTION//
void setup_timer(){
       timer->control = 0x8; //Stop the timer in case it is on
       timer->status = 0; //Clear TO bit in case it is on
       timer->periodlo = (COUNTER_DELAY & 0x0000FFFF); //laod the 16 smaller bit of counter
       timer->periodhi = (COUNTER_DELAY & 0xFFFF0000) >> 16; //shift right the high 16bit of counteer
       timer->control = 0b110; //Start the timer + CONT
}

bool delay_sec(){
    if ((timer->status & 0x1) == 1){ //get the TO bit, if 1 then 1 sec passed
        timer->status = 0; // reset TO 
        return true;
    }

    return false;
}

void no_more_time(){
    if (time == 0){
        losing = true;
    }
}

/////////////////////////////////////////////////////////////////////////////////////////
////HELPER FUNCTION///////
void draw_line(int x0, int y0, int x1, int y1, short int color){
    bool is_steep = abs(y1-y0) > abs(x1 - x0);

    if (is_steep) { //change evaluation from x to y if the slope is steep
        swap(&x0, &y0);
        swap(&x1, &y1);
    }
    if (x0 > x1) { //swap the start and ends points if x0 is bigger than x1
        swap(&x0, &x1);
        swap(&y0, &y1);
    }

    int deltax = x1 - x0;
    int deltay = abs(y1 - y0);
    int error = -(deltax / 2);
    //Error taking in account the relative difference btwn width(deltax) 
    //and height of line(deltay) and decide how often y should be incremented
    int y = y0;
    int y_step;

    if (y0 < y1){
        y_step = 1;
    } else {
        y_step = -1;
    }

    for (int x = x0; x <= x1; x++){
        if (is_steep){
            plot_pixel(y, x, color);
        } else {
            plot_pixel(x, y, color);
        }
        
        error = error + deltay;

        if (error > 0){
            y = y + y_step;
            error = error - deltax;
        }
    }

}

void swap(int *a, int *b) {
    //pointer a and b get the adress of the 2 values to swap
    int temp = *a;
    *a = *b;
    *b = temp;
}


void wait_for_vsync(){
    volatile int * pixel_ctrl_ptr = (int *)0xFF203020; //Base address
    int status;

    *pixel_ctrl_ptr = 1; //start the synchronization process
                         //Write 1 into front buffer address register
                         //Synchronize with the vertical syncronization cycle of VGA controller

    status = *(pixel_ctrl_ptr + 3); //read the status register

    while ((status & 0x01) != 0){ //polling loop waiting for S bit to go to 0
        status = *(pixel_ctrl_ptr+3);
    }
}

void clear_screen(){
    for (int x = 0; x < 320; x++){
        for (int y = 0; y < 240; y++){
            plot_pixel(x, y, 0x0000);
        }
    }

}

void plot_pixel(int x, int y, short int line_color)
{
    volatile short int *one_pixel_address;
        
        one_pixel_address =  pixel_buffer_start + (y << 10) + (x << 1);
        
        *one_pixel_address = line_color;
}

//////////////////////////////////////////////////////////////////////////////






//////////////////////////////////////////////////////////////////////////////
//AUDIO//
void play_audio(int *samples, int numSamples){
    audiop->control = 0x8; //clear output FIFOS
    audiop->control = 0x0; //resume input conversion

    for (int i = 0; i < numSamples; i++){
        // output data if there is space in the output FIFOs
        //Wait for at least one empty FIFO slot
        while (audiop->wsrc == 0);

        //Output the samples
        audiop->ldata = samples[i];
        audiop->rdata = samples[i];
    }
}
