# raspi2fb
Program to copy the Raspberry Pi display to a secondary framebuffer.

# usage

    raspi2fb <options>

    --daemon - start in the background as a daemon
    --device <device> - framebuffer device (default /dev/fb1)
    --display <number> - Raspberry Pi display number (default 0)
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

    sudo make install
    sudo cp ../raspi2fb.init.d /etc/init.d/raspi2fb
    sudo update-rc.d raspi2fb defaults
    sudo service raspi2fb start

#uninstall

    sudo service raspi2fb stop
    sudo update-rc.d -f raspi2fb remove
    sudo rm /usr/local/bin/raspi2fb
    sudo rm /etc/init.d/raspi2fb

