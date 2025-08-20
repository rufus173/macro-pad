#include <stdio.h>
#include <unistd.h>
void button_1_press(){
	pid_t pid = fork();
	if (pid != 0){
		execlp("xeyes","xeyes",NULL);
	}
}
void button_2_press(){
	printf("button 2 pressed\n");
}
