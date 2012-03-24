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

#include "infill.h"
/*
 * How settings are intended to work:
 *
 * The large table below provides pointers into a settings instance, and
 * some simple (POD) type information for the common settings. It also
 * provides the configuration name for each setting, and a Gtk::Builder
 * XML widget name, with which the setting should be associated.
 */


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
  { OFFSET (field.array[0]), T_COLOUR_MEMBER, config_name "R", NULL, def_valueR, NULL, redraw }, \
  { OFFSET (field.array[1]), T_COLOUR_MEMBER, config_name "G", NULL, def_valueG, NULL, redraw }, \
  { OFFSET (field.array[2]), T_COLOUR_MEMBER, config_name "B", NULL, def_valueB, NULL, redraw }, \
  { OFFSET (field.array[3]), T_COLOUR_MEMBER, config_name "A", NULL, def_valueA, NULL, redraw }


#define FLOAT_PHASE_MEMBER(phase, phasestd, member, def_value, redraw) \
  { OFFSET (Raft.Phase[Settings::RaftSettings::PHASE_##phase].member), T_FLOAT, \
    #phasestd #member, #phasestd #member, def_value, NULL, redraw }

// converting our offsets into type pointers
#define PTR_OFFSET(obj, offset)  (((guchar *)obj) + offset)
#define PTR_BOOL(obj, idx)      ((bool *)PTR_OFFSET (obj, settings[idx].member_offset))
#define PTR_INT(obj, idx)       ((int *)PTR_OFFSET (obj, settings[idx].member_offset))
#define PTR_UINT(obj, idx)      ((uint *)PTR_OFFSET (obj, settings[idx].member_offset))
#define PTR_FLOAT(obj, idx)     ((float *)PTR_OFFSET (obj, settings[idx].member_offset))
#define PTR_DOUBLE(obj, idx)     ((double *)PTR_OFFSET (obj, settings[idx].member_offset))
#define PTR_STRING(obj, idx)    ((std::string *)PTR_OFFSET (obj, settings[idx].member_offset))
#define PTR_COLOUR(obj, idx)    ((Vector4f *)PTR_OFFSET (obj, settings[idx].member_offset))

enum SettingType { T_BOOL, T_INT, T_FLOAT, T_DOUBLE, T_STRING, T_COLOUR_MEMBER };
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
    "BaseLayerCount", "BaseLayerCount", 1, NULL, true },
  FLOAT_PHASE_MEMBER(BASE, Base, MaterialDistanceRatio, 1.8, true),
  FLOAT_PHASE_MEMBER(BASE, Base, Rotation, 0.0, false),
  FLOAT_PHASE_MEMBER(BASE, Base, RotationPrLayer, 90.0, false),
  FLOAT_PHASE_MEMBER(BASE, Base, Distance, 2.0, false),
  FLOAT_PHASE_MEMBER(BASE, Base, Thickness, 1.0, true),
  FLOAT_PHASE_MEMBER(BASE, Base, Temperature, 1.10, false),
  
  // Raft Interface
  { OFFSET (Raft.Phase[Settings::RaftSettings::PHASE_INTERFACE].LayerCount), T_INT,
    "InterfaceLayerCount", "InterfaceLayerCount", 2, NULL, true },
  FLOAT_PHASE_MEMBER(INTERFACE, Interface, MaterialDistanceRatio, 1.0, true),
  FLOAT_PHASE_MEMBER(INTERFACE, Interface, Rotation, 90.0, false),
  FLOAT_PHASE_MEMBER(INTERFACE, Interface, RotationPrLayer, 90.0, false),
  FLOAT_PHASE_MEMBER(INTERFACE, Interface, Distance, 2.0, false),
  FLOAT_PHASE_MEMBER(INTERFACE, Interface, Thickness, 1.0, true),
  FLOAT_PHASE_MEMBER(INTERFACE, Interface, Temperature, 1.0, false),

#undef FLOAT_PHASE_MEMBER

  // Hardware
  BOOL_MEMBER  (Hardware.CalibrateInput,  "CalibrateInput",  false, false),
  FLOAT_MEMBER (Hardware.MinPrintSpeedXY, "MinPrintSpeedXY", 1000, false),
  FLOAT_MEMBER (Hardware.MaxPrintSpeedXY, "MaxPrintSpeedXY", 4000, false),
  FLOAT_MEMBER (Hardware.MoveSpeed,       "MoveSpeed",  4000, false),
  FLOAT_MEMBER (Hardware.MinPrintSpeedZ,  "MinPrintSpeedZ",  50, false),
  FLOAT_MEMBER (Hardware.MaxPrintSpeedZ,  "MaxPrintSpeedZ",  150, false),
  FLOAT_MEMBER (Hardware.EMaxSpeed,       "EMaxSpeed",  100, false),
  FLOAT_MEMBER (Hardware.MaxShellSpeed,   "MaxShellSpeed",  3000, false),

  // FLOAT_MEMBER (Hardware.DistanceToReachFullSpeed, "DistanceToReachFullSpeed", 1.5, false),
  FLOAT_MEMBER (Hardware.ExtrusionFactor, "ExtrusionFactor", 1.0, true),
  FLOAT_MEMBER (Hardware.FilamentDiameter, "FilamentDiameter", 3.0, true),
  FLOAT_MEMBER (Hardware.LayerThickness, "LayerThickness", 0.4, true),
  FLOAT_MEMBER (Hardware.DownstreamMultiplier, "DownstreamMultiplier", 1.0, false),
  FLOAT_MEMBER (Hardware.DownstreamExtrusionMultiplier, "DownstreamExtrusionMultiplier", 1.0, false),

  // Volume.
  { OFFSET (Hardware.Volume.array[0]), T_DOUBLE, "mfVolumeX", "Hardware.Volume.X", 200, NULL, true },
  { OFFSET (Hardware.Volume.array[1]), T_DOUBLE, "mfVolumeY", "Hardware.Volume.Y", 200, NULL, true },
  { OFFSET (Hardware.Volume.array[2]), T_DOUBLE, "mfVolumeZ", "Hardware.Volume.Z", 140, NULL, true },

  // PrintMargin
  { OFFSET (Hardware.PrintMargin.array[0]), T_DOUBLE, "PrintMarginX", "Hardware.PrintMargin.X", 10, NULL, true },
  { OFFSET (Hardware.PrintMargin.array[1]), T_DOUBLE, "PrintMarginY", "Hardware.PrintMargin.Y", 10, NULL, true },
  { OFFSET (Hardware.PrintMargin.array[2]), T_DOUBLE, "PrintMarginZ", "Hardware.PrintMargin.Z", 0, NULL, true },

  FLOAT_MEMBER  (Hardware.ExtrudedMaterialWidthRatio, "ExtrudedMaterialWidthRatio", 1.8, true),
  { OFFSET (Hardware.PortName), T_STRING, "Hardware.PortName", NULL, 0, DEFAULT_COM_PORT, false },
  { OFFSET (Hardware.SerialSpeed), T_INT, "Hardware.SerialSpeed", NULL, 115200, false },
  BOOL_MEMBER   (Hardware.ValidateConnection, "ValidateConnection", true, false),
  INT_MEMBER    (Hardware.KeepLines, "KeepLines", 1000, false),
  INT_MEMBER    (Hardware.ReceivingBufferSize, "ReceivingBufferSize", 4, false),

  // Printer
  FLOAT_MEMBER  (Printer.ExtrudeAmount, "Printer.ExtrudeAmount", 5, false),
  FLOAT_MEMBER  (Printer.ExtrudeSpeed, "Printer.ExtrudeSpeed", 100, false),
  INT_MEMBER    (Printer.FanVoltage, "Printer.FanVoltage", 200, false),
  BOOL_MEMBER   (Printer.Logging, "Printer.Logging", false, false),
  BOOL_MEMBER   (Printer.ClearLogOnPrintStart, "Printer.ClearLogOnPrintStart", false, false),
  FLOAT_MEMBER  (Printer.NozzleTemp, "Printer.NozzleTemp", 210, false),
  FLOAT_MEMBER  (Printer.BedTemp, "Printer.BedTemp", 60, false),

  // Slicing
  BOOL_MEMBER  (Slicing.RelativeEcode, "RelativeEcode", false, false),
  BOOL_MEMBER  (Slicing.EnableAntiooze, "EnableAntiooze", false, true),
  FLOAT_MEMBER (Slicing.AntioozeDistance, "AntioozeDistance", 4.5, true),
  FLOAT_MEMBER (Slicing.AntioozeAmount, "AntioozeAmount", 1, true),
  FLOAT_MEMBER (Slicing.AntioozeHaltRatio, "AntioozeHaltRatio", 0.2, true),
  FLOAT_MEMBER (Slicing.AntioozeSpeed, "AntioozeSpeed", 1000.0, true),

  FLOAT_MEMBER  (Slicing.InfillPercent, "InfillPercent", 30, true),
  FLOAT_MEMBER  (Slicing.InfillRotation, "InfillRotation", 90.0, true),
  FLOAT_MEMBER  (Slicing.InfillRotationPrLayer, "InfillRotationPrLayer", 60.0, true),
  FLOAT_MEMBER  (Slicing.AltInfillPercent, "AltInfillPercent", 80, true),
  FLOAT_MEMBER  (Slicing.InfillOverlap, "InfillOverlap", 0.2, true),
  INT_MEMBER    (Slicing.AltInfillLayers, "AltInfillLayers", 0, true),
  INT_MEMBER    (Slicing.NormalFilltype, "NormalFilltype", 0, true),
  FLOAT_MEMBER  (Slicing.NormalFillExtrusion, "NormalFillExtrusion", 1, true),
  INT_MEMBER    (Slicing.FullFilltype, "FullFilltype", 0, true),
  FLOAT_MEMBER  (Slicing.FullFillExtrusion, "FullFillExtrusion", 1, true),
  INT_MEMBER    (Slicing.SupportFilltype, "SupportFilltype", 0, true),
  FLOAT_MEMBER  (Slicing.SupportExtrusion, "SupportExtrusion", 0.5, true),
  BOOL_MEMBER   (Slicing.MakeDecor, "MakeDecor", true, true),
  INT_MEMBER    (Slicing.DecorFilltype, "DecorFilltype", 0, true),
  FLOAT_MEMBER  (Slicing.DecorInfillRotation, "DecorInfillRotation", 0, true),
  FLOAT_MEMBER  (Slicing.DecorInfillDistance, "DecorInfillDistance", 2.0, true),
  INT_MEMBER    (Slicing.SolidLayers, "SolidLayers", 2, true),
  BOOL_MEMBER   (Slicing.Support, "Support", true, true),
  FLOAT_MEMBER  (Slicing.SupportWiden, "SupportWiden", 0, true),
  BOOL_MEMBER   (Slicing.Skirt, "Skirt", false, true),
  FLOAT_MEMBER  (Slicing.SkirtHeight, "SkirtHeight", 0.0, true),
  FLOAT_MEMBER  (Slicing.SkirtDistance, "SkirtDistance", 3, true),
  INT_MEMBER    (Slicing.Skins, "Skins", 1, true),
  BOOL_MEMBER   (Slicing.Varslicing, "Varslicing", false, true),
  BOOL_MEMBER   (Slicing.ShellOnly, "ShellOnly", false, true),
  INT_MEMBER    (Slicing.ShellCount, "ShellCount", 1, true),
  // BOOL_MEMBER   (Slicing.EnableAcceleration, "EnableAcceleration", true, false),
  FLOAT_MEMBER  (Slicing.MinLayertime, "MinLayertime", 5, false),
  BOOL_MEMBER   (Slicing.FanControl, "FanControl", false, false),
  INT_MEMBER    (Slicing.MinFanSpeed, "MinFanSpeed", 150, false),
  INT_MEMBER    (Slicing.MaxFanSpeed, "MaxFanSpeed", 255, false),

  //FLOAT_MEMBER  (Slicing.Optimization, "Optimization", 0.01, true),
  BOOL_MEMBER   (Slicing.BuildSerial, "BuildSerial", false, true),
  FLOAT_MEMBER  (Slicing.ShellOffset, "ShellOffset", 0.1, true),
  BOOL_MEMBER   (Slicing.LinelengthSort, "LinelengthSort", false, true),
  INT_MEMBER    (Slicing.FirstLayersNum, "FirstLayersNum", 1, true),
  FLOAT_MEMBER  (Slicing.FirstLayersSpeed, "FirstLayersSpeed", 0.5, true),
  FLOAT_MEMBER  (Slicing.FirstLayersInfillDist, "FirstLayersInfillDist", 0.8, true),
  FLOAT_MEMBER  (Slicing.FirstLayerHeight, "FirstLayerHeight", 0.7, true),
  BOOL_MEMBER   (Slicing.UseArcs, "UseArcs", false, true),
  FLOAT_MEMBER  (Slicing.ArcsMaxAngle, "ArcsMaxAngle", 20, true),
  BOOL_MEMBER   (Slicing.RoundCorners, "RoundCorners", true, true),
  FLOAT_MEMBER  (Slicing.CornerRadius, "CornerRadius", 1., true),
  BOOL_MEMBER   (Slicing.NoBridges, "NoBridges", false, true),
  FLOAT_MEMBER  (Slicing.BridgeExtrusion, "BridgeExtrusion", 1, true),

  // Misc.
  BOOL_MEMBER (Misc.ShapeAutoplace, "ShapeAutoplace", true, false),
  //BOOL_MEMBER (Misc.FileLoggingEnabled, "FileLoggingEnabled", true, false),
  BOOL_MEMBER (Misc.TempReadingEnabled, "TempReadingEnabled", true, false),

  // GCode - handled by GCodeImpl
  BOOL_MEMBER (Display.DisplayGCode, "DisplayGCode", true, true),
  FLOAT_MEMBER (Display.GCodeDrawStart, "GCodeDrawStart", 0.0, true),
  FLOAT_MEMBER (Display.GCodeDrawEnd, "GCodeDrawEnd", 1.0, true),
  BOOL_MEMBER (Display.DisplayGCodeBorders, "DisplayGCodeBorders", true, true),
  BOOL_MEMBER (Display.DisplayGCodeArrows, "DisplayGCodeArrows", true, true),
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
  BOOL_MEMBER (Display.TerminalProgress, "TerminalProgress", false, true),
  BOOL_MEMBER (Display.DisplayLayer, "DisplayLayer", false, true),
  BOOL_MEMBER (Display.DrawVertexNumbers, "DrawVertexNumbers", false, true),
  BOOL_MEMBER (Display.DrawLineNumbers, "DrawLineNumbers", false, true),
  BOOL_MEMBER (Display.DrawOutlineNumbers, "DrawOutlineNumbers", false, true),
  BOOL_MEMBER (Display.DrawCPVertexNumbers, "DrawCPVertexNumbers", false, true),
  BOOL_MEMBER (Display.DrawCPLineNumbers, "DrawCPLineNumbers", false, true),
  BOOL_MEMBER (Display.DrawCPOutlineNumbers, "DrawCPOutlineNumbers", false, true),
  BOOL_MEMBER (Display.DrawMeasures, "DrawMeasures", true, true),

  FLOAT_MEMBER (Display.LayerValue, "LayerValue", 0, true),
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
  COLOUR_MEMBER(Display.GCodePrintingRGBA,
		"Display.GCodePrintingColour", 0.1, 0.5, 0.0, 1.0, true),
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
  { "Slicing.SolidLayers", 0, 100, 1, 5 },
  { "Slicing.InfillRotation", -360, 360, 5, 45 },
  { "Slicing.InfillRotationPrLayer", -360, 360, 5, 90 },
  { "Slicing.InfillPercent", 0.0, 100.0, 1, 10.0 },
  { "Slicing.AltInfillPercent", 0.0, 100.0, 1, 10.0 },
  { "Slicing.AltInfillLayers", 0, 10000, 10, 100 },
  { "Slicing.DecorInfillDistance", 0.0, 10, 0.1, 1 },
  { "Slicing.DecorInfillRotation", -360, 360, 5, 45 },
  { "Slicing.InfillOverlap", 0, 1.0 , 0.01, 0.1},
  { "Slicing.NormalFillExtrusion", 0.01, 3, 0.01, 0.1},
  { "Slicing.FullFillExtrusion", 0.01, 3, 0.01, 0.1},
  { "Slicing.BridgeExtrusion", 0.01, 3, 0.01, 0.1},
  { "Slicing.SupportExtrusion", 0.01, 3, 0.01, 0.1},
  { "Slicing.SupportWiden", -5, 5, 0.01, 0.1},
  //{ "Slicing.Optimization", 0.0, 10.0, 0.01, 0.1 },
  { "Slicing.AntioozeDistance", 0.0, 25.0, 0.1, 1 },
  { "Slicing.AntioozeAmount", 0.0, 25.0, 0.1, 1 },
  { "Slicing.AntioozeSpeed", 0.0, 10000.0, 25.0, 100.0 },
  { "Slicing.AntioozeHaltRatio", 0.0, 1.0, 0.01, 0.1 },
  { "Slicing.SkirtHeight", 0.0, 1000, 0.1, 1 },
  { "Slicing.SkirtDistance", 0.0, 100, 0.1, 1 },
  { "Slicing.Skins", 1, 5, 1, 1 },
  { "Slicing.MinLayertime", 0.0, 100, 1, 10 },
  { "Slicing.MinFanSpeed", 0, 255, 5, 25 },
  { "Slicing.MaxFanSpeed", 0, 255, 5, 25 },
  //{ "Slicing.SerialBuildHeight", 0.0, 1000.0, 0.1, 1 },
  { "Slicing.ShellOffset", -10, 10, 0.1, 1 },
  { "Slicing.FirstLayersNum", 0, 1000, 1, 10 },
  { "Slicing.FirstLayersSpeed", 0.01, 3, 0.01, 0.1 },
  { "Slicing.FirstLayersInfillDist", 0.0, 100, 0.01, 0.1 },
  { "Slicing.FirstLayerHeight", 0.0, 1., 0.01, 0.1 },
  { "Slicing.ArcsMaxAngle", 0, 180, 1, 10 },
  { "Slicing.CornerRadius", 0, 5, 0.01, 0.1 },

  // Hardware
  { "Hardware.Volume.X", 0.0, 1000.0, 5.0, 25.0 },
  { "Hardware.Volume.Y", 0.0, 1000.0, 5.0, 25.0 },
  { "Hardware.Volume.Z", 0.0, 1000.0, 5.0, 25.0 },
  { "Hardware.PrintMargin.X", 0.0, 100.0, 1.0, 5.0 },
  { "Hardware.PrintMargin.Y", 0.0, 100.0, 1.0, 5.0 },
  { "Hardware.PrintMargin.Z", 0.0, 100.0, 1.0, 5.0 },
  { "Hardware.DistanceToReachFullSpeed", 0.0, 10.0, 0.1, 1.0 },
  { "Hardware.ExtrudedMaterialWidthRatio", 0.0, 10.0, 0.01, 1.8 },
  { "Hardware.LayerThickness", 0.01, 3.0, 0.01, 0.2 },
  { "Hardware.ExtrusionFactor", 0.0, 2.0, 0.1, 0.2 },
  { "Hardware.FilamentDiameter", 0.5, 5.0, 0.01, 0.05 },

  { "Hardware.MinPrintSpeedXY", 1.0, 20000.0, 10.0, 100.0 },
  { "Hardware.MaxPrintSpeedXY", 1.0, 20000.0, 10.0, 100.0 },
  { "Hardware.MoveSpeed", 1.0, 20000.0, 10.0, 100.0 },
  { "Hardware.MinPrintSpeedZ", 1.0, 2500.0, 10.0, 100.0 },
  { "Hardware.MaxPrintSpeedZ", 1.0, 2500.0, 10.0, 100.0 },
  { "Hardware.EMaxSpeed", 1.0, 20000.0, 10.0, 100.0 },
  { "Hardware.MaxShellSpeed", 1.0, 20000.0, 10.0, 100.0 },

  { "Hardware.ReceivingBufferSize", 1.0, 100.0, 1.0, 5.0 },
  { "Hardware.KeepLines", 100.0, 100000.0, 1.0, 500.0 },

  { "Hardware.DownstreamMultiplier", 0.01, 25.0, 0.01, 0.1 },
  { "Hardware.DownstreamExtrusionMultiplier", 0.01, 25.0, 0.01, 0.1 },

  //Printer
  { "Printer.ExtrudeAmount", 0.0, 1000.0, 1.0, 10.0 },
  { "Printer.ExtrudeSpeed", 0.0, 1000.0, 1.0, 10.0 },
  { "Printer.FanVoltage", 0, 255, 5, 25 },
  { "Printer.NozzleTemp", 0.0, 300.0, 1.0, 10.0 },
  { "Printer.BedTemp", 0.0, 200.0, 1.0, 10.0 },

  // Display pane
  { "Display.TempUpdateSpeed", 0.1, 10.0, 0.5, 1.0 },
  { "m_scale_value", 0.0001, 1000.0, 0.01, 0.1 },
  { "scale_x", 0.0001, 1000.0, 0.01, 0.1 },
  { "scale_y", 0.0001, 1000.0, 0.01, 0.1 },
  { "scale_z", 0.0001, 1000.0, 0.01, 0.1 },
};

// Add any [HV]Ranges to this array:
static struct {
  const char *widget;
  float min, max;
  float inc, inc_page;
} ranges[] = {
  // Display plane
  { "Display.LayerValue", 0.0, 1.0, 0.0, 0.01 },
  { "Display.GCodeDrawStart", 0.0, 1.0, 0.0, 0.1 },
  { "Display.GCodeDrawEnd", 0.0, 1.0, 0.0, 0.1 },
};

static struct {
  uint  member_offset;
  const char *glade_name;
}
colour_selectors[] = {
  { OFFSET(Display.PolygonRGBA), "Display.PolygonRGBA" },
  { OFFSET(Display.WireframeRGBA), "Display.WireframeRGBA" },
  { OFFSET(Display.NormalsRGBA), "Display.NormalsRGBA" },
  { OFFSET(Display.EndpointsRGBA), "Display.EndpointsRGBA" },
  { OFFSET(Display.GCodeExtrudeRGBA), "Display.GCodeExtrudeRGBA" },
  { OFFSET(Display.GCodePrintingRGBA), "Display.GCodePrintingRGBA" },
  { OFFSET(Display.GCodeMoveRGBA), "Display.GCodeMoveRGBA" }
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

// return infill distance in mm
double Settings::GetInfillDistance(double layerthickness, float percent) const
{
  double fullInfillDistance = 
    Hardware.GetExtrudedMaterialWidth(layerthickness);
  if (percent == 0) return 10000000;
  return fullInfillDistance * (100./percent);  
}
// void Settings::SlicingSettings::GetAltInfillLayers(std::vector<int>& layers, uint layerCount) const
// {
//   size_t start = 0, end = AltInfillLayersText.find(',');

//   if (AltInfillLayersText == "")
//     return;

//   while(start != std::string::npos) {
//     int num = atoi(AltInfillLayersText.data() + start);
//     if(num < 0) {
//       num += layerCount;
//     }
//     layers.push_back (num);

//     start = end;
//     end = AltInfillLayersText.find(',', start+1);
//   }
// }

Settings::Settings ()
{
  GCode.m_impl = new GCodeImpl();
  set_defaults();
}

Settings::~Settings()
{
  delete GCode.m_impl;
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
    case T_DOUBLE:
      *PTR_DOUBLE(this, i) = settings[i].def_float;
      break;
    case T_STRING:
      *PTR_STRING(this, i) = std::string (settings[i].def_string);
      break;
    default:
      std::cerr << "corrupt setting type " << endl;
      break;
    }
  }

  //Slicing.ShrinkQuality = SHRINK_FAST;
  // Slicing.NormalFilltype = ParallelInfill;
  // Slicing.FullFilltype = ParallelInfill;
  // Slicing.SupportFilltype = PolyInfill;

  GCode.m_impl->setDefaults();

  // The vectors map each to 3 spin boxes, one per dimension
  Hardware.Volume = Vector3d (200,200,140);
  Hardware.PrintMargin = Vector3d (10,10,0);
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
      std::cout << _("Failed to load settings from file '") << file->get_path() << "\n";
      return;
    }
  } catch (const Glib::KeyFileError &err) {
    std::cout << _("Exception ") << err.what() << _(" loading settings from file '") << file->get_path() << "\n";
    return;
  }

  std::cout << _("parsing config from '") << file->get_path() << "\n";

  for (uint i = 0; i < G_N_ELEMENTS (settings); i++) {

    Glib::ustring group, key;

    if (!get_group_and_key (i, group, key))
      continue;

    try {
      if (!cfg.has_key (group, key))
	continue;
    } catch (const Glib::KeyFileError &err) {
      continue;
    }

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
    case T_DOUBLE:
      *PTR_DOUBLE(this, i) = cfg.get_double (group, key);
      break;
    case T_STRING:
      *PTR_STRING(this, i) = cfg.get_string (group, key);
      break;
    default:
      std::cerr << _("corrupt setting type") << group << " : " << key << endl;;
      break;
    }
  }

  GCode.m_impl->loadSettings (cfg);

  try {
    Misc.window_width = cfg.get_integer ("Misc", "WindowWidth");
    Misc.window_height = cfg.get_integer ("Misc", "WindowHeight");
  } catch (const Glib::KeyFileError &err) {
    Misc.window_width =-1;
    Misc.window_height=-1;
  }
  
  try {
    vector<string> cbkeys = cfg.get_keys ("CustomButtons");
    CustomButtonLabel.resize(cbkeys.size());
    CustomButtonGcode.resize(cbkeys.size());
    for (guint i = 0; i < cbkeys.size(); i++) {  
      string s = cbkeys[i];
      std::replace(s.begin(),s.end(),'_',' ');
      CustomButtonLabel[i] = s;
      CustomButtonGcode[i] = cfg.get_string("CustomButtons", cbkeys[i]);
    }
  } catch (const Glib::KeyFileError &err) {
  }

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
    case T_DOUBLE:
      cfg.set_double (group, key, *PTR_DOUBLE(this, i));
      break;
    case T_STRING:
      cfg.set_string (group, key, *PTR_STRING(this, i));
      break;
    default:
      std::cerr << "Can't save setting of unknown type\n";
      break;
    };
  }
  ostringstream os; os << Misc.window_width;
  cfg.set_string("Misc", "WindowWidth", os.str());
  os.str(""); os << Misc.window_height;
  cfg.set_string("Misc", "WindowHeight", os.str());

  GCode.m_impl->saveSettings (cfg);
  
  string CBgroup="CustomButtons";
  for (guint i=0; i<CustomButtonLabel.size(); i++)  {
    string s = CustomButtonLabel[i];
    std::replace(s.begin(),s.end(),' ','_');
    cfg.set_string(CBgroup, s, CustomButtonGcode[i]);
  }

  Glib::ustring contents = cfg.to_data();
  Glib::file_set_contents (file->get_path(), contents);
}


