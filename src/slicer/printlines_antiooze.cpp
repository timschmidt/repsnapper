/*
    This file is a part of the RepSnapper project.
    Copyright (C) 2011-12  martin.dieringer@gmx.de

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

#include "printlines.h"
#include "ui/progress.h"

#define AODEBUG 0

#if AODEBUG
void test_range(uint movestart,  uint moveend,
		uint tractstart, uint pushend,
		const vector<PLine3> &lines)
{
  ostringstream o;
  bool error=false;
  for (uint i = tractstart; i < movestart; i++)
    if (lines[i].is_command()) o << "C";
    else if (lines[i].is_move()) {
      error = true;
      //cerr <<movestart << lines[i].info() << endl;
      o << "_";
    }
    else o << "+"; // ok
  o << "|";
  for (uint i = movestart; i<=moveend; i++)
    if (lines[i].is_command()) o << "C";
    else if (lines[i].is_move()) o << "-"; //ok
    else {
      error = true;
      //cerr <<lines[i].info() << endl;
      o << "!";
    }
  o << "|";
  for (uint i = moveend+1; i<=pushend; i++)
    if (lines[i].is_command()) o << "C";
    else if (lines[i].is_move()) {
      error = true;
      //cerr <<lines[i].info() << endl;
      o << "_";
    }
    else o << "+"; //ok
  o << endl;
  if (error) cerr << o.str() << endl;
}
#endif


inline bool move_start(uint from, uint &movestart, const vector<PLine3> &lines)
{
  uint i = from;
  movestart = from;
  uint num_lines = lines.size();
  while (i < num_lines && (!lines[i].is_move() || lines[i].is_command()) ) {
    i++;
    movestart = i;
  }
  while (movestart < num_lines-1 && lines[movestart].is_command()) movestart++;
  if (!lines[movestart].is_move()) return false;
  if (movestart == num_lines-1) return false;
  return true;
}

inline bool move_end(uint from, uint &moveend, const vector<PLine3> &lines)
{
  uint i = from;
  moveend = i;
  uint num_lines = lines.size();
  while (i < num_lines && (lines[i].is_move() || lines[i].is_command() ) ) {
    moveend = i;
    i++;
  }
  while (moveend>0 && lines[moveend].is_command()) moveend--;
  if (!lines[moveend].is_move()) return false;
  if (moveend > num_lines-1) moveend = num_lines-1;
  return true;
}

inline bool find_moverange(double minlength, uint startindex,
			   uint &movestart,  uint &moveend,
			   const vector<PLine3> &lines)
{
  uint i = startindex;
  uint num_lines = lines.size();
  while (i < num_lines-2) {
    if (move_start (i, movestart, lines)) {        // find move start
      if (!move_end (movestart, moveend, lines)) {// find move end
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

// find ranges for retract and repush
bool Printlines::find_nextmoves(double minlength, uint startindex,
				uint &movestart,  uint &moveend,
				uint &tractstart, uint &pushend,
				const vector<PLine3> &lines)
{
  if (!find_moverange(minlength, startindex, movestart,  moveend, lines)) return false;
  uint num_lines = lines.size();

  // find previous move
  if (movestart == 0) tractstart = 0;
  else {
    int i = movestart-1;
    tractstart = movestart;
    while ( i >= (int)startindex
	    && ( !(lines[i].is_move() || lines[i].has_absolute_extrusion())
		 || lines[i].is_command() )) {
      tractstart = i; i--;
    }
  }
  while (tractstart < num_lines-1 && lines[tractstart].is_command()) tractstart++;

  // find next move after
  if (moveend == num_lines-1) pushend = moveend;
  else {
    uint i = moveend+1;
    pushend = moveend;
    while ( i < num_lines && (!(lines[i].is_move() || lines[i].has_absolute_extrusion())
			      || lines[i].is_command()) ) {
      pushend = i; i++;
    }
  }
  while (pushend > 0 && lines[pushend].is_command()) pushend--;

#if AODEBUG
  test_range(movestart,moveend,tractstart,pushend, lines);
#endif
  // cerr << "found move " << tractstart << "..." <<movestart
  //      << "--"<<moveend<< "..."<< pushend  << endl;
  return true;
}


uint Printlines::insertAntioozeHaltBefore(uint index, double amount, double AOspeed,
					  vector< PLine3 > &lines)
{
  Vector3d where;
  if (index > lines.size()) return 0;
  if (index == lines.size()) where = lines.back().to;
  else where = lines[index].from;
  PLine3 halt (lines[index].area, lines.front().extruder_no,
	       where, where, AOspeed, 0);
  halt.addAbsoluteExtrusionAmount(amount, AOspeed);
  lines.insert(lines.begin()+index, halt); // (inserts before)
  return 1;
}


#if AODEBUG
static int distCase = 0;
#endif

int Printlines::distribute_AntioozeAmount(double AOamount, double AOspeed,
					  uint fromline, uint &toline,
					  vector< PLine3 > &lines,
					  double &havedistributed)
{
  if (AOamount == 0) return 0;
  bool negative = (AOamount < 0);
  bool reverse  = (toline < fromline); // begin distribution at end of range

  // CASE 1: no lines to distribute the amount could be found
  // negative means retract which normally means reverse distribution
  // if this is not the case, no lines were found
  if (negative != reverse) {
    uint added = 0;
    uint at_line = fromline;
#if AODEBUG
    distCase = 1;
#endif
    if (!reverse) {  // retract case // add retracting halt _after_ fromline
#if AODEBUG
      distCase = -1;
#endif
      at_line++;
    }
    toline = at_line;
    //cerr << "halt at " << at_line <<" - " <<toline<< endl;
    added = insertAntioozeHaltBefore(at_line, AOamount, AOspeed, lines);
    if (added == 1) havedistributed += AOamount;
#if AODEBUG
    else cerr << "no AO on halt possible!" << endl;
#endif
    return added;
  }

#if AODEBUG
  assert(AOspeed > 0);
#endif
  double AOtime = abs(AOamount) / AOspeed; // time needed for AO on move
  // time all lines normally need:
  double linestime = Printlines::time(lines, fromline, toline);


  // CASE 2: simple case, fit AOamount exactly into whole range while slowing down:
  if (linestime > 0 && linestime <= AOtime) {
#if AODEBUG
    distCase = 2;
#endif
    for (uint i=fromline; i<=toline; i++) {
      double ratio = lines[i].time() / linestime;
      lines[i].addAbsoluteExtrusionAmount(AOamount * ratio, AOspeed);// will slow line down
#if AODEBUG
      havedistributed += AOamount * ratio;
#endif
    }
    return 0;
  }

#if AODEBUG
  distCase = 3;
#endif

  // CASE 3: distribute on the needed part of possible range
  // distribute in range fromline--toline
  int di = ( reverse ? -1 : 1);
  double restamount = AOamount;
  const double sign = (negative?-1.:1.);
  const double signedSpeed = AOspeed * sign;
  int i;
  for (i = (int)fromline; i != (int)toline+di; i+=di) {
    if (i<0) break;
    double lineamount = lines[i].addMaxAbsoluteExtrusionAmount(signedSpeed); // +-
    havedistributed += lineamount;
    restamount -= lineamount;
    if (restamount * sign < 0)
      break;
  }
  // restamount is negative -> done too much
  lines[i].absolute_extrusion += restamount;
  havedistributed += restamount;
  uint added = 0;
  // now split last line (i)
#if 1
  const double line_ex = lines[i].absolute_extrusion;
  const double neededtime = abs(line_ex / AOspeed);
  double fraction = neededtime/lines[i].time();
  if (fraction < 1) {
    if (reverse) fraction = 1-fraction;
    added = divideline(i, fraction * lines[i].length(), lines);
    if (added == 1) {
      if (reverse) {
	lines[i].absolute_extrusion   = 0;
	lines[i+1].absolute_extrusion = line_ex;
      } else {
	lines[i].absolute_extrusion   = line_ex;
	lines[i+1].absolute_extrusion = 0;
      }
    }
  }
#endif
  toline = i + added;
  //  cerr << "rest " << AOamount - havedistributed << endl;
  return added;
}



uint Printlines::makeAntioozeRetract(vector<PLine3> &lines,
				     const Settings &settings,
				     ViewProgress * progress)
{
  if (!settings.Extruder.EnableAntiooze) return 0;


  double
    AOmindistance = settings.Extruder.AntioozeDistance,
    AOamount      = settings.Extruder.AntioozeAmount,
    AOspeed       = settings.Extruder.AntioozeSpeed * 60;
    //AOonhaltratio = settings.Slicing.AntioozeHaltRatio;
  if (lines.size() < 2 || AOmindistance <=0 || AOamount == 0) return 0;
  // const double onhalt_amount = AOamount * AOonhaltratio;
  // const double onmove_amount = AOamount - onhalt_amount;

  uint linescount = lines.size();
  if (progress) progress->restart (_("Antiooze Retract"), linescount);

  uint total_added = 0;
#if AODEBUG
  double total_extrusionsum = 0;
  double total_ext = total_Extrusion(lines);
  double total_rel = total_rel_Extrusion(lines);
#endif
  uint
    movestart  = 0, moveend = 0, // move-only range
    tractstart = 0, pushend = 0; // ends of ranges of retract and repush

  uint count = 0;
  while ( find_nextmoves(AOmindistance,
			 pushend+1, // set
			 movestart,  moveend, // get
			 tractstart, pushend, // get
			 lines) ) {

    if (movestart > linescount-1) break;

    if (progress){
      if (count%20 == 0)
	if (!progress->update(movestart)) break;
    }
    count++;

    uint added = 0;
    if (moveend > lines.size()-2) moveend = lines.size()-2;

    // lift move-only range
    if (settings.Extruder.AntioozeZlift > 0)
      for (uint i = movestart; i <= moveend; i++) {
	lines[i].lifted = settings.Extruder.AntioozeZlift;
      }

    // do repush first to keep indices before right
    double havedist = 0;
    uint newl = distribute_AntioozeAmount(AOamount, AOspeed,
					  moveend+1, pushend,
					  lines, havedist);
    added += newl;
    pushend += newl;
    //test_range(movestart,moveend,tractstart,pushend, lines);

#if AODEBUG
    double extrusionsum = 0;
    double linesext = 0;
    for (uint i = moveend+1; i<=pushend; i++) linesext+=lines[i].absolute_extrusion;
    if (abs(linesext-AOamount)>0.01) cerr  << "wrong lines dist push " << linesext << endl;
    extrusionsum += havedist;
    if (abs(havedist-AOamount)>0.01) cerr << " wrong distrib push " << havedist << endl;
#endif

    // find lines to distribute retract
    if (movestart < 1) movestart = 1;
#if AODEBUG
    double linesextbefore = 0;
    for (uint i = tractstart; i < movestart; i++)
      linesextbefore += lines[i].absolute_extrusion;
#endif
    havedist = 0;
    newl = distribute_AntioozeAmount(-AOamount, AOspeed,
				     movestart-1, tractstart,
				     lines, havedist);
    added += newl;
    movestart += newl;
    moveend += newl;
    pushend += newl;
    total_added += added;
#if AODEBUG
    linesext = -linesextbefore;
    for (uint i = tractstart; i < movestart; i++)
      linesext += lines[i].absolute_extrusion;
    if (abs(linesext+AOamount)>0.01)
      cerr  << "wrong lines dist tract " << distCase  << " : "<<linesextbefore << " : "<<linesext << " (" << havedist << ") != "  << -AOamount
	    << " - " << tractstart << "->" <<  movestart
	    << " new: "<< newl << " -- before: "<<linesextbefore<< endl;
    // else
    //   cerr << "dist tract ok: " << distCase << " : " << linesext  << " new: " << newl <<endl;
    extrusionsum += havedist;
    if (abs(havedist+AOamount)>0.01) cerr << " wrong distrib tract " << havedist << endl;
    if (abs(extrusionsum) > 0.01) cerr << "wrong AO extr.: " << extrusionsum << endl;
    total_extrusionsum += extrusionsum;
#endif
  }

#if AODEBUG
  if (abs(total_extrusionsum) > 0.01) cerr << "wrong total AO extr.: " << total_extrusionsum << endl;

  double totalabs = total_abs_Extrusion(lines);
  if (abs(totalabs)>0.01)
    cerr << "abs-extrusion difference after antiooze " << totalabs << endl;
  double total_rel2 = total_rel_Extrusion(lines) - total_rel;
  if (abs(total_rel2)>0.01)
    cerr << "rel-extrusion difference after antiooze " << total_rel2 << endl;
  double total_ext2 = total_Extrusion(lines) - total_ext;
  if (abs(total_ext2)>0.01)
    cerr << "total extrusion difference after antiooze " << total_ext2 << endl;
#endif

  return total_added;
}

