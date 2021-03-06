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

#include "stdafx.h"
#include "model.h"
#include "objtree.h"
#include "settings.h"
#include "layer.h"
#include "infill.h"
#include "ui/progress.h"
#include "shape.h"
#include "flatshape.h"
#include "render.h"

Model::Model() :
  m_previewLayer(NULL),
  //m_previewGCodeLayer(NULL),
  currentprintingline(0),
  settings(),
  Min(), Max(),
  m_inhibit_modelchange(false),
  errlog (Gtk::TextBuffer::create()),
  echolog (Gtk::TextBuffer::create()),
  is_calculating(false),
  is_printing(false)
{
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
  signal_alert.emit (Gtk::MESSAGE_INFO, message, NULL);
}

void Model::error (const char *message, const char *secondary)
{
  signal_alert.emit (Gtk::MESSAGE_ERROR, message, secondary);
}

void Model::SaveConfig(Glib::RefPtr<Gio::File> file)
{
  settings.save_settings(file);
}

void Model::LoadConfig(Glib::RefPtr<Gio::File> file)
{
  settings.load_settings(file);
  ModelChanged();
}

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
  m_previewGCode.clear();
  m_previewGCode_z = -100000;
  gcode.clear();
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
  if (m_previewLayer) delete m_previewLayer;
  m_previewLayer = NULL;
  m_previewGCode.clear();
  m_previewGCode_z = -100000;
}

Glib::RefPtr<Gtk::TextBuffer> Model::GetGCodeBuffer()
{
  return gcode.buffer;
}

void Model::GlDrawGCode(int layerno)
{
  if (settings.get_boolean("Display","DisplayGCode"))  {
    gcode.draw (settings, layerno, false);
  }
  // assume that the real printing line is the one at the start of the buffer
  if (currentprintingline > 0) {
    int currentlayer = gcode.getLayerNo(currentprintingline);
    if (currentlayer>=0) {
      int start = gcode.getLayerStart(currentlayer);
      int end   = gcode.getLayerEnd(currentlayer);
      //gcode.draw (settings, currentlayer, true, 1);
      bool displaygcodeborders = settings.get_boolean("Display","DisplayGCodeBorders");
      gcode.drawCommands(settings, start, currentprintingline, true, 4, false,
			 displaygcodeborders);
      gcode.drawCommands(settings, currentprintingline,  end,  true, 1, false,
			 displaygcodeborders);
    }
    // gcode.drawCommands(settings, currentprintingline-currentbufferedlines,
    // 		       currentprintingline, false, 3, true,
    // 		       settings.Display.DisplayGCodeBorders);
  }
}

void Model::GlDrawGCode(double layerz)
{
  if (!settings.get_boolean("Display","DisplayGCode")) return;
  int layer = gcode.getLayerNo(layerz);
  if (layer>=0)
    GlDrawGCode(layer);
}


void Model::init() {}

void Model::WriteGCode(Glib::RefPtr<Gio::File> file)
{
  Glib::ustring contents = gcode.get_text();
  Glib::file_set_contents (file->get_path(), contents);
  settings.GCodePath = file->get_parent()->get_path();
}

void Model::ReadSVG(Glib::RefPtr<Gio::File> file)
{
  if (is_calculating) return;
  if (is_printing) return;
  bool autoplace = settings.get_boolean("Misc","ShapeAutoplace");
  string path = file->get_path();
  FlatShape * svgshape = new FlatShape(path);
  cerr << svgshape->info() << endl;
  AddShape(NULL, svgshape, path, autoplace);
  ClearLayers();
}


vector<Shape*> Model::ReadShapes(Glib::RefPtr<Gio::File> file,
				 uint max_triangles)
{
  vector<Shape*> shapes;
  if (!file) return shapes;
  File sfile(file);
  vector< vector<Triangle> > triangles;
  vector<ustring> shapenames;
  sfile.loadTriangles(triangles, shapenames, max_triangles);
  for (uint i = 0; i < triangles.size(); i++) {
    if (triangles[i].size() > 0) {
      Shape *shape = new Shape();
      shape->setTriangles(triangles[i]);
      shape->filename = shapenames[i];
      shape->FitToVolume(settings.getPrintVolume() - 2.*settings.getPrintMargin());
      shapes.push_back(shape);
    }
  }
  return shapes;
}


