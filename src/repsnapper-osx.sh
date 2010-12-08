#!/bin/bash

# get actual path to .app
script_dir_link=$(dirname "$(readlink "$0")")
if [[ $script_dir_link == "." ]]; then
APP_PATH=$(dirname "$0")
else
APP_PATH=$script_dir_link
fi

# set place to load dylibs from
DYLD_FALLBACK_LIBRARY_PATH=$APP_PATH/lib

# run the program itself
$APP_PATH/repsnapper
