/* -------------------------------------------------------- *
*
* processcontroller.cpp
*
* Copyright 2009+ Michael Holm - www.kulitorum.com
*
* This file is part of RepSnapper and is made available under
* the terms of the GNU General Public License, version 2, or at your
* option, any later version, incorporated herein by reference.
*
* ------------------------------------------------------------------------- */
#include "stdafx.h"
#include "xml/xml.h"
#include "modelviewcontroller.h"
#include "processcontroller.h"
#include <boost/algorithm/string.hpp>

void ProcessController::ConvertToGCode(string &GcodeTxt, const string &GcodeStart, const string &GcodeLayer, const string &GcodeEnd)
{
	if(gui)
	{
		gui->ProgressBar->value(0);
		gui->ProgressBar->label("Converting");
		gui->ProgressBar->maximum(Max.z);
	}

	// Make Layers
	uint LayerNr = 0;
	uint LayerCount = (uint)ceil((Max.z+LayerThickness*0.5f)/LayerThickness);

	vector<int> altInfillLayers;
	GetAltInfillLayers(altInfillLayers, LayerCount);

	printOffset = PrintMargin;

	float z=Min.z+LayerThickness*0.5f;				// Offset it a bit in Z, z=0 gives a empty slice because no triangles crosses this Z value

	gcode.commands.clear();

	float printOffsetZ=PrintMargin.z;

	if(RaftEnable)
	{
		printOffset += Vector3f(RaftSize, RaftSize, 0);
		MakeRaft(printOffsetZ);
	}
	float E=0.0f;
	while(z<Max.z)
	{
		if(gui)
		{
			gui->ProgressBar->value(z);
			gui->ProgressBar->redraw();
			Fl::check();
		}
		for(uint o=0;o<rfo.Objects.size();o++)
		{
			for(uint f=0;f<rfo.Objects[o].files.size();f++)
			{
				STL* stl = &rfo.Objects[o].files[f].stl;	// Get a pointer to the object
				Matrix4f T = GetSTLTransformationMatrix(o,f);
				Vector3f t = T.getTranslation();
				t+= Vector3f(PrintMargin.x+RaftSize*RaftEnable, PrintMargin.y+RaftSize*RaftEnable, 0);
				T.setTranslation(t);
				CuttingPlane plane;
				stl->CalcCuttingPlane(z, plane, T);	// output is alot of un-connected line segments with individual vertices, describing the outline

				float hackedZ = z;
				while (plane.LinkSegments (hackedZ, Optimization) == false)	// If segment linking fails, re-calc a new layer close to this one, and use that.
				{ // This happens when there's triangles missing in the input STL
					hackedZ+= 0.1f;
					stl->CalcCuttingPlane (hackedZ, plane, T);	// output is alot of un-connected line segments with individual vertices
				}
				plane.SetZ (z + printOffsetZ);

				// inFill
				vector<Vector2f> infill;

//				CuttingPlane infillCuttingPlane;
//				plane.MakeContainedPlane(infillCuttingPlane);
				if(ShellOnly == false)
				{
					switch( m_ShrinkQuality )
					{
					case SHRINK_FAST:
						plane.ShrinkFast(ExtrudedMaterialWidth*0.5f, Optimization, DisplayCuttingPlane, false, ShellCount);
						break;

					case SHRINK_NICE:
#if defined(ENABLE_GPC) && ENABLE_GPC
						plane.ShrinkNice(ExtrudedMaterialWidth*0.5f, Optimization, DisplayCuttingPlane, false, ShellCount);
#else
						// "Warning: ShrinkNice is disabled without gpc code\n";
#endif
						break;
					case SHRINK_LOGICK:
						plane.ShrinkLogick(ExtrudedMaterialWidth, Optimization, DisplayCuttingPlane, ShellCount);
						break;
					}

					// check if this if a layer we should use the alternate infill distance on
					float infillDistance = InfillDistance;
					if (std::find(altInfillLayers.begin(), altInfillLayers.end(), LayerNr) != altInfillLayers.end())
					{
						infillDistance = AltInfillDistance;
					}

					plane.CalcInFill(infill, LayerNr, infillDistance, InfillRotation, InfillRotationPrLayer, DisplayDebuginFill);
				}
				// Make the GCode from the plane and the infill
				plane.MakeGcode(infill, gcode, E, z+printOffsetZ, MinPrintSpeedXY, MaxPrintSpeedXY, MinPrintSpeedZ, MaxPrintSpeedZ, DistanceToReachFullSpeed, extrusionFactor, UseIncrementalEcode, Use3DGcode, EnableAcceleration);
			}
		}
		LayerNr++;
		z+=LayerThickness;
	}

	GcodeTxt.clear();
	if (!EnableAntiooze)
	{
		AntioozeDistance = 0;
	}
	gcode.MakeText(GcodeTxt, GcodeStart, GcodeLayer, GcodeEnd, UseIncrementalEcode, Use3DGcode, AntioozeDistance, AntioozeSpeed);
	gui->ProgressBar->label("Done");
}

Matrix4f ProcessController::GetSTLTransformationMatrix(int object, int file) const
{
	Matrix4f result = rfo.transform3D.transform;
	Vector3f translation = result.getTranslation();
//	result.setTranslation(translation+PrintMargin);

	if(object >= 0)
		result *= rfo.Objects[object].transform3D.transform;
	if(file >= 0)
		result *= rfo.Objects[object].files[file].transform3D.transform;
	return result;
}
#ifndef MIN
	#define MIN(a, b)  (((a) < (b)) ? (a) : (b))
#endif
#ifndef MAX
	#define MAX(a, b)  (((a) > (b)) ? (a) : (b))
#endif

void ProcessController::CalcBoundingBoxAndZoom()
{
	Max = Vector3f(-500,-500,-500);
	Min = Vector3f(500,500,500);

	for(uint o=0;o<rfo.Objects.size();o++)
	{
		for(uint f=0;f<rfo.Objects[o].files.size();f++)
		{
			Matrix4f M = GetSTLTransformationMatrix(o,f);
			Vector3f stlMin = M*rfo.Objects[o].files[f].stl.Min;
			Vector3f stlMax = M*rfo.Objects[o].files[f].stl.Max;
			Min.x = MIN(stlMin.x, Min.x);
			Min.y = MIN(stlMin.y, Min.y);
			Min.z = MIN(stlMin.z, Min.z);
			Max.x = MAX(stlMax.x, Max.x);
			Max.y = MAX(stlMax.y, Max.y);
			Max.z = MAX(stlMax.z, Max.z);
		}
	}

	if((Max-Min).length() > 0)
	{
		// Find zoom
		float L=0;
		if(Max.x - Min.x > L)	L = Max.x - Min.x;
		if(Max.y - Min.y > L)	L = Max.y - Min.y;
		if(Max.z - Min.z > L)	L = Max.z - Min.z;
		if(gui->MVC)
			gui->MVC->zoom= L;
	}
	else
		if(gui->MVC)
			gui->MVC->zoom = 100.0f;

	Center = (Max-Min)*0.5f;
}

