#ifndef ARDUINO
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

static volatile int dpms_status = 0;

void button_1_press(){
	pid_t pid = fork();
	if (pid == 0){
		execlp("kitty","kitty",NULL);
	}
}
void button_2_press(){
	pid_t pid = fork();
	if (pid == 0){
		execlp("firefox","firefox",NULL);
	}
}
void button_3_press(){
	if (fork() == 0){
		//minimise all windows
		execlp("bluetoothctl","bluetoothctl","connect","90:7a:58:e9:9e:7b",NULL);
	}
}
void button_4_press(){
	if (fork() == 0){
		//minimise all windows
		system("hyprctl dispatch focusmonitor DVI-D-1");
		system("hyprctl dispatch workspace 20");
		system("hyprctl dispatch focusmonitor HDMI-A-1");
		system("hyprctl dispatch workspace 21");
		exit(0);
	}
}
void button_8_release(time_t msecs_held){
	if (msecs_held > 2000){
		if (fork() == 0){
			execlp("shutdown","shutdown","now",NULL);
		}
	}
}
void button_5_press(){
	const char *dpms_modes[] = {"on","off"};
	dpms_status++;
	dpms_status%=2;
	if (fork() == 0){
		//minimise all windows
		execlp("hyprctl","hyprctl","dispatch","dpms",dpms_modes[dpms_status],NULL);
	}
}
#endif
