/*******************************************************************************
Copyright (c) 2012, Jonathan Hiller

This file is part of the AMF Tools suite. http://amf.wikispaces.com/
AMF Tools is free software: you can redistribute it and/or modify it under the terms of the GNU Lesser General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
AMF Tools is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more details.
See <http://www.opensource.org/licenses/lgpl-3.0.html> for license details.
*******************************************************************************/

#include "MeshSlice.h"
#include <algorithm>
#include "SimpleImage.h"

//#include "nMaterial.h" //TODO: Get rid of this AMF dependence!!!
//#include "AMF_Render.h" //GET rid of this, too...
#include "AMF_File.h" //...and this...






CMeshSlice::CMeshSlice(void)
{
	pMaterial = NULL;
//	pObjExt = NULL;
	Initialized = false;
	pGetColor = NULL;

}


CMeshSlice::~CMeshSlice(void)
{
}

void CMeshSlice::MeshChanged(void) //invalidates all cached voxelizing info!
{
	if (Initialized){
		Initialized = false;
		_TriLayerZ = -1;
		_TriLayerPZ = -1;
		CurTriLayer.clear();
		YEnvMaxIterator = CurTriLayer.begin();
		_TriLineY = -1;
		_TriLinePY = -1;
		CurTriLine.clear();
		XEnvMaxIterator = CurTriLine.begin();
		Envelopes.clear();
		ZEnvMaxIterator = Envelopes.begin(); //but no elements... basically just voids it!
		_ZActual = -1;
		_YActual = -1;
		_XActual = -1;
		CurTriPoints.clear();
		XPtIter = CurTriPoints.begin();
		_TriPointPX = -1;
		_IsCurInside = false;
		CurTriPoint.clear();
	}
}

double CMeshSlice::GetClosestFreeZ(double ZIn) //returns the next Z value that does not intersect any vertices
{
	double CurZ = ZIn;
//	double MultFact = 1.0;
	for (std::vector<CVertex>::iterator it = Vertices.begin(); it != Vertices.end(); it++){
		if (it->v.z == CurZ){
			CurZ += 2*CurZ*DBL_EPSILON;
			//MultFact *= 2.0;
			it = Vertices.begin(); //-1 is because it will increment before next time around, and we want to start over from 0
		}
	}
	return CurZ;
}

//double CMeshSlice::GetClosestFreeY(double YIn) //returns the next Y value that does not intersect any vertices IN CurTriLayer!
//{
//	double CurY = YIn;
//	for (std::vector<CFacet>::iterator it = Facets.begin(); it != Facets.end(); it++){
//		if (Vertices[it->vi[0]].v.y == CurY || Vertices[it->vi[1]].v.y == CurY || Vertices[it->vi[2]].v.y == CurY){
//			CurY += DBL_EPSILON;
//			it = Facets.begin()-1; //-1 is because it will increment before next time around, and we want to start over from 0
//		}
//	}
//	return CurY;
//}

void CMeshSlice::FillTriLayer(double z, double ZPad) //fills in TriHeight with all triangles that bridge this plane
{
	if (ZPad < 0) ZPad = -ZPad; // only positive!
	if(z == _TriLayerZ && ZPad == _TriLayerPZ) return; //if we've already done this...
	double LastMaxZ = _ZActual + _TriLayerPZ;
	double LastMinZ = _ZActual - _TriLayerPZ;
	_ZActual = GetClosestFreeZ(z);
	double CurMaxZ = _ZActual+ZPad;
	double CurMinZ = _ZActual-ZPad;
	_TriLayerZ = z; //cache these calls
	_TriLayerPZ = ZPad;

		//optimized for sequential ascending Z steps, but works otherwise...
	if (CurMaxZ < LastMaxZ || CurMinZ < LastMinZ){ //if we've gone backwards (downwards) we have to start over from the beginning!
		CurTriLayer.clear(); //start over...
		ZEnvMaxIterator = Envelopes.begin(); 
	}

	//Add in triangles with minimum Z envelopes less than the CurMaxZ
	while (ZEnvMaxIterator != Envelopes.end() && ZEnvMaxIterator->MinZ <= CurMaxZ){ //add in all newly within range, just from where we left the iterator...
		if(ZEnvMaxIterator->MaxZ >= CurMinZ) CurTriLayer.push_back(*ZEnvMaxIterator); // only add if Max Z Envelope is less than CurMinZ before adding
		ZEnvMaxIterator++;
	}

	//remove elements elready in list with max Z envelope less than CurMinZ
	std::list<TriEnvelope>::iterator it = CurTriLayer.begin();
	while (it != CurTriLayer.end()){	//get rid of all elements in the list with maximum values less than MinZ
		if (it->MaxZ<CurMinZ){
			it=CurTriLayer.erase(it); //points to next element after erasing this one
		}
		else it++;
	}

	//sort the CurTriLayer list by y
	CurTriLayer.sort(TriEnvelope::MinYLessThan);
	YEnvMaxIterator = CurTriLayer.begin();
	CurTriLine.clear(); //we've made a new layer array, so clear out all the line info...

	////add any Facets whose Z coordinates are not all above or all below this Z plane
	//bool IsAbove, IsBelow;
	//int m;
	//double CurZ;
	//std::vector<CFacet>::iterator FIter;
	//bool NoneEqual = false;
	//
	//while (!NoneEqual){
	//	NoneEqual = true;
	//	CurTriLayer.clear(); //clear previous list
	//	m=0;
	//	for (FIter = Facets.begin(); FIter != Facets.end(); FIter++){
	//		IsAbove = true; IsBelow = true;

	//		for (int n=0; n<3; n++){
	//			CurZ = Vertices[FIter->vi[n]].v.z;
	//		
	//			if(CurZ > z) IsBelow = false;
	//			else if(CurZ < z) IsAbove = false;
	//			else /* if(CurZ == z) */{ //START OVER with infintesimebly incremented Z value
	//				z += FLT_EPSILON;
	//				NoneEqual = false;
	//				break; break; //start the whole process over
	//			};
	//		}
	//		if (!IsAbove && !IsBelow) CurTriLayer.push_back(m); //if this facet is not fully above or fully below our ZPlane
	//	
	//		m++;
	//	}
	//}
}

