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

#include "infillpattern.h"


vector<Poly> rotatedPolys(const vector<Poly> &polys,
                          const Vector2d &center, double angle) {
    vector<Poly> rpolys(polys);
    for (Poly &p: rpolys)
        p.rotate(center,angle);
    return rpolys;
}

void rotatePolys(vector<Poly> &polys,
                 const Vector2d &center, double angle) {
    for (Poly &p: polys)
        p.rotate(center,angle);
}

void toGrid(Vector2d &v, Vector2d baseV){
    if (baseV.x()!=0.)
        v.x() = floor(v.x()/baseV.x())*baseV.x();
    if (baseV.y()!=0.)
        v.y() = floor(v.y()/baseV.y())*baseV.y();
}


InfillPattern::InfillPattern(const InfillType type, double infilldistance,
                             double baseRot, double rotPerLayer)
    : type(type),
      infillDistance(infilldistance),
      baseRotation(baseRot), m_rotatePerLayer(rotPerLayer)
{
    if (type == SmallZigzagInfill || type == ZigzagInfill) {
        m_rotatePerLayer = M_PI/2.;
//        baseVec = Vector2d(2*infillDistance, 2*infillDistance);
    }
}

void InfillPattern::drawPatternPolys(double z) const
{
    for (const Poly &p: m_polys)
        p.draw(GL_LINES, z, true);
    glPointSize(13);
    glBegin(GL_POINTS);
    glVertex3dv(Vector3d(m_center,0));
    glEnd();
}

ClipperLib::Paths InfillPattern::getPattern(int layerNo) {
//    if (!covers(min,max)) {
//        makeInfillPattern(min, max);
//    }
    vector<Poly> rpolys = rotatedPolys(m_polys, m_center,
                                       layerNo * m_rotatePerLayer);
    return Clipping::getClipperPolygons(rpolys);
}

bool InfillPattern::covers(const Vector2d &min, const Vector2d &max){
    return (modelMin.x() <= min.x() && modelMax.x() >= max.x() &&
            modelMin.y() <= min.y() && modelMax.y() >= max.y());
}


void InfillPattern::setMinMax(const Vector2d &min, const Vector2d &max)
{
    modelMin = min;
    modelMax = max;
    m_center = 0.5 * (min + max);
    if (m_rotatePerLayer == 0.) {
        // rotate bbox with
        Poly rect;
        rect.addVertex(min);
        rect.addVertex(max.x(),min.y());
        rect.addVertex(max);
        rect.addVertex(min.x(),max.y());
        rect.rotate(m_center, -baseRotation);
        Min = Max = m_center;
        rect.accumulateMinMax(Min,Max);

    } else {
        //    toGrid(m_center, baseVec);
        // pattern big enough for rotation (1.41... * longer side)
        const double w = std::max (max.x()-min.x(), max.y()-min.y()) * 1.5 / 2.;
        Vector2d diag = Vector2d(w,w);
        Min = m_center - diag;
        //    toGrid(Min, baseVec);
        Max = m_center + diag;// + baseVec;
        //    toGrid(Max, baseVec);
        //    m_center += centerOff;
    }

}


void ParallelPattern::makeInfillPattern(const Vector2d &min, const Vector2d &max)
{
    setMinMax(min, max);
    assert(infillDistance>0.001);
//    cerr << "make " << min << max << endl;
    bool zigzag = type == SmallZigzagInfill;
//    if (zigzag) Min=Vector2d::ZERO; // fixed position
    Poly poly(0);
    uint count = 0;
    for (double x = Min.x(); x < Max.x(); ) {
        double x2 = x+infillDistance;
        poly.addVertex(x, Min.y());
        if (zigzag) { // zigzag -> squares
            double ymax=Max.y();
            for (double y = Min.y(); y < Max.y(); y += 2*infillDistance) {
                poly.addVertex(x, y);
                poly.addVertex(x2, y+infillDistance);
                ymax = y;
            }
            for (double y = ymax; y > Min.y(); y -= 2*infillDistance) {
                poly.addVertex(x2, y+infillDistance);
                poly.addVertex(x2+infillDistance, y);
            }
            x += 2*infillDistance;
        } else {      // normal line infill
            if (count%2){
                poly.addVertex(x,  Min.y());
                poly.addVertex(x2, Min.y());
            } else {
                poly.addVertex(x,  Max.y());
                poly.addVertex(x2, Max.y());
            }
            x += infillDistance;
        }
        count ++;
    }
    poly.addVertex(Max.x(), Min.y()-infillDistance);
    poly.addVertex(Min.x(), Min.y()-infillDistance);
    poly.rotate(m_center, baseRotation);

//    Poly poly2(0);
//    poly2.addVertex(Min);
//    poly2.addVertex(Max.x(), Min.y());
//    poly2.addVertex(Max);
//    poly2.addVertex(Min.x(),Max.y());

    m_polys.clear();
    m_polys.push_back(poly);
    //polys.push_back(poly2);
}