void Settings::set_to_gui (Builder &builder, int i)
{
  const char *glade_name = settings[i].glade_name;
  SettingType t = settings[i].type;

  if (!glade_name)
        return;

  switch (t) {
  case T_BOOL: {
    Gtk::CheckButton *check = NULL;
    builder->get_widget (glade_name, check);
    if (!check)
      std::cerr << _("Missing boolean config item ") << glade_name << "\n";
    else
      check->set_active (*PTR_BOOL(this, i));
    break;
  }
  case T_INT:
  case T_FLOAT:
  case T_DOUBLE: {
    Gtk::Widget *w = NULL;
    builder->get_widget (glade_name, w);
    if (!w) {
      std::cerr << _("Missing user interface item ") << glade_name << "\n";
      break;
    }

    Gtk::SpinButton *spin = dynamic_cast<Gtk::SpinButton *>(w);
    if (spin) {
      if (t == T_INT)
          spin->set_value (*PTR_INT(this, i));
      else if (t == T_FLOAT)
          spin->set_value (*PTR_FLOAT(this, i));
      else
          spin->set_value (*PTR_DOUBLE(this, i));
      break;
    }
    Gtk::Range *range = dynamic_cast<Gtk::Range *>(w);
    if (range) {
      if (t == T_INT)
        range->set_value (*PTR_INT(this, i));
      else if (t == T_FLOAT)
        range->set_value (*PTR_FLOAT(this, i));
      else
        range->set_value (*PTR_DOUBLE(this, i));
    }
    break;
  }
  case T_STRING: {
    Gtk::Entry *e = NULL;
    builder->get_widget (glade_name, e);
    if (!e) {
      std::cerr << _("Missing user interface item ") << glade_name << "\n";
      break;
    }
    e->set_text(*PTR_STRING(this, i));
    break;
  }
  case T_COLOUR_MEMBER:
    break; // Ignore, Colour members are special 
  default:
    std::cerr << _("corrupt setting type\n") << glade_name <<endl;;
    break;
  }

  Gtk::Window *pWindow = NULL;
  builder->get_widget("main_window", pWindow);
  if (pWindow && Misc.window_width > 0 && Misc.window_height > 0)
    pWindow->resize(Misc.window_width, Misc.window_height);
}


