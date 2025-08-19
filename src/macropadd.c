#include <stdio.h>
#include <dlfcn.h>
#include <sys/time.h>
//#include <asm/termbits.h>
#include <sys/ioctl.h>
#include <stdint.h>
#include <libudev.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <termios.h>

#define BUTTON(buttons,i) (!(buttons & (1 << (i-1))))

struct mods {
	void **handles;
	size_t handle_count;
	time_t *timestamps;
};

int open_serial_dev(char *serial, char *vendor, int o_flags);
int init_arduino_serial(int fd);
unsigned long long int time_ms();

int main(int argc, char **argv){
	//====== initialise the char device and connect ======
	printf("finding device...\n");
	int mpfd = open_serial_dev("1a86_USB_Serial","1a86",O_RDONLY);
	if (mpfd < 0){
		fprintf(stderr,"Could not find serial device\n");
		return 1;
	}
	printf("initialising serial...\n");
	if (init_arduino_serial(mpfd) < 0) return 1;
	//====== load symbols from shared libraries ======
	struct mods *modules = load_modules();
	//====== read ======
	printf("ready\n");
	time_t button_hold_lengths[8] = {0};
	for (;;){
		unsigned long long t_start = time_ms();
		uint8_t buttons;
		int result = read(mpfd,&buttons,sizeof(uint8_t));
		if (result == 0) break;
		if (result < 0){
			perror("read");
			close(mpfd);
			return 1;
		}
		unsigned long long t_now = time_ms();
		printf("%d\n",buttons);
		for (int button = 0; button < 8; button++){
			if (BUTTON(buttons,button+1)){
				if (button_hold_lengths[button] == 0){
					//call function
					char buffer[1024];
					snprintf(buffer,sizeof(buffer),"button_%d_press",button+1);
					void (*function)(void) = dlsym(RTLD_NEXT,buffer);
					if (function) function();
				}
				button_hold_lengths[button] += t_now - t_start;
			}else{
				if (button_hold_lengths[button] != 0){
					//call function
					char buffer[1024];
					snprintf(buffer,sizeof(buffer),"button_%d_release",button+1);
					void (*function)(unsigned long long) = dlsym(RTLD_NEXT,buffer);
					if (function) function(button_hold_lengths[button]);
				}
				button_hold_lengths[button] = 0;
			}
			printf("button %d: %lu\n",button+1,button_hold_lengths[button]);
		}
		printf("---------\n");
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
	//cfsetspeed(&settings,B9600);
	cfmakeraw(&settings);
	settings.c_cflag |= (CLOCAL | CREAD);
	settings.c_iflag &= ~(IXOFF | IXANY);
	if (tcsetattr(fd,TCSANOW,&settings) < 0){
		perror("tcsetattr");
		return -1;
	}
	//send reset
	int rts = TIOCM_RTS;
	int result = ioctl(fd,TIOCMBIS,&rts);
	if (result < 0){
		perror("ioctl");
		return -1;
	}
	return 0;
}
unsigned long long int time_ms(){
	struct timeval now;
	gettimeofday(&now,NULL);
	return now.tv_sec * 1000 + now.tv_usec / 1000;
}
struct mods *load_modules(){
	struct mods *modules = malloc(sizeof(struct mods));
	struct passwd *pw = getpwuid(getuid());
	char buffer[1024] = {0};
	snprintf(buffer,sizeof(buffer),"%s/.config/macropadd",pw->pw_dir);
	DIR *mod_dir = opendir(buffer);
	if (mod_dir == NULL){
		perror("opendir");
		free(modules);
		return NULL;
	}
	closedir(mod_dir);
	return modules;
}
void free_modules(struct mods *modules){
	free(modules);
}
