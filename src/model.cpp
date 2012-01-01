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
#include "rfo.h"
#include "file.h"
#include "settings.h"
#include "connectview.h"

Model::Model() :
  errlog (Gtk::TextBuffer::create()),
  echolog (Gtk::TextBuffer::create())
{
  // Variable defaults
  Center.x = Center.y = 100.0;
  Center.z = 0.0;
}

Model::~Model()
{
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

void Model::ReadGCode(Glib::RefPtr<Gio::File> file)
{
  m_progress.start (_("Converting"), 100.0);
  gcode.Read (this, &m_progress, file->get_path());
  m_progress.stop (_("Done"));
  ModelChanged();
}

void Model::ClearGCode()
{
  gcode.clear();
}

void Model::ClearCuttingPlanes()
{
  for(uint i=0;i<cuttingplanes.size();i++)
    cuttingplanes[i]->Clear();
  cuttingplanes.clear();
}

Glib::RefPtr<Gtk::TextBuffer> Model::GetGCodeBuffer()
{
  return gcode.buffer;
}

void Model::GlDrawGCode()
{
  gcode.draw (settings);
}

void Model::init() {}

void Model::WriteGCode(Glib::RefPtr<Gio::File> file)
{
  Glib::ustring contents = gcode.get_text();
  Glib::file_set_contents (file->get_path(), contents);
}


void Model::ReadStl(Glib::RefPtr<Gio::File> file)
{
  Slicer stl;
  if (stl.load (file->get_path()) == 0)
    AddStl(NULL, stl, file->get_path());
  ModelChanged();
  ClearCuttingPlanes();
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
	//      return;
      }
  }
  ReadStl (file);
}

void Model::ModelChanged()
{
  //printer.update_temp_poll_interval(); // necessary?
  m_model_changed.emit();
}

static bool ClosestToOrigin (Vector3d a, Vector3d b)
{
  return (a.x*a.x + a.y*a.y + a.z*a.z) < (b.x*b.x + b.y*b.y + b.z*b.z);
}


