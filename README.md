# wlr-autorotate

**wlr-autorotate** is a simple daemon which listens for accelerometer events sent by net.hadess.SensorProxy (iio-sensor-proxy) and changes screen orientation in wlroots based wayland compositors with the changes on sensor.

By the way, this is my first project in C

## Building

### Dependencies:

`iio-sensor-proxy` is a runtime dependencies and is not needed to build

Arch Linux:
```sh
sudo pacman -Sy glib2 wayland
```

After installing all required dependencies run
```sh
make all
```
Created binary will be located in `build/wlr-autorotate`