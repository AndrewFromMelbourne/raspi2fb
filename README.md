# raspi2fb
Program to copy the Raspberry Pi display to a secondary framebuffer.

# usage

    raspi2fb <options>

    --daemon - start in the background as a daemon
    --device - framebuffer device (default /dev/fb1)
    --fps <fps> - set desired frames per second (default 10 frames per second)
    --pidfile <pidfile> - create and lock PID file (if being run as a daemon)
    --help - print usage and exit


# libraries

You will need to install libbsd-dev

    sudo apt-get install libbsd-dev

# build

    mkdir build
    cd build
    cmake ..
    make

#install

    to be completed ...
