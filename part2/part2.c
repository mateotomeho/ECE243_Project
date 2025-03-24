//Part2 of Project ECE243: Towers of Hanoi

/*Goal 

Finish the game logic movement 
Connect the game with the PS/2 Keyboard 
If time left, add the Score
For the game complexity, make a hard mode with 4 disks

*/

#include <stdbool.h>
#include <stdlib.h> //to use randomness ran() function

#define LEDR_BASE			0xFF200000
#define SW_BASE				0xFF200040
#define KEY_BASE			0xFF200050
#define TIMER_BASE			0xFF202000
#define AUDIO_BASE			0xFF203040
#define PS2_BASE			0xFF200100
#define HEX3_HEX0_BASE	    0xFF200020
#define HEX5_HEX4_BASE		0xFF200030


volatile int pixel_buffer_start; // global variable
short int Buffer1[240][512]; // 240 rows, 512 (320 + padding) columns
short int Buffer2[240][512];

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

int column0[3] = {0};
int column1[3] = {0};
int column2[3] = {0};
int rod_positions[3] = {55, 159, 263};

int column0_hard[4] = {0};
int column1_hard[4] = {0};
int column2_hard[4] = {0};


int N = 3; //number of Disks
int num_move = 0; //number of Moves

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
void display_hex(int num);


int main(void){
    volatile int * pixel_ctrl_ptr = (int *)0xFF203020;
    //volatile int * ps2_ptr = (volatile int *) PS2_BASE;
    volatile int * KEY_ptr = (volatile int *) KEY_BASE;
    volatile int * SW_ptr = (volatile int *) SW_BASE;

    int mode = (*SW_ptr) & 0b1000000000; //Get SW[9]
    if (mode == 0){
        N = 3; //easy mode
    } else {
        N = 4; //hard mode
    }


    struct disk_info disks[N]; //Create an array of disks of struct disk_info


                                  //White,  Red,   Green, Blue,  Cyan,   Magenta, Yellow, Orange, Purple, 
    short int colour_array[9] = {0xFFFF, 0xF800, 0x07E0, 0x001F, 0x07FF, 0xF81F, 0xFFE0, 0xFC60, 0x780F}; 
  
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
	

    if (mode == 0) {
        //Intialize the size
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
    } else {
        //Intialize the size
        disks[0].size = 30;  // Very Small (Yellow)
        disks[1].size = 50;  // Small (Blue)
        disks[2].size = 70;  // Medium (Green)
        disks[3].size = 90;  // Large (Red)

        //Initialize the colour
        disks[0].colour = colour_array[6];  // Very Small (Yellow)
        disks[1].colour = colour_array[4];  // Small (Blue)
        disks[2].colour = colour_array[2];  // Medium (Green)
        disks[3].colour = colour_array[1];  // Large (Red)

        //Initialize the column
        for (int i = 0; i < N; i++) {
            column0_hard[i] = disks[i].size;
        }
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
        /* Erase any disk and lines that were drawn in the last iteration */
		draw(disks, KEY_ptr, SW_ptr);
		

        // code for drawing the disk and lines (not shown)
        // code for updating the locations of disk (not shown)

        wait_for_vsync(); // swap front and back buffers on VGA vertical sync
        pixel_buffer_start = *(pixel_ctrl_ptr + 1); // new back buffer
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

    //Draw each disk
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

        SW_value = (*SW_ptr) & 0b1111; //Get the value of the first 4 SW
        direction_rods(disks, SW_value, bit_3210);
    }

    //Display the num of move
    display_hex(num_move);
}
/////////////////////////////////////////////////////////////////////////////////////////
void display_hex(int num){
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
	for(int row = 120; row<=225; row++){
		draw_line(53, row, 57, row, 0xFFFF);
		draw_line(157, row, 161, row, 0xFFFF);
		draw_line(261, row, 265, row, 0xFFFF);
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
    } else { //Hard mode
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
        while (i < 4 && column[i] == 0) {
            i++;
        }

        //If i = 3 then the entire column is full of 0 == empty
        //If i < 3 then there is a item in the column
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
    } else { //Hard mode
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
    } else { //hard mode
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
            } else if (current->size == 30){
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
