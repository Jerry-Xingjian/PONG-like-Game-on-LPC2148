#include <LPC214x.H>
// "Pong"-like Game

int data, x, y, v, matrixScanLine;
unsigned int t1, t2;
unsigned long int JoyState;
int matrixData[8];
int matrixScan[8];
int matrixLine[3];
int ball_track_val[11] = {0x40,0x40,0x40,0x40,0x20,0x10,0x08,0x04,0x02,0x02,0x01};
int ball_track_row[11] = {7,6,5,4,4,4,4,3,2,1,0};
int ball_up_val[8] = {128, 64, 32, 16, 8, 4, 2, 1};
char welcome[] = {'"', 'P', 'O', 'N', 'G', '"', '-', 'l', 'i', 'k', 'e', ' ', 'G', 'a', 'm', 'e'};
char score[] = {'S', 'C', 'O', 'R', 'E', ':'};
char win[] = {'C', 'O', 'N', 'G', 'R', 'A', 'T', 'U', 'L', 'A', 'T', 'I', 'O', 'N'};
char lose[] = {'Y', 'O', 'U', ' ', 'L', 'O', 'S', 'E'};

// LED Screen Definition
int RS = 1 << 24;	/* Bit mask for RS pin on LCD = P1.24 */
int E =  1 << 25;	/* Bit mask for E pin on LCD = P1.25 */
int RW = 1 << 22;	/* Bit mask for RW pin on LCD = P0.22 */
int Backlight = (1<<30);	/* Bit mask for Backlight pin on LCD = P0.30 */
int DL = (1<<4);	/* Bit position for DL bit in Function Set Command */
int N = (1<<3);		/* 2-line if 1 */
int F = (0<<2);		/* 5 * 10 if 1 */
int I_D = (0<<1);	/* cursor blink move to right if 1 */
int SH = (0<<0);	/* shift entire display if 1 */
int D = (1<<2);		/* Display On/Off Command */
int C = (0<<1);		/* Cursor on/off*/
int B = (1<<0);		/* Blink */
int FUNC = (1<<5);	/* Function Mode Set Command */
int ENTRY = (1<<2);	/* Entry Mode Set Command */
int DISP = (1<<3);	/* Display On/Off Command) */
int CLEAR_DISP = 0x01;	/* Clear LCD Display Command */

#define J_Press !(JoyState&0x00010000) 
#define J_Up !(JoyState&0x00020000) 
#define J_Down !(JoyState&0x00100000) 
#define J_Left !(JoyState&0x00080000)
#define J_Right !(JoyState&0x00040000)

void JoyInit(void){ // Joystick initialization
	IODIR0 &= ~0x1F4000;
	JoyState=0;
}

void JoyChange(void){ // Joystick change
	JoyState=IOPIN0;
}

int random(t1, t2) // random number generation
{
	unsigned int b;
	b = t1 ^ (t1 >> 2) ^ (t1 >> 6) ^ (t1 >> 7);
	t1 = (t1 >> 1) | (~b << 31);
	b = (t2 << 1) ^ (t2 << 2) ^ (t1 << 3) ^ (t2 << 4);
	t2 = (t2 << 1 ) | (~b >> 31);
	return (t1 ^ t2) % 8;
}

void initMatrix(void){ // LED Matrix initialization
	int i;
	IODIR0 |= (1 << 15);
	IOSET0 = (1 << 15);
	PINSEL0 |= (1 << 8) | (1 << 10) | (1 << 12);
	S0SPCR = (0 << 2) | (1 << 3) | (1 << 4) | (1 << 5) | (0 << 8); 
	S0SPCCR = 64;
	for(i=0;i<8;i++) matrixData[i] = 0xFF;
	matrixScanLine=0;
	matrixScan[0]=0x7F;
	matrixScan[1]=0xBF;
	matrixScan[2]=0xDF;
	matrixScan[3]=0xEF;
	matrixScan[4]=0xF7;
	matrixScan[5]=0xFB;
	matrixScan[6]=0xFD;
	matrixScan[7]=0xFE;
}
void displayMatrix(void){ // Matrix display
	IOSET0 = 0x00008000;
	S0SPDR = matrixScan[matrixScanLine];
	while ((S0SPSR & 0x80) == 0);
	S0SPDR = matrixData[matrixScanLine];
	while ((S0SPSR & 0x80) == 0);
	IOCLR0 = 0x00008000;
	
	matrixScanLine = (matrixScanLine+1) % 8;
}
void setMatrixRow(int y, int data){ // Set the matrix row 
	matrixData[y] = ~data;
	matrixScanLine = y;
}

void delay(unsigned int x, int y) // Used for matrix display delay
{
	unsigned int i, j;
	for(i=0; i<x; i++)
		for(j=0; j<y;j++);
}