void Model::ReadStl(Glib::RefPtr<Gio::File> file)
{
  bool autoplace = settings.get_boolean("Misc","ShapeAutoplace");
  vector<Shape*> shapes = ReadShapes(file, 0);
  // do not autoplace in multishape files
  if (shapes.size() > 1)  autoplace = false;
  for (uint i = 0; i < shapes.size(); i++){
    AddShape(NULL, shapes[i], shapes[i]->filename, autoplace);
  }
  shapes.clear();
  ModelChanged();
}

void Model::SaveStl(Glib::RefPtr<Gio::File> file)
{
  vector<Shape*> shapes;
  vector<Matrix4d> transforms;
  objtree.get_all_shapes(shapes,transforms);

  if(shapes.size() == 1) {
    shapes[0]->saveBinarySTL(file->get_path());
  }
  else {
    if (settings.get_boolean("Misc","SaveSingleShapeSTL")) {
      Shape single = GetCombinedShape();
      single.saveBinarySTL(file->get_path());
    } else {
      set_locales("C");
      stringstream sstr;
      for(uint s=0; s < shapes.size(); s++) {
	sstr << shapes[s]->getSTLsolid() << endl;
      }
      Glib::file_set_contents (file->get_path(), sstr.str());
      reset_locales();
    }
  }
  settings.STLPath = file->get_parent()->get_path();
}

// everything in one shape
Shape Model::GetCombinedShape() const
{
  Shape shape;
  for (uint o = 0; o<objtree.Objects.size(); o++) {
    for (uint s = 0; s<objtree.Objects[o]->shapes.size(); s++) {
      vector<Triangle> tr =
	objtree.Objects[o]->shapes[s]->getTriangles(objtree.Objects[o]->transform3D.transform);
      shape.addTriangles(tr);
    }
  }
  return shape;
}

void Model::SaveAMF(Glib::RefPtr<Gio::File> file)
{
  vector<Shape*> shapes;
  vector<Matrix4d> transforms;
  objtree.get_all_shapes(shapes,transforms);
  vector< vector<Triangle> > triangles;
  vector<ustring> names;
  for(uint s = 0; s < shapes.size(); s++) {
    triangles.push_back(shapes[s]->getTriangles(transforms[s]));
    names.push_back(shapes[s]->filename);
  }
  File::save_AMF(file->get_path(), triangles, names);
}

void Model::Read(Glib::RefPtr<Gio::File> file)
{
  std::string basename = file->get_basename();
  size_t pos = basename.rfind('.');
  cerr << "reading " << basename<< endl;
  string directory_path = file->get_parent()->get_path();
  if (pos != std::string::npos) {
    std::string extn = basename.substr(pos);
    if (extn == ".conf")
      {
	LoadConfig (file);
	settings.SettingsPath = directory_path;
	return;
      }
    else if (extn == ".gcode")
      {
	ReadGCode (file);
	settings.GCodePath = directory_path;
	return;
      }
    else if (extn == ".svg")
      {
	ReadSVG (file);
	settings.STLPath = directory_path;
	return;
      }
    else if (extn == ".rfo")
      {
	//      ReadRFO (file);
	settings.STLPath = directory_path;
	return;
      }
  }
  ReadStl (file);
  settings.STLPath = directory_path;
}

void Model::ReadGCode(Glib::RefPtr<Gio::File> file)
{
  if (is_calculating) return;
  if (is_printing) return;
  is_calculating=true;
  settings.set_boolean("Display","DisplayGCode",true);
  m_progress->start (_("Reading GCode"), 100.0);
  gcode.Read (this, settings.get_extruder_letters(), m_progress, file->get_path());
  m_progress->stop (_("Done"));
  is_calculating=false;
  Max = gcode.Max;
  Min = gcode.Min;
  Center = (Max + Min) / 2.0;
  m_signal_zoom.emit();
}


