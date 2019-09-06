/*
    This file is a part of the RepSnapper project.
    Copyright (C) 2011 Michael Meeks

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#define  MODEL_IMPLEMENTATION
#include <vector>
#include <string>
#include <cerrno>
#include <functional>
#include <numeric>

#include <QFileInfo>
#include <QDir>

#include <src/gcode/gcodestate.h>

#include "stdafx.h"
#include "model.h"
#include "settings.h"
#include "slicer/layer.h"
#include "slicer/infill.h"
#include "ui/progress.h"
#include "shape.h"
//#include "flatshape.h"
#include "render.h"

Model::Model(MainWindow *main) :
  m_previewLayer(nullptr),
  //m_previewGCodeLayer(NULL),
  currentprintingcommand(0),
  Min(), Max(),
  m_inhibit_modelchange(false),
  errlog(),
  echolog(),
  settings(main->get_settings()),
  is_calculating(false),
  main(main)
{
    gcode = new GCode();
    m_previewGCode = new GCode();
  // Variable defaults
  Center.set(100.,100.,0.);
  preview_shapes.clear();
}

Model::~Model()
{
  ClearLayers();
  ClearGCode();
  delete m_previewLayer;
  preview_shapes.clear();
}

void Model::alert (const char *message)
{
    emit alert(QString(message));
}

void Model::error (const char *message, const char *secondary)
{
    emit alert(QString(message) + QString(secondary));
}

//void Model::SaveConfig(QString filename)
//{
//  settings->save_settings(filename);
//}

void Model::SimpleAdvancedToggle()
{
  cout << _("not yet implemented\n");
}

void Model::SetViewProgress (ViewProgress *progress)
{
  m_progress = progress;
}

void Model::ClearGCode()
{
  m_previewGCode->clear();
  m_previewGCode_z = -100000;
//  delete m_previewGCode;
//  m_previewGCode = nullptr;
  gcode->clear();
//  delete gcode;
//  gcode = nullptr;
  statusbar->clearMessage();
}

void Model::ClearLayers()
{
    for (Layer *l : layers){
        l->Clear();
        delete l;
    }
  layers.clear();
  ClearPreview();
}

void Model::ClearPreview()
{
    if (m_previewGCode) {
        m_previewGCode->clear();
    }
    if (m_previewLayer){
        m_previewLayer->Clear();
    }
  m_previewGCode_z = -100000;
  vector<Shape *> shapes = objectList.get_all_shapes();
  for (Shape* s : shapes)
      s->clearGlList();
}

void Model::GlDrawGCode(int layerno)
{
  if (settings->get_boolean("Display/DisplayGCode", false))  {
    gcode->draw (settings, layerno, false);
  }
  // currently printing:
  // assume that the real printing line is the one at the start of the buffer
  if (currentprintingcommand > 0) {
    int currentlayer = gcode->getLayerNo(currentprintingcommand);
    if (currentlayer>=0) {
      ulong start = gcode->getLayerStart(uint(currentlayer));
      ulong end   = gcode->getLayerEnd(uint(currentlayer));
      //gcode->draw (settings, currentlayer, true, 1);
      bool displaygcodeborders = settings->get_boolean("Display/DisplayGCodeBorders");
      gcode->drawCommands(settings, start, currentprintingcommand, true, 4, false,
             displaygcodeborders);
      gcode->drawCommands(settings, currentprintingcommand,  end,  true, 1, false,
             displaygcodeborders);
    }
    // gcode->drawCommands(settings, currentprintingline-currentbufferedlines,
    // 		       currentprintingline, false, 3, true,
    // 		       settings->Display.DisplayGCodeBorders);
  }
}

void Model::GlDrawGCode(double layerz)
{
  if (!settings->get_boolean("Display/DisplayGCode")) return;
  int layer = gcode->getLayerNo(layerz);
  if (layer>=0)
    GlDrawGCode(layer);
}

void Model::WriteGCode(QFile *file)
{
  QString contents = gcode->get_text(settings, m_progress);
  QTextStream fstream(file);
  file->open(QIODevice::WriteOnly);
  fstream << contents << endl;
  file->close();
  settings->GCodePath =  QFileInfo(*file).dir().absolutePath();
}

/*
void Model::ReadSVG(QFile *file)
{
  if (is_calculating) return;
  if (is_printing) return;
  bool autoplace = settings->get_boolean("Misc/ShapeAutoplace");
  QString path = QFileInfo(*file).absolutePath();
  FlatShape * svgshape = new FlatShape(path);
  cerr << svgshape->info() << endl;
  AddShape(nullptr, dynamic_cast<Shape*>(svgshape), path, autoplace);
  ModelChanged(true);
}
*/

vector<Shape*> Model::ReadShapes(QFile *file,
                                 uint max_triangles)
{
  vector<Shape*> shapes;
  if (!file) return shapes;
  File sfile(file);
  vector< vector<Triangle> > triangles;
  vector<QString> shapenames;
  sfile.loadTriangles(triangles, shapenames, max_triangles);
  for (uint i = 0; i < triangles.size(); i++) {
    if (triangles[i].size() > 0) {
      Shape *shape = new Shape();
      shape->setTriangles(triangles[i]);
      shape->filename = shapenames[i];
      shape->FitToVolume(settings->getPrintVolume() - 2.*settings->getPrintMargin());
      shapes.push_back(shape);
    }
  }
  preview_shapes.clear();
  cerr << shapes.size() << " shapes"<< endl;
  ModelChanged(true);
  return shapes;
}

