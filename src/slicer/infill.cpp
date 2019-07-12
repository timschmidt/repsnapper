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

#include "infill.h"
#include "poly.h"
#include "layer.h"

#ifdef _OPENMP
 omp_lock_t Infill::save_lock;
#endif



Infill::Infill()
    : m_type(INVALIDINFILL), extrusionfactor(1), baseRotation(0),
      rotationPerLayer(0)
{
}


Infill::Infill (InfillType type, double infillDistance, double extrfactor,
                double rotation, double rotatePerLayer)
    : pattern(nullptr), m_type(type), infillDistance(infillDistance),
      extrusionfactor(extrfactor), baseRotation(rotation),
      rotationPerLayer(rotatePerLayer)
{
    if (abs(infillDistance) < 0.001) {
        cerr << "Zero Infill Distance type="<< type << endl;
    }
}

Infill::~Infill()
{
    if (pattern)
        delete pattern;
}

// fill polys with type etc.
void Infill::apply(double z, const Poly &poly, int layerNo,
                           vector<Poly> &toPolys)
{
  vector<Poly> polys; polys.push_back(poly);
  apply(z, polys, layerNo, toPolys);
}

void Infill::apply(double z, const vector<Poly> &polys, int layerNo,
                   vector<Poly> &toPolys)
{
  ClipperLib::Paths patterncpolys = getInfillPattern(polys, layerNo);
  vector<Poly> result = apply(z, polys, patterncpolys);
  toPolys.insert(toPolys.end(), result.begin(), result.end());
}

void Infill::apply(double z, const ExPoly &expoly, int layerNo,
                           vector<Poly> &toPolys)
{
  vector<Poly> polys = Clipping::getPolys(expoly);
  apply (z, polys, layerNo, toPolys);
}

void Infill::apply(double z, const vector<ExPoly> &expolys, int layerNo,
                   vector<Poly> &toPolys)
{
  vector<Poly> polys = Clipping::getPolys(expolys);
  apply (z, polys, layerNo, toPolys);
}

// calculates angles for bridges
// void Infill::addBridgeInfill(double z, const vector<Poly> polys,
// 			     double infillDistance, double offsetDistance,
// 			     vector<polys> bridgepillars)
// {
//   type = BridgeInfill;
//   this->infillDistance = infillDistance;

//   omp_set_lock(&save_lock);
//   ClipperLib::Paths patterncpolys =
//     makeInfillPattern(type, polys, infillDistance, offsetDistance, rotation);
//   addPolys(z, polys, patterncpolys, offsetDistance);
//   omp_unset_lock(&save_lock);
// }

// append fillpolys
void Infill::apply(double z, const vector<Poly> &polys,
                   const vector<Poly> &fillpolys,
                   vector<Poly> &toPolys)
{
    vector<Poly> result = apply(z, polys, Clipping::getClipperPolygons(fillpolys));
    toPolys.insert(toPolys.end(), result.begin(), result.end());
}

// clip infill pattern polys against polys
vector<Poly> Infill::apply(double z, const vector<Poly> &polys,
                           const ClipperLib::Paths &patterncpolys)
{
  Clipping clipp;
  clipp.addPolys   (polys,         subject);
  clipp.addPolygons(patterncpolys, clip);
  clipp.setExtrusionFactor(extrusionfactor); // set my extfactor
  clipp.setZ(z);
  vector<Poly> result = clipp.intersect();
  if (m_type==PolyInfill)  // reversal from evenodd clipping
    for (uint i = 0; i<result.size(); i+=2)
      result[i].reverse();
  return getInfillPolys(result);
}


void Infill::makeBaseInfillPattern(const Vector2d &min,
                                   const Vector2d &max)
{
    if (pattern) delete pattern;
    pattern = nullptr;
    switch (m_type) {
    case HexInfill: {
      pattern = new HexPattern(infillDistance, baseRotation);
    }
        break;
    case SmallZigzagInfill: // small zigzag lines -> square pattern
        //case ZigzagInfill: // long zigzag lines
    case ParallelInfill: {
        pattern = new ParallelPattern(m_type, infillDistance,
                                      baseRotation, rotationPerLayer);
    }
        break;
    case HilbertInfill:{
        pattern = new HilbertPattern(infillDistance, baseRotation);
    }
    }
    if (pattern && !pattern->covers(min, max))
        pattern->makeInfillPattern(min, max);
}