void ProcessController::MakeRaft(float &z)
{
	vector<InFillHit> HitsBuffer;

	uint LayerNr = 0;

	float step;


	float size = RaftSize;

	Vector2f raftMin =  Vector2f(Min.x - size+printOffset.x, Min.y - size+printOffset.y);
	Vector2f raftMax =  Vector2f(Max.x + size+printOffset.x, Max.y + size+printOffset.y);

	Vector2f Center = (Vector2f(Max.x + size, Max.y + size)-Vector2f(Min.x + size, Min.y + size))/2+Vector2f(printOffset.x, printOffset.y);

	float Length = sqrtf(2)*(   ((raftMax.x)>(raftMax.y)? (raftMax.x):(raftMax.y))  -  ((raftMin.x)<(raftMin.y)? (raftMin.x):(raftMin.y))  )/2.0f;	// bbox of object

	float E = 0.0f;

	float rot = RaftRotation/180.0f*M_PI;

	while(LayerNr < RaftBaseLayerCount+RaftInterfaceLayerCount)
	{
		rot = (RaftRotation+(float)LayerNr*RaftRotationPrLayer)/180.0f*M_PI;
		Vector2f InfillDirX(cosf(rot), sinf(rot));
		Vector2f InfillDirY(-InfillDirX.y, InfillDirX.x);

		Vector3f LastPosition;
		bool reverseLines = false;

		if(LayerNr < RaftBaseLayerCount)
			step = RaftBaseDistance;
		else
			step = RaftInterfaceDistance;
		Vector2f P1, P2;
		for(float x = -Length ; x < Length ; x+=step)
		{
			P1 = (InfillDirX * Length)+(InfillDirY*x)+ Center;
			P2 = (InfillDirX * -Length)+(InfillDirY*x)+ Center;

			if(reverseLines)
			{
				Vector2f tmp = P1;
				P1 = P2;
				P2 = tmp;
			}

//			glBegin(GL_LINES);
//			glVertex2fv(&P1.x);
//			glVertex2fv(&P2.x);

			// Crop lines to bbox*size
			Vector3f point;
			InFillHit hit;
			HitsBuffer.clear();
			Vector2f P3(raftMin.x, raftMin.y);
			Vector2f P4(raftMin.x, raftMax.y);
//			glVertex2fv(&P3.x);
//			glVertex2fv(&P4.x);
			if(IntersectXY(P1,P2,P3,P4,hit))	//Intersect edges of bbox
				HitsBuffer.push_back(hit);
			P3 = Vector2f(raftMax.x,raftMax.y);
//			glVertex2fv(&P3.x);
//			glVertex2fv(&P4.x);
			if(IntersectXY(P1,P2,P3,P4,hit))
				HitsBuffer.push_back(hit);
			P4 = Vector2f(raftMax.x,raftMin.y);
//			glVertex2fv(&P3.x);
//			glVertex2fv(&P4.x);
			if(IntersectXY(P1,P2,P3,P4,hit))
				HitsBuffer.push_back(hit);
			P3 = Vector2f(raftMin.x,raftMin.y);
//			glVertex2fv(&P3.x);
//			glVertex2fv(&P4.x);
			if(IntersectXY(P1,P2,P3,P4,hit))
				HitsBuffer.push_back(hit);
//			glEnd();

			if(HitsBuffer.size() == 0)	// it can only be 2 or zero
				continue;
			if(HitsBuffer.size() != 2)
				continue;

			std::sort(HitsBuffer.begin(), HitsBuffer.end(), InFillHitCompareFunc);

			P1 = HitsBuffer[0].p;
			P2 = HitsBuffer[1].p;

			float materialRatio;
			if(LayerNr < RaftBaseLayerCount)
				materialRatio = RaftMaterialPrDistanceRatio;		// move or extrude?
			else
				materialRatio = RaftInterfaceMaterialPrDistanceRatio;		// move or extrude?

			MakeAcceleratedGCodeLine(Vector3f(P1.x,P1.y,z), Vector3f(P2.x,P2.y,z), DistanceToReachFullSpeed, materialRatio, gcode, z, MinPrintSpeedXY, MaxPrintSpeedXY, MinPrintSpeedZ, MaxPrintSpeedZ, UseIncrementalEcode, Use3DGcode, E, EnableAcceleration);

			reverseLines = !reverseLines;
		}
		// Set startspeed for Z-move
		Command g;
		g.Code = SETSPEED;
		g.where = Vector3f(P2.x, P2.y, z);
		g.f=MinPrintSpeedZ;
		g.comment = "Move Z";
		g.e = E;
		gcode.commands.push_back(g);
		if(LayerNr < RaftBaseLayerCount)
			z+=RaftBaseThickness*LayerThickness;
		else
			z+=RaftInterfaceThickness*LayerThickness;
		// Move Z
		g.Code = ZMOVE;
		g.where = Vector3f(P2.x, P2.y, z);
		g.f=MinPrintSpeedZ;
		g.comment = "Move Z";
		g.e = E;
		gcode.commands.push_back(g);

		LayerNr++;
	}

	// restore the E state
	Command gotoE;
	gotoE.Code = GOTO;
	gotoE.e = 0;
	gotoE.comment = "Reset E for the remaining print";
	gcode.commands.push_back(gotoE);
}

void ProcessController::OptimizeRotation()
{
//	stl.OptimizeRotation();
	cout << "Reimplementate ProcessController::OptimizeRotation";
}

void ProcessController::RotateObject(Vector3f axis, float a)
{
	Flu_Tree_Browser::Node *node=gui->RFP_Browser->get_selected( 1 );
	// first check files
	for(uint o=0;o<rfo.Objects.size();o++)
	{
		for(uint f=0;f<rfo.Objects[o].files.size();f++)
		{
			if(rfo.Objects[o].files[f].node == node)
			{
				rfo.Objects[o].files[f].stl.RotateObject(axis,a);
				gui->MVC->redraw();
				return;
			}
		}
	}
}

