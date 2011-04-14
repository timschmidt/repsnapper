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
#include "config.h"
#include <vector>
#include <string>
#include <cerrno>
#include <functional>

// should move to platform.h with com port fun.
#include <sys/types.h>
#include <dirent.h>

#include <glib/gutils.h>

#include "stdafx.h"
#include "model.h"
#include "rfo.h"
#include "file.h"
#include "view.h"
#include "settings.h"
#include "connectview.h"

// Exception safe guard to stop people printing
// GCode while loading it / converting etc.
struct PrintInhibitor {
  Model *m_model;
public:
  PrintInhibitor(Model *model) : m_model (model)
  {
    m_model->m_inhibit_print = true;
    m_model->m_signal_inhibit_changed.emit();
  }
  ~PrintInhibitor()
  {
    m_model->m_inhibit_print = false;
    m_model->m_signal_inhibit_changed.emit();
  }
};

Model::Model() :
  m_printing(false),
  m_inhibit_print(false),
  commlog(Gtk::TextBuffer::create()),
  errlog(Gtk::TextBuffer::create()),
  echolog(Gtk::TextBuffer::create())
{
  // Variable defaults
  Center.x = Center.y = 100.0f;
  Center.z = 0.0f;

  // TODO: Configurable protocol, cache size
  m_device = rr_create(RR_PROTO_FIVED,
                     &handle_send, static_cast<void*>(this),
                     &handle_recv, static_cast<void*>(this),
                     &handle_reply, static_cast<void*>(this),
                     NULL, NULL,
                     &handle_want_writable, static_cast<void*>(this),
                     64);

}

Model::~Model()
{
  if(rr_dev_fd(m_device) >= 0) {
    rr_close(m_device);
  }
  rr_free(m_device);
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
   cout << "not yet implemented\n";
}

void Model::ReadGCode(Glib::RefPtr<Gio::File> file)
{
  PrintInhibitor inhibitPrint(this);
  m_progress.start ("Converting", 100.0);
  gcode.Read (this, &m_progress, file->get_path());
  m_progress.stop ("Done");
  ModelChanged();
}

