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
#ifndef MODEL_H
#define MODEL_H

#include <math.h>

#include <giomm/file.h>

//#include "linked_ptr.h"

//#include "slicer.h"
#include "rfo.h"
#include "types.h"
#include "slicer/gcode.h"
#include "slicer/cuttingplane.h"
#include "settings.h"
#include "progress.h"

#ifdef WIN32
#  pragma warning( disable : 4244 4267)
#endif



class Model
{
  
	sigc::signal< void > m_signal_rfo_changed;

public:

	Progress m_progress;

	// Something in the rfo changed
	sigc::signal< void > signal_rfo_changed() { return m_signal_rfo_changed; }


	Model();
	~Model();
	void progess_bar_start (const char *label, double max);

	void SimpleAdvancedToggle();
	void SaveConfig(Glib::RefPtr<Gio::File> file);
	void LoadConfig() { LoadConfig(Gio::File::create_for_path("repsnapper.conf")); }
	void LoadConfig(Glib::RefPtr<Gio::File> file);

	// STL Functions
	void ReadStl(Glib::RefPtr<Gio::File> file);
	RFO_File *AddStl(RFO_Object *parent, Slicer stl, string filename);
	sigc::signal< void, Gtk::TreePath & > m_signal_stl_added;

	void Read(Glib::RefPtr<Gio::File> file);

	void DeleteRFO(Gtk::TreeModel::iterator &iter);

	void OptimizeRotation(RFO_File *file, RFO_Object *object);
	void ScaleObject(RFO_File *file, RFO_Object *object, double scale);
	void RotateObject(RFO_File *file, RFO_Object *object, Vector4d rotate);
	bool updateStatusBar(GdkEventCrossing *event, Glib::ustring = "");

	vector<CuttingPlane*> cuttingplanes;
	
	// Slicing
	void Slice(GCodeState &state, double printoffsetZ);
	void CalcInfill(GCodeState &state);
	void MakeShells();
	void MakeUncoveredPolygons();
	void MakeUncoveredPolygons(CuttingPlane *subjplane,const CuttingPlane *clipplane);
	void MultiplyUncoveredPolygons();
	void MakeSupportPolygons();

	// GCode Functions
	void init();
	void ReadGCode(Glib::RefPtr<Gio::File> file);
	void ConvertToGCode();
	void MakeRaft(GCodeState &state, double &z);
	void WriteGCode(Glib::RefPtr<Gio::File> file);
	void ClearGCode();
	void ClearCuttingPlanes();
	Glib::RefPtr<Gtk::TextBuffer> GetGCodeBuffer();
	void GlDrawGCode(); // should be in the view


	Matrix4f &SelectedNodeMatrix(guint objectNr = 1);
	void SelectedNodeMatrices(vector<Matrix4d *> &result );
	void newObject();


	Settings settings;

	// Model derived: Bounding box info
	Vector3d Center;
	Vector3d Min;
	Vector3d Max;
	vmml::Vector3d printOffset; // margin + raft

	void CalcBoundingBoxAndCenter();
        bool FindEmptyLocation(Vector3d &result, Slicer *stl);

	sigc::signal< void > m_model_changed;
	void ModelChanged();

	// Truly the model
	RFO rfo;
	Glib::RefPtr<Gtk::TextBuffer> errlog, echolog;

	void draw(Gtk::TreeModel::iterator &selected);
	void drawCuttingPlanes(Vector3d offset) const;

	sigc::signal< void, Gtk::MessageType, const char *, const char * > signal_alert;
	void alert (const char *message);
	void error (const char *message, const char *secondary);

	void ClearLogs();

	GCode gcode;
 private:

	//GCodeIter *m_iter;
};

#endif // MODEL_H