void ProcessController::Draw(Flu_Tree_Browser::Node *selected_node)
{
	printOffset = PrintMargin;
	if(RaftEnable)
		printOffset += Vector3f(RaftSize, RaftSize, 0);

	Vector3f translation = rfo.transform3D.transform.getTranslation();

	printer.Draw(*this);

	// Move objects
	glTranslatef(translation.x+printOffset.x, translation.y+printOffset.y, translation.z+PrintMargin.z);
	glPolygonOffset (0.5f, 0.5f);
	rfo.Draw(*this, 1.0f, selected_node);

	if (DisplayGCode)
	{
		glTranslatef(-(translation.x+printOffset.x), -(translation.y+printOffset.y), -(translation.z+PrintMargin.z));
		gcode.draw(*this);
		glTranslatef(translation.x+printOffset.x, translation.y+printOffset.y, translation.z+PrintMargin.z);
	}
	glPolygonOffset (-0.5f, -0.5f);
	rfo.Draw(*this, PolygonOpasity);

	if(DisplayBBox)
	{
		// Draw bbox
		glColor3f(1,0,0);
		glBegin(GL_LINE_LOOP);
		glVertex3f(Min.x, Min.y, Min.z);
		glVertex3f(Min.x, Max.y, Min.z);
		glVertex3f(Max.x, Max.y, Min.z);
		glVertex3f(Max.x, Min.y, Min.z);
		glEnd();
		glBegin(GL_LINE_LOOP);
		glVertex3f(Min.x, Min.y, Max.z);
		glVertex3f(Min.x, Max.y, Max.z);
		glVertex3f(Max.x, Max.y, Max.z);
		glVertex3f(Max.x, Min.y, Max.z);
		glEnd();
		glBegin(GL_LINES);
		glVertex3f(Min.x, Min.y, Min.z);
		glVertex3f(Min.x, Min.y, Max.z);
		glVertex3f(Min.x, Max.y, Min.z);
		glVertex3f(Min.x, Max.y, Max.z);
		glVertex3f(Max.x, Max.y, Min.z);
		glVertex3f(Max.x, Max.y, Max.z);
		glVertex3f(Max.x, Min.y, Min.z);
		glVertex3f(Max.x, Min.y, Max.z);
		glEnd();
	}

}

void ProcessController::ReadGCode(string filename)
{
	gcode.Read (gui->MVC, filename);
}

void ProcessController::WriteGCode(string &GcodeTxt, const string &GcodeStart, const string &GcodeLayer, const string &GcodeEnd, string filename)
{
	fprintf (stderr, "Unimplemented\n");
}

void ProcessController::SaveXML(string filename)
{
	XML* xml = new XML(filename.c_str());
	XMLElement* e = xml->GetRootElement();
	SaveXML(e);
	if (xml->IntegrityTest())
		xml->Save(); // Saves back to file
	delete xml;
}

namespace {
	void setXMLString (XMLElement *x, const char *name, const char *value)
	{
		x->FindVariableZ ((char *)name, true, (char *)"")->SetValue ((char *)value);
	}

	void setXMLString (XMLElement *x, const std::ostringstream &name, const char *value)
	{
		std::string s = name.str();
		setXMLString (x, s.c_str(), value);
	}

	// Calm our warning problem down ...
	XMLVariable *setVariable (XMLElement *x, const char *variable)
	{
		return x->FindVariableZ ((char *)variable, true, (char *)"");
	}

};

