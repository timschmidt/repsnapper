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
#include <algorithm>
#include "antiooze.h"

#define AODEBUG

Antiooze::Antiooze(vector<PLine<3> *> &plines,
                   ulong fromIndex, double minlength,
                   const double amount, const double speed,
                   const double zLift, bool distribute)
    : lines(plines), movestart(0), moveend(0), amount(amount), aospeed(speed)
{
    if (find_moverange(fromIndex, minlength)) {
        if (distribute) {
            // do repush first to keep indices before right
            uint added = distribute_AntioozeAmount(amount, movestart, moveend+1);
            moveend += added;
            added = distribute_AntioozeAmount(-amount, fromIndex, movestart-1);
            moveend += added;
            movestart += added;
        } else {
            uint added = insert_AntioozeHalt(amount, moveend+1, -zLift);
            if (added == 1){
                added = insert_AntioozeHalt(-amount, movestart, zLift);
                moveend += added;
                movestart += added;
            }
        }
        // lift move-only range
        if (zLift > 0)
            for (ulong i = movestart; i <= moveend; i++)
                lines[i]->lifted = zLift;
#ifdef AODEBUG
//        cerr << " added " << (lines.size() - numlines) << endl;
#endif
    } else {
        moveend = 0;
    }
}

void Antiooze::applyAntiooze(vector<PLine<3> *> &lines,
                             const double minlength,
                             const double amount, const double speed,
                             const double zLift,
                             bool distribute)
{
    ulong lastend = 0;
    bool ok = true;
    while (ok) {
        Antiooze ao(lines, lastend+1, minlength, amount, speed, zLift, distribute);
        ok = ao.moveend > 0;
        lastend = ao.moveend;
    }
}

bool Antiooze::find_movestart(ulong from)
{
  ulong i = from;
  movestart = from;
  ulong num_lines = lines.size();
  while (i < num_lines-1 && (!lines[i]->is_move() || lines[i]->is_command()) ) {
      i++;
      movestart = i;
  }
  while (movestart < num_lines-1 && lines[movestart]->is_command())
      movestart++;
  if (!lines[movestart]->is_move()) return false;
  if (movestart == num_lines-1) return false;
  return true;
}

bool Antiooze::find_moveend(ulong from)
{
    ulong i = from;
    moveend = i;
    ulong num_lines = lines.size();
    if (num_lines < 2) return false;
    while (i < num_lines && (lines[i]->is_move() || lines[i]->is_command() ) ) {
        moveend = i;
        i++;
    }
    while (moveend>0 && lines[moveend]->is_command())
        moveend--;
    if (!lines[moveend]->is_move()) return false;
    if (moveend > num_lines-1) moveend = num_lines-1;
    return true;

}

bool Antiooze::find_moverange(ulong startindex, double minlength)
{
    ulong i = startindex;
    ulong num_lines = lines.size();
    if (num_lines < 2) return false;
    while (i < num_lines-2) {
      if (find_movestart(i)) {        // find move start
          if (!find_moveend(movestart)) {// find move end
              i = movestart+1;
              continue;
          }
          if ( Printlines::length(lines, movestart, moveend) >= minlength )
              return true;
      }
      else return false; // not found
      i = moveend+1; // not long enough, continue search
    }
    return false;
}

uint  Antiooze::distribute_AntioozeAmount(double amount,
                                          ulong fromIndex, ulong untilIndex)
{
    // slow down if total lines time is not enough
    // for total AO amount at AO speed
    double aotimeneeded =abs(amount) / aospeed;
    double linestime = 0;
    for (ulong i = fromIndex; i <= untilIndex; i++) {
        linestime += lines[i]->time();
    }
    if (linestime < aotimeneeded) {
        double factor = linestime / aotimeneeded;
        linestime = 0;
        for (ulong i = fromIndex; i <= untilIndex; i++) {
            lines[i]->speed *= factor;
            linestime += lines[i]->time();
        }
    }

    // distribute
    double rest = amount;
    double distributed = 0.;
    ulong i = untilIndex;
    uint added = 0;
    while (abs(rest) > 0) {//&& i >= fromIndex) {
        const double lineamount = lines[i]->time() * aospeed * (amount < 0 ? -1. : 1.)
                - lines[i]->absolute_extrusion;
        if (abs(rest) < abs(lineamount)) {
            //split line
            const double fraction = 1. - rest/lineamount; // add to second half
            if (fraction > 0.05 && fraction < 0.95) {
                const double lenbefore = lines[i]->length();
                const int sbefore = lines.size();
                added = Printlines::divideline(i, fraction * lenbefore, lines);
                if (added == 1) {
//                    cerr << "added 1? "<< sbefore << " -> " << lines.size() << endl ;
                    if (abs(lines[i]->length()  + lines[i+1]->length() - lenbefore)>0.1) {
                        cerr << "error " << "frac " <<  fraction << " * " << lenbefore
                             << " != " << lines[i]->length()
                             << " + " << lines[i+1]->length() << endl;
                    }
                }
            }
            lines[i+added]->absolute_extrusion += rest;
            distributed += rest;
            rest = 0;
            break;
        } else {
            lines[i]->absolute_extrusion += lineamount;
            distributed += lineamount;
            rest -= lineamount;
        }
        if (i==0) break;
        if (i>lines.size()) break;
        i--;
    }
#ifdef AODEBUG
    if (abs(distributed-amount) > 0.001) {
        double absex = 0.;
        for (ulong j = fromIndex; j <= untilIndex; j++) {
            absex += lines[j]->absolute_extrusion;
        }
        cerr << (i+1) << "--" << untilIndex << " \t total " << absex
             << " of " << amount << endl;
        cerr << "error " << distributed << " != " << amount << " rest " << rest << endl;
    }
#endif
    return added;
}

uint Antiooze::insert_AntioozeHalt(double amount, ulong atIndex, double zLift)
{
    if (atIndex >= lines.size()) return 0;
    Vector3d where = lines[atIndex]->from;
    const double speed = zLift==0. ? aospeed : abs(zLift*aospeed/amount);
    PLine3 *halt = new PLine3(lines[atIndex]->area, lines[atIndex]->extruder_no,
                              where, where + Vector3d(0.,0.,zLift), speed, 0);
    if (zLift < 0) halt->lifted = -zLift; // make endpoint lifted
    halt->addAbsoluteExtrusionAmount(amount, aospeed);
    lines.insert(lines.begin() + long(atIndex), halt);
    return 1;
}
