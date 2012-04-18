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
#include "config.h"
#define  MODEL_IMPLEMENTATION
#include <vector>
#include <string>
#include <cerrno>
#include <functional>
#include <numeric>

#include <glib/gutils.h>
#include <libreprap/comms.h>

#include "stdafx.h"
#include "model.h"
#include "objtree.h"
#include "file.h"
#include "settings.h"
#include "connectview.h"
#include "layer.h"
#include "infill.h"
#include "progress.h"
#include "shape.h"

Model::Model() :
  m_previewLayer(NULL),
  //m_previewGCodeLayer(NULL),
  currentprintingline(0),
  currentbufferedlines(0),
  settings(),
  Min(), Max(),
  errlog (Gtk::TextBuffer::create()),
  echolog (Gtk::TextBuffer::create()),
  is_calculating(false),
  is_printing(false)
{
  // Variable defaults
  Center.set(100.,100.,0.);
}

Model::~Model()
{
  ClearLayers();
  ClearGCode();
  delete m_previewLayer;
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
  for(uint i=0;i<layers.size();i++)
    layers[i]->Clear();
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
  if (settings.Display.DisplayGCode) 
    gcode.draw (settings, layerno, false);
  // assume that the real printing line is the one at the start of the buffer
  unsigned long printedline = currentprintingline - currentbufferedlines;
  if (printedline > 0) {
    int currentlayer = gcode.getLayerNo(printedline);
    if (currentlayer>=0) {
      int start = gcode.getLayerStart(currentlayer);
      int end = gcode.getLayerEnd(currentlayer);
      //gcode.draw (settings, currentlayer, true, 1);
      gcode.drawCommands(settings, start, printedline, true, 4, false, 
			 settings.Display.DisplayGCodeBorders);
      gcode.drawCommands(settings,  printedline, end,  true, 1, false, 
			 settings.Display.DisplayGCodeBorders);
    }
    // gcode.drawCommands(settings, currentprintingline-currentbufferedlines, 
    // 		       currentprintingline, false, 3, true, 
    // 		       settings.Display.DisplayGCodeBorders);
  }
}

void Model::GlDrawGCode(double layerz)
{
  if (!settings.Display.DisplayGCode) return;
  int layer = gcode.getLayerNo(layerz);
  if (layer>=0)
    GlDrawGCode(layer);
}


void Model::init() {}

void Model::WriteGCode(Glib::RefPtr<Gio::File> file)
{
  Glib::ustring contents = gcode.get_text();
  Glib::file_set_contents (file->get_path(), contents);
}

void Model::ReadSVG(Glib::RefPtr<Gio::File> file)
{
  if (is_calculating) return;
  if (is_printing) return;
  bool autoplace = settings.Misc.ShapeAutoplace;
  string path = file->get_path();
  FlatShape * svgshape = new FlatShape(path);
  cerr << svgshape->info() << endl;
  AddShape(NULL, svgshape, path, autoplace);
  ModelChanged();
  ClearLayers();
}