void ProcessController::SaveXML(XMLElement *e)
{
	XMLElement *x = e->FindElementZ("ProcessController", true);

	// Raft parameters
	setVariable (x, "RaftSize")->SetValueFloat(RaftSize);
	setVariable (x, "RaftBaseLayerCount")->SetValueInt(RaftBaseLayerCount);
	setVariable (x, "RaftMaterialPrDistanceRatio")->SetValueFloat(RaftMaterialPrDistanceRatio);
	setVariable (x, "RaftRotation")->SetValueFloat(RaftRotation);
	setVariable (x, "RaftBaseDistance")->SetValueFloat(RaftBaseDistance);
	setVariable (x, "RaftBaseThickness")->SetValueFloat(RaftBaseThickness);
	setVariable (x, "RaftBaseTemperature")->SetValueFloat(RaftBaseTemperature);
	setVariable (x, "RaftInterfaceLayerCount")->SetValueInt(RaftInterfaceLayerCount);
	setVariable (x, "RaftInterfaceMaterialPrDistanceRatio")->SetValueFloat(RaftInterfaceMaterialPrDistanceRatio);
	setVariable (x, "RaftRotationPrLayer")->SetValueFloat(RaftRotationPrLayer);
	setVariable (x, "RaftInterfaceDistance")->SetValueFloat(RaftInterfaceDistance);
	setVariable (x, "RaftInterfaceThickness")->SetValueFloat(RaftInterfaceThickness);
	setVariable (x, "RaftInterfaceTemperature")->SetValueFloat(RaftInterfaceTemperature);

	// GCode parameters
	setVariable (x, "GCodeStartText")->SetValue(GCodeStartText.c_str());
	setVariable (x, "GCodeLayerText")->SetValue(GCodeLayerText.c_str());
	setVariable (x, "GCodeEndText")->SetValue(GCodeEndText.c_str());
        //setVariable (x, "Notes", true,"[Empty]")->SetValue(Notes.c_str()); // overwriting GCodeEndText
	setVariable (x, "m_sPortName")->SetValue(m_sPortName.c_str());
	setVariable (x, "ValidateConnection")->SetValueInt((int)m_bValidateConnection);

	for (int i = 0; i < 20; i++) {
		std::ostringstream os, name;

		os << "CustomButton" << i + 1 << "Text";
		setXMLString (x, os, CustomButtonGcode[i].c_str());

		name << "CustomButton" << i + 1 << "Label";
		setXMLString (x, name, CustomButtonLabel[i].c_str());
	}

	setVariable (x, "m_iSerialSpeed")->SetValueInt(m_iSerialSpeed);

	setVariable (x, "GCodeDrawStart")->SetValueFloat(GCodeDrawStart);
	setVariable (x, "GCodeDrawEnd")->SetValueFloat(GCodeDrawEnd);
	setVariable (x, "ShellOnly")->SetValueInt((int)ShellOnly);
	setVariable (x, "ShellCount")->SetValueInt(ShellCount);

	// Printer parameters
	setVariable (x, "MinPrintSpeedXY")->SetValueFloat(MinPrintSpeedXY);
	setVariable (x, "MaxPrintSpeedXY")->SetValueFloat(MaxPrintSpeedXY);
	setVariable (x, "MinPrintSpeedZ")->SetValueFloat(MinPrintSpeedZ);
	setVariable (x, "MaxPrintSpeedZ")->SetValueFloat(MaxPrintSpeedZ);
//	setVariable (x, "accelerationSteps")->SetValueInt(accelerationSteps);
	setVariable (x, "DistanceToReachFullSpeed")->SetValueFloat(DistanceToReachFullSpeed);
//	setVariable (x, "UseFirmwareAcceleration")->SetValueInt(UseFirmwareAcceleration);
	setVariable (x, "extrusionFactor")->SetValueFloat(extrusionFactor);
	setVariable (x, "EnableAcceleration")->SetValueInt((int)EnableAcceleration);
	setVariable (x, "UseIncrementalEcode")->SetValueInt((int)UseIncrementalEcode);
	setVariable (x, "Use3DGcode")->SetValueInt((int)Use3DGcode);
	setVariable (x, "EnableAntiooze")->SetValueInt((int)EnableAntiooze);
	setVariable (x, "AntioozeDistance")->SetValueFloat(AntioozeDistance);
	setVariable (x, "AntioozeSpeed")->SetValueFloat(AntioozeSpeed);

	setVariable (x, "FileLogginEnabled")->SetValueInt((int)FileLogginEnabled);
	setVariable (x, "TempReadingEnabled")->SetValueInt((int)TempReadingEnabled);
	setVariable (x, "ClearLogfilesWhenPrintStarts")->SetValueInt((int)ClearLogfilesWhenPrintStarts);

	setVariable (x, "m_fVolume.x")->SetValueFloat(m_fVolume.x);
	setVariable (x, "m_fVolume.y")->SetValueFloat(m_fVolume.y);
	setVariable (x, "m_fVolume.z")->SetValueFloat(m_fVolume.z);
	setVariable (x, "PrintMargin.x")->SetValueFloat(PrintMargin.x);
	setVariable (x, "PrintMargin.y")->SetValueFloat(PrintMargin.y);
	setVariable (x, "PrintMargin.z")->SetValueFloat(PrintMargin.z);
	setVariable (x, "extrusionFactor")->SetValueFloat(extrusionFactor);
	setVariable (x, "ExtrudedMaterialWidth")->SetValueFloat(ExtrudedMaterialWidth);
	setVariable (x, "LayerThickness")->SetValueFloat(LayerThickness);

	// CuttingPlane parameters
	setVariable (x, "InfillDistance")->SetValueFloat(InfillDistance);
	setVariable (x, "InfillRotation")->SetValueFloat(InfillRotation);
	setVariable (x, "InfillRotationPrLayer")->SetValueFloat(InfillRotationPrLayer);
	setVariable (x, "AltInfillDistance")->SetValueFloat(AltInfillDistance);
	setVariable (x, "AltInfillLayers")->SetValue(AltInfillLayersText.c_str());
	setVariable (x, "PolygonOpasity")->SetValueFloat(PolygonOpasity);


	// GUI parameters
	setVariable (x, "CuttingPlaneValue")->SetValueFloat(CuttingPlaneValue);
	setVariable (x, "Examine")->SetValueFloat(Examine);

	setVariable (x, "DisplayEndpoints")->SetValueInt((int)DisplayEndpoints);
	setVariable (x, "DisplayNormals")->SetValueInt((int)DisplayNormals);
	setVariable (x, "DisplayWireframe")->SetValueInt((int)DisplayWireframe);
	setVariable (x, "DisplayWireframeShaded")->SetValueInt((int)DisplayWireframeShaded);

	setVariable (x, "DisplayPolygons")->SetValueInt((int)DisplayPolygons);
	setVariable (x, "DisplayAllLayers")->SetValueInt((int)DisplayAllLayers);
	setVariable (x, "DisplayinFill")->SetValueInt((int)DisplayinFill);
	setVariable (x, "DisplayDebuginFill")->SetValueInt((int)DisplayDebuginFill);
	setVariable (x, "DisplayDebug")->SetValueInt((int)DisplayDebug);
	setVariable (x, "DisplayCuttingPlane")->SetValueInt((int)DisplayCuttingPlane);
	setVariable (x, "DrawVertexNumbers")->SetValueInt((int)DrawVertexNumbers);
	setVariable (x, "DrawLineNumbers")->SetValueInt((int)DrawLineNumbers);

	setVariable (x, "PolygonVal")->SetValueFloat(PolygonVal);
	setVariable (x, "PolygonSat")->SetValueFloat(PolygonSat);
	setVariable (x, "PolygonHue")->SetValueFloat(PolygonHue);
	setVariable (x, "WireframeVal")->SetValueFloat(WireframeVal);
	setVariable (x, "WireframeSat")->SetValueFloat(WireframeSat);
	setVariable (x, "WireframeHue")->SetValueFloat(WireframeHue);
	setVariable (x, "NormalsSat")->SetValueFloat(NormalsSat);
	setVariable (x, "NormalsVal")->SetValueFloat(NormalsVal);
	setVariable (x, "NormalsHue")->SetValueFloat(NormalsHue);
	setVariable (x, "EndpointsSat")->SetValueFloat(EndpointsSat);
	setVariable (x, "EndpointsVal")->SetValueFloat(EndpointsVal);
	setVariable (x, "EndpointsHue")->SetValueFloat(EndpointsHue);
	setVariable (x, "GCodeExtrudeHue")->SetValueFloat(GCodeExtrudeHue);
	setVariable (x, "GCodeExtrudeSat")->SetValueFloat(GCodeExtrudeSat);
	setVariable (x, "GCodeExtrudeVal")->SetValueFloat(GCodeExtrudeVal);
	setVariable (x, "GCodeMoveHue")->SetValueFloat(GCodeMoveHue);
	setVariable (x, "GCodeMoveSat")->SetValueFloat(GCodeMoveSat);
	setVariable (x, "GCodeMoveVal")->SetValueFloat(GCodeMoveVal);
	setVariable (x, "Highlight")->SetValueFloat(Highlight);
	setVariable (x, "NormalsLength")->SetValueFloat(NormalsLength);
	setVariable (x, "EndPointSize")->SetValueFloat(EndPointSize);
	setVariable (x, "TempUpdateSpeed")->SetValueFloat(TempUpdateSpeed);

	setVariable (x, "DisplayGCode")->SetValueFloat(DisplayGCode);
	setVariable (x, "LuminanceShowsSpeed")->SetValueFloat(LuminanceShowsSpeed);

	setVariable (x, "RaftEnable")->SetValueFloat(RaftEnable);
	setVariable (x, "ApronEnable")->SetValueFloat(ApronEnable);
	setVariable (x, "ApronPreview")->SetValueFloat(ApronPreview);
	setVariable (x, "ApronSize")->SetValueFloat(ApronSize);
	setVariable (x, "ApronHeight")->SetValueFloat(ApronHeight);
	setVariable (x, "ApronCoverageX")->SetValueFloat(ApronCoverageX);
	setVariable (x, "ApronCoverageY")->SetValueFloat(ApronCoverageY);
	setVariable (x, "ApronDistanceToObject")->SetValueFloat(ApronDistanceToObject);
	setVariable (x, "ApronInfillDistance")->SetValueFloat(ApronInfillDistance);

	setVariable (x, "ShrinkFast")->SetValueInt(m_ShrinkQuality == SHRINK_FAST);
	setVariable (x, "ShrinkNice")->SetValueInt(m_ShrinkQuality == SHRINK_NICE);
	setVariable (x, "ShrinkLogick")->SetValueInt(m_ShrinkQuality == SHRINK_LOGICK);
	setVariable (x, "Optimization")->SetValueFloat(Optimization);
	setVariable (x, "ReceivingBufferSize")->SetValueInt(ReceivingBufferSize);

	setVariable (x, "ApronInfillDistance")->SetValueFloat(ApronInfillDistance);

	setVariable (x, "STLPath")->SetValue(STLPath.c_str());
	setVariable (x, "RFOPath")->SetValue(RFOPath.c_str());
	setVariable (x, "GCodePath")->SetValue(GCodePath.c_str());
	setVariable (x, "SettingsPath")->SetValue(SettingsPath.c_str());
}

