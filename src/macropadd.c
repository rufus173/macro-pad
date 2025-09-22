#ifndef ARDUINO
#define _GNU_SOURCE
#include <sys/wait.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <sys/stat.h>
#include <pwd.h>
#include <dirent.h>
#include <sys/time.h>
//#include <asm/termbits.h>
#include <sys/ioctl.h>
#include <stdint.h>
#include <libudev.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <termios.h>
#include <dlfcn.h>

#define BUTTON(buttons,i) (!(buttons & (1 << (i-1))))
#define COMMAND_WRITE_TOP_LCD 0
#define COMMAND_WRITE_BOTTOM_LCD 1
#define COMMAND_CLEAR_LCD 2
#define MAX(a,b) (((a) > (b)) ? (a) : (b))
#define MIN(a,b) (((a) < (b)) ? (a) : (b))

struct mod {
	void *handle;
	char *path;
	time_t timestamp;
};
struct __attribute__((packed)) serial_command {
	uint8_t type; //0 for lcd write
	uint8_t length;
	char data[];
};
struct mods {
	struct mod *mods;
	size_t mod_count;
};

int open_serial_dev(char *serial, char *vendor, int o_flags);
int init_arduino_serial(int fd);
unsigned long long int time_ms();
void *dlsym_find(struct mods *modules,char *name);
int reload_modules(struct mods *modules);
struct mods *load_modules();
void free_modules(struct mods *modules);
int issue_command(int fd,int command,char *data,size_t len);

int main(int argc, char **argv){
	//====== initialise the char device and connect ======
	printf("finding device...\n");
	int mpfd = open_serial_dev("1a86_USB_Serial","1a86",O_RDWR);
	if (mpfd < 0){
		fprintf(stderr,"Could not find serial device\n");
		return 1;
	}
	printf("initialising serial...\n");
	if (init_arduino_serial(mpfd) < 0) return 1;
	//====== load symbols from shared libraries ======
	void *global_dlopen_handle = dlopen(NULL,RTLD_NOW);
	if (global_dlopen_handle == NULL){
		fprintf(stderr,"%s\n",dlerror());
		return 1;
	}
	struct mods *modules = load_modules();
	if (modules == NULL){
		fprintf(stderr,"Could not load modules\n");
		return 1;
	}
	//====== read ======
	//read some of the rubbish and discard it
	char rubbish[100];
	int result = read(mpfd,rubbish,sizeof(rubbish));
	if (result < 0){
		perror("read");
		free_modules(modules);
		return -1;
	}
	//setup
	printf("ready\n");
	time_t button_hold_lengths[8] = {0};
	unsigned long long t_start = time_ms();
	for (size_t cycle = 0;;){
		//====== read button status ======
		uint8_t buttons;
		long long int result = read(mpfd,&buttons,sizeof(uint8_t));
		if (result == 0) break;
		if (result < 0){
			perror("read");
			close(mpfd);
			return 1;
		}
		//printf("%d\n",buttons);
		//====== print data to lcd ======
		if ((cycle % 30) == 0){
			char top_message[UINT8_MAX+1] = "Active";
			char bottom_message[UINT8_MAX+1];
			time_t timestamp = time(NULL);
			struct tm *current_time = localtime(&timestamp);
			strftime(bottom_message,sizeof(bottom_message),"%H:%M",current_time);
			//issue_command(mpfd,COMMAND_CLEAR_LCD,NULL,0);
			issue_command(mpfd,COMMAND_WRITE_TOP_LCD,top_message,strlen(top_message));
			issue_command(mpfd,COMMAND_WRITE_BOTTOM_LCD,bottom_message,strlen(bottom_message));
		}
		//====== process button status ======
		unsigned long long t_now = time_ms();
		//printf("%d\n",buttons);
		for (int button = 0; button < 8; button++){
			if (BUTTON(buttons,button+1)){
				if (button_hold_lengths[button] == 0){
					//call function
					char buffer[1024];
					snprintf(buffer,sizeof(buffer),"button_%d_press",button+1);
					void (*func)(void) = dlsym_find(modules,buffer);
					if (func) func();
				}
				button_hold_lengths[button] += t_now - t_start;
			}else{
				if (button_hold_lengths[button] != 0){
					//call function
					char buffer[1024];
					snprintf(buffer,sizeof(buffer),"button_%d_release",button+1);
					void (*func)(time_t) = dlsym_find(modules,buffer);
					if (func) func(button_hold_lengths[button]);
				}
				button_hold_lengths[button] = 0;
			}
			//printf("button %d: %lu\n",button+1,button_hold_lengths[button]);
		}
		//printf("---------\n");
		//====== reload any modified modules ======
		//reload_modules(modules);
		//cant due to segmentation fault
		//====== misc cleanup ======
		waitpid(-1,NULL,WNOHANG); //wait for forks button presses may have created
		t_start = t_now;
	}
	//====== cleanup ======
	free_modules(modules);
	close(mpfd);
	return 0;
}

