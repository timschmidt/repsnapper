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

  // General
  { OFFSET (Name), T_STRING, "SettingsName", NULL, 0, "Default Settings", false },
  { OFFSET (Image), T_STRING, "SettingsImage", NULL, 0, "", false },

  // Raft:
  BOOL_MEMBER  (Raft.Enable, "Raft.Enable", false, true),
  FLOAT_MEMBER (Raft.Size,  "Raft.Size",   1.33, true),

  // Raft Base
  { OFFSET (Raft.Phase[Settings::RaftSettings::PHASE_BASE].LayerCount), T_INT,
    "Raft.Base.LayerCount", "Raft.Base.LayerCount", 1, NULL, true },
  FLOAT_PHASE_MEMBER(BASE, Raft.Base., MaterialDistanceRatio, 1.8, true),
  FLOAT_PHASE_MEMBER(BASE, Raft.Base., Rotation, 0.0, false),
  FLOAT_PHASE_MEMBER(BASE, Raft.Base., RotationPrLayer, 90.0, false),
  FLOAT_PHASE_MEMBER(BASE, Raft.Base., Distance, 2.0, false),
  FLOAT_PHASE_MEMBER(BASE, Raft.Base., Thickness, 1.0, true),
  FLOAT_PHASE_MEMBER(BASE, Raft.Base., Temperature, 1.10, false),

  // Raft Interface
  { OFFSET (Raft.Phase[Settings::RaftSettings::PHASE_INTERFACE].LayerCount), T_INT,
    "Raft.Interface.LayerCount", "Raft.Interface.LayerCount", 2, NULL, true },
  FLOAT_PHASE_MEMBER(INTERFACE, Raft.Interface., MaterialDistanceRatio, 1.0, true),
  FLOAT_PHASE_MEMBER(INTERFACE, Raft.Interface., Rotation, 90.0, true),
  FLOAT_PHASE_MEMBER(INTERFACE, Raft.Interface., RotationPrLayer, 90.0, true),
  FLOAT_PHASE_MEMBER(INTERFACE, Raft.Interface., Distance, 2.0, true),
  FLOAT_PHASE_MEMBER(INTERFACE, Raft.Interface., Thickness, 1.0, true),
  FLOAT_PHASE_MEMBER(INTERFACE, Raft.Interface., Temperature, 1.0, false),