namespace {
	std::string getXMLString (XMLElement *x, const char *name, const char *default_value)
	{
		XMLVariable *v = x->FindVariableZ ((char *)name, true, (char *)default_value);
		if (v) {
			int size = v->MemoryUsage() + 1024;
			char *buffer = new char[size];
			buffer[0] = '\0';
			v->GetValue (buffer);
			std::string retval(buffer);
			delete[] buffer;
			return retval;
		} else
			return std::string();
	}

	std::string getXMLString (XMLElement *x, const std::ostringstream &name,
				  const std::ostringstream &default_value)
	{
		std::string s = name.str();
		std::string d = default_value.str();
		return getXMLString (x, s.c_str(), d.c_str());
	}

	// Calm our warning problem down ...
	XMLVariable *getVariable (XMLElement *x, const char *variable, const char *default_value)
	{
		return x->FindVariableZ ((char *)variable, true, (char *)default_value);
	}
};

void ProcessController::LoadXML(XMLElement *e)
{
	XMLElement *x = e->FindElementZ("ProcessController", true);

	XMLVariable* y;

	y = getVariable (x, "RaftSize", "1.33");
	if(y)	RaftSize = y->GetValueFloat();
	y = getVariable (x, "RaftBaseLayerCount", "1");
	if(y)	RaftBaseLayerCount = y->GetValueInt();
	y = getVariable (x, "RaftMaterialPrDistanceRatio", "1.7");
	if(y)	RaftMaterialPrDistanceRatio = y->GetValueFloat();
	y = getVariable (x, "RaftRotation", "90");
	if(y)	RaftRotation = y->GetValueFloat();
	y = getVariable (x, "RaftBaseDistance", "2.5");
	if(y)	RaftBaseDistance = y->GetValueFloat();
	y = getVariable (x, "RaftBaseThickness", "1");
	if(y)	RaftBaseThickness = y->GetValueFloat();
	y = getVariable (x, "RaftBaseTemperature", "190");
	if(y)	RaftBaseTemperature = y->GetValueFloat();
	y = getVariable (x, "RaftInterfaceLayerCount", "2");
	if(y)	RaftInterfaceLayerCount = y->GetValueInt();
	y = getVariable (x, "RaftInterfaceMaterialPrDistanceRatio", "1");
	if(y)	RaftInterfaceMaterialPrDistanceRatio = y->GetValueFloat();
	y = getVariable (x, "RaftRotationPrLayer", "90");
	if(y)	RaftRotationPrLayer = y->GetValueFloat();
	y = getVariable (x, "RaftInterfaceDistance", "2");
	if(y)	RaftInterfaceDistance = y->GetValueFloat();
	y = getVariable (x, "RaftInterfaceThickness", "1");
	if(y)	RaftInterfaceThickness = y->GetValueFloat();
	y = getVariable (x, "RaftInterfaceTemperature", "190");
	if(y)	RaftInterfaceTemperature = y->GetValueFloat();

	// GCode parameters

	GCodeStartText = getXMLString (x, "GCodeStartText",
				       "; GCode generated by RepSnapper by Kulitorum\n"
				       "G21                              ;metric is good!\n"
				       "G90                              ;absolute positioning\n"
				       "T0                                 ;select new extruder\n"
				       "G28                               ;go home\n"
				       "G92 E0                          ;set extruder home\n"
				       "M104 S73.0                   ;set temperature\n"
				       "G1 X20 Y20 F500            ;Move away from 0.0, so we use the same reset (in the layer code) for each layer\n"
				       "\n");

	for (int i = 0; i < 20; i++) {
		std::ostringstream os, empty, name, label;

		os << "CustomButton" << i + 1 << "Text";
		CustomButtonGcode[i] = getXMLString (x, os, empty);

		name << "CustomButton" << i + 1 << "Label";
		label << "Custom button" << i + 1;
		CustomButtonLabel[i] = getXMLString (x, name, label);
	}

	if (gui && gui->MVC )
		gui->MVC->RefreshCustomButtonLabels();

	GCodeLayerText = getXMLString (x, "GCodeLayerText", "");
	GCodeEndText = getXMLString (x, "GCodeEndText",
				     "G1 X0 Y0 F2000.0       ;feed for start of next move\n"
				     "M104 S0.0              ;Heater off\n");

	m_sPortName = getXMLString (x, "m_sPortName", "");
	if ( gui && gui->MVC )
		  m_sPortName = gui->MVC->ValidateComPort( m_sPortName );

	y = getVariable (x, "ValidateConnection", "1");
	if(y)	m_bValidateConnection = (bool)y->GetValueInt();


	STLPath = getXMLString (x, "STLPath", "");
	RFOPath = getXMLString (x, "RFOPath", "");
	GCodePath = getXMLString (x, "GCodePath", "");
	SettingsPath = getXMLString (x, "SettingsPath", "");

	y = getVariable (x, "m_iSerialSpeed", "19200");
	if(y)	m_iSerialSpeed = y->GetValueInt();

	y = getVariable (x, "GCodeDrawStart", "0");
	if(y)	GCodeDrawStart = y->GetValueFloat();
	y = getVariable (x, "GCodeDrawEnd", "1");
	if(y)	GCodeDrawEnd = y->GetValueFloat();
	y = getVariable (x, "MinPrintSpeedXY", "2300");
	if(y)	MinPrintSpeedXY = y->GetValueFloat();
	y = getVariable (x, "MaxPrintSpeedXY", "2300");
	if(y)	MaxPrintSpeedXY = y->GetValueFloat();
	y = getVariable (x, "MinPrintSpeedZ", "70");
	if(y)	MinPrintSpeedZ = y->GetValueFloat();
	y = getVariable (x, "MaxPrintSpeedZ", "70");
	if(y)	MaxPrintSpeedZ = y->GetValueFloat();
	
	y = getVariable (x, "EnableAntiooze", "0");
	if(y)	EnableAntiooze = (bool)y->GetValueInt();
	y = getVariable (x, "AntioozeDistance", "4.5");
	if(y)	AntioozeDistance = y->GetValueFloat();
	y = getVariable (x, "AntioozeSpeed", "1000");
	if(y)	AntioozeSpeed = y->GetValueFloat();

	y = getVariable (x, "DistanceToReachFullSpeed", "3");
	if(y)	DistanceToReachFullSpeed = y->GetValueFloat();
	y = getVariable (x, "extrusionFactor", "1");
	if(y)	extrusionFactor = y->GetValueFloat();

	// Printer parameters
	y = getVariable (x, "m_fVolume.x", "200");
	if(y)	m_fVolume.x = y->GetValueFloat();
	y = getVariable (x, "m_fVolume.y", "200");
	if(y)	m_fVolume.y = y->GetValueFloat();
	y = getVariable (x, "m_fVolume.z", "140");
	if(y)	m_fVolume.z = y->GetValueFloat();
	y = getVariable (x, "PrintMargin.x", "10");
	if(y)	PrintMargin.x = y->GetValueFloat();
	y = getVariable (x, "PrintMargin.y", "10");
	if(y)	PrintMargin.y = y->GetValueFloat();
	y = getVariable (x, "PrintMargin.z", "0");
	if(y)	PrintMargin.z = y->GetValueFloat();
	y = getVariable (x, "ExtrudedMaterialWidth", "0.7");
	if(y)	ExtrudedMaterialWidth = y->GetValueFloat();


	// STL parameters
	y = getVariable (x, "LayerThickness", "0.4");
	if(y)	LayerThickness = y->GetValueFloat();
	y = getVariable (x, "CuttingPlaneValue", "0.5");
	if(y)	CuttingPlaneValue = y->GetValueFloat();

	// CuttingPlane
	y = getVariable (x, "InfillDistance", "2");
	if(y)	InfillDistance = y->GetValueFloat();
	y = getVariable (x, "InfillRotation", "45");
	if(y)	InfillRotation = y->GetValueFloat();
	y = getVariable (x, "InfillRotationPrLayer", "90");
	if(y)	InfillRotationPrLayer = y->GetValueFloat();
	y = getVariable (x, "AltInfillDistance", "2");
	if(y) AltInfillDistance = y->GetValueFloat();

	AltInfillLayersText = getXMLString (x, "AltInfillLayers", "");

	y = getVariable (x, "Optimization", "0.05");
	if(y)	Optimization = y->GetValueFloat();
	y = getVariable (x, "ReceivingBufferSize", "4");
	if(y)	ReceivingBufferSize = y->GetValueInt();
	y = getVariable (x, "ShellOnly", "0");
	if(y)	ShellOnly = y->GetValueFloat();
	y = getVariable (x, "ShellCount", "1");
	if(y)	ShellCount = y->GetValueFloat();
	y = getVariable (x, "EnableAcceleration", "0");
	if(y)	EnableAcceleration = (bool)y->GetValueInt();
	y = getVariable (x, "UseIncrementalEcode", "1");
	if(y)	UseIncrementalEcode= (bool)y->GetValueInt();
	y = getVariable (x, "Use3DGcode", "0");
	if(y)	Use3DGcode= (bool)y->GetValueInt();

	y = getVariable (x, "FileLogginEnabled", "1");
	if(y)	FileLogginEnabled= (bool)y->GetValueInt();
	y = getVariable (x, "TempReadingEnabled", "1");
	if(y)	TempReadingEnabled= (bool)y->GetValueInt();
	y = getVariable (x, "ClearLogfilesWhenPrintStarts", "1");
	if(y)	ClearLogfilesWhenPrintStarts= (bool)y->GetValueInt();

	y = getVariable (x, "ValidateConnection", "1");
	if(y)	m_bValidateConnection = (bool)y->GetValueInt();

	// GUI... ?
	y = getVariable (x, "DisplayEndpoints", "0");
	if(y)	DisplayEndpoints = (bool)y->GetValueInt();
	y = getVariable (x, "DisplayNormals", "0");
	if(y)	DisplayNormals = (bool)y->GetValueInt();
	y = getVariable (x, "DisplayWireframe", "0");
	if(y)	DisplayWireframe = (bool)y->GetValueInt();
	y = getVariable (x, "DisplayWireframeShaded", "1");
	if(y)	DisplayWireframeShaded = (bool)y->GetValueInt();
	y = getVariable (x, "DisplayPolygons", "1");
	if(y)	DisplayPolygons = (bool)y->GetValueInt();
	y = getVariable (x, "DisplayAllLayers", "0");
	if(y)	DisplayAllLayers = (bool)y->GetValueInt();
	y = getVariable (x, "DisplayinFill", "1");
	if(y)	DisplayinFill = (bool)y->GetValueInt();
	y = getVariable (x, "DisplayDebuginFill", "0");
	if(y)	DisplayDebuginFill = (bool)y->GetValueInt();
	y = getVariable (x, "DisplayDebug", "0");
	if(y)	DisplayDebug = (bool)y->GetValueInt();
	y = getVariable (x, "DisplayCuttingPlane", "0");
	if(y)	DisplayCuttingPlane =(bool)y->GetValueInt();
	y = getVariable (x, "DrawVertexNumbers", "0");
	if(y)	DrawVertexNumbers = (bool)y->GetValueInt();
	y = getVariable (x, "DrawLineNumbers", "0");
	if(y)	DrawLineNumbers = (bool)y->GetValueInt();

	y = getVariable (x, "PolygonVal", "0.5");
	if(y)	PolygonVal = y->GetValueFloat();
	y = getVariable (x, "PolygonSat", "1");
	if(y)	PolygonSat = y->GetValueFloat();
	y = getVariable (x, "PolygonHue", "0.54");
	if(y)	PolygonHue = y->GetValueFloat();
	y = getVariable (x, "WireframeVal", "1");
	if(y)	WireframeVal = y->GetValueFloat();
	y = getVariable (x, "WireframeSat", "1");
	if(y)	WireframeSat = y->GetValueFloat();
	y = getVariable (x, "WireframeHue", "0.08");
	if(y)	WireframeHue = y->GetValueFloat();
	y = getVariable (x, "NormalsSat", "1");
	if(y)	NormalsSat = y->GetValueFloat();
	y = getVariable (x, "NormalsVal", "1");
	if(y)	NormalsVal = y->GetValueFloat();
	y = getVariable (x, "NormalsHue", "0.23");
	if(y)	NormalsHue = y->GetValueFloat();
	y = getVariable (x, "EndpointsSat", "1");
	if(y)	EndpointsSat = y->GetValueFloat();
	y = getVariable (x, "EndpointsVal", "1");
	if(y)	EndpointsVal = y->GetValueFloat();
	y = getVariable (x, "EndpointsHue", "0.45");
	if(y)	EndpointsHue = y->GetValueFloat();
	y = getVariable (x, "GCodeExtrudeHue", "0.18");
	if(y)	GCodeExtrudeHue = y->GetValueFloat();
	y = getVariable (x, "GCodeExtrudeSat", "1");
	if(y)	GCodeExtrudeSat = y->GetValueFloat();
	y = getVariable (x, "GCodeExtrudeVal", "1");
	if(y)	GCodeExtrudeVal = y->GetValueFloat();
	y = getVariable (x, "GCodeMoveHue", "1");
	if(y)	GCodeMoveHue = y->GetValueFloat();
	y = getVariable (x, "GCodeMoveSat", "0.95");
	if(y)	GCodeMoveSat = y->GetValueFloat();
	y = getVariable (x, "GCodeMoveVal", "1");
	if(y)	GCodeMoveVal = y->GetValueFloat();
	y = getVariable (x, "Highlight", "0.7");
	if(y)	Highlight = y->GetValueFloat();
	y = getVariable (x, "NormalsLength", "10");
	if(y)	NormalsLength = y->GetValueFloat();
	y = getVariable (x, "EndPointSize", "8");
	if(y)	EndPointSize = y->GetValueFloat();

	y = getVariable (x, "TempUpdateSpeed", "3");
	if(y)	TempUpdateSpeed = y->GetValueFloat();


	y = getVariable (x, "DisplayGCode", "1");
	if(y)	DisplayGCode = (bool)y->GetValueInt();
	y = getVariable (x, "LuminanceShowsSpeed", "0");
	if(y)	LuminanceShowsSpeed = (bool)y->GetValueInt();

	y = getVariable (x, "RaftEnable", "0");
	if(y)	RaftEnable = (bool)y->GetValueInt();
	y = getVariable (x, "ApronEnable", "0");
	if(y)	ApronEnable = (bool)y->GetValueInt();
	y = getVariable (x, "ApronPreview", "1");
	if(y)	ApronPreview = (bool)y->GetValueInt();
	y = getVariable (x, "ApronSize", "1.33");
	if(y)	ApronSize = (bool)y->GetValueFloat();
	y = getVariable (x, "ApronHeight", "7");
	if(y)	ApronHeight = (bool)y->GetValueFloat();
	y = getVariable (x, "ApronCoverageX", "60");
	if(y)	ApronCoverageX = (bool)y->GetValueFloat();
	y = getVariable (x, "ApronCoverageY", "60");
	if(y)	ApronCoverageY = (bool)y->GetValueFloat();
	y = getVariable (x, "ApronDistanceToObject", "0.5");
	if(y)	ApronDistanceToObject = (bool)y->GetValueFloat();
	y = getVariable (x, "ApronInfillDistance", "2");
	if(y)	ApronInfillDistance = (bool)y->GetValueFloat();

	y = getVariable (x, "ShrinkFast", "1"); // "1" makes this the default, must be on top to allow the others to overwrite if set.
	if(y && (bool)y->GetValueInt())	m_ShrinkQuality = SHRINK_FAST;
	y = getVariable (x, "ShrinkNice", "0");
	if(y && (bool)y->GetValueInt())	m_ShrinkQuality = SHRINK_NICE;
	y = getVariable (x, "ShrinkLogick", "0");
	if(y && (bool)y->GetValueInt())	m_ShrinkQuality = SHRINK_LOGICK;
}