void Model::ConvertToGCode()
{
	string GcodeTxt;
	string GcodeStart = settings.GCode.getStartText();
	string GcodeLayer = settings.GCode.getLayerText();
	string GcodeEnd = settings.GCode.getEndText();

	PrintInhibitor inhibitPrint(this);
	m_progress.start ("Converting", Max.z);

	// Make Layers
	uint LayerNr = 0;
	uint LayerCount = (uint)ceil((Max.z+settings.Hardware.LayerThickness*0.5f)/settings.Hardware.LayerThickness);

	vector<int> altInfillLayers;
	settings.Slicing.GetAltInfillLayers (altInfillLayers, LayerCount);

	printOffset = settings.Hardware.PrintMargin;

	float z = Min.z + settings.Hardware.LayerThickness*0.5f;				// Offset it a bit in Z, z=0 gives a empty slice because no triangles crosses this Z value

	gcode.clear();

	float printOffsetZ = settings.Hardware.PrintMargin.z;

	if(settings.RaftEnable)
	{
		printOffset += Vector3f(settings.Raft.Size, settings.Raft.Size, 0);
		MakeRaft(printOffsetZ);
	}
	float E=0.0f;
	while(z<Max.z)
	{
	  m_progress.update(z);
		for(uint o=0;o<rfo.Objects.size();o++)
		{
			for(uint f=0;f<rfo.Objects[o].files.size();f++)
			{
				STL* stl = &rfo.Objects[o].files[f].stl;	// Get a pointer to the object
				Matrix4f T = rfo.GetSTLTransformationMatrix(o,f);
				Vector3f t = T.getTranslation();
				t+= Vector3f(settings.Hardware.PrintMargin.x+settings.Raft.Size*settings.RaftEnable, settings.Hardware.PrintMargin.y+settings.Raft.Size*settings.RaftEnable, 0);
				T.setTranslation(t);
				CuttingPlane plane;
				stl->CalcCuttingPlane(z, plane, T);	// output is alot of un-connected line segments with individual vertices, describing the outline

				float hackedZ = z;
				while (plane.LinkSegments (hackedZ, settings.Slicing.Optimization) == false)	// If segment linking fails, re-calc a new layer close to this one, and use that.
				{ // This happens when there's triangles missing in the input STL
					hackedZ+= 0.1f;
					stl->CalcCuttingPlane (hackedZ, plane, T);	// output is alot of un-connected line segments with individual vertices
				}
				plane.SetZ (z + printOffsetZ);

				// inFill
				vector<Vector2f> infill;

//				CuttingPlane infillCuttingPlane;
//				plane.MakeContainedPlane(infillCuttingPlane);
				if(settings.Slicing.ShellOnly == false)
				{
					switch( settings.Slicing.ShrinkQuality )
					{
					case SHRINK_FAST:
						plane.ShrinkFast(settings.Hardware.ExtrudedMaterialWidth*0.5f, settings.Slicing.Optimization, settings.Display.DisplayCuttingPlane, false, settings.Slicing.ShellCount);
						break;
					case SHRINK_LOGICK:
						plane.ShrinkLogick(settings.Hardware.ExtrudedMaterialWidth, settings.Slicing.Optimization, settings.Display.DisplayCuttingPlane, settings.Slicing.ShellCount);
						break;
					default:
						g_warning ("unknown shrinking algorithm");
						break;
					}

					// check if this if a layer we should use the alternate infill distance on
					float infillDistance = settings.Slicing.InfillDistance;
					if (std::find(altInfillLayers.begin(), altInfillLayers.end(), LayerNr) != altInfillLayers.end())
					{
						infillDistance = settings.Slicing.AltInfillDistance;
					}

					plane.CalcInFill(infill, LayerNr, infillDistance, settings.Slicing.InfillRotation, settings.Slicing.InfillRotationPrLayer, settings.Display.DisplayDebuginFill);
				}
				// Make the GCode from the plane and the infill
				plane.MakeGcode (infill, gcode, E, z + printOffsetZ,
						 settings.Slicing, settings.Hardware);
			}
		}
		LayerNr++;
		z+=settings.Hardware.LayerThickness;
	}

	float AntioozeDistance = settings.Slicing.AntioozeDistance;
	if (!settings.Slicing.EnableAntiooze)
	  AntioozeDistance = 0;

	gcode.MakeText (GcodeTxt, GcodeStart, GcodeLayer, GcodeEnd,
			settings.Slicing.UseIncrementalEcode,
			settings.Slicing.Use3DGcode,
			AntioozeDistance,
			settings.Slicing.AntioozeSpeed);
	m_progress.stop("Done");
}

void Model::init() {}

void Model::WriteGCode(Glib::RefPtr<Gio::File> file)
{
  Glib::ustring contents = gcode.get_text();
  Glib::file_set_contents (file->get_path(), contents);
}

void Model::Restart()
{
  Print();
}

void Model::ContinuePauseButton()
{
  if (m_printing) {
    // TODO: Actually pause (stop enqueueing)
    m_printing = false;
  } else {
    Continue();
  }
}

void Model::Continue()
{
  m_printing = true;
  // TODO: Actually unpause (resume enqueueing)
}

void Model::PrintButton()
{
  if (m_printing) {
    Restart();
  } else {
    Print();
  }
}

bool Model::IsConnected()
{
	return rr_dev_fd(m_device) >= 0;
}


// we try to change the state of the connection
void Model::serial_try_connect (bool connect)
{
  int result;
  if(connect) {
    m_signal_serial_state_changed.emit (SERIAL_CONNECTING);
    result = rr_open (m_device, settings.Hardware.PortName.c_str(),
		      settings.Hardware.SerialSpeed);
    if(result < 0) {
      m_signal_serial_state_changed.emit (SERIAL_DISCONNECTED);
      Gtk::MessageDialog dialog("Failed to connect to device", false,
                                Gtk::MESSAGE_ERROR, Gtk::BUTTONS_CLOSE);
      dialog.set_secondary_text(strerror(errno));
      dialog.run();
    } else {
      m_signal_serial_state_changed.emit (SERIAL_CONNECTED);
    }
  } else {
    m_signal_serial_state_changed.emit (SERIAL_DISCONNECTING);
    result = rr_close(m_device);
    if(result < 0) {
      m_signal_serial_state_changed.emit (SERIAL_CONNECTED);
      // TODO: Error dialog
      std::string message();
      Gtk::MessageDialog dialog("Failed to disconnect from device", false,
                                Gtk::MESSAGE_ERROR, Gtk::BUTTONS_CLOSE);
      dialog.set_secondary_text(strerror(errno));
      dialog.run();
    } else {
      m_signal_serial_state_changed.emit (SERIAL_DISCONNECTED);
    }
  }
}

