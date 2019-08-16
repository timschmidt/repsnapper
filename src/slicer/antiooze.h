/*
    This file is a part of the RepSnapper project.
    Copyright (C) 2019  hurzl@online.de

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/
#ifndef ANTIOOZE_H
#define ANTIOOZE_H


#include "printlines.h"

class Antiooze
{
public:
    /** find next move area and apply antiooze */
    Antiooze(vector<PLine<3> *> &lines, ulong fromIndex,
             const double minlength, const double amount,
             const double aospeed, const double zLift);

    static void applyAntiooze(vector<PLine<3> *> &lines,
                              const double minlength,
                              const double amount, const double aospeed,
                              const double zLift);

private:
    vector<PLine<3> *> &lines;

    ulong movestart, moveend;
    double amount, aospeed;

    bool find_movestart(ulong from);
    bool find_moveend  (ulong from);
    bool find_moverange(ulong from, double minlength);

    uint distribute_AntioozeAmount(double amount,
                                   ulong fromIndex, ulong untilIndex);

};

#endif // ANTIOOZE_H