bool CMeshSlice::FillTriLine(double y, double z, double YPad, double ZPad) //fills in TriHeight with all triangles that bridge this plane
{
	if (YPad < 0) YPad = -YPad; // only positive!
	if(y == _TriLineY && z == _TriLayerZ && YPad == _TriLinePY && ZPad == _TriLayerPZ) return true; //if we've already done this...
	FillTriLayer(z, ZPad); //exits immediately if z is the same as cached...
	if (CurTriLayer.size() == 0){CurTriLine.clear(); return true;} //if no triangles in this layer, no lines! easy!

	double LastMaxY = _YActual + _TriLinePY;
	double LastMinY = _YActual - _TriLinePY;
	_YActual = y; //start here. We will increment if we hit an edge dead on...
	_TriLineY = y; //cache these calls
	_TriLinePY = YPad;

	bool HitEdge = true;
	while(HitEdge){
		HitEdge = false;
		double CurMaxY = _YActual+YPad;
		double CurMinY = _YActual-YPad;
	

		//optimized for sequential ascending Y steps, but works otherwise...
		if (CurMaxY < LastMaxY || CurMinY < LastMinY){ //if we've gone backwards (downwards) we have to start over from the beginning!
			CurTriLine.clear(); //start over...
			YEnvMaxIterator = CurTriLayer.begin(); 
		}

		//Add in triangles with minimum Y envelopes less than the CurMaxY
		while (YEnvMaxIterator != CurTriLayer.end() && YEnvMaxIterator->MinY <= CurMaxY){ //add in all newly within range, just from where we left the iterator...
			if(YEnvMaxIterator->MaxY >= CurMinY) CurTriLine.push_back(*YEnvMaxIterator); // only add if Max Y Envelope is less than CurMinY before adding
			YEnvMaxIterator++;
		}

		//remove elements elready in list with max Y envelope less than CurMinY
		std::list<TriEnvelope>::iterator it = CurTriLine.begin();
		while (it != CurTriLine.end()){	//get rid of all elements in the list with maximum values less than MinY
			if (it->MaxY<CurMinY){
				it=CurTriLine.erase(it); //points to next element after erasing this one
			}
			else it++;
		}

		//sort the CurTriLayer list by y
		CurTriLine.sort(TriEnvelope::MinXLessThan);
		XEnvMaxIterator = CurTriLine.begin();
		CurTriPoint.clear(); //we've made a new line, so clear out all the point info!

		if (!FillTriPoints()){
			_YActual += 2*_YActual*DBL_EPSILON; //increment _YActual and try again. This should make only very small changes to CurTriLine.
			HitEdge = true;
		}

	}
	return true;
	/*
	IntersectionType IntType;
	Vec3D p;
	double pu, pv, V1y, V2y, V3y;

	//add any Facets whose Y coordinates are not all above or all below this y plane (from within those in the Z plane)
	bool NoneEqual = false;
	
	while (!NoneEqual){
		NoneEqual = true;
		CurTriLine.clear(); //clear previous list
		std::list<TriEnvelope>::iterator ZFIter;
		for (ZFIter = CurTriLayer.begin(); ZFIter != CurTriLayer.end(); ZFIter++){
			V1y = Vertices[Facets[ZFIter->TriIndex].vi[0]].v.y;
			V2y = Vertices[Facets[ZFIter->TriIndex].vi[1]].v.y;
			V3y = Vertices[Facets[ZFIter->TriIndex].vi[2]].v.y;
			//trivial checks (should get most of them...)
			if (V1y < y && V2y < y && V3y < y)
				continue;
			if (V1y > y && V2y > y && V3y > y)
				continue;

			//IntersectLine(
			double tmp;
//			IntType = IntersectXRay(&Facets[*ZFIter], y, z, p, pu, pv);
			IntersectionType IntType2 = IntersectXRay(&Facets[ZFIter->TriIndex], y, z, tmp);
//			if (IntType != IntType2 || (IntType==IT_INSIDE && p.x != tmp))
//				int UhOh=1;

			IntType = IntType2;
			p.x = tmp;

			if (IntType == IT_EDGE){ //if any equal, add a tiny amount to Y
				y += FLT_EPSILON;
				NoneEqual = false;
				break; break;
			}
			else if (IntType == IT_INSIDE){
	//		if(IntersectXRay(&Facets[*ZFIter], y, z, p, pu, pv)) { //if it intersects
//				if (InsideTri(p, Vertices[Facets[*ZFIter].vi[0]].v, Vertices[Facets[*ZFIter].vi[1]].v, Vertices[Facets[*ZFIter].vi[2]].v)){
					CurTriLine.push_back(p.x); 
				}
//			}
		}
	}
	if (CurTriLine.size()%2 ==1) return false;

	std::sort(CurTriLine.begin(), CurTriLine.end());

	return true;
	*/
}

