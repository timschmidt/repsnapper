/* -------------------------------------------------------- *
*
* ProcessController.h
*
* Copyright 2009+ Michael Holm - www.kulitorum.com
*
* This file is part of RepSnapper and is made available under
* the terms of the GNU General Public License, version 2, or at your
* option, any later version, incorporated herein by reference.
*
* ------------------------------------------------------------------------- */
#pragma once

#include "stdafx.h"
#include "Printer.h"
#include <vmmlib/vmmlib.h>
#include "gcode.h"
#include "RFO.h"

// struct lua_State;
using namespace std;

class XML_Element;

class ProcessController
{
public:
	ProcessController(){

		m_iSerialSpeed = 19200;
		// default parameters (are overwritten by the xml loading)
		RaftSize = 1.33f;
		RaftBaseLayerCount = 1;
		RaftMaterialPrDistanceRatio = 1.8f;
		RaftRotation = 90.0f;
		RaftBaseDistance = 2.0f;
		RaftBaseThickness = 1.0f;
		RaftBaseTemperature = 1.10f;
		RaftInterfaceLayerCount = 2;
		RaftInterfaceMaterialPrDistanceRatio = 1.0f;
		RaftRotationPrLayer = 90.0f;
		RaftInterfaceDistance = 2.0f;
		RaftInterfaceThickness = 1.0f;
		RaftInterfaceTemperature = 1.0f;
		m_Filename = "./repsnapper";

		// Printer
		m_fVolume = Vector3f(200,200,140);
		PrintMargin = Vector3f(10,10,0);
		ExtrudedMaterialWidth = 0.7f;	// 0.7

		KeepLines = 1000;

		//GCode
		GCodeDrawStart = 0.0f;;
		GCodeDrawEnd = 1.0f;

		MinPrintSpeedXY = 1000.0f;
		MaxPrintSpeedXY = 4000.0f;
		MinPrintSpeedZ = 50.0f;
		MaxPrintSpeedZ = 150.0f;

		DistanceToReachFullSpeed= 1.5f;
		extrusionFactor = 1.0f;
		UseIncrementalEcode = false;
		Use3DGcode = false;

		EnableAntiooze = false;
		AntioozeDistance = 4.5f;
		AntioozeSpeed = 1000.0f;

		LayerThickness = 0.4f;
		CuttingPlaneValue = 0.5f;
		PolygonOpasity = 0.5f;

		DisplayEndpoints = false;
		DisplayNormals = false;
		DisplayBBox = false;
		DisplayWireframe = false;
		DisplayWireframeShaded = true;
		DisplayPolygons = false;
		DisplayAllLayers = false;
		DisplayinFill = false;

		InfillDistance = 2.0f;
		InfillRotation = 45.0f;
		InfillRotationPrLayer = 90.0f;
		AltInfillDistance = 2.0f;
		AltInfillLayersText = "";
		Examine = 0.5f;
		Optimization = 0.02f;

		DisplayDebuginFill = false;
		DisplayDebug = false;
		DisplayCuttingPlane = true;
		DrawVertexNumbers=false;
		DrawLineNumbers=false;

		ShellOnly = false;
		ShellCount = 1;

		EnableAcceleration = true;
		DisplayDebuginFill = true;
		DisplayCuttingPlane = true;

		DrawCPVertexNumbers = false;
		DrawCPLineNumbers = false;
		DrawCPOutlineNumbers = false;

		FileLogginEnabled = true;
		TempReadingEnabled = true;
		ClearLogfilesWhenPrintStarts = true;

		Min = Vector3f(0, 0, 0);
		Max = Vector3f(200,200,200);
		Center.x = Center.y = 100.0f;
		Center.z = 0.0f;

		gui = 0;
		CustomButtonGcode.resize(20);
		CustomButtonLabel.resize(20);
};

//	ProcessController::~ProcessController();

	void SetFilename(string filename) { m_Filename = filename;}

	void Draw(Flu_Tree_Browser::Node *selected_node);
	
	// STL Functions
	bool ReadStl(string filename, STL &newstl) { return newstl.Read(filename);};
	void OptimizeRotation();
	void RotateObject(Vector3f axis, float a);
	Matrix4f GetSTLTransformationMatrix(int object=-1, int file=-1) const ;
	void CalcBoundingBoxAndZoom();

	void ConvertToGCode(string &GcodeTxt, const string &GcodeStart, const string &GcodeLayer, const string &GcodeEnd);

	// GCode Functions
	void ReadGCode(string filename);
	void WriteGCode(string &GcodeTxt, const string &GcodeStart, const string &GcodeLayer, const string &GcodeEnd, string filename);

	void MakeRaft(float &z);
	//Printer
	void SetVolume(float x, float y, float z) { m_fVolume = Vector3f(x,y,z);}