void Model::ReadStl(QFile *file)
{
  vector<Shape*> shapes = ReadShapes(file, 0);
  // do not autoplace in multishape files
  bool autoplace = settings->get_boolean("Misc/ShapeAutoplace")
          && shapes.size() == 1;
  // add all shapes to the same new ListObject
  ListObject *listObj = objectList.newObject(file->fileName());
  for (Shape *s : shapes){
      AddShape(listObj, s, s->filename, autoplace);
  }
  shapes.clear();
  ModelChanged(true);
}

void Model::SaveStl(QFile *file)
{
  vector<Shape*> shapes = objectList.get_all_shapes();

  if(shapes.size() == 1) {
    shapes[0]->saveBinarySTL(QFileInfo(*file).absoluteFilePath());
  }
  else {
    if (settings->get_boolean("Misc/SaveSingleShapeSTL")) {
      Shape single = GetCombinedShape();
      single.saveBinarySTL(QFileInfo(*file).absoluteFilePath());
    } else {
//      set_locales("C");
      stringstream sstr;
      if (file->open(QFile::WriteOnly | QFile::Truncate)) {
          QTextStream fstream(file);
          for(uint s=0; s < shapes.size(); s++) {
              fstream << QString::fromStdString(shapes[s]->getSTLsolid()) << endl;
          }
          file->close();
      }
//      reset_locales();
    }
  }
  settings->STLPath = QFileInfo(*file).absolutePath();
}

// everything in one shape
Shape Model::GetCombinedShape() const
{
  Shape shape;
  for (uint o = 0; o<objectList.objects.size(); o++) {
    for (uint s = 0; s<objectList.objects[o]->shapes.size(); s++) {
      vector<Triangle> tr = objectList.objects[o]->shapes[s]
              ->getTriangles(objectList.objects[o]->transform3D.getTransform());
      shape.addTriangles(tr);
    }
  }
  return shape;
}

#if ENABLE_AMF
void Model::SaveAMF(QFile *file)
{
  vector<Shape*> shapes;
  vector<Matrix4d> transforms;
  objects.get_all_shapes(shapes,transforms);
  vector< vector<Triangle> > triangles;
  vector<QString> names;
  for(uint s = 0; s < shapes.size(); s++) {
    triangles.push_back(shapes[s]->getTriangles(transforms[s]));
    names.push_back(shapes[s]->filename);
  }
  File::save_AMF(QFileInfo(*file).absolutePath(), triangles, names);
}
#endif

void Model::Read(QFile *file)
{
    QFileInfo finfo(*file);
    QString extn = finfo.suffix().toLower();
//    cerr << "extension " << extn.toUtf8().constData()<<  endl;
    QString directory_path = finfo.absoluteDir().path();
    /*if (extn == "svg")
      {
    ReadSVG (file);
    settings->STLPath = directory_path;
    return;
      }
    else */
    if (extn == "rfo")
      {
    //      ReadRFO (file);
    settings->STLPath = directory_path;
    return;
      }
    else if (extn == "stl")
    {
        ReadStl (file);
        settings->STLPath = directory_path;
    }
    else if (extn == "obj")
    {
        ReadStl (file);
        settings->STLPath = directory_path;
    }
    CalcBoundingBoxAndCenter();
}

void Model::ReadGCode(QTextDocument *doc, QFile *file)
{
  if (is_calculating) return;
  is_calculating=true;
  settings->setValue("Display/DisplayGCode",true);
  m_progress->start (_("Reading GCode"), 100.0);
  gcode->Read (doc, m_progress, QFileInfo(*file).absoluteFilePath().toStdString());
  m_progress->stop (_("Done"));
  is_calculating=false;
  Max = gcode->Max;
  Min = gcode->Min;
  Center = (Max + Min) / 2.0;
 // ModelChanged();
//  m_signal_zoom.emit();
}


void Model::translateGCode(Vector3d trans)
{
  if (is_calculating) return;
  is_calculating=true;
  gcode->translate(trans);

  gcode->MakeText (settings, m_progress);
  Max = gcode->Max;
  Min = gcode->Min;
  Center = (Max + Min) / 2.0;
  is_calculating=false;
}

void Model::ModelChanged(bool objectsAddedOrRemoved)
{
  //printer.update_temp_poll_interval(); // necessary?
    ClearLayers();
    ClearGCode();
    CalcBoundingBoxAndCenter();
    ClearPreview();
    setCurrentPrintingCommand(0);
    emit model_changed(objectsAddedOrRemoved ? &objectList : nullptr);
}

static bool ClosestToOrigin (const Vector3d &a, const Vector3d &b)
{
  return (a.squared_length()) < (b.squared_length());
}