void Settings::set_filltypes_to_gui (Builder &builder)
{
  // cerr << Slicing.NormalFilltype << " ! " << Slicing.FullFilltype
  //      << " ! " << Slicing.SupportFilltype<< endl;
  Gtk::ComboBox *combo = NULL;
  // avoid getting overwritten by callback
  uint norm = Slicing.NormalFilltype;
  uint full = Slicing.FullFilltype;
  uint support = Slicing.SupportFilltype;
  uint decor = Slicing.DecorFilltype;
  builder->get_widget ("Slicing.NormalFilltype", combo);
  if (combo)
    combo->set_active (norm);
  combo = NULL;
  builder->get_widget ("Slicing.FullFilltype", combo);
  if (combo)
    combo->set_active (full);
  combo = NULL;
  builder->get_widget ("Slicing.SupportFilltype", combo);
  if (combo)
    combo->set_active (support);
  builder->get_widget ("Slicing.DecorFilltype", combo);
  if (combo)
    combo->set_active (decor);
}

void Settings::get_from_gui (Builder &builder, int i)
{
  const char *glade_name = settings[i].glade_name;
  SettingType t = settings[i].type;

  if (glade_name == NULL)
        return; /* Not an automatically connected setting */

  switch (t) {
  case T_BOOL: {
    Gtk::CheckButton *check = NULL;
    builder->get_widget (glade_name, check);
    if (!check)
      std::cerr << _("Missing boolean config item ") << glade_name << "\n";
    else
      *PTR_BOOL(this, i) = check->get_active();
    break;
  }
  case T_INT:
  case T_FLOAT:
  case T_DOUBLE: {
    Gtk::Widget *w = NULL;
    builder->get_widget (glade_name, w);
    if (!w) {
      std::cerr << _("Missing GUI element ") << glade_name << "\n";
      break;
    }

    Gtk::SpinButton *spin = dynamic_cast<Gtk::SpinButton *>(w);
    if (spin) {
      if (t == T_INT)
          *PTR_INT(this, i) = spin->get_value();
      else if (t == T_FLOAT)
          *PTR_FLOAT(this, i) = spin->get_value();
      else
          *PTR_DOUBLE(this, i) = spin->get_value();
      break;
    }

    Gtk::Range *range = dynamic_cast<Gtk::Range *>(w);
    if (range) {
      if (t == T_INT)
          *PTR_INT(this, i) = range->get_value();
      else if (t == T_FLOAT)
          *PTR_FLOAT(this, i) = range->get_value();
      else
          *PTR_DOUBLE(this, i) = range->get_value();
    }
    break;
  }
  case T_STRING: {
    Gtk::Entry *e = NULL;
    builder->get_widget (glade_name, e);
    if (!e) {
      std::cerr << _("Missing user interface item ") << glade_name << "\n";
      break;
    }
    *PTR_STRING(this, i) = std::string(e->get_text());
    break;
  }
  case T_COLOUR_MEMBER:
    // Ignore, colour members are special
    break;
  default:
    std::cerr << _("corrupt setting type") << glade_name << endl;;
    break;
  }

  // bit of a hack ...
  if (!strcmp (settings[i].config_name, "CommsDebug"))
    m_signal_core_settings_changed.emit();

  if (settings[i].triggers_redraw)
    m_signal_visual_settings_changed.emit();
}

