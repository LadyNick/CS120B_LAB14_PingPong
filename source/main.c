/*	Author: Nicole Navarro
 *  Partner(s) Name: 
 *	Lab Section: 21
 *	Assignment: Lab #14  Exercise #1
 *	Video Demo: https://youtu.be/gxqEj8YCxsc
 *	Exercise Description: [optional - include for your own benefit]
 *
 *	I acknowledge all content contained herein, excluding template or example
 *	code, is my own original work.
 */
#include <avr/io.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#ifdef _SIMULATE_
#include "simAVRHeader.h"
#include "scheduler.h"
#include "timer.h"
#endif

//---------------------------
//global variables
//---------------------------
unsigned char row[5] = {0xFE, 0xFD, 0xFB, 0xF7, 0xEF}; 
unsigned char update = 0;
unsigned char P1POS = 1; //get position of paddle's highest bit by pattern index
unsigned char P2AIPOS = 1; //this will get the higher part of the 3 bits position by it's pattern index
unsigned char P1UP; //A0
unsigned char P1DOWN; //A1
unsigned char P2;
unsigned char reset;  //A7 later
unsigned char P1MOVE = 0; //these will track whether or not the paddles are moving or just static
unsigned char P2MOVE = 0;
bool P2SPIN = false; //this is for activating the spin features
bool P1SPIN = false;
unsigned short ballspeed = 300; //base speed
unsigned char spin; //later for if the paddle moves when hitting ball
unsigned char currbit = 6; //Which bit in the pattern is the ball
unsigned char currow = 2; //which row the ball is in
unsigned char scoreP1 = 0;
unsigned char scoreP2 = 0;
unsigned char gamemode = 0; //later on for single or multi
unsigned char donedisplay = 1; //later for advancement 4
int direction = 2; 
unsigned char Single;
unsigned char Double;
unsigned char game = 1; //this is to determine when the ball is allowed to move and when its not
unsigned char gameend = 0; //this is for when the whole game is over to display who won
//1 is straightleft, 2 straightright, 3upright, 4 downright, 5 upleft, 6 downleft
//---------------------------
//declaring functions
//---------------------------
void transmit_data(unsigned char data, unsigned char reg);
void A2D_init();
void Set_A2D_Pin(unsigned char pinNum);
void moveball(int direction);
//--------------------------
//declaring SMs and SM functions
//--------------------------
enum Ball_States{ballposition}Ball_State;
int Ball_Tick(int Ball_State);
enum Player1_States{paddle1}Player1_State;
int Player1_Tick(int Player1_State);
enum Display_States{display, delay, clear}Display_State;
int Display_Tick(int Display_State);
enum AI_States{AIoff, AIactive}AI_State;
int AI_Tick(int AI_State);
enum Player2_States{P2off, P2active}Player2_State;
int Player2_Tick(int Player2_State);
enum Menu_States{waiting}Menu_State;
int Menu_Tick(int Menu_State);
//--------------------------

void transmit_data(unsigned char data, unsigned char reg) {
    //for some reason they values come out weird so you have to take each nibble, switch them and flip each nibble but not in the sense that you just flip 1 to 0 and 0 to 1, more like making abcd to dcba 
    data = (data & 0xF0) >> 4 | (data & 0x0F) << 4; 
    data = (data & 0xCC) >> 2 | (data & 0x33) << 2;
    data = (data & 0xAA) >> 1 | (data & 0x55) << 1;
	
    int i;
    if (reg == 1) {
        for (i = 0; i < 8 ; ++i) {
        // Sets SRCLR to 1 allowing data to be set
        // Also clears SRCLK in preparation of sending data
        PORTC = 0x08;
            // set SER = next bit of data to be sent.
            PORTC |= ((data >> i) & 0x01);
            // set SRCLK = 1. Rising edge shifts next bit of data into the shift register
            PORTC |= 0x02;  
        }
        // set RCLK = 1. 
        PORTC |= 0x04;
    }

    else if (reg == 2) {
        for (i = 0; i < 8 ; ++i) {
        // Sets SRCLR to 1 allowing data to be set
        // Also clears SRCLK in preparation of sending data
        PORTD = 0x08;
            // set SER = next bit of data to be sent.
            PORTD |= ((data >> i) & 0x01);
            // set SRCLK = 1. Rising edge shifts next bit of data into the shift register
            PORTD |= 0x02;  
        }
        // set RCLK = 1. 
        PORTD |= 0x04;
    }
}

