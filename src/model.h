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

#include <QFile>

#include "files.h"


#include "objlist.h"
#include "settings.h"

#include <QStatusBar>
#include <QTextStream>
#include <mainwindow.h>

class GCode;
#ifdef _MSC_VER // Visual C++ compiler
#  pragma warning( disable : 4244 4267)
#endif


class Model
{
//        sigc::signal< void > m_signal_zoom;
        ViewProgress *m_progress;

public:
        QStatusBar *statusbar;

        // Something in the rfo changed
//        sigc::signal< void > signal_zoom() { return m_signal_zoom; }
//        sigc::signal< void > m_signal_gcode_changed;

        Model(MainWindow *main);
        ~Model();

        void SimpleAdvancedToggle();
//        void SaveConfig(QString filename);
        void LoadConfig() { LoadConfig(QString::fromStdString("repsnapper.conf")); }
        void LoadConfig(QString filename);

        // STL Functions
        void ReadStl(QFile *file);
        vector<Shape*> ReadShapes(QFile *file,
                                  uint max_triangles = 0);
        void SaveStl(QFile *file);
#if ENABLE_AMF
        void SaveAMF(QFile *file);
#endif
        int AddShape(ListObject *parent, Shape * shape, string filename,
                     bool autoplace = true);
        int SplitShape(ListObject *parent, Shape *shape, string filename);
        int MergeShapes(ListObject *parent, const vector<Shape*> shapes);
        int DivideShape(ListObject *parent, Shape *shape, string filename);
        Shape GetCombinedShape() const;

//        sigc::signal< void, Gtk::TreePath & > m_signal_stl_added;

        void Read(QFile *file);
        void SetViewProgress (ViewProgress *progress);

        void DeleteSelectedObjects(QModelIndexList *objectList);
        QModelIndexList m_current_selectionpath;

        void OptimizeRotation(Shape *shape, ListObject *object);
        void ScaleObject(Shape *shape, ListObject *object, double scale);
        void ScaleObjectX(Shape *shape, ListObject *object, double scale);
        void ScaleObjectY(Shape *shape, ListObject *object, double scale);
        void ScaleObjectZ(Shape *shape, ListObject *object, double scale);
        void RotateObject(Shape *shape, ListObject *object, Vector4d rotate);
        void TwistObject(Shape *shape, ListObject *object, double angle);
        void PlaceOnPlatform(Shape *shape, ListObject *object);
//        bool updateStatusBar(GdkEventCrossing *event, Glib::ustring = "");
        void InvertNormals(Shape *shape, ListObject *object);
        void Mirror(Shape *shape, ListObject *object);

        vector<Layer*> layers;

        Layer * m_previewLayer;
        double get_preview_Z();
        //Layer * m_previewGCodeLayer;
        GCode *m_previewGCode;
        double m_previewGCode_z;

        vector<Shape*> preview_shapes;

        // Slicing
        void SliceToSVG(QFile *file, bool single_layer=false);

        // GCode Functions
        void ReadGCode(QFile *file);
        void translateGCode(Vector3d trans);

        void ConvertToGCode();

        void MakeRaft(GCodeState &state, double &z);
        void WriteGCode(QFile *file);
        void ClearGCode();
        void ClearLayers();
        void ClearPreview();
        QTextDocument * GetGCodeBuffer();
        void GlDrawGCode(int layer=-1); // should be in the view
        void GlDrawGCode(double Z);
        void setCurrentPrintingLine(ulong line){ currentprintingline = line; }
        unsigned long currentprintingline;

        Matrix4f &SelectedNodeMatrix(guint objectNr = 1);
        void SelectedNodeMatrices(vector<Matrix4d *> &result );
        ListObject *newObject();

        // Model derived: Bounding box info
        Vector3d Center;
        Vector3d Min;
        Vector3d Max;

        void CalcBoundingBoxAndCenter(bool selected_only = false);
        Vector3d GetViewCenter();
        bool AutoArrange(QModelIndexList *selected);
        Vector3d FindEmptyLocation(const vector<Shape*> &shapes,
                                   const vector<Matrix4d> &transforms,
                                   const Shape *shape);
        bool FindEmptyLocation(Vector3d &result, const Shape *stl);

//        sigc::signal< void > m_model_changed;
        void ModelChanged();
        bool m_inhibit_modelchange;

        // Truly the model
        ObjectsList objectList;

        QTextStream errlog, echolog;

        int draw(QModelIndexList *selected);
        int drawLayers(float height, const Vector3d &offset, bool calconly = false);
        void setMeasuresPoint(const Vector3d &point);
        Vector2d measuresPoint;

        Layer * calcSingleLayer(double z, uint LayerNr, double thickness,
                                bool calcinfill, bool for_gcode=false);

//        sigc::signal< void, Gtk::MessageType, const char *, const char * > signal_alert;
        void alert (const char *message);
        void error (const char *message, const char *secondary);

        void ClearLogs();

        GCode *gcode;

        void SetIsPrinting(bool printing) { is_printing = printing; }

        string getSVG(int single_layer_no = -1);
        void ReadSVG(QFile *file);

        bool isCalculating() const { return is_calculating; }

        Settings * settings;

 private:
        bool is_calculating;
        bool is_printing;
        //GCodeIter *m_iter;
        Layer * lastlayer;

        // Slicing/GCode conversion functions
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

        MainWindow * main;
};

#endif // MODEL_H