void Settings::get_filltypes_from_gui (Builder &builder)
{
  // cerr <<"Get_filltypes " << endl;
  Gtk::ComboBox *combo = NULL;
  builder->get_widget ("Slicing.NormalFilltype", combo);
  if (combo) {
    Slicing.NormalFilltype = combo->get_active_row_number ();
  }
  else cerr << "no Slicing.NormalFilltype combo" << endl;
  builder->get_widget ("Slicing.FullFilltype", combo);
  if (combo) {
    Slicing.FullFilltype = combo->get_active_row_number ();
  }
  else cerr << "no Slicing.FullFilltype combo" << endl;
  builder->get_widget ("Slicing.SupportFilltype", combo);
  if (combo){
    Slicing.SupportFilltype = combo->get_active_row_number ();
  }
  else cerr << "no Slicing.SupportFilltype combo" << endl;
  builder->get_widget ("Slicing.DecorFilltype", combo);
  if (combo){
    Slicing.DecorFilltype = combo->get_active_row_number ();
  }
  else cerr << "no Slicing.DecorFilltype combo" << endl;
  // cerr << "read combos: " << Slicing.NormalFilltype 
  //      <<  " / " << Slicing.FullFilltype 
  //      <<  " / " << Slicing.SupportFilltype << endl;
}