bool CMeshSlice::FillTriPoints(void) //fills points based on _YActual _ZActual, and the tris in TriLine. returns false increments _YActual (to re-do FillTriLine with) and try again.
{
	IntersectionType IntType;
	double TmpXIntersect;

	CurTriPoints.clear();
	for (std::list<TriEnvelope>::iterator it = CurTriLine.begin(); it != CurTriLine.end(); it++){
		if (_YActual <= it->MaxY && _YActual >= it->MinY && _ZActual <= it->MaxZ && _ZActual >= it->MinZ){
			IntType = IntersectXRay(&Facets[it->TriIndex], _YActual, _ZActual, TmpXIntersect);
			if (IntType == IT_EDGE)	return false;
			else if (IntType == IT_INSIDE) CurTriPoints.push_back(TmpXIntersect);
		}
	}

	std::sort(CurTriPoints.begin(), CurTriPoints.end());
	XPtIter = CurTriPoints.begin(); //ready to iterate
	_IsCurInside = false; //not inside the part if we just reset this array!!
	return true;
}

//---------------------------------------------------------------------------
bool CMeshSlice::IsInside(Vec3D* Point, double TexDepth, CColor* pColor)
//---------------------------------------------------------------------------
{
	if (TexDepth<0) TexDepth = -TexDepth;
	FillTriLine(Point->y, Point->z, TexDepth, TexDepth); //returns very fast if previously used z or y layers...



	//////
	double LastMaxX = _XActual + _TriPointPX;
	double LastMinX = _XActual - _TriPointPX;
	_XActual = Point->x;
	_TriPointPX = TexDepth;
	double CurMaxX = _XActual + _TriPointPX;
	double CurMinX = _XActual - _TriPointPX;


	//optimized for sequential ascending X steps, but works otherwise...
	if (CurMaxX < LastMaxX || CurMinX < LastMinX){ //if we've gone backwards (downwards) we have to start over from the beginning!
		_IsCurInside = false;
		CurTriPoint.clear();
		XPtIter = CurTriPoints.begin(); //reset the iterators
		XEnvMaxIterator = CurTriLine.begin();
	}

	//Add in triangles with minimum X envelopes less than the CurMaxX
	while (XEnvMaxIterator != CurTriLine.end() && XEnvMaxIterator->MinX <= CurMaxX){ //add in all newly within range, just from where we left the iterator...
		if(XEnvMaxIterator->MaxX >= CurMinX) CurTriPoint.push_back(*XEnvMaxIterator); // only add if Max Z Envelope is less than CurMinZ before adding
		XEnvMaxIterator++;
	}

	//remove elements elready in list with max X envelope less than CurMinZ
	std::list<TriEnvelope>::iterator it = CurTriPoint.begin();
	while (it != CurTriPoint.end()){	//get rid of all elements in the list with maximum values less than MinZ
		if (it->MaxX<CurMinX){
			it=CurTriPoint.erase(it); //points to next element after erasing this one
		}
		else it++;
	}


	//figure out if we're inside or outside the body
	while (XPtIter != CurTriPoints.end() && *XPtIter <= _XActual){ //add in all newly within range, just from where we left the iterator...
		_IsCurInside = !_IsCurInside; //every time we cross a point, we've crossed a boundary!
		XPtIter++;
	}

	if (_IsCurInside){
		if (pColor){
			double u, v, dist2, mindist2 = 9e9;
			*pColor = CColor();

			for (std::list<TriEnvelope>::iterator it = CurTriPoint.begin(); it != CurTriPoint.end(); it++){ //iterate through all the triangle within scope
				CFacet* pCurFacet = &Facets[it->TriIndex];
				GetTriDist(pCurFacet, Point, u, v, dist2);
				if (dist2<TexDepth*TexDepth){ //if we're closer than the texture depth...
					if (dist2<mindist2){ //FOR NOW just take minimum... eventually average?
						mindist2 = dist2;

						double PicU, PicV;
						bool UseTexColors = false;
						CTexture* pCurTexture = NULL;
						if (pCurFacet->HasTexture){	
							if (pCurFacet->Map.TexIndex < (int)Textures.size()) pCurTexture = &Textures[pCurFacet->Map.TexIndex];
							
							//u coord is from vertex 1 to vertex 2, v coord is v1 to v3
							PicU = pCurFacet->Map.uc[0] + u*(pCurFacet->Map.uc[1]-pCurFacet->Map.uc[0]) + v*(pCurFacet->Map.uc[2]-pCurFacet->Map.uc[0]);
							PicV = pCurFacet->Map.vc[0] + u*(pCurFacet->Map.vc[1]-pCurFacet->Map.vc[0]) + v*(pCurFacet->Map.vc[2]-pCurFacet->Map.vc[0]);
							if (PicU == 1.0) PicU = 1.0-DBL_MIN; //stay off the upper edge without grabbing a pix from 0.0...
							if (PicV == 1.0) PicV = 1.0-DBL_MIN; //stay off the upper edge...

							if (pCurTexture && pCurTexture->Tiled){ //if we're tiling the textures do the mod...
								UseTexColors = true;
								PicU = fmod(PicU, 1.0);
								if (PicU < 0) PicU += 1.0; //becuase fmod of a negative number is negative remainder
								PicV = fmod(PicV, 1.0);
								if (PicV < 0) PicV += 1.0; //becuase fmod of a negative number is negative remainder

							}
							else { //not tiling texture, only do texture if we're within [0, 1]
								if (PicU >= 0 && PicU < 1 && PicV >= 0 && PicV < 1) UseTexColors = true;
							}

						}
						if (UseTexColors && pCurTexture){
							int XPix = (int)(PicU*pCurTexture->Width);
							//int YPix = Texture.Height - (int)(PicV*Texture.Height)-1;
							int YPix = (int)(PicV*pCurTexture->Height);
						
							unsigned char* pPix = &(pCurTexture->RGBAImage[4*(YPix*pCurTexture->Width + XPix)]);
							*pColor = CColor(*pPix/255.0, *(pPix+1)/255.0, *(pPix+2)/255.0, *(pPix+3)/255.0);
						}
						else { //no texture, so linearly interpolate the vertices!
							CColor V0 = Vertices[pCurFacet->vi[0]].VColor;
							CColor V1 = Vertices[pCurFacet->vi[1]].VColor;
							CColor V2 = Vertices[pCurFacet->vi[2]].VColor;

							double R = V0.r + u*(V1.r-V0.r) + v*(V2.r-V0.r);
							double G = V0.g + u*(V1.g-V0.g) + v*(V2.g-V0.g);
							double B = V0.b + u*(V1.b-V0.b) + v*(V2.b-V0.b);
							*pColor = CColor(R, G, B, 1.0);
						}
					}
				}
			}


		}
		return true;
	}
	else return false;


	//////




	//std::vector<double>::iterator LIter;
	//int count = 0;
	//for (LIter = CurTriLine.begin(); LIter != CurTriLine.end(); LIter++){
	//	if (Point->x < *LIter) break;
	//	count ++;

	//}
	//if (count%2 == 1) return true; //if we've passed an odd number of facets...
	//else return false;


}
/*

//---------------------------------------------------------------------------
int CMeshSlice::GetXIntersections(double z, double y, double* pIntersections, int NumtoCheck, int* pToCheck)
//---------------------------------------------------------------------------
{ //returns the number of intersections, stored in pIntersections. pToCheck is a vector of facet indices that are in this Z plane...
	Vec3D p;
	double pu, pv, V1y, V2y, V3y;
	int NumFound = 0;

	for (int i=0; i<NumtoCheck; i++){ //for each facet we wish to check...
		V1y = Vertices[Facets[pToCheck[i]].vi[0]].v.y;
		V2y = Vertices[Facets[pToCheck[i]].vi[1]].v.y;
		V3y = Vertices[Facets[pToCheck[i]].vi[2]].v.y;
		//trivial checks (should get most of them...)
		if (V1y < y && V2y < y && V3y < y)
			continue;
		if (V1y > y && V2y > y && V3y > y)
			continue;

		if(IntersectXRay(&Facets[pToCheck[i]], y, z, p, pu, pv)) { //if it intersects
			if (InsideTri(p, Vertices[Facets[pToCheck[i]].vi[0]].v, Vertices[Facets[pToCheck[i]].vi[1]].v, Vertices[Facets[pToCheck[i]].vi[2]].v)){
				pIntersections[NumFound++] = p.x; //(1.0 - pu - pv)*Vertices[Facets[pToCheck[i]].vi[0]].v.x + pu*Vertices[Facets[pToCheck[i]].vi[1]].v.x + pv*Vertices[Facets[pToCheck[i]].vi[2]].v.x;
			}
		}
	}
	
//	if (NumFound%2 != 0) std::cout << "Uh-oh! Found an odd number of intersections!";
	
	//sort intersections... (bubble sort = slow, but these should be super small...
	double tmp;
	for (int i=0; i<NumFound; i++){
		for (int j=0; j<NumFound - i - 1; j++){ //each iteration gets the largest element to the end...
			if(pIntersections[j] > pIntersections[j+1]){
				tmp = pIntersections[j+1];
				pIntersections[j+1] = pIntersections[j];
				pIntersections[j] = tmp;
			}
		}
	}

	return NumFound;
}

//---------------------------------------------------------------------------
bool CMeshSlice::InsideTri(Vec3D& p, Vec3D& v0, Vec3D& v1, Vec3D& v2)
//---------------------------------------------------------------------------
{// True if point p projects to within triangle (v0;v1;v2)

	Vec3D xax = (v1-v0).Normalized();
	Vec3D zax = ((v2-v0).Cross(xax)).Normalized();
	Vec3D yax = zax.Cross(xax).Normalized();

	Vec3D p0(0,0,1);
	Vec3D p1((v1-v0).Dot(xax),(v1-v0).Dot(yax),1);
	Vec3D p2((v2-v0).Dot(xax),(v2-v0).Dot(yax),1);
	Vec3D pt((p-v0).Dot(xax),(p-v0).Dot(yax),1);

	double d0 = Det(p0,p1,pt);
	double d1 = Det(p1,p2,pt);
	double d2 = Det(p2,p0,pt);

	if (d0<=0 && d1<=0 && d2<=0)
		return true;
	if (d0>=0 && d1>=0 && d2>=0)
		return true;

	return false;

}

//---------------------------------------------------------------------------
double CMeshSlice::Det(Vec3D& v0, Vec3D& v1, Vec3D& v2)
//---------------------------------------------------------------------------
{ // Compute determinant of 3x3 matrix v0,v1,v2

	return 

		v0.x*(v1.y*v2.z-v1.z*v2.y) +
		v0.y*(v1.z*v2.x-v1.x*v2.z) +
		v0.z*(v1.x*v2.y-v1.y*v2.x);

}
*/
IntersectionType CMeshSlice::IntersectXRay(CFacet* pFacet, double y, double z, double& XIntersect)
{
	//http://www.blackpawn.com/texts/pointinpoly/default.html

	Vec3D vA = Vertices[pFacet->vi[0]].v;
	Vec3D vB = Vertices[pFacet->vi[1]].v;
	Vec3D vC = Vertices[pFacet->vi[2]].v;

	double v0y = vC.y-vA.y; //u
	double v0z = vC.z-vA.z;
	double v1y = vB.y-vA.y; //v
	double v1z = vB.z-vA.z;
	double v2y = y-vA.y;
	double v2z = z-vA.z;

	double dot00=v0y*v0y+v0z*v0z;
	double dot01=v0y*v1y+v0z*v1z;
	double dot02=v0y*v2y+v0z*v2z;
	double dot11=v1y*v1y+v1z*v1z;
	double dot12=v1y*v2y+v1z*v2z;

	double invDenom = 1.0/(dot00*dot11-dot01*dot01);
	double u=(dot11*dot02-dot01*dot12)*invDenom;
	double v=(dot00*dot12-dot01*dot02)*invDenom;

	if ((u >= 0) && (v >= 0) && (u + v <= 1)){
		if (u == 0 || v == 0 || u+v==1) 
			return IT_EDGE;
		else {
			XIntersect = vA.x+u*(vC.x-vA.x)+v*(vB.x-vA.x);
			return IT_INSIDE;
		}
	}
	else return IT_OUTSIDE;
}

