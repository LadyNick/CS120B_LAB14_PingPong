/*	Author: lab
 *  Partner(s) Name: 
 *	Lab Section:
 *	Assignment: Lab #  Exercise #
 *	Exercise Description: [optional - include for your own benefit]
 *
 *	I acknowledge all content contained herein, excluding template or example
 *	code, is my own original work.
 */
#include <avr/io.h>
#include <stdbool.h>
#include <stdio.h>
#ifdef _SIMULATE_
#include "simAVRHeader.h"
#include "scheduler.h"
#include "timer.h"
#endif

//---------------------------
//global variables
//---------------------------
//unsigned char pattern[5] = {0x00, 0x81, 0xC1, 0x81, 0x00}; //ball will start on the left middle going to the right
unsigned char row[5] = {0xFE, 0xFD, 0xFB, 0xF7, 0xEF}; 
unsigned char update = 0;
unsigned char P1POS = 1; //get position of paddle's highest bit by pattern index
unsigned char P2AIPOS = 1; //this will get the higher part of the 3 bits position by it's pattern index
unsigned char P1UP; //A0
unsigned char P1DOWN; //A1
unsigned char P2UP;   //A2 later
unsigned char P2DOWN; //A3 later
unsigned char reset;  //A7 later
unsigned char ballspeed; //later
unsigned char spin; //later for if the paddle moves when hitting ball
unsigned char currbit = 6; //Which bit in the pattern is the ball
unsigned char currow = 2; //which row the ball is in
unsigned char score; //later on
unsigned char gamemode; //later on for single or multi
int direction = 2; 
unsigned char game = 1;
//1 is straightleft, 2 straightright, 3upright, 4 downright, 5 upleft, 6 downleft
//---------------------------
//declaring functions
//---------------------------
void transmit_data(unsigned char data, unsigned char reg);
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
	}
	if(direction == 4){//downright
		--currbit;
		++currow;
	}
	if(direction == 5){//upleft
		++currbit;
		--currow;
	}
	if(direction == 6){//downleft
		++currbit;
		++currow;
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
				if(currow == P1POS + 1){//P1POS + 1 is the center of paddle1
					direction = 1;	
				}
				if(currow == P1POS){//P1POS is the top corner
					if(P1POS == 0){//if the paddle is in the corner, it cant bounce up so itll bounce down
						direction = 6;
					}
					else{ direction = 5; }
				}
				if(currow == P1POS + 2){ //bottom corner paddle 1
					if(P1POS == 2){//if the paddle is in the bottom corner, it cant bounce down so itll bounce up
						direction = 5;
					}
					else{ direction = 6; }	
				}
			}
			if(currbit == 6){
				if(currow == P2AIPOS + 1){//same things as above but for paddle2
					direction = 2;
				}
				if(currow == P2AIPOS){
					if(P2AIPOS == 0){//Same situation as above
						direction = 4;
					}
					else{ direction = 3; }
				}
				if(currow == P2AIPOS + 2){//^^
					if(P2AIPOS == 2){
						direction = 3;
					}
					else{ direction = 4; }
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
			}
			else if(P1UP && P1DOWN){
				//no movement
			}
			else if(P1UP){
				//move the paddle up by 1, but check if its already at the max
				if(P1POS == 0){
					//do nothing, it's at the top edge
				}
				else{
					//move paddle up
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
					P1POS = P1POS + 1;
				}
			}
			break;
		default: Player1_State = paddle1; break;
	}
	return Player1_State;
}

int Player2_Tick(int Player2_State){
//	P2AIPOS = 1;
	switch(Player2_State){
		case P2active:
		//	P2AIPOS = 1;
			Player2_State = P2active;
			break;
		case P2off:
			Player2_State = P2active;
			//stuff will go here later with the menu;
			break;
		default: Player2_State = P2active; break;
	}
	return Player2_State;
}

int Menu_Tick(int Menu_State){
	switch(Menu_State){
		case waiting:
			if((currbit == 0) || (currbit == 7)){
				game = 0;
			}
			Menu_State = waiting; break;
		default: Menu_State = waiting; break;
	}
	return Menu_State;
}

int Display_Tick(int Display_State){
	switch(Display_State){
		//i have an idea so im going to ignore the computer paddle and the ball for now
		case display:
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
    task2.period = 200; //base speed for how fast the user can move their paddle 
    task2.elapsedTime = task2.period;
    task2.TickFct = &Player1_Tick;
	
    //DISPLAY 
    task3.state = start;
    task3.period = 1; //constantly displaying
    task3.elapsedTime = task3.period;
    task3.TickFct = &Display_Tick;

    //PLAYER2 AI
    task4.state = start;
    task4.period = 200;
    task4.elapsedTime = task4.period;
    task4.TickFct = &Player2_Tick;

    //MENU
    task5.state = start;
    task5.period = 300; //to match ball speed
    task5.elapsedTime = task5.period;
    task5.TickFct = &Menu_Tick;

    TimerSet(1);
    TimerOn();
    
    while (1) {

	    P1UP = ~PINA & 0x01;
	    P1DOWN = ~PINA & 0x02;
	    PORTB = P1UP;
	   // P2UP P2 will use the keypad
	   // P2DOWN 
	   // reset = ~PINA & 0x80;
	    
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
