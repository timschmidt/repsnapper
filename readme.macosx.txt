Tested on iMac with Leopard (10.5)

Build Steps:

1.) Install XCode for gcc stuff http://developer.apple.com/technologies/xcode.html

2.) Install MacPorts from http://www.macports.org/
    # run from a terminal window:
    $ sudo port install fltk glut

3.) Download and build boost >= 1.43 http://www.boost.org
    # the macports boost version doesn't seem to work properly with the asio serial code
    # set the target install directory to RepSnapper/Libraries
    $ cd to path/to/boost_1_43_0
    $ ./bootstrap.sh --prefix=/path/to/RepSnapper/Libraries/boost-darwin
    $ ./bjam --layout=versioned variant=release link=static threading=multi install
    # optionally compile the debug versions
    $ ./bjam --layout=versioned variant=debug link=static threading=multi install

4.) Build RepSnapper
    $ cd path/to/RepSnapper
    $ cd ./Src
    $ make

Executable (repsnapper) will be left in the src folder. Currently there is no install rule.