// rearrange unselected shapes in random sequence
bool Model::AutoArrange(const QModelIndexList *selected, bool unselected)
{
  // all shapes
  vector<Shape*>   allshapes = objectList.get_all_shapes();
  // selected shapes
  vector<Shape*>   selshapes = objectList.get_selected_shapes(selected);
  if (selshapes.empty())
      selshapes = allshapes;
  // unselected shapes
  vector<Shape*> unselshapes;
  for(Shape * ashape : allshapes) {
      bool issel = false;
      for(const Shape * sshape : selshapes)
          if (sshape == ashape) {
              issel = true; break;
          }
      if (!issel) {
          unselshapes.push_back(ashape);
      }
  }

  vector<Shape*> shapesToArrange = unselected ? unselshapes : selshapes;
  vector<Shape*> finished = unselected ? selshapes : unselshapes;

  // find place for shapes
  ulong num = shapesToArrange.size();
  vector<uint> rand_seq(num,1); // 1,1,1,...
  partial_sum(rand_seq.begin(), rand_seq.end(), rand_seq.begin()); // 1,2,3,...,N

  srandom(QDateTime::currentMSecsSinceEpoch());
  random_shuffle(rand_seq.begin(), rand_seq.end()); // shuffle

  for(ulong s = 0; s < num; s++) {
    uint index = rand_seq[s]-1;
    shapesToArrange[index]->moveLowerLeftTo(Vector3d::ZERO);
    const Vector3d diag = shapesToArrange[index]->Max - shapesToArrange[index]->Min;
    const Vector3d location = FindEmptyLocation(finished, Vector2d(diag.x(), diag.y()));
    shapesToArrange[index]->moveLowerLeftTo(location);
    finished.push_back(shapesToArrange[index]);
  }
  ModelChanged();
  return true;
}

Vector3d Model::FindEmptyLocation(const vector<Shape*> &fixedshapes,
                                  const Vector2d &diag)
{
  if (fixedshapes.empty()) return Vector3d::ZERO;

  double d = 5.0; // 5mm spacing between objects

  vector<Vector2d> candidates;
  candidates.push_back(Vector2d::ZERO);
  for (const Shape * sh : fixedshapes)
  {
    candidates.push_back(Vector2d(sh->Max.x() + d, sh->Min.y()));
    candidates.push_back(Vector2d(sh->Min.x(), sh->Max.y() + d));
    candidates.push_back(Vector2d(sh->Max.x() + d, sh->Max.y() + d));
  }
  // Prefer positions closest to origin
  sort(candidates.begin(), candidates.end(), ClosestToOrigin);

  Vector2d result;
  // Check all candidates for collisions with existing objects
  Vector3d plate = settings->getPrintVolume() - 2.*settings->getPrintMargin();
  for (const Vector2d &candidate : candidates) {
      // exceeds volume boundary?
      if (candidate.x() + diag.x() > plate.x()
              || candidate.y() + diag.y() > plate.y()) {
          continue;
      }
      bool ok = true;
      // collision of candidate with other shapes
      for (const Shape * sh : fixedshapes) {
          if (sh->inRectangle(candidate, candidate + diag)) {
              ok = false;
              break;
          }
      }
      if (ok)
          return Vector3d(candidate.x(), candidate.y(), 0.);
  }
  // no empty spots
  return Vector3d(plate/2.,0.);
}

bool Model::FindEmptyLocation(Vector3d &result, const Shape *shape)
{
  vector<Shape*> allshapes = objectList.get_all_shapes();
  result = FindEmptyLocation(allshapes, Vector2d(shape->Max-shape->Min));
  return true;
}

ListObject * Model::AddShape(ListObject *parentLO, Shape *shape,
                             QString filename, bool autoplace)
{
  if (m_inhibit_modelchange) return nullptr;
//  FlatShape* flatshape = dynamic_cast<FlatShape*>(shape);
//  if (flatshape != nullptr)
//      shape = flatshape;

  if (parentLO == nullptr) {
       parentLO = objectList.newObject(_("Unnamed Object"));
  }
  if (autoplace){
      // Decide where it's going
      Vector3d trans;
      if (FindEmptyLocation(trans, shape)) {
          shape->moveLowerLeftTo(trans);
      }
  }
  // Add it to the parent LO
  cerr << "adding shape " << filename.toStdString() << endl;
  parentLO->addShape(shape, filename);
  return parentLO;
}

ulong Model::SplitShape(ListObject *parent, Shape *shape, QString filename)
{
  if (m_inhibit_modelchange) return 0;
  vector<Shape*> splitshapes;
  shape->splitshapes(splitshapes, m_progress);
  if (splitshapes.size()<2) return splitshapes.size();
  for (uint s = 0; s <  splitshapes.size(); s++) {
      QString sf;
      QTextStream (&sf) << filename << "_" << (s+1) ;
      AddShape(parent, splitshapes[s], sf, false);
  }
  ModelChanged(true);
  return splitshapes.size();
}

int Model::MergeShapes(ListObject *parent, const vector<Shape*> shapes)
{
  if (m_inhibit_modelchange) return 0;
  Shape * shape = new Shape();
  for (uint s = 0; s <  shapes.size(); s++) {
    vector<Triangle> str = shapes[s]->getTriangles();
    shape->addTriangles(str);
  }
  AddShape(parent, shape, "merged", true);
  ModelChanged(true);
  return 1;
}