void A2D_init() {
      ADCSRA |= (1 << ADEN) | (1 << ADSC) | (1 << ADATE);
	// ADEN: Enables analog-to-digital conversion
	// ADSC: Starts analog-to-digital conversion
	// ADATE: Enables auto-triggering, allowing for constant
	//	    analog to digital conversions.
}

// Pins on PORTA are used as input for A2D conversion
	//    The default channel is 0 (PA0)
	// The value of pinNum determines the pin on PORTA
	//    used for A2D conversion
	// Valid values range between 0 and 7, where the value
	//    represents the desired pin for A2D conversion

void Set_A2D_Pin(unsigned char pinNum) {
ADMUX = (pinNum <= 0x07) ? pinNum : ADMUX;
// Allow channel to stabilize
static unsigned char i = 0;
for ( i=0; i<15; i++ ) { asm("nop"); }
}

void moveball(int direction){
	if(direction == 1){//straightleft
		++currbit;
	}
	if(direction == 2){//straightright
		--currbit;
	}
	if(direction == 3){//upright
		--currbit;
		--currow;
		if(P2SPIN){
			--currow;	
		}
	}
	if(direction == 4){//downright
		--currbit;
		++currow;
		if(P2SPIN){
			++currow;
		}
	}
	if(direction == 5){//upleft
		++currbit;
		--currow;
		if(P1SPIN){
			--currow;
		}
	}
	if(direction == 6){//downleft
		++currbit;
		++currow;
		if(P1SPIN){
			++currow;
		}
	}
}


int Ball_Tick(int Ball_State){
	switch(Ball_State){
		case ballposition:
			if(currow == 0){
				if((currbit != 1) && (currbit != 6)){
				if(direction == 3){ direction = 4;}
				if(direction == 5){ direction = 6;}
				}
			}
			else if(currow == 4){
				if((currbit != 1) && (currbit != 6)){ 
				if(direction == 4){ direction = 3;}
				if(direction == 6){ direction = 5;}
				}
			}
			if(currbit == 1){
				P2SPIN = false; // spin from left to right reset
				if(P1MOVE == 1 ){
					if((P1POS == 0) || (P1POS == 2)){ 
					P1SPIN = true; 
					}}
				if(currow == P1POS + 1){//P1POS + 1 is the center of paddle1
					direction = 1;
					ballspeed += 50;
					if(ballspeed >= 300){ ballspeed = 300; }	
				}
				if(currow == P1POS){//P1POS is the top corner
					if(P1POS == 0){//if the paddle is in the corner, it cant bounce up so itll bounce down
						direction = 6;	
					}
					else{ direction = 5; }
					if(P1SPIN == false){ //if the spis is true I don't want it to go too fast because the matrix is small
					ballspeed -= 50;
					if(ballspeed <= 100) { ballspeed = 100; }
					}
				}
				if(currow == P1POS + 2){ //bottom corner paddle 1
					if(P1POS == 2){//if the paddle is in the bottom corner, it cant bounce down so itll bounce up
						direction = 5;
					}
					else{ direction = 6; }
					if(P1SPIN == false){ // ^^
					ballspeed -= 50;
					if(ballspeed <= 100){ ballspeed = 100; }
					}
				}
			}
			if(currbit == 6){
				P1SPIN = false; //reset spin from right to left
				if(P2MOVE == 1){ 
					if((P2AIPOS == 0) || (P2AIPOS == 2)){
						P2SPIN = true; 
					}
				}
				if(currow == P2AIPOS + 1){//same things as above but for paddle2
					direction = 2;
					if(P2SPIN == false){
					ballspeed += 50;
					if(ballspeed >=300){ ballspeed = 300; }
					}
				}
				if(currow == P2AIPOS){
					if(P2AIPOS == 0){//Same situation as above
						direction = 4;
					}
					else{ direction = 3; }
					if(P2SPIN == false){
					ballspeed -= 50;
					if(ballspeed <= 100){ ballspeed = 100;}
					}
				}
				if(currow == P2AIPOS + 2){//^^
					if(P2AIPOS == 2){
						direction = 3;
					}
					else{ direction = 4; }
					ballspeed -= 50;
					if(ballspeed <= 100){ballspeed = 100;}
				}
			}
			moveball(direction);						
			Ball_State = ballposition;
			break;
		default: Ball_State = ballposition;
	}
	return Ball_State;
}


