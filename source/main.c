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
unsigned char pattern[5] = {0x00, 0x81, 0xC1, 0x81, 0x00}; //ball will start on the left middle going to the right
unsigned char row[5] = {0xFE, 0xFD, 0xFB, 0xF7, 0xEF}; 
unsigned char update = 0;
unsigned char P1POS; //get position of paddle's highest bit by pattern index
unsigned char P2AIPOS; //this will get the higher part of the 3 bits position by it's pattern index
unsigned char P1UP; //A0
unsigned char P1DOWN; //A1
unsigned char P2UP;   //A2 later
unsigned char P2DOWN; //A3 later
unsigned char reset;  //A7 later
unsigned char ballspeed; //later
unsigned char spin; //later for if the paddle moves when hitting ball
unsigned char currbit; //Which bit in the pattern is the ball
unsigned char currrow; //which row the ball is in
unsigned char score; //later on
unsigned char gamemode; //later on for single or multi
int direction;
//---------------------------
//declaring functions
//---------------------------
void transmit_data(unsigned char data, unsigned char reg);
void moveball(int direction);
unsigned char SetBit(unsigned char x, unsigned char k, unsigned char b);
unsigned char GetBit(unsigned char x, unsigned char k);
bool checkcenter(int centerbit); //these are to check which part of the paddle the ball hits
bool checktop(int topbit); 
bool checkbot(int botbit);
void paddleup(int player);
void paddledown(int player);
bool checkfirst();//this checks for the top row, so if it hits top row it bounces off
bool checklast();//this checks for bottom row, so if it hits the bottom row it bounces off
//--------------------------
//declaring SMs and SM functions
//--------------------------
enum Ball_States{ballposition}Ball_State;
int Ball_Tick(int Ball_State);
enum Player1_States{paddle1}Player1_State;
int Player1_Tick(int Player1_State);
enum Display_States{display, delay}Display_State;
int Display_Tick(int Display_State);
enum AI_States{AIoff, AIactive}AI_State;
int AI_Tick(int AI_State);
enum Player2_States{P2off, P2active}Player2_State;
int Player2_Tick(int Player2_State);
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

//void moveball(int direction);

unsigned char SetBit(unsigned char x, unsigned char k, unsigned char b){
	return(b ? (x | (0x01 << k)) : (x & ~(0x01 << k)) );
}

unsigned char GetBit(unsigned char x, unsigned char k){
	return ((x & (0x01 << k)) != 0);
}

//bool checkcenter(int centerbit);
//bool checktop(int topbit); 
//bool checkbot(int botbit);

void paddleup(int player){
	//clear & replace the paddle
	if(player == 1){
		//clear p1 paddle
		for(int i = 0; i < 5; ++i){
			pattern[i] = SetBit(pattern[i], 0, 0);	
		}
		//set p1 paddle
		pattern[P1POS - 1] = SetBit(pattern[P1POS - 1], 0, 1);
		pattern[P1POS] = SetBit(pattern[P1POS], 0, 1);
		pattern[P1POS + 1] = SetBit(pattern[P1POS + 1], 0, 1);
	}
	if(player == 2){
		//clear p2 paddle
		for(int i = 0; i < 5; ++i){
			pattern[i] = SetBit(pattern[i], 7, 0);	
		}
		//set p2 paddle
		pattern[P2AIPOS - 1] = SetBit(pattern[P2AIPOS - 1], 7, 1);
		pattern[P2AIPOS] = SetBit(pattern[P2AIPOS], 7, 1);
		pattern[P2AIPOS + 1] = SetBit(pattern[P2AIPOS + 1], 7, 1);
	}	
}

void paddledown(int player){
	//Clear & replace paddle
	if(player == 1){
		//clear p1 paddle
		for(int i = 0; i < 5; ++i){
			pattern[i] = SetBit(pattern[i], 0, 0);	
		}
		//set p1 paddle
		pattern[P1POS + 1] = SetBit(pattern[P1POS + 1], 0, 1);
		pattern[P1POS + 2] = SetBit(pattern[P1POS + 2], 0, 1);
		pattern[P1POS + 3] = SetBit(pattern[P1POS + 3], 0, 1);
	}
	if(player == 2){
		//clear p2 paddle
		for(int i = 0; i < 5; ++i){
			pattern[i] = SetBit(pattern[i], 7, 0);	
		}
		//set p2 paddle
		pattern[P2AIPOS + 1] = SetBit(pattern[P2AIPOS + 1], 7, 1);
		pattern[P2AIPOS + 2] = SetBit(pattern[P2AIPOS + 2], 7, 1);
		pattern[P2AIPOS + 3] = SetBit(pattern[P2AIPOS + 3], 7, 1);
	}
}

//bool checkfirst();
//bool checklast();

int Ball_Tick(int Ball_State){
	/*switch(Ball_State){

	}*/
	return Ball_State;
}


int Player1_Tick(int Player1_State){
	unsigned char got1 = 0; //for when position is determined
	
	switch(Player1_State){	
		case paddle1:
			//small segment to set P1POS
			for(int i = 0; i < 5; ++i){
				if(got1 == 1){
					//do nothing, position found
				}
				else if(GetBit(pattern[i],0)){
					P1POS = i;
					got1 = 1;
				}
			}
			got1 = 0;	
				
			
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
					paddleup(1);
				}
			}
			else if(P1DOWN){
				//this is the only option left, but I want the option that its doing to be obvious in the code
				if(P1POS == 4){
					//do nothing, it's at the bottom edge
				}
				else{
					//move paddle1 down
					paddledown(1);
				}
			}
			break;
		default: Player1_State = paddle1; break;
	}
	return Player1_State;
}


int Display_Tick(int Display_State){
	switch(Display_State){
		case display:
			transmit_data(pattern[update],1);
			transmit_data(row[update], 2);
			Display_State = delay;
			break;
		case delay:
			++update;
			if(update == 5){
				update = 0;
			}
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

    static task task1, task2, task3;
    task *tasks[] = {&task1, &task2, &task3};
    const unsigned short numTasks = sizeof(tasks)/sizeof(task*);

    const char start = -1;
	
    //MOVE BALL + SPEED
    task1.state = start;
    task1.period = 300; //this is the base before all the ball physics
    task1.elapsedTime = task1.period;
    task1.TickFct = &Ball_Tick;
	
    //MOVE PLAYER 1
    task2.state = start;
    task2.period = 100; //base speed for how fast the user can move their paddle 
    task2.elapsedTime = task2.period;
    task2.TickFct = &Player1_Tick;
	
    //DISPLAY 
    task3.state = start;
    task3.period = 2; //constantly displaying
    task3.elapsedTime = task2.period;
    task3.TickFct = &Display_Tick;

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