int Model::DivideShapeAtZ(ListObject *parent, Shape *shape, QString filename)
{
  if (m_inhibit_modelchange) return 0;
  Shape *upper = new Shape();
  Shape *lower = new Shape();
  Matrix4d T = Matrix4d::IDENTITY; //FIXME! objtree.GetSTLTransformationMatrix(parent);
  int num = shape->divideAtZ(0, upper, lower, T);
  if (num<2) return num;
  else if (num==2) {
    AddShape(parent, upper, filename+_("_upper") ,false);
    AddShape(parent, lower, filename+_("_lower") ,false);
  }
  ModelChanged(true);
  return num;
}

ListObject *Model::newObject()
{
    return objectList.newObject(_("Unnamed Object"));
}

/* Scales the object on changes of the scale slider */
void Model::ScaleObject(Shape *shape, ListObject *object, double scale)
{
  if (m_inhibit_modelchange) return;
  if (shape)
    shape->Scale(scale);
  else if(object)
    //    for (uint s = 0;s<object->shapes.size(); s++) {
      //double fact = object->shapes[s].getScaleFactor();
      object->transform3D.setScale(scale);
  //}
  else return;
  ModelChanged();
}
void Model::ScaleObjectX(Shape *shape, ListObject *object, double scale)
{
  if (m_inhibit_modelchange) return;
  if (shape)
    shape->ScaleX(scale);
  else if(object)
    for (uint s = 0;s<object->shapes.size(); s++) {
      //double fact = object->shapes[s].getScaleFactor();
      object->shapes[s]->ScaleX(scale);
    }
  else return;
  ModelChanged();
}
void Model::ScaleObjectY(Shape *shape, ListObject *object, double scale)
{
  if (m_inhibit_modelchange) return;
  if (shape)
    shape->ScaleY(scale);
  else if(object)
    for (uint s = 0;s<object->shapes.size(); s++) {
      //double fact = object->shapes[s].getScaleFactor();
      object->shapes[s]->ScaleY(scale);
    }
  else return;
  ModelChanged();
}
void Model::ScaleObjectZ(Shape *shape, ListObject *object, double scale)
{
  if (m_inhibit_modelchange) return;
  if (shape)
    shape->ScaleZ(scale);
  else if(object)
    for (uint s = 0;s<object->shapes.size(); s++) {
      //      double fact = object->shapes[s].getScaleFactorZ();
      object->shapes[s]->ScaleZ(scale);
    }
  else return;
  ModelChanged();
}

void Model::rotate_selection(QModelIndexList * selected, const Vector3d axis,
                             double angle)
{
    if (m_inhibit_modelchange) return;
    vector<Shape*> selshapes = objectList.get_selected_shapes(selected);
    for (Shape * shape: selshapes) {
        shape->Rotate(axis, angle);
    }
    if (selshapes.size() == 1){
        main->showTransforms(selshapes[0]);
    }
    ModelChanged();
}

void Model::move_selection(QModelIndexList *selected, const Vector3d move)
{
    if (m_inhibit_modelchange) return;
    vector<Shape*> selshapes = objectList.get_selected_shapes(selected);
    for (Shape * shape: selshapes){
        shape->move(move);
    }
    if (selshapes.size() == 1){
        main->showTransforms(selshapes[0]);
    }
    ModelChanged();
}

void Model::RotateObject(Shape* shape, ListObject* object, Vector4d rotate)
{
  if (m_inhibit_modelchange) return;
  if (!shape)
    return;
  Vector3d rot(rotate.x(), rotate.y(), rotate.z());
  shape->Rotate(rot, rotate.w());
  ModelChanged();
}

void Model::TwistObject(Shape *shape, ListObject *object, double angle)
{
  if (m_inhibit_modelchange) return;
  if (!shape)
    return;
  shape->Twist(angle);
  ModelChanged();
}

void Model::OptimizeRotation(Shape *shape, ListObject *object)
{
  if (m_inhibit_modelchange) return;
  if (!shape)
    return; // FIXME: rotate entire Objects ...
  shape->OptimizeRotation();
  ModelChanged();
}

void Model::InvertNormals(const QModelIndexList * selection)
{
    if (m_inhibit_modelchange) return;
    vector<Shape*>   selshapes = objectList.get_selected_shapes(selection);
    for (Shape * shape: selshapes)
        shape->invertNormals();
    ModelChanged();
}
void Model::Mirror(const QModelIndexList * selection)
{
    if (m_inhibit_modelchange) return;
    vector<Shape*>   selshapes = objectList.get_selected_shapes(selection);
    for (Shape * shape: selshapes)
        shape->mirror();
    ModelChanged();
}

void Model::Hollow(const QModelIndexList *selection)
{
    if (m_inhibit_modelchange) return;
    vector<Shape*>   selshapes = objectList.get_selected_shapes(selection);
    for (Shape * shape: selshapes) {
        shape->makeHollow(3);
        shape->PlaceOnPlatform();
    }
    ModelChanged();
}

void Model::Duplicate(const QModelIndexList *selection)
{
    if (m_inhibit_modelchange) return;
    vector<Shape*>   selshapes = objectList.get_selected_shapes(selection);
    for (Shape * shape: selshapes) {
        Shape * newshape;
//        FlatShape* flatshape = dynamic_cast<FlatShape*>(shape);
//        if (flatshape != NULL)
//          newshape = new FlatShape(*flatshape);
//        else
        newshape = new Shape(*shape);
        ListObject * parent = objectList.getParent(shape);
        if (parent){
            AddShape(parent, newshape, shape->filename);
        }
    }
    ModelChanged(true);
}

