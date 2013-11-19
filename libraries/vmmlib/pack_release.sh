#!/bin/bash
#
# @author Susanne Suter


if [ -z "$1" ]; then
  echo "usage: ./"$0" [release-tag]";
  exit 1;
fi


export EXCLUDING_PATTERN="*.sw* *.svn* *.DS_Store* *.o*"
export INCLUDE_DIR="include/vmmlib"
export TESTS_DIR="tests"

export README_FILES="README AUTHORS LICENSE.txt ACKNOWLEDGEMENTS"
export PROJECT_FILES="vmmlib.xcodeproj Makefile Makefile.atlas CMakeLists.txt"
export RELEASE_NUM=$1

echo "finished init"

################
# ZIP CREATION #
################
zip_release() {

  # zipping the include and test files excluding VIM
  # swap files and SVN directories
  zip -x ${EXCLUDING_PATTERN} \
      -r ../vmmlib-${RELEASE_NUM}.zip \
      ${INCLUDE_DIR} \
      ${TESTS_DIR} \
      ${README_FILES} \
      ${PROJECT_FILES}
}


################
# TGZ CREATION #
################
tgz_release() {

  # tar gzipping the include and test files excluding VIM
  # swap files and SVN directories
  tar cvfz \
      ../vmmlib-${RELEASE_NUM}.tgz \
	  --exclude="*.o" \
	  --exclude="*.DS_Store" \
	  --exclude="*.svn*" \
          --transform "s|^|vmmlib-${RELEASE_NUM}/|" \
      ${INCLUDE_DIR} \
      ${TESTS_DIR} \
      ${README_FILES} \
      ${PROJECT_FILES}
}


##########
# CALLS ##
##########

zip_release

echo "finished zipping"

tgz_release

echo "finished tgzipping"