int open_serial_dev(char *serial, char *vendor, int o_flags){
	int result = -1;
	struct udev *udev = udev_new();
	if (udev == NULL){
		perror("udev_new");
		return -1;
	}
	//enumerate through all devices
	struct udev_enumerate *enumerate = udev_enumerate_new(udev);
	if (enumerate == NULL){
		udev_unref(udev);
		perror("udev_enumerate_new");
		return -1;
	}
	struct udev_list_entry *entry;
	if (udev_enumerate_add_match_subsystem(enumerate,"tty") >= 0){
	if (udev_enumerate_add_match_property(enumerate,"ID_SERIAL",serial) >= 0){
	if (udev_enumerate_add_match_property(enumerate,"ID_VENDOR",vendor) >= 0){
	if (udev_enumerate_add_match_is_initialized(enumerate) >= 0){
	if (udev_enumerate_scan_subsystems(enumerate) >= 0){
	if (udev_enumerate_scan_devices(enumerate) >= 0){
	if (entry = udev_enumerate_get_list_entry(enumerate)){
	//------- should be our device ------
		printf("%s - %s\n",udev_list_entry_get_name(entry),udev_list_entry_get_value(entry));
		struct udev_device *dev;
		if (dev = udev_device_new_from_syspath(udev,udev_list_entry_get_name(entry))){
			const char *path = udev_device_get_devnode(dev);
			if (path){
				printf("path: %s\n",path);
				result = open(path,o_flags);
				if (result < 0) perror("open");
			}else perror("udev_device_get_devnode");
			udev_device_unref(dev);
		}else perror("udev_device_new_from_syspath");
	//-----------------------------------
	}else perror("udev_enumerate_get_list_entry");
	}else perror("udev_enumerate_scan_devices");
	}else perror("udev_enumerate_scan_subsystems");
	}else perror("udev_enumerate_add_match_is_initialized");
	}else perror("udev_enumerate_add_match_property");
	}else perror("udev_enumerate_add_match_property");
	}else perror("udev_enumerate_add_match_subsystem");

	udev_enumerate_unref(enumerate);
	udev_unref(udev);
	return result;
}
int init_arduino_serial(int fd){
	//set baud and such
	struct termios settings;
	if (tcgetattr(fd,&settings) < 0){
		perror("tcgetattr");
		return -1;
	}
	//settings.c_iflag &= ~(BRKINT | ICRNL | IMAXBEL);
	//settings.c_oflag &= ~(OPOST);
	//settings.c_cflag &= ~(ISIG | ICANON | IEXTEN | ECHO | ECHOE | ECHOK | ECHOCTL | ECHOKE);
	//config.c_cflag |= CS8
	//settings.c_cc[VMIN] = 1;
	//settings.c_cc[VTIME] = 0;
	cfsetspeed(&settings,B9600);
	cfmakeraw(&settings);
	settings.c_cflag |= (CLOCAL | CREAD);
	settings.c_iflag &= ~(IXOFF | IXANY);
	if (tcsetattr(fd,TCSANOW,&settings) < 0){
		perror("tcsetattr");
		return -1;
	}
	//send reset
	//int rts = TIOCM_RTS;
	//int result = ioctl(fd,TIOCMBIS,&rts);
	//if (result < 0){
	//	perror("ioctl");
	//	return -1;
	//}
	return 0;
}
unsigned long long int time_ms(){
	struct timeval now;
	gettimeofday(&now,NULL);
	return now.tv_sec * 1000 + now.tv_usec / 1000;
}
struct mods *load_modules(){
	struct mods *modules = malloc(sizeof(struct mods));
	memset(modules,0,sizeof(struct mods));
	struct passwd *pw = getpwuid(getuid());
	char name_buffer[1024] = {0};
	snprintf(name_buffer,sizeof(name_buffer),"%s/.config/macropadd",pw->pw_dir);
	//sort alphabeticaly
	struct dirent **entry_list;
	int count = scandir(name_buffer,&entry_list,NULL,alphasort);
	if (count < 0){
		perror("scandir");
		free(modules);
		return NULL;
	}
	for (int i = 0; i < count; i++){
		if (entry_list[i]->d_type == DT_REG){
			snprintf(name_buffer,sizeof(name_buffer),"%s/.config/macropadd/%s",pw->pw_dir,entry_list[i]->d_name);
			void *handle = dlmopen(LM_ID_NEWLM,name_buffer,RTLD_NOW);
			if (handle == NULL){
				fprintf(stderr,"Could not open %s: %s\n",name_buffer,dlerror());
			}else{
				modules->mod_count++;
				modules->mods = realloc(modules->mods,modules->mod_count*sizeof(struct mod));
				modules->mods[modules->mod_count-1].handle = handle;
				modules->mods[modules->mod_count-1].path = strdup(name_buffer);
				struct stat statbuf = {0};
				int result = stat(name_buffer,&statbuf);
				if (result < 0) perror("stat");
				modules->mods[modules->mod_count-1].timestamp = statbuf.st_mtim.tv_sec;
				printf("loaded %s\n",name_buffer);
			}
		}
		free(entry_list[i]);
	}
	free(entry_list);
	return modules;
}
void free_modules(struct mods *modules){
	for (size_t i = 0; i < modules->mod_count; i++){
		dlclose(modules->mods[i].handle);
		free(modules->mods[i].path);
	}
	free(modules->mods);
	free(modules);
}
int reload_modules(struct mods *modules){
	for (size_t i = 0; i < modules->mod_count; i++){
		//read module timestamp
		struct mod *module = modules->mods+i;
		struct stat statbuf;
		int result = stat(module->path,&statbuf);
		if (result < 0) continue;
		if (module->timestamp < statbuf.st_mtim.tv_sec && module->handle != NULL){
			printf("Reloading module %s...\n",module->path);
			//reset timestamp
			module->timestamp = statbuf.st_mtim.tv_sec;
			dlclose(module->handle);
			void *handle = dlmopen(LM_ID_NEWLM,module->path,RTLD_NOW);
			if (handle == NULL){
				fprintf(stderr,"Error reopening module; %s\n",dlerror());
			}
			module->handle = handle;
		}
	}
	return 0;
}
void *dlsym_find(struct mods *modules,char *name){
	for (size_t i = 0; i < modules->mod_count; i++){
		struct mod *module = modules->mods + i;
		if (module->handle != NULL){
			void *func = dlsym(module->handle,name);
			if (func != NULL) return func;
		}
	}
	return NULL;
}
int issue_command(int fd,int command,char *data,size_t len){
	len = MIN(len,UINT8_MAX);
	size_t command_size = len+2;
	struct serial_command *packet = malloc(command_size);
	packet->type = command;
	packet->length = len;
	if (len > 0) memcpy(packet->data,data,len);
	long long int length = write(fd,packet,command_size);
	if (length == -1){
		perror("write");
		return -1;
	}
	return 0;
}
#endif