void Model::SimplePrint()
{
  if (m_printing)
    alert ("Already printing.\nCannot start printing");

  if (rr_dev_fd(m_device) < 0)
      serial_try_connect (true);
  Print();
}

void Model::Print()
{
	if(rr_dev_fd(m_device) < 0) {
		alert ("Not connected to printer.\nCannot start printing");
		return;
	}

	rr_reset(m_device);
	gcode.queue_to_serial(m_device);
	m_progress.start ("Printing", gcode.commands.size());
	m_printing = true;
}

void Model::RunExtruder (double extruder_speed, double extruder_length, bool reverse)
{
  static bool extruderIsRunning = false;
  if (settings.Slicing.Use3DGcode) {
    if (extruderIsRunning)
      SendNow("M103");
    else
      SendNow("M101");
    extruderIsRunning = !extruderIsRunning;
    return;
  }

  std::stringstream oss;
  string command("G1 F");
  oss << extruder_speed;
  command += oss.str();
  SendNow(command);
  oss.str("");

  // set extruder zero
  SendNow("G92 E0");
  oss << extruder_length;
  string command2("G1 E");

  if (reverse)
    command2+="-";
  command2+=oss.str();
  SendNow(command2);
  SendNow("G1 F1500.0");
  SendNow("G92 E0");	// set extruder zero
}

void Model::SendNow(string str)
{
  if(rr_dev_fd(m_device) > 0) {
    rr_enqueue(m_device, RR_PRIO_HIGH, NULL, str.data(), str.size());
  } else {
    Gtk::MessageDialog dialog("Can't send command", false,
                              Gtk::MESSAGE_INFO, Gtk::BUTTONS_CLOSE);
    dialog.set_secondary_text("You must first connect to a device!");
    dialog.run();
  }
}

void Model::Home(string axis)
{
	if(m_printing)
	{
		alert("Can't go home while printing");
		return;
	}
	if(axis == "X" || axis == "Y" || axis == "Z")
	{
		string buffer="G1 F";
		std::stringstream oss;
		if(axis == "Z")
			oss << settings.Hardware.MaxPrintSpeedZ;
		else
			oss << settings.Hardware.MaxPrintSpeedXY;
		buffer+= oss.str();
		SendNow(buffer);
		buffer="G1 ";
		buffer += axis;
		buffer+="-250 F";
		buffer+= oss.str();
		SendNow(buffer);
		buffer="G92 ";
		buffer += axis;
		buffer+="0";
		SendNow(buffer);	// Set this as home
		oss.str("");
		buffer="G1 ";
		buffer += axis;
		buffer+="1 F";
		buffer+= oss.str();
		SendNow(buffer);
		if(axis == "Z")
			oss << settings.Hardware.MinPrintSpeedZ;
		else
			oss << settings.Hardware.MinPrintSpeedXY;
		buffer="G1 ";
		buffer+="F";
		buffer+= oss.str();
		SendNow(buffer);	// set slow speed
		buffer="G1 ";
		buffer += axis;
		buffer+="-10 F";
		buffer+= oss.str();
		SendNow(buffer);
		buffer="G92 ";
		buffer += axis;
		buffer+="0";
		SendNow(buffer);	// Set this as home
	}
	else if(axis == "ALL")
	{
		SendNow("G28");
	}
	else
		alert("Home called with unknown axis");
}