void Model::ReadStl(Glib::RefPtr<Gio::File> file, filetype_t ftype)
{
  bool autoplace = settings.Misc.ShapeAutoplace;
  string path = file->get_path();
  if (ftype==UNKNOWN_TYPE)
    ftype =  Shape::getFileType(path);
  
  if (ftype == BINARY_STL) // only one shape per file
    {
      Shape *shape = new Shape();
      shape->loadBinarySTL(path);      
      AddShape(NULL, shape, path,autoplace);
      ModelChanged();
     }
  else if (ftype == ASCII_STL) // multiple shapes per file
    {
      ifstream fileis;
      fileis.open(path.c_str());
      size_t found = path.find_last_of("/\\");
      vector<Shape*> shapes;
      while (!fileis.eof())
	{
	  Shape *shape = new Shape();
	  int ok = shape->parseASCIISTL(&fileis);
	  if (ok>=0) {
	    shapes.push_back(shape);
	    // go back to get "solid " keyword again
	    streampos where = fileis.tellg();
	    where-=100;
	    if (where < 0) break;
	    fileis.seekg(where,ios::beg);
	  }
	  else if (shapes.size()==0) {
	    cerr <<"Could not read STL in ASCII mode: "<< path 
	   	 << " (bad header?), trying Binary " << endl ;
	    ReadStl(file, BINARY_STL);
	    return;
	  }
	}
      if (shapes.size()==1){
	shapes.front()->filename = (string)path.substr(found+1); 
	AddShape(NULL, shapes.front(), shapes.front()->filename, autoplace);
      }
      else for (uint i=0;i<shapes.size();i++){
	  // do not autoplace to keep saved positions
	  AddShape(NULL, shapes[i], shapes[i]->filename, false);
	}
      shapes.clear();
      ModelChanged();
      fileis.close();
    }
  else if (ftype == VRML) 
    {
      Shape *shape = new Shape();
      shape->loadASCIIVRML(path);      
      AddShape(NULL, shape, path,autoplace);
      ModelChanged();
    }

  ClearLayers();
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
    stringstream sstr;
    for(uint s=0; s < shapes.size(); s++) {
      sstr << shapes[s]->getSTLsolid() << endl;
    }
  // for(uint o=0;o<objtree.Objects.size();o++)
  // {
  //   for(uint f=0;f<objtree.Objects[o]->shapes.size();f++)
  //   {
  //     Shape *shape = objtree.Objects[o]->shapes[f];
  //     sstr << shape->getSTLsolid() << endl;
  //   }
  // }
    Glib::file_set_contents (file->get_path(), sstr.str());
  }
}

void Model::Read(Glib::RefPtr<Gio::File> file)
{
  std::string basename = file->get_basename();
  size_t pos = basename.rfind('.');
  if (pos != std::string::npos) {
    std::string extn = basename.substr(pos);
    if (extn == ".gcode")
      {
	ReadGCode (file);
	return;
      }
    else if (extn == ".svg")
      {
	ReadSVG (file);
	return;
      }
    else if (extn == ".rfo")
      {
	//      ReadRFO (file);
	return;
      }
  }
  ReadStl (file);
}

void Model::ReadGCode(Glib::RefPtr<Gio::File> file)
{
  if (is_calculating) return;
  if (is_printing) return;
  is_calculating=true;
  m_progress->start (_("Reading GCode"), 100.0);
  gcode.Read (this, m_progress, file->get_path());
  m_progress->stop (_("Done"));
  is_calculating=false;
}


void Model::translateGCode(Vector3d trans) 
{
  gcode.translate(trans);
  string GcodeTxt;
  string GcodeStart = settings.GCode.getStartText();
  string GcodeLayer = settings.GCode.getLayerText();
  string GcodeEnd   = settings.GCode.getEndText();
  m_progress->start(_("Moving GCode"),gcode.commands.size());
  gcode.MakeText (GcodeTxt, GcodeStart, GcodeLayer, GcodeEnd,
		  settings.Slicing.RelativeEcode,
		  m_progress);
  m_progress->stop();
}



void Model::ModelChanged()
{
  //printer.update_temp_poll_interval(); // necessary?
  if (!is_printing) {
    CalcBoundingBoxAndCenter();
    Infill::clearPatterns();
    if ( layers.size()>0 || m_previewGCode.size()>0 || m_previewLayer ) {
      ClearGCode();
      ClearLayers();
    }
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
    //Matrix4d strans = (transforms[s] * allshapes[s]->transform3D.transform);
    Vector3d min = transforms[s] * shapes[s]->Min;
    Vector3d max = transforms[s] * shapes[s]->Max;
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
      // if (candidates[c].x()+StlDelta.x() > (settings.Hardware.Volume.x() - 2*settings.Hardware.PrintMargin.x()) ||
      //     candidates[c].y()+StlDelta.y() > (settings.Hardware.Volume.y() - 2*settings.Hardware.PrintMargin.y()))
      // {
      //   ok = false;
      //   break;
      // }
    }
    if (ok)
    {
      result.x() = candidates[c].x();
      result.y() = candidates[c].y();
      result.z() = candidates[c].z();
      result -= shape->Min;
      return result;
    }
  }

  // no empty spots
  return result;
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
  retshape->PlaceOnPlatform();

  // Update the view to include the new object
  CalcBoundingBoxAndCenter();
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
    AddShape(parent, splitshapes[s], sfn.str() ,false);
  }
  return splitshapes.size();
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
  ModelChanged();
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
  CalcBoundingBoxAndCenter();
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
  CalcBoundingBoxAndCenter();
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
  CalcBoundingBoxAndCenter();
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
  CalcBoundingBoxAndCenter();
  ModelChanged();
}