bool Model::FindEmptyLocation(Vector3d &result, Slicer *stl)
{
  // Get all object positions
  vector<Vector3d> maxpos;
  vector<Vector3d> minpos;

  for(uint o=0;o<rfo.Objects.size();o++)
  {
    for(uint f=0;f<rfo.Objects[o].files.size();f++)
    {
      RFO_File *selectedFile = &rfo.Objects[o].files[f];
      Vector3d p = selectedFile->transform3D.transform.getTranslation();
      Vector3d size = selectedFile->stl.Max - selectedFile->stl.Min;

      minpos.push_back(Vector3d(p.x, p.y, p.z));
      maxpos.push_back(Vector3d(p.x+size.x, p.y+size.y, p.z));
    }
  }

  // Get all possible object positions

  double d = 5.0; // 5mm spacing between objects
  Vector3d StlDelta = (stl->Max-stl->Min);
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

RFO_File* Model::AddStl(RFO_Object *parent, Slicer stl, string filename)
{
  RFO_File *file;
  bool found_location;

  if (!parent) {
    if (rfo.Objects.size() <= 0)
      rfo.newObject();
    parent = &rfo.Objects.back();
  }
  g_assert (parent != NULL);

  // Decide where it's going
  Vector3d trans = Vector3d(0,0,0);
  found_location = FindEmptyLocation(trans, &stl);

  // Add it to the set
  size_t found = filename.find_last_of("/\\");
  Gtk::TreePath path = rfo.createFile (parent, stl, filename.substr(found+1));
  file = &parent->files.back();

  // Move it, if we found a suitable place
  if (found_location)
    file->transform3D.transform.setTranslation(trans);

  // Update the view to include the new object
  CalcBoundingBoxAndCenter();

  // Tell everyone
  m_signal_stl_added.emit (path);

  return file;
}

void Model::newObject()
{
  rfo.newObject();
}

/* Scales the object on changes of the scale slider */
void Model::ScaleObject(RFO_File *file, RFO_Object *object, double scale)
{
  if (!file)
    return;

  file->stl.Scale(scale);
  CalcBoundingBoxAndCenter();
}

void Model::RotateObject(RFO_File *file, RFO_Object *object, Vector4d rotate)
{
  Vector3d rot(rotate.x, rotate.y, rotate.z);

  if (!file)
    return; // FIXME: rotate entire Objects ...

  file->stl.RotateObject(rot, rotate.w);
  CalcBoundingBoxAndCenter();
}

void Model::OptimizeRotation(RFO_File *file, RFO_Object *object)
{
  if (!file)
    return; // FIXME: rotate entire Objects ...

  file->stl.OptimizeRotation();
  CalcBoundingBoxAndCenter();
}

void Model::DeleteRFO(Gtk::TreeModel::iterator &iter)
{
  rfo.DeleteSelected (iter);
  ClearGCode();
  ClearCuttingPlanes();
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

  for (uint i = 0 ; i < rfo.Objects.size(); i++) {
    for (uint j = 0; j < rfo.Objects[i].files.size(); j++) {
      Matrix4d M = rfo.GetSTLTransformationMatrix (i, j);
      Vector3d stlMin = M * rfo.Objects[i].files[j].stl.Min;
      Vector3d stlMax = M * rfo.Objects[i].files[j].stl.Max;
      for (uint k = 0; k < 3; k++) {
	newMin.xyz[k] = MIN(stlMin.xyz[k], newMin.xyz[k]);
	newMax.xyz[k] = MAX(stlMax.xyz[k], newMax.xyz[k]);
      }
    }
  }

  // Leave the alone if there's no objects
  if (newMin.x <= newMax.x) {
    Max = newMax;
    Min = newMin;
    Center = (Max + Min) / 2.0;
    m_signal_rfo_changed.emit();
  }
}

// called from View::Draw
void Model::draw (Gtk::TreeModel::iterator &iter)
{

  RFO_File *sel_file;
  RFO_Object *sel_object;
  gint index = 1; // pick/select index. matches computation in update_model()
  rfo.get_selected_stl (iter, sel_object, sel_file);

  Vector3d printOffset = settings.Hardware.PrintMargin;
  if(settings.RaftEnable)
    printOffset += Vector3d(settings.Raft.Size, settings.Raft.Size, 0);
  Vector3d translation = rfo.transform3D.transform.getTranslation();
  Vector3d offset = printOffset + translation;
  
  // Add the print offset to the drawing location of the STL objects.
  glTranslatef(offset.x, offset.y, offset.z);

  glPushMatrix();
  glMultMatrixd (&rfo.transform3D.transform.array[0]);

  for (uint i = 0; i < rfo.Objects.size(); i++) {
    RFO_Object *object = &rfo.Objects[i];
    index++;

    glPushMatrix();
    glMultMatrixd (&object->transform3D.transform.array[0]);
    for (uint j = 0; j < object->files.size(); j++) {
      RFO_File *file = &object->files[j];
      glLoadName(index); // Load select/pick index
      index++;
      glPushMatrix();
      glMultMatrixd (&file->transform3D.transform.array[0]);

      bool is_selected = (sel_file == file ||
	  (!sel_file && sel_object == object));

      if (is_selected) {
        // Enable stencil buffer when we draw the selected object.
        glEnable(GL_STENCIL_TEST);
        glStencilFunc(GL_ALWAYS, 1, 1);
        glStencilOp(GL_KEEP, GL_REPLACE, GL_REPLACE);

        file->stl.draw (this, settings);

        if (!settings.Display.DisplayPolygons) {
                // If not drawing polygons, need to draw the geometry
                // manually, but invisible, to set up the stencil buffer
                glEnable(GL_CULL_FACE);
                glEnable(GL_DEPTH_TEST);
                glEnable(GL_BLEND);
                // Set to not draw anything, and not update depth buffer
                glDepthMask(GL_FALSE);
                glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

                file->stl.draw_geometry();

                glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
                glDepthMask(GL_TRUE);
        }

        // draw highlight around selected object
        glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
        glLineWidth(1.0);
        glEnable(GL_LINE_SMOOTH);
	glEnable (GL_POLYGON_OFFSET_LINE);

        glDisable (GL_CULL_FACE);
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
        glStencilFunc(GL_NOTEQUAL, 1, 1);
	glEnable(GL_DEPTH_TEST);

	file->stl.draw_geometry();

        glEnable (GL_CULL_FACE);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        glDisable(GL_STENCIL_TEST);
        glDisable(GL_LINE_SMOOTH);
	glDisable(GL_POLYGON_OFFSET_LINE);
      }
      else {
        file->stl.draw (this, settings);
      }

      glPopMatrix();
    }
    glPopMatrix();
  }
  glPopMatrix();


  // draw total bounding box
  if(settings.Display.DisplayBBox)
    {
      // Draw bbox
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

  if(settings.Display.DisplayCuttingPlane)
    drawCuttingPlanes(offset);
  
}

void Model::drawCuttingPlanes(Vector3d offset) const
{
  uint LayerNr;

  bool have_planes = cuttingplanes.size() > 0; // have sliced already

  double z;
  double zStep = settings.Hardware.LayerThickness;
  double zSize = (Max.z-Min.z);
  uint LayerCount = (uint)ceil((zSize + zStep*0.5)/zStep);
  double sel_Z = settings.Display.CuttingPlaneValue*zSize;
  uint sel_Layer;
  if (have_planes) 
      sel_Layer = (uint)ceil(settings.Display.CuttingPlaneValue*(cuttingplanes.size()-1));
  else 
      sel_Layer = (uint)ceil(LayerCount*(sel_Z)/zSize);
  LayerCount = sel_Layer+1;
  if(settings.Display.DisplayAllLayers)
    {
      LayerNr = 0;
      z=Min.z + 0.5*zStep;
    } else 
    {
      LayerNr = sel_Layer;
      z=Min.z + sel_Z;
    }
  //cerr << zStep << ";"<<Max.z<<";"<<Min.z<<";"<<zSize<<";"<<LayerNr<<";"<<LayerCount<<";"<<endl;

  vector<int> altInfillLayers;
  settings.Slicing.GetAltInfillLayers (altInfillLayers, LayerCount);
  
  CuttingPlane* plane;
  if (have_planes) 
    glTranslatef(-offset.x, -offset.y, -offset.z);

  while(LayerNr < LayerCount)
    {
      if (have_planes)
	{
	  plane = cuttingplanes[LayerNr];
	  z = plane->getZ();
	}
      else
	{
	  plane = new CuttingPlane(LayerNr);
	  plane->setZ(z);
	  for(size_t o=0;o<rfo.Objects.size();o++)
	    {
	      for(size_t f=0;f<rfo.Objects[o].files.size();f++)
		{
		  Matrix4d T = rfo.GetSTLTransformationMatrix(o,f);
		  Vector3d t = T.getTranslation();
		  T.setTranslation(t);
		  rfo.Objects[o].files[f].stl.CalcCuttingPlane(T, settings.Slicing.Optimization, plane);
		}
	    }
	  plane->MakePolygons(settings.Slicing.Optimization);
	  plane->MakeShells(//settings.Slicing.ShrinkQuality,
			    settings.Slicing.ShellCount,
			    settings.Hardware.ExtrudedMaterialWidth,
			    settings.Slicing.Optimization,
			    false);
	  if (settings.Display.DisplayinFill)
	    {
	      double fullInfillDistance = settings.Hardware.ExtrudedMaterialWidth
		* settings.Hardware.ExtrusionFactor;  
	      double infillDistance = fullInfillDistance *(1+settings.Slicing.InfillDistance);
	      // if (std::find(altInfillLayers.begin(), altInfillLayers.end(), LayerNr) 
	      // 	  != altInfillLayers.end())
	      // 	infillDistance = settings.Slicing.AltInfillDistance;
	      // else
	      // 	infillDistance = settings.Slicing.InfillDistance;
	      plane->CalcInfill(infillDistance, fullInfillDistance,
				settings.Slicing.InfillRotation,
				settings.Slicing.InfillRotationPrLayer, 
				settings.Display.DisplayDebuginFill);
	    }
	}
      plane->Draw(settings.Display.DrawVertexNumbers,
		  settings.Display.DrawLineNumbers,
		  settings.Display.DrawCPOutlineNumbers,
		  settings.Display.DrawCPLineNumbers, 
		  settings.Display.DrawCPVertexNumbers,
		  settings.Display.DisplayinFill);
      // displayInfillOld(settings, plane, plane.LayerNo, altInfillLayers);
      
      LayerNr++; 
      z+=zStep;
    }// while
}

