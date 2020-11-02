# raspi2fb
Program to copy the Raspberry Pi display to a secondary framebuffer.
# usage

    raspi2fb <options>

    --daemon - start in the background as a daemon
    --device <device> - framebuffer device (default /dev/fb1)
    --display <number> - Raspberry Pi display number (default 0)
    --fps <fps> - set desired frames per second (default 10 frames per second)
    --copyrect - copy only a rectangle the same size as the dest framebuffer
    --rectx <x> - copy rectangle from source fb at <x> in copyrect mode (default 0)
    --recty <y> - copy rectangle from source fb at <y> in copyrect mode (default 0)
    --pidfile <pidfile> - create and lock PID file (if being run as a daemon)
    --once - copy only one time, then exit
    --help - print usage and exit

# build prerequisites
## cmake
You will need to install cmake

    sudo apt-get install cmake
## libraries
You will need to install libbsd-dev

    sudo apt-get install libbsd-dev
# build
    mkdir build
    cd build
    cmake ..
    make
# install
## Raspian Wheezy
    sudo make install
    sudo cp ../raspi2fb.init.d /etc/init.d/raspi2fb
    sudo update-rc.d raspi2fb defaults
    sudo service raspi2fb start
## Raspian Jessie
    sudo make install
    sudo cp ../raspi2fb@.service /etc/systemd/system/
    sudo systemctl daemon-reload
    sudo systemctl enable raspi2fb@1.service
    sudo systemctl start raspi2fb@1
# uninstall
## Raspian Wheezy
    sudo service raspi2fb stop
    sudo update-rc.d -f raspi2fb remove
    sudo rm /usr/local/bin/raspi2fb
    sudo rm /etc/init.d/raspi2fb
## Raspian Jessie
    sudo systemctl stop raspi2fb@1
    sudo systemctl disable raspi2fb@1.service
    sudo rm /etc/systemd/system/raspi2fb@.service
    sudo rm /usr/local/bin/raspi2fb