void Model::translateGCode(Vector3d trans)
{
  if (is_calculating) return;
  if (is_printing) return;
  is_calculating=true;
  gcode.translate(trans);

  string GcodeTxt;
  gcode.MakeText (GcodeTxt, settings, m_progress);
  Max = gcode.Max;
  Min = gcode.Min;
  Center = (Max + Min) / 2.0;
  is_calculating=false;
}



void Model::ModelChanged()
{
  if (m_inhibit_modelchange) return;
  if (objtree.empty()) return;
  //printer.update_temp_poll_interval(); // necessary?
  if (!is_printing) {
    CalcBoundingBoxAndCenter();
    Infill::clearPatterns();
    if ( layers.size()>0 || m_previewGCode.size()>0 || m_previewLayer ) {
      ClearGCode();
      ClearLayers();
    }
    setCurrentPrintingLine(0);
    m_model_changed.emit();
  }
}

static bool ClosestToOrigin (Vector3d a, Vector3d b)
{
  return (a.squared_length()) < (b.squared_length());
}

// rearrange unselected shapes in random sequence
bool Model::AutoArrange(vector<Gtk::TreeModel::Path> &path)
{
  // all shapes
  vector<Shape*>   allshapes;
  vector<Matrix4d> transforms;
  objtree.get_all_shapes(allshapes, transforms);

  // selected shapes
  vector<Shape*>   selshapes;
  vector<Matrix4d> seltransforms;
  objtree.get_selected_shapes(path, selshapes, seltransforms);

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

  Glib::TimeVal timeval;
  timeval.assign_current_time();
  srandom((unsigned long)(timeval.as_double()));
  random_shuffle(rand_seq.begin(), rand_seq.end()); // shuffle

  for(int s=0; s < num; s++) {
    int index = rand_seq[s]-1;
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
	  (settings.getPrintVolume().x() - 2*settings.getPrintMargin().x())
	  || candidates[c].y()+StlDelta.y() >
	  (settings.getPrintVolume().y() - 2*settings.getPrintMargin().y()))
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
  objtree.get_all_shapes(allshapes, transforms);
  result = FindEmptyLocation(allshapes, transforms, shape);
  return true;
}

int Model::AddShape(TreeObject *parent, Shape *shape, string filename, bool autoplace)
{
  //Shape *retshape;
  bool found_location=false;


  FlatShape* flatshape = dynamic_cast<FlatShape*>(shape);
  if (flatshape != NULL)
    shape = flatshape;

  if (!parent) {
    if (objtree.Objects.size() <= 0)
      objtree.newObject();
    parent = objtree.Objects.back();
  }
  g_assert (parent != NULL);

  // Decide where it's going
  Vector3d trans = Vector3d(0,0,0);
  if (autoplace) found_location = FindEmptyLocation(trans, shape);
  // Add it to the set
  size_t found = filename.find_last_of("/\\");
  Gtk::TreePath path = objtree.addShape(parent, shape, filename.substr(found+1));
  Shape *retshape = parent->shapes.back();

  // Move it, if we found a suitable place
  if (found_location) {
    retshape->transform3D.move(trans);
  }

  //if (autoplace) retshape->PlaceOnPlatform();

  // Update the view to include the new object
  ModelChanged();
    // Tell everyone
  m_signal_stl_added.emit (path);

  return 0;
}

int Model::SplitShape(TreeObject *parent, Shape *shape, string filename)
{
  vector<Shape*> splitshapes;
  shape->splitshapes(splitshapes, m_progress);
  if (splitshapes.size()<2) return splitshapes.size();
  for (uint s = 0; s <  splitshapes.size(); s++) {
    ostringstream sfn;
    sfn << filename << "_" << (s+1) ;
    AddShape(parent, splitshapes[s], sfn.str(), false);
  }
  return splitshapes.size();
}

int Model::MergeShapes(TreeObject *parent, const vector<Shape*> shapes)
{
  Shape * shape = new Shape();
  for (uint s = 0; s <  shapes.size(); s++) {
    vector<Triangle> str = shapes[s]->getTriangles();
    shape->addTriangles(str);
  }
  AddShape(parent, shape, "merged", true);
  return 1;
}

