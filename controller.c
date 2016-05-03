#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/joystick.h>

#include "controller.h"


struct ps3Ctls {
	int fd;
	unsigned char nr_buttons;	// Max number of Buttons
	unsigned char nr_sticks;	// Max number of Sticks
	short *button;			// button[nr_buttons]
	short *stick;			// stick[nr_sticks]
};


int joystick_test(struct ps3Ctls *ps3dat) {

	unsigned char i, nr_btn, nr_stk;

	nr_btn = ps3dat->nr_buttons;
	nr_stk = ps3dat->nr_sticks;

	printf(" 1=%2d ",ps3dat->button[PAD_KEY_LEFT]);
	printf(" 2=%2d ",ps3dat->button[PAD_KEY_RIGHT]);
	printf(" 3=%2d ",ps3dat->button[PAD_KEY_UP]);
	printf(" 4=%2d ",ps3dat->button[PAD_KEY_DOWN]);
	printf(" 5=%4d ",ps3dat->stick [PAD_LEFT_X]);
	printf(" 6=%4d ",ps3dat->stick [PAD_LEFT_Y]);
	printf(" 7=%4d ",ps3dat->stick [PAD_RIGHT_X]);
	printf(" 8=%4d ",ps3dat->stick [PAD_RIGHT_Y]);
	printf("\n");

	return 0;
}


int joystick_input(struct ps3Ctls *ps3dat) {

	int rp;
	struct js_event ev;
	unsigned char num;

	do {
		rp = read(ps3dat->fd, &ev, sizeof(struct js_event));
		if (rp != sizeof(struct js_event)) {
			return -1;
		}
	} while (ev.type & JS_EVENT_INIT);

	num = ev.number;

	switch (ev.type) {
		case JS_EVENT_BUTTON:
			if (num < ps3dat->nr_buttons) {
				ps3dat->button[num] = ev.value;
			}
			break;
		case JS_EVENT_AXIS:
			if (num < ps3dat->nr_sticks) {
				ps3dat->stick[num] = ev.value / 327; // range -32767 ~ +32768 -> -100 ~ +100
			}
			break;
		default:
			break;
	}

	return 0;
}


int joystick_getinfo(struct ps3Ctls *ps3dat) {

	if(ioctl(ps3dat->fd , JSIOCGBUTTONS , &ps3dat->nr_buttons) < 0) return -1;
	if(ioctl(ps3dat->fd , JSIOCGAXES    , &ps3dat->nr_sticks ) < 0) return -2;

	return 0;
}


int joystick_init(struct ps3Ctls *ps3dat, const char *df) {

	unsigned char nr_btn, nr_stk;
	unsigned char *p;

	ps3dat->fd = open(df, O_RDONLY);
	if (ps3dat->fd < 0) return -1;

	if (joystick_getinfo(ps3dat) < 0) {
		close(ps3dat->fd);
		return -2;
	}

	nr_btn = ps3dat->nr_buttons;
	nr_stk = ps3dat->nr_sticks;

	p = malloc((nr_btn + nr_stk) * sizeof(short));
	if (p == NULL) {
		close(ps3dat->fd);
		return -3;
	}
	ps3dat->button = (short *)p;
	ps3dat->stick  = (short *)&p[nr_btn * sizeof(short)];

	return 0;
}

void joystick_exit   (struct ps3Ctls *ps3dat) {

	free (ps3dat->button);
	close(ps3dat->fd);
}


void main() {

	char *df = "/dev/input/js0";
	struct ps3Ctls ps3dat;

	if(!(joystick_init(&ps3dat, df))) {

		do {
			if (joystick_test(&ps3dat) < 0) break;
		} while (!(joystick_input(&ps3dat)));
		
		joystick_exit(&ps3dat);		
	}
}