void Model::Move(string axis, float distance)
{
	if(m_printing)
	{
		alert("Can't move manually while printing");
		return;
	}
	if(axis == "X" || axis == "Y" || axis == "Z")
	{
		SendNow("G91");	// relative positioning
		string buffer="G1 F";
		std::stringstream oss;
		if(axis == "Z")
			oss << settings.Hardware.MaxPrintSpeedZ;
		else
			oss << settings.Hardware.MaxPrintSpeedXY;
		buffer+= oss.str();
		SendNow(buffer);
		buffer="G1 ";
		buffer += axis;
		oss.str("");
		oss << distance;
		buffer+= oss.str();
		oss.str("");
		if(axis == "Z")
			oss << settings.Hardware.MaxPrintSpeedZ;
		else
			oss << settings.Hardware.MaxPrintSpeedXY;
		buffer+=" F"+oss.str();
		SendNow(buffer);
		SendNow("G90");	// absolute positioning
	}
	else
		alert("Move called with unknown axis");
}
void Model::Goto(string axis, float position)
{
	if(m_printing)
	{
		alert("Can't move manually while printing");
		return;
	}
	if(axis == "X" || axis == "Y" || axis == "Z")
	{
		string buffer="G1 F";
		std::stringstream oss;
		oss << settings.Hardware.MaxPrintSpeedXY;
		buffer+= oss.str();
		SendNow(buffer);
		buffer="G1 ";
		buffer += axis;
		oss.str("");
		oss << position;
		buffer+= oss.str();
		oss.str("");
		oss << settings.Hardware.MaxPrintSpeedXY;
		buffer+=" F"+oss.str();
		SendNow(buffer);
	}
	else
		alert("Goto called with unknown axis");
}
void Model::STOP()
{
	SendNow("M112");
  rr_reset(m_device);
}

void Model::ReadStl(Glib::RefPtr<Gio::File> file)
{
  STL stl;
  if (stl.loadFile (file->get_path()) == 0)
    AddStl(stl, file->get_path());
  ModelChanged();
}

void Model::ModelChanged()
{
  m_model_changed.emit();
}

bool Model::get_selected_stl(RFO_Object *&object, RFO_File *&file)
{
  // FIXME: busts model/view separation
  if (!m_view)
    return false;
  return m_view->get_selected_stl (object, file);
}

RFO_File* Model::AddStl(STL stl, string filename)
{
  RFO_File *file, *lastfile;
  RFO_Object *parent;
  get_selected_stl (parent, file);

  if (!parent) {
    if (rfo.Objects.size() <= 0)
      rfo.newObject();
    parent = &rfo.Objects.back();
  }
  g_assert (parent != NULL);

  size_t found = filename.find_last_of("/\\");
  lastfile = NULL;
  if (parent->files.size() > 0)
    lastfile = &parent->files.back();

  Gtk::TreePath path = rfo.createFile (parent, stl, filename.substr(found+1));
  m_signal_stl_added.emit (path);

  file = &parent->files.back();
  if (lastfile) {
    Vector3f p = lastfile->transform3D.transform.getTranslation();
    Vector3f size = lastfile->stl.Max - lastfile->stl.Min;
    p.x += size.x + 5.0f; // 5mm space
    file->transform3D.transform.setTranslation(p);
  }

  CalcBoundingBoxAndCenter();

  return file;
}

void Model::newObject()
{
  rfo.newObject();
}

void Model::setObjectname(string name)
{
  RFO_File *file;
  RFO_Object *object;
  get_selected_stl (object, file);
  if (object)
    object->name = name;
}

void Model::setFileMaterial(string material)
{
  RFO_File *file;
  RFO_Object *object;
  get_selected_stl (object, file);
  if (file)
    file->material = material;
}

void Model::setFileType(string type)
{
  RFO_File *file;
  RFO_Object *object;
  get_selected_stl (object, file);
  if (file)
    file->filetype = type;
}

void Model::setFileLocation(string location)
{
  RFO_File *file;
  RFO_Object *object;
  get_selected_stl (object, file);
  if (file)
    file->location = location;
}

void Model::RotateObject(RFO_File *file, RFO_Object *object, Vector4f rotate)
{
  Vector3f rot(rotate.x, rotate.y, rotate.z);

  if (!file)
    return; // FIXME: rotate entire Objects ...

  file->stl.RotateObject(rot, rotate.w);
  CalcBoundingBoxAndCenter();
}

void Model::OptimizeRotation()
{
//	stl.OptimizeRotation();
  CalcBoundingBoxAndCenter();
}