int Model::DivideShape(TreeObject *parent, Shape *shape, string filename)
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
  return num;
}

void Model::newObject()
{
  objtree.newObject();
}

/* Scales the object on changes of the scale slider */
void Model::ScaleObject(Shape *shape, TreeObject *object, double scale)
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
void Model::ScaleObjectX(Shape *shape, TreeObject *object, double scale)
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
void Model::ScaleObjectY(Shape *shape, TreeObject *object, double scale)
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
void Model::ScaleObjectZ(Shape *shape, TreeObject *object, double scale)
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

void Model::RotateObject(Shape* shape, TreeObject* object, Vector4d rotate)
{
  if (!shape)
    return;
  Vector3d rot(rotate.x(), rotate.y(), rotate.z());
  shape->Rotate(rot, rotate.w());
  ModelChanged();
}

void Model::TwistObject(Shape *shape, TreeObject *object, double angle)
{
  if (!shape)
    return;
  shape->Twist(angle);
  ModelChanged();
}

void Model::OptimizeRotation(Shape *shape, TreeObject *object)
{
  if (!shape)
    return; // FIXME: rotate entire Objects ...
  shape->OptimizeRotation();
  ModelChanged();
}

void Model::InvertNormals(Shape *shape, TreeObject *object)
{
  if (shape)
    shape->invertNormals();
  else // if (object) object->invertNormals();
    return;
  ModelChanged();
}
void Model::Mirror(Shape *shape, TreeObject *object)
{
  if (shape)
    shape->mirror();
  else // if (object) object->mirror();
    return;
  ModelChanged();
}

void Model::PlaceOnPlatform(Shape *shape, TreeObject *object)
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

void Model::DeleteObjTree(vector<Gtk::TreeModel::Path> &iter)
{
  objtree.DeleteSelected (iter);
  ClearGCode();
  ClearLayers();
  ModelChanged();
}


void Model::ClearLogs()
{
  errlog->set_text("");
  echolog->set_text("");
}

void Model::CalcBoundingBoxAndCenter(bool selected_only)
{
  Vector3d newMax = Vector3d(G_MINDOUBLE, G_MINDOUBLE, G_MINDOUBLE);
  Vector3d newMin = Vector3d(G_MAXDOUBLE, G_MAXDOUBLE, G_MAXDOUBLE);

  vector<Shape*> shapes;
  vector<Matrix4d> transforms;
  if (selected_only)
    objtree.get_selected_shapes(m_current_selectionpath, shapes, transforms);
  else
    objtree.get_all_shapes(shapes, transforms);

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
    Vector3d pM = settings.getPrintMargin();
    Max = settings.getPrintVolume() - pM - pM;
    Max.z() = 0;
  }
  else {
    Max = newMax;
    Min = newMin;
  }

  Center = (Max + Min) / 2.0;
  m_signal_zoom.emit();
}

Vector3d Model::GetViewCenter()
{
  Vector3d printOffset = settings.getPrintMargin();
  if(settings.get_boolean("Raft","Enable")){
    const double rsize = settings.get_double("Raft","Size");
    printOffset += Vector3d(rsize, rsize, 0);
  }
  return printOffset + Center;
}