void HexPattern::makeInfillPattern(const Vector2d &min, const Vector2d &max)
{
//    cerr << "new pattern " << min << max << endl;
    setMinMax(min, max);

    // two alternating layers, 1:
    Poly poly(0);
    const double dx = 0.1;// offset to keep lines apart
    for (double x = Min.x(); x < Max.x(); x += hexa) {
        double y = Min.y();
        while (y < Max.y() ) {
            poly.addVertex(x, y);
            poly.addVertex(x + hexa - dx, y+hexd/2);
            y+=1.5*hexd;
            poly.addVertex(x + hexa - dx, y);
            poly.addVertex(x, y+hexd/2);
            y+=1.5*hexd;
        }
        poly.addVertex(x, y);
        x += hexa;
        while (y > Min.y()) {
            y+=0.5*hexd;
            poly.addVertex(x, y);
            poly.addVertex(x + hexa - dx, y-hexd/2);
            y-=1.5*hexd;
            poly.addVertex(x + hexa - dx, y);
            poly.addVertex(x, y-hexd/2);
            y-=2*hexd;
        }
    }
    poly.addVertex(Max.x(), Min.y()-infillDistance);
    poly.addVertex(Min.x(), Min.y()-infillDistance);
    poly.rotate(m_center, baseRotation);
    m_polys.clear();
    m_polys.push_back(poly);

    poly.clear();
    // other layer, simpler
    for (double y = Min.y(); y < Max.y(); y += 3.*hexd) {
        double x = Min.x();
        while (x < Max.x()) {
            poly.addVertex(x, y);
            poly.addVertex(x+hexa, y+hexd/2.);
            x += 2.*hexa;
        }
        double y2 = y+1.5*hexd;
        while (x > Min.y()) {
            poly.addVertex(x+hexa,  y2);
            poly.addVertex(x, y2+hexd/2.);
            x -= 2.*hexa;
        }
    }
    poly.addVertex(Min.x(), Max.y()-infillDistance);
    poly.addVertex(Min.x(), Min.y()-infillDistance);
    poly.rotate(m_center, baseRotation);
    polys2.clear();
    polys2.push_back(poly);
}

HexPattern::HexPattern(double infilldistance, double baseRot)
    : InfillPattern(HexInfill, infilldistance, baseRot, 0){
    hexd = infillDistance;     // /(1+sqrt(3.)/4.);
    hexa = hexd * sqrt(3.)/2.; // ~= 0.866 hexd
//    baseVec = Vector2d(2.*hexa, 3.*hexd);
//    centerOff = Vector2d(0, hexd);
}

ClipperLib::Paths HexPattern::getPattern(bool alternatePattern) {
    return Clipping::getClipperPolygons(alternatePattern ? polys2 : m_polys);
}


void PolyPattern::makeInfillPattern(const Vector2d &min, const Vector2d &max)
{ // cant save this pattern
    setMinMax(min,max);
}