string combobox_get_active_value(Gtk::ComboBox *combo){
#if GTK_VERSION_GE(2, 24)
  if (combo->get_has_entry()) 
    {
      Gtk::Entry *entry = combo->get_entry();
      if (entry)
	return string(entry->get_text());
    } else 
#endif
    {
      uint c = combo->get_active_row_number();
      Glib::ustring rval;
      combo->get_model()->children()[c].get_value(0,rval);
      return string(rval);
    }
  cerr << "could not get combobox active value" << endl;
  return "";
}

bool combobox_set_to(Gtk::ComboBox *combo, string value){
  Glib::ustring val(value);
  Glib::RefPtr<Gtk::TreeModel> model = combo->get_model();
  uint nch = model->children().size();
  Glib::ustring rval;
  Glib::ustring gvalue(value.c_str());
#if GTK_VERSION_GE(2, 24)
  if (combo->get_has_entry())
    {
      Gtk::Entry *entry = combo->get_entry();
      if (entry) {
	entry->set_text(value);
	return true;
      }
    }
  else
#endif
    {
      for (uint c=0; c < nch; c++) {
	Gtk::TreeRow row = model->children()[c];
	row.get_value(0,rval);
	if (rval== gvalue) {
	  combo->set_active(c);
	  return true;
	}
      }
    }
  cerr << "value " << value << " not found in combobox" << endl;
  return false;
}

