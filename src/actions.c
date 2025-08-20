#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
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
