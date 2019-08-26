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


class Model : public QObject
{
    Q_OBJECT

        ViewProgress *m_progress;

public:
        Model(MainWindow *main);
        ~Model();

        QStatusBar *statusbar;

        void SimpleAdvancedToggle();

// //        void SaveConfig(QString filename);
//        void LoadConfig() { LoadConfig(QString::fromStdString("repsnapper.conf")); }
        void LoadConfig(QString filename);

//        // STL Functions
        void ReadStl(QFile *file);
        vector<Shape*> ReadShapes(QFile *file,
                                  uint max_triangles = 0);
        void SaveStl(QFile *Readfile);
#if ENABLE_AMF
//        void SaveAMF(QFile *file);
#endif
        ListObject * AddShape(ListObject *parent, Shape * shape, QString filename,
                     bool autoplace = true);
        ulong SplitShape(ListObject *parent, Shape *shape, QString filename);
        int MergeShapes(ListObject *parent, const vector<Shape*> shapes);
        int DivideShapeAtZ(ListObject *parent, Shape *shape, QString filename);
        Shape GetCombinedShape() const;

////        sigc::signal< void, Gtk::TreePath & > m_signal_stl_added;

        void Read(QFile *file);
        void SetViewProgress (ViewProgress *progress);

        void DeleteSelectedObjects(const QModelIndexList *objectList);

        void OptimizeRotation(Shape *shape, ListObject *object);
        void ScaleObject(Shape *shape, ListObject *object, double scale);
        void ScaleObjectX(Shape *shape, ListObject *object, double scale);
        void ScaleObjectY(Shape *shape, ListObject *object, double scale);
        void ScaleObjectZ(Shape *shape, ListObject *object, double scale);
        void RotateObject(Shape *shape, ListObject *object, Vector4d rotate);
        void TwistObject(Shape *shape, ListObject *object, double angle);
        void PlaceOnPlatform(const QModelIndexList * selection);
////        bool updateStatusBar(GdkEventCrossing *event, QString = "");
        void InvertNormals(const QModelIndexList * selection);
        void Mirror(const QModelIndexList * selection);
        void Hollow(const QModelIndexList * selection);
        void Duplicate(const QModelIndexList * selection);
        void Split(const QModelIndexList * selection);
        void Merge(const QModelIndexList * selection);
        void DivideAtZ(const QModelIndexList * selection);

        vector<Layer*> layers;

        Layer * m_previewLayer;
        double get_previewLayer_Z();
//        //Layer * m_previewGCodeLayer;
        GCode *m_previewGCode;
        double m_previewGCode_z;

        vector<Shape*> preview_shapes;

        bool haveGCode() const;

//        // Slicing
        void SliceToSVG(QFile *file, bool single_layer=false);

//        // GCode Functions
        void ReadGCode(QTextDocument *doc, QFile *file);
        void translateGCode(Vector3d trans);

        void ConvertToGCode();

        double MakeRaft();
        void WriteGCode(QFile *file);
        void ClearGCode();
        void ClearLayers();
        void ClearPreview();

        void GlDrawGCode(int layer=-1); // should be in the view
        void GlDrawGCode(double Z);
        void setCurrentPrintingCommand(ulong commandno){ currentprintingcommand = commandno; }
        unsigned long currentprintingcommand;

//        Matrix4f &SelectedNodeMatrix(guint objectNr = 1);
//        void SelectedNodeMatrices(vector<Matrix4d *> &result );
        ListObject *newObject();

//        // Model derived: Bounding box info
        Vector3d Center;
        Vector3d Min;
        Vector3d Max;

        void CalcBoundingBoxAndCenter(bool selected_only = false);
        Vector3d GetViewCenter();
        bool AutoArrange(const QModelIndexList *selected);
        Vector3d FindEmptyLocation(const vector<Shape*> &shapes,
                                   const Shape *shape);
        bool FindEmptyLocation(Vector3d &result, const Shape *stl);

        void ModelChanged(bool objectsAddedOrRemoved = false);
        bool m_inhibit_modelchange;

//        // Truly the model
        ObjectsList objectList;

        QTextStream errlog, echolog;

        int getLayerNo(float height) const;

        int draw(const QModelIndexList *selected, bool objects_only = false);
        int drawLayers(double height, const Vector3d &offset, bool calconly = false);
        void setMeasuresPoint(const Vector2d &point);
        Vector2d measuresPoint;

        Layer * calcSingleLayer(double z, uint LayerNr, double thickness,
                                bool calcinfill, bool for_gcode=false);

////        sigc::signal< void, Gtk::MessageType, const char *, const char * > signal_alert;
        void alert (const char *message);
        void error (const char *message, const char *secondary);

        void ClearLogs();

        GCode *gcode;

//        void SetIsPrinting(bool printing) { is_printing = printing; }

//        string getSVG(int single_layer_no = -1);
        void ReadSVG(QFile *file);

        bool isCalculating() const { return is_calculating; }

        Settings * settings;

        void rotate_selection(QModelIndexList * selection, const Vector3d axis,
                              double angle);
        void move_selection(QModelIndexList * selection, const Vector3d move);
        void AutoRotate(const QModelIndexList *selected);

private:
        bool is_calculating;
        bool is_printing;
//        //GCodeIter *m_iter;
        Layer * lastlayer;

//        // Slicing/GCode conversion functions
        void Slice();

        void CleanupLayers();
        void CalcInfill();
        void MakeShells();
        void MakeUncoveredPolygons(bool make_decor, bool make_bridges=true,
                                   bool fillbottom = true);
        vector<Poly> GetUncoveredPolygons(const Layer *subjlayer,
                                          const Layer *cliplayer);
        void MakeFullSkins();
        void MultiplyUncoveredPolygons();
        void MakeSupportPolygons(Layer * subjlayer, const Layer * cliplayer,
                                 uint extruder, double widen=0);
        void MakeSupportPolygons(uint extruder, double widen=0);
        void MakeSkirt();

        MainWindow * main;


        // Something in the rfo changed
//        sigc::signal< void > signal_zoom() { return m_signal_zoom; }
//        sigc::signal< void > m_signal_gcode_changed;

signals:
        void model_changed(const ObjectsList * objlist = nullptr);
        void alert(const QString &message);

};

#endif // MODEL_H