int Player1_Tick(int Player1_State){
	switch(Player1_State){	
		case paddle1:	
			if(!P1UP && !P1DOWN){
				//no movement
				P1MOVE = 0;
			}
			else if(P1UP && P1DOWN){
				//no movement
				P1MOVE = 0;
			}
			else if(P1UP){
				//move the paddle up by 1, but check if its already at the max
				if(P1POS == 0){
					//do nothing, it's at the top edge
				}
				else{
					//move paddle up
					P1MOVE = 1;
					P1POS = P1POS - 1;
				}
			}
			else if(P1DOWN){
				//this is the only option left, but I want the option that its doing to be obvious in the code
				if(P1POS == 2){
					//do nothing, it's at the bottom edge
				}
				else{
					//move paddle1 down
					P1MOVE = 1;
					P1POS = P1POS + 1;
				}
			}
			break;
		default: Player1_State = paddle1; break;
	}
	return Player1_State;
}

int Player2_Tick(int Player2_State){
	switch(Player2_State){
		case waitingformenu:
			if((gamemode != 0) && (game == 1)){
				Player2_State = P2movement;
			}
			else{
				Player2_State = waitingformenu;
			}
			break;
		case P2movement: //if the P2 is off, it means the AI is on, option1 == AI option 2 == 2 player
			if(option == 1){
			if((rand() % 2) == 1){
				//I want it to not be too hard to beat the AI because the matrix is so small
					if(currow < P2AIPOS){
						if(P2AIPOS == 0){ 
							//do nothing
							P2MOVE = 0;
						}
						else{ 
							--P2AIPOS;
							P2MOVE = 1;
						    }		
					}
					if(currow > P2AIPOS){
						if(P2AIPOS == 2){
							//do nothing
							P2MOVE = 0;
						}
						else{ 
							++P2AIPOS; 
						    	P2MOVE = 1;
						    }
					}
				
			} }
			if(gamemode == 2){ //here goes P2 movements with double player
				
			}
			if(gameend == 1){
				Player2_State = waitingformenu;
			}
			else{
				Player2_State = P2movement;
			}
			break;
		default: Player2_State = P2off; break;
	}
	return Player2_State;
}

int Menu_Tick(int Menu_State){
	unsigned char count = 0;
	
	switch(Menu_State){
		case choose:
			if(Single){
				gamemode = 1;
			}
			if(Double){
				gamemode = 2;
			}
			if(gamemode != 0){
				Menu_State = counting;
			}
			else{
				Menu_State = choose;
			}
			break;
		case counting:
			//since this sm matches the ball speed, we will count up to 3000 ms for 3 seconds before every round start
			count += ballspeed;
			if(count >= 3000){
				Menu_State = ingame;
				P2AIPOS = 1;
				P1POS = 1;
				currbit = 1;
				currow = 2;
				direction = 2;
				ballspeed = 300;
				counting = 0;
			}
			else{
				Menu_State = counting;
			}
			break;
		case ingame:
			gameend = 0;
			game = 1;
			if((currbit <= 0) || (currbit >= 7)){
				game = 0;
				if(currbit <= 0){
					++P2score;
				}
				if(currbit >= 7){
					++P1score;
				}
			}
			if((P1score == 5) || (P2score == 5)){
				Menu_State = gameover;
			}
			else if(reset){
				Menu_State = resetsetup;
			}
			else if((currbit <= 0) || (currbit >= 7)){
				game = 0;
				Menu_State = counting;
			}
			else{
				Menu_State = ingame;
			}
			break;
		case resetsetup:
			P1score = 0;
			P2score = 0;
			Menu_State = choose;
			break;
		case gameover:
			gameend = 1;
			gamemode = 0;
			if(donedisplay){ //this ill change in advancement 4
				Menu_State = resetsetup;	
			}
			else{
				Menu_State = gameover;
			}
			break;
		default: Menu_State = choose; break;
	}
	return Menu_State;
}