void delayUs(unsigned int delayInus) // Used for LCD screen delay
{

			// Setup timer #1 for delay in the order of microseconds 
  T1TCR = 0x02;          					// Stop and reset timer 
  T1PR  = 0x00;										// Set prescaler to zero
  T1MR0 = delayInus * 15;					// Based upon Pclk = 15 MHz.
  T1IR  = 0xff;										// Reset all the interrupt flags
  T1MCR = 0x04;										// Stop the timer on a match
  T1TCR = 0x01;										// Start the timer now
  
			// Wait until the timer has timed out TCR[0] will be set 0 when MR[2] = 1
  while (T1TCR & 0x01);
}

void LCD_CommandWrite (unsigned int Command)
	{
		IOCLR1 = RS;
		IOCLR1 = RW;	
		IOSET1 = Command << 16;
		IOSET1 = E;
		IOCLR1 = E;
		delayUs(4000);
		IOCLR1 = 0xff << 16;
	} 

void LCD_DataWrite (unsigned int Character)
	{ 
		IOSET1 = RS;
		IOCLR1 = RW;
		IOSET1 = Character << 16;
		IOSET1 = E;
		IOCLR1 = E;
		delayUs(100);
		IOCLR1 = 0xff << 16;
	} 
		

void LCD_Init (void)
	{
		LCD_CommandWrite (FUNC | DL | N);	// Send Function Set command with 8-bit I/F, 2 lines, 5x7 font 
		LCD_CommandWrite (DISP | D | C);	// Send Display On/Off command with Display ON, Cursor ON, Blink OFF 
		LCD_CommandWrite (ENTRY | I_D);		// Send Entry Mode Set command with Increment Selected 
	}

void wait (void) { // designed for step motor
	int d;
	int speed = 60000;
	for(d = 0; d < speed; d++);
}
	
void forward (void) { // Step motor move forward
	unsigned int phase_AB = (1 << 12 | 1 << 21);
	unsigned int phase_A = (1 << 12);
	IOSET0 = phase_A;
	wait();
	IOSET0 = phase_AB;
	wait();
	IOCLR0 = phase_A;
	wait();
	IOCLR0 = phase_AB;
	wait();
}

void reverse (void) { // Step motor move backward
	unsigned int phase_AB = (1 << 12 | 1 << 21);
	unsigned int phase_B = (1 << 21);
	IOSET0 = phase_B;
	wait();
	IOSET0 = phase_AB;
	wait();
	IOCLR0 = phase_B;
	wait();
	IOCLR0 = phase_AB;
	wait();
}