// generate infill pattern as a vector of polygons
ClipperLib::Paths Infill::getInfillPattern(const vector<Poly> &tofillpolys,
                                           int layerNo)
{
  ClipperLib::Paths cpolys;

  if (abs(infillDistance) < 0.0001)  {
      qDebug() << "Zero Infill Distance";
      return cpolys;
  }

  if (tofillpolys.size()==0) return cpolys;

  Vector2d min( INFTY, INFTY), max(-INFTY, -INFTY);
  for (const Poly &p : tofillpolys) {
      p.accumulateMinMax(min, max);
  }

  // none found - make new:
  if (!pattern)
      makeBaseInfillPattern(min, max);

#if 0
#ifdef _OPENMP
  if (!omp_in_parallel())
#endif
  if (pattern) // debug pattern
      pattern->drawPatternPolys(0);
#endif

  switch (m_type) {
  case HexInfill: {
      cpolys = ((HexPattern*)pattern)->getPattern(layerNo%2!=0);
  }
      break;
  case SmallZigzagInfill: // small zigzag lines -> square pattern
      //case ZigzagInfill: // long zigzag lines
  case ParallelInfill: {
      cpolys = pattern->getPattern(layerNo);
  }
      break;
  case HilbertInfill:{
      cpolys = pattern->getPattern(layerNo);
  }
      break;
  case ThinInfill: {
      // just use the poly itself at half extrusion rate
      cpolys = Clipping::getClipperPolygons(tofillpolys);
      // adjust extrusion rate - see how thin it is:
      const uint num_div = 10;
      double shrink = 0.5*infillDistance/num_div;
      //cerr << "shrink " << shrink << endl;
      uint count = 0;
      //	uint num_polys = tofillpolys.size();
      vector<Poly> shrinked = tofillpolys;
      while (true) {
          shrinked = Clipping::getOffset(shrinked,-shrink);
          count++;
          //cerr << shrinked.size() << " - " << num_polys << endl;
          if (shrinked.size() == 0) break; // stop when poly is gone
      }
      extrusionfactor = 0.5 + 0.5/num_div * count;
      //cerr << "ex " << extrusionfactor << endl;
      //cpolys = Clipping::getClipperPolygons(opolys);
  }
      break;
  case PolyInfill: // fill all polygons with their shrinked polys
  {
      if (pattern) delete pattern;
      pattern = new PolyPattern(infillDistance);
      cpolys = ((PolyPattern*)pattern)->getPattern(tofillpolys);
  }
      break;
  default:
      cerr << "infill type " << m_type << " unknown "<< endl;
  }
//  if (pattern)
//      pattern->drawPatternPolys(z);
  return cpolys;
}


uint smallest(const vector<double> &nums, double &minimum)
{
  minimum = INFTY;
  uint minind = 0;
  for (uint i = 0; i < nums.size(); i++ ) {
    if (nums[i] < minimum) {
      minimum = nums[i];
      minind = i;
    }
  }
  return minind;
}

bool intersectsPolys(const Vector2d &P1, const Vector2d &P2,
                     const vector<Poly> &polys, double err=0.01)
{
  Intersection inter;
  for (size_t ci = 0; ci < polys.size(); ci++) {
    // bool in1 = polys[ci].vertexInside(P1);
    // bool in2 = polys[ci].vertexInside(P2);
    // if (in1 != in2) return true;
    vector<Intersection> inter = polys[ci].lineIntersections(P1, P2, err);
    if (inter.size() > 0) {
      return true;
    }
  }
  return false;
}