void Model::RotateObject(Shape* shape, TreeObject* object, Vector4d rotate)
{
  if (!shape)
    return; 
  Vector3d rot(rotate.x(), rotate.y(), rotate.z());
  shape->Rotate(rot, rotate.w());
  CalcBoundingBoxAndCenter();
  ModelChanged();
}

void Model::TwistObject(Shape *shape, TreeObject *object, double angle)
{
  if (!shape)
    return; 
  shape->Twist(angle);
  CalcBoundingBoxAndCenter();
  ModelChanged();
}

void Model::OptimizeRotation(Shape *shape, TreeObject *object)
{
  if (!shape)
    return; // FIXME: rotate entire Objects ...
  shape->OptimizeRotation();
  CalcBoundingBoxAndCenter();
  ModelChanged();
}

void Model::InvertNormals(Shape *shape, TreeObject *object)
{
  if (shape)
    shape->invertNormals();
  else // if (object) object->invertNormals();
    return; 
  //CalcBoundingBoxAndCenter();
  ModelChanged();
}
void Model::Mirror(Shape *shape, TreeObject *object)
{
  if (shape)
    shape->mirror();
  else // if (object) object->mirror();
    return; 
  CalcBoundingBoxAndCenter();
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
  CalcBoundingBoxAndCenter();
  ModelChanged();
}

void Model::DeleteObjTree(vector<Gtk::TreeModel::Path> &iter)
{
  objtree.DeleteSelected (iter);
  ClearGCode();
  ClearLayers();
  CalcBoundingBoxAndCenter();
  ModelChanged();
}


void Model::ClearLogs()
{
  errlog->set_text("");
  echolog->set_text("");
}

void Model::CalcBoundingBoxAndCenter()
{
  Vector3d newMax = Vector3d(G_MINDOUBLE, G_MINDOUBLE, G_MINDOUBLE);
  Vector3d newMin = Vector3d(G_MAXDOUBLE, G_MAXDOUBLE, G_MAXDOUBLE);

  for (uint i = 0 ; i < objtree.Objects.size(); i++) {
    Matrix4d M = objtree.getTransformationMatrix (i);
    for (uint j = 0; j < objtree.Objects[i]->shapes.size(); j++) {
      objtree.Objects[i]->shapes[j]->CalcBBox();
      Vector3d stlMin = M * objtree.Objects[i]->shapes[j]->Min;
      Vector3d stlMax = M * objtree.Objects[i]->shapes[j]->Max;
      for (uint k = 0; k < 3; k++) {
	newMin[k] = MIN(stlMin[k], newMin[k]);
	newMax[k] = MAX(stlMax[k], newMax[k]);
      }
    }
  }

  if (newMin.x() > newMax.x()) {
    // Show the whole platform if there's no objects
    Min = Vector3d(0,0,0);
    Vector3d pM = settings.Hardware.PrintMargin;
    Max = settings.Hardware.Volume - pM - pM;
    Max.z() = 0;
  }
  else {
    Max = newMax;
    Min = newMin;
  }

  Center = (Max + Min) / 2.0;
  m_signal_tree_changed.emit();
}

Vector3d Model::GetViewCenter()
{
  Vector3d printOffset = settings.Hardware.PrintMargin;
  if(settings.RaftEnable)
    printOffset += Vector3d(settings.Raft.Size, settings.Raft.Size, 0);

  return printOffset + Center;
}