void Model::Split(const QModelIndexList *selection)
{
    if (m_inhibit_modelchange) return;
    vector<Shape*>   selshapes = objectList.get_selected_shapes(selection);
    for (Shape * shape: selshapes) {
        ListObject* parent = objectList.getParent(shape);
        if (parent)
            if (SplitShape (parent, shape, shape->filename) > 1) {
                // delete shape?
            }
    }
    ModelChanged(true);
}

void Model::Merge(const QModelIndexList *selection)
{
    if (m_inhibit_modelchange) return;
    vector<Shape*>   selshapes = objectList.get_selected_shapes(selection);
    if (selshapes.size()>0) {
        ListObject* parent = objectList.getParent(selshapes[0]);
        if (parent)
            MergeShapes(parent, selshapes);
    }
    ModelChanged(true);
}

void Model::DivideAtZ(const QModelIndexList *selection)
{
    if (m_inhibit_modelchange) return;
    vector<Shape*>   selshapes = objectList.get_selected_shapes(selection);
    for (Shape * shape: selshapes) {
        ListObject* parent = objectList.getParent(shape);
        if (parent)
            DivideShapeAtZ(parent, shape, shape->filename);
    }
    ModelChanged(true);
}

void Model::PlaceOnPlatform(const QModelIndexList *selected)
{
    if (m_inhibit_modelchange) return;
    vector<Shape*>   selshapes = objectList.get_selected_shapes(selected);
    if (selshapes.empty()) selshapes = objectList.get_all_shapes();
    for (Shape * shape: selshapes){
        shape->PlaceOnPlatform();
    }
//    Transform3D * transf = &object->transform3D;
//    transf->move(Vector3f(0, 0, -transf->getTranslation().z()));
//    for (uint s = 0;s<object->shapes.size(); s++) {
//      object->shapes[s]->PlaceOnPlatform();
//    }
    ModelChanged(true);
}

void Model::AutoRotate(const QModelIndexList *selected)
{
    if (m_inhibit_modelchange) return;
    vector<Shape*>   selshapes = objectList.get_selected_shapes(selected);
    for (Shape * shape: selshapes){
        OptimizeRotation(shape,nullptr);
    }
    //  m_model->OptimizeRotation(NULL, objects[i]);
//  update_scale_value();
//  update_rot_value();
}


void Model::DeleteSelectedObjects(const QModelIndexList *selected)
{
    if (m_inhibit_modelchange) return;
    vector<int> ind;
    for (QModelIndex i : *selected)
        ind.push_back(i.row());
    std::sort(ind.rbegin(), ind.rend());
    for (int i : ind) {
        objectList.DeleteRow(i);
    }
    ClearGCode();
    ClearLayers();
    ModelChanged(true);
}


void Model::ClearLogs()
{
  errlog.setString(nullptr);
  echolog.setString(nullptr);
}

void Model::CalcBoundingBoxAndCenter(bool selected_only)
{
  Vector3d newMax = Vector3d(-INFTY, -INFTY, -INFTY);
  Vector3d newMin = Vector3d(INFTY, INFTY, INFTY);

  vector<Shape*> shapes = selected_only
          ? objectList.get_selected_shapes(main->getSelectedIndexes())
          : objectList.get_all_shapes();

  for (uint s = 0 ; s < shapes.size(); s++) {
    shapes[s]->CalcBBox();
    Vector3d stlMin = shapes[s]->Min;
    Vector3d stlMax = shapes[s]->Max;
    for (uint k = 0; k < 3; k++) {
      newMin[k] = min(stlMin[k], newMin[k]);
      newMax[k] = max(stlMax[k], newMax[k]);
    }
  }

  // for (uint i = 0 ; i < objtree.Objects.size(); i++) {
  //   Matrix4d M = objtree.getTransformationMatrix (i);
  //   for (uint j = 0; j < objtree.Objects[i]->shapes.size(); j++) {
  //     objtree.Objects[i]->shapes[j]->CalcBBox();
  //     Vector3d stlMin = M * objtree.Objects[i]->shapes[j]->Min;
  //     Vector3d stlMax = M * objtree.Objects[i]->shapes[j]->Max;
  //     for (uint k = 0; k < 3; k++) {
  // 	newMin[k] = MIN(stlMin[k], newMin[k]);
  // 	newMax[k] = MAX(stlMax[k], newMax[k]);
  //     }
  //   }
  // }

  if (newMin.x() > newMax.x()) {
    // Show the whole platform if there's no objects
    Min = Vector3d(0,0,0);
    Vector3d pM = settings->getPrintMargin();
    Max = settings->getPrintVolume() - pM - pM;
    Max.z() = 0;
  }
  else {
    Max = newMax;
    Min = newMin;
  }

  Center = (Max + Min) / 2.0;
}

Vector3d Model::GetViewCenter()
{
  Vector3d printOffset = settings->getPrintMargin();
  if(settings->get_boolean("Raft/Enable")){
    const double rsize = settings->get_double("Raft/Size");
    printOffset += Vector3d(rsize, rsize, 0);
  }
  return printOffset + Center;
}