	// Load and save settings
	void LoadXML();
	void LoadXML(string filename);
	void SaveXML();
	void SaveXML(string filename);
	void LoadXML(XMLElement *e);
	void SaveXML(XMLElement *e);

	// LUA
//	void BindLua(lua_State *myLuaState);

	void GetAltInfillLayers(vector<int>& layers, uint layerCount) const;
	void SaveBuffers();
	void SaveSettings();
	void SaveSettingsAs(string path);

	// Process functions
	string m_Filename;

	// Start, layer, end GCode
	string GCodeStartText;
	string GCodeLayerText;
	string GCodeEndText;

	/*--------------Models-------------------*/
	Printer printer;					// Printer settings and functions
	string m_sPortName;
	int m_iSerialSpeed;
	bool m_bValidateConnection;
	int KeepLines;
//	STL stl;							// A STL file
	RFO rfo;
	GCode gcode;						// Gcode as binary data
	string GcodeTxt;					// Final GCode as text

	float Optimization;
	int ReceivingBufferSize;

	// Raft
	float RaftSize;
	uint RaftBaseLayerCount;
	float RaftMaterialPrDistanceRatio;
	float RaftRotation;
	float RaftBaseDistance;
	float RaftBaseThickness;
	float RaftBaseTemperature;
	uint RaftInterfaceLayerCount;
	float RaftInterfaceMaterialPrDistanceRatio;
	float RaftRotationPrLayer;
	float RaftInterfaceDistance;
	float RaftInterfaceThickness;
	float RaftInterfaceTemperature;

	// GCode
	float GCodeDrawStart;
	float GCodeDrawEnd;
	float MinPrintSpeedXY;
	float MaxPrintSpeedXY;
	float MinPrintSpeedZ;
	float MaxPrintSpeedZ;

	float DistanceToReachFullSpeed;
	float extrusionFactor;

	bool EnableAntiooze;
	float AntioozeDistance;
	float AntioozeSpeed;

	// Printer
	Vector3f	m_fVolume;				// Max print volume
	Vector3f	PrintMargin;
	Vector3f	printOffset;			// Summed up margin+raft+Apron etc.
	float		ExtrudedMaterialWidth;	// Width of extruded material
	bool		UseIncrementalEcode;
	bool		Use3DGcode;

	// STL 
	float LayerThickness;
	float CuttingPlaneValue;
	float PolygonOpasity;

	// CuttingPlane
	float InfillDistance;
	float InfillRotation;
	float InfillRotationPrLayer;
	float AltInfillDistance;
	string AltInfillLayersText;
	float Examine;

	bool ShellOnly;
	uint ShellCount;

	bool EnableAcceleration;

	bool FileLogginEnabled;
	bool TempReadingEnabled;
	bool ClearLogfilesWhenPrintStarts;


	// GUI... ?
	bool DisplayEndpoints;
	bool DisplayNormals;
	bool DisplayBBox;
	bool DisplayWireframe;
	bool DisplayWireframeShaded;
	bool DisplayPolygons;
	bool DisplayAllLayers;
	bool DisplayinFill;
	bool DisplayDebuginFill;
	bool DisplayDebug;
	bool DisplayCuttingPlane;
	bool DrawVertexNumbers;
	bool DrawLineNumbers;
	bool DrawOutlineNumbers;
	bool DrawCPVertexNumbers;
	bool DrawCPLineNumbers;
	bool DrawCPOutlineNumbers;
	//string Notes; // Thers is no ui element for this field, it was causing problems with GCodeEnd UI field

	// Rendering
	float PolygonVal;
	float PolygonSat;
	float PolygonHue;
	float WireframeVal;
	float WireframeSat;
	float WireframeHue;
	float NormalsSat;
	float NormalsVal;
	float NormalsHue;
	float EndpointsSat;
	float EndpointsVal;
	float EndpointsHue;
	float GCodeExtrudeHue;
	float GCodeExtrudeSat;
	float GCodeExtrudeVal;
	float GCodeMoveHue;
	float GCodeMoveSat;
	float GCodeMoveVal;
	float Highlight;
	float NormalsLength;
	float EndPointSize;
	float TempUpdateSpeed;

	bool LuminanceShowsSpeed;
	bool DisplayGCode;

	bool ApronEnable;
	bool ApronPreview;
	bool RaftEnable;
	float ApronSize;
	float ApronHeight;
	float ApronCoverageX;
	float ApronCoverageY;
	float ApronDistanceToObject;
	float ApronInfillDistance;

	int m_ShrinkQuality;

	// Bounding box info
	Vector3f Center;
	Vector3f Min;
	Vector3f Max;

	vector<string>CustomButtonGcode;
	vector<string>CustomButtonLabel;

	string STLPath;
	string RFOPath;
	string GCodePath;
	string SettingsPath;

	// Maybe a pointer to the gui
	GUI *gui;
};
