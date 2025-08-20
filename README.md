
# Driver

The driver sits as a systemd user service, that connects to the macro pad and listens.
Bindings are done through shared objects located in `~/.config/macropadd/`. When it starts, It will load all objects in that directory. On button 1 being pressed, it will resolve `button_1_press` from the libraries in alphabetical order, and call the first one it finds.
Functions should be called `button_%d_press(void)` or `button_%d_release(time_t time_held_in_ms)`. Feel free to call `fork()` for a long running function or to `exec()`, as the driver will wait for any child processes.
Example actions are in `src/actions.c`

# Install
(Do not do as root)
`make all`
`./configure`
`./install`
`./configure-systemd`