int Model::draw (const QModelIndexList *selected, bool select_mode)
{
    vector<int> selectedshapes;
    if (!select_mode && selected && !selected->isEmpty())
        for(int i = 0; i < selected->size(); i++) {
            selectedshapes.push_back((*selected)[i].row());
        }
  uint select_index = 0; // pick/select index. matches computation in update_model()

  Render *render = main->get_render();

  Vector3d printOffset = settings->getPrintMargin();
  if(settings->get_boolean("Raft/Enable")) {
    const double rsize = settings->get_double("Raft/Size");
    printOffset += Vector3d(rsize, rsize, 0);
  }
  Vector3d translation = objectList.transform3D.getTranslation();
  Vector3d offset = printOffset + translation;

  // Add the print offset to the drawing location of the STL objects.
  glTranslated(offset.x(),offset.y(),offset.z());

  glPushMatrix();
  glMultMatrixd (&objectList.transform3D.getTransform().array[0]);

  // draw preview shapes and nothing else
  if (!select_mode && preview_shapes.size() > 0
          && settings->get_boolean("Display/PreviewLoad")) {
      Vector3d v_center = GetViewCenter() - offset;
      glTranslated( v_center.x(), v_center.y(), v_center.z());
      for (uint i = 0; i < preview_shapes.size(); i++) {
          offset = -preview_shapes[i]->Center;
          glTranslated(offset.x(), offset.y(), -preview_shapes[i]->Min.z());
          // glPushMatrix();
          // glMultMatrixd (&preview_shapes[i]->transform3D.getTransform().array[0]);
          preview_shapes[i]->draw (settings, false, 20000);
          preview_shapes[i]->drawBBox (render);
          // glPopMatrix();
      }
      glPopMatrix();
      glPopMatrix();
      return 0;
    }
  bool displaypolygons = !select_mode && settings->get_boolean("Display/DisplayPolygons");
  bool displaybbox = !select_mode && settings->get_boolean("Display/DisplayBBox");
  int shapeindex=0;
//  cerr << "drawing "<< objectList.objects.size() << " objects"<< endl;
  for (uint i = 0; i < objectList.objects.size(); i++) {
      ListObject *object = objectList.objects[i];

      glPushMatrix();
      glMultMatrixd (object->transform3D.getTransform().array);
      for (uint j = 0; j < object->shapes.size(); j++) {
          select_index++; // only count inner shapes
          Shape *shape = object->shapes[j];
          glPushMatrix();
          glMultMatrixd (&shape->transform3D.getTransform().array[0]);

          bool is_selected = !select_mode &&
                  (std::find(selectedshapes.begin(),selectedshapes.end(),
                             shapeindex) != selectedshapes.end());

          // this is slow for big shapes
          if (is_selected) {
              if (!shape->slow_drawing && shape->dimensions()>2) {
                  // Enable stencil buffer when we draw the selected object.
                  glEnable(GL_STENCIL_TEST);
                  glStencilFunc(GL_ALWAYS, 1, 1);
                  glStencilOp(GL_KEEP, GL_REPLACE, GL_REPLACE);

                  shape->draw (settings);

                  if (!displaypolygons) {
                      // If not drawing polygons, need to draw the geometry
                      // manually, but invisible, to set up the stencil buffer
                      glEnable(GL_CULL_FACE);
                      glEnable(GL_DEPTH_TEST);
                      glEnable(GL_BLEND);
                      // Set to not draw anything, and not update depth buffer
                      glDepthMask(GL_FALSE);
                      glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

                      shape->draw_geometry();

                      glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
                      glDepthMask(GL_TRUE);
                  }

                  // draw highlight around selected object
                  glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
                  glLineWidth(3.0);
                  glEnable (GL_POLYGON_OFFSET_LINE);

                  glDisable (GL_CULL_FACE);
                  glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
                  glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
                  glStencilFunc(GL_NOTEQUAL, 1, 1);
                  glEnable(GL_DEPTH_TEST);

                  shape->draw_geometry();

                  glEnable (GL_CULL_FACE);
                  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
                  glDisable(GL_STENCIL_TEST);
                  glDisable(GL_POLYGON_OFFSET_LINE);
              }
              else shape->draw (settings, true);
          }
          else {
              if (select_mode)
                  shape->draw (settings, false, 0, int(select_index));
              else
                  shape->draw (settings, false);
          }
          glPopMatrix();
          if(displaybbox)
              shape->drawBBox(render);
          shapeindex++;
      }
      glPopMatrix();
  }
  glPopMatrix();
  glLoadName(0); // Clear selection name to avoid selecting last object with later rendering.

  // draw total bounding box
  if(displaybbox && Max.z() > 0)
    {
      const double minz = max(0., Min.z()); // above xy plane only
      // Draw bbox
      glDisable(GL_DEPTH_TEST);
      glLineWidth(1);
      glColor3f(1,0,0);
      glBegin(GL_LINE_LOOP);
      glVertex3d(Min.x(), Min.y(), minz);
      glVertex3d(Min.x(), Max.y(), minz);
      glVertex3d(Max.x(), Max.y(), minz);
      glVertex3d(Max.x(), Min.y(), minz);
      glEnd();
      glBegin(GL_LINE_LOOP);
      glVertex3d(Min.x(), Min.y(), Max.z());
      glVertex3d(Min.x(), Max.y(), Max.z());
      glVertex3d(Max.x(), Max.y(), Max.z());
      glVertex3d(Max.x(), Min.y(), Max.z());
      glEnd();
      glBegin(GL_LINES);
      glVertex3d(Min.x(), Min.y(), minz);
      glVertex3d(Min.x(), Min.y(), Max.z());
      glVertex3d(Min.x(), Max.y(), minz);
      glVertex3d(Min.x(), Max.y(), Max.z());
      glVertex3d(Max.x(), Max.y(), minz);
      glVertex3d(Max.x(), Max.y(), Max.z());
      glVertex3d(Max.x(), Min.y(), minz);
      glVertex3d(Max.x(), Min.y(), Max.z());
      glEnd();
      glColor3f(1.f,0.6f,0.6f);
      ostringstream val;
      val.precision(1);
      Vector3d pos;
      val << fixed << (Max.x()-Min.x());
      pos = Vector3d((Max.x()+Min.x())/2.,Min.y(),Max.z());
      render->draw_string(pos,val.str());
      val.str("");
      val << fixed << (Max.y()-Min.y());
      pos = Vector3d(Min.x(),(Max.y()+Min.y())/2.,Max.z());
      render->draw_string(pos,val.str());
      val.str("");
      val << fixed << (Max.z()-minz);
      pos = Vector3d(Min.x(),Min.y(),(Max.z()+minz)/2.);
      render->draw_string(pos,val.str());
    }
  int drawnlayer = -1;
  if(!select_mode && settings->get_boolean("Display/DisplayLayer")) {
       double z = settings->get_integer("Display/LayerValue")/1000.;
       drawnlayer = drawLayers(z, offset, false);
  }
  if(!select_mode && settings->get_boolean("Display/DisplayGCode") && gcode->size() == 0) {
      // preview gcode if not calculated yet
      if ( m_previewGCode->size() != 0 ||
           ( layers.size() == 0 && gcode->commands.size() == 0 ) ) {
          Vector3d start(0,0,0);
          const double thickness = settings->get_double("Slicing/LayerThickness");
          if (thickness > 0) {
              const double z = settings->get_integer("Display/GCodeDrawStart")/1000. + thickness/2.;
              const uint LayerNo = uint(z/thickness);
              const uint numLayers = uint(Max.z()/thickness);
              if (z != m_previewGCode_z) {
                  //uint prevext = settings->selectedExtruder;
                  Layer * previewGCodeLayer = calcSingleLayer(z, LayerNo, thickness,
                                                              true, true, numLayers);
                  if (previewGCodeLayer) {
                      m_previewGCode->clear();
                      //      vector<Command> commands;
                      GCodeState state(*m_previewGCode);
                      previewGCodeLayer->MakeGCode(start, state, 0, settings);
                      // state.AppendCommands(commands, settings->Slicing.RelativeEcode);
                      m_previewGCode_z = z;
                      delete previewGCodeLayer;
                  }
                  //settings->SelectExtruder(prevext);
              }
              glDisable(GL_DEPTH_TEST);
              m_previewGCode->drawCommands(
                          settings, 1, m_previewGCode->commands.size(), true, 2,
                          settings->get_boolean("Display/DisplayGCodeArrows"),
                          settings->get_boolean("Display/DisplayGCodeBorders"));
          }
      }
  }
  return drawnlayer;
}