void Model::MakeRaft(float &z)
{
	vector<InFillHit> HitsBuffer;

	uint LayerNr = 0;
	float size = settings.Raft.Size;

	Vector2f raftMin =  Vector2f(Min.x - size + printOffset.x, Min.y - size + printOffset.y);
	Vector2f raftMax =  Vector2f(Max.x + size + printOffset.x, Max.y + size + printOffset.y);

	Vector2f Center = (Vector2f(Max.x + size, Max.y + size)-Vector2f(Min.x + size, Min.y + size))/2+Vector2f(printOffset.x, printOffset.y);

	float Length = sqrtf(2)*(   ((raftMax.x)>(raftMax.y)? (raftMax.x):(raftMax.y))  -  ((raftMin.x)<(raftMin.y)? (raftMin.x):(raftMin.y))  )/2.0f;	// bbox of object

	float E = 0.0f;
	float rot;

	while(LayerNr < settings.Raft.Phase[0].LayerCount + settings.Raft.Phase[1].LayerCount)
	{
	  Settings::RaftSettings::PhasePropertiesType *props;
	  props = LayerNr < settings.Raft.Phase[0].LayerCount ?
	    &settings.Raft.Phase[0] : &settings.Raft.Phase[1];
	  rot = (props->Rotation+(float)LayerNr * props->RotationPrLayer)/180.0f*M_PI;
		Vector2f InfillDirX(cosf(rot), sinf(rot));
		Vector2f InfillDirY(-InfillDirX.y, InfillDirX.x);

		Vector3f LastPosition;
		bool reverseLines = false;

		Vector2f P1, P2;
		for(float x = -Length ; x < Length ; x+=props->Distance)
		{
			P1 = (InfillDirX *  Length)+(InfillDirY*x) + Center;
			P2 = (InfillDirX * -Length)+(InfillDirY*x) + Center;

			if(reverseLines)
			{
				Vector2f tmp = P1;
				P1 = P2;
				P2 = tmp;
			}

//			glBegin(GL_LINES);
//			glVertex2fv(&P1.x);
//			glVertex2fv(&P2.x);

			// Crop lines to bbox*size
			Vector3f point;
			InFillHit hit;
			HitsBuffer.clear();
			Vector2f P3(raftMin.x, raftMin.y);
			Vector2f P4(raftMin.x, raftMax.y);
//			glVertex2fv(&P3.x);
//			glVertex2fv(&P4.x);
			if(IntersectXY(P1,P2,P3,P4,hit))	//Intersect edges of bbox
				HitsBuffer.push_back(hit);
			P3 = Vector2f(raftMax.x,raftMax.y);
//			glVertex2fv(&P3.x);
//			glVertex2fv(&P4.x);
			if(IntersectXY(P1,P2,P3,P4,hit))
				HitsBuffer.push_back(hit);
			P4 = Vector2f(raftMax.x,raftMin.y);
//			glVertex2fv(&P3.x);
//			glVertex2fv(&P4.x);
			if(IntersectXY(P1,P2,P3,P4,hit))
				HitsBuffer.push_back(hit);
			P3 = Vector2f(raftMin.x,raftMin.y);
//			glVertex2fv(&P3.x);
//			glVertex2fv(&P4.x);
			if(IntersectXY(P1,P2,P3,P4,hit))
				HitsBuffer.push_back(hit);
//			glEnd();

			if(HitsBuffer.size() == 0)	// it can only be 2 or zero
				continue;
			if(HitsBuffer.size() != 2)
				continue;

			std::sort(HitsBuffer.begin(), HitsBuffer.end(), InFillHitCompareFunc);

			P1 = HitsBuffer[0].p;
			P2 = HitsBuffer[1].p;

			MakeAcceleratedGCodeLine (Vector3f(P1.x,P1.y,z), Vector3f(P2.x,P2.y,z),
						  props->MaterialDistanceRatio, gcode,
						  E, z, settings.Slicing, settings.Hardware);
			reverseLines = !reverseLines;
		}
		// Set startspeed for Z-move
		Command g;
		g.Code = SETSPEED;
		g.where = Vector3f(P2.x, P2.y, z);
		g.f=settings.Hardware.MinPrintSpeedZ;
		g.comment = "Move Z";
		g.e = E;
		gcode.commands.push_back(g);
		z += props->Thickness * settings.Hardware.LayerThickness;

		// Move Z
		g.Code = ZMOVE;
		g.where = Vector3f(P2.x, P2.y, z);
		g.f = settings.Hardware.MinPrintSpeedZ;
		g.comment = "Move Z";
		g.e = E;
		gcode.commands.push_back(g);

		LayerNr++;
	}

	// restore the E state
	Command gotoE;
	gotoE.Code = GOTO;
	gotoE.e = 0;
	gotoE.comment = "Reset E for the remaining print";
	gcode.commands.push_back(gotoE);
}

