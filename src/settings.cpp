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

#include <stdafx.h>

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
#define BOOL_MEMBER(field, config_name, def_value, redraw ) \
  { OFFSET (field), T_BOOL, config_name, #field, def_value, NULL, redraw }
#define INT_MEMBER(field, config_name, def_value, redraw) \
  { OFFSET (field), T_INT, config_name, #field, def_value, NULL, redraw }
#define FLOAT_MEMBER(field, config_name, def_value, redraw) \
  { OFFSET (field), T_FLOAT, config_name, #field, def_value, NULL, redraw }
#define STRING_MEMBER(field, config_name, def_value, redraw) \
  { OFFSET (field), T_STRING, config_name, #field, 0.0, def_value, redraw }

#define COLOUR_MEMBER(field, config_name, def_valueR, def_valueG, def_valueB, def_valueA, redraw) \
  { OFFSET (field.r), T_COLOUR_MEMBER, config_name "R", NULL, def_valueR, NULL, redraw }, \
  { OFFSET (field.g), T_COLOUR_MEMBER, config_name "G", NULL, def_valueG, NULL, redraw }, \
  { OFFSET (field.b), T_COLOUR_MEMBER, config_name "B", NULL, def_valueB, NULL, redraw }, \
  { OFFSET (field.a), T_COLOUR_MEMBER, config_name "A", NULL, def_valueA, NULL, redraw }

#define FLOAT_PHASE_MEMBER(phase, phasestd, member, def_value, redraw) \
  { OFFSET (Raft.Phase[Settings::RaftSettings::PHASE_##phase].member), T_FLOAT, \
    #phasestd #member, #phasestd #member, def_value, NULL, redraw }

// converting our offsets into type pointers
#define PTR_OFFSET(obj, offset)  (((guchar *)obj) + offset)
#define PTR_BOOL(obj, idx)      ((bool *)PTR_OFFSET (obj, settings[idx].member_offset))
#define PTR_INT(obj, idx)       ((int *)PTR_OFFSET (obj, settings[idx].member_offset))
#define PTR_UINT(obj, idx)      ((uint *)PTR_OFFSET (obj, settings[idx].member_offset))
#define PTR_FLOAT(obj, idx)     ((float *)PTR_OFFSET (obj, settings[idx].member_offset))
#define PTR_STRING(obj, idx)    ((std::string *)PTR_OFFSET (obj, settings[idx].member_offset))
#define PTR_COLOUR(obj, idx)    ((vmml::Vector4f *)PTR_OFFSET (obj, settings[idx].member_offset))

enum SettingType { T_BOOL, T_INT, T_FLOAT, T_STRING, T_COLOUR_MEMBER };
static struct {
  uint  member_offset;
  SettingType type;
  const char *config_name; // This is obsolete - we split and re-use the glade_name
  const char *glade_name;
  float def_float;
  const char *def_string;
  gboolean triggers_redraw;
} settings[] = {
  // Raft:

  BOOL_MEMBER  (RaftEnable, "RaftEnable", false, true),
  FLOAT_MEMBER (Raft.Size,  "RaftSize",   1.33, true),

  // Raft Base
  { OFFSET (Raft.Phase[Settings::RaftSettings::PHASE_BASE].LayerCount), T_INT,
    "BaseLayerCount", "BaseLayerCount", 1, NULL, false},
  FLOAT_PHASE_MEMBER(BASE, Base, MaterialDistanceRatio, 1.8, false),
  FLOAT_PHASE_MEMBER(BASE, Base, Rotation, 0.0, false),
  FLOAT_PHASE_MEMBER(BASE, Base, RotationPrLayer, 90.0, false),
  FLOAT_PHASE_MEMBER(BASE, Base, Distance, 2.0, false),
  FLOAT_PHASE_MEMBER(BASE, Base, Thickness, 1.0, false),
  FLOAT_PHASE_MEMBER(BASE, Base, Temperature, 1.10, false),

  // Raft Interface
  { OFFSET (Raft.Phase[Settings::RaftSettings::PHASE_INTERFACE].LayerCount), T_INT,
    "InterfaceLayerCount", "InterfaceLayerCount", 2, false },
  FLOAT_PHASE_MEMBER(INTERFACE, Interface, MaterialDistanceRatio, 1.0, false),
  FLOAT_PHASE_MEMBER(INTERFACE, Interface, Rotation, 90.0, false),
  FLOAT_PHASE_MEMBER(INTERFACE, Interface, RotationPrLayer, 90.0, false),
  FLOAT_PHASE_MEMBER(INTERFACE, Interface, Distance, 2.0, false),
  FLOAT_PHASE_MEMBER(INTERFACE, Interface, Thickness, 1.0, false),
  FLOAT_PHASE_MEMBER(INTERFACE, Interface, Temperature, 1.0, false),

#undef FLOAT_PHASE_MEMBER

  // Hardware
  BOOL_MEMBER  (Hardware.CalibrateInput,  "CalibrateInput",  false, false),
  FLOAT_MEMBER (Hardware.MinPrintSpeedXY, "MinPrintSpeedXY", 1000, false),
  FLOAT_MEMBER (Hardware.MaxPrintSpeedXY, "MaxPrintSpeedXY", 4000, false),
  FLOAT_MEMBER (Hardware.MinPrintSpeedZ,  "MinPrintSpeedZ",  50, false),
  FLOAT_MEMBER (Hardware.MaxPrintSpeedZ,  "MaxPrintSpeedZ",  150, false),

  FLOAT_MEMBER (Hardware.DistanceToReachFullSpeed, "DistanceToReachFullSpeed", 1.5, false),
  FLOAT_MEMBER (Hardware.ExtrusionFactor, "ExtrusionFactor", 1.0, false),
  FLOAT_MEMBER (Hardware.FilamentDiameter, "FilamentDiameter", 3.0, false),
  FLOAT_MEMBER (Hardware.LayerThickness, "LayerThickness", 0.4, false),
  FLOAT_MEMBER (Hardware.DownstreamMultiplier, "DownstreamMultiplier", 1.0, false),
  FLOAT_MEMBER (Hardware.DownstreamExtrusionMultiplier, "DownstreamExtrusionMultiplier", 1.0, false),

  // Volume
  { OFFSET (Hardware.Volume.x), T_FLOAT, "mfVolumeX", "Hardware.Volume.X", 200, NULL, true },
  { OFFSET (Hardware.Volume.y), T_FLOAT, "mfVolumeY", "Hardware.Volume.Y", 200, NULL, true },
  { OFFSET (Hardware.Volume.z), T_FLOAT, "mfVolumeZ", "Hardware.Volume.Z", 140, NULL, true },
  // PrintMargin
  { OFFSET (Hardware.PrintMargin.x), T_FLOAT, "PrintMarginX", "Hardware.PrintMargin.X", 10, NULL, true },
  { OFFSET (Hardware.PrintMargin.y), T_FLOAT, "PrintMarginY", "Hardware.PrintMargin.Y", 10, NULL, true },
  { OFFSET (Hardware.PrintMargin.z), T_FLOAT, "PrintMarginZ", "Hardware.PrintMargin.Z", 0, NULL, true },

  FLOAT_MEMBER  (Hardware.ExtrudedMaterialWidth, "ExtrudedMaterialWidth", 0.7, false),
  { OFFSET (Hardware.PortName), T_STRING, "Hardware.PortName", NULL, 0, DEFAULT_COM_PORT, false },
  { OFFSET (Hardware.SerialSpeed), T_INT, "Hardware.SerialSpeed", NULL, 19200, false }, /* 57600 ? */
  BOOL_MEMBER   (Hardware.ValidateConnection, "ValidateConnection", true, false),
  INT_MEMBER    (Hardware.KeepLines, "KeepLines", 1000, false),
  INT_MEMBER    (Hardware.ReceivingBufferSize, "ReceivingBufferSize", 4, false),

  // Slicing
  BOOL_MEMBER  (Slicing.UseIncrementalEcode, "UseIncrementalEcode", true, false),
  BOOL_MEMBER  (Slicing.Use3DGcode, "Use3DGcode", false, false),
  BOOL_MEMBER  (Slicing.EnableAntiooze, "EnableAntiooze", false, false),
  FLOAT_MEMBER (Slicing.AntioozeDistance, "AntioozeDistance", 4.5, false),
  FLOAT_MEMBER (Slicing.AntioozeSpeed, "AntioozeSpeed", 1000.0, false),

  FLOAT_MEMBER  (Slicing.InfillDistance, "InfillDistance", 2.0, false),
  FLOAT_MEMBER  (Slicing.InfillRotation, "InfillRotation", 45.0, false),
  FLOAT_MEMBER  (Slicing.InfillRotationPrLayer, "InfillRotationPrLayer", 90.0, false),
  FLOAT_MEMBER  (Slicing.AltInfillDistance, "AltInfillDistance", 2.0, false),
  STRING_MEMBER (Slicing.AltInfillLayersText, "AltInfillLayersText", "", false),
  BOOL_MEMBER   (Slicing.SolidTopAndBottom, "SolidTopAndBottom", true, false),

  BOOL_MEMBER   (Slicing.ShellOnly, "ShellOnly", false, true),
  INT_MEMBER    (Slicing.ShellCount, "ShellCount", 1, true),
  BOOL_MEMBER   (Slicing.EnableAcceleration, "EnableAcceleration", true, false),
// ShrinkQuality is a special enumeration ...
  INT_MEMBER    (Slicing.ShrinkQuality, "ShrinkQuality", 0, true),
  FLOAT_MEMBER  (Slicing.Optimization, "Optimization", 0.02, false),

  // Misc.
  BOOL_MEMBER (Misc.FileLoggingEnabled, "FileLoggingEnabled", true, false),
  BOOL_MEMBER (Misc.TempReadingEnabled, "TempReadingEnabled", true, false),
  BOOL_MEMBER (Misc.ClearLogfilesWhenPrintStarts, "ClearLogfilesWhenPrintStarts", true, false),

  // GCode - handled by GCodeImpl
  BOOL_MEMBER (Display.DisplayGCode, "DisplayGCode", true, true),
  FLOAT_MEMBER (Display.GCodeDrawStart, "GCodeDrawStart", 0.0, true),
  FLOAT_MEMBER (Display.GCodeDrawEnd, "GCodeDrawEnd", 1.0, true),
  BOOL_MEMBER (Display.DisplayEndpoints, "DisplayEndpoints", false, true),
  BOOL_MEMBER (Display.DisplayNormals, "DisplayNormals", false, true),
  BOOL_MEMBER (Display.DisplayBBox, "DisplayBBox", false, true),
  BOOL_MEMBER (Display.DisplayWireframe, "DisplayWireframe", false, true),
  BOOL_MEMBER (Display.DisplayWireframeShaded, "DisplayWireframeShaded", true, true),
  BOOL_MEMBER (Display.DisplayPolygons, "DisplayPolygons", true, true),
  BOOL_MEMBER (Display.DisplayAllLayers, "DisplayAllLayers", false, true),
  BOOL_MEMBER (Display.DisplayinFill, "DisplayinFill", false, true),
  BOOL_MEMBER (Display.DisplayDebuginFill, "DisplayDebuginFill", false, false),
  BOOL_MEMBER (Display.DisplayDebug, "DisplayDebug", false, true),
  BOOL_MEMBER (Display.CommsDebug, "CommsDebug", false, true),
  BOOL_MEMBER (Display.DisplayCuttingPlane, "DisplayCuttingPlane", false, true),
  BOOL_MEMBER (Display.DrawVertexNumbers, "DrawVertexNumbers", false, true),
  BOOL_MEMBER (Display.DrawLineNumbers, "DrawLineNumbers", false, true),
  BOOL_MEMBER (Display.DrawOutlineNumbers, "DrawOutlineNumbers", false, true),
  BOOL_MEMBER (Display.DrawCPVertexNumbers, "DrawCPVertexNumbers", false, true),
  BOOL_MEMBER (Display.DrawCPLineNumbers, "DrawCPLineNumbers", false, true),
  BOOL_MEMBER (Display.DrawCPOutlineNumbers, "DrawCPOutlineNumbers", false, true),

  FLOAT_MEMBER (Display.CuttingPlaneValue, "CuttingPlaneValue", 0, true),
  BOOL_MEMBER  (Display.LuminanceShowsSpeed, "LuminanceShowsSpeed", false, true),
  FLOAT_MEMBER (Display.Highlight, "Highlight", 0.7, true),
  FLOAT_MEMBER (Display.NormalsLength, "NormalsLength", 10, true),
  FLOAT_MEMBER (Display.EndPointSize, "EndPointSize", 8, true),
  FLOAT_MEMBER (Display.TempUpdateSpeed, "TempUpdateSpeed", 3, false),

  // Colour selectors settings
  COLOUR_MEMBER(Display.PolygonRGBA,
      "Display.PolygonColour", 0, 1.0, 1.0, 0.5, true),
  COLOUR_MEMBER(Display.WireframeRGBA,
      "Display.WireframeColour", 1.0, 0.48, 0, 0.5, true),
  COLOUR_MEMBER(Display.NormalsRGBA,
      "Display.NormalsColour", 0.62, 1.0, 0, 1.0, true),
  COLOUR_MEMBER(Display.EndpointsRGBA,
      "Display.EndpointsColour", 0, 1.0, 0.7, 1.0, true),
  COLOUR_MEMBER(Display.GCodeExtrudeRGBA,
      "Display.GCodeExtrudeColour", 1.0, 1.0, 0.0, 1.0, true),
  COLOUR_MEMBER(Display.GCodeMoveRGBA,
      "Display.GCodeMoveColour", 1.0, 0.05, 1, 0.5, true),
};

// Add any GtkSpinButtons to this array:
static struct {
  const char *widget;
  float min, max;
  float inc, inc_page;
} spin_ranges[] = {
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
  { "Slicing.InfillDistance", 0.1, 10, 0.1, 1 },
  { "Slicing.AltInfillDistance", 0.1, 10, 0.1, 2 },
  { "Slicing.Optimization", 0.0, 1.0, 0.001, 0.1 },
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
  { "Hardware.LayerThickness", 0.01, 3.0, 0.01, 0.2 },
  { "Hardware.ExtrusionFactor", 0.0, 2.0, 0.1, 0.2 },
  { "Hardware.FilamentDiameter", 0.5, 5.0, 0.01, 0.05 },

  { "Hardware.MinPrintSpeedXY", 1.0, 8000.0, 10.0, 100.0 },
  { "Hardware.MaxPrintSpeedXY", 1.0, 8000.0, 10.0, 100.0 },
  { "Hardware.MinPrintSpeedZ", 1.0, 2500.0, 10.0, 100.0 },
  { "Hardware.MaxPrintSpeedZ", 1.0, 2500.0, 10.0, 100.0 },

  { "Hardware.ReceivingBufferSize", 1.0, 100.0, 1.0, 5.0 },
  { "Hardware.KeepLines", 100.0, 100000.0, 1.0, 500.0 },

  { "Hardware.DownstreamMultiplier", 0.01, 25.0, 0.01, 0.1 },
  { "Hardware.DownstreamExtrusionMultiplier", 0.01, 25.0, 0.01, 0.1 },

  // Display pane
  { "Display.TempUpdateSpeed", 0.1, 10.0, 0.5, 1.0 },
};

// Add any [HV]Ranges to this array:
static struct {
  const char *widget;
  float min, max;
  float inc, inc_page;
} ranges[] = {
  // Display plane
  { "Display.CuttingPlaneValue", 0.0, 1.0, 0.0, 0.01 },
  { "Display.GCodeDrawStart", 0.0, 1.0, 0.0, 0.1 },
  { "Display.GCodeDrawEnd", 0.0, 1.0, 0.0, 0.1 },
};

static struct {
  uint  member_offset;
  const char *glade_name;
}
colour_selectors[] = {
  { OFFSET(Display.PolygonRGBA), "Display.PolygonRGB" },
  { OFFSET(Display.WireframeRGBA), "Display.WireframeRGB" },
  { OFFSET(Display.NormalsRGBA), "Display.NormalsRGB" },
  { OFFSET(Display.EndpointsRGBA), "Display.EndpointsRGB" },
  { OFFSET(Display.GCodeExtrudeRGBA), "Display.GCodeExtrudeRGB" },
  { OFFSET(Display.GCodeMoveRGBA), "Display.GCodeMoveRGB" }
};

static const char *GCodeNames[] = { "Start", "Layer", "End" };

class Settings::GCodeImpl {
public:
  Glib::RefPtr<Gtk::TextBuffer> m_GCode[GCODE_TEXT_TYPE_COUNT];

  GCodeImpl()
  {
    for (guint i = 0; i < GCODE_TEXT_TYPE_COUNT; i++)
      m_GCode[i] = Gtk::TextBuffer::create();
  }
  void loadSettings(Glib::KeyFile &cfg)
  {
    for (guint i = 0; i < GCODE_TEXT_TYPE_COUNT; i++)
    {
      if (cfg.has_key ("GCode", GCodeNames[i]))
	m_GCode[i]->set_text(cfg.get_string ("GCode", GCodeNames[i]));
    }
  }
  void saveSettings(Glib::KeyFile &cfg)
  {
    for (guint i = 0; i < GCODE_TEXT_TYPE_COUNT; i++)
      cfg.set_string ("GCode", GCodeNames[i], m_GCode[i]->get_text());
  }
  void connectToUI(Builder &builder)
  {
    static const char *ui_names[] =
      { "txt_gcode_start", "txt_gcode_next_layer", "txt_gcode_end" };
    for (guint i = 0; i < GCODE_TEXT_TYPE_COUNT; i++) {
      Gtk::TextView *textv = NULL;
      builder->get_widget (ui_names [i], textv);
      if (textv)
	textv->set_buffer (m_GCode[i]);
    }
  }
  void setDefaults()
  {
    m_GCode[GCODE_TEXT_START]->set_text
      ("; GCode generated by RepSnapper:\n"
       "; http://reprap.org/wiki/RepSnapper_Manual:Introduction\n"
       "G21             ; metric is good!\n"
       "G90             ; absolute positioning\n"
       "T0              ; select new extruder\n"
       "G28             ; go home\n"
       "G92 E0          ; set extruder home\n"
       "M104 S200.0     ; set temperature to 200.0\n"
       "G1 X20 Y20 F500 ; move away from 0.0, to use the same reset for each layer\n\n");
    m_GCode[GCODE_TEXT_LAYER]->set_text ("");
    m_GCode[GCODE_TEXT_END]->set_text
      ("G1 X0 Y0 F2000.0 ; feed for start of next move\n"
       "M104 S0.0        ; heater off\n");
  }
};

std::string Settings::GCodeType::getText(GCodeTextType t)
{
  return m_impl->m_GCode[t]->get_text();
}

void Settings::SlicingSettings::GetAltInfillLayers(std::vector<int>& layers, uint layerCount) const
{
  size_t start = 0, end = AltInfillLayersText.find(',');

  if (AltInfillLayersText == "")
    return;

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

Settings::~Settings()
{
}

void Settings::set_defaults ()
{
  for (uint i = 0; i < G_N_ELEMENTS (settings); i++) {
    switch (settings[i].type) {
    case T_BOOL:
      *PTR_BOOL(this, i) = settings[i].def_float != 0.0;
      break;
    case T_INT:
      *PTR_INT(this, i) = settings[i].def_float;
      break;
    case T_FLOAT:
    case T_COLOUR_MEMBER:
      *PTR_FLOAT(this, i) = settings[i].def_float;
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

  GCode.m_impl->setDefaults();

  // The vectors map each to 3 spin boxes, one per dimension
  Hardware.Volume = vmml::Vector3d (200,200,140);
  Hardware.PrintMargin = vmml::Vector3d (10,10,0);
}

bool Settings::get_group_and_key (int i, Glib::ustring &group, Glib::ustring &key)
{
  const char *name = settings[i].config_name;
  if (!name) {
    std::cerr << "Odd - some useful use of this field for setting " << i << "\n";
    return false;
  }
  // re-use the display name
  char *field = g_strdup (settings[i].glade_name);
  if (!field)
    field = g_strdup (settings[i].config_name);
  char *p = strchr (field, '.');
  if (!p) {
    group = "Global";
    key = field;
  } else {
    *p = '\0';
    group = field;
    key = p + 1;
  }
  g_free (field);

  return true;
}

void Settings::load_settings (Glib::RefPtr<Gio::File> file)
{
  Glib::KeyFile cfg;

  set_defaults();

  try {
    if (!cfg.load_from_file (file->get_path())) {
      std::cout << "Failed to load settings from file '" << file->get_path() << "\n";
      return;
    }
  } catch (const Glib::KeyFileError &err) {
    std::cout << "Exception " << err.what() << " loading settings from file '" << file->get_path() << "\n";
    return;
  }

  std::cout << "parsing config from '" << file->get_path() << "\n";

  for (uint i = 0; i < G_N_ELEMENTS (settings); i++) {
    Glib::ustring group, key;

    if (!get_group_and_key (i, group, key))
      continue;

    if (!cfg.has_key (group, key))
      continue;

    // group & string ...
    switch (settings[i].type) {
    case T_BOOL:
      *PTR_BOOL(this, i) = cfg.get_boolean (group, key);
      break;
    case T_INT:
      *PTR_INT(this, i) = cfg.get_integer (group, key);
      break;
    case T_FLOAT:
    case T_COLOUR_MEMBER:
      *PTR_FLOAT(this, i) = cfg.get_double (group, key);
      break;
    case T_STRING:
      *PTR_STRING(this, i) = cfg.get_string (group, key);
      break;
    default:
      std::cerr << "corrupt setting type\n";
      break;
    }
  }

  GCode.m_impl->loadSettings (cfg);

  m_signal_visual_settings_changed.emit();
  m_signal_update_settings_gui.emit();
}

void Settings::save_settings(Glib::RefPtr<Gio::File> file)
{
  Glib::KeyFile cfg;

  for (uint i = 0; i < G_N_ELEMENTS (settings); i++) {
    Glib::ustring group, key;

    if (!get_group_and_key (i, group, key))
      continue;

    switch (settings[i].type) {
    case T_BOOL:
      cfg.set_boolean (group, key, *PTR_BOOL(this, i));
      break;
    case T_INT:
      cfg.set_integer (group, key, *PTR_INT(this, i));
      break;
    case T_FLOAT:
    case T_COLOUR_MEMBER:
      cfg.set_double (group, key, *PTR_FLOAT(this, i));
      break;
    case T_STRING:
      cfg.set_string (group, key, *PTR_STRING(this, i));
      break;
    default:
      std::cerr << "Can't save setting of unknown type\n";
      break;
    };
  }

  GCode.m_impl->saveSettings (cfg);

  Glib::ustring contents = cfg.to_data();
  Glib::file_set_contents (file->get_path(), contents);
}


void Settings::set_to_gui (Builder &builder, int i)
{
  const char *glade_name = settings[i].glade_name;

  if (!glade_name)
        return;

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
    Gtk::Widget *w = NULL;
    builder->get_widget (glade_name, w);
    if (!w) {
      std::cerr << "Missing user interface item " << glade_name << "\n";
      break;
    }

    Gtk::SpinButton *spin = dynamic_cast<Gtk::SpinButton *>(w);
    if (spin) {
      if (settings[i].type == T_INT)
          spin->set_value (*PTR_INT(this, i));
      else
          spin->set_value (*PTR_FLOAT(this, i));
      break;
    }
    Gtk::Range *range = dynamic_cast<Gtk::Range *>(w);
    if (range) {
      if (settings[i].type == T_INT)
        range->set_value (*PTR_INT(this, i));
      else
        range->set_value (*PTR_FLOAT(this, i));
    }
    break;
  }
  case T_STRING: {
    Gtk::Entry *e = NULL;
    builder->get_widget (glade_name, e);
    if (!e) {
      std::cerr << "Missing user interface item " << glade_name << "\n";
      break;
    }
    e->set_text(*PTR_STRING(this, i));
    break;
  }
  case T_COLOUR_MEMBER:
    break; // Ignore, Colour members are special 
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

  if (glade_name == NULL)
        return; /* Not an automatically connected setting */

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
    Gtk::Widget *w = NULL;
    builder->get_widget (glade_name, w);
    if (!w) {
      std::cerr << "Missing GUI element " << glade_name << "\n";
      break;
    }

    Gtk::SpinButton *spin = dynamic_cast<Gtk::SpinButton *>(w);
    if (spin) {
      if (settings[i].type == T_INT)
          *PTR_INT(this, i) = spin->get_value();
      else
          *PTR_FLOAT(this, i) = spin->get_value();
      break;
    }

    Gtk::Range *range = dynamic_cast<Gtk::Range *>(w);
    if (range) {
      if (settings[i].type == T_INT)
          *PTR_INT(this, i) = range->get_value();
      else
          *PTR_FLOAT(this, i) = range->get_value();
    }
    break;
  }
  case T_STRING: {
    Gtk::Entry *e = NULL;
    builder->get_widget (glade_name, e);
    if (!e) {
      std::cerr << "Missing user interface item " << glade_name << "\n";
      break;
    }
    *PTR_STRING(this, i) = std::string(e->get_text());
    break;
  }
  case T_COLOUR_MEMBER:
    // Ignore, colour members are special
    break;
  default:
    std::cerr << "corrupt setting type\n";
    break;
  }

  // bit of a hack ...
  if (!strcmp (settings[i].config_name, "CommsDebug"))
    m_signal_core_settings_changed.emit();

  if (settings[i].triggers_redraw)
    m_signal_visual_settings_changed.emit();
}

void Settings::get_shrink_from_gui (Builder &builder)
{
  // Slicing.ShrinkQuality
  Gtk::ComboBox *combo = NULL;
  builder->get_widget ("Slicing.ShrinkQuality", combo);
  if (combo)
    Slicing.ShrinkQuality = combo->get_active_row_number ();
}

void Settings::get_port_speed_from_gui (Builder &builder)
{
  // Hardware.SerialSpeed
  Gtk::ComboBoxEntry *combo = NULL;
  builder->get_widget ("Hardware.SerialSpeed", combo);
  if (combo)
    Hardware.SerialSpeed = atoi(combo->get_active_text().c_str());
}

void Settings::get_colour_from_gui (Builder &builder, int i)
{
  const char *glade_name = colour_selectors[i].glade_name;
  vmml::Vector4f *dest =
      (vmml::Vector4f *)PTR_OFFSET(this, colour_selectors[i].member_offset);
  Gdk::Color c;
  Gtk::ColorButton *w = NULL;
  builder->get_widget (glade_name, w);
  if (!w) return;

  c = w->get_color();
  dest->r = c.get_red_p();
  dest->g = c.get_green_p();
  dest->b = c.get_blue_p();
  dest->a = (float) (w->get_alpha()) / 65535.0;

  m_signal_visual_settings_changed.emit();
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

  for (uint i = 0; i < G_N_ELEMENTS (colour_selectors); i++) {
      const char *glade_name = colour_selectors[i].glade_name;
      vmml::Vector4f *src =
        (vmml::Vector4f *) PTR_OFFSET(this, colour_selectors[i].member_offset);
      Gdk::Color c;
      Gtk::ColorButton *w = NULL;
      builder->get_widget (glade_name, w);
      if (w) {
        w->set_use_alpha(true);
        c.set_rgb_p(src->r, src->g, src->b);
        w->set_color(c);
        w->set_alpha(src->a * 65535.0);
      }
  }

  // Set serial speed. Find the row that holds this value
  Gtk::ComboBoxEntry *portspeed = NULL;
  builder->get_widget ("Hardware.SerialSpeed", portspeed);
  if (portspeed) {
    std::ostringstream ostr;
    ostr << Hardware.SerialSpeed;
    Glib::ustring val(ostr.str());
    portspeed->get_entry()->set_text(val);
  }
}

void Settings::connect_to_ui (Builder &builder)
{
  // connect gcode configurable text sections
  GCode.m_impl->connectToUI (builder);

  // first setup ranges on spinbuttons ...
  for (uint i = 0; i < G_N_ELEMENTS (spin_ranges); i++) {
    Gtk::SpinButton *spin = NULL;
    builder->get_widget (spin_ranges[i].widget, spin);
    if (!spin)
      std::cerr << "missing spin button of name '" << spin_ranges[i].widget << "'\n";
    else {
      spin->set_range (spin_ranges[i].min, spin_ranges[i].max);
      spin->set_increments (spin_ranges[i].inc, spin_ranges[i].inc_page);
    }
  }
  // Ranges on [HV]Range widgets
  for (uint i = 0; i < G_N_ELEMENTS (ranges); i++) {
    Gtk::Range *range = NULL;
    builder->get_widget (ranges[i].widget, range);
    if (!range)
      std::cerr << "missing range slider of name '" << ranges[i].widget << "'\n";
    else {
      range->set_range (ranges[i].min, ranges[i].max);
      range->set_increments (ranges[i].inc, ranges[i].inc_page);
    }
  }

  // connect widget / values from our table
  for (uint i = 0; i < G_N_ELEMENTS (settings); i++) {
    const char *glade_name = settings[i].glade_name;

    if (!glade_name)
      continue;

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
      Gtk::Widget *w = NULL;
      builder->get_widget (glade_name, w);
      if (!w) {
        std::cerr << "Missing user interface item " << glade_name << "\n";
        break;
      }
      Gtk::SpinButton *spin = dynamic_cast<Gtk::SpinButton *>(w);
      if (spin) {
        spin->signal_value_changed().connect
          (sigc::bind(sigc::bind(sigc::mem_fun(*this, &Settings::get_from_gui), i), builder));
        break;
      }
      Gtk::Range *range = dynamic_cast<Gtk::Range *>(w);
      if (range) {
        range->signal_value_changed().connect
          (sigc::bind(sigc::bind(sigc::mem_fun(*this, &Settings::get_from_gui), i), builder));
        break;
      }
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

    combo->signal_changed().connect
      (sigc::bind(sigc::mem_fun(*this, &Settings::get_shrink_from_gui), builder));
  }

  // Colour selectors
  for (uint i = 0; i < G_N_ELEMENTS (colour_selectors); i++) {
    const char *glade_name = colour_selectors[i].glade_name;
    Gdk::Color c;
    Gtk::ColorButton *w = NULL;

    if (!glade_name)
      continue;

    builder->get_widget (glade_name, w);
    if (!w) continue;

    w->signal_color_set().connect
          (sigc::bind(sigc::bind(sigc::mem_fun(*this,
            &Settings::get_colour_from_gui), i), builder));
  }

  // Serial port speed
  Gtk::ComboBoxEntry *portspeed = NULL;
  builder->get_widget ("Hardware.SerialSpeed", portspeed);
  if (portspeed) {
    const guint32 speeds[] = {
        9600, 19200, 38400, 57600, 115200, 230400, 576000
    };

    Glib::RefPtr<Gtk::ListStore> model;
    Gtk::TreeModelColumnRecord record;
    Gtk::TreeModelColumn<Glib::ustring> column;
    record.add (column);
    model = Gtk::ListStore::create(record);
    for (guint i = 0; i < G_N_ELEMENTS(speeds); i++) {
      std::ostringstream val;
      val << speeds[i];
      model->append()->set_value (0, Glib::ustring(val.str()));
    }
    portspeed->set_model (model);
    portspeed->set_text_column (0);

    portspeed->signal_changed().connect
      (sigc::bind(sigc::mem_fun(*this, &Settings::get_port_speed_from_gui), builder));
  }

  /* Update UI with defaults */
  m_signal_update_settings_gui.emit();
}

// We do our internal measurement in terms of the length of extruded material
float Settings::HardwareSettings::GetExtrudeFactor() const
{
  float f = 1.0;
  if (CalibrateInput) {
    // otherwise we just work back from the extruded diameter for now.
    f = (ExtrudedMaterialWidth * ExtrudedMaterialWidth) / (FilamentDiameter * FilamentDiameter);
  } // else: we work in terms of output anyway;

  return f * ExtrusionFactor;
}