// if single layer returns layerno of drawn layer
// else returns -1
int Model::drawLayers(double height, const Vector3d &offset, bool calconly)
{
 if (is_calculating) return -1; // infill calculation (saved patterns) would be disturbed

  glDisable(GL_DEPTH_TEST);
  int drawn = -1;
  size_t LayerNr;

  bool have_layers = (layers.size() > 0); // have sliced already

  bool fillAreas = settings->get_boolean("Display/DisplayFilledAreas");

  double minZ = 0.;//max(0.0, Min.z());
  double z;
  double zStep = settings->get_double("Slicing/LayerThickness");
  double zSize = (Max.z() - minZ - zStep*0.5);
  size_t LayerCount = size_t(ceil((zSize - zStep*0.5)/zStep)-1);
  double sel_Z = height; //*zSize;
  uint sel_Layer;
  if (have_layers)
    sel_Layer = uint(floor(height*(layers.size())/zSize));
  else
    sel_Layer = uint(LayerCount*sel_Z/zSize);
  LayerCount = sel_Layer+1;
  if(have_layers && settings->get_boolean("Display/DisplayAllLayers"))
    {
      LayerNr = 0;
      z=minZ;
      // don't fill areas if multiple layers
      settings->setValue("Display/DisplayFilledAreas",false);
    }
  else
    {
      LayerNr = sel_Layer;
      z= minZ + sel_Z;
    }
  if (have_layers) {
    LayerNr    = min(LayerNr,  layers.size() - 1);
    LayerCount = min(LayerCount, layers.size());
  }
  z = min(max(z, 0.), Max.z());
  z += 0.5*zStep; // always cut in middle of layer

  //cerr << zStep << ";"<<Max.z()<<";"<<Min.z()<<";"<<zSize<<";"<<LayerNr<<";"<<LayerCount<<";"<<endl;

  Layer* layer=NULL;
  if (have_layers)
    glTranslated(-offset.x(), -offset.y(), -offset.z());

  const double lthickness = settings->get_double("Slicing/LayerThickness");
  bool displayinfill = settings->get_boolean("Display/DisplayInfill");
  bool drawrulers = settings->get_boolean("Display/DrawRulers");
  while(LayerNr < LayerCount)
    {
      if (have_layers)
    {
      layer = layers[LayerNr];
      z = layer->getZ();
      drawn = layer->LayerNo;
    }
      else
    {
      if (!m_previewLayer || m_previewLayer->getZ() != z) {
          delete m_previewLayer;
          m_previewLayer = calcSingleLayer(z, LayerNr, lthickness,
                                           displayinfill, false);
        layer = m_previewLayer;
        Layer * previous = NULL;
//        if (LayerNr>0 && z >= lthickness)
//          previous = calcSingleLayer(z-lthickness, LayerNr-1, lthickness,
//                     false, false);
        layer->setPrevious(previous);
      }
      layer = m_previewLayer;
    }
      if (!calconly) {
    layer->Draw(*settings, main->get_render());

    if (drawrulers)
      layer->DrawRulers(measuresPoint, main->get_render());
      }

      // if (!have_layers)
      // {
      //       // need to delete the temporary  layer
      //       delete layer;
      // }
      LayerNr++;
      z+=zStep;
    }// while

  settings->setValue("Display/DisplayFilledAreas", fillAreas); // set to value before
  return drawn;
}