// called from View::Draw
int Model::draw (vector<Gtk::TreeModel::Path> &iter)
{
  vector<Shape*> sel_shapes;
  vector<Matrix4d> transforms;
  objtree.get_selected_shapes(iter, sel_shapes, transforms);

  gint index = 1; // pick/select index. matches computation in update_model()

  Vector3d printOffset = settings.Hardware.PrintMargin;
  if(settings.RaftEnable)
    printOffset += Vector3d(settings.Raft.Size, settings.Raft.Size, 0);
  Vector3d translation = objtree.transform3D.getTranslation();
  Vector3d offset = printOffset + translation;
  
  // Add the print offset to the drawing location of the STL objects.
  glTranslatef(offset.x(),offset.y(),offset.z());

  glPushMatrix();
  glMultMatrixd (&objtree.transform3D.transform.array[0]);

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

	  shape->draw (this, settings);

	  if (!settings.Display.DisplayPolygons) {
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
	else shape->draw (this, settings, true);
      }
      else {
	shape->draw (this, settings, false);
      }
      glPopMatrix();
      if(settings.Display.DisplayBBox)
	shape->drawBBox();
     }
    glPopMatrix();
  }
  glPopMatrix();
  glLoadName(0); // Clear selection name to avoid selecting last object with later rendering.

  // draw total bounding box
  if(settings.Display.DisplayBBox)
    {
      // Draw bbox
      glDisable(GL_DEPTH_TEST);
      glColor3f(1,0,0);
      glBegin(GL_LINE_LOOP);
      glVertex3f(Min.x(), Min.y(), Min.z());
      glVertex3f(Min.x(), Max.y(), Min.z());
      glVertex3f(Max.x(), Max.y(), Min.z());
      glVertex3f(Max.x(), Min.y(), Min.z());
      glEnd();
      glBegin(GL_LINE_LOOP);
      glVertex3f(Min.x(), Min.y(), Max.z());
      glVertex3f(Min.x(), Max.y(), Max.z());
      glVertex3f(Max.x(), Max.y(), Max.z());
      glVertex3f(Max.x(), Min.y(), Max.z());
      glEnd();
      glBegin(GL_LINES);
      glVertex3f(Min.x(), Min.y(), Min.z());
      glVertex3f(Min.x(), Min.y(), Max.z());
      glVertex3f(Min.x(), Max.y(), Min.z());
      glVertex3f(Min.x(), Max.y(), Max.z());
      glVertex3f(Max.x(), Max.y(), Min.z());
      glVertex3f(Max.x(), Max.y(), Max.z());
      glVertex3f(Max.x(), Min.y(), Min.z());
      glVertex3f(Max.x(), Min.y(), Max.z());
      glEnd();
      glColor3f(1,0.6,0.6);
      ostringstream val;
      val.precision(1);
      Vector3d pos;
      val << fixed << (Max.x()-Min.x());
      pos = Vector3d((Max.x()+Min.x())/2.,Min.y(),Max.z());
      drawString(pos,val.str());
      val.str("");
      val << fixed << (Max.y()-Min.y());
      pos = Vector3d(Min.x(),(Max.y()+Min.y())/2.,Max.z());
      drawString(pos,val.str());
      val.str("");
      val << fixed << (Max.z()-Min.z());
      pos = Vector3d(Min.x(),Min.y(),(Max.z()+Min.z())/2.);
      drawString(pos,val.str());
    }
    
    if(settings.Display.DisplayLayer) {
      drawLayers(settings.Display.LayerValue,
		 offset, !settings.Display.DisplayLayer);
    }
    if(settings.Display.DisplayGCode && gcode.size() == 0) { 
      // preview gcode if not calculated yet
      if ( m_previewGCode.size() != 0 ||
	   ( layers.size() == 0 && gcode.commands.size() == 0 ) ) { 
	Vector3d start(0,0,0);
	const double thickness = settings.Hardware.LayerThickness;
	const double z = settings.Display.GCodeDrawStart * Max.z() + thickness/2;
	const int LayerCount = (int)ceil(Max.z()/thickness)-1;
	const uint LayerNo = (uint)ceil(settings.Display.GCodeDrawStart*(LayerCount-1));
	if (z != m_previewGCode_z) {
	  Layer * previewGCodeLayer = calcSingleLayer(z, LayerNo, thickness, true, true);
	  if (previewGCodeLayer) {
	    vector<Command> commands;
	    previewGCodeLayer->MakeGcode(start, commands, 0, settings);
	    GCodeState state(m_previewGCode);
	    m_previewGCode.clear();
	    state.AppendCommands(commands, settings.Slicing.RelativeEcode);
	    m_previewGCode_z = z;
	  }
	}
	glDisable(GL_DEPTH_TEST);
	m_previewGCode.drawCommands(settings, 1, m_previewGCode.commands.size(), true, 2, 
				    settings.Display.DisplayGCodeArrows,
				    settings.Display.DisplayGCodeBorders);
      }
    }
    return -1;
}