int main(void)
{	int count = 1; // Ball movement count
	int row_up, row_down, pos, temp, i;
	int row_val = ball_up_val[4]; // Ball value init
	int row_val_temp = 0;
	int count_score_tens = 0; // Tens digit
	int count_score_unit = 0; // Unit digit
	
	unsigned int counter, step; 
	unsigned int phase_AB = (1 << 12 | 1 << 21); // Step motor phase
	
	step = 50;
	IODIR0 = phase_AB; // Output pin for servo
	IOCLR0 = phase_AB; // Clear phase_AB
	
	initMatrix(); // LED matrix initialization
	delay(10,10);
	JoyInit(); // Joystick initialization
	
	PINSEL1 = 0x05000000;					// Define A/D pins: P0.28, P0.29 = AIN1, AIN2 
	PINSEL2 = 0x00000000;					// P1.0 - P1.36 as GPIO 

	IODIR0 = (255<<8 | 1<<21 | 1<<22 | 1<<30);	// P0.8 - P0.15 as output - LEDs P0.22 P0.30 as Outputs - LCD 'R/W' and Backlight
																							// P0.12 and P0.21 as outputs for stepper motor
	IODIR1 = (255<<16 | 1<<24 | 1<<25);		// P1.16 - P1.23 as outputs: LCD Data...P1.24 as output: RS...P1.25 as output E:
	IOSET0 = Backlight;	// Turn on the LCD Backlight
	
	while(1){
		JoyChange();
		delayUs(1000);	// Delay 1 ms to give LCD controller time to intialize internally 
		LCD_Init();	// Initialize the LCD Display to our operating conditions 
		IOSET0 = Backlight;	// Turn on the LCD Backlight 

		LCD_CommandWrite (CLEAR_DISP);	// Clear the display 
		//LCD_DataWrite (welcome[0]);	// Write a 'S' onto the LCD display in 1st location 
		for(i = 0; i < 16; i++){
			LCD_DataWrite (welcome[i]);	// Write a ':' onto the LCD display in 3rd location
		}
		if(J_Press) break;
	}
	
	
	while(1){
		pos = 240;	// postion of the ball
		row_up = 7; // ball start up row
		row_down = 2; // ball move down row
		
		delayUs(8000);	// Delay 8 ms to give LCD controller time to intialize internally 
		LCD_Init();	// Initialize the LCD Display to our operating conditions 
		IOSET0 = Backlight;	// Turn on the LCD Backlight 

		LCD_CommandWrite (CLEAR_DISP);	// Clear the display 
		for(i = 0; i < 6; i++){
			LCD_DataWrite (score[i]);	// Write a ':' onto the LCD display in 3rd location
		}
			
		
		if(row_val_temp != 0) {row_val = ball_up_val[row_val_temp];} // Ball position randomly determined
		else {row_val = ball_up_val[4];} // If not, set the start position to the middle of the bottom row
		while(row_up >= 0){ // pay attention, might need modified
			JoyChange();
			if(J_Right && count != 16){ // Joystick to the right
			count = count * 2;}
			else if(J_Left && count != 1){ // Joystick to the left
			count = count / 2;}
			
			// Paddle position
			setMatrixRow(0,pos/count); 
			temp = pos/count;
			displayMatrix();
			
			// Ball position
			setMatrixRow(row_up,row_val);
			delay(175,175); // Little delay
			displayMatrix();
			
			row_up = row_up - 1;
			// If else function used to randomly generate ball path
			if((count+random(row_up,row_val))%2==0 && row_val != 128) {row_val = row_val * 2; row_val_temp = random(count, row_up);}
			else if((row_up+random(row_up,row_val))%2==1 && row_val != 1) {row_val = row_val / 2; row_val_temp = random(count, row_up);}
			else if(row_up == 0){row_val = row_val; row_val_temp = random(count, row_up);}
			delay(1000,1000);
		}
		//Long if statement to determine whether the ball hit the paddle or not
		if((row_val==128&&temp==240)||(row_val==64&&temp==240)||(row_val==64&&temp==120)||(row_val==32&&temp==240)||(row_val==32&&temp==120)||(row_val==32&&temp==60)||(row_val==16&&temp==240)||(row_val==16&&temp==120)||(row_val==16&&temp==60)||(row_val==16&&temp==30)||(row_val==8&&temp==120)||(row_val==8&&temp==60)||(row_val==8&&temp==30)||(row_val==8&&temp==15)||(row_val==4&&temp==60)||(row_val==4&&temp==30)||(row_val==4&&temp==15)||(row_val==2&&temp==30)||(row_val==2&&temp==15)||(row_val==1&&temp==15)){
			if(count_score_unit < 9) { // For unit count
				count_score_unit++;
				count_score_unit = '0' + count_score_unit; // Convert to character
				count_score_tens = '0' + count_score_tens;
				LCD_DataWrite (count_score_tens);
				LCD_DataWrite (count_score_unit);
			}
			else{ // For tens count
				count_score_unit = 0;
				count_score_tens = count_score_tens + 1;
				count_score_unit = '0' + count_score_unit;
				count_score_tens = '0' + count_score_tens;
				LCD_DataWrite (count_score_tens);
				LCD_DataWrite (count_score_unit);
			}
			delay(100,100);
			count_score_unit = count_score_unit - '0'; // Transform back to integer
			count_score_tens = count_score_tens - '0'; // Transfor back to integer
		}
		else {break;} // If lose, jump outside of the while loop.
		if((count_score_tens == 0) && (count_score_unit == 5)) {break;} // If win, jump outside of the while loop.
		
		while(row_down != 8){ // Ball move downward
			JoyChange();
			if(J_Right && count != 16){ // Joystick to right
			count = count * 2;}
			else if(J_Left && count != 1){ // Joystick to Left
			count = count / 2;}

			// Paddle position
			setMatrixRow(0,pos/count);
			displayMatrix();
			
			// Ball position
			setMatrixRow(row_down,row_val);
			delay(175,175);
			displayMatrix();
			
			delay(1000,1000);
			row_down = row_down + 1;
			// If else determine ball backward path
			if((count+row_down)%2==0 && row_val != 128) {row_val = row_val * 2;} 
			else if((count+row_down)%2==1 && row_val != 1) {row_val = row_val / 2;}
			else {row_val = row_val;}
		}
		}
		
		// LCD result display
		LCD_CommandWrite (CLEAR_DISP); // Clear the display
		if((count_score_tens == 0) && (count_score_unit == 5)) {
			// Win
			while(1){
				for(i = 0; i < 14; i++){
						LCD_DataWrite (win[i]);	// Write "CONGRATULATION" onto the LCD display screen
				}
				
				while(1){
						for (counter = 0; counter < step; counter++){ // Step motor move forward
						forward();
						}
						for (counter = 0; counter < step; counter++){ // Step motor move backward
						reverse();
						}
				};
		}}
		// Lose
		while(1){
			for(i = 0; i < 8; i++){
				LCD_DataWrite (lose[i]);	// Write "YOU LOSE" onto the LCD display
			}
			while(1);
		}
}
	
