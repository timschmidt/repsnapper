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

#ifndef INFILLPATTERN_H
#define INFILLPATTERN_H

#include "../stdafx.h"
#include "clipping.h"

// these are available for user selection (order must be same as types):
const string InfillNames[] = {_("Parallel"), _("Zigzag"), _("Hexagons"), _("Polygons"), _("Hilbert Curve")};

// user selectable have to be first
enum InfillType {ParallelInfill, SmallZigzagInfill, HexInfill, PolyInfill,
                 HilbertInfill, ZigzagInfill, ThinInfill, INVALIDINFILL};

class InfillPattern
{
    friend class Infill;

protected:
    InfillType type;
    Vector2d Min, Max;
    Vector2d modelMin, modelMax; // has been set for these values
    double infillDistance;
    vector<Poly> m_polys;
    Vector2d m_center;
//    Vector2d baseVec = Vector2d::ZERO; // unit cell size, for rotation
//    Vector2d centerOff = Vector2d::ZERO;
    double baseRotation = 0;
    double m_rotatePerLayer;
    virtual void makeInfillPattern(const Vector2d &Min, const Vector2d &Max) = 0;
    virtual void setMinMax(const Vector2d &min, const Vector2d &max);

public:
    InfillPattern();
    InfillPattern(const InfillType type, double infillDistance,
                  double baseRot, double m_rotatePerLayer);
    virtual ~InfillPattern() {
        type = INVALIDINFILL;
        m_polys.clear();
    }
    void drawPatternPolys(double z) const;
    virtual ClipperLib::Paths getPattern(int layerNo);
    bool covers(const Vector2d &min, const Vector2d &max);
};


class ParallelPattern : public InfillPattern {
    void makeInfillPattern(const Vector2d &Min, const Vector2d &Max);
public:
    ParallelPattern(const InfillType ftype, double infilldistance,
                    double baseRot, double rotPerLayer)
        : InfillPattern(ftype, infilldistance, baseRot, rotPerLayer){}
};

class PolyPattern : public InfillPattern {
    void makeInfillPattern(const Vector2d &Min, const Vector2d &Max);
public:
    const bool rotateablePerLayer = false;
    PolyPattern(double infilldistance)
        : InfillPattern(PolyInfill, infilldistance, 0, 0){}
    ClipperLib::Paths getPattern(const vector<Poly> &m_polys);
private:
    using InfillPattern::getPattern; // -Woverloaded-virtual
};

class HexPattern : public InfillPattern {
    vector<Poly> polys2; // alternating pattern
    void makeInfillPattern(const Vector2d &Min, const Vector2d &Max);
    const bool rotateablePerLayer = false;
public:
    HexPattern(double infilldistance, double baseRot);
    ClipperLib::Paths getPattern(bool alternatePattern);
private:
    double hexa, hexd;
    using InfillPattern::getPattern;
};

class HilbertPattern : public InfillPattern
{
public:
    HilbertPattern(double infilldistance, double baseRot)
        : InfillPattern(HilbertInfill, infilldistance, baseRot, 0){}
    ClipperLib::Paths getPattern(const vector<Poly> &tofillpolys,
                                 double angle, int layerno);
protected:
    void makeInfillPattern(const Vector2d &Min, const Vector2d &Max);
private:
    enum {UP, LEFT, DOWN, RIGHT};
    void move(int direction, vector<Vector2d> &v);
    void hilbert(int level, int direction, vector<Vector2d> &v);
    using InfillPattern::getPattern;
};

#endif // INFILLPATTERN_H
