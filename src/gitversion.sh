#!/bin/sh

AWK=awk

FSEQUAL="="
# test for BSD awk which needs a blackslash:
if [ "X`$AWK --version | head -n 1 | grep BSD`" != "X" ] ; then
    FSEQUAL="\="
fi


VERSION_H=gitversion.h

LOG=`git log -1`

COMMIT=`echo "$LOG" | $AWK '/commit/{print $2; exit}'`
DATE=`  echo "$LOG" | $AWK -v FS="Date:   " '/Date:/{print $2; exit}'`

OLDCOMMIT=""
if [ -e $VERSION_H ]; then
    OLDCOMMIT=`$AWK -v FS="$FSEQUAL" '/const string GIT_COMMIT=/{print $2}' $VERSION_H`
fi

if [ "x\"$COMMIT\";" == "x$OLDCOMMIT" ]; then
    echo "Commit has not changed"
else
    echo "Updating commit"
    echo const string GIT_COMMIT=\"$COMMIT\"\;     > $VERSION_H
    echo const string GIT_COMMIT_DATE=\"$DATE\"\; >> $VERSION_H
fi