// sort (parallel) lines into polys so that each poly can be an extrusion path
// polys will later be connected by moves (as printlines)
// that is: connect nearest lines, but when connection intersects anything,
//          start a new path (poly)
vector<Poly> Infill::sortedpolysfromlines(const vector<infillline> &lines,
                                          const vector<Poly> &tofillpolys,
                                          double z)
{
  vector<Poly> polys;
  ulong count = lines.size();
  if (count == 0) return polys;
  vector<Poly> clippolys = Clipping::getOffset(tofillpolys,0.1);

  vector<bool> done(count);
  for (uint i = 0; i < count; i++ ) done[i] = false;
  Poly p(z, extrusionfactor);
  p.setClosed(false);
  p.addVertex(lines[0].from);
  p.addVertex(lines[0].to);
  uint donelines = 1;
  while(donelines < count)
    {
      vector<double> dist(4);
      Vector2d l1,l2;
      double mindistline = INFTY;
      uint minindline = 0;
      uint minind = 0;
      double mindist = INFTY;
      // find nearest line for current p endpoints:
      for (uint j = 1; j < count; j++)
    if (!done[j]) {
      dist[0] = (p.front()-lines[j].from).squared_length();
      dist[1] = (p.front()-lines[j].to  ).squared_length();
      dist[2] = (p.back() -lines[j].from).squared_length();
      dist[3] = (p.back() -lines[j].to  ).squared_length();
      uint mind = smallest(dist, mindist);
      if (mindist < mindistline) {
        minindline = j;
        mindistline = mindist;
        minind = uint(mind);
      }
    }
      uint i = minindline;
      // make new line l1--l2:
      if (minind % 2 == 0) { l1 = lines[i].from; l2 = lines[i].to; }
      else                 { l1 = lines[i].to; l2 = lines[i].from; }

      // connection to last/first
      Vector2d conn1, conn2 = l1;
      if (minind < 2) conn1 = p.front();
      else            conn1 = p.back();

      // try polygons intersect
      bool intersects = intersectsPolys(conn1, conn2, clippolys);

      if (!intersects) {
        // detect crossings: try intersect with any line
        Intersection inter;
        for (uint li = 0; li<lines.size(); li++) {
          if (IntersectXY(conn1, conn2, lines[li].from, lines[li].to, inter,
              infillDistance/2.))  {
            double t = conn2.x() - conn1.x();
            if (t != 0.)
              t = (inter.p.x() - conn1.x()) / t;

        // don't catch endpoint intersections (continuations)
        if (t > 0.1 && t < 0.9) {
        intersects = true;
        break;
            }
          }
        }
      }

      if (intersects) { // start new poly
    if (p.size() > 0) {
      p.cleanup(infillDistance/2.);
      polys.push_back(p);
    }
    p = Poly(z, extrusionfactor);
    p.setClosed(false);
      }
      // add new line to current p
      if (minind < 2) { p.push_front(l1); p.push_front(l2); }
      else            { p.push_back(l1);  p.push_back(l2);  }
      done[i] = true;
      donelines++;
    }
  if (p.size() > 0) {
    p.cleanup(infillDistance/2.);
    polys.push_back(p);
  }
  return polys;
}

bool sameAngle(double angle1, double angle2, double err)
{
  while (angle1 < 0) angle1+=2.*M_PI;
  while (angle2 < 0) angle2+=2.*M_PI;
  while (angle1 >= 2*M_PI) angle1-=2.*M_PI;
  while (angle2 >= 2*M_PI) angle2-=2.*M_PI;
  return (abs(angle1-angle2) < err);
}


vector<Poly> Infill::getInfillPolys(const vector<Poly> &polys)
{
    vector<Poly> infillPolys;
  if (polys.size() == 0) return infillPolys;
#define NEWINFILL 1
#if NEWINFILL
  switch (m_type) {
/*  case BridgeInfill:
  case ParallelInfill:
  {
      vector<infillline> lines;
      const Vector2d UNITX(1,0);
      for (uint j = 0; j < polys.size(); j++) {
          for (uint i = 0; i < polys[j].size() ; i++ )
          {
              Vector2d l = (polys[j][i+1] - polys[j][i]);
              double langle = double(planeAngleBetween(UNITX, l)) + M_PI/2;
              if (sameAngle(langle,      m_angle, 0.2) ||
                      sameAngle(langle+M_PI, m_angle, 0.2))
              {
                  infillline il = { polys[j][i], polys[j][i+1] };
                  lines.push_back( il );
              }
          }
      }
      infillpolys = sortedpolysfromlines(lines, polys.back().getZ());
      break;
  }*/
  default:
#endif
#if NEWINFILL
    infillPolys = polys;
    // Poly newpoly(p.getZ(), extrusionfactor);
    // infillpolys.push_back(newpoly);
  }
#else
  for (uint i=0; i<polys.size();i++)
    addInfillPoly(polys[i]);
#endif
  return infillPolys;
}

string Infill::info() const
{
  ostringstream ostr;
  ostr << "Infill "
       << ": type=" << m_type
       << ", extrf=" << extrusionfactor;
  return ostr.str();
}