bool CMeshSlice::GetTriDist(CFacet* pFacetCheck, Vec3D* pPointIn, double& UOut, double& VOut, double& Dist2Out) //gets distance of provided point to closest UV coordinate of within the triangle. returns true if sensible distance, otherwise false
{
	//http://www.geometrictools.com/Documentation/DistancePoint3Triangle3.pdf

	//Regions:
	//    E1 (t)
	// \ 2|
	//   \|
	//	  |\
	//    |  \    1
	//  3 |    \
	//    | 0    \
	// ___|________\____   E0 (s)
	//    |          \ 6
	// 4  |    5       \

	

	Vec3D B = Vertices[pFacetCheck->vi[0]].v;
	Vec3D E0 = Vertices[pFacetCheck->vi[1]].v - B; //(s)
	Vec3D E1 = Vertices[pFacetCheck->vi[2]].v - B; //(t)
	Vec3D D = B - *pPointIn;

	double a = E0.Dot(E0);
	double b = E0.Dot(E1);
	double c = E1.Dot(E1);
	double d = E0.Dot(D);
	double e = E1.Dot(D);
	double f = D.Dot(D);

	double det=a*c-b*b;
	double s = b*e-c*d;
	double t = b*d-a*e;

	//determine which region...
	//Q(s,t) = as^2+2bst+ct^2+2ds+2et+f
	if (s+t <= det){
		if (s<0){
			if (t<0){ //Region 4 //TODO: CHECK ACCURATE!
				// Grad(Q) = 2(as+bt+d,bs+ct+e) :: (derivative w respect to s, t)
				// (0,1)*Grad(Q(0,0)) = (0,1)*(d,e) = e :: is grad from tip of region 2 in directon of leg toward origin (0,1)...
				// (1,0)*Grad(Q(0,0)) = (1,0)*(d,e) = d
				// min on edge t=0 if (0,1)*Grad(Q(0,0)) < 0 )
				// min on edge s=0 otherwise
				if (e<0){ // minimum on edge t=0
					t=0;
					t=(d >=0 ? 0 :(-d >=a ? 1:-d/a));
				}
				else { // minimum on edge s=0
					s=0;
					t=(e >=0 ? 0 :(-e >=c ? 1:-e/c));
				}
			}
			else { //Region 3
				// F(t) = Q(0,t) = ct^2 + 2et + f
				// F’(t)/2 = ct+e
				// F’(T) = 0 when T = -e/c
				s=0;
				t=(e >=0 ? 0 :(-e >=c ? 1:-e/c));
			}
		}
		else if (t<0){ //Region 5 (like region 3)
			// F(s) = Q(s,0) = as^2 + 2ds + f
			// F’(s)/2 = as+d
			// F’(S) = 0 when S = -d/a
			t=0;
			t=(d >=0 ? 0 :(-d >=a ? 1:-d/a));
		}
		else { //Region 0
			double invdet = 1/det;
			s *= invdet;
			t *= invdet;
		}
	}
	else {
		if (s<0){ //Region 2
			// Grad(Q) = 2(as+bt+d,bs+ct+e) :: (derivative w respect to s, t)
			// (0,-1)*Grad(Q(0,1)) = (0,-1)*(b+d,c+e) = -(c+e) :: is grad from tip of region 2 in directon of leg toward origin (0,1)...
			// (1,-1)*Grad(Q(0,1)) = (1,-1)*(b+d,c+e) = (b+d)-(c+e)
			// min on edge s+t=1 if (1,-1)*Grad(Q(0,1)) < 0 )
			// min on edge s=0 otherwise
			double tmp0 = b+d;
			double tmp1 = c+e;
			if ( tmp1 > tmp0 ){ // minimum on edge s+t=1
				double numer = tmp1 - tmp0;
				double denom = a-2*b+c;
				s = ( numer >= denom ? 1 : numer/denom );
				t = 1-s;
			}
			else { // minimum on edge s=0
				s = 0;
				t = ( tmp1 <= 0 ? 1 : ( e >= 0 ? 0 : -e/c ) );
			}
		}
		else if (t<0){ //Region 6 //TODO: CHECK ACCURATE!
			// Grad(Q) = 2(as+bt+d,bs+ct+e) :: (derivative w respect to s, t)
			// (-1,0)*Grad(Q(1,0)) = (-1,0)*(a+d,b+e) = -(a+d) :: is grad from tip of region 2 in directon of leg toward origin (0,1)...
			// (-1,1)*Grad(Q(1,0)) = (-1,1)*(a+d,b+e) = (b+e)-(a+d)
			// min on edge s+t=1 if (-1,1)*Grad(Q(0,1)) < 0 )
			// min on edge t=0 otherwise
			double tmp0 = b+e;
			double tmp1 = a+d;
			if ( tmp1 > tmp0 ){ // minimum on edge s+t=1
				double numer = tmp1 - tmp0;
				double denom = a-2*b+c;
				t = ( numer >= denom ? 1 : numer/denom );
				s = 1-t;
			}
			else { // minimum on edge t=0
				t = 0;
				s = ( tmp1 <= 0 ? 1 : ( d >= 0 ? 0 : -d/a ) );
			}
		}
		else { //Region 1
			// F(s) = Q(s,1-s) = (a-2b+c)s^2 + 2(b-c+d-e)s + (c+2e+f)
			// F’(s)/2 = (a-2b+c)s + (b-c+d-e)
			// F’(S) = 0 when S = (c+e-b-d)/(a-2b+c)
			// a-2b+c = |E0-E1|^2 > 0, so only sign of c+e-b-d need be considered
			double numer = c+e-b-d;
			if (numer <= 0) s=0;
			else {
				double denom = a-2*b+c;
				s = (numer >= denom ? 1 : numer/denom);
			}
			t=1-s;
		}
	}

	//check one or the other? should be same!
	Vec3D ClosestPoint = B+s*E0+t*E1;
	double D2 = (ClosestPoint-*pPointIn).Length2();
	double D22 = a*s*s+2*b*s*t+c*t*t+2*d*s+2*e*t+f;

	UOut = s;
	VOut = t;
	Dist2Out = D22;
	return true;
}


