/*
    This file is a part of the RepSnapper project.
    Copyright (C) 2010  Kulitorum

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
#include "stdafx.h"
#include "modelviewcontroller.h"
#include "processcontroller.h"
#include <boost/algorithm/string.hpp>
#include <libconfig.h++>
using namespace libconfig;
using namespace std;

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

void ProcessController::LoadConfig()
{
	LoadConfig(m_Filename + ".conf");
}

void ProcessController::LoadConfig(string filename)
{
	Config cfg;
	try
	{
		cfg.readFile(filename.c_str());
	}
	catch (exception &e)
	{
		cout << "Could not load settings file: " << e.what() << endl;
		cout << "Settings to defaults." << endl;
	}
	if (not cfg.lookupValue("RaftSize",RaftSize))
		RaftSize = 1.33f;
	if (not cfg.lookupValue("RaftBaseLayerCount",RaftBaseLayerCount))
		RaftBaseLayerCount = 1;
	if (not cfg.lookupValue("RaftMaterialPrDistanceRatio",RaftMaterialPrDistanceRatio))
		RaftMaterialPrDistanceRatio = 1.7f;
	if (not cfg.lookupValue("RaftRotation",RaftRotation))
		RaftRotation = 90;
	if (not cfg.lookupValue("RaftBaseDistance",RaftBaseDistance))
		RaftBaseDistance = 2.5;
	if (not cfg.lookupValue("RaftBaseThickness",RaftBaseThickness))
		RaftBaseThickness = 1;
	if (not cfg.lookupValue("RaftBaseTemperature",RaftBaseTemperature))
		RaftBaseThickness = 190;
	if (not cfg.lookupValue("RaftInterfaceLayerCount",RaftInterfaceLayerCount))
		RaftInterfaceLayerCount = 2;
	if (not cfg.lookupValue("RaftInterfaceMaterialPrDistanceRatio",RaftInterfaceMaterialPrDistanceRatio))
		RaftInterfaceMaterialPrDistanceRatio = 1;
	if (not cfg.lookupValue("RaftRotationPrLayer",RaftRotationPrLayer))
		RaftRotationPrLayer = 90;
	if (not cfg.lookupValue("RaftInterfaceDistance",RaftInterfaceDistance))
		RaftInterfaceDistance = 2;
	if (not cfg.lookupValue("RaftInterfaceThickness",RaftInterfaceThickness))
		RaftInterfaceThickness = 1;
	if (not cfg.lookupValue("RaftInterfaceTemperature",RaftInterfaceTemperature))
		RaftInterfaceTemperature = 1;

		// GCode parameters

	if (not cfg.lookupValue("GCodeStartText",GCodeStartText))
		GCodeStartText = "; GCode generated by RepSnapper by Kulitorum\n"
					   "G21                              ;metric is good!\n"
					   "G90                              ;absolute positioning\n"
					   "T0                                 ;select new extruder\n"
					   "G28                               ;go home\n"
					   "G92 E0                          ;set extruder home\n"
					   "M104 S200.0                   ;set temperature to 200.0\n"
					   "G1 X20 Y20 F500            ;Move away from 0.0, so we use the same reset (in the layer code) for each layer\n\n";
	for (int i = 0; i < 20; i++) {
		std::ostringstream cbg, cbl, cbld;
			cbg << "CustomButtonGcode.[" << i << "]";
		if (not cfg.lookupValue(cbg.str(),CustomButtonGcode[i]))
			CustomButtonGcode[i] = "";
			
		cbl << "CustomButtonLabel.[" << i << "]";
		if (not cfg.lookupValue(cbl.str(),CustomButtonLabel[i]))
		{
			cbld << "Custom button" << (i + 1);
			CustomButtonLabel[i] = cbld.str();
		}
	}

	if (gui && gui->MVC)
		gui->MVC->RefreshCustomButtonLabels();
	if (not cfg.lookupValue("GCodeLayerText",GCodeLayerText))
		GCodeLayerText = "";

	if (not cfg.lookupValue("GCodeEndText",GCodeEndText))
		GCodeEndText = "G1 X0 Y0 F2000.0       ;feed for start of next move\n"
					   "M104 S0.0              ;Heater off\n";

	if (not cfg.lookupValue("msPortName",m_sPortName))
		m_sPortName = "";
	if (gui && gui->MVC)
		  m_sPortName = gui->MVC->ValidateComPort(m_sPortName);

	if (not cfg.lookupValue("ValidateConnection",m_bValidateConnection))
		m_bValidateConnection = true;


	if (not cfg.lookupValue("STLPath",STLPath))
		STLPath = "";
	if (not cfg.lookupValue("RFOPath",RFOPath))
		RFOPath = "";
	if (not cfg.lookupValue("GCodePath",GCodePath))
		GCodePath = "";
	if (not cfg.lookupValue("SettingsPath",SettingsPath))
		SettingsPath = "";

	if (not cfg.lookupValue("miSerialSpeed",m_iSerialSpeed))
		m_iSerialSpeed = 19200;

	if (not cfg.lookupValue("GCodeDrawStart",GCodeDrawStart))
		GCodeDrawStart = 0;
	if (not cfg.lookupValue("GCodeDrawEnd",GCodeDrawEnd))
		GCodeDrawEnd = 1;
	if (not cfg.lookupValue("MinPrintSpeedXY",MinPrintSpeedXY))
		MinPrintSpeedXY = 2300;
	if (not cfg.lookupValue("MaxPrintSpeedXY",MaxPrintSpeedXY))
		MaxPrintSpeedXY = 2300;
	if (not cfg.lookupValue("MinPrintSpeedZ",MinPrintSpeedZ))
		MinPrintSpeedZ = 70;
	if (not cfg.lookupValue("MaxPrintSpeedZ",MaxPrintSpeedZ))
		MaxPrintSpeedZ = 70;
	if (not cfg.lookupValue("EnableAntiooze",EnableAntiooze))
		EnableAntiooze = false;
	if (not cfg.lookupValue("AntioozeDistance",AntioozeDistance))
		AntioozeDistance = 4.5;
	if (not cfg.lookupValue("AntioozeSpeed",AntioozeSpeed))
		AntioozeSpeed = 1000;

	if (not cfg.lookupValue("DistanceToReachFullSpeed",DistanceToReachFullSpeed))
		DistanceToReachFullSpeed = 3;
	if (not cfg.lookupValue("extrusionFactor",extrusionFactor))
		extrusionFactor = 1;

	// Printer parameters
	if (not cfg.lookupValue("mfVolumeX",m_fVolume.x))
		m_fVolume.x = 200;
	if (not cfg.lookupValue("mfVolumeY",m_fVolume.y))
		m_fVolume.y = 200;
	if (not cfg.lookupValue("mfVolumeZ",m_fVolume.z))
		m_fVolume.z = 140;
	if (not cfg.lookupValue("PrintMarginX",PrintMargin.x))
		PrintMargin.x = 10;
	if (not cfg.lookupValue("PrintMarginY",PrintMargin.y))
		PrintMargin.y = 10;
	if (not cfg.lookupValue("PrintMarginZ",PrintMargin.z))
		PrintMargin.z = 0;
	if (not cfg.lookupValue("ExtrudedMaterialWidth",ExtrudedMaterialWidth))
		ExtrudedMaterialWidth = 0.7;

	// STL parameters
	if (not cfg.lookupValue("LayerThickness",LayerThickness))
		LayerThickness = 0.4;
	if (not cfg.lookupValue("CuttingPlaneValue",CuttingPlaneValue))
		CuttingPlaneValue = 0.5;
		// CuttingPlane
	if (not cfg.lookupValue("InfillDistance",InfillDistance))
			InfillDistance = 2;
	if (not cfg.lookupValue("InfillRotation",InfillRotation))
		InfillRotation = 45;
	if (not cfg.lookupValue("InfillRotationPrLayer",InfillRotationPrLayer))
		InfillRotationPrLayer = 90;
	if (not cfg.lookupValue("AltInfillDistance",AltInfillDistance))
		AltInfillDistance = 2;
	if (not cfg.lookupValue("AltInfillLayersText",AltInfillLayersText))
		AltInfillLayersText = "";
	if (not cfg.lookupValue("Optimization",Optimization))
		Optimization = 0.05;
	if (not cfg.lookupValue("ReceivingBufferSize",ReceivingBufferSize))
		ReceivingBufferSize = 4;
	if (not cfg.lookupValue("ShellOnly",ShellOnly))
		ShellOnly = 0;
	if (not cfg.lookupValue("ShellCount",ShellCount))
		ShellCount = 1;
	if (not cfg.lookupValue("EnableAcceleration",EnableAcceleration))
		ShellCount = false;
	if (not cfg.lookupValue("UseIncrementalEcode",UseIncrementalEcode))
		UseIncrementalEcode = true;
	if (not cfg.lookupValue("Use3DGcode",Use3DGcode))
		Use3DGcode = false;

	if (not cfg.lookupValue("FileLogginEnabled",FileLogginEnabled))
		FileLogginEnabled = true;
	if (not cfg.lookupValue("TempReadingEnabled",TempReadingEnabled))
		TempReadingEnabled = true;
	if (not cfg.lookupValue("ClearLogfilesWhenPrintStarts",ClearLogfilesWhenPrintStarts))
		ClearLogfilesWhenPrintStarts = true;

	if (not cfg.lookupValue("ValidateConnection",m_bValidateConnection))
		m_bValidateConnection = true;

		// GUI... ?
	if (not cfg.lookupValue("DisplayEndpoints",DisplayEndpoints))
		DisplayEndpoints = false;
	if (not cfg.lookupValue("DisplayNormals",DisplayNormals))
		DisplayNormals = false;
	if (not cfg.lookupValue("DisplayWireframe",DisplayWireframe))
		DisplayWireframe = false;
	if (not cfg.lookupValue("DisplayWireframeShaded",DisplayWireframeShaded))
		DisplayWireframeShaded = true;
	if (not cfg.lookupValue("DisplayPolygons",DisplayPolygons))
		DisplayPolygons = true;
	if (not cfg.lookupValue("DisplayAllLayers",DisplayAllLayers))
		DisplayAllLayers = false;
	if (not cfg.lookupValue("DisplayinFill",DisplayinFill))
		DisplayinFill = true;
	if (not cfg.lookupValue("DisplayDebuginFill",DisplayDebuginFill))
		DisplayDebuginFill = false;
	if (not cfg.lookupValue("DisplayDebug",DisplayDebug))
		DisplayDebug = false;
	if (not cfg.lookupValue("DisplayCuttingPlane",DisplayCuttingPlane))
		DisplayCuttingPlane = false;
	if (not cfg.lookupValue("DrawVertexNumbers",DrawVertexNumbers))
		DrawVertexNumbers = false;
	if (not cfg.lookupValue("DrawLineNumbers",DrawLineNumbers))
		DrawLineNumbers = false;

	if (not cfg.lookupValue("PolygonVal",PolygonVal))
		PolygonVal = 0.5;
	if (not cfg.lookupValue("PolygonSat",PolygonSat))
		PolygonSat = 1;
	if (not cfg.lookupValue("PolygonHue",PolygonHue))
		PolygonHue = 0.54;
	if (not cfg.lookupValue("WireframeVal",WireframeVal))
		WireframeVal = 1;
	if (not cfg.lookupValue("WireframeSat",WireframeSat))
		WireframeSat = 1;
	if (not cfg.lookupValue("WireframeHue",WireframeHue))
		WireframeHue = 0.08;
	if (not cfg.lookupValue("NormalsSat",NormalsSat))
		NormalsSat = 1;
	if (not cfg.lookupValue("NormalsVal",NormalsVal))
		NormalsVal = 1;
	if (not cfg.lookupValue("NormalsHue",NormalsHue))
		NormalsHue = 0.23;
	if (not cfg.lookupValue("EndpointsSat",EndpointsSat))
		EndpointsSat = 1;
	if (not cfg.lookupValue("EndpointsVal",EndpointsVal))
		EndpointsVal = 1;
	if (not cfg.lookupValue("EndpointsHue",EndpointsHue))
		EndpointsHue = 0.45;
	if (not cfg.lookupValue("GCodeExtrudeHue",GCodeExtrudeHue))
		GCodeExtrudeHue = 0.18;
	if (not cfg.lookupValue("GCodeExtrudeSat",GCodeExtrudeSat))
		GCodeExtrudeSat = 1;
	if (not cfg.lookupValue("GCodeExtrudeVal",GCodeExtrudeVal))
		GCodeExtrudeVal = 1;
	if (not cfg.lookupValue("GCodeMoveHue",GCodeMoveHue))
		GCodeMoveHue = 1;
	if (not cfg.lookupValue("GCodeMoveSat",GCodeMoveSat))
		GCodeMoveSat = 0.95;
	if (not cfg.lookupValue("GCodeMoveVal",GCodeMoveVal))
		GCodeMoveVal = 1;
	if (not cfg.lookupValue("Highlight",Highlight))
		Highlight = 0.7;
	if (not cfg.lookupValue("NormalsLength",NormalsLength))
		NormalsLength = 10;
	if (not cfg.lookupValue("EndPointSize",EndPointSize))
		EndPointSize = 8;

	if (not cfg.lookupValue("TempUpdateSpeed",TempUpdateSpeed))
		TempUpdateSpeed = 3;

	if (not cfg.lookupValue("DisplayGCode",DisplayGCode))
		DisplayGCode = true;
	if (not cfg.lookupValue("LuminanceShowsSpeed",LuminanceShowsSpeed))
		LuminanceShowsSpeed = false;

	if (not cfg.lookupValue("RaftEnable",RaftEnable))
		RaftEnable = false;
	if (not cfg.lookupValue("ApronEnable",ApronEnable))
		ApronEnable = false;
	if (not cfg.lookupValue("ApronPreview",ApronPreview))
		ApronPreview = true;
	if (not cfg.lookupValue("ApronSize",ApronSize))
		ApronSize = 1.33;
	if (not cfg.lookupValue("ApronHeight",ApronHeight))
		ApronHeight = 7;
	if (not cfg.lookupValue("ApronCoverageX",ApronCoverageX))
		ApronCoverageX = 60;
	if (not cfg.lookupValue("ApronCoverageY",ApronCoverageY))
		ApronCoverageY = 60;
	if (not cfg.lookupValue("ApronDistanceToObject",ApronDistanceToObject))
		ApronDistanceToObject = 0.5;
	if (not cfg.lookupValue("ApronInfillDistance",ApronInfillDistance))
		ApronInfillDistance = 2;

	if (not cfg.lookupValue("ShrinkLogick",m_ShrinkQuality))
	{
		if (not cfg.lookupValue("ShrinkFast",m_ShrinkQuality))
			m_ShrinkQuality = SHRINK_FAST;
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
	SaveConfig();
}

void ProcessController::SaveSettingsAs(string path)
{
	SaveBuffers();
	SaveConfig(path + ".conf");
}

void ProcessController::SaveConfig()
{
	string filename = m_Filename + ".conf";
	SaveConfig(filename);
}

void ProcessController::SaveConfig(string path)
{
	// add error handling
	Config cfg;
	Setting &root = cfg.getRoot();

	// Raft parameters
	Setting &rs = root.add("RaftSize", Setting::TypeFloat);
	rs = RaftSize;

	Setting &rblc = root.add("RaftBaseLayerCount", Setting::TypeInt);
	rblc = int(RaftBaseLayerCount);
	Setting &rmpdr = root.add("RaftMaterialPrDistanceRatio", Setting::TypeFloat);
	rmpdr = RaftMaterialPrDistanceRatio;
	Setting &rafrot = root.add("RaftRotation", Setting::TypeFloat);
	rafrot = RaftRotation;
	Setting &rbd = root.add("RaftBaseDistance", Setting::TypeFloat);
	rbd = RaftBaseDistance;
	Setting &rbt = root.add("RaftBaseThickness", Setting::TypeFloat);
	rbt = RaftBaseThickness;
	Setting &rbT = root.add("RaftBaseTemperature", Setting::TypeFloat);
	rbT = RaftBaseTemperature;
	Setting &rilc = root.add("RaftInterfaceLayerCount", Setting::TypeInt);
	rilc = int(RaftInterfaceLayerCount);
	Setting &rimpdr = root.add("RaftInterfaceMaterialPrDistanceRatio", Setting::TypeFloat);
	rimpdr = RaftInterfaceMaterialPrDistanceRatio;
	Setting &rrpl = root.add("RaftRotationPrLayer", Setting::TypeFloat);
	rrpl = RaftRotationPrLayer;
	Setting &rid = root.add("RaftInterfaceDistance", Setting::TypeFloat);
	rid = RaftInterfaceDistance;
	Setting &rit = root.add("RaftInterfaceThickness", Setting::TypeFloat);
	rit = RaftInterfaceDistance;
	Setting &riT = root.add("RaftInterfaceTemperature", Setting::TypeFloat);
	riT = RaftInterfaceTemperature;

	// GCode parameters
	Setting &gst = root.add("GCodeStartText", Setting::TypeString);
	gst = GCodeStartText;
	Setting &glt = root.add("GCodeLayerText", Setting::TypeString);
	glt = GCodeLayerText;
	Setting &get = root.add("GCodeEndText", Setting::TypeString);
	get = GCodeEndText;
	Setting &mspn = root.add("msPortName", Setting::TypeString);
	mspn = m_sPortName;
	Setting &vc = root.add("ValidateConnection", Setting::TypeInt);
	vc = int(m_bValidateConnection);

	Setting &cbg = root.add("CustomButtonGcode", Setting::TypeArray);	
	Setting &cbl = root.add("CustomButtonLabel", Setting::TypeArray);
	for (int i = 0; i < 20; i++) {
		cbg.add(Setting::TypeString);
		cbl.add(Setting::TypeString);
		cbg[i] = CustomButtonGcode[i];
		cbl[i] = CustomButtonLabel[i];
	}

	Setting &miss = root.add("miSerialSpeed", Setting::TypeInt);
	miss = m_iSerialSpeed;
	Setting &gds = root.add("GCodeDrawStart", Setting::TypeFloat);
	gds = GCodeDrawStart;

	Setting &gde = root.add("GCodeDrawEnd", Setting::TypeFloat);
	gde = GCodeDrawEnd;
	Setting &se = root.add("ShellOnly", Setting::TypeBoolean);
	se = ShellOnly;
	Setting &sc = root.add("ShellCount", Setting::TypeInt);
	sc = int(ShellCount);

	// Printer parameters
	Setting &mpsxy = root.add("MinPrintSpeedXY", Setting::TypeFloat);
	mpsxy = MinPrintSpeedXY;
	Setting &Mpsxy = root.add("MaxPrintSpeedXY", Setting::TypeFloat);
	Mpsxy = MaxPrintSpeedXY;
	Setting &mpsz = root.add("MinPrintSpeedZ", Setting::TypeFloat);
	mpsz = MinPrintSpeedZ;
	Setting &Mpsz = root.add("MaxPrintSpeedZ", Setting::TypeFloat);
	Mpsz = MaxPrintSpeedZ;
	Setting &dtrfs = root.add("DistanceToReachFullSpeed", Setting::TypeFloat);
	dtrfs = DistanceToReachFullSpeed;
	Setting &ef = root.add("extrusionFactor", Setting::TypeFloat);
	ef = extrusionFactor;
	Setting &ea = root.add("EnableAcceleration", Setting::TypeBoolean);
	ea = EnableAcceleration;
	Setting &uie = root.add("UseIncrementalEcode", Setting::TypeBoolean);
	uie = UseIncrementalEcode;
	Setting &u3g = root.add("Use3DGcode", Setting::TypeBoolean);
	u3g = Use3DGcode;
	Setting &Ea = root.add("EnableAntiooze", Setting::TypeBoolean);
	Ea = EnableAntiooze;
	Setting &ad = root.add("AntioozeDistance", Setting::TypeFloat);
	ad = AntioozeDistance;
	Setting &as = root.add("AntioozeSpeed", Setting::TypeFloat);
	as = AntioozeSpeed;

	Setting &fle = root.add("FileLogginEnabled", Setting::TypeBoolean);
	fle = FileLogginEnabled;
	Setting &tre = root.add("TempReadingEnabled", Setting::TypeBoolean);
	tre = TempReadingEnabled;
	Setting &clwps = root.add("ClearLogfilesWhenPrintStarts", Setting::TypeBoolean);
	clwps = ClearLogfilesWhenPrintStarts;

	Setting &mfvx = root.add("mfVolumeX", Setting::TypeFloat);
	mfvx = m_fVolume.x;
	Setting &mfvy = root.add("mfVolumeY", Setting::TypeFloat);
	mfvy = m_fVolume.y;
	Setting &mfvz = root.add("mfVolumeZ", Setting::TypeFloat);
	mfvz = m_fVolume.z;
	Setting &pmx = root.add("PrintMarginX", Setting::TypeFloat);
	pmx = PrintMargin.x;
	Setting &pmy = root.add("PrintMarginY", Setting::TypeFloat);
	pmy = PrintMargin.y;
	Setting &prinmz = root.add("PrintMarginZ", Setting::TypeFloat);
	prinmz = PrintMargin.z;

	Setting &emw = root.add("ExtrudedMaterialWidth", Setting::TypeFloat);
	emw = ExtrudedMaterialWidth;
	Setting &lt = root.add("LayerThickness", Setting::TypeFloat);
	lt = LayerThickness;

	// CuttingPlane parameters
	Setting &id = root.add("InfillDistance", Setting::TypeFloat);
	id = InfillDistance;
	Setting &ir = root.add("InfillRotation", Setting::TypeFloat);
	ir = InfillRotation;
	Setting &irpl = root.add("InfillRotationPrLayer", Setting::TypeFloat);
	irpl = InfillRotationPrLayer;
	Setting &aid = root.add("AltInfillDistance", Setting::TypeFloat);
	aid = AltInfillDistance;
	Setting &ail = root.add("AltInfillLayersText", Setting::TypeString);
	ail = AltInfillLayersText;
	Setting &po = root.add("PolygonOpasity", Setting::TypeFloat);
	po = PolygonOpasity;

	// GUI parameters
	Setting &cpv = root.add("CuttingPlaneValue", Setting::TypeFloat);
	cpv = CuttingPlaneValue;
	Setting &exam = root.add("Examine", Setting::TypeFloat);
	exam = Examine;

	Setting &de = root.add("DisplayEndpoints", Setting::TypeBoolean);
	de = DisplayEndpoints;
	Setting &dn = root.add("DisplayNormals", Setting::TypeBoolean);
	dn = DisplayNormals;
	Setting &dw = root.add("DisplayWireframe", Setting::TypeBoolean);
	dw = DisplayWireframe;
	Setting &dws = root.add("DisplayWireframeShaded", Setting::TypeBoolean);
	dws = DisplayWireframeShaded;
	Setting &dp = root.add("DisplayPolygons", Setting::TypeBoolean);
	dp = DisplayPolygons;
	Setting &dal = root.add("DisplayAllLayers", Setting::TypeBoolean);
	dal = DisplayAllLayers;
	Setting &dif = root.add("DisplayinFill", Setting::TypeBoolean);
	dif = DisplayinFill;
	Setting &ddif = root.add("DisplayDebuginFill", Setting::TypeBoolean);
	ddif = DisplayDebuginFill;
	Setting &dd = root.add("DisplayDebug", Setting::TypeBoolean);
	dd = DisplayDebug;
	Setting &dcp = root.add("DisplayCuttingPlane", Setting::TypeBoolean);
	dcp = DisplayCuttingPlane;
	Setting &dvn = root.add("DrawVertexNumbers", Setting::TypeBoolean);
	dvn = DrawVertexNumbers;
	Setting &dln = root.add("DrawLineNumbers", Setting::TypeBoolean);
	dln = DrawLineNumbers;
	Setting &pv = root.add("PolygonVal", Setting::TypeFloat);
	pv = PolygonVal;
	Setting &ps = root.add("PolygonSat", Setting::TypeFloat);
	ps = PolygonSat;
	Setting &ph = root.add("PolygonHue", Setting::TypeFloat);
	ph = PolygonHue;
	Setting &wfv = root.add("WireframeVal", Setting::TypeFloat);
	wfv = WireframeVal;
	Setting &wfs = root.add("WireframeSat", Setting::TypeFloat);
	wfs = WireframeSat;
	Setting &wfh = root.add("WireframeHue", Setting::TypeFloat);
	wfh = WireframeHue;
	Setting &ns = root.add("NormalsSat", Setting::TypeFloat);
	ns = NormalsSat;
	Setting &nv = root.add("NormalsVal", Setting::TypeFloat);
	nv = NormalsVal;
	Setting &nh = root.add("NormalsHue", Setting::TypeFloat);
	nh = NormalsHue;
	Setting &es = root.add("EndpointsSat", Setting::TypeFloat);
	es = EndpointsSat;
	Setting &ev = root.add("EndpointsVal", Setting::TypeFloat);
	ev = EndpointsVal;
	Setting &eh = root.add("EndpointsHue", Setting::TypeFloat);
	eh = EndpointsHue;
	Setting &geh = root.add("GCodeExtrudeHue", Setting::TypeFloat);
	geh = GCodeExtrudeHue;
	Setting &ges = root.add("GCodeExtrudeSat", Setting::TypeFloat);
	ges = GCodeExtrudeSat;
	Setting &gev = root.add("GCodeExtrudeVal", Setting::TypeFloat);
	gev = GCodeExtrudeVal;
	Setting &gmh = root.add("GCodeMoveHue", Setting::TypeFloat);
	gmh = GCodeMoveHue;
	Setting &gms = root.add("GCodeMoveSat", Setting::TypeFloat);
	gms = GCodeMoveSat;
	Setting &gmv = root.add("GCodeMoveVal", Setting::TypeFloat);
	gmv = GCodeMoveVal;
	Setting &hil = root.add("Highlight", Setting::TypeFloat);
	hil = Highlight;

	Setting &nl = root.add("NormalsLength", Setting::TypeFloat);
	nl = NormalsLength;
	Setting &eps = root.add("EndPointSize", Setting::TypeFloat);
	eps = EndPointSize;
	Setting &tus = root.add("TempUpdateSpeed", Setting::TypeFloat);
	tus = TempUpdateSpeed;

	Setting &dgc = root.add("DisplayGCode", Setting::TypeBoolean);
	dgc = DisplayGCode;

	Setting &lss = root.add("LuminanceShowsSpeed", Setting::TypeBoolean);
	lss = LuminanceShowsSpeed;

	Setting &rafte = root.add("RaftEnable", Setting::TypeBoolean);
	rafte = RaftEnable;

	Setting &aproe = root.add("ApronEnable", Setting::TypeBoolean);
	aproe = ApronEnable;

	Setting &aprpre = root.add("ApronPreview", Setting::TypeBoolean);
	aprpre = ApronPreview;
	Setting &aprsiz = root.add("ApronSize", Setting::TypeFloat);
	aprsiz = ApronSize;
	
	Setting &aprh = root.add("ApronHeight", Setting::TypeFloat);
	aprh = ApronHeight;
	Setting &acx = root.add("ApronCoverageX", Setting::TypeFloat);
	acx = ApronCoverageX;
	Setting &acy = root.add("ApronCoverageY", Setting::TypeFloat);
	acy = ApronCoverageY;
	Setting &adto = root.add("ApronDistanceToObject", Setting::TypeFloat);
	adto = ApronDistanceToObject;
	Setting &ainfd = root.add("ApronInfillDistance", Setting::TypeFloat);
	ainfd = ApronInfillDistance;

	Setting &shf = root.add("ShrinkFast", Setting::TypeBoolean);
	shf = (m_ShrinkQuality == SHRINK_FAST);
	Setting &shl = root.add("ShrinkLogick", Setting::TypeBoolean);
	shl = (m_ShrinkQuality == SHRINK_LOGICK);
	Setting &optim = root.add("Optimization", Setting::TypeFloat);
	optim = Optimization;
	Setting &rebs = root.add("ReceivingBufferSize", Setting::TypeInt);
	rebs = ReceivingBufferSize;

	Setting &stlp = root.add("STLPath", Setting::TypeString);
	stlp = STLPath;
	Setting &rfop = root.add("RFOPath", Setting::TypeString);
	rfop = RFOPath;
	Setting &gcop = root.add("GCodePath", Setting::TypeString);
	gcop = GCodePath;
	Setting &setp = root.add("SettingsPath", Setting::TypeString);
	setp = SettingsPath;
	
	cfg.writeFile(path.c_str());
}