#undef FLOAT_PHASE_MEMBER

  // Hardware
  FLOAT_MEMBER (Hardware.MinMoveSpeedXY, "MinMoveSpeedXY", 20, true),
  FLOAT_MEMBER (Hardware.MaxMoveSpeedXY, "MaxMoveSpeedXY", 180, true),
  FLOAT_MEMBER (Hardware.MinMoveSpeedZ,  "MinMoveSpeedZ",  1, true),
  FLOAT_MEMBER (Hardware.MaxMoveSpeedZ,  "MaxMoveSpeedZ",  3, true),

  // FLOAT_MEMBER (Hardware.DistanceToReachFullSpeed, "DistanceToReachFullSpeed", 1.5, false),
  // Volume.
  { OFFSET (Hardware.Volume.array[0]), T_DOUBLE, "mfVolumeX", "Hardware.Volume.X", 200, NULL, true },
  { OFFSET (Hardware.Volume.array[1]), T_DOUBLE, "mfVolumeY", "Hardware.Volume.Y", 200, NULL, true },
  { OFFSET (Hardware.Volume.array[2]), T_DOUBLE, "mfVolumeZ", "Hardware.Volume.Z", 140, NULL, true },

  // PrintMargin
  { OFFSET (Hardware.PrintMargin.array[0]), T_DOUBLE, "PrintMarginX", "Hardware.PrintMargin.X", 10, NULL, true },
  { OFFSET (Hardware.PrintMargin.array[1]), T_DOUBLE, "PrintMarginY", "Hardware.PrintMargin.Y", 10, NULL, true },
  { OFFSET (Hardware.PrintMargin.array[2]), T_DOUBLE, "PrintMarginZ", "Hardware.PrintMargin.Z", 0, NULL, true },


  { OFFSET (Hardware.PortName), T_STRING, "Hardware.PortName", NULL, 0, DEFAULT_COM_PORT, false },
  { OFFSET (Hardware.SerialSpeed), T_INT, "Hardware.SerialSpeed", NULL, 115200, NULL, false },
  BOOL_MEMBER   (Hardware.ValidateConnection, "ValidateConnection", true, false),
  INT_MEMBER    (Hardware.KeepLines, "KeepLines", 1000, false),
  INT_MEMBER    (Hardware.ReceivingBufferSize, "ReceivingBufferSize", 4, false),

  // Extruder
  BOOL_MEMBER  (Extruder.CalibrateInput,  "CalibrateInput",  true, true),
  FLOAT_MEMBER (Extruder.OffsetX, "Extruder.OffsetX", 0.0, true),
  FLOAT_MEMBER (Extruder.OffsetY, "Extruder.OffsetY", 0.0, true),
  FLOAT_MEMBER (Extruder.FilamentDiameter, "FilamentDiameter", 3.0, true),
  FLOAT_MEMBER (Extruder.ExtrusionFactor, "Extruder.ExtrusionFactor", 1.0, true),
  /* FLOAT_MEMBER (Extruder.DownstreamMultiplier, "DownstreamMultiplier", 1.0, true), */
  /* FLOAT_MEMBER (Extruder.DownstreamExtrusionMultiplier, "DownstreamExtrusionMultiplier",  1.0, true),*/
  FLOAT_MEMBER (Extruder.ExtrudedMaterialWidthRatio, "ExtrudedMaterialWidthRatio", 1.8, true),
  FLOAT_MEMBER (Extruder.MinimumLineWidth, "MinimumLineWidth", 0.4, true),
  FLOAT_MEMBER (Extruder.MaximumLineWidth, "MaximumLineWidth", 0.7, true),
  FLOAT_MEMBER (Extruder.MaxLineSpeed,     "MaxLineSpeed",  180, true),
  FLOAT_MEMBER (Extruder.EMaxSpeed,        "EMaxSpeed",  1.5, true),
  FLOAT_MEMBER (Extruder.MaxShellSpeed,    "MaxShellSpeed",  150, true),
  STRING_MEMBER(Extruder.GCLetter,         "Extruder.GCLetter", "E", false),
  BOOL_MEMBER  (Extruder.UseForSupport,    "Extruder.UseForSupport",  false, true),
  { OFFSET (Extruder.name), T_STRING, "Extruder.name", NULL, 0, "Extruder", false },

  BOOL_MEMBER  (Extruder.EnableAntiooze, "EnableAntiooze", false, true),
  FLOAT_MEMBER (Extruder.AntioozeDistance, "AntioozeDistance", 4.5, true),
  FLOAT_MEMBER (Extruder.AntioozeAmount, "AntioozeAmount", 1, true),
  //FLOAT_MEMBER (Extruder.AntioozeHaltRatio, "AntioozeHaltRatio", 0.2, true),
  FLOAT_MEMBER (Extruder.AntioozeSpeed, "AntioozeSpeed", 15.0, true),
  FLOAT_MEMBER (Extruder.AntioozeZlift, "AntioozeZlift", 0, true),
  BOOL_MEMBER  (Extruder.ZliftAlways, "ZliftAlways", false, true),
  COLOUR_MEMBER(Extruder.DisplayRGBA,
		"Extruder.DisplayRGBA", 1.0, 1.0, 0.0, 1.0, true),

  // Printer
  FLOAT_MEMBER  (Printer.ExtrudeAmount, "Printer.ExtrudeAmount", 1, false),
  FLOAT_MEMBER  (Printer.ExtrudeSpeed, "Printer.ExtrudeSpeed", 1.5, false),
  INT_MEMBER    (Printer.FanVoltage, "Printer.FanVoltage", 200, false),
  BOOL_MEMBER   (Printer.Logging, "Printer.Logging", false, false),
  BOOL_MEMBER   (Printer.ClearLogOnPrintStart, "Printer.ClearLogOnPrintStart", false, false),
  { OFFSET (Printer.NozzleTemp), T_FLOAT, "Printer.NozzleTemp", NULL, 210, NULL, false },
  { OFFSET (Printer.BedTemp)   , T_FLOAT, "Printer.BedTemp", NULL,  60, NULL, false },

  // Slicing
  BOOL_MEMBER   (Slicing.RelativeEcode, "RelativeEcode", false, false),
  BOOL_MEMBER   (Slicing.UseTCommand, "UseTCommand", true, false),
  FLOAT_MEMBER  (Slicing.LayerThickness, "LayerThickness", 0.3, true),
  BOOL_MEMBER   (Slicing.MoveNearest, "MoveNearest", true, true),
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
  FLOAT_MEMBER  (Slicing.SupportInfillDistance, "SupportInfillDistance", 3.0, true),
  BOOL_MEMBER   (Slicing.MakeDecor, "MakeDecor", true, true),
  INT_MEMBER    (Slicing.DecorFilltype, "DecorFilltype", 0, true),
  INT_MEMBER    (Slicing.DecorLayers, "DecorLayers", 0, true),
  FLOAT_MEMBER  (Slicing.DecorInfillRotation, "DecorInfillRotation", 0, true),
  FLOAT_MEMBER  (Slicing.DecorInfillDistance, "DecorInfillDistance", 2.0, true),
  //INT_MEMBER    (Slicing.SolidLayers, "SolidLayers", 2, true),
  FLOAT_MEMBER  (Slicing.SolidThickness, "SolidThickness", 0.4, true),
  BOOL_MEMBER   (Slicing.NoTopAndBottom, "NoTopAndBottom", false, true),
  BOOL_MEMBER   (Slicing.Support, "Support", true, true),
  FLOAT_MEMBER  (Slicing.SupportAngle, "SupportAngle", 0, true),
  FLOAT_MEMBER  (Slicing.SupportWiden, "SupportWiden", 0, true),
  BOOL_MEMBER   (Slicing.Skirt, "Skirt", false, true),
  BOOL_MEMBER   (Slicing.SingleSkirt, "SingleSkirt", true, true),
  FLOAT_MEMBER  (Slicing.SkirtHeight, "SkirtHeight", 0.0, true),
  FLOAT_MEMBER  (Slicing.SkirtDistance, "SkirtDistance", 3, true),
  BOOL_MEMBER   (Slicing.FillSkirt, "FillSkirt", false, true),
  INT_MEMBER    (Slicing.Skins, "Skins", 1, true),
  BOOL_MEMBER   (Slicing.Varslicing, "Varslicing", false, true),
  BOOL_MEMBER   (Slicing.DoInfill, "DoInfill", true, true),
  INT_MEMBER    (Slicing.ShellCount, "ShellCount", 1, true),
  // BOOL_MEMBER   (Slicing.EnableAcceleration, "EnableAcceleration", true, false),
  FLOAT_MEMBER  (Slicing.MinShelltime, "MinShelltime", 5, true),
  FLOAT_MEMBER  (Slicing.MinLayertime, "MinLayertime", 5, true),
  BOOL_MEMBER   (Slicing.FanControl, "FanControl", false, false),
  INT_MEMBER    (Slicing.MinFanSpeed, "MinFanSpeed", 150, false),
  INT_MEMBER    (Slicing.MaxFanSpeed, "MaxFanSpeed", 255, false),
  FLOAT_MEMBER    (Slicing.MaxOverhangSpeed, "MaxOverhangSpeed", 20, false),

  //FLOAT_MEMBER  (Slicing.Optimization, "Optimization", 0.01, true),
  BOOL_MEMBER   (Slicing.BuildSerial, "BuildSerial", false, true),
  BOOL_MEMBER   (Slicing.SelectedOnly, "SelectedOnly", false, true),
  FLOAT_MEMBER  (Slicing.ShellOffset, "ShellOffset", 0.1, true),
  INT_MEMBER    (Slicing.FirstLayersNum, "FirstLayersNum", 1, true),
  FLOAT_MEMBER  (Slicing.FirstLayersSpeed, "FirstLayersSpeed", 0.5, true),
  FLOAT_MEMBER  (Slicing.FirstLayersInfillDist, "FirstLayersInfillDist", 0.8, true),
  FLOAT_MEMBER  (Slicing.FirstLayerHeight, "FirstLayerHeight", 0.7, true),
  BOOL_MEMBER   (Slicing.UseArcs, "UseArcs", false, true),
  FLOAT_MEMBER  (Slicing.ArcsMaxAngle, "ArcsMaxAngle", 20, true),
  FLOAT_MEMBER  (Slicing.MinArcLength, "MinArcLength", 1, true),
  BOOL_MEMBER   (Slicing.RoundCorners, "RoundCorners", true, true),
  FLOAT_MEMBER  (Slicing.CornerRadius, "CornerRadius", 1., true),
  BOOL_MEMBER   (Slicing.NoBridges, "NoBridges", false, true),
  FLOAT_MEMBER  (Slicing.BridgeExtrusion, "BridgeExtrusion", 1, true),

  BOOL_MEMBER (Slicing.GCodePostprocess, "GCodePostprocess", false, false),
  STRING_MEMBER (Slicing.GCodePostprocessor, "GCodePostprocessor", "", false),

  // Milling
  FLOAT_MEMBER  (Milling.ToolDiameter, "ToolDiameter", 2, true),

  // Misc.
  BOOL_MEMBER (Misc.SpeedsAreMMperSec, "SpeedsAreMMperSec", false, false),
  BOOL_MEMBER (Misc.ShapeAutoplace, "ShapeAutoplace", true, false),
  //BOOL_MEMBER (Misc.FileLoggingEnabled, "FileLoggingEnabled", true, false),
  BOOL_MEMBER (Misc.TempReadingEnabled, "TempReadingEnabled", true, false),
  BOOL_MEMBER (Misc.SaveSingleShapeSTL, "Misc.SaveSingleShapeSTL", false, false),

  // GCode - handled by GCodeImpl
  BOOL_MEMBER (Display.DisplayGCode, "DisplayGCode", true, true),
  FLOAT_MEMBER (Display.GCodeDrawStart, "GCodeDrawStart", 0.0, true),
  FLOAT_MEMBER (Display.GCodeDrawEnd, "GCodeDrawEnd", 1.0, true),
  BOOL_MEMBER (Display.DisplayGCodeBorders, "DisplayGCodeBorders", true, true),
  BOOL_MEMBER (Display.DisplayGCodeMoves, "DisplayGCodeMoves", true, true),
  BOOL_MEMBER (Display.DisplayGCodeArrows, "DisplayGCodeArrows", true, true),
  BOOL_MEMBER (Display.DisplayEndpoints, "DisplayEndpoints", false, true),
  BOOL_MEMBER (Display.DisplayNormals, "DisplayNormals", false, true),
  BOOL_MEMBER (Display.DisplayBBox, "DisplayBBox", false, true),
  BOOL_MEMBER (Display.DisplayWireframe, "DisplayWireframe", false, true),
  BOOL_MEMBER (Display.DisplayWireframeShaded, "DisplayWireframeShaded", true, true),
  BOOL_MEMBER (Display.DisplayPolygons, "DisplayPolygons", true, true),
  BOOL_MEMBER (Display.DisplayAllLayers, "DisplayAllLayers", false, true),
  BOOL_MEMBER (Display.DisplayinFill, "DisplayinFill", false, true),
  BOOL_MEMBER (Display.DisplayDebuginFill, "DisplayDebuginFill", false, true),
  BOOL_MEMBER (Display.DisplayDebug, "DisplayDebug", false, true),
  BOOL_MEMBER (Display.DisplayDebugArcs, "DisplayDebugArcs", true, true),
  BOOL_MEMBER (Display.DebugGCodeExtruders, "Display.DebugGCodeExtruders", false, true),
  BOOL_MEMBER (Display.DebugGCodeOffset, "Display.DebugGCodeOffset", true, true),
  BOOL_MEMBER (Display.DisplayFilledAreas, "DisplayFilledAreas", true, true),
  BOOL_MEMBER (Display.ShowLayerOverhang, "ShowLayerOverhang", true, true),
  BOOL_MEMBER (Display.CommsDebug, "CommsDebug", false, true),
  BOOL_MEMBER (Display.TerminalProgress, "TerminalProgress", false, true),
  BOOL_MEMBER (Display.DisplayLayer, "DisplayLayer", false, true),
  BOOL_MEMBER (Display.RandomizedLines, "RandomizedLines", false, true),
  BOOL_MEMBER (Display.DrawVertexNumbers, "DrawVertexNumbers", false, true),
  BOOL_MEMBER (Display.DrawLineNumbers, "DrawLineNumbers", false, true),
  BOOL_MEMBER (Display.DrawOutlineNumbers, "DrawOutlineNumbers", false, true),
  BOOL_MEMBER (Display.DrawCPVertexNumbers, "DrawCPVertexNumbers", false, true),
  BOOL_MEMBER (Display.DrawCPLineNumbers, "DrawCPLineNumbers", false, true),
  BOOL_MEMBER (Display.DrawCPOutlineNumbers, "DrawCPOutlineNumbers", false, true),
  BOOL_MEMBER (Display.DrawRulers, "DrawRulers", true, true),

  FLOAT_MEMBER (Display.LayerValue, "LayerValue", 0, true),
  BOOL_MEMBER  (Display.LuminanceShowsSpeed, "LuminanceShowsSpeed", false, true),
  FLOAT_MEMBER (Display.Highlight, "Highlight", 0.7, true),
  FLOAT_MEMBER (Display.NormalsLength, "NormalsLength", 10, true),
  FLOAT_MEMBER (Display.EndPointSize, "EndPointSize", 8, true),
  FLOAT_MEMBER (Display.TempUpdateSpeed, "TempUpdateSpeed", 3, false),

  BOOL_MEMBER (Display.PreviewLoad, "PreviewLoad", true, true),

  // Colour selectors settings

  COLOUR_MEMBER(Display.PolygonRGBA,
		"Display.PolygonColour", 0, 1.0, 1.0, 0.5, true),
  COLOUR_MEMBER(Display.WireframeRGBA,
		"Display.WireframeColour", 1.0, 0.48, 0, 0.5, true),
  COLOUR_MEMBER(Display.NormalsRGBA,
		"Display.NormalsColour", 0.62, 1.0, 0, 1.0, true),
  /* COLOUR_MEMBER(Display.GCodeExtrudeRGBA, */
  /* 		"Display.GCodeExtrudeColour", 1.0, 1.0, 0.0, 1.0, true), */
  COLOUR_MEMBER(Display.GCodeMoveRGBA,
		"Display.GCodeMoveColour", 1.0, 0.05, 1, 0.5, true),
  COLOUR_MEMBER(Display.GCodePrintingRGBA,
		"Display.GCodePrintingColour", 0.1, 0.5, 0.0, 1.0, true)

};

