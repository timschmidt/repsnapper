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
#include "flatshape.h"
#include "render.h"

Model::Model(MainWindow *main) :
  m_previewLayer(nullptr),
  //m_previewGCodeLayer(NULL),
  currentprintingline(0),
  Min(), Max(),
  m_inhibit_modelchange(false),
  errlog(),
  echolog(),
  settings(main->get_settings()),
  is_calculating(false),
  is_printing(false),
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
//  signal_alert.emit (Gtk::MESSAGE_INFO, message, NULL);
}

void Model::error (const char *message, const char *secondary)
{
//  signal_alert.emit (Gtk::MESSAGE_ERROR, message, secondary);
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
//  delete m_previewGCode;
  m_previewGCode_z = -100000;
  gcode->clear();
//  delete gcode;
}

void Model::ClearLayers()
{
  for(vector<Layer *>::iterator i=layers.begin(); i != layers.end(); i++) {
    if ((*i)) (*i)->Clear();
    delete *i;
  }
  layers.clear();
  Infill::clearPatterns();
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
}

QTextDocument *Model::GetGCodeBuffer()
{
  return &gcode->buffer;
}

void Model::GlDrawGCode(int layerno)
{
  if (settings->get_boolean("Display/DisplayGCode"))  {
    gcode->draw (settings, layerno, false);
  }
  // assume that the real printing line is the one at the start of the buffer
  if (currentprintingline > 0) {
    int currentlayer = gcode->getLayerNo(currentprintingline);
    if (currentlayer>=0) {
      int start = gcode->getLayerStart(currentlayer);
      int end   = gcode->getLayerEnd(currentlayer);
      //gcode->draw (settings, currentlayer, true, 1);
      bool displaygcodeborders = settings->get_boolean("Display/DisplayGCodeBorders");
      gcode->drawCommands(settings, start, currentprintingline, true, 4, false,
             displaygcodeborders);
      gcode->drawCommands(settings, currentprintingline,  end,  true, 1, false,
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
  QString contents = gcode->get_text();
  QTextStream fstream(file);
  file->open(QIODevice::WriteOnly);
  fstream << contents << endl;
  file->close();
  settings->GCodePath =  QFileInfo(*file).dir().absolutePath();
}

void Model::ReadSVG(QFile *file)
{
  if (is_calculating) return;
  if (is_printing) return;
  bool autoplace = settings->get_boolean("Misc/ShapeAutoplace");
  QString path = QFileInfo(*file).absolutePath();
  FlatShape * svgshape = new FlatShape(path);
  cerr << svgshape->info() << endl;
  AddShape(nullptr, (Shape*)svgshape, path, autoplace);
  ClearLayers();
  emit model_changed(&objectList);
}


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
  cerr << shapes.size() << " shapes"<< endl;
  emit model_changed(&objectList);
  return shapes;
}


void Model::ReadStl(QFile *file)
{
  vector<Shape*> shapes = ReadShapes(file, 0);
  // do not autoplace in multishape files
  bool autoplace = settings->get_boolean("Misc/ShapeAutoplace")
          && shapes.size() == 1;
  for (Shape *s : shapes){
    AddShape(NULL, s, s->filename, autoplace);
  }
  shapes.clear();
  emit model_changed(&objectList);
}

void Model::SaveStl(QFile *file)
{
  vector<Shape*> shapes;
  vector<Matrix4d> transforms;
  objectList.get_all_shapes(shapes,transforms);

  if(shapes.size() == 1) {
    shapes[0]->saveBinarySTL(QFileInfo(*file).absolutePath());
  }
  else {
    if (settings->get_boolean("Misc/SaveSingleShapeSTL")) {
      Shape single = GetCombinedShape();
      single.saveBinarySTL(QFileInfo(*file).absolutePath());
    } else {
//      set_locales("C");
      stringstream sstr;
      QTextStream fstream(file);
      for(uint s=0; s < shapes.size(); s++) {
          fstream << QString::fromStdString(shapes[s]->getSTLsolid()) << endl;
      }
      file->close();
//      reset_locales();
    }
  }
  settings->STLPath = QFileInfo(*file).dir().path().toUtf8().constData();
}

// everything in one shape
Shape Model::GetCombinedShape() const
{
  Shape shape;
  for (uint o = 0; o<objectList.objects.size(); o++) {
    for (uint s = 0; s<objectList.objects[o]->shapes.size(); s++) {
      vector<Triangle> tr = objectList.objects[o]->shapes[s]
              ->getTriangles(objectList.objects[o]->transform3D.transform);
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
    if (extn == "gcode")
      {
    ReadGCode (file);
    settings->GCodePath = directory_path;
    return;
      }
    else if (extn == "svg")
      {
    ReadSVG (file);
    settings->STLPath = directory_path;
    return;
      }
    else if (extn == "rfo")
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
    cerr << objectList.info() << endl;
}

void Model::ReadGCode(QFile *file)
{
  if (is_calculating) return;
  if (is_printing) return;
  is_calculating=true;
  settings->setValue("Display/DisplayGCode",true);
  m_progress->start (_("Reading GCode"), 100.0);
  gcode->Read (this, settings->get_extruder_letters(), m_progress,
              QFileInfo(*file).absoluteFilePath().toUtf8().constData());
  m_progress->stop (_("Done"));
  is_calculating=false;
  Max = gcode->Max;
  Min = gcode->Min;
  Center = (Max + Min) / 2.0;
  emit model_changed(nullptr);
//  m_signal_zoom.emit();
}


void Model::translateGCode(Vector3d trans)
{
  if (is_calculating) return;
  if (is_printing) return;
  is_calculating=true;
  gcode->translate(trans);

  QString GcodeTxt;
  gcode->MakeText (GcodeTxt, settings, m_progress);
  Max = gcode->Max;
  Min = gcode->Min;
  Center = (Max + Min) / 2.0;
  is_calculating=false;
}



void Model::ModelChanged()
{
  if (m_inhibit_modelchange) return;
  if (objectList.empty()) return;
  //printer.update_temp_poll_interval(); // necessary?
  if (!is_printing) {
    CalcBoundingBoxAndCenter();
    Infill::clearPatterns();
    if ( layers.size()>0 || m_previewGCode->size()>0 || m_previewLayer ) {
      ClearGCode();
      ClearLayers();
    }
    setCurrentPrintingLine(0);
    gcode->emit gcode_changed();
    emit model_changed(&objectList);
  }
}

static bool ClosestToOrigin (Vector3d a, Vector3d b)
{
  return (a.squared_length()) < (b.squared_length());
}

// rearrange unselected shapes in random sequence
bool Model::AutoArrange(QModelIndexList *selected)
{
  // all shapes
  vector<Shape*>   allshapes;
  vector<Matrix4d> transforms;
  objectList.get_all_shapes(allshapes, transforms);

  // selected shapes
  vector<Shape*>   selshapes;
  vector<Matrix4d> seltransforms;
  objectList.get_selected_shapes(selected, selshapes, seltransforms);

  // get unselected shapes
  vector<Shape*>   unselshapes;
  vector<Matrix4d> unseltransforms;

  for(uint s=0; s < allshapes.size(); s++) {
    bool issel = false;
    for(uint ss=0; ss < selshapes.size(); ss++)
      if (selshapes[ss] == allshapes[s]) {
    issel = true; break;
      }
    if (!issel) {
      unselshapes.    push_back(allshapes[s]);
      unseltransforms.push_back(transforms[s]);
    }
  }

  // find place for unselected shapes
  int num = unselshapes.size();
  vector<int> rand_seq(num,1); // 1,1,1...
  partial_sum(rand_seq.begin(), rand_seq.end(), rand_seq.begin()); // 1,2,3,...,N

  srandom(QDateTime::currentMSecsSinceEpoch());
  random_shuffle(rand_seq.begin(), rand_seq.end()); // shuffle

  for(int s=0; s < num; s++) {
    uint index = rand_seq[s]-1;
    // use selshapes as vector to fill up
    Vector3d trans = FindEmptyLocation(selshapes, seltransforms, unselshapes[index]);
    selshapes.push_back(unselshapes[index]);
    seltransforms.push_back(unseltransforms[index]); // basic transform, not shape
    selshapes.back()->transform3D.move(trans);
    CalcBoundingBoxAndCenter();
  }
  ModelChanged();
  return true;
}

Vector3d Model::FindEmptyLocation(const vector<Shape*> &shapes,
                  const vector<Matrix4d> &transforms,
                  const Shape *shape)
{
  // Get all object positions
  std::vector<Vector3d> maxpos;
  std::vector<Vector3d> minpos;
  for(uint s=0; s<shapes.size(); s++) {
    Vector3d p;
    Matrix4d strans = transforms[s];
    Vector3d min = strans * shapes[s]->Min;
    Vector3d max = strans * shapes[s]->Max;
    minpos.push_back(Vector3d(min.x(), min.y(), 0));
    maxpos.push_back(Vector3d(max.x(), max.y(), 0));
  }

  double d = 5.0; // 5mm spacing between objects
  Vector3d StlDelta = (shape->Max - shape->Min);
  vector<Vector3d> candidates;

  candidates.push_back(Vector3d(0.0, 0.0, 0.0));

  for (uint j=0; j<maxpos.size(); j++)
  {
    candidates.push_back(Vector3d(maxpos[j].x() + d, minpos[j].y(), 0));
    candidates.push_back(Vector3d(minpos[j].x(), maxpos[j].y() + d, 0));
    candidates.push_back(maxpos[j] + Vector3d(d,d,0));
  }

  // Prefer positions closest to origin
  sort(candidates.begin(), candidates.end(), ClosestToOrigin);


  Vector3d result;
  // Check all candidates for collisions with existing objects
  for (uint c=0; c<candidates.size(); c++)
  {
    bool ok = true;

    for (uint k=0; k<maxpos.size(); k++)
    {
      if (
          // check x
          ( ( ( minpos[k].x()     <= candidates[c].x() &&
        candidates[c].x() <= maxpos[k].x() ) ||
          ( candidates[c].x() <= minpos[k].x() &&
        maxpos[k].x() <= candidates[c].x()+StlDelta.x()+d ) ) ||
        ( ( minpos[k].x() <= candidates[c].x()+StlDelta.x()+d &&
        candidates[c].x()+StlDelta.x()+d <= maxpos[k].x() ) ) )
          &&
          // check y
          ( ( ( minpos[k].y() <= candidates[c].y() &&
        candidates[c].y() <= maxpos[k].y() ) ||
          ( candidates[c].y() <= minpos[k].y() &&
        maxpos[k].y() <= candidates[c].y()+StlDelta.y()+d ) ) ||
        ( ( minpos[k].y() <= candidates[c].y()+StlDelta.y()+d &&
        candidates[c].y()+StlDelta.y()+d <= maxpos[k].y() ) ) )
      )
    {
      ok = false;
      break;
    }

      // volume boundary
      if (candidates[c].x()+StlDelta.x() >
      (settings->getPrintVolume().x() - 2*settings->getPrintMargin().x())
      || candidates[c].y()+StlDelta.y() >
      (settings->getPrintVolume().y() - 2*settings->getPrintMargin().y()))
    {
      ok = false;
      break;
    }
    }
    if (ok) {
      result.x() = candidates[c].x();
      result.y() = candidates[c].y();
      result.z() = candidates[c].z();
      // keep z
      result.x() -= shape->Min.x();
      result.y() -= shape->Min.y();
      return result;
    }
  }

  // no empty spots
  return Vector3d(100,100,0);
}

bool Model::FindEmptyLocation(Vector3d &result, const Shape *shape)
{
  // Get all object positions
  std::vector<Vector3d> maxpos;
  std::vector<Vector3d> minpos;

  vector<Shape*>   allshapes;
  vector<Matrix4d> transforms;
  objectList.get_all_shapes(allshapes, transforms);
  result = FindEmptyLocation(allshapes, transforms, shape);
  return true;
}

int Model::AddShape(ListObject *parentLO, Shape *shape, QString filename, bool autoplace)
{
  //Shape *retshape;
  bool found_location=false;

  FlatShape* flatshape = dynamic_cast<FlatShape*>(shape);
  if (flatshape != nullptr)
      shape = flatshape;

  if (parentLO == nullptr) {
       parentLO = objectList.newObject(_("Unnamed Object"));
  }
  if (autoplace){
      // Decide where it's going
      Vector3d trans;
      if (FindEmptyLocation(trans, shape)) {
          shape->transform3D.move(trans);
      }
      shape->PlaceOnPlatform();
  }
  // Add it to the parent LO
  cerr << "adding shape " << filename.toStdString() << endl;
  parentLO->addShape(shape, filename);
    // Tell everyone
//  m_signal_stl_added.emit (path);

  return 0;
}

int Model::SplitShape(ListObject *parent, Shape *shape, QString filename)
{
  vector<Shape*> splitshapes;
  shape->splitshapes(splitshapes, m_progress);
  if (splitshapes.size()<2) return splitshapes.size();
  for (uint s = 0; s <  splitshapes.size(); s++) {
      QString sf;
      QTextStream (&sf) << filename << "_" << (s+1) ;
      AddShape(parent, splitshapes[s], sf, false);
  }
  emit model_changed(&objectList);
  return splitshapes.size();
}

int Model::MergeShapes(ListObject *parent, const vector<Shape*> shapes)
{
  Shape * shape = new Shape();
  for (uint s = 0; s <  shapes.size(); s++) {
    vector<Triangle> str = shapes[s]->getTriangles();
    shape->addTriangles(str);
  }
  AddShape(parent, shape, "merged", true);
  emit model_changed(&objectList);
  return 1;
}

int Model::DivideShape(ListObject *parent, Shape *shape, QString filename)
{
  Shape *upper = new Shape();
  Shape *lower = new Shape();
  Matrix4d T = Matrix4d::IDENTITY;;//FIXME! objtree.GetSTLTransformationMatrix(parent);
  int num = shape->divideAtZ(0, upper, lower, T);
  if (num<2) return num;
  else if (num==2) {
    AddShape(parent, upper, filename+_("_upper") ,false);
    AddShape(parent, lower, filename+_("_lower") ,false);
  }
  emit model_changed(&objectList);
  return num;
}

ListObject *Model::newObject()
{
    return objectList.newObject(_("Unnamed Object"));
}

/* Scales the object on changes of the scale slider */
void Model::ScaleObject(Shape *shape, ListObject *object, double scale)
{
  if (shape)
    shape->Scale(scale);
  else if(object)
    //    for (uint s = 0;s<object->shapes.size(); s++) {
      //double fact = object->shapes[s].getScaleFactor();
      object->transform3D.scale(scale);
  //}
  else return;
  ModelChanged();
}
void Model::ScaleObjectX(Shape *shape, ListObject *object, double scale)
{
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

void Model::RotateObject(Shape* shape, ListObject* object, Vector4d rotate)
{
  if (!shape)
    return;
  Vector3d rot(rotate.x(), rotate.y(), rotate.z());
  shape->Rotate(rot, rotate.w());
  ModelChanged();
}

void Model::TwistObject(Shape *shape, ListObject *object, double angle)
{
  if (!shape)
    return;
  shape->Twist(angle);
  ModelChanged();
}

void Model::OptimizeRotation(Shape *shape, ListObject *object)
{
  if (!shape)
    return; // FIXME: rotate entire Objects ...
  shape->OptimizeRotation();
  ModelChanged();
}

void Model::InvertNormals(Shape *shape, ListObject *object)
{
  if (shape)
    shape->invertNormals();
  else // if (object) object->invertNormals();
    return;
  ModelChanged();
}
void Model::Mirror(Shape *shape, ListObject *object)
{
  if (shape)
    shape->mirror();
  else // if (object) object->mirror();
    return;
  ModelChanged();
}

void Model::PlaceOnPlatform(Shape *shape, ListObject *object)
{
  if (shape)
    shape->PlaceOnPlatform();
  else if(object) {
    Transform3D * transf = &object->transform3D;
    transf->move(Vector3f(0, 0, -transf->getTranslation().z()));
    for (uint s = 0;s<object->shapes.size(); s++) {
      object->shapes[s]->PlaceOnPlatform();
    }
  }
  else return;
  ModelChanged();
}

void Model::DeleteSelectedObjects(QModelIndexList *selected)
{
    for (QModelIndex i : *selected)
        objectList.DeleteSelected(&i);
    ClearGCode();
    ClearLayers();
    ModelChanged();
}


void Model::ClearLogs()
{
  errlog.setString(nullptr);
  echolog.setString(nullptr);
}

void Model::CalcBoundingBoxAndCenter(bool selected_only)
{
  Vector3d newMax = Vector3d(G_MINDOUBLE, G_MINDOUBLE, G_MINDOUBLE);
  Vector3d newMin = Vector3d(G_MAXDOUBLE, G_MAXDOUBLE, G_MAXDOUBLE);

  vector<Shape*> shapes;
  vector<Matrix4d> transforms;
  if (selected_only)
    objectList.get_selected_shapes(&m_current_selectionpath, shapes, transforms);
  else
    objectList.get_all_shapes(shapes, transforms);

  for (uint s = 0 ; s < shapes.size(); s++) {
    shapes[s]->CalcBBox();
    Vector3d stlMin = transforms[s] * shapes[s]->Min;
    Vector3d stlMax = transforms[s] * shapes[s]->Max;
    for (uint k = 0; k < 3; k++) {
      newMin[k] = MIN(stlMin[k], newMin[k]);
      newMax[k] = MAX(stlMax[k], newMax[k]);
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
//  m_signal_zoom.emit();
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

int Model::draw (const QModelIndexList *selected)
{
    vector<int> selectedshapes;
    if (selected && selected->size()>0)
        for(int i = 0; i < selected->size(); i++) {
            selectedshapes.push_back((*selected)[i].row());
        }
  gint index = 1; // pick/select index. matches computation in update_model()

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
  glMultMatrixd (&objectList.transform3D.transform.array[0]);

  // draw preview shapes and nothing else
  if (preview_shapes.size() > 0
          && settings->get_boolean("Display/PreviewLoad")) {
      Vector3d v_center = GetViewCenter() - offset;
      glTranslated( v_center.x(), v_center.y(), v_center.z());
      for (uint i = 0; i < preview_shapes.size(); i++) {
    offset = preview_shapes[i]->t_Center();
    glTranslated(offset.x(), offset.y(), offset.z());
    // glPushMatrix();
    // glMultMatrixd (&preview_shapes[i]->transform3D.transform.array[0]);
    preview_shapes[i]->draw (settings, false, 20000);
    preview_shapes[i]->drawBBox (render);
    // glPopMatrix();
      }
      glPopMatrix();
      glPopMatrix();
      return 0;
    }
  bool support = settings->get_boolean("Slicing/Support");
  double supportangle = support ? settings->get_double("Slicing/SupportAngle")
                                : 0.;
  bool displaypolygons = settings->get_boolean("Display/DisplayPolygons");
  bool displaybbox = settings->get_boolean("Display/DisplayBBox");
  gint shapeindex=0;
//  cerr << "drawing "<< objectList.objects.size() << " objects"<< endl;
  for (uint i = 0; i < objectList.objects.size(); i++) {
      ListObject *object = objectList.objects[i];
//      index++; // only count inner shapes

      glPushMatrix();
      glMultMatrixd (object->transform3D.transform.array);
      for (uint j = 0; j < object->shapes.size(); j++) {
          Shape *shape = object->shapes[j];
          glLoadName(index); // Load select/pick index
          index++;
          glPushMatrix();
          glMultMatrixd (&shape->transform3D.transform.array[0]);

          bool is_selected = (std::find(selectedshapes.begin(),selectedshapes.end(),
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
              shape->draw (settings, false);
          }
          // draw support triangles
          if (support) {
              glColor4f(0.8f,0.f,0.f,0.5f);
              vector<Triangle> suppTr =
                      shape->trianglesSteeperThan(supportangle*M_PI/180.);
              for (uint i=0; i < suppTr.size(); i++)
                  suppTr[i].draw(GL_TRIANGLES);
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
  if(displaybbox)
    {
      const float minz = max(0., Min.z()); // above xy plane only
      // Draw bbox
      glDisable(GL_DEPTH_TEST);
      glLineWidth(1);
      glColor3f(1,0,0);
      glBegin(GL_LINE_LOOP);
      glVertex3f(Min.x(), Min.y(), minz);
      glVertex3f(Min.x(), Max.y(), minz);
      glVertex3f(Max.x(), Max.y(), minz);
      glVertex3f(Max.x(), Min.y(), minz);
      glEnd();
      glBegin(GL_LINE_LOOP);
      glVertex3f(Min.x(), Min.y(), Max.z());
      glVertex3f(Min.x(), Max.y(), Max.z());
      glVertex3f(Max.x(), Max.y(), Max.z());
      glVertex3f(Max.x(), Min.y(), Max.z());
      glEnd();
      glBegin(GL_LINES);
      glVertex3f(Min.x(), Min.y(), minz);
      glVertex3f(Min.x(), Min.y(), Max.z());
      glVertex3f(Min.x(), Max.y(), minz);
      glVertex3f(Min.x(), Max.y(), Max.z());
      glVertex3f(Max.x(), Max.y(), minz);
      glVertex3f(Max.x(), Max.y(), Max.z());
      glVertex3f(Max.x(), Min.y(), minz);
      glVertex3f(Max.x(), Min.y(), Max.z());
      glEnd();
      glColor3f(1.f,0.6f,0.6f);
      ostringstream val;
      val.precision(1);
      Vector3f pos;
      val << fixed << (Max.x()-Min.x());
      pos = Vector3f((Max.x()+Min.x())/2.f,Min.y(),Max.z());
      render->draw_string(pos,val.str());
      val.str("");
      val << fixed << (Max.y()-Min.y());
      pos = Vector3f(Min.x(),(Max.y()+Min.y())/2.f,Max.z());
      render->draw_string(pos,val.str());
      val.str("");
      val << fixed << (Max.z()-minz);
      pos = Vector3f(Min.x(),Min.y(),(Max.z()+minz)/2.f);
      render->draw_string(pos,val.str());
    }
  int drawnlayer = -1;
  if(settings->get_boolean("Display/DisplayLayer")) {
       float z = Max.z() * settings->get_slider_fraction("Display/LayerValue");
       drawnlayer = drawLayers(z, offset, false);
  }
  if(settings->get_boolean("Display/DisplayGCode") && gcode->size() == 0) {
    // preview gcode if not calculated yet
    if ( m_previewGCode->size() != 0 ||
     ( layers.size() == 0 && gcode->commands.size() == 0 ) ) {
      Vector3d start(0,0,0);
      const double thickness = settings->get_double("Slicing/LayerThickness");
      const double zFrac =settings->get_slider_fraction("Display/GCodeDrawStart");
      const int LayerCount = int(ceil(Max.z()/thickness))-1;
      const uint LayerNo = uint(ceil(zFrac * (LayerCount-1)));
      const double z = LayerNo * thickness;
      if (z != m_previewGCode_z) {
    //uint prevext = settings->selectedExtruder;
    Layer * previewGCodeLayer = calcSingleLayer(z, LayerNo, thickness, true, true);
    if (previewGCodeLayer) {
      m_previewGCode->clear();
      vector<Command> commands;
      GCodeState state(*m_previewGCode);
      previewGCodeLayer->MakeGCode(start, state, 0, settings);
      // state.AppendCommands(commands, settings->Slicing.RelativeEcode);
      m_previewGCode_z = z;
    }
    //settings->SelectExtruder(prevext);
      }
      glDisable(GL_DEPTH_TEST);
      m_previewGCode->drawCommands(settings, 1, m_previewGCode->commands.size(), true, 2,
                  settings->get_boolean("Display/DisplayGCodeArrows"),
                  settings->get_boolean("Display/DisplayGCodeBorders"));
    }
  }
  return drawnlayer;
}

// if single layer returns layerno of drawn layer
// else returns -1
int Model::drawLayers(float height, const Vector3d &offset, bool calconly)
{
 if (is_calculating) return -1; // infill calculation (saved patterns) would be disturbed

  glDisable(GL_DEPTH_TEST);
  int drawn = -1;
  int LayerNr;

  bool have_layers = (layers.size() > 0); // have sliced already

  bool fillAreas = settings->get_boolean("Display/DisplayFilledAreas");

  float minZ = 0.f;//max(0.0, Min.z());
  float z;
  float zStep = settings->get_double("Slicing/LayerThickness");
  float zSize = (Max.z() - minZ - zStep*0.5f);
  int LayerCount = int(ceil((zSize - zStep*0.5f)/zStep))-1;
  float sel_Z = height; //*zSize;
  uint sel_Layer;
  if (have_layers)
    sel_Layer = (uint)floor(height*(layers.size())/zSize);
  else
    sel_Layer = (uint)ceil(LayerCount*sel_Z/zSize);
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
    LayerNr = CLAMP(LayerNr, 0, (int)layers.size() - 1);
    LayerCount = CLAMP(LayerCount, 0, (int)layers.size());
  }
  z = CLAMP(z, 0.f, Max.z());
  z += 0.5f*zStep; // always cut in middle of layer

  //cerr << zStep << ";"<<Max.z()<<";"<<Min.z()<<";"<<zSize<<";"<<LayerNr<<";"<<LayerCount<<";"<<endl;

  Layer* layer=NULL;
  if (have_layers)
    glTranslatef(-offset.x(), -offset.y(), -offset.z());

  const float lthickness = settings->get_double("Slicing/LayerThickness");
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
        m_previewLayer = calcSingleLayer(z, LayerNr, lthickness,
                         displayinfill, false);
        layer = m_previewLayer;
        Layer * previous = NULL;
        if (LayerNr>0 && z >= lthickness)
          previous = calcSingleLayer(z-lthickness, LayerNr-1, lthickness,
                     false, false);
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
                               bool calcinfill, bool for_gcode)
{
  if (is_calculating) return NULL; // infill calculation (saved patterns) would be disturbed
  if (!for_gcode) {
    if (m_previewLayer && m_previewLayer->getZ() == z
    && m_previewLayer->thickness == thickness) return m_previewLayer;
  }
  vector<Shape*> shapes;
  vector<Matrix4d> transforms;

  if (settings->value("Slicing/SelectedOnly").toBool())
    objectList.get_selected_shapes(&m_current_selectionpath, shapes, transforms);
  else
    objectList.get_all_shapes(shapes, transforms);

  double max_grad = 0;
  double supportangle = settings->value("Slicing/SupportAngle").toDouble()*M_PI/180.;
  if (!settings->value("Slicing/Support").toBool()) supportangle = -1;

  Layer * layer = new Layer(NULL, LayerNr, thickness,
                settings->value("Slicing/Skins").toInt());
  layer->setZ(z);
  for(size_t f = 0; f < shapes.size(); f++) {
    layer->addShape(transforms[f], *shapes[f], z, max_grad, supportangle);
  }

  // vector<Poly> polys = layer->GetPolygons();
  // for (guint i=0; i<polys.size();i++){
  //   vector<Triangle> tri;
  //   polys[i].getTriangulation(tri);
  //   for (guint j=0; j<tri.size();j++){
  //     tri[j].draw(GL_LINE_LOOP);
  //   }
  // }

  layer->MakeShells(*settings);

  if (settings->value("Slicing/Skirt").toBool()) {
    if (layer->getZ() - layer->thickness <= settings->get_double("Slicing/SkirtHeight"))
      layer->MakeSkirt(settings->get_double("Slicing/SkirtDistance"),
               settings->get_boolean("Slicing/SingleSkirt") &&
               !settings->get_boolean("Slicing/Support"));
  }

  if (calcinfill)
    layer->CalcInfill(*settings);

#define DEBUGPOLYS 0
#if DEBUGPOLYS
  // write out polygons for gnuplot
  vector<Poly> polys = layer->GetPolygons();
  vector< vector<Poly> > offs = layer->GetShellPolygons();
  cout << "# polygons "<< endl;
  for (guint i=0; i<polys.size();i++){
    cout << polys[i].gnuplot_path() << endl;
  }
  for (guint s=0; s<offs.size();s++){
    cout << "# offset polygons " << s << endl;
    for (guint i=0; i<offs[s].size();i++){
      cout << offs[s][i].gnuplot_path() << endl;
    }
  }
#endif

  return layer;
}


double Model::get_preview_Z()
{
  if (m_previewLayer) return m_previewLayer->getZ();
  return 0;
}

void Model::setMeasuresPoint(const Vector3d &point)
{
  measuresPoint = Vector2d(point.x(), point.y()) ;
}
