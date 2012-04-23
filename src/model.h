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

#include "stdafx.h"

#include "objtree.h" 
#include "gcode.h" 
/* #include "gcodestate.h" */
#include "settings.h"
/* #include "progress.h" */
/* #include "slicer/poly.h" */

#ifdef _MSC_VER // Visual C++ compiler
#  pragma warning( disable : 4244 4267)
#endif


class Model
{
  
	sigc::signal< void > m_signal_tree_changed;
	ViewProgress *m_progress;

public:
	Gtk::Statusbar *statusbar;
	// Something in the rfo changed
	sigc::signal< void > signal_tree_changed() { return m_signal_tree_changed; }

	Model();
	~Model();

	void SimpleAdvancedToggle();
	void SaveConfig(Glib::RefPtr<Gio::File> file);
	void LoadConfig() { LoadConfig(Gio::File::create_for_path("repsnapper.conf")); }
	void LoadConfig(Glib::RefPtr<Gio::File> file);

	// STL Functions
	void ReadStl(Glib::RefPtr<Gio::File> file, filetype_t ftype=UNKNOWN_TYPE);
	void SaveStl(Glib::RefPtr<Gio::File> file);

	int AddShape(TreeObject *parent, Shape * shape, string filename,
		     bool autoplace = true);
	int SplitShape(TreeObject *parent, Shape *shape, string filename);
	int DivideShape(TreeObject *parent, Shape *shape, string filename);

	sigc::signal< void, Gtk::TreePath & > m_signal_stl_added;

	void Read(Glib::RefPtr<Gio::File> file);
	void SetViewProgress (ViewProgress *progress);

	void DeleteObjTree(vector<Gtk::TreeModel::Path> &iter);
	vector<Gtk::TreeModel::Path> m_current_selectionpath;

	void OptimizeRotation(Shape *shape, TreeObject *object);
	void ScaleObject(Shape *shape, TreeObject *object, double scale);
	void ScaleObjectX(Shape *shape, TreeObject *object, double scale);
	void ScaleObjectY(Shape *shape, TreeObject *object, double scale);
	void ScaleObjectZ(Shape *shape, TreeObject *object, double scale);
	void RotateObject(Shape *shape, TreeObject *object, Vector4d rotate);
	void TwistObject(Shape *shape, TreeObject *object, double angle);
	void PlaceOnPlatform(Shape *shape, TreeObject *object);
	bool updateStatusBar(GdkEventCrossing *event, Glib::ustring = "");
	void InvertNormals(Shape *shape, TreeObject *object);
	void Mirror(Shape *shape, TreeObject *object);

	vector<Layer*> layers;
	
	Layer * m_previewLayer;
	double get_preview_Z();
	//Layer * m_previewGCodeLayer;
	GCode m_previewGCode;
	double m_previewGCode_z;
	// Slicing
	void SliceToSVG(Glib::RefPtr<Gio::File> file);
	void Slice();

	void CleanupLayers();
	void CalcInfill();
	void MakeShells();
	void MakeUncoveredPolygons(bool make_decor, bool make_bridges=true);
	vector<Poly> GetUncoveredPolygons(const Layer *subjlayer, 
					  const Layer *cliplayer);
	void MakeFullSkins();
	void MultiplyUncoveredPolygons();
	void MakeSupportPolygons(Layer * subjlayer, const Layer * cliplayer, 
				 double widen=0);
	void MakeSupportPolygons(double widen=0);
	void MakeSkirt();

	// GCode Functions
	void init();
	void ReadGCode(Glib::RefPtr<Gio::File> file);
	void translateGCode(Vector3d trans);

	void ConvertToGCode();

	void MakeRaft(GCodeState &state, double &z);
	void WriteGCode(Glib::RefPtr<Gio::File> file);
	void ClearGCode();
	void ClearLayers();
	void ClearPreview();
	Glib::RefPtr<Gtk::TextBuffer> GetGCodeBuffer();
	void GlDrawGCode(int layer=-1); // should be in the view
	void GlDrawGCode(double Z); 
	void setCurrentPrintingLine(long fromline, long toline){
	  currentprintingline = toline;
	  currentbufferedlines = (int)(toline-fromline);
	}
	unsigned long currentprintingline;
	int currentbufferedlines;

	Matrix4f &SelectedNodeMatrix(guint objectNr = 1);
	void SelectedNodeMatrices(vector<Matrix4d *> &result );
	void newObject();


	Settings settings;

	// Model derived: Bounding box info
	Vector3d Center;
	Vector3d Min;
	Vector3d Max;

	void CalcBoundingBoxAndCenter(bool selected_only = false);
	Vector3d GetViewCenter();
        bool AutoArrange(vector<Gtk::TreeModel::Path> &iter);
	Vector3d FindEmptyLocation(const vector<Shape*> &shapes,
				   const vector<Matrix4d> &transforms,
				   const Shape *shape);
        bool FindEmptyLocation(Vector3d &result, const Shape *stl);

	sigc::signal< void > m_model_changed;
	void ModelChanged();

	// Truly the model
	ObjectsTree objtree;
	Glib::RefPtr<Gtk::TextBuffer> errlog, echolog;

	int draw(vector<Gtk::TreeModel::Path> &selected);
	int drawLayers(double height, const Vector3d &offset, bool calconly = false);
	void setMeasuresPoint(const Vector3d &point);
	Vector2d measuresPoint;

	Layer * calcSingleLayer(double z, uint LayerNr, double thickness, 
				bool calcinfill, bool for_gcode=false) const ;

	sigc::signal< void, Gtk::MessageType, const char *, const char * > signal_alert;
	void alert (const char *message);
	void error (const char *message, const char *secondary);

	void ClearLogs();

	GCode gcode;

	void SetIsPrinting(bool printing) { is_printing = printing; };

	string getSVG() const;
	void ReadSVG(Glib::RefPtr<Gio::File> file);
	
 private:
	bool is_calculating;
	bool is_printing;
	//GCodeIter *m_iter;
	Layer * lastlayer;
};

#endif // MODEL_H