void Model::handle_send(rr_dev device, void *cbdata, void *blkdata, const char *block, size_t len)
{
  Model *instance = static_cast<Model*>(cbdata);
  if (instance->m_printing)
    instance->m_progress.update((unsigned long)(blkdata));
  instance->commlog->insert(instance->commlog->end(), string(block, len));
}

void Model::handle_recv(rr_dev device, void *data, const char *reply, size_t len)
{
  Model *instance = static_cast<Model*>(data);
  instance->commlog->insert (instance->commlog->end(), string (reply, len) + "\n");
}

bool Model::handle_dev_fd(Glib::IOCondition cond)
{
  int result;
  if (cond & Glib::IO_IN) {
    result = rr_handle_readable (m_device);
    if (result < 0) {
      // FIXME: upgrade alert ...
      Gtk::MessageDialog dialog(*m_view, "Error reading from device!", false,
                                Gtk::MESSAGE_ERROR, Gtk::BUTTONS_CLOSE);
      dialog.set_secondary_text(strerror(errno));
      dialog.run();
    }
  }
  if (cond & Glib::IO_OUT) {
    result = rr_handle_writable(m_device);
    if (result < 0) {
      // FIXME: upgrade 'alert' to be able to do this ...
      Gtk::MessageDialog dialog (*m_view, "Error writing to device!", false,
				 Gtk::MESSAGE_ERROR, Gtk::BUTTONS_CLOSE);
      dialog.set_secondary_text(strerror(errno));
      dialog.run();
    }
  }
  return true;
}

void Model::handle_want_writable(rr_dev device, void *data, char state) {
  Model *instance = static_cast<Model*>(data);
  // TODO: Is there a way to merely change the flags to listen on?
  instance->m_devconn.disconnect();
  if (state) {
    instance->m_devconn = Glib::signal_io().connect(sigc::mem_fun(*instance, &Model::handle_dev_fd), rr_dev_fd(device), Glib::IO_IN | Glib::IO_OUT);
  } else {
    instance->m_devconn = Glib::signal_io().connect(sigc::mem_fun(*instance, &Model::handle_dev_fd), rr_dev_fd(device), Glib::IO_IN);
  }
}

void Model::handle_reply(rr_dev device, void *data, rr_reply reply, float value)
{
  Model *model = static_cast<Model *>(data);
  switch(reply) {
  case RR_NOZZLE_TEMP:
    model->m_temps[TEMP_NOZZLE] = value;
    model->m_signal_temp_changed.emit();
    break;
  case RR_BED_TEMP:
    model->m_temps[TEMP_BED] = value;
    model->m_signal_temp_changed.emit();
    break;
  case RR_OK:
    // TODO: Only enqueue a certain number of blocks at a time
    break;
  default:
    break;
  }
}

void Model::ClearLogs()
{
  commlog->set_text("");
  errlog->set_text("");
  echolog->set_text("");
}

void Model::CalcBoundingBoxAndCenter()
{
  Max = Vector3f(G_MINFLOAT, G_MINFLOAT, G_MINFLOAT);
  Min = Vector3f(G_MAXFLOAT, G_MAXFLOAT, G_MAXFLOAT);

  for (uint i = 0 ; i < rfo.Objects.size(); i++) {
    for (uint j = 0; j < rfo.Objects[i].files.size(); j++) {
      Matrix4f M = rfo.GetSTLTransformationMatrix (i, j);
      Vector3f stlMin = M * rfo.Objects[i].files[j].stl.Min;
      Vector3f stlMax = M * rfo.Objects[i].files[j].stl.Max;
      for (uint k = 0; k < 3; k++) {
	Min.xyz[k] = MIN(stlMin.xyz[k], Min.xyz[k]);
	Max.xyz[k] = MAX(stlMax.xyz[k], Max.xyz[k]);
      }
    }
  }

  Center = (Max - Min) / 2.0;

  m_signal_rfo_changed.emit();
}
