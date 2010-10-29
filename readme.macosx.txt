Build Steps:

1.) Install XCode for gcc stuff http://developer.apple.com/technologies/xcode.html

2.) Install MacPorts from http://www.macports.org/
    # run from a terminal window:
    $ sudo port install fltk glut libconfig-hr boost

3.) Build RepSnapper
    $ cd path/to/repsnapper
    $ cd ./Src
    $ make

Executable (repsnapper) will be left in the src folder. Currently there is no install rule.