int Display_Tick(int Display_State){
	switch(Display_State){
		//i have an idea so im going to ignore the computer paddle and the ball for now
		case display:
		if(game && !gameend){
			if((update >= 0) && (update < 3)){
				transmit_data(0x01, 1);
			}
			if(update == 0){
				transmit_data(row[P1POS],2);
			}
			if(update == 1){
				transmit_data(row[P1POS + 1], 2);
			}
			if(update == 2){
				transmit_data(row[P1POS + 2], 2);
			}
			if((update > 2) && (update < 6)){
			        transmit_data(0x80, 1);
			}
			if(update == 3){
				transmit_data(row[P2AIPOS], 2);
			}
			if(update == 4){
				transmit_data(row[P2AIPOS + 1], 2);
			}
			if(update == 5){
				transmit_data(row[P2AIPOS + 2], 2);
			}	 
		  	if((update == 6) && game){
				transmit_data((1 << currbit), 1);
				transmit_data(row[currow], 2);
			}
		}
		if(gameend || !game){
			transmit_data(0,1);
			transmit_data(0xFF,2);
		}
			Display_State = delay;
			break;
		case delay:
			++update;
			if(update == 7){
				update = 0;
			}
			Display_State = clear;
			break;
		case clear:
			transmit_data(0,1);
			transmit_data(0xFF, 2);
			Display_State = display;
			break;	
		//case delay: Display_State = display; break;
		default: Display_State = display; break;
	}
	return Display_State;
}


int main(void) {
    DDRD = 0xFF; PORTD = 0x00;
    DDRC = 0xFF; PORTC = 0x00;
    DDRB = 0xFF; PORTB = 0x00; //This is for the leds later for gamemode and score
    DDRA = 0x00; PORTA = 0xFF;

    static task task1, task2, task3, task4, task5;
    task *tasks[] = {&task1, &task2, &task3, &task4, &task5};
    const unsigned short numTasks = sizeof(tasks)/sizeof(task*);

    const char start = -1;
	
    //MOVE BALL + SPEED
    task1.state = start;
    task1.period = 300; //this is the base before all the ball physics
    task1.elapsedTime = task1.period;
    task1.TickFct = &Ball_Tick;
	
    //MOVE PLAYER 1
    task2.state = start;
    task2.period = 150; //base speed for how fast the user can move their paddle 
    task2.elapsedTime = task2.period;
    task2.TickFct = &Player1_Tick;
	
    //DISPLAY 
    task3.state = start;
    task3.period = 1; //constantly displaying
    task3.elapsedTime = task3.period;
    task3.TickFct = &Display_Tick;

    //PLAYER2 AI
    task4.state = start;
    task4.period = 150;
    task4.elapsedTime = task4.period;
    task4.TickFct = &Player2_Tick;

    //MENU
    task5.state = start;
    task5.period = ballspeed; //to match ball speed
    task5.elapsedTime = task5.period;
    task5.TickFct = &Menu_Tick;

    A2D_init();
    TimerSet(1);
    TimerOn();
    srand((int)time(0));
    
    while (1) {
	    Set_A2D_Pin(0);
	    P2AI = ADC;
	    task1.period = ballspeed;
	    task5.period = ballspeed;
	    P1UP = ~PINA & 0x04;
	    P1DOWN = ~PINA & 0x08;
	    reset = ~PINA & 0x10;
	    Single = ~PINA & 0x20;
	    Double = ~PINA & 040;
	    
	    for(int i=0; i<numTasks; i++){ //Scheduler code
			if(tasks[i]->elapsedTime >= tasks[i]->period){
				tasks[i]->state = tasks[i]->TickFct(tasks[i]->state);
				tasks[i]->elapsedTime = 0;
			}
			tasks[i]->elapsedTime += 1;
		}
		while(!TimerFlag);
		TimerFlag = 0;
    }
    return 1;
}