void set_up_combobox(Gtk::ComboBox *combo, vector<string> values)
{
  if (combo->get_model()) return;
  //cerr << "setup " ;
  Gtk::TreeModelColumn<Glib::ustring> column;
  Gtk::TreeModelColumnRecord record;
  record.add(column);
  Glib::RefPtr<Gtk::ListStore> store = Gtk::ListStore::create(record);
  combo->pack_start (column);
  combo->set_model(store);
  for (uint i=0; i<values.size(); i++) {
    //cerr << " adding " << values[i] << endl;
    store->append()->set_value(0, Glib::ustring(values[i].c_str()));
  }
#if GTK_VERSION_GE(2, 24)
  if (!combo->get_has_entry())
#endif
    combo->set_active(0);
  //cerr << "ok" << endl;
}


void Settings::get_port_speed_from_gui (Builder &builder)
{
  Gtk::ComboBox *combo = NULL;
  // Gtk::ComboBoxEntryText *tcombo = NULL;
  // builder->get_widget_derived ("Hardware.SerialSpeed", tcombo);
  builder->get_widget ("Hardware.SerialSpeed", combo);
  if (combo) {
#if GTK_VERSION_GE(2, 24)
    if (combo->get_has_entry()) {
      Gtk::Entry *entry = combo->get_entry();
      if (entry) {
	Hardware.SerialSpeed = atoi(entry->get_text().c_str()); 
      }
    }
    else
#endif
      Hardware.SerialSpeed = atoi(combobox_get_active_value(combo).c_str());
  }
}