ClipperLib::Paths PolyPattern::getPattern(const vector<Poly> &tofillpolys)
{
    vector< vector<Poly> > ipolys; // all offset shells
    for (uint i=0; i < tofillpolys.size(); i++){
        double parea = Clipping::Area(tofillpolys[i]);
        // make first larger to get clip overlap
        double firstshrink = 0.5*infillDistance;
        if (parea<0) firstshrink = -firstshrink;
        vector<Poly> shrinked  = Clipping::getOffset(tofillpolys[i], firstshrink);
        vector<Poly> shrinked2 = Clipping::getOffset(shrinked, 0.5*infillDistance);
        for (uint i=0;i<shrinked2.size();i++)
            shrinked2[i].cleanup(0.1*infillDistance);
        ipolys.push_back(shrinked2);
        double area = Clipping::Area(shrinked);
        //cerr << "shr " << parea << " - " <<area<< " - " << " : " <<endl;
        // int lastnumpolys=0;
        // int shrcount=0;
        while (shrinked.size()>0){
            if (area*parea < 0)  break; // went beyond zero size
            // cerr << "shr " <<parea << " - " <<area<< " - " << shrcount << " : " <<endl;
            shrinked2 = Clipping::getOffset(shrinked, 0.5*infillDistance);
            for (uint i=0;i<shrinked2.size();i++)
                shrinked2[i].cleanup(0.1*infillDistance);
            ipolys.push_back(shrinked2);
            //lastnumpolys = shrinked.size();
            shrinked = Clipping::getOffset(shrinked,-infillDistance);
            for (uint i=0;i<shrinked.size();i++)
                shrinked[i].cleanup(0.1*infillDistance);
            // cerr << "shr2 " <<parea << " - " <<area<< " - " << shrcount << " : " <<
            //   shrinked.size()<<endl;
            // shrcount++;
            area = Clipping::Area(shrinked);
        }
    }
    vector<Poly> opolys;
    for (uint i=0;i<ipolys.size();i++){
        opolys.insert(opolys.end(),ipolys[i].begin(),ipolys[i].end());
    }
    return Clipping::getClipperPolygons(opolys);
}

// Hilbert curve
// http://www.compuphase.com/hilbert.htm
void HilbertPattern::makeInfillPattern(const Vector2d &min, const Vector2d &max)
{
    setMinMax(min, max);
    int level = int(ceil(log2(1.42*(Max.x()-Min.x())/infillDistance)));
    if (level < 0) return;
    Poly poly(0);
    poly.addVertex(Min);
    hilbert(level, 0, poly.vertices);
    poly.rotate(m_center, baseRotation);
    m_polys.push_back(poly);
}

void HilbertPattern::move(int direction, vector<Vector2d> &v){
  Vector2d d = v.back(); // have one point at least
  switch (direction) {
  case LEFT:  d.x() -= infillDistance;break;
  case RIGHT: d.x() += infillDistance;break;
  case UP:    d.y() -= infillDistance;break;
  case DOWN:  d.y() += infillDistance;break;
  }
  v.push_back (d);
}

void HilbertPattern::hilbert(int level, int direction,
                            vector<Vector2d> &v)
{
  //cerr <<"hilbert level " << level<< endl;
  if (level==1) { // break if over Max
    switch (direction) {
    case LEFT:
      move(RIGHT,v);      /* move() could draw a line in... */
      move(DOWN, v);       /* ...the indicated direction */
      move(LEFT, v);
      break;
    case RIGHT:
      move(LEFT, v);
      move(UP,   v);
      move(RIGHT,v);
      break;
    case UP:
      move(DOWN, v);
      move(RIGHT,v);
      move(UP,   v);
      break;
    case DOWN:
      move(UP,   v);
      move(LEFT, v);
      move(DOWN, v);
      break;
    } /* switch */
  } else {
    switch (direction) {
    case LEFT:
      hilbert(level-1,UP,v);
      move(RIGHT,v);
      hilbert(level-1,LEFT,v);
      move(DOWN,v);
      hilbert(level-1,LEFT,v);
      move(LEFT,v);
      hilbert(level-1,DOWN,v);
      break;
    case RIGHT:
      hilbert(level-1,DOWN,v);
      move(LEFT,v);
      hilbert(level-1,RIGHT,v);
      move(UP,v);
      hilbert(level-1,RIGHT,v);
      move(RIGHT,v);
      hilbert(level-1,UP,v);
      break;
    case UP:
      hilbert(level-1,LEFT,v);
      move(DOWN,v);
      hilbert(level-1,UP,v);
      move(RIGHT,v);
      hilbert(level-1,UP,v);
      move(UP,v);
      hilbert(level-1,RIGHT,v);
      break;
    case DOWN:
      hilbert(level-1,RIGHT,v);
      move(UP,v);
      hilbert(level-1,DOWN,v);
      move(LEFT,v);
      hilbert(level-1,DOWN,v);
      move(DOWN,v);
      hilbert(level-1,LEFT,v);
      break;
    } /* switch */
  } /* if */
}