// Add any GtkSpinButtons to this array:
static struct {
  const char *widget;
  float min, max;
  float inc, inc_page;
} spin_ranges[] = {
  // Raft
  { "Raft.Size", 0, 50, 1, 3 },
  { "Raft.Base.LayerCount",      0, 8, 1, 2 },
  { "Raft.Interface.LayerCount", 0, 8, 1, 2 },
  { "Raft.Base.MaterialDistanceRatio",      0.1, 4.0, 0.1, 1 },
  { "Raft.Interface.MaterialDistanceRatio", 0.1, 4.0, 0.1, 1 },
  { "Raft.Base.Rotation",             -360.0, 360.0, 45, 90 },
  { "Raft.Interface.Rotation",        -360.0, 360.0, 45, 90 },
  { "Raft.Base.RotationPrLayer",      -360.0, 360.0, 45, 90 },
  { "Raft.Interface.RotationPrLayer", -360.0, 360.0, 45, 90 },
  { "Raft.Base.Distance",      0.1, 8.0, 0.1, 1 },
  { "Raft.Interface.Distance", 0.1, 8.0, 0.1, 1 },
  { "Raft.Base.Thickness",      0.1, 4.0, 0.1, 1 },
  { "Raft.Interface.Thickness", 0.1, 4.0, 0.1, 1 },
  { "Raft.Base.Temperature",      0.9, 1.2, 0.01, 0.1 },
  { "Raft.Interface.Temperature", 0.9, 1.2, 0.01, 0.1 },

  // Slicing
  { "Slicing.LayerThickness", 0.01, 3.0, 0.01, 0.2 },
  { "Slicing.ShellCount", 0, 100, 1, 5 },
  // { "Slicing.SolidLayers", 0, 100, 1, 5 },
  { "Slicing.SolidThickness", 0, 10, 0.01, 0.1 },
  { "Slicing.InfillRotation", -360, 360, 5, 45 },
  { "Slicing.InfillRotationPrLayer", -360, 360, 5, 90 },
  { "Slicing.InfillPercent", 0.0, 100.0, 1, 10.0 },
  { "Slicing.AltInfillPercent", 0.0, 100.0, 1, 10.0 },
  { "Slicing.AltInfillLayers", 0, 10000, 10, 100 },
  { "Slicing.DecorLayers", 1, 100, 1, 5 },
  { "Slicing.DecorInfillDistance", 0.0, 10, 0.1, 1 },
  { "Slicing.DecorInfillRotation", -360, 360, 5, 45 },
  { "Slicing.InfillOverlap", 0, 1.0 , 0.01, 0.1},
  { "Slicing.NormalFillExtrusion", 0.01, 3, 0.01, 0.1},
  { "Slicing.FullFillExtrusion", 0.01, 3, 0.01, 0.1},
  { "Slicing.BridgeExtrusion", 0.01, 3, 0.01, 0.1},
  { "Slicing.SupportExtrusion", 0.01, 3, 0.01, 0.1},
  { "Slicing.SupportInfillDistance", 0.0, 10, 0.1, 1 },
  { "Slicing.SupportWiden", -5, 5, 0.01, 0.1},
  { "Slicing.SupportAngle", 0, 90, 1, 10},
  //{ "Slicing.Optimization", 0.0, 10.0, 0.01, 0.1 },
  { "Slicing.SkirtHeight", 0.0, 1000, 0.1, 1 },
  { "Slicing.SkirtDistance", 0.0, 100, 0.1, 1 },
  { "Slicing.Skins", 1, 5, 1, 1 },
  { "Slicing.MinShelltime", 0.0, 100, 0.1, 1 },
  { "Slicing.MinLayertime", 0.0, 100, 0.1, 1 },
  { "Slicing.MinFanSpeed", 0, 255, 5, 25 },
  { "Slicing.MaxFanSpeed", 0, 255, 5, 25 },
  { "Slicing.MaxOverhangSpeed", 0, 1000, 1, 10 },
  //{ "Slicing.SerialBuildHeight", 0.0, 1000.0, 0.1, 1 },
  { "Slicing.ShellOffset", -10, 10, 0.01, 0.1 },
  { "Slicing.FirstLayersNum", 0, 1000, 1, 10 },
  { "Slicing.FirstLayersSpeed", 0.01, 3, 0.01, 0.1 },
  { "Slicing.FirstLayersInfillDist", 0.0, 100, 0.01, 0.1 },
  { "Slicing.FirstLayerHeight", 0.0, 100., 0.01, 0.1 },
  { "Slicing.ArcsMaxAngle", 0, 180, 1, 10 },
  { "Slicing.MinArcLength", 0, 10, 0.01, 0.1 },
  { "Slicing.CornerRadius", 0, 5, 0.01, 0.1 },

  // Milling
  { "Milling.ToolDiameter", 0, 5, 0.01, 0.1 },

  // Hardware
  { "Hardware.Volume.X", 0.0, 1000.0, 5.0, 25.0 },
  { "Hardware.Volume.Y", 0.0, 1000.0, 5.0, 25.0 },
  { "Hardware.Volume.Z", 0.0, 1000.0, 5.0, 25.0 },
  { "Hardware.PrintMargin.X", 0.0, 100.0, 1.0, 5.0 },
  { "Hardware.PrintMargin.Y", 0.0, 100.0, 1.0, 5.0 },
  { "Hardware.PrintMargin.Z", 0.0, 100.0, 1.0, 5.0 },
  { "Hardware.MinMoveSpeedXY", 0.1, 2000.0, 1.0, 10.0 },
  { "Hardware.MaxMoveSpeedXY", 0.1, 2000.0, 1.0, 10.0 },
  { "Hardware.MinMoveSpeedZ", 0.1, 250.0, 1.0, 10.0 },
  { "Hardware.MaxMoveSpeedZ", 0.1, 250.0, 1.0, 10.0 },
  { "Hardware.ReceivingBufferSize", 1.0, 100.0, 1.0, 5.0 },
  { "Hardware.KeepLines", 100.0, 100000.0, 1.0, 500.0 },
  // { "Hardware.DistanceToReachFullSpeed", 0.0, 10.0, 0.1, 1.0 },

  // Extruder
  { "Extruder.OffsetX", -5000.0, 5000.0, 0.1, 1.0 },
  { "Extruder.OffsetY", -5000.0, 5000.0, 0.1, 1.0 },
  { "Extruder.ExtrudedMaterialWidthRatio", 0.0, 10.0, 0.01, 0.1 },
  { "Extruder.MinimumLineWidth", 0.0, 10.0, 0.01, 0.1 },
  { "Extruder.MaximumLineWidth", 0.0, 10.0, 0.01, 0.1 },
  { "Extruder.ExtrusionFactor", 0.0, 10.0, 0.1, 0.5 },
  { "Extruder.FilamentDiameter", 0.5, 5.0, 0.01, 0.05 },
  { "Extruder.MaxLineSpeed", 0.1, 2000.0, 1.0, 10.0 },
  { "Extruder.MaxShellSpeed", 0.1, 2000.0, 1.0, 10.0 },
  { "Extruder.EMaxSpeed", 0.01, 2000.0, 0.1, 1.0 },
  // { "Extruder.DownstreamMultiplier", 0.01, 25.0, 0.01, 0.1 },
  // { "Extruder.DownstreamExtrusionMultiplier", 0.01, 25.0, 0.01, 0.1 },
  { "Extruder.AntioozeDistance", 0.0, 25.0, 0.1, 1 },
  { "Extruder.AntioozeAmount", 0.0, 25.0, 0.1, 1 },
  { "Extruder.AntioozeSpeed", 0.0, 1000.0, 1.0, 5.0 },
  //{ "Extruder.AntioozeHaltRatio", 0.0, 1.0, 0.01, 0.1 },
  { "Extruder.AntioozeZlift", 0.0, 10, 0.01, 0.1 },

  //Printer
  { "Printer.ExtrudeAmount", -1000.0, 1000.0, 0.1, 1.0 },
  { "Printer.ExtrudeSpeed", 0.0, 100.0, 0.1, 1.0 },
  { "Printer.FanVoltage", 0, 255, 5, 25 },
  // { "Printer.NozzleTemp", 0.0, 300.0, 1.0, 10.0 },
  // { "Printer.BedTemp", 0.0, 200.0, 1.0, 10.0 },

  // Display pane
  { "Display.TempUpdateSpeed", 1, 1000, 1, 5 },
  { "m_scale_value", 0.0001, 1000.0, 0.01, 0.1 },
  { "scale_x", 0.0001, 1000.0, 0.01, 0.1 },
  { "scale_y", 0.0001, 1000.0, 0.01, 0.1 },
  { "scale_z", 0.0001, 1000.0, 0.01, 0.1 },
  { "translate_x", -5000, 5000.0, 1, 10 },
  { "translate_y", -5000, 5000.0, 1, 10 },
  { "translate_z", -5000, 5000.0, 0.1, 1 },
  { "rot_x", -360.0, 360.0, 1, 10 },
  { "rot_y", -360.0, 360.0, 1, 10 },
  { "rot_z", -360.0, 360.0, 1, 10 },
};