void Settings::get_colour_from_gui (Builder &builder, int i)
{
  const char *glade_name = colour_selectors[i].glade_name;
  Vector4f *dest =
      (Vector4f *)PTR_OFFSET(this, colour_selectors[i].member_offset);
  Gdk::Color c;
  Gtk::ColorButton *w = NULL;
  builder->get_widget (glade_name, w);
  if (!w) return;

  c = w->get_color();
  dest->r() = c.get_red_p();
  dest->g() = c.get_green_p();
  dest->b() = c.get_blue_p();
  dest->a() = (float) (w->get_alpha()) / 65535.0;

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

  set_filltypes_to_gui (builder);

  for (uint i = 0; i < G_N_ELEMENTS (colour_selectors); i++) {
      const char *glade_name = colour_selectors[i].glade_name;
      Vector4f *src =
        (Vector4f *) PTR_OFFSET(this, colour_selectors[i].member_offset);
      Gdk::Color c;
      Gtk::ColorButton *w = NULL;
      builder->get_widget (glade_name, w);
      if (w) {
        w->set_use_alpha(true);
        c.set_rgb_p(src->r(), src->g(), src->b());
        w->set_color(c);
        w->set_alpha(src->a() * 65535.0);
      }
  }

  // Set serial speed. Find the row that holds this value
  Gtk::ComboBox *portspeed = NULL;
  builder->get_widget ("Hardware.SerialSpeed", portspeed);
  if (portspeed) {
    std::ostringstream ostr;
    ostr << Hardware.SerialSpeed;
    combobox_set_to(portspeed, ostr.str());
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
    case T_FLOAT:
    case T_DOUBLE: {
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

  // Slicing.*Filltype
  Gtk::ComboBox *combo = NULL;
  uint nfills = sizeof(InfillNames)/sizeof(string);
  vector<string> infills(InfillNames,InfillNames+nfills);
  builder->get_widget ("Slicing.NormalFilltype", combo);
  set_up_combobox(combo,infills);
  combo->signal_changed().connect
    (sigc::bind(sigc::mem_fun(*this, &Settings::get_filltypes_from_gui), builder));
  builder->get_widget ("Slicing.FullFilltype", combo);
  set_up_combobox(combo,infills);
  combo->signal_changed().connect
    (sigc::bind(sigc::mem_fun(*this, &Settings::get_filltypes_from_gui), builder));
  builder->get_widget ("Slicing.SupportFilltype", combo);
  set_up_combobox(combo,infills);
  combo->signal_changed().connect
    (sigc::bind(sigc::mem_fun(*this, &Settings::get_filltypes_from_gui), builder));
  builder->get_widget ("Slicing.DecorFilltype", combo);
  set_up_combobox(combo,infills);
  combo->signal_changed().connect
    (sigc::bind(sigc::mem_fun(*this, &Settings::get_filltypes_from_gui), builder));

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
  Gtk::ComboBox *portspeed = NULL;
  builder->get_widget ("Hardware.SerialSpeed",portspeed);
  if (portspeed) {
    const char *speeds[] = {
      "9600", "19200", "38400", "57600", "115200", "230400", "250000", "500000", "576000", "1000000"
    };
    vector<string> speedstr(speeds, speeds+sizeof(speeds)/sizeof(string));
    set_up_combobox(portspeed, speedstr);
    portspeed->signal_changed().connect
      (sigc::bind(sigc::mem_fun(*this, &Settings::get_port_speed_from_gui), builder));
  }

  /* Update UI with defaults */
  m_signal_update_settings_gui.emit();
}


double Settings::HardwareSettings::GetExtrudedMaterialWidth(double layerheight) const
{
  return ExtrudedMaterialWidthRatio * layerheight;
}

// TODO This depends whether lines are packed or not - ellipsis/rectangle

// We do our internal measurement in terms of the length of extruded material
double Settings::HardwareSettings::GetExtrudeFactor(double layerheight) const
{
  double f = ExtrusionFactor;
  double matWidth = GetExtrudedMaterialWidth(layerheight);
  if (CalibrateInput) {
    // otherwise we just work back from the extruded diameter for now.
    f *= (matWidth * matWidth) / (FilamentDiameter * FilamentDiameter);
  } // else: we work in terms of output anyway;

  return f;
}

Matrix4d Settings::getBasicTransformation(Matrix4d T) const
{
  Vector3d t;
  T.get_translation(t);
  t+= Vector3d(Hardware.PrintMargin.x()+Raft.Size*RaftEnable, 
	       Hardware.PrintMargin.y()+Raft.Size*RaftEnable, 
	       0);
  T.set_translation(t);
  return T;
}
