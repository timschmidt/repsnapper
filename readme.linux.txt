Generic requirements:

gcc / g++ and stuff
fluid
libfltk1.1
libfltk1.1-dev
libglut3
libglut3-dev
libboost1.37 and -dev (or higher version)
libconfig++8-dev and libconfig-dev

On Ubuntu or similar:
sudo apt-get install build-essential libfltk1.1 libfltk1.1-dev libglut3 libglut3-dev libboost1.37-dev fluid libconfig++8-dev libconfig-dev

compile:

cd src
make

Executable will be left in the src folder. Currently there is no install rule.