void ProcessController::LoadXML(string filename)
{
	XML* xml = new XML (filename.c_str());
	XMLElement* e = xml->GetRootElement ();
	LoadXML (e);
}

void ProcessController::LoadXML()
{
	LoadXML (m_Filename + ".xml");
}

void ProcessController::SaveXML()
{
	string filename = m_Filename + ".xml";

	XML* xml = new XML (filename.c_str());
	if (xml) {
		XMLElement* e = xml->GetRootElement();
		SaveXML (e);
		if (xml->IntegrityTest())
			xml->Save(); // Saves back to file
		delete xml;
	}
}
/*
void ProcessController::BindLua(lua_State *myLuaState)
{
#ifdef ENABLE_LUA
	// Export our class with LuaBind
	luabind::module(myLuaState)
		[
			//		luabind::class_<NumberPrinter>("NumberPrinter")
			//		.def(luabind::constructor<int>())
			//		.def("print", &NumberPrinter::print)

//			luabind::def ("OptimizeRotation", &stl.OptimizeRotation());

			// Process functions
			class_<ProcessController>("ProcessController")
			.def ("setFilename", &ProcessController::SetFilename)
			.def( "ReadStl",  &ProcessController::ReadStl)

//			.def ("GCode", ProcessController::GCodeResult)


			// Start, layer, end GCode
			.def ("GCodeStartText", GCodeStartText)
			.def ("GCodeLayerText", GCodeLayerText)
			.def ("GCodeEndText", GCodeEndText)

			//--------------Models-------------------
			.def ("printer", printer)
			.def ("m_sPortName", m_sPortName)
			.def ("stl", stl)
			.def ("gcode", gcode)
			.def ("GcodeTxt", GcodeTxt)

			// Raft
			.def ("RaftSize", RaftSize)
			.def ("RaftBaseLayerCount", RaftBaseLayerCount)
			.def ("RaftMaterialPrDistanceRatio", RaftMaterialPrDistanceRatio)
			.def ("RaftRotation", RaftRotation)
			.def ("RaftBaseDistance", RaftBaseDistance)
			.def ("RaftBaseThickness", RaftBaseThickness)
			.def ("RaftBaseTemperature", RaftBaseTemperature)
			.def ("RaftInterfaceLayerCount", RaftInterfaceLayerCount)
			.def ("RaftInterfaceMaterialPrDistanceRatio", RaftInterfaceMaterialPrDistanceRatio)
			.def ("RaftRotationPrLayer", RaftRotationPrLayer)
			.def ("RaftInterfaceDistance", RaftInterfaceDistance)
			.def ("RaftInterfaceThickness", RaftInterfaceThickness)
			.def ("RaftInterfaceTemperature", RaftInterfaceTemperature)

			// GCode
			.def ("GCodeDrawStart", GCodeDrawStart)
			.def ("GCodeDrawEnd", GCodeDrawEnd)
			.def ("MinPrintSpeedXY", MinPrintSpeedXY)
			.def ("MaxPrintSpeedXY", MaxPrintSpeedXY)
			.def ("MinPrintSpeedZ", MinPrintSpeedZ)
			.def ("MaxPrintSpeedZ", MaxPrintSpeedZ)

			.def ("accelerationSteps", accelerationSteps)
			.def ("DistanceToReachFullSpeed", DistanceToReachFullSpeed)
			.def ("extrusionFactor", extrusionFactor)
//			.def ("UseFirmwareAcceleration", UseFirmwareAcceleration)

			// Printer
			.def ("m_fVolume", m_fVolume)
			.def ("PrintMargin", PrintMargin)
			.def ("ExtrudedMaterialWidth", ExtrudedMaterialWidth)
			.def ("UseIncrementalEcode", UseIncrementalEcode)

			// STL
			.def ("LayerThickness", LayerThickness)
			.def ("CuttingPlaneValue", CuttingPlaneValue)

			// CuttingPlane
			.def ("InfillDistance", InfillDistance)
			.def ("InfillRotation", InfillRotation)
			.def ("InfillRotationPrLayer", InfillRotationPrLayer)
			.def ("Optimization", Optimization)
			.def ("Examine", Examine)

			.def ("ShellOnly", ShellOnly)
			.def ("ShellCount", ShellCount)

			.def ("EnableAcceleration", EnableAcceleration)

			// GUI... ?
			.def ("DisplayEndpoints", DisplayEndpoints)
			.def ("DisplayNormals", DisplayNormals)
			.def ("DisplayBBox", DisplayBBox)
			.def ("DisplayWireframe", DisplayWireframe)
			.def ("DisplayWireframeShaded", DisplayWireframeShaded)
			.def ("DisplayPolygons", DisplayPolygons)
			.def ("DisplayAllLayers", DisplayAllLayers)
			.def ("DisplayinFill", DisplayinFill)
			.def ("DisplayDebuginFill", DisplayDebuginFill)
			.def ("DisplayDebug", DisplayDebug)
			.def ("DisplayCuttingPlane", DisplayCuttingPlane)
			.def ("DrawVertexNumbers", DrawVertexNumbers)
			.def ("DrawLineNumbers", DrawLineNumbers)
			.def ("Notes", Notes)

			// Rendering
			.def ("PolygonVal", PolygonVal)
			.def ("PolygonSat", PolygonSat)
			.def ("PolygonHue", PolygonHue)
			.def ("WireframeVal", WireframeVal)
			.def ("WireframeSat", WireframeSat)
			.def ("WireframeHue", WireframeHue)
			.def ("NormalsSat", NormalsSat)
			.def ("NormalsVal", NormalsVal)
			.def ("NormalsHue", NormalsHue)
			.def ("EndpointsSat", EndpointsSat)
			.def ("EndpointsVal", EndpointsVal)
			.def ("EndpointsHue", EndpointsHue)
			.def ("GCodeExtrudeHue", GCodeExtrudeHue)
			.def ("GCodeExtrudeSat", GCodeExtrudeSat)
			.def ("GCodeExtrudeVal", GCodeExtrudeVal)
			.def ("GCodeMoveHue", GCodeMoveHue)
			.def ("GCodeMoveSat", GCodeMoveSat)
			.def ("GCodeMoveVal", GCodeMoveVal)
			.def ("Highlight", Highlight)
			.def ("NormalsLength", NormalsLength)
			.def ("EndPointSize", EndPointSize)

			.def ("LuminanceShowsSpeed", LuminanceShowsSpeed)
			.def ("DisplayGCode", DisplayGCode)

			.def ("ApronEnable", ApronEnable)
			.def ("ApronPreview", ApronPreview)
			.def ("RaftEnable", RaftEnable)
			.def ("ApronSize", ApronSize)
			.def ("ApronHeight", ApronHeight)
			.def ("ApronCoverageX", ApronCoverageX)
			.def ("ApronCoverageY", ApronCoverageY)
			.def ("ApronDistanceToObject", ApronDistanceToObject)
			.def ("ApronInfillDistance", ApronInfillDistance)
		];
#endif // ENABLE_LUA
}
*/