// called from View::Draw
int Model::draw (vector<Gtk::TreeModel::Path> &iter)
{
  vector<Shape*> sel_shapes;
  vector<Matrix4d> transforms;
  objtree.get_selected_shapes(iter, sel_shapes, transforms);

  gint index = 1; // pick/select index. matches computation in update_model()

  Vector3d printOffset = settings.getPrintMargin();
  if(settings.get_boolean("Raft","Enable")) {
    const double rsize = settings.get_double("Raft","Size");
    printOffset += Vector3d(rsize, rsize, 0);
  }
  Vector3d translation = objtree.transform3D.getTranslation();
  Vector3d offset = printOffset + translation;

  // Add the print offset to the drawing location of the STL objects.
  glTranslated(offset.x(),offset.y(),offset.z());

  glPushMatrix();
  glMultMatrixd (&objtree.transform3D.transform.array[0]);

  // draw preview shapes and nothing else
  if (settings.get_boolean("Display","PreviewLoad"))
    if (preview_shapes.size() > 0) {
      Vector3d v_center = GetViewCenter() - offset;
      glTranslated( v_center.x(), v_center.y(), v_center.z());
      for (uint i = 0; i < preview_shapes.size(); i++) {
	offset = preview_shapes[i]->t_Center();
	glTranslated(offset.x(), offset.y(), offset.z());
	// glPushMatrix();
	// glMultMatrixd (&preview_shapes[i]->transform3D.transform.array[0]);
	preview_shapes[i]->draw (settings, false, 20000);
	preview_shapes[i]->drawBBox ();
	// glPopMatrix();
      }
      glPopMatrix();
      glPopMatrix();
      return 0;
    }
  bool support = settings.get_boolean("Slicing","Support");
  double supportangle = settings.get_double("Slicing","SupportAngle");
  bool displaypolygons = settings.get_boolean("Display","DisplayPolygons");
  bool displaybbox = settings.get_boolean("Display","DisplayBBox");
  for (uint i = 0; i < objtree.Objects.size(); i++) {
    TreeObject *object = objtree.Objects[i];
    index++;

    glPushMatrix();
    glMultMatrixd (&object->transform3D.transform.array[0]);
    for (uint j = 0; j < object->shapes.size(); j++) {
      Shape *shape = object->shapes[j];
      glLoadName(index); // Load select/pick index
      index++;
      glPushMatrix();
      glMultMatrixd (&shape->transform3D.transform.array[0]);

      bool is_selected = false;
      for (uint s = 0; s < sel_shapes.size(); s++)
	if (sel_shapes[s] == shape)
	  is_selected = true;

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
	shape->drawBBox();
     }
    glPopMatrix();
  }
  glPopMatrix();
  glLoadName(0); // Clear selection name to avoid selecting last object with later rendering.

  // draw total bounding box
  if(displaybbox)
    {
      const double minz = max(0., Min.z()); // above xy plane only
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
      glColor3f(1,0.6,0.6);
      ostringstream val;
      val.precision(1);
      Vector3d pos;
      val << fixed << (Max.x()-Min.x());
      pos = Vector3d((Max.x()+Min.x())/2.,Min.y(),Max.z());
      Render::draw_string(pos,val.str());
      val.str("");
      val << fixed << (Max.y()-Min.y());
      pos = Vector3d(Min.x(),(Max.y()+Min.y())/2.,Max.z());
      Render::draw_string(pos,val.str());
      val.str("");
      val << fixed << (Max.z()-minz);
      pos = Vector3d(Min.x(),Min.y(),(Max.z()+minz)/2.);
      Render::draw_string(pos,val.str());
    }
  int drawnlayer = -1;
  if(settings.get_boolean("Display","DisplayLayer")) {
    drawnlayer = drawLayers(settings.get_double("Display","LayerValue"),
			    offset, false);
  }
  if(settings.get_boolean("Display","DisplayGCode") && gcode.size() == 0) {
    // preview gcode if not calculated yet
    if ( m_previewGCode.size() != 0 ||
	 ( layers.size() == 0 && gcode.commands.size() == 0 ) ) {
      Vector3d start(0,0,0);
      const double thickness = settings.get_double("Slicing","LayerThickness");
      const double gcodedrawstart = settings.get_double("Display","GCodeDrawStart");
      const double z = gcodedrawstart + thickness/2;
      const int LayerCount = (int)ceil(Max.z()/thickness)-1;
      const uint LayerNo = (uint)ceil(gcodedrawstart*(LayerCount-1));
      if (z != m_previewGCode_z) {
	//uint prevext = settings.selectedExtruder;
	Layer * previewGCodeLayer = calcSingleLayer(z, LayerNo, thickness, true, true);
	if (previewGCodeLayer) {
	  m_previewGCode.clear();
	  vector<Command> commands;
	  GCodeState state(m_previewGCode);
	  previewGCodeLayer->MakeGCode(start, state, 0, settings);
	  // state.AppendCommands(commands, settings.Slicing.RelativeEcode);
	  m_previewGCode_z = z;
	}
	//settings.SelectExtruder(prevext);
      }
      glDisable(GL_DEPTH_TEST);
      m_previewGCode.drawCommands(settings, 1, m_previewGCode.commands.size(), true, 2,
				  settings.get_boolean("Display","DisplayGCodeArrows"),
				  settings.get_boolean("Display","DisplayGCodeBorders"));
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
  int LayerNr;

 ;

  bool have_layers = (layers.size() > 0); // have sliced already

  bool fillAreas = settings.get_boolean("Display","DisplayFilledAreas");

  double minZ = 0;//max(0.0, Min.z());
  double z;
  double zStep = settings.get_double("Slicing","LayerThickness");
  double zSize = (Max.z() - minZ - zStep*0.5);
  int LayerCount = (int)ceil((zSize - zStep*0.5)/zStep)-1;
  double sel_Z = height; //*zSize;
  uint sel_Layer;
  if (have_layers)
    sel_Layer = (uint)floor(height*(layers.size())/zSize);
  else
    sel_Layer = (uint)ceil(LayerCount*sel_Z/zSize);
  LayerCount = sel_Layer+1;
  if(have_layers && settings.get_boolean("Display","DisplayAllLayers"))
    {
      LayerNr = 0;
      z=minZ;
      // don't fill areas if multiple layers
      settings.set_boolean("Display","DisplayFilledAreas",false);
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
  z = CLAMP(z, 0, Max.z());
  z += 0.5*zStep; // always cut in middle of layer

  //cerr << zStep << ";"<<Max.z()<<";"<<Min.z()<<";"<<zSize<<";"<<LayerNr<<";"<<LayerCount<<";"<<endl;

  Layer* layer=NULL;
  if (have_layers)
    glTranslatef(-offset.x(), -offset.y(), -offset.z());

  const float lthickness = settings.get_double("Slicing","LayerThickness");
  bool displayinfill = settings.get_boolean("Display","DisplayinFill");
  bool drawrulers = settings.get_boolean("Display","DrawRulers");
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
	layer->Draw(settings);

	if (drawrulers)
	  layer->DrawRulers(measuresPoint);
      }

      // if (!have_layers)
      // {
      //       // need to delete the temporary  layer
      //       delete layer;
      // }
      LayerNr++;
      z+=zStep;
    }// while

  settings.set_boolean("Display","DisplayFilledAreas", fillAreas); // set to value before
  return drawn;
}


Layer * Model::calcSingleLayer(double z, uint LayerNr, double thickness,
			       bool calcinfill, bool for_gcode) const
{
  if (is_calculating) return NULL; // infill calculation (saved patterns) would be disturbed
  if (!for_gcode) {
    if (m_previewLayer && m_previewLayer->getZ() == z
	&& m_previewLayer->thickness == thickness) return m_previewLayer;
  }
  vector<Shape*> shapes;
  vector<Matrix4d> transforms;

  if (settings.get_boolean("Slicing","SelectedOnly"))
    objtree.get_selected_shapes(m_current_selectionpath, shapes, transforms);
  else
    objtree.get_all_shapes(shapes, transforms);

  double max_grad = 0;
  double supportangle = settings.get_double("Slicing","SupportAngle")*M_PI/180.;
  if (!settings.get_boolean("Slicing","Support")) supportangle = -1;

  Layer * layer = new Layer(NULL, LayerNr, thickness,
			    settings.get_integer("Slicing","Skins"));
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

  layer->MakeShells(settings);

  if (settings.get_boolean("Slicing","Skirt")) {
    if (layer->getZ() - layer->thickness <= settings.get_double("Slicing","SkirtHeight"))
      layer->MakeSkirt(settings.get_double("Slicing","SkirtDistance"),
		       settings.get_boolean("Slicing","SingleSkirt") &&
		       !settings.get_boolean("Slicing","Support"));
  }

  if (calcinfill)
    layer->CalcInfill(settings);

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