/*
//---------------------------------------------------------------------------
IntersectionType CMeshSlice::IntersectXRay(CFacet* pFacet, double y, double z, Vec3D& p, double& pu, double& pv)
//---------------------------------------------------------------------------
{
	// compute intersection point P of triangle plane with ray from origin O in direction D
	// D assumed to be normalized
	// if no interstion, return false
	// u and v are barycentric coordinates of the intersection point P = (1 - u - v)A + uB + vC 
	// see http://www.devmaster.net/wiki/Ray-triangle_intersection
	Vec3D d = Vec3D(1,0,0);
	Vec3D o = Vec3D(-1e9, y, z);

	Vec3D a = Vertices[pFacet->vi[0]].v;
	Vec3D b = Vertices[pFacet->vi[1]].v;
	Vec3D c = Vertices[pFacet->vi[2]].v;
	
	//Vec3D n = pFacet->n; //((b-a).Cross(c-a)).Normalized();
	Vec3D n = ((b-a).Cross(c-a)).Normalized();
	//if (n.x > 0){ //flip vertices...
	//	Vec3D tmp = a;
	//	a = b;
	//	b = tmp;
	//	n = ((b-a).Cross(c-a)).Normalized();
	//}

	double dn = d.Dot(n);
	if (fabs(dn)<1E-5)
		return IT_OUTSIDE; //parallel

	double dist = -(o-a).Dot(n)/dn;
	Vec3D sD = d*dist;
	p = o+sD;

	double V1, V2, V3;
	V1 = (b-a).Cross(p-a).Dot(n);
	V2 = (c-b).Cross(p-b).Dot(n);
	V3 = (a-c).Cross(p-c).Dot(n);
	
	if (abs(V1)<1e-12 || abs(V2)<1e-12 || abs(V3)<1e-12) return IT_EDGE; //TODO:FISHY THRESHHOLD!
	if (V1 >=0 && V2 >=0 && V3 >=0) return IT_INSIDE;
//	if (V1 >=0 && V2 >=0 && V2 >=0) return true; <-Was this, but pretty obviously typo I think!!!
	//if (V1 <=0 && V2 <=0 && V2 <=0) return true;
	else return IT_OUTSIDE;

}
*/
bool CMeshSlice::GetSlice(CSimpleImage* pImgOut, double ZHeight, double SurfDepth, double XMin, double XMax, double YMin, double YMax, int XPix, int YPix, int aObjectID, std::string* pMessage)
{
	//initial checks...
	if (XMax == XMin || YMax == YMin || XPix <= 0 || YPix <= 0) return false;
	if (XMin > XMax){ double tmp = XMin; XMin = XMax; XMax = tmp;} //make sure Max is bigger then Min...
	if (YMin > YMax){ double tmp = YMin; YMin = YMax; YMax = tmp;} //make sure Max is bigger then Min...
	
	if (!Initialized) Initialize();

	//prepare the image (should already exist...)
	//*pImgOut = QImage(XPix, YPix, QImage::Format_ARGB32);
	if (!pImgOut->AllocateRGBA(XPix, YPix)){
		if (pMessage) *pMessage += "Insufficient memory to create bitmap. Please lower resolution.\n";
		return false;
	}

	//*pImgOut = QImage(XPix, YPix, QImage::Format_Mono);
	//pImgOut->setColor(0, qRgb(255, 255, 255)); //for now, 1-bit image, index 0 is white
	//pImgOut->setColor(1, qRgb(0, 0, 0)); //index 1 is black
	//pImgOut->fill(0);

	CColor CurColor;

	//Min and Max sections are the very edge of the picture... but we must sample at the center of pixels!
	Vec3D CurPoint = Vec3D(0,0,ZHeight);
	double XStep = (XMax-XMin)/XPix;
	double YStep = (YMax-YMin)/YPix;
	double XBegin = XMin + XStep/2; //offset half pixel to sample at center of grid points
	double YBegin = YMin + YStep/2; //offset half pixel to sample at center of grid points
	
	double tR, tG, tB, tA;

	//do each pixel...
	//QRgb* pCurPixel;
	unsigned char* pCurPixel = pImgOut->GetRGBABits();
	for (int j=0; j<YPix; j++){
		CurPoint.y = YBegin+YStep*j;
//		pCurPixel = (QRgb*)pImgOut->scanLine(j);
		for (int i=0; i<XPix; i++){
			CurPoint.x = XBegin+XStep*i;
			if (IsInside(&CurPoint, SurfDepth, &CurColor)){
				if (CurColor.a == -1){ //if there is no surface texture... (added from within IsInside())
//using AMF info..
					if (pMaterial){ //if material, calculate material color
						Vec3D TmpPt = CurPoint;
//						pAmf->FromCurrentUnits(1.0, UNIT_MM)
//	double UnitScale = pMaterial->pnAmf->GetUnitsScaleFromMm();

//						if (pObjExt) TmpPt = /*UnitScale*pObjExt-> */OriginalLoc(TmpPt);
						TmpPt = OriginalLoc(TmpPt);

						pMaterial->GetColorAt(TmpPt.x, TmpPt.y, TmpPt.z, &tR, &tG, &tB, &tA);
						*pCurPixel = (unsigned char)(tR*255);
						*(pCurPixel+1) = (unsigned char)(tG*255);
						*(pCurPixel+2) = (unsigned char)(tB*255);
						*(pCurPixel+3) = (unsigned char)(tA*255);
					}
					else { //if no material defined, put opaque black
						*pCurPixel = *(pCurPixel+1) = *(pCurPixel+2) = 0;
						*(pCurPixel+3) = 255;
					}

//using agnostic info...
					//if (pGetColor){
					//	pGetColor(CurPoint.x, CurPoint.y, CurPoint.z, &tR, &tG, &tB, &tA, aObjectID);
					//	*pCurPixel = qRgba(tR*255, tG*255, tB*255, tA*255);
					//}
					//else *pCurPixel = qRgba(0, 0, 0, 255);
				}
				else { //if there was a surface texture here, use the color returned by IsInside()
					*pCurPixel = (unsigned char)(CurColor.r*255);
					*(pCurPixel+1) = (unsigned char)(CurColor.g*255);
					*(pCurPixel+2) = (unsigned char)(CurColor.b*255);
					*(pCurPixel+3) = (unsigned char)(CurColor.a*255);
				}
			}
			else { //if not inside, make it transparent!
				*pCurPixel = *(pCurPixel+1) = *(pCurPixel+2) = *(pCurPixel+3) = 0;
			}
			pCurPixel += 4; //go to next pixel
		}
	}
	return true;
}

void CMeshSlice::Initialize()
{
	Initialized = true;

	int NumFacets = Facets.size();
	Envelopes.reserve(NumFacets);
	for (int i=0; i<NumFacets; i++){	
		Envelopes.push_back(TriEnvelope(*this, i));
	}

	std::sort(Envelopes.begin(), Envelopes.end(), TriEnvelope::MinZLessThan); //sort in ascending minimum Z
	ZEnvMaxIterator = Envelopes.begin();
}

Vec3D CMeshSlice::OriginalLoc(Vec3D& vIn){ //returns the location of vIn in the original coordinate system of the object

	CQuat ToRotate = Rot.Inverse();
	return (vIn-Offset).Rot(ToRotate);
}