InfillSet::InfillSet(Settings &settings, const Vector2d &mmin, const Vector2d &mmax) {

    const Vector2d center = 0.5 * (mmin + mmax);
    const Vector2d diag = mmax - center;
    const Vector2d min = center - diag*1.1;
    const Vector2d max = center + diag*1.1;

    // inFill distances in real mm:
    // for full polys/layers:
    double fullInfillDistance=0;
    double infillDistance = 3; // normal fill
    double infillPercent=settings.get_double("Slicing/InfillPercent");
    double altInfillDistance=0;
    double normalInfilldist=3;
    bool shellOnly = !settings.get_boolean("Slicing/DoInfill");
    uint extruder = 0;

    const double thickness = settings.get_double("Slicing/LayerThickness");
    fullInfillDistance = settings.GetInfillDistance(thickness, 100, extruder);

    const double baseRot = (settings.get_double("Slicing/InfillRotation")/180.*M_PI);
    const double rotPerLayer = settings.get_double("Slicing/InfillRotationPrLayer")/180.*M_PI ;

    if (infillPercent < 0.01)
      shellOnly = true;
    else
      infillDistance = settings.GetInfillDistance(thickness, infillPercent, extruder);
    int altinfill = settings.get_integer("Slicing/AltInfillLayers");
    normalInfilldist = infillDistance;

    const InfillType normInfillType = InfillType(settings.get_integer("Slicing/NormalFilltype"));
    if ( altinfill != 0  && infillPercent != 0.) {
      altInfillDistance = settings.GetInfillDistance(thickness,
      settings.get_double("Slicing/AltInfillPercent"), extruder);
      altInfill = new Infill(normInfillType, altInfillDistance,
                             settings.get_double("Slicing/NormalFillExtrusion"),
                             baseRot, rotPerLayer);
    }
    normalInfill = new Infill(normInfillType, normalInfilldist,
                              settings.get_double("Slicing/NormalFillExtrusion"),
                              baseRot, rotPerLayer);
    normalInfill->makeBaseInfillPattern(min, max);

    uint firstLayers = uint(settings.get_integer("Slicing/FirstLayersNum"));
    if (firstLayers > 0) {
        double first_infdist =std::max(normalInfilldist,
                                       fullInfillDistance * (1.+settings.get_double("Slicing/FirstLayersInfillDist")));
        firstInfill = new Infill(normInfillType, first_infdist,
                                 settings.get_double("Slicing/NormalFillExtrusion"),
                                 baseRot, rotPerLayer);
        firstFullInfill = new Infill(normInfillType,
                                     std::max(fullInfillDistance, first_infdist),
                                     settings.get_double("Slicing/NormalFillExtrusion"),
                                     baseRot, rotPerLayer);
        firstInfill->makeBaseInfillPattern(min, max);
        firstFullInfill->makeBaseInfillPattern(min, max);
    } else {
        firstInfill = firstFullInfill = nullptr;
    }

    InfillType fullType = InfillType(settings.get_integer("Slicing/FullFilltype"));
    const double fullextr = settings.get_double("Slicing/FullFillExtrusion");

    if (settings.get_boolean("Slicing/FillSkirt")) {
        const double skirtDist = settings.get_double("Slicing/SkirtDistance");
        const Vector2d sd(skirtDist, skirtDist);
        skirtInfill = new Infill(fullType, fullInfillDistance, fullextr, 0, 0);
        skirtInfill->makeBaseInfillPattern(min-sd, max+sd);
    } else
        skirtInfill = nullptr;

    fullInfill = new Infill(fullType, fullInfillDistance, fullextr,
                            baseRot, rotPerLayer);
    fullInfill->makeBaseInfillPattern(min, max);

    uint skins = uint(std::max(1, settings.get_integer("Slicing/Skins")));
    skinInfills.resize(skins);
    for (uint ns = 0; ns < skins; ns++) {
        const double skinfillextrf = fullextr/skins/skins;
        double skinDist = fullInfillDistance/skins;
        skinInfills[ns] = new Infill(InfillType(settings.get_integer("Slicing/FullFilltype")),
                                     skinfillextrf, skinDist, baseRot, rotPerLayer);
        skinInfills[ns]->makeBaseInfillPattern(min, max);
    }

    if (settings.get_boolean("Slicing/MakeDecor")) {
        decorInfill = new Infill(InfillType(settings.get_integer("Slicing/DecorFilltype")),
                                 settings.get_double("Slicing/DecorInfillDistance"),
                                 1.,
                                 settings.get_double("Slicing/DecorInfillRotation")/180.0*M_PI,
                                 rotPerLayer);
        decorInfill->makeBaseInfillPattern(min, max);
    } else
        decorInfill = nullptr;

    if (settings.get_boolean("Slicing/Support")){
        supportInfill = new Infill(InfillType(settings.get_integer("Slicing/SupportFilltype")),
                                   settings.get_double("Slicing/SupportInfillDistance"),
                                   settings.get_double("Slicing/SupportExtrusion"),
                                   0, 0);
        supportInfill->makeBaseInfillPattern(min, max);
    } else
        supportInfill = nullptr;
    thinInfill = new Infill(ThinInfill, fullInfillDistance, 1., 0, 0);
    thinInfill->makeBaseInfillPattern(min, max);
}

InfillSet::~InfillSet()
{
    delete normalInfill;
    delete firstInfill;
    delete firstFullInfill;
    delete altInfill;
    delete fullInfill;
    skinInfills.clear();
    delete skirtInfill;
    delete decorInfill;
    delete supportInfill;
    delete thinInfill;

}
