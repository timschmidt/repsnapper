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
#pragma once

#include "stdafx.h"
#include "printer.h"
#include <vmmlib/vmmlib.h>
#include "gcode.h"
#include "rfo.h"

// struct lua_State;
using namespace std;

class ProcessController
{
	sigc::signal< void > m_signal_rfo_changed;
	sigc::signal< void > m_signal_setting_changed;
public:
	// Something in the rfo changed
	sigc::signal< void > signal_rfo_changed() { return m_signal_rfo_changed; }
	// Some setting changed
	sigc::signal< void > signal_setting_changed() { return m_signal_setting_changed; }
	void emit_rfo_changed() { m_signal_rfo_changed.emit(); }
	void emit_setting_changed() { m_signal_setting_changed.emit(); }

	ProcessController(){
		m_iSerialSpeed = 57600;
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
		PolygonOpacity = 0.5f;

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

		FileLoggingEnabled = true;
		TempReadingEnabled = true;
		ClearLogfilesWhenPrintStarts = true;

		Min = Vector3f(0, 0, 0);
		Max = Vector3f(200,200,200);
		Center.x = Center.y = 100.0f;
		Center.z = 0.0f;

		m_progress = NULL;

		gui = 0;
		CustomButtonGcode.resize(20);
		CustomButtonLabel.resize(20);

		m_GCodeStartText = Gtk::TextBuffer::create();
		m_GCodeLayerText = Gtk::TextBuffer::create();
		m_GCodeEndText = Gtk::TextBuffer::create();
	}

	void SetProgress(Progress *progress) { m_progress = progress; }
	void SetFilename(string filename) { m_Filename = filename;}

	// STL Functions
	bool ReadStl(string filename, STL &newstl) { return newstl.Read(filename);};
	void OptimizeRotation();
	Matrix4f GetSTLTransformationMatrix(int object=-1, int file=-1) const ;
	void CalcBoundingBoxAndCenter();

	void ConvertToGCode();

	// GCode Functions
	void ReadGCode(string filename);
	void WriteGCode(string &GcodeTxt, const string &GcodeStart, const string &GcodeLayer, const string &GcodeEnd, string filename);
	void Draw (Gtk::TreeModel::iterator &selected);

	void MakeRaft(float &z);
	//Printer
	void SetVolume(float x, float y, float z) { m_fVolume = Vector3f(x,y,z);}

	// Load and save settings
	void SaveConfig(string path);
	void SaveConfig();
	void LoadConfig();
	void LoadConfig(string filename);

	// LUA
//	void BindLua(lua_State *myLuaState);

	void GetAltInfillLayers(vector<int>& layers, uint layerCount) const;
	void SaveBuffers();
	void SaveSettings();
	void SaveSettingsAs(string path);

	// Process functions
	string m_Filename;

	// Start, layer, end GCode
	Glib::RefPtr<Gtk::TextBuffer> m_GCodeStartText;
	Glib::RefPtr<Gtk::TextBuffer> m_GCodeLayerText;
	Glib::RefPtr<Gtk::TextBuffer> m_GCodeEndText;

	/*--------------Models-------------------*/
	Printer printer;					// Printer settings and functions
	string m_sPortName;
	int m_iSerialSpeed;
	bool m_bValidateConnection;
	int KeepLines;
	RFO rfo;
	GCode gcode;

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
	float PolygonOpacity;

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

	bool FileLoggingEnabled;
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

	Progress *m_progress;
	// Maybe a pointer to the gui
	GUI *gui;
};
