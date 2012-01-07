/*
    This file is a part of the RepSnapper project.
    Copyright (C) 2010  Kulitorum
    Copyright (C) 2012  martin.dieringer@gmx.de

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

#include "layer.h"
#include "poly.h"
#include "shape.h"
#include "infill.h"

Layer::Layer(int layerno, double thick) : LayerNo(layerno), thickness(thick) 
{
  normalInfill = new Infill(this);
  fullInfill = new Infill(this);
  supportInfill = new Infill(this);
  Min = Vector2d(G_MAXDOUBLE, G_MAXDOUBLE);
  Max = Vector2d(G_MINDOUBLE, G_MINDOUBLE);
}


Layer::~Layer()
{
  Clear();
  delete normalInfill;
  delete fullInfill;
  delete supportInfill;
}


void clearpolys(vector<Poly> &polys){
  for (uint i=0; i<polys.size();i++)
    polys[i].clear();
  polys.clear();
}
void clearpolys(vector< vector<Poly> > &polys){
  for (uint i=0; i<polys.size();i++)
    clearpolys(polys[i]);
  polys.clear();
}

void Layer::Clear()
{
  normalInfill->clear();
  fullInfill->clear();
  supportInfill->clear();
  clearpolys(polygons);
  clearpolys(shellPolygons);
  clearpolys(offsetPolygons);
  clearpolys(fullFillPolygons);
  clearpolys(supportPolygons);
  clearpolys(skinPolygons);
  // polygons.clear();
  // shellPolygons.clear();
  // offsetPolygons.clear();
  // fullFillPolygons.clear();
  // supportPolygons.clear();
  hullPolygon.clear();
  skirtPolygon.clear();
  // skinPolygons.clear();
}

void Layer::setBBox(Vector2d min, Vector2d max) 
{
  Min.x = MIN(Min.x,min.x);
  Min.y = MIN(Min.y,min.y);
  Max.x = MAX(Max.x,max.x);
  Max.y = MAX(Max.y,max.y);
}	
void Layer::setBBox(vector<Vector2d> minmax) 
{
  setBBox(minmax[0],minmax[1]);
}
void Layer::setBBox(Vector3d min, Vector3d max) 
{
  Min.x = MIN(Min.x,min.x);
  Min.y = MIN(Min.y,min.y);
  Max.x = MAX(Max.x,max.x);
  Max.y = MAX(Max.y,max.y);
}	


void Layer::SetPolygons(vector<Poly> polys) {
  this->polygons = polys;
}
void Layer::SetPolygons(const Matrix4d &T, const Shape shape, 
			double z, double Optimization) {
  double offsetZ = Z;
  bool have_polys=false;
  while (!have_polys) {
    clearpolys(polygons);
    have_polys = shape.getPolygonsAtZ(T, Optimization, offsetZ, polygons);
    offsetZ+=thickness/10.;
  }
  for(uint i=0;i<polygons.size();i++)
    polygons[i].setZ(Z);
}

void Layer::addPolygons(vector<Poly> polys)
{
  for (uint i =0;i<polys.size();i++)
    {
      polygons.push_back(polys[i]);
      setBBox(polys[i].getMinMax());
    }
}

void Layer::CalcInfill (double InfillDistance, 
			double FullInfillDistance,
			double InfillRotation, 
			double InfillRotationPrLayer,
			bool ShellOnly, // only infill for fullfill (vertical shells)
			bool DisplayDebuginFill)
{
  normalInfill = new Infill(this);
  fullInfill = new Infill(this);
  supportInfill = new Infill(this);
  double rot = (InfillRotation + (double)LayerNo*InfillRotationPrLayer)/180.0*M_PI;
  InfillType type = ParallelInfill;
  if (!ShellOnly)
    normalInfill->addInfill(Z, offsetPolygons, type, 
			    InfillDistance, FullInfillDistance, rot);
  fullInfill->addInfill(Z, fullFillPolygons, type, 
			FullInfillDistance, FullInfillDistance, rot);
  supportInfill->addInfill(Z, supportPolygons, SupportInfill, 
			   InfillDistance, InfillDistance, InfillRotation/180.0*M_PI);
}


// add full fill and substract them from normal fill polys 
void Layer::addFullPolygons(const vector<Poly> fullpolys) 
{
  Clipping clipp;
  clipp.clear();
  // full fill only where already normal fill
  clipp.addPolys(GetOffsetPolygons(),subject); 
  clipp.addPolys(fullpolys,clip);
  vector<Poly> inter = clipp.intersect();
  addFullFillPolygons(inter);
  //substract from normals
  clipp.clear();
  clipp.addPolys(GetOffsetPolygons(),subject);
  clipp.addPolys(fullpolys,clip);
  setNormalFillPolygons(clipp.substract());
}


void Layer::mergeFullPolygons() 
{
  setFullFillPolygons(Clipping::getMerged(GetFullFillPolygons()));
}
void Layer::mergeSupportPolygons() 
{
  setSupportPolygons(Clipping::getMerged(GetSupportPolygons()));
}

// circular numbering
vector<Poly> Layer::GetShellPolygonsCirc(int number) const
{
  number = (shellPolygons.size() +  number) % shellPolygons.size();
  return shellPolygons[number];
}

void Layer::setNormalFillPolygons(const vector<Poly> polys)
{
  clearpolys(offsetPolygons);
  offsetPolygons = polys;
  for (uint i=0; i<offsetPolygons.size();i++)
    offsetPolygons[i].setZ(Z);
}

void Layer::setFullFillPolygons(const vector<Poly>  polys)
{
  clearpolys(fullFillPolygons);
  fullFillPolygons = polys;
  for (uint i=0; i<fullFillPolygons.size();i++)
    fullFillPolygons[i].setZ(Z);
}

void Layer::addFullFillPolygons(const vector<Poly> polys)
{
  fullFillPolygons.insert(fullFillPolygons.end(),polys.begin(),polys.end());
}

void Layer::setSupportPolygons(const vector<Poly> polys)
{
  clearpolys(supportPolygons);
  supportPolygons = polys;
}

void Layer::setSkirtPolygon(const Poly poly)
{
  skirtPolygon = poly;
  skirtPolygon.setZ(Z);
}


void Layer::MakeShells(uint shellcount, double extrudedWidth, 
		       double optimization, 
		       bool makeskirt, uint skins,
		       bool useFillets)
{
  //cerr << "make " << shellcount << " shells "  << endl;
  this->skins = skins;
  double distance = 0.5 * extrudedWidth;
  if (skins>1) {
    skinPolygons = Clipping::getOffset(polygons,-distance,jmiter,0.2);
    shellcount--;
    distance += extrudedWidth; 
  }
  vector<Poly> shrinked = Clipping::getOffset(polygons,-distance,jmiter,0.5);
  clearpolys(shellPolygons);
  shellPolygons.push_back(shrinked); // outer
  distance = extrudedWidth;
  for (uint i = 1; i<shellcount; i++) // shrink from shell to shell
    {
      shrinked = Clipping::getOffset(shrinked,-distance); 
      shellPolygons.push_back(shrinked);
    }
  offsetPolygons = Clipping::getOffset(shrinked,-distance);

  if (makeskirt) {
    MakeSkirt(2*distance);
  }

  //cerr << " .. made " << offsetPolygons.size() << " offsetpolys "  << endl;
  // for (uint i =0; i<shellPolygons.size(); i++) {
  //   cout << "shell " << i << endl;
  //   for (uint j =1; j<shellPolygons[i].size(); j++) {
  //     shellPolygons[i][j].printinfo();
  //   }
  // }
}

void Layer::MakeSkirt(double distance)
{
  calcConvexHull();
  skirtPolygon.clear();
  skirtPolygon = Clipping::getOffset(hullPolygon,distance,jround);//(ShrinkedPolys(convex, -distance,ClipperLib::jtRound)).front();
}


// is B left of A wrt center?
bool isleftof(Vector2d center, Vector2d A, Vector2d B)
{
  double position = (B.x-A.x)*(center.y-A.y) - (B.y-A.y)*(center.x-A.x);
  // position = sign((p2-p1).cross(center-p1)[2]) ; // this would be 3d
  return (position > 0);
}

// gift wrapping algo
void Layer::calcConvexHull() 
{
  hullPolygon.clear();
  hullPolygon=Poly(Z);
  vector<Vector2d> p;
  for (uint i = 0; i<polygons.size(); i++)
    p.insert(p.end(), polygons[i].vertices.begin(), polygons[i].vertices.end());
  uint np = p.size();
  //cerr << np << " points"<< endl;
  if (np==0) return;
  uint current=np;
  double minx=G_MAXDOUBLE;
  double miny=G_MAXDOUBLE;
  for (uint i = 0; i < np; i++) { // get leftmost bottom
    if (p[i].x < minx){
      minx = p[i].x;
      miny = p[i].y;
      current = i;
    }
    else if (p[i].x==minx && p[i].y<miny){
      minx = p[i].x;
      miny = p[i].y;
      current = i;
    }
  }
  assert (current != np);

  hullPolygon.addVertex(p[current]);
  uint start = current;
  uint leftmost ;
  do 
    {
      leftmost=0;
      for (uint i=1; i < np; i++)
	if (leftmost == current || isleftof(p[current], p[i], p[leftmost])) // i more to the left?
	  leftmost = i;
      current = leftmost;
      hullPolygon.addVertex(p[current], true); // to front: reverse order
    }
  while (current != start);
}

void Layer::getOrderedPolyLines(const vector<Poly> polys, 
				Vector2d &startPoint,
				vector<Vector3d> &lines) const
{
  uint count  = polys.size();
  uint nvindex=-1;
  uint npindex=-1;
  uint nindex;
  vector<bool> done; // polys not yet handled
  done.resize(count);
  for(size_t q=0;q<count;q++) done[q]=false;
  uint ndone=0;
  double pdist, nstdist;
  //double nlength;
  while (ndone < count) 
    {
      nstdist = 1000000;
      for(size_t q=0;q<count;q++) // find nearest polygon
	{
	  if (!done[q])
	    {
	      pdist = 1000000;
	      nindex = polys[q].nearestDistanceSqTo(startPoint,pdist);
	      // nlength = polys[q].getLineLengthSq(nindex); // ...feature to come...
	      if (pdist<nstdist){
		npindex = q;      // index of nearest poly in polysleft
		nstdist = pdist;  // distance of nearest poly
		nvindex = nindex; // nearest point in nearest poly
	      }
	    }
	}
      polys[npindex].getLines(lines,nvindex);
      done[npindex]=true;
      ndone++;
      startPoint = Vector2d(lines.back().x,lines.back().y);
    }
}

// Convert to GCode
void Layer::MakeGcode(GCodeState &state,
		      double &E, double offsetZ,
		      const Settings::SlicingSettings &slicing,
		      const Settings::HardwareSettings &hardware)
{
	// Why move z axis separately?
        // Command command;
	// double lastLayerZ = state.GetLastLayerZ(z);
	// // Set speed for next move
	// command.Code = SETSPEED;
	// command.where = Vector3d(0,0,lastLayerZ);
	// command.e = E;					// move
	// command.f = hardware.MinPrintSpeedZ;		// Use Min Z speed
	// state.AppendCommand(command);
	// command.comment = "";
	// // Move Z axis
	// command.Code = ZMOVE;
	// command.where = Vector3d(0,0,z);
	// command.e = E;					// move
	// command.f = hardware.MinPrintSpeedZ;		// Use Min Z speed
	// state.AppendCommand(command);
	// command.comment = "";

        Vector3d start3 = state.LastPosition();
	Vector2d startPoint(start3.x,start3.y);

	double extrf = hardware.GetExtrudeFactor(thickness);
	double skinextrf = extrf/skins; // /skins; // only height change, keep width

	vector<Vector3d> lines;

	// Skirt
	lines.clear();
	skirtPolygon.getLines(lines, startPoint);

	// Support
	getOrderedPolyLines(supportInfill->infillpolys, startPoint,lines);
	state.AddLines(lines, extrf, offsetZ, E, slicing, hardware);	       

	// Skin (different extrusion factor)
	lines.clear();
	vector<Vector3d> skinlines;
	for(size_t p=0;p<skinPolygons.size();p++) 
	  skinPolygons[p].getLines(skinlines,startPoint);
	for(int s=skins-1;s >= 0;s--) { // z offset
	  double sz = Z - s*thickness/skins;
	  for(size_t p=0;p<skinlines.size();p++) 
	    lines.push_back(Vector3d(skinlines[p].x, skinlines[p].y, sz));
	}
	state.AddLines(lines, skinextrf, offsetZ, E, slicing, hardware);
	
	// Shells
	lines.clear();
	for(size_t p=0;p<shellPolygons.size();p++) // outer to inner, in this order
	  getOrderedPolyLines(shellPolygons[p], startPoint, lines); // sorted
	// Infill
	getOrderedPolyLines(normalInfill->infillpolys, startPoint,lines);
	getOrderedPolyLines(fullInfill->infillpolys, startPoint,lines);
	state.AddLines(lines, extrf, offsetZ, E, slicing, hardware);	       
}


void Layer::printinfo() const 
{
  cout <<"Layer at Z="<<Z<<" Lno="<<LayerNo 
       <<", "<<polygons.size() <<" polys"
       <<", "<<shellPolygons.size() <<" shells"
       <<", "<<fullFillPolygons.size() <<" fullfill polys"
       <<", "<<supportPolygons.size() <<" support polys";
  // if (next)
  //   cout <<", next: "<<next->LayerNo;
  // if (previous)
  // cout <<", previous: "<<previous->LayerNo;
  cout << endl;
}



void Layer::Draw(bool DrawVertexNumbers, bool DrawLineNumbers, 
		 bool DrawOutlineNumbers, bool DrawCPLineNumbers, 
		 bool DrawCPVertexNumbers, bool DisplayInfill) const 
{
  glColor3f(1,0,0);
  glLineWidth(1);
  //  cerr <<"draw "<< polygons.size()<< " polys"<< endl;
  for(size_t p=0; p<polygons.size();p++)
    {
      polygons[p].draw(GL_LINE_LOOP);
      polygons[p].draw(GL_POINTS);
      
      if(DrawOutlineNumbers)
	{
	  ostringstream oss;
	  oss << p;
	  renderBitmapString(Vector3f(polygons[p].center.x, polygons[p].center.y, Z) , GLUT_BITMAP_8_BY_13 , oss.str());
	}
    }

  // cerr <<"draw "<< shellPolygons.size()<< " shells"<< endl;
  
  glColor4f(1.,1.,.2,1);
  glPointSize(3);
  glLineWidth(1);
  //cerr << "draw " << shellPolygons.size() << " shellpolys"<<endl;
  for(size_t p=0;p<shellPolygons.size();p++)
    for(size_t q=0;q<shellPolygons[p].size();q++)
      {
	//cerr << "draw " << p << ":"<<q<<endl;
	shellPolygons[p][q].draw(GL_LINE_LOOP);
	shellPolygons[p][q].draw(GL_POINTS);
      }
  
  // cerr <<"draw "<< skinPolygons.size()<< " skins"<< endl;
  
  glColor4f(0.5,0.9,1,1);
  glLineWidth(1);
  double zs = Z;
  for(size_t s=0;s<skins;s++){
    for(size_t p=0;p<skinPolygons.size();p++)
      skinPolygons[p].draw(GL_LINE_LOOP,zs);
    zs-=thickness/skins;
  }
  
  glColor4f(0.5,0.8,0.8,.7);
  glLineWidth(2);
  skirtPolygon.draw(GL_LINE_LOOP);
  glLineWidth(1);
  
  glColor4f(1,1,1,1);
  glPointSize(3);
  for(size_t p=0;p<offsetPolygons.size();p++)
    {
      offsetPolygons[p].draw(GL_LINE_LOOP);
      //offsetPolygons[p].draw(GL_POINTS);
    }
  
  glColor4f(0.5,0.5,1.0,1);
  glPointSize(3);
  glLineWidth(3);
  //cerr << "draw " << supportPolygons.size() << " supportpolys"<<endl;
  for(size_t p=0;p<supportPolygons.size();p++)
    {
      //cerr << "supp draw " << p <<endl;
      supportPolygons[p].draw(GL_LINE_LOOP);
      supportPolygons[p].draw(GL_POINTS);
    }
  glLineWidth(1);
  
  if(DisplayInfill)
    {
      glColor4f(0.5,0.5,0.5,0.8);
      glPointSize(3);
      glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);
      for(size_t p=0;p<fullFillPolygons.size();p++)
	{
	  //cerr << "drawing full poly no " <<p << " with size " << fullFillPolygons[p].size() <<endl;
	  fullFillPolygons[p].draw(GL_LINE_LOOP);
	  //fullFillPolygons[p].draw(GL_POLYGON, true);
	}
      glLineWidth(1);
      glColor4f(0.1,1,0.1,0.8);
      
      // mimick gcode optimization of ordered lines
      // Vector2d startp(0,0);
      // vector<Vector3d> lines;
      // getOrderedPolyLines(infill->infillpolys, startp, lines);
      // glBegin(GL_LINES);	  
      // for(size_t p=0;p < lines.size();p++)	      
      //   {	      
      // 	glVertex3f(lines[p].x,lines[p].y,lines[p].z);
      //   }
      // glEnd();
      for(size_t p=0;p < normalInfill->infillpolys.size();p++)
	normalInfill->infillpolys[p].draw(GL_LINE_LOOP);
      for(size_t p=0;p < fullInfill->infillpolys.size();p++)
	fullInfill->infillpolys[p].draw(GL_LINE_LOOP);
      for(size_t p=0;p < supportInfill->infillpolys.size();p++)
	supportInfill->infillpolys[p].draw(GL_LINE_LOOP);
    }
  glLineWidth(1);
  
  if(DrawCPVertexNumbers)
    for(size_t p=0; p<polygons.size();p++)
      polygons[p].drawVertexNumbers();
  
  if(DrawCPLineNumbers)
    for(size_t p=0; p<polygons.size();p++)
      polygons[p].drawLineNumbers();
  

  
}
