// cc -o servo-controller servo-controller.c -lwiringPi -lm
#include <stdio.h>
#include <wiringPi.h>
#include <wiringPiI2C.h>
#include <math.h>

// cc -o controller controller.c
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/joystick.h>

#include "controller.h"


struct ps3ctls {
	
	int fd;
	unsigned char nr_buttons;	// Max number of Buttons
	unsigned char nr_sticks;	// Max number of Sticks
	short *button;			// button[nr_buttons]
	short *stick;			// stick[nr_sticks]
};

int fds;
#define saki3o 47
#define saki4o 52
#define saki5o 3
int saki1  = 0;
int saki2  = 0;
int saki3  = saki3o;
int saki4  = saki4o;
int saki5  = saki5o;

int resetPCA9685(int fd) {
	wiringPiI2CWriteReg8(fd,0,0);
}

int setPCA9685Freq(int fd , float freq) {
	float prescaleval;
	int prescale , oldmode , newmode;
	freq = 0.9 * freq;
	prescaleval = 25000000.0;
	prescaleval /= 4096.0;
	prescaleval /= freq;
	prescaleval -= 1.0;
	prescale = prescaleval + 0.5;
	oldmode = wiringPiI2CReadReg8(fd,0x00);
	newmode = (oldmode & 0x7F)|0x10;
	wiringPiI2CWriteReg8(fd , 0x00 , newmode);
	wiringPiI2CWriteReg8(fd , 0xFE , prescale);
	wiringPiI2CWriteReg8(fd , 0x00 , oldmode);
	sleep(0.005);
	wiringPiI2CWriteReg8(fd , 0x00 , oldmode | 0xA1);
}


int setPCA9685Duty(int fd , int channel , int off) {
	int channelpos;
	int on;
	
	on   = 0;
	off += 276;
	channelpos = 0x6 + 4 * channel;
	wiringPiI2CWriteReg16(fd , channelpos   , on  & 0x0FFF);
	wiringPiI2CWriteReg16(fd , channelpos+2 , off & 0x0FFF);
}


int ps3c_test(struct ps3ctls *ps3dat) {

	unsigned char i;
	unsigned char nr_btn = ps3dat->nr_buttons;
	unsigned char nr_stk = ps3dat->nr_sticks;
	int x,y;
	int x2,y2;

//	printf("%d %d\n",nr_btn,nr_stk);

	printf(" 1=%2d ",ps3dat->button[PAD_KEY_LEFT]);
	printf(" 2=%2d ",ps3dat->button[PAD_KEY_RIGHT]);
	printf(" 3=%2d ",ps3dat->button[PAD_KEY_UP]);
	printf(" 4=%2d ",ps3dat->button[PAD_KEY_DOWN]);
	printf(" 5=%4d ",ps3dat->stick [PAD_LEFT_X]);
	printf(" 6=%4d ",ps3dat->stick [PAD_LEFT_Y]);
	printf(" 7=%4d ",ps3dat->stick [PAD_RIGHT_X]);
	printf(" 8=%4d ",ps3dat->stick [PAD_RIGHT_Y]);
	printf("\n");

	if(ps3dat->button[PAD_KEY_CROSS]==1) return -1; // end of program



	setPCA9685Duty(fds , 0 , ps3dat->stick [PAD_LEFT_X]);
	setPCA9685Duty(fds , 1 , ps3dat->stick [PAD_RIGHT_X]-30);
	setPCA9685Duty(fds , 2 , ps3dat->stick [PAD_RIGHT_X]);
//	// y=0.0013x^2-1.0769x-5.3594
//	//setPCA9685Duty(fds , 1 , ps3dat->stick [PAD_LEFT_X]);
//	x = ps3dat->stick [PAD_LEFT_X];
//	y = 0.0013*x*x-1.0769*x-5.3594; test
//	setPCA9685Duty(fds , 1 , y);
//
//	if(ps3dat->button[PAD_KEY_LEFT] ) saki=32;
//	if(ps3dat->button[PAD_KEY_UP])    saki=52;
//	if(ps3dat->button[PAD_KEY_RIGHT]) saki=72;
//	setPCA9685Duty(fds , 4 , saki);
//	
//	x2 = ps3dat->stick [PAD_RIGHT_X];
//	y2 = -2.0*0.00001*x2*x2-0.935*x2+7;
//	setPCA9685Duty(fds , 2 , x2);
//	setPCA9685Duty(fds , 3 , y2);

//	if(ps3dat->button[PAD_KEY_UP])    {saki3=saki3o;   saki4=saki4o;   };

//	if(ps3dat->button[PAD_KEY_RIGHT]) {saki1++;if(saki1>+160) saki1=+160;};
//	if(ps3dat->button[PAD_KEY_LEFT] ) {saki1--;if(saki1<-160) saki1=-160;};
//	setPCA9685Duty(fds , 2 , saki1);

	if(ps3dat->button[PAD_KEY_SQUARE]) saki5=saki5o-10; 
	if(ps3dat->button[PAD_KEY_CIRCLE]) saki5=saki5o+10;
	if(ps3dat->button[PAD_KEY_TRIANGLE]) saki5=saki5o+3;
	if(ps3dat->button[PAD_KEY_SQUARE]+ps3dat->button[PAD_KEY_CIRCLE]==0) saki5=saki5o;
	setPCA9685Duty(fds , 4 , saki5);

	return 0;
}