Layer * Model::calcSingleLayer(double z, uint LayerNr, double thickness,
                               bool calcinfill, bool for_gcode, ulong numLayers)
{
  if (is_calculating) return NULL; // infill calculation (saved patterns) would be disturbed
  is_calculating = true;
  if (!for_gcode) {
    if (m_previewLayer && m_previewLayer->getZ() == z
    && m_previewLayer->thickness == thickness) return m_previewLayer;
  }
  vector<Shape*> shapes;
  if (settings->get_boolean("Slicing/SelectedOnly"))
      shapes = objectList.get_selected_shapes(main->getSelectedIndexes());
  else
      shapes = objectList.get_all_shapes();

  double max_grad = 0;
  double supportangle = settings->get_boolean("Slicing/Support") ?
              supportangle = settings->get_double("Slicing/SupportAngle")*M_PI/180.
          : -1;

  Layer * layer = new Layer(NULL, int(LayerNr), thickness,
                            uint(settings->get_integer("Slicing/Skins")));
  layer->setZ(z);

  for(size_t f = 0; f < shapes.size(); f++) {
      if (shapes[f])
          layer->addShape(Matrix4d::IDENTITY, *shapes[f], max_grad, supportangle);
  }
#if 0
  // show layer triangulation
   vector<Poly> polys = layer->GetPolygons();
   for (size_t i=0; i<polys.size();i++){
     vector<Triangle> tri;
     polys[i].getTriangulation(tri);
     for (size_t j=0; j<tri.size();j++){
       tri[j].draw(GL_LINE_LOOP);
     }
   }
#endif
  uint extruder = 0;
  layer->MakeShells(*settings, extruder);

  if (settings->get_boolean("Slicing/Skirt")) {
    if (layer->getZ() - layer->thickness <= settings->get_double("Slicing/SkirtHeight"))
        if (!settings->get_boolean("Slicing/Support"))
            layer->MakeSkirt(settings->get_double("Slicing/SkirtDistance"),
                             settings->get_boolean("Slicing/SingleSkirt"));
  }
  uint firstLayers = uint(settings->get_integer("Slicing/FirstLayersNum"));
  uint altinfill = uint(settings->get_integer("Slicing/AltInfillLayers"));
  uint shellcount = uint(settings->get_integer("Slicing/ShellCount"));
  bool nobottom = uint(settings->get_integer("Slicing/NoBottom"));
  bool nocovers = uint(settings->get_integer("Slicing/NoTopAndBottom"));

  bool fullInfill = (!nobottom && !nocovers && LayerNr < shellcount)
          || (!nocovers && LayerNr > numLayers - shellcount);

  InfillSet infills(*settings, layer->getMin(), layer->getMax());
  if (calcinfill)
    layer->CalcInfill(*settings, infills, fullInfill ,
                      altinfill && LayerNr % altinfill == 0,
                      LayerNr < firstLayers);

#define DEBUGPOLYS 0
#if DEBUGPOLYS
  // write out polygons for gnuplot
  vector<Poly> polys = layer->GetPolygons();
  vector< vector<Poly> > offs = layer->GetShellPolygons();
  cout << "# polygons "<< endl;
  for (size_t i=0; i<polys.size();i++){
    cout << polys[i].gnuplot_path() << endl;
  }
  for (size_t s=0; s<offs.size();s++){
    cout << "# offset polygons " << s << endl;
    for (size_t i=0; i<offs[s].size();i++){
      cout << offs[s][i].gnuplot_path() << endl;
    }
  }
#endif

  is_calculating = false;
  return layer;
}


double Model::get_previewLayer_Z()
{
    if (!settings->get_boolean("Display/DisplayLayer")) return -1.;
  return settings->get_double("Display/LayerValue",0.)/1000.;
}

bool Model::haveGCode() const
{
    return gcode && gcode->commands.size()>0;
}

void Model::setMeasuresPoint(const Vector2d &point)
{
  measuresPoint = (point - settings->getPrintMargin().get_sub_vector<2>(0));
}