void ProcessController::GetAltInfillLayers(vector<int>& layers, uint layerCount) const
{
	vector<string> numstrs;
	boost::algorithm::split(numstrs, AltInfillLayersText, boost::is_any_of(","));

	vector<string>::iterator numstr_i;
	for (numstr_i = numstrs.begin(); numstr_i != numstrs.end(); numstr_i++)
	{
		int num;
		int retval = sscanf((*numstr_i).c_str(), "%d", &num);
		if (retval == 1)
		{
			if (num < 0)
			{
				num += layerCount;
			}

			layers.push_back(num);
		}
	}
}

void ProcessController::SaveBuffers()
{
	Fl_Text_Buffer* buffer = gui->GCodeStart->buffer();
	char* pText = buffer->text();
	GCodeStartText = string(pText);
	free(pText);

	buffer = gui->GCodeLayer->buffer();
	pText = buffer->text();
	GCodeLayerText = string(pText);
	free(pText);

	buffer = gui->GCodeEnd->buffer();
	pText = buffer->text();
	GCodeEndText = string(pText);
	free(pText);

	AltInfillLayersText = string(gui->AltInfillLayersInput->value());
}

void ProcessController::SaveSettings()
{
	SaveBuffers();
	SaveXML();
}

void ProcessController::SaveSettingsAs(string path)
{
	SaveBuffers();
	SaveXML(path);
}
