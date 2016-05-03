#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/joystick.h>


struct ps3Ctls {
	int fd;
	unsigned char nr_buttons;
	unsigned char nr_sticks;
	short *button;	// button[nr_buttons]
	short *stick;		// stick[nr_sticks]
	char  *input;		// input[nr_btn * 2 + nr_stk * 7 + 1]
};


int joystick_output(struct ps3Ctls *ps3dat) {

	unsigned char i, nr_btn, nr_stk;
	long t;

	nr_btn = ps3dat->nr_buttons;
	nr_stk = ps3dat->nr_sticks;

//	printf("btn %d %d\n",nr_btn,nr_stk);

	for (i = 0; i < nr_btn; i++) {
		sprintf(&ps3dat->input[i * 2], "%2d", ps3dat->button[i] ? 1 : 0);
	}
//	for (i = 0; i < nr_stk; i++) {
//		t = ps3dat->stick[i] / 327;
//		sprintf(&ps3dat->input[nr_btn * 2 + i * 7], "%7d", t);
//	}

	ps3dat->input[nr_btn * 2] = '\0';
//	ps3dat->input[nr_btn * 2 + nr_stk * 7] = '\0';

//	fprintf(fp, "%s\n", ps3dat->input);
//	printf("%s\n", ps3dat->input);
	printf("%2d\n",ps3dat->button[0]);

	return 0;
}


int joystick_input(struct ps3Ctls *ps3dat) {

	int ret;
	struct js_event ev;
	unsigned char num;

	do {
		ret = read(ps3dat->fd, &ev, sizeof(struct js_event));
		if (ret != sizeof(struct js_event)) {
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
				ps3dat->stick[num] = ev.value;
			}
			break;
		default:
			break;
	}

	return 0;
}


int joystick_getinfo(struct ps3Ctls *ps3dat) {

	int fd = ps3dat->fd;

	if(ioctl(fd,JSIOCGBUTTONS,&ps3dat->nr_buttons) < 0) return -1;
	if(ioctl(fd,JSIOCGAXES   ,&ps3dat->nr_sticks ) < 0) return -2;

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

	p = malloc((nr_btn + nr_stk) * sizeof(short) + nr_btn * 2 + nr_stk * 7 + 1);
	if (p == NULL) {
		close(ps3dat->fd);
		return -2;
	}
	ps3dat->button = (short *)p;
	ps3dat->stick  = (short *)&p[nr_btn * sizeof(short)];
	ps3dat->input  = (char  *)&p[(nr_btn + nr_stk) * sizeof(short)];

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
			if (joystick_output(&ps3dat) < 0) break;
		} while (!(joystick_input(&ps3dat)));
		
		joystick_exit(&ps3dat);		
	}
}