int ps3c_input(struct ps3ctls *ps3dat) {

	int rp;
	struct js_event ev;

	do {
		rp = read(ps3dat->fd, &ev, sizeof(struct js_event));
		if (rp != sizeof(struct js_event)) {
			return -1;
		}
	} while (ev.type & JS_EVENT_INIT);

	switch (ev.type) {
		case JS_EVENT_BUTTON:
			if (ev.number < ps3dat->nr_buttons) {
				ps3dat->button[ev.number] = ev.value;
			}
			break;
		case JS_EVENT_AXIS:
			if (ev.number < ps3dat->nr_sticks) {
				ps3dat->stick[ev.number] = ev.value / 200; //327 range -32767 ~ +32768 -> -100 ~ +100
			}
			break;
		default:
			break;
	}

	return 0;
}


int ps3c_getinfo(struct ps3ctls *ps3dat) {

	if(ioctl(ps3dat->fd , JSIOCGBUTTONS , &ps3dat->nr_buttons) < 0) return -1;
	if(ioctl(ps3dat->fd , JSIOCGAXES    , &ps3dat->nr_sticks ) < 0) return -2;

	return 0;
}


int ps3c_init(struct ps3ctls *ps3dat, const char *df) {

	unsigned char nr_btn;
	unsigned char nr_stk;
	unsigned char *p;
	int i;

	ps3dat->fd = open(df, O_RDONLY);
	if (ps3dat->fd < 0) return -1;

	if (ps3c_getinfo(ps3dat) < 0) {
		close(ps3dat->fd);
		return -2;
	}

	nr_btn = ps3dat->nr_buttons;
	nr_stk = ps3dat->nr_sticks;
//	printf("%d %d\n",nr_btn,nr_stk);

	p = calloc(nr_btn + nr_stk , sizeof(short));
	if (p == NULL) {
		close(ps3dat->fd);
		return -3;
	}
	ps3dat->button = (short *)p;
	ps3dat->stick  = (short *)&p[nr_btn * sizeof(short)];
	
//	for (i = 0; i<nr_btn; i++) ps3dat->button[i] = 0;
//	for (i = 0; i<nr_stk; i++) ps3dat->stick [i] = 0;
//	ps3dat->button[PAD_KEY_LEFT]=0;
//	ps3dat->button[PAD_KEY_RIGHT]=0;
//	ps3dat->button[PAD_KEY_UP]=0;;
//	ps3dat->button[PAD_KEY_DOWN]=0;
//	ps3dat->stick [PAD_LEFT_X]=0;
//	ps3dat->stick [PAD_LEFT_Y]=0;
//	ps3dat->stick [PAD_RIGHT_X]=0;
//	ps3dat->stick [PAD_RIGHT_Y]=0;

	return 0;
}

void ps3c_exit   (struct ps3ctls *ps3dat) {

	free (ps3dat->button);
	close(ps3dat->fd);
}


void main() {

	char *df = "/dev/input/js0";
	struct ps3ctls ps3dat;

	fds = wiringPiI2CSetup(0x40);	// PCA9685
	resetPCA9685(fds);
	setPCA9685Freq(fds,50);

	if(!(ps3c_init(&ps3dat, df))) {

		while(1) do {
			if (ps3c_test(&ps3dat) < 0) break;
		} while (!(ps3c_input(&ps3dat)));
		
		ps3c_exit(&ps3dat);		
	}
}
