/*
    This file is a part of the RepSnapper project.
    Copyright (C) 2010 Michael Meeks

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
#include <cstdlib>
#include <gtkmm.h>
#include "settings.h"
#include <libconfig.h++>

/*
 * How settings are intended to work:
 *
 * The large table below provides pointers into a settings instance, and
 * some simple (POD) type information for the common settings. It also
 * provides the configuration name for each setting, and a Gtk::Builder
 * XML widget name, with which the setting should be associated.
 */

// Allow passing as a pointer to something to
// avoid including glibmm in every header.
class Builder : public Glib::RefPtr<Gtk::Builder>
{
public:
  Builder() {}
  ~Builder() {}
};

#ifdef WIN32
#  define DEFAULT_COM_PORT "COM0"
#else
#  define DEFAULT_COM_PORT "/dev/ttyUSB0"
#endif

#define OFFSET(field) offsetof (class Settings, field)
#define BOOL_MEMBER(field, config_name, def_value) \
  { OFFSET (field), T_BOOL, config_name, #field, def_value, }
#define INT_MEMBER(field, config_name, def_value) \
  { OFFSET (field), T_INT, config_name, #field, def_value, }
#define FLOAT_MEMBER(field, config_name, def_value) \
  { OFFSET (field), T_FLOAT, config_name, #field, def_value, }
#define STRING_MEMBER(field, config_name, def_value) \
  { OFFSET (field), T_STRING, config_name, #field, 0.0, def_value }

// converting our offsets into type pointers
#define PTR_OFFSET(obj, offset)  (((guchar *)obj) + offset)
#define PTR_BOOL(obj, idx)      ((bool *)PTR_OFFSET (obj, settings[idx].member_offset))
#define PTR_INT(obj, idx)       ((int *)PTR_OFFSET (obj, settings[idx].member_offset))
#define PTR_UINT(obj, idx)      ((uint *)PTR_OFFSET (obj, settings[idx].member_offset))
#define PTR_FLOAT(obj, idx)     ((float *)PTR_OFFSET (obj, settings[idx].member_offset))
#define PTR_STRING(obj, idx)    ((std::string *)PTR_OFFSET (obj, settings[idx].member_offset))

enum SettingType { T_BOOL, T_INT, T_FLOAT, T_STRING };
static struct {
  uint  member_offset;
  SettingType type;
  const char *config_name;
  const char *glade_name;
  double def_double;
  const char *def_string;
} settings[] = {
  // Raft:

  BOOL_MEMBER  (RaftEnable, "RaftEnable", false),
  FLOAT_MEMBER (Raft.Size,  "RaftSize",   1.33),
#define FLOAT_PHASE_MEMBER(phase, phasestd, member, def_value) \
  { OFFSET (Raft.Phase[Settings::RaftSettings::PHASE_##phase].member), T_FLOAT, \
    #phasestd #member, #phasestd #member, def_value, }

  // Raft Base
  { OFFSET (Raft.Phase[Settings::RaftSettings::PHASE_BASE].LayerCount), T_INT,
    "BaseLayerCount", "BaseLayerCount", 1, },
  FLOAT_PHASE_MEMBER(BASE, Base, MaterialDistanceRatio, 1.8),
  FLOAT_PHASE_MEMBER(BASE, Base, Rotation, 0.0),
  FLOAT_PHASE_MEMBER(BASE, Base, RotationPrLayer, 90.0),
  FLOAT_PHASE_MEMBER(BASE, Base, Distance, 2.0),
  FLOAT_PHASE_MEMBER(BASE, Base, Thickness, 1.0),
  FLOAT_PHASE_MEMBER(BASE, Base, Temperature, 1.10),

  // Raft Interface
  { OFFSET (Raft.Phase[Settings::RaftSettings::PHASE_INTERFACE].LayerCount), T_INT,
    "InterfaceLayerCount", "InterfaceLayerCount", 2, },
  FLOAT_PHASE_MEMBER(INTERFACE, Interface, MaterialDistanceRatio, 1.0),
  FLOAT_PHASE_MEMBER(INTERFACE, Interface, Rotation, 90.0),
  FLOAT_PHASE_MEMBER(INTERFACE, Interface, RotationPrLayer, 90.0),
  FLOAT_PHASE_MEMBER(INTERFACE, Interface, Distance, 2.0),
  FLOAT_PHASE_MEMBER(INTERFACE, Interface, Thickness, 1.0),
  FLOAT_PHASE_MEMBER(INTERFACE, Interface, Temperature, 1.0),

#undef FLOAT_PHASE_MEMBER

  // Hardware
  FLOAT_MEMBER (Hardware.MinPrintSpeedXY, "MinPrintSpeedXY", 1000),
  FLOAT_MEMBER (Hardware.MaxPrintSpeedXY, "MaxPrintSpeedXY", 4000),
  FLOAT_MEMBER (Hardware.MinPrintSpeedZ,  "MinPrintSpeedZ",  50),
  FLOAT_MEMBER (Hardware.MaxPrintSpeedZ,  "MaxPrintSpeedZ",  150),

  FLOAT_MEMBER (Hardware.DistanceToReachFullSpeed, "DistanceToReachFullSpeed", 1.5),
  FLOAT_MEMBER (Hardware.ExtrusionFactor, "ExtrusionFactor", 1.0),
  FLOAT_MEMBER (Hardware.LayerThickness, "LayerThickness", 0.4),
  FLOAT_MEMBER (Hardware.DownstreamMultiplier, "DownstreamMultiplier", 1.0),
  FLOAT_MEMBER (Hardware.DownstreamExtrusionMultiplier, "DownstreamExtrusionMultiplier", 1.0),

  // Volume
  { OFFSET (Hardware.Volume.x), T_FLOAT, "mfVolumeX", "Hardware.Volume.X", 200 },
  { OFFSET (Hardware.Volume.y), T_FLOAT, "mfVolumeY", "Hardware.Volume.Y", 200 },
  { OFFSET (Hardware.Volume.z), T_FLOAT, "mfVolumeZ", "Hardware.Volume.Z", 140 },
  // PrintMargin
  { OFFSET (Hardware.PrintMargin.x), T_FLOAT, "PrintMarginX", "Hardware.PrintMargin.X", 10 },
  { OFFSET (Hardware.PrintMargin.y), T_FLOAT, "PrintMarginY", "Hardware.PrintMargin.Y", 10 },
  { OFFSET (Hardware.PrintMargin.z), T_FLOAT, "PrintMarginZ", "Hardware.PrintMargin.Z", 0 },

  FLOAT_MEMBER  (Hardware.ExtrudedMaterialWidth, "ExtrudedMaterialWidth", 0.7),
  STRING_MEMBER (Hardware.PortName, "msPortName", DEFAULT_COM_PORT),
  { OFFSET (Hardware.SerialSpeed), T_INT, "miSerialSpeed", NULL, 19200, }, /* 57600 ? */
  BOOL_MEMBER   (Hardware.ValidateConnection, "ValidateConnection", true),
  INT_MEMBER    (Hardware.KeepLines, "KeepLines", 1000),
  INT_MEMBER    (Hardware.ReceivingBufferSize, "ReceivingBufferSize", 4),

  // Slicing
  BOOL_MEMBER  (Slicing.UseIncrementalEcode, "UseIncrementalEcode", true),
  BOOL_MEMBER  (Slicing.Use3DGcode, "Use3DGcode", false),
  BOOL_MEMBER  (Slicing.EnableAntiooze, "EnableAntiooze", false),
  FLOAT_MEMBER (Slicing.AntioozeDistance, "AntioozeDistance", 4.5),
  FLOAT_MEMBER (Slicing.AntioozeSpeed, "AntioozeSpeed", 1000.0),

  FLOAT_MEMBER  (Slicing.InfillDistance, "InFillDistance", 2.0),
  FLOAT_MEMBER  (Slicing.InfillRotation, "InfillRotation", 45.0),
  FLOAT_MEMBER  (Slicing.InfillRotationPrLayer, "InfillRotationPrLayer", 90.0),
  FLOAT_MEMBER  (Slicing.AltInfillDistance, "AltInfillDistance", 2.0),
  STRING_MEMBER (Slicing.AltInfillLayersText, "AltInfillLayersText", ""),

  BOOL_MEMBER   (Slicing.ShellOnly, "ShellOnly", false),
  INT_MEMBER    (Slicing.ShellCount, "ShellCount", 1),
  BOOL_MEMBER   (Slicing.EnableAcceleration, "EnableAcceleration", true),
// ShrinkQuality is a special enumeration ...
  FLOAT_MEMBER  (Slicing.Optimization, "Optimization", 0.02),

  // Misc.
  BOOL_MEMBER (Misc.FileLoggingEnabled, "FileLoggingEnabled", true),
  BOOL_MEMBER (Misc.TempReadingEnabled, "TempReadingEnabled", true),
  BOOL_MEMBER (Misc.ClearLogfilesWhenPrintStarts, "ClearLogfilesWhenPrintStarts", true),

  // GCode - handled by GCodeImpl
  // Display - pending ...
  BOOL_MEMBER (Display.DisplayGCode, "DisplayGCode", true),
  FLOAT_MEMBER (Display.GCodeDrawStart, "GCodeDrawStart", 0.0),
  FLOAT_MEMBER (Display.GCodeDrawEnd, "GCodeDrawEnd", 1.0),
  BOOL_MEMBER (Display.DisplayEndpoints, "DisplayEndpoints", false),
  BOOL_MEMBER (Display.DisplayNormals, "DisplayNormals", false),
  BOOL_MEMBER (Display.DisplayBBox, "DisplayBBox", false),
  BOOL_MEMBER (Display.DisplayWireframe, "DisplayWireframe", false),
  BOOL_MEMBER (Display.DisplayWireframeShaded, "DisplayWireframeShaded", true),
  BOOL_MEMBER (Display.DisplayPolygons, "DisplayPolygons", true),
  BOOL_MEMBER (Display.DisplayAllLayers, "DisplayAllLayers", false),
  BOOL_MEMBER (Display.DisplayinFill, "DisplayinFill", false),
  BOOL_MEMBER (Display.DisplayDebug, "DisplayDebuginFill", false),
  BOOL_MEMBER (Display.DisplayDebug, "DisplayDebug", false),
  BOOL_MEMBER (Display.DisplayCuttingPlane, "DisplayCuttingPlane", false),
  BOOL_MEMBER (Display.DrawVertexNumbers, "DrawVertexNumbers", false),
  BOOL_MEMBER (Display.DrawLineNumbers, "DrawLineNumbers", false),
  BOOL_MEMBER (Display.DrawOutlineNumbers, "DrawOutlineNumbers", false),
  BOOL_MEMBER (Display.DrawCPVertexNumbers, "DrawCPVertexNumbers", false),
  BOOL_MEMBER (Display.DrawCPLineNumbers, "DrawCPLineNumbers", false),
  BOOL_MEMBER (Display.DrawCPOutlineNumbers, "DrawCPOutlineNumbers", false),

  FLOAT_MEMBER (Display.CuttingPlaneValue, "CuttingPlaneValue", 0),
  FLOAT_MEMBER (Display.PolygonOpacity, "PolygonOpacity", 0.5),
  BOOL_MEMBER  (Display.LuminanceShowsSpeed, "LuminanceShowsSpeed", false),
  FLOAT_MEMBER (Display.Highlight, "Highlight", 0.7),
  FLOAT_MEMBER (Display.NormalsLength, "NormalsLength", 10),
  FLOAT_MEMBER (Display.EndPointSize, "EndPointSize", 8),
  FLOAT_MEMBER (Display.TempUpdateSpeed, "TempUpdateSpeed", 3),
};


class Settings::GCodeImpl {
public:
  Glib::RefPtr<Gtk::TextBuffer> m_GCodeStartText;
  Glib::RefPtr<Gtk::TextBuffer> m_GCodeLayerText;
  Glib::RefPtr<Gtk::TextBuffer> m_GCodeEndText;

  GCodeImpl()
  {
    m_GCodeStartText = Gtk::TextBuffer::create();
    m_GCodeLayerText = Gtk::TextBuffer::create();
    m_GCodeEndText = Gtk::TextBuffer::create();
  }
};

void Settings::SlicingSettings::GetAltInfillLayers(std::vector<int>& layers, uint layerCount) const
{
  size_t start = 0, end = AltInfillLayersText.find(',');
  while(start != std::string::npos) {
    int num = atoi(AltInfillLayersText.data() + start);
    if(num < 0) {
      num += layerCount;
    }
    layers.push_back (num);
    
    start = end;
    end = AltInfillLayersText.find(',', start+1);
  }
}

Settings::Settings ()
{
  GCode.m_impl = new GCodeImpl();
  set_defaults();
}

std::string Settings::GCodeType::getStartText()
{
  return m_impl->m_GCodeStartText->get_text();
}

std::string Settings::GCodeType::getLayerText()
{
  return m_impl->m_GCodeLayerText->get_text();
}

std::string Settings::GCodeType::getEndText()
{
  return m_impl->m_GCodeEndText->get_text();
}

Settings::~Settings()
{
}

void Settings::set_defaults ()
{
  for (uint i = 0; i < G_N_ELEMENTS (settings); i++) {
    switch (settings[i].type) {
    case T_BOOL:
      *PTR_BOOL(this, i) = settings[i].def_double != 0.0;
      break;
    case T_INT:
      *PTR_INT(this, i) = settings[i].def_double;
      break;
    case T_FLOAT:
      *PTR_FLOAT(this, i) = settings[i].def_double;
      break;
    case T_STRING:
      *PTR_STRING(this, i) = std::string (settings[i].def_string);
      break;
    default:
      std::cerr << "corrupt setting type\n";
      break;
    }
  }

  Slicing.ShrinkQuality = SHRINK_FAST;

  GCode.m_impl->m_GCodeStartText->set_text
    ("; GCode generated by RepSnapper:\n"
     "; http://reprap.org/wiki/RepSnapper_Manual:Introduction\n"
     "G21             ; metric is good!\n"
     "G90             ; absolute positioning\n"
     "T0              ; select new extruder\n"
     "G28             ; go home\n"
     "G92 E0          ; set extruder home\n"
     "M104 S200.0     ; set temperature to 200.0\n"
     "G1 X20 Y20 F500 ; move away from 0.0, to use the same reset for each layer\n\n");
  GCode.m_impl->m_GCodeLayerText->set_text
    ("");
  GCode.m_impl->m_GCodeEndText->set_text
    ("G1 X0 Y0 F2000.0 ; feed for start of next move\n"
     "M104 S0.0        ; heater off\n");

  Display.PolygonHSV = vmml::Vector3f (0.54, 1, 0.5);
  Display.WireframeHSV = vmml::Vector3f (0.08, 1, 1);
  Display.NormalsHSV = vmml::Vector3f (0.23, 1, 1);
  Display.EndpointsHSV = vmml::Vector3f (0.45, 1, 1);
  Display.GCodeExtrudeHSV = vmml::Vector3f (1, 1, 0.18);
  Display.GCodeMoveHSV = vmml::Vector3f (1, 0.95, 1);

  // FIXME: implement connecting me to the UI !
  Hardware.Volume = vmml::Vector3f (200,200,140);
  Hardware.PrintMargin = vmml::Vector3f (10,10,0);
}

void Settings::load_settings(Builder &builder, Glib::RefPtr<Gio::File> file)
{
  libconfig::Config cfg;

  set_defaults();

  try {
    cfg.readFile(file->get_path().c_str());
    std::cout << "parsing config from '" << file->get_path() << "\n";

    for (uint i = 0; i < G_N_ELEMENTS (settings); i++) {
      const char *name = settings[i].config_name;
      if (!name)
	continue;
      switch (settings[i].type) {
      case T_BOOL:
	cfg.lookupValue (name, *PTR_BOOL(this, i));
	break;
      case T_INT:
	cfg.lookupValue (name, *PTR_INT(this, i));
	break;
      case T_FLOAT:
	cfg.lookupValue (name, *PTR_FLOAT(this, i));
	break;
      case T_STRING:
	cfg.lookupValue (name, *PTR_STRING(this, i));
	break;
      default:
	std::cerr << "corrupt setting type\n";
	break;
      }
    }
  } catch (std::exception &e) {
    std::cout << "Failed to load settings from file '" << file->get_path()
	      << "' - error '" << e.what() << "\n";
  }

  if (cfg.lookupValue("ShrinkFast", Slicing.ShrinkQuality))
    Slicing.ShrinkQuality = SHRINK_FAST;
  else
    Slicing.ShrinkQuality = SHRINK_LOGICK;

  std::string txt;
  if (cfg.lookupValue("GCodeStartText", txt))
    GCode.m_impl->m_GCodeStartText->set_text (txt);
  if (cfg.lookupValue("GCodeLayerText", txt))
    GCode.m_impl->m_GCodeLayerText->set_text (txt);
  if (cfg.lookupValue("GCodeEndText", txt))
    GCode.m_impl->m_GCodeEndText->set_text (txt);

  set_to_gui (builder);
}

void Settings::save_settings(Glib::RefPtr<Gio::File> file)
{
  libconfig::Config   cfg;
  libconfig::Setting &root = cfg.getRoot();

  for (uint i = 0; i < G_N_ELEMENTS (settings); i++) {
    libconfig::Setting::Type t;
    const char *name = settings[i].config_name;

    if (!name)
      continue;

    switch (settings[i].type) {
    case T_BOOL: 
      t = libconfig::Setting::TypeBoolean;
      break;
    case T_INT:
      t = libconfig::Setting::TypeInt;
      break;
    case T_FLOAT:
      t = libconfig::Setting::TypeFloat;
      break;
    case T_STRING:
      t = libconfig::Setting::TypeString;
      break;
    default:
      t = libconfig::Setting::TypeNone;
      std::cerr << "corrupt setting type\n";
      break;
    }

    libconfig::Setting &s = root.add (name, t);
    switch (settings[i].type) {
    case T_BOOL:
      s = *PTR_BOOL(this, i);
      break;
    case T_INT:
      s = *PTR_INT(this, i);
      break;
    case T_FLOAT:
      s = *PTR_FLOAT(this, i);
      break;
    case T_STRING:
      s = *PTR_STRING(this, i);
      break;
    default:
      break;
    };
  }

  // this is pretty unpleasant:
  root.add ("ShrinkFast", libconfig::Setting::TypeBoolean) = (Slicing.ShrinkQuality == SHRINK_FAST);

  root.add("GCodeStartText", libconfig::Setting::TypeString) = GCode.getStartText();
  root.add("GCodeLayerText", libconfig::Setting::TypeString) = GCode.getLayerText();
  root.add("GCodeEndText", libconfig::Setting::TypeString) = GCode.getEndText();

  cfg.writeFile(file->get_path().c_str());
}

static struct {
  const char *widget;
  double min, max;
  double inc, inc_page;
} ranges[] = {
  // Raft
  { "Raft.Size", 0, 50, 1, 3 },
  { "BaseLayerCount",      0, 8, 1, 2 },
  { "InterfaceLayerCount", 0, 8, 1, 2 },
  { "BaseMaterialDistanceRatio",      0.1, 4.0, 0.1, 1 },
  { "InterfaceMaterialDistanceRatio", 0.1, 4.0, 0.1, 1 },
  { "BaseRotation",             -360.0, 360.0, 45, 90 },
  { "InterfaceRotation",        -360.0, 360.0, 45, 90 },
  { "BaseRotationPrLayer",      -360.0, 360.0, 45, 90 },
  { "InterfaceRotationPrLayer", -360.0, 360.0, 45, 90 },
  { "BaseDistance",      0.1, 8.0, 0.1, 1 },
  { "InterfaceDistance", 0.1, 8.0, 0.1, 1 },
  { "BaseThickness",      0.1, 4.0, 0.1, 1 },
  { "InterfaceThickness", 0.1, 4.0, 0.1, 1 },
  { "BaseTemperature",      0.9, 1.2, 0.01, 0.1 },
  { "InterfaceTemperature", 0.9, 1.2, 0.01, 0.1 },

  // Slicing
  { "Slicing.ShellCount", 0, 100, 1, 5 },
  { "Slicing.InfillRotation", -360, 360, 5, 45 },
  { "Slicing.InfillRotationPrLayer", -360, 360, 5, 90 },
  { "Slicing.InfillDistance", 0, 10, 0.1, 1 },
  { "Slicing.AltInfillDistance", 0, 10, 0.1, 2 },
  { "Slicing.InfillDistance", 0, 10, 0.1, 1 },
  { "Slicing.Optimization", 0.0, 1.0, 0.01, 0.1 },
  { "Slicing.AntioozeDistance", 0.0, 25.0, 0.1, 1 },
  { "Slicing.AntioozeSpeed", 0.0, 10000.0, 25.0, 100.0 },

  // Hardware
  { "Hardware.Volume.X", 0.0, 1000.0, 5.0, 25.0 },
  { "Hardware.Volume.Y", 0.0, 1000.0, 5.0, 25.0 },
  { "Hardware.Volume.Z", 0.0, 1000.0, 5.0, 25.0 },
  { "Hardware.PrintMargin.X", 0.0, 100.0, 1.0, 5.0 },
  { "Hardware.PrintMargin.Y", 0.0, 100.0, 1.0, 5.0 },
  { "Hardware.PrintMargin.Z", 0.0, 100.0, 1.0, 5.0 },
  { "Hardware.DistanceToReachFullSpeed", 0.0, 10.0, 0.1, 1.0 },
  { "Hardware.ExtrudedMaterialWidth", 0.0, 10.0, 0.01, 0.1 },
  { "Hardware.LayerThickness", 0.1, 3.0, 0.1, 0.2 },
  { "Hardware.ExtrusionFactor", 0.0, 2.0, 0.1, 0.2 },

  { "Hardware.MinPrintSpeedXY", 1.0, 8000.0, 10.0, 100.0 },
  { "Hardware.MaxPrintSpeedXY", 1.0, 8000.0, 10.0, 100.0 },
  { "Hardware.MinPrintSpeedZ", 1.0, 2500.0, 10.0, 100.0 },
  { "Hardware.MaxPrintSpeedZ", 1.0, 2500.0, 10.0, 100.0 },

  { "Hardware.ReceivingBufferSize", 1.0, 100.0, 1.0, 5.0 },
  { "Hardware.KeepLines", 100.0, 100000.0, 1.0, 500.0 },
};

void Settings::set_to_gui (Builder &builder, int i)
{
  const char *glade_name = settings[i].glade_name;

  switch (settings[i].type) {
  case T_BOOL: {
    Gtk::CheckButton *check = NULL;
    builder->get_widget (glade_name, check);
    if (!check)
      std::cerr << "Missing boolean config item " << glade_name << "\n";
    else
      check->set_active (*PTR_BOOL(this, i));
    break;
  }
  case T_INT:
  case T_FLOAT: {
    Gtk::SpinButton *spin = NULL;
    builder->get_widget (glade_name, spin);
    if (!spin)
      std::cerr << "Missing spin button config item " << glade_name << "\n";
    else if (settings[i].type == T_INT)
      spin->set_value (*PTR_INT(this, i));
    else
      spin->set_value (*PTR_FLOAT(this, i));
    break;
  }
  case T_STRING:
    std::cerr << "string unimplemented " << glade_name << "\n";
    break;
  default:
    std::cerr << "corrupt setting type\n";
    break;
  }
}

void Settings::set_shrink_to_gui (Builder &builder)
{
  // Slicing.ShrinkQuality
  Gtk::ComboBox *combo = NULL;
  builder->get_widget ("Slicing.ShrinkQuality", combo);
  if (combo)
    combo->set_active (Slicing.ShrinkQuality);
}

void Settings::get_from_gui (Builder &builder, int i)
{
  const char *glade_name = settings[i].glade_name;

  switch (settings[i].type) {
  case T_BOOL: {
    Gtk::CheckButton *check = NULL;
    builder->get_widget (glade_name, check);
    if (!check)
      std::cerr << "Missing boolean config item " << glade_name << "\n";
    else
      *PTR_BOOL(this, i) = check->get_active();
    break;
  }
  case T_INT:
  case T_FLOAT: {
    Gtk::SpinButton *spin = NULL;
    builder->get_widget (glade_name, spin);
    if (!spin)
      std::cerr << "Missing, or different type button config item " << glade_name << "\n";
    else if (settings[i].type == T_INT)
      *PTR_INT(this, i) = spin->get_value();
    else
      *PTR_FLOAT(this, i) = spin->get_value();
    break;
  }
  case T_STRING:
    std::cerr << "string get from gui unimplemented " << glade_name << "\n";
    break;
  default:
    std::cerr << "corrupt setting type\n";
    break;
  }
}

void Settings::get_shrink_from_gui (Builder &builder)
{
  // Slicing.ShrinkQuality
  Gtk::ComboBox *combo = NULL;
  builder->get_widget ("Slicing.ShrinkQuality", combo);
  if (combo)
    Slicing.ShrinkQuality = combo->get_active_row_number ();
}

void Settings::set_to_gui (Builder &builder)
{
  for (uint i = 0; i < G_N_ELEMENTS (settings); i++) {
    const char *glade_name = settings[i].glade_name;

    if (!glade_name)
      continue;

    set_to_gui (builder, i);
  }
  set_shrink_to_gui (builder);
}

void Settings::connectToUI (Builder &builder)
{
  Gtk::TextView *textv = NULL;
  builder->get_widget ("txt_gcode_start", textv);
  textv->set_buffer (GCode.m_impl->m_GCodeStartText);
  builder->get_widget ("txt_gcode_end", textv);
  textv->set_buffer (GCode.m_impl->m_GCodeEndText);
  builder->get_widget ("txt_gcode_next_layer", textv);
  textv->set_buffer (GCode.m_impl->m_GCodeLayerText);

  // first setup ranges on spinbuttons ...
  for (uint i = 0; i < G_N_ELEMENTS (ranges); i++) {
    Gtk::SpinButton *spin = NULL;
    builder->get_widget (ranges[i].widget, spin);
    if (!spin)
      std::cerr << "missing spin button of name '" << ranges[i].widget << "'\n";
    else {
      spin->set_range (ranges[i].min, ranges[i].max);
      spin->set_increments (ranges[i].inc, ranges[i].inc_page);
    }
  }

  // connect widget / values from our table
  for (uint i = 0; i < G_N_ELEMENTS (settings); i++) {
    const char *glade_name = settings[i].glade_name;

    if (!glade_name)
      continue;

    set_to_gui (builder, i);

    switch (settings[i].type) {
    case T_BOOL: {
      Gtk::CheckButton *check = NULL;
      builder->get_widget (glade_name, check);
      if (check)
	check->signal_toggled().connect
          (sigc::bind(sigc::bind(sigc::mem_fun(*this, &Settings::get_from_gui), i), builder));
      break;
    }
    case T_INT:
    case T_FLOAT: {
      Gtk::SpinButton *spin = NULL;
      builder->get_widget (glade_name, spin);
      if (spin)
	spin->signal_value_changed().connect
          (sigc::bind(sigc::bind(sigc::mem_fun(*this, &Settings::get_from_gui), i), builder));
	break;
    }
    case T_STRING: // unimplemented
      break;
    default:
      break;
    }
  }

  // Slicing.ShrinkQuality
  Gtk::ComboBox *combo = NULL;
  builder->get_widget ("Slicing.ShrinkQuality", combo);
  if (combo) {
    Glib::RefPtr<Gtk::ListStore> model;
    Gtk::TreeModelColumnRecord record;
    Gtk::TreeModelColumn<Glib::ustring> column;
    record.add (column);
    model = Gtk::ListStore::create(record);
    model->append()->set_value (0, Glib::ustring("Fast"));
    model->append()->set_value (0, Glib::ustring("Logick"));
    combo->set_model (model);
    combo->pack_start (column);

    set_shrink_to_gui (builder);

    combo->signal_changed().connect
      (sigc::bind(sigc::mem_fun(*this, &Settings::get_shrink_from_gui), builder));
  }
}
