CFLAGS=-Wall -g -Wextra
all : macropadd actions.so
actions.so : src/actions.c
	gcc $(CFLAGS) -shared -o $@ $^
macropadd : src/macropadd.c
	gcc $(CFLAGS) `pkg-config --cflags libudev` -o $@ $^ `pkg-config --libs libudev`
rebuild:
	arduino-cli compile --fqbn esp32:esp32:esp32