// if single layer returns layerno of drawn layer 
// else returns -1
int Model::drawLayers(double height, const Vector3d &offset, bool calconly) 
{
  if (is_calculating) return -1; // infill calculation (saved patterns) would be disturbed

  glDisable(GL_DEPTH_TEST);
  int drawn = -1;
  int LayerNr;

  bool have_layers = (layers.size() > 0); // have sliced already

  double minZ = max(0.0, Min.z());
  double z;
  double zStep = settings.Hardware.LayerThickness;
  double zSize = (Max.z() - minZ - zStep*0.5);
  int LayerCount = (int)ceil((zSize - zStep*0.5)/zStep)-1;
  double sel_Z = height*zSize;
  uint sel_Layer;
  if (have_layers) 
      sel_Layer = (uint)ceil(height*(layers.size()-1));
  else 
      sel_Layer = (uint)ceil(LayerCount*(sel_Z)/zSize);
  LayerCount = sel_Layer+1;
  if(settings.Display.DisplayAllLayers)
    {
      LayerNr = 0;
      z=minZ;
    }
  else 
    {
      LayerNr = sel_Layer;
      z= minZ + sel_Z;
    }
  z += 0.5*zStep; // always cut in middle of layer

  //cerr << zStep << ";"<<Max.z()<<";"<<Min.z()<<";"<<zSize<<";"<<LayerNr<<";"<<LayerCount<<";"<<endl;

  Layer* layer=NULL;
  if (have_layers) 
    glTranslatef(-offset.x(), -offset.y(), -offset.z());

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
	  m_previewLayer = calcSingleLayer(z, LayerNr, 
					   settings.Hardware.LayerThickness,
					   settings.Display.DisplayinFill, false);
	  layer = m_previewLayer;
	  Layer * previous = calcSingleLayer(z-settings.Hardware.LayerThickness,
					     LayerNr-1, 
					     settings.Hardware.LayerThickness,
					     false, false);
	  layer->setPrevious(previous);
	}

      if (!calconly) {
	layer->Draw(settings.Display.DrawVertexNumbers,
		    settings.Display.DrawLineNumbers,
		    settings.Display.DrawCPOutlineNumbers,
		    settings.Display.DrawCPLineNumbers, 
		    settings.Display.DrawCPVertexNumbers,
		    settings.Display.DisplayinFill,
		    settings.Display.DisplayDebuginFill,
		    settings.Display.ShowLayerOverhang,
		    settings.Display.RandomizedLines);

	if (settings.Display.DrawMeasures)
	  layer->DrawMeasures(measuresPoint);
      }

      // if (!have_layers)
      // {
      //       // need to delete the temporary  layer
      //       delete layer;
      // }
      LayerNr++; 
      z+=zStep;
    }// while
  return drawn;
}


Layer * Model::calcSingleLayer(double z, uint LayerNr, double thickness,
			       bool calcinfill, bool for_gcode) const
{
  if (is_calculating) return NULL; // infill calculation (saved patterns) would be disturbed
  if (!for_gcode) {
    if (m_previewLayer && m_previewLayer->getZ() == z
	&& m_previewLayer->thickness) return m_previewLayer; 
  }
  vector<Shape*> shapes;
  vector<Matrix4d> transforms;
  
  // if (settings.Slicing.SelectedOnly)
  //   objtree.get_selected_shapes(iter, shapes, transforms);
  // else 
  objtree.get_all_shapes(shapes, transforms);

  Layer * layer = new Layer(NULL, LayerNr, thickness);
  layer->setZ(z);
  for(size_t f=0;f < shapes.size();f++)
    {
      // Vector3d t = T.getTranslation();
      // T.setTranslation(t);
      vector<Poly> polys;
      double max_grad;
      bool polys_ok = shapes[f]->getPolygonsAtZ(transforms[f], z, 
						polys,
						max_grad);
      if (polys_ok){
	layer->addPolygons(polys);
      }
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

  if (settings.Slicing.Skirt) {
    if (layer->getZ() - layer->thickness <= settings.Slicing.SkirtHeight)
      layer->MakeSkirt(settings.Slicing.SkirtDistance);
  }

  if (calcinfill)
    layer->CalcInfill(settings);
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