// Add any [HV]Ranges to this array:
static struct {
  const char *widget;
  float min, max;
  float inc, inc_page;
} ranges[] = {
  // Display plane
  { "Display.LayerValue", 0.0, 1000.0, 0.0, 0.01 },
  { "Display.GCodeDrawStart", 0.0, 1000.0, 0.0, 0.1 },
  { "Display.GCodeDrawEnd", 0.0, 1000.0, 0.0, 0.1 },
};

static struct {
  uint  member_offset;
  const char *glade_name;
}
colour_selectors[] = {
  { OFFSET(Extruder.DisplayRGBA), "Extruder.DisplayRGBA" },
  { OFFSET(Display.PolygonRGBA), "Display.PolygonRGBA" },
  { OFFSET(Display.WireframeRGBA), "Display.WireframeRGBA" },
  { OFFSET(Display.NormalsRGBA), "Display.NormalsRGBA" },
  { OFFSET(Display.EndpointsRGBA), "Display.EndpointsRGBA" },
  /* { OFFSET(Display.GCodeExtrudeRGBA), "Display.GCodeExtrudeRGBA" }, */
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
      try {
	if (cfg.has_key ("GCode", GCodeNames[i]))
	  m_GCode[i]->set_text(cfg.get_string ("GCode", GCodeNames[i]));
      } catch (const Glib::KeyFileError &err) {
      }
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

std::string Settings::GCodeType::getText(GCodeTextType t) const
{
  return m_impl->m_GCode[t]->get_text();
}

// return infill distance in mm
double Settings::GetInfillDistance(double layerthickness, float percent) const
{
  double fullInfillDistance =
    Extruder.GetExtrudedMaterialWidth(layerthickness);
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
  m_user_changed = false;
}

Settings::~Settings()
{
  delete GCode.m_impl;
}

void
Settings::assign_from(Settings *pSettings)
{
  // default copy operators can be simply wonderful
  *this = *pSettings;
  m_user_changed = false;
  m_signal_visual_settings_changed.emit();
  m_signal_update_settings_gui.emit();
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

  GCode.m_impl->setDefaults();

  Extruders.clear();
  Extruders.push_back(Extruder);

  // The vectors map each to 3 spin boxes, one per dimension
  Hardware.Volume = Vector3d (200,200,140);
  Hardware.PrintMargin = Vector3d (10,10,0);

  Misc.SpeedsAreMMperSec = true;
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


// load settings of group onlygroup into setting group as_group
// (if given)
// as_group must have same settings as onlygroup (Extruders)
void Settings::load_settings_as (const Glib::KeyFile &cfg,
				 const Glib::ustring onlygroup,
				 const Glib::ustring as_group)
{
  for (uint i = 0; i < G_N_ELEMENTS (settings); i++) {

    Glib::ustring group, key;

    if (!get_group_and_key (i, group, key))
      continue;

    if (as_group != "") { // if as_group given
      if (as_group != group.substr(0,as_group.length()))
	continue;         // don't load other settings
    }

    if (onlygroup != "") // if onlygroup given
      group = onlygroup; // load setting from onlygroup

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

}
void Settings::load_settings (Glib::RefPtr<Gio::File> file)
{

  Glib::KeyFile cfg;

  Filename = file->get_path();

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

  std::cerr << _("Parsing config from '") << file->get_path() << "\n";

  Misc.SpeedsAreMMperSec = false;

  load_settings_as(cfg);

  // first extruder is the loaded "Extruder"
  if (Extruders.size()==0)
    Extruders.push_back(Extruder);
  else
    Extruders[0] = Extruder;
  // Load other extruders "ExtruderN" (N=2..)
  std::vector< Glib::ustring > all_groups = cfg.get_groups();
  for (uint i = 0; i<all_groups.size(); i++) {
    if (all_groups[i].substr(0,8) == "Extruder") {
      if (all_groups[i].length() > 8) {
	load_settings_as(cfg, all_groups[i], "Extruder");
	Extruders.push_back(Extruder);
      }
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
    Misc.window_posx = cfg.get_integer ("Misc", "WindowPosX");
    Misc.window_posy = cfg.get_integer ("Misc", "WindowPosY");
  } catch (const Glib::KeyFileError &err) {
    Misc.window_posx =-1;
    Misc.window_posy=-1;
  }
  try {
    Misc.ExpandLayerDisplay = cfg.get_boolean ("Misc", "ExpandLayerDisplay");
    Misc.ExpandModelDisplay = cfg.get_boolean ("Misc", "ExpandModelDisplay");
    Misc.ExpandPAxisDisplay = cfg.get_boolean ("Misc", "ExpandPAxisDisplay");
  } catch (const Glib::KeyFileError &err) {
    Misc.ExpandLayerDisplay = false;
    Misc.ExpandModelDisplay = false;
    Misc.ExpandPAxisDisplay = false;
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


  // if loading old settings with mm/min instead of mm/sec, recalc:
  if (!Misc.SpeedsAreMMperSec) {
    cerr << "Feedrates to mm/sec" << endl;
    Hardware.MinMoveSpeedXY /= 60.0;
    Hardware.MaxMoveSpeedXY /= 60.0;
    Hardware.MinMoveSpeedZ /= 60.0;
    Hardware.MaxMoveSpeedZ /= 60.0;
    Extruder.EMaxSpeed /= 60.0;
    Extruder.MaxLineSpeed /= 60.0;
    Extruder.MaxShellSpeed /= 60.0;
    Printer.ExtrudeSpeed /= 60.0;
    Extruder.AntioozeSpeed /= 60.0;
    Slicing.MaxOverhangSpeed /= 60.0;
  }
  Misc.SpeedsAreMMperSec = true;

  m_user_changed = false;
  m_signal_visual_settings_changed.emit();
  m_signal_update_settings_gui.emit();
}

void Settings::save_settings_as(Glib::KeyFile &cfg,
				const Glib::ustring onlygroup,
				const Glib::ustring as_group)
{
  for (uint i = 0; i < G_N_ELEMENTS (settings); i++) {
    Glib::ustring group, key;

    if (!get_group_and_key (i, group, key))
      continue;

    if (onlygroup != "") { // if onlygroup given
      if (onlygroup != group.substr(0,onlygroup.length()))
	continue;          // only save onlygroup
      if (as_group != "")  // and use name as_group if given
	group = as_group;
    }

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
}

void Settings::save_settings(Glib::RefPtr<Gio::File> file)
{
  Glib::KeyFile cfg;

  Extruder = Extruders[0]; // save first extruder as "Extruder"

  save_settings_as(cfg); // all settings

  ostringstream os; os << Misc.window_width;
  cfg.set_string("Misc", "WindowWidth", os.str());
  os.str(""); os << Misc.window_height;
  cfg.set_string("Misc", "WindowHeight", os.str());

  os.str(""); os << Misc.window_posx;
  cfg.set_string("Misc", "WindowPosX", os.str());
  os.str(""); os << Misc.window_posy;
  cfg.set_string("Misc", "WindowPosY", os.str());

  cfg.set_boolean("Misc", "ExpandLayerDisplay", Misc.ExpandLayerDisplay);
  cfg.set_boolean("Misc", "ExpandModelDisplay", Misc.ExpandModelDisplay);
  cfg.set_boolean("Misc", "ExpandPAxisDisplay", Misc.ExpandPAxisDisplay);

  GCode.m_impl->saveSettings (cfg);

  string CBgroup="CustomButtons";
  for (guint i=0; i<CustomButtonLabel.size(); i++)  {
    string s = CustomButtonLabel[i];
    std::replace(s.begin(),s.end(),' ','_');
    cfg.set_string(CBgroup, s, CustomButtonGcode[i]);
  }

  // save extruders 2.. as ExtruderN
  for (uint e = 1; e < Extruders.size(); e++) {
    Extruder = Extruders[e];
    ostringstream o; o << "Extruder" << e+1 ;
    save_settings_as(cfg, "Extruder", o.str());
  }


  Glib::ustring contents = cfg.to_data();
  Glib::file_set_contents (file->get_path(), contents);

  // all changes safely saved
  m_user_changed = false;
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
}


void Settings::set_filltypes_to_gui (Builder &builder)
{
  // cerr << Slicing.NormalFilltype << " ! " << Slicing.FullFilltype
  //      << " ! " << Slicing.SupportFilltype<< endl;
  Gtk::ComboBox *combo = NULL;
  // avoid getting overwritten by callback
  uint norm    = Slicing.NormalFilltype;
  uint full    = Slicing.FullFilltype;
  uint support = Slicing.SupportFilltype;
  uint decor   = Slicing.DecorFilltype;
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
  bool is_changed = false;
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
    else {
      is_changed = *PTR_BOOL(this, i) != check->get_active();
      *PTR_BOOL(this, i) = check->get_active();
    }
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

    double value = 0.0;
    Gtk::SpinButton *spin = dynamic_cast<Gtk::SpinButton *>(w);
    if (spin)
      value = spin->get_value();
    Gtk::Range *range = dynamic_cast<Gtk::Range *>(w);
    if (range)
      value = range->get_value();

    if (range || spin)
    {
      if (t == T_INT)
      {
	is_changed = *PTR_INT(this, i) != (int)value;
	*PTR_INT(this, i) = value;
      }
      else if (t == T_FLOAT)
      {
	is_changed = *PTR_FLOAT(this, i) != (float)value;
	*PTR_FLOAT(this, i) = value;
      }
      else
      {
	is_changed = *PTR_DOUBLE(this, i) != value;
	*PTR_DOUBLE(this, i) = value;
      }
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
    // Ignore, colour members are special -> get_colour_from_gui
    break;
  default:
    std::cerr << _("corrupt setting type") << glade_name << endl;;
    break;
  }

  m_user_changed |= true; // is_changed;


  if (string(glade_name).substr(0,8) == "Extruder") {
    // copy settings to selected Extruder
    if (selectedExtruder < Extruders.size()) {
      Extruders[selectedExtruder] = Extruder;
      // only one extruders must be selected for support
      if (Extruder.UseForSupport) {
	for (uint i = 0; i < Extruders.size(); i++)
	  if (i != selectedExtruder)
	    Extruders[i].UseForSupport = false;
      } else
	Extruders[0].UseForSupport = true;
    }
  }


  // bit of a hack ...
  if (!strcmp (settings[i].config_name, "CommsDebug"))
    m_signal_core_settings_changed.emit();

  if (settings[i].triggers_redraw && is_changed)
    m_signal_visual_settings_changed.emit();
}

static bool get_filltype(Builder &builder, const char *combo_name, int *type)
{
  Gtk::ComboBox *combo = NULL;
  bool is_changed = false;
  builder->get_widget (combo_name, combo);
  if (combo) {
    int value = combo->get_active_row_number ();
    is_changed |= *type != value;
    *type = value;
  }
  else
    cerr << "no " << combo_name << "combo" << endl;
  return is_changed;
}

void Settings::get_filltypes_from_gui (Builder &builder)
{
  bool is_changed = false;
  // cerr <<"Get_filltypes " << endl;
  is_changed |= get_filltype(builder, "Slicing.NormalFilltype",  &Slicing.NormalFilltype);
  is_changed |= get_filltype(builder, "Slicing.FullFilltype",    &Slicing.FullFilltype);
  is_changed |= get_filltype(builder, "Slicing.SupportFilltype", &Slicing.SupportFilltype);
  is_changed |= get_filltype(builder, "Slicing.DecorFilltype",   &Slicing.DecorFilltype);
  // cerr << "read combos: " << Slicing.NormalFilltype
  //      <<  " / " << Slicing.FullFilltype
  //      <<  " / " << Slicing.SupportFilltype << endl;
  m_user_changed |= is_changed;
  if (is_changed)
    m_signal_visual_settings_changed.emit();
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

bool combobox_set_to(Gtk::ComboBox *combo, string value)
{
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
  if (combo->get_model())
    return;
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
  int serial_speed = Hardware.SerialSpeed;
  if (combo) {
#if GTK_VERSION_GE(2, 24)
    if (combo->get_has_entry()) {
      Gtk::Entry *entry = combo->get_entry();
      if (entry) {
	serial_speed = atoi(entry->get_text().c_str());
      }
    }
    else
#endif
      serial_speed = atoi(combobox_get_active_value(combo).c_str());
  }
  m_user_changed |= Hardware.SerialSpeed != serial_speed;
  Hardware.SerialSpeed = serial_speed;
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

  // FIXME: detect 'changed' etc.
  dest->r() = c.get_red_p();
  dest->g() = c.get_green_p();
  dest->b() = c.get_blue_p();
  dest->a() = (float) (w->get_alpha()) / 65535.0;

  if (string(glade_name).substr(0,8) == "Extruder") {
    // copy settings to selected Extruder
    if (selectedExtruder < Extruders.size())
      Extruders[selectedExtruder] = Extruder;
  }
  m_signal_visual_settings_changed.emit();
}

void Settings::set_to_gui (Builder &builder, const string filter)
{
  for (uint i = 0; i < G_N_ELEMENTS (settings); i++) {
    const char *glade_name = settings[i].glade_name;

    if (!glade_name)
      continue;
    if (filter != "" && string(glade_name).substr(0,filter.length()) != filter) {
      continue;
    }
    set_to_gui (builder, i);
  }

  set_filltypes_to_gui (builder);

  for (uint i = 0; i < G_N_ELEMENTS (colour_selectors); i++) {
      const char *glade_name = colour_selectors[i].glade_name;
      if (filter != "" && string(glade_name).substr(0,filter.length()) != filter)
	continue;
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
  if (filter == "" || filter == "Hardware") {
    Gtk::ComboBox *portspeed = NULL;
    builder->get_widget ("Hardware.SerialSpeed", portspeed);
    if (portspeed) {
      std::ostringstream ostr;
      ostr << Hardware.SerialSpeed;
      combobox_set_to(portspeed, ostr.str());
    }
  }

  if (filter == "" || filter == "Misc") {
    Gtk::Window *pWindow = NULL;
    builder->get_widget("main_window", pWindow);
    if (pWindow && Misc.window_width > 0 && Misc.window_height > 0)
      pWindow->resize(Misc.window_width, Misc.window_height);
    if (pWindow && Misc.window_posx > 0 && Misc.window_posy > 0)
      pWindow->move(Misc.window_posx,Misc.window_posy);
    Gtk::Expander *exp = NULL;
    builder->get_widget ("layer_expander", exp);
    if (exp)
      exp->set_expanded(Misc.ExpandLayerDisplay);
    builder->get_widget ("model_expander", exp);
    if (exp)
      exp->set_expanded(Misc.ExpandModelDisplay);
    builder->get_widget ("printeraxis_expander", exp);
    if (exp)
      exp->set_expanded(Misc.ExpandPAxisDisplay);
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
    case T_STRING: {
      Gtk::Entry *e = NULL;
      builder->get_widget (glade_name, e);
      if (!e) {
	std::cerr << _("Missing user interface item ") << glade_name << "\n";
	break;
      }
      e->signal_changed().connect
	(sigc::bind(sigc::bind(sigc::mem_fun(*this, &Settings::get_from_gui), i), builder));
      break;
    }
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


// extrusion ratio for round-edge lines
double Settings::ExtruderSettings::RoundedLinewidthCorrection(double extr_width,
							      double layerheight)
{
  double factor = 1 + (M_PI/4.-1) * layerheight/extr_width;
  // assume 2 half circles at edges
  //    /-------------------\     //
  //   |                     |    //
  //    \-------------------/     //
  //cerr << "round factor " << factor << endl;
  return factor;
}


double Settings::ExtruderSettings::GetExtrudedMaterialWidth(double layerheight) const
{
  // ExtrudedMaterialWidthRatio is preset by user
  return min(max((double)MinimumLineWidth,
		 ExtrudedMaterialWidthRatio * layerheight),
	     (double)MaximumLineWidth);
}

// TODO This depends whether lines are packed or not - ellipsis/rectangle

// how much mm filament material per extruded line length mm -> E gcode
double Settings::ExtruderSettings::GetExtrudeFactor(double layerheight) const
{
  double f = ExtrusionFactor; // overall factor
  if (CalibrateInput) {  // means we use input filament diameter
    double matWidth = GetExtrudedMaterialWidth(layerheight); // this is the goal
    // otherwise we just work back from the extruded diameter for now.
    f *= (matWidth * matWidth) / (FilamentDiameter * FilamentDiameter);
  } // else: we work in terms of output anyway;

  return f;
}

std::vector<char> Settings::get_extruder_letters() const
{
  std::vector<char> letters(Extruders.size());
  for (uint i = 0; i< Extruders.size(); i++)
    letters[i] = Extruders[i].GCLetter[0];
  return letters;
}

uint Settings::GetSupportExtruder() const
{
  for (uint i = 0; i< Extruders.size(); i++)
    if (Extruders[i].UseForSupport) return i;
  return 0;
}

Vector3d Settings::get_extruder_offset(uint num) const
{
  return Vector3d(Extruders[num].OffsetX, Extruders[num].OffsetY, 0.);
}

void Settings::CopyExtruder(uint num)
{
  if (num >= Extruders.size()) return;
  ExtruderSettings newsettings;
  if (Extruders.size() == 0)
    newsettings = Extruder;
  else
    newsettings = Extruders[num];
  Extruders.push_back(newsettings);
}
void Settings::RemoveExtruder(uint num)
{
  if (num >= Extruders.size()) return;
  if (Extruders.size() < 2) return;
  Extruders.erase(Extruders.begin()+num);
}
void Settings::SelectExtruder(uint num)
{
  if (num < Extruders.size()) {
    selectedExtruder = num;
  }
  Extruder = Extruders[selectedExtruder];
  // if (Slicing.UseTCommand) // use letter of first extruder
  //   Extruder.GCLetter = Extruders[0].GCLetter;
}

Matrix4d Settings::getBasicTransformation(Matrix4d T) const
{
  Vector3d t;
  T.get_translation(t);
  const Vector3d margin = getPrintMargin();
  t+= Vector3d(margin.x()+Raft.Size*Raft.Enable,
	       margin.y()+Raft.Size*Raft.Enable,
	       0);
  T.set_translation(t);
  return T;
}

Vector3d Settings::getPrintVolume() const
{
  return Hardware.Volume;
  // Vector3d maxoff = Vector3d::ZERO;
  // for (uint i = 0; i< Extruders.size(); i++) {
  //   if (abs(Extruders[i].OffsetX) > abs(maxoff.x()))
  //     maxoff.x() = Extruders[i].OffsetX;
  //   if (abs(Extruders[i].OffsetY) > abs(maxoff.y()))
  //     maxoff.y() = Extruders[i].OffsetY;
  // }
  // return vol - maxoff;
}
Vector3d Settings::getPrintMargin() const
{
  const Vector3d margin = Hardware.PrintMargin;
  Vector3d maxoff = Vector3d::ZERO;
  for (uint i = 0; i< Extruders.size(); i++) {
    if (abs(Extruders[i].OffsetX) > abs(maxoff.x()))
      maxoff.x() = abs(Extruders[i].OffsetX);
    if (abs(Extruders[i].OffsetY) > abs(maxoff.y()))
      maxoff.y() = abs(Extruders[i].OffsetY);
  }
  if (Slicing.Skirt) {
    maxoff += Vector3d(Slicing.SkirtDistance,Slicing.SkirtDistance,0);
  }
  return margin + maxoff;
}


// Locate it in relation to ourselves ...
std::string Settings::get_image_path()
{
  std::string basename = Glib::path_get_dirname(Filename);
  return Glib::build_filename (basename, Image);
}


