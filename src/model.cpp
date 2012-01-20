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

Model::Model() :
  currentprintingline(0),
  settings(),
  Min(), Max(),
  errlog (Gtk::TextBuffer::create()),
  echolog (Gtk::TextBuffer::create())
{
  // Variable defaults
  Center.x = Center.y = 100.0;
  Center.z = 0.0;
  is_calculating= false;
}

Model::~Model()
{
  ClearLayers();
  ClearGCode();
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

void Model::ReadGCode(Glib::RefPtr<Gio::File> file)
{
  m_progress->start (_("Converting"), 100.0);
  gcode.Read (this, m_progress, file->get_path());
  m_progress->stop (_("Done"));
  ModelChanged();
}

void Model::ClearGCode()
{
  gcode.clear();
}

void Model::ClearLayers()
{
  for(uint i=0;i<layers.size();i++)
    layers[i]->Clear();
  layers.clear();
  Infill::clearPatterns();
}

Glib::RefPtr<Gtk::TextBuffer> Model::GetGCodeBuffer()
{
  return gcode.buffer;
}

void Model::GlDrawGCode(int layerno)
{
  gcode.draw (settings, layerno, false);
  int currentlayer = gcode.getLayerNo(currentprintingline);
  if (currentlayer>=0)
    gcode.draw (settings, currentlayer, true);
}

void Model::GlDrawGCode(double layerz)
{
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


void Model::ReadStl(Glib::RefPtr<Gio::File> file)
{
  bool autoplace = settings.Misc.ShapeAutoplace;
  string path = file->get_path();
  filetype_t ftype =  Shape::getFileType(path);
  if (ftype == BINARY_STL) // only one shape per file
    {
      Shape shape;
      shape.loadBinarySTL(path);      
      AddShape(NULL, shape, path,autoplace);
      ModelChanged();
     }
  else if (ftype == ASCII_STL) // multiple shapes per file
    {
      ifstream fileis;
      fileis.open(path.c_str());
      size_t found = path.find_last_of("/\\");
      vector<Shape> shapes;
      while (!fileis.eof())
	{
	  Shape shape;
	  int ok = shape.parseASCIISTL(&fileis);
	  if (ok==0) {
	    shapes.push_back(shape);
	    // go back to get "solid " keyword again
	    streampos where = fileis.tellg();
	    where-=100;
	    if (where < 0) break;
	    fileis.seekg(where,ios::beg);
	  }
	}
      if (shapes.size()==1){
	shapes.front().filename = (string)path.substr(found+1);
	AddShape(NULL, shapes.front(), shapes.front().filename , autoplace);
      }
      else for (uint i=0;i<shapes.size();i++){
	  // do not autoplace to keep saved positions
	  AddShape(NULL, shapes[i], shapes[i].filename, false);
	}
      shapes.clear();
      ModelChanged();
      fileis.close();
    }
  else if (ftype == VRML) 
    {
      Shape shape;
      shape.loadASCIIVRML(path);      
      AddShape(NULL, shape, path,autoplace);
      ModelChanged();
    }

  ClearLayers();
}

void Model::SaveStl(Glib::RefPtr<Gio::File> file)
{
  stringstream sstr;
  for(uint o=0;o<objtree.Objects.size();o++)
  {
    for(uint f=0;f<objtree.Objects[o].shapes.size();f++)
    {
      Shape *shape = &objtree.Objects[o].shapes[f];
      sstr << shape->getSTLsolid() << endl;
    }
  }
  Glib::file_set_contents (file->get_path(), sstr.str());
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
    else if (extn == ".rfo")
      {
	//      ReadRFO (file);
	return;
      }
  }
  ReadStl (file);
}

void Model::ModelChanged()
{
  //printer.update_temp_poll_interval(); // necessary?
  Infill::clearPatterns();
  CalcBoundingBoxAndCenter();
  m_model_changed.emit();
}

static bool ClosestToOrigin (Vector3d a, Vector3d b)
{
  return (a.x*a.x + a.y*a.y + a.z*a.z) < (b.x*b.x + b.y*b.y + b.z*b.z);
}


bool Model::FindEmptyLocation(Vector3d &result, Shape *shape)
{
  // Get all object positions
  vector<Vector3d> maxpos;
  vector<Vector3d> minpos;

  for(uint o=0;o<objtree.Objects.size();o++)
  {
    for(uint f=0;f<objtree.Objects[o].shapes.size();f++)
    {
      Shape *selectedShape = &objtree.Objects[o].shapes[f];
      Vector3d p = selectedShape->transform3D.transform.getTranslation();
      Vector3d size = selectedShape->Max - selectedShape->Min;

      minpos.push_back(Vector3d(p.x, p.y, p.z));
      maxpos.push_back(Vector3d(p.x+size.x, p.y+size.y, p.z));
    }
  }

  // Get all possible object positions

  double d = 5.0; // 5mm spacing between objects
  Vector3d StlDelta = (shape->Max - shape->Min);
  vector<Vector3d> candidates;

  candidates.push_back(Vector3d(0.0, 0.0, 0.0));

  for (uint j=0; j<maxpos.size(); j++)
  {
    candidates.push_back(Vector3d(maxpos[j].x+d, minpos[j].y, maxpos[j].z));
    candidates.push_back(Vector3d(minpos[j].x, maxpos[j].y+d, maxpos[j].z));
    candidates.push_back(Vector3d(maxpos[j].x+d, maxpos[j].y+d, maxpos[j].z));
  }

  // Prefer positions closest to origin
  sort(candidates.begin(), candidates.end(), ClosestToOrigin);

  // Check all candidates for collisions with existing objects
  for (uint c=0; c<candidates.size(); c++)
  {
    bool ok = true;

    for (uint k=0; k<maxpos.size(); k++)
    {
      if (
          // check x
          ((minpos[k].x <= candidates[c].x && candidates[c].x <= maxpos[k].x) ||
          (candidates[c].x <= minpos[k].x && maxpos[k].x <= candidates[c].x+StlDelta.x+d) ||
          (minpos[k].x <= candidates[c].x+StlDelta.x+d && candidates[c].x+StlDelta.x+d <= maxpos[k].x))
          &&
          // check y
          ((minpos[k].y <= candidates[c].y && candidates[c].y <= maxpos[k].y) ||
          (candidates[c].y <= minpos[k].y && maxpos[k].y <= candidates[c].y+StlDelta.y+d) ||
          (minpos[k].y <= candidates[c].y+StlDelta.y+d && candidates[c].y+StlDelta.y+d <= maxpos[k].y))
      )
      {
        ok = false;
        break;
      }

      // volume boundary
      if (candidates[c].x+StlDelta.x > (settings.Hardware.Volume.x - 2*settings.Hardware.PrintMargin.x) ||
          candidates[c].y+StlDelta.y > (settings.Hardware.Volume.y - 2*settings.Hardware.PrintMargin.y))
      {
        ok = false;
        break;
      }
    }
    if (ok)
    {
      result.x = candidates[c].x;
      result.y = candidates[c].y;
      result.z = candidates[c].z;
      return true;
    }
  }

  // no empty spots
  return false;
}

Shape* Model::AddShape(TreeObject *parent, Shape shape, string filename, bool autoplace)
{
  Shape *retshape;
  bool found_location=false;

  if (!parent) {
    if (objtree.Objects.size() <= 0)
      objtree.newObject();
    parent = &objtree.Objects.back();
  }
  g_assert (parent != NULL);

  // Decide where it's going
  Vector3d trans = Vector3d(0,0,0);
  if (autoplace) found_location = FindEmptyLocation(trans, &shape);

  // Add it to the set
  size_t found = filename.find_last_of("/\\");
  Gtk::TreePath path = objtree.addShape(parent, shape, filename.substr(found+1));
  retshape = &parent->shapes.back();

  // Move it, if we found a suitable place
  if (found_location)
    retshape->transform3D.transform.setTranslation(trans);

  // Update the view to include the new object
  CalcBoundingBoxAndCenter();

  // Tell everyone
  m_signal_stl_added.emit (path);

  return retshape;
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
    for (uint s = 0;s<object->shapes.size(); s++) {
      //double fact = object->shapes[s].getScaleFactor();
      object->shapes[s].Scale(scale);
    }
  else return;
  CalcBoundingBoxAndCenter();
}
void Model::ScaleObjectX(Shape *shape, TreeObject *object, double scale)
{
  if (shape)
    shape->ScaleX(scale);
  else if(object)
    for (uint s = 0;s<object->shapes.size(); s++) {
      //double fact = object->shapes[s].getScaleFactor();
      object->shapes[s].ScaleX(scale);
    }
  else return;
  CalcBoundingBoxAndCenter();
}
void Model::ScaleObjectY(Shape *shape, TreeObject *object, double scale)
{
  if (shape)
    shape->ScaleY(scale);
  else if(object)
    for (uint s = 0;s<object->shapes.size(); s++) {
      //double fact = object->shapes[s].getScaleFactor();
      object->shapes[s].ScaleY(scale);
    }
  else return;
  CalcBoundingBoxAndCenter();
}
void Model::ScaleObjectZ(Shape *shape, TreeObject *object, double scale)
{
  if (shape)
    shape->ScaleZ(scale);
  else if(object)
    for (uint s = 0;s<object->shapes.size(); s++) {
      //      double fact = object->shapes[s].getScaleFactorZ();
      object->shapes[s].ScaleZ(scale);
    }
  else return;
  CalcBoundingBoxAndCenter();
}

// void Model::ScaleObject(Shape *shape, TreeObject *object,
// 			 double scale_x, double scale_y, double scale_z)
// {
//   if (shape)
//     shape->Scale(scale_x,scale_y,scale_z);
//   else if (object)
//     for (uint s = 0;s<object->shapes.size(); s++) {
//       double xfact = object->shapes[s].getScaleFactorX();
//       double yfact = object->shapes[s].getScaleFactorY();
//       double zfact = object->shapes[s].getScaleFactorZ();
//       object->shapes[s].Scale(xfact*scale_x,yfact*scale_y,zfact*scale_z);
//     }
//   else return;
//   CalcBoundingBoxAndCenter();
// }

void Model::RotateObject(Shape *shape, TreeObject *object, Vector4d rotate)
{
  Vector3d rot(rotate.x, rotate.y, rotate.z);

  if (!shape)
    return; // FIXME: rotate entire Objects ...

  shape->Rotate(rot, rotate.w);
  CalcBoundingBoxAndCenter();
}

void Model::OptimizeRotation(Shape *shape, TreeObject *object)
{
  if (!shape)
    return; // FIXME: rotate entire Objects ...

  shape->OptimizeRotation();
  CalcBoundingBoxAndCenter();
}

void Model::InvertNormals(Shape *shape, TreeObject *object)
{
  if (shape)
    shape->invertNormals();
  else // if (object) object->invertNormals();
    return; 
  CalcBoundingBoxAndCenter();
}
void Model::Mirror(Shape *shape, TreeObject *object)
{
  if (shape)
    shape->mirror();
  else // if (object) object->mirror();
    return; 
  CalcBoundingBoxAndCenter();
}

void Model::DeleteObjTree(Gtk::TreeModel::iterator &iter)
{
  objtree.DeleteSelected (iter);
  ClearGCode();
  ClearLayers();
  CalcBoundingBoxAndCenter();
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
    Matrix4d M = objtree.GetSTLTransformationMatrix (i);
    for (uint j = 0; j < objtree.Objects[i].shapes.size(); j++) {
      objtree.Objects[i].shapes[j].CalcBBox();
      Vector3d stlMin = M * objtree.Objects[i].shapes[j].Min;
      Vector3d stlMax = M * objtree.Objects[i].shapes[j].Max;
      for (uint k = 0; k < 3; k++) {
	newMin.xyz[k] = MIN(stlMin.xyz[k], newMin.xyz[k]);
	newMax.xyz[k] = MAX(stlMax.xyz[k], newMax.xyz[k]);
      }
    }
  }

  if (newMin.x > newMax.x) {
    // Show the whole platform if there's no objects
    Min = Vector3d(0,0,0);
    Vector3d pM = settings.Hardware.PrintMargin;
    Max = settings.Hardware.Volume - pM - pM;
    Max.z = 0;
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
void Model::draw (Gtk::TreeModel::iterator &iter)
{
  
  Shape *sel_shape;
  TreeObject *sel_object;
  gint index = 1; // pick/select index. matches computation in update_model()
  objtree.get_selected_stl (iter, sel_object, sel_shape);

  Vector3d printOffset = settings.Hardware.PrintMargin;
  if(settings.RaftEnable)
    printOffset += Vector3d(settings.Raft.Size, settings.Raft.Size, 0);
  Vector3d translation = objtree.transform3D.transform.getTranslation();
  Vector3d offset = printOffset + translation;
  
  // Add the print offset to the drawing location of the STL objects.
  glTranslatef(offset.x, offset.y, offset.z);

  glPushMatrix();
  glMultMatrixd (&objtree.transform3D.transform.array[0]);

  for (uint i = 0; i < objtree.Objects.size(); i++) {
    TreeObject *object = &objtree.Objects[i];
    index++;

    glPushMatrix();
    glMultMatrixd (&object->transform3D.transform.array[0]);
    for (uint j = 0; j < object->shapes.size(); j++) {
      Shape *shape = &object->shapes[j];
      glLoadName(index); // Load select/pick index
      index++;
      glPushMatrix();
      glMultMatrixd (&shape->transform3D.transform.array[0]);

      bool is_selected = (sel_shape == shape ||
	  (!sel_shape && sel_object == object));

      if (is_selected) {
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
      else {
        shape->draw (this, settings);
      }

      glPopMatrix();
    }
    glPopMatrix();
  }
  glPopMatrix();
  glLoadName(0); // Clear selection name to avoid selecting last object with later rendering.

  // draw total bounding box
  if(settings.Display.DisplayBBox)
    {
      for (uint i = 0; i < objtree.Objects.size(); i++) {
	for (uint j = 0; j < objtree.Objects[i].size(); j++) 
	  objtree.Objects[i].shapes[j].drawBBox();
      }
      // Draw bbox
      glDisable(GL_DEPTH_TEST);
      glColor3f(1,0,0);
      glBegin(GL_LINE_LOOP);
      glVertex3f(Min.x, Min.y, Min.z);
      glVertex3f(Min.x, Max.y, Min.z);
      glVertex3f(Max.x, Max.y, Min.z);
      glVertex3f(Max.x, Min.y, Min.z);
      glEnd();
      glBegin(GL_LINE_LOOP);
      glVertex3f(Min.x, Min.y, Max.z);
      glVertex3f(Min.x, Max.y, Max.z);
      glVertex3f(Max.x, Max.y, Max.z);
      glVertex3f(Max.x, Min.y, Max.z);
      glEnd();
      glBegin(GL_LINES);
      glVertex3f(Min.x, Min.y, Min.z);
      glVertex3f(Min.x, Min.y, Max.z);
      glVertex3f(Min.x, Max.y, Min.z);
      glVertex3f(Min.x, Max.y, Max.z);
      glVertex3f(Max.x, Max.y, Min.z);
      glVertex3f(Max.x, Max.y, Max.z);
      glVertex3f(Max.x, Min.y, Min.z);
      glVertex3f(Max.x, Min.y, Max.z);
      glEnd();
    }

  if(settings.Display.DisplayLayer) {
    glDisable(GL_DEPTH_TEST);
    drawLayers(offset);
  }
}

void Model::drawLayers(Vector3d offset) const
{
  int LayerNr;

  bool have_layers = layers.size() > 0; // have sliced already

  double z;
  double zStep = settings.Hardware.LayerThickness;
  double zSize = (Max.z-Min.z-zStep*0.5);
  int LayerCount = (int)ceil((zSize - zStep*0.5)/zStep)-1;
  double sel_Z = settings.Display.LayerValue*zSize;
  uint sel_Layer;
  if (have_layers) 
      sel_Layer = (uint)ceil(settings.Display.LayerValue*(layers.size()-1));
  else 
      sel_Layer = (uint)ceil(LayerCount*(sel_Z)/zSize);
  LayerCount = sel_Layer+1;
  if(settings.Display.DisplayAllLayers)
    {
      LayerNr = 0;
      z=Min.z;
    }
  else 
    {
      LayerNr = sel_Layer;
      z= Min.z + sel_Z;
    }
  z += 0.5*zStep; // always cut in middle of layer


  //cerr << zStep << ";"<<Max.z<<";"<<Min.z<<";"<<zSize<<";"<<LayerNr<<";"<<LayerCount<<";"<<endl;

  vector<int> altInfillLayers;
  settings.Slicing.GetAltInfillLayers (altInfillLayers, LayerCount);
  
  Layer* layer=NULL;
  if (have_layers) 
    glTranslatef(-offset.x, -offset.y, -offset.z);

  double matwidth;
  
  while(LayerNr < LayerCount)
    {
      if (have_layers)
	{
	  layer = layers[LayerNr];
	  z = layer->getZ();
	}
      else
	{
	  layer = new Layer(LayerNr,settings.Hardware.LayerThickness);
	  layer->setZ(z);
	  matwidth = settings.Hardware.GetExtrudedMaterialWidth(layer->thickness);
	  for(size_t o=0;o<objtree.Objects.size();o++){
	    Matrix4d T = objtree.GetSTLTransformationMatrix(o);
	    for(size_t f=0;f<objtree.Objects[o].shapes.size();f++)
	      {
		Vector3d t = T.getTranslation();
		T.setTranslation(t);
		vector<Poly> polys;
		double max_grad;
		bool polys_ok=
		  objtree.Objects[o].shapes[f].getPolygonsAtZ(T, z, 
							      settings.Slicing.Optimization,
							      polys,
							      max_grad);
		if (polys_ok) layer->addPolygons(polys);
	      }
	  }
	  
	  bool makeskirt = (layer->getZ() <= settings.Slicing.SkirtHeight);

	  layer->MakeShells(settings.Slicing.ShellCount,
			    matwidth, settings.Slicing.Optimization,
			    makeskirt, false);
	  if (settings.Display.DisplayinFill)
	    {
	      double fullInfillDistance = matwidth;
	      double infillDistance = fullInfillDistance * (1+settings.Slicing.InfillDistance);
	      double altInfillDistance = fullInfillDistance *(1+settings.Slicing.AltInfillDistance);
	      double infilldist;
	      if (std::find(altInfillLayers.begin(), altInfillLayers.end(), LayerNr) 
		  != altInfillLayers.end())
		infilldist = altInfillDistance;
	      else
		infilldist = infillDistance;
	      layer->CalcInfill(settings.Slicing.NormalFilltype,
				settings.Slicing.FullFilltype,
				settings.Slicing.SupportFilltype,
				infillDistance, fullInfillDistance,
				settings.Slicing.InfillRotation,
				settings.Slicing.InfillRotationPrLayer, 
				settings.Slicing.ShellOnly,
				settings.Display.DisplayDebuginFill);
	    }
	}
      layer->Draw(settings.Display.DrawVertexNumbers,
		  settings.Display.DrawLineNumbers,
		  settings.Display.DrawCPOutlineNumbers,
		  settings.Display.DrawCPLineNumbers, 
		  settings.Display.DrawCPVertexNumbers,
		  settings.Display.DisplayinFill);

      if (!have_layers)
      {
            // need to delete the temporary  layer
            delete layer;
      }
      LayerNr++; 
      z+=zStep;
    }// while
}


