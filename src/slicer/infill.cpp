/*
    This file is a part of the RepSnapper project.
    Copyright (C) 2011  martin.dieringer@gmx.de

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

#include <vmmlib/vmmlib.h> 

using namespace std; 
using namespace vmml;

Infill::Infill(){
  infill.clear();
}
void Infill::calcInfill(Poly *poly){cerr << "virtual Infill::calcInfill called!" << endl;}

void Infill::printinfo() const
{ 
  cout << "Infill with " << infill.size() << " vertices" <<endl;
}


// use default values
void ZigZagInfill::calcInfill(Poly *poly)
{
  Vector2d start = poly->getVertexCircular(0);
  double startangle=M_PI/4.;
  double fillratio=0.3;
  calcInfill(poly,start,startangle,fillratio);
} 

void ZigZagInfill::calcInfill(Poly *poly, const Vector2d startpoint, 
			      double startAngle, const double fillRatio)
{
  double rot = startAngle;
  Vector2d InfillDirX(cos(rot), sin(rot));
  Vector2d P1 = startpoint;
  double length=1000.;
  Vector2d P2;
  double dangle = 10.*M_PI/180.; // depend on fillRatio!
  double maxerr = 0.0001; // !!!

  infill.clear();
  bool full=false;
  infill.push_back(P1);
  while (!full) 
    {
      InfillDirX = Vector2d(cos(rot)*length, sin(rot)*length);
      P2 = P1 + InfillDirX;
      vector<InFillHit> phits = poly->lineIntersections(P1,P2,maxerr);
      if (phits.size() > 0) {
	P1 = phits[0].p;
	infill.push_back(P1);
      }
      rot += dangle;
    }
}



void ParallelInfill::calcInfill(Poly *poly)
{
  double startangle=M_PI/4.;
  double fillratio=0.3;
  //cout <<"pinfill calc " ;
  //poly->printinfo();
  calcInfill(poly,startangle,fillratio);
} 


void ParallelInfill::calcInfill(Poly *poly,  
				double startAngle, const double infillDistance)
{
  vector<InFillHit> HitsBuffer;
  double rot = startAngle;
  Vector2d InfillDirX(cos(rot), sin(rot));
  Vector2d InfillDirY(-InfillDirX.y, InfillDirX.x);
  double step = infillDistance;

  vector<Vector2d> minmax = poly->getMinMax();
  Vector2d Min = minmax[0], Max = minmax[1];
  
  double Length = 2.*(((Max.x)>(Max.y)? (Max.x):(Max.y))  -  ((Min.x)<(Min.y)? (Min.x):(Min.y))  );	// bbox of lines to intersect the poly with

  // bool examine = false;
  // bool examineThis = true;

  cerr << "calc parallel infill rot=" << rot  << " at layer z=" << poly->getZ() << " no " << poly->getLayerNo() << endl;
  cerr << "MINMAx" << Min << "--" << Max << endl;

  HitsBuffer.clear();
  for(double x = -Length ; x < Length ; x+=step)
    {
      //cerr <<"x="<<x<<" of " << Length<<endl;
      Vector2d P1 = (InfillDirX * Length) + (InfillDirY*x) + poly->center;
      Vector2d P2 = (InfillDirX * -Length)+ (InfillDirY*x) + poly->center;

      // double Examine = 0;

      vector<InFillHit> hits = poly->lineIntersections(P1,P2,0.1*step);

      HitsBuffer.insert(HitsBuffer.end(),hits.begin(),hits.end());

      for(size_t i=0;i<poly->points.size();i++)
  	{
  	  Vector2d P3 = poly->getVertexCircular(i);
  	  Vector2d P4 = poly->getVertexCircular(i+1);
  	  InFillHit hit;
  	  if (IntersectXY (P1,P2,P3,P4, hit, 0.1*step)) {
  	    HitsBuffer.push_back(hit);
  	  }
  	}
  	// Sort hits
  	// Sort the vector using predicate and std::sort
      std::sort (HitsBuffer.begin(), HitsBuffer.end(), InFillHitCompareFunc);
      // Verify hits intregrety
      // Check if hit extists in table
    restart_check:
      for (size_t i=0;i<HitsBuffer.size();i++)
  	{
  	  bool found = false;
  	  for (size_t j=i+1;j<HitsBuffer.size();j++)
  	    {
  	      if( ABS(HitsBuffer[i].d - HitsBuffer[j].d) < 0.0001)
  		{
  		  found = true;
  		  // Delete both points, and continue
  		  HitsBuffer.erase(HitsBuffer.begin()+j);
  		  // If we are "Going IN" to solid material, and there's
  		  // more points, keep one of the points
  		  if (i != 0 && i != HitsBuffer.size()-1)
  		    HitsBuffer.erase(HitsBuffer.begin()+i);
  		  goto restart_check;
  		}
  	    }
  	  if (found)
  	    continue;
  	}
      // Sort hits by distance and transfer to InFill Buffer
      if (HitsBuffer.size() != 0 && HitsBuffer.size() % 2)
  	continue;  // There's a uneven number of hits, skip this infill line (U'll live)
    }
  //return HitsBuffer;

  infill.clear();
  cerr << "infill found " << HitsBuffer.size() << " hits"<< endl;
  for (size_t i=0;i<HitsBuffer.size();i++){
    infill.push_back(HitsBuffer[i].p);
  }


}
