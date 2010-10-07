Generic requirements:

gcc / g++ and stuff
fluid
libfltk1.1
libfltk1.1-dev
libglut3
libglut3-dev
libboost1.37 and -dev (or higher version)

Tested on openSUSE 11.2 / SLED 11 SP1

sudo zypper install fltk-devel freeglut-devel boost-devel
cd Src
make
./repsnapper
 
On Ubuntu or similar:
sudo apt-get install build-essential libfltk1.1 libfltk1.1-dev libglut3 libglut3-dev libboost1.37-dev fluid

compile:

cd src
make

Executable will be left in the src folder. Currently there is no install rule.
