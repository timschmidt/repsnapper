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
#ifndef SETTINGS_H
#define SETTINGS_H

#include <string>
#include <giomm/file.h>

#include "stdafx.h"


// Allow passing as a pointer to something to
// avoid including glibmm in every header.
class Builder : public Glib::RefPtr<Gtk::Builder>
{
public:
  Builder() {}
  ~Builder() {}
};

class Settings {
 public:

  std::string Filename;

  bool   RaftEnable;
  struct RaftSettings {
    float Size;
    struct PhasePropertiesType {
      guint  LayerCount;
      float MaterialDistanceRatio;
      float Rotation;
      float RotationPrLayer;
      float Distance;
      float Thickness;
      float Temperature;
    };
    enum PhaseType { PHASE_BASE, PHASE_INTERFACE };
    PhasePropertiesType Phase[2];
  };
  RaftSettings Raft;

  struct HardwareSettings {
    float MinPrintSpeedXY;
    float MaxPrintSpeedXY;
    float MoveSpeed;
    float MinPrintSpeedZ;
    float MaxPrintSpeedZ;
    float EMaxSpeed;

    float MaxShellSpeed;

    bool  CalibrateInput; // hardware treats 'mm' as filament input mm not of nozzle output.
    float DistanceToReachFullSpeed;
    float ExtrusionFactor;
    float FilamentDiameter;

    float LayerThickness;

    vmml::vec3d	Volume;      // Print volume
    vmml::vec3d	PrintMargin;
    float ExtrudedMaterialWidthRatio; // ratio of with to (layer) height
    double GetExtrudedMaterialWidth(const double layerheight) const;

    std::string PortName;
    int SerialSpeed;
    bool ValidateConnection;
    int KeepLines;

    int ReceivingBufferSize;

    float DownstreamMultiplier;
    float DownstreamExtrusionMultiplier;
    static double RoundedLinewidthCorrection(double extr_width, double layerheight);
    double GetExtrudeFactor(double layerheight) const;
  };
  HardwareSettings Hardware;

  struct PrinterSettings {
    float ExtrudeAmount;
    float ExtrudeSpeed;
    float NozzleTemp;
    float BedTemp;
    int FanVoltage;
    bool Logging;
    bool ClearLogOnPrintStart;
  };
  PrinterSettings Printer;

  struct SlicingSettings {
    bool  RelativeEcode;
    bool  UseArcs;
    float ArcsMaxAngle;
    float MinArcLength;
    bool  RoundCorners;
    float CornerRadius;

    bool NoBridges;
    float BridgeExtrusion;

    bool  EnableAntiooze;
    float AntioozeDistance;
    float AntioozeAmount;
    float AntioozeSpeed;
    float AntioozeHaltRatio;
    float AntioozeZlift;

    float InfillPercent;
    float InfillRotation;
    float InfillRotationPrLayer;
    float AltInfillPercent;
    int AltInfillLayers;
    float InfillOverlap;
    //bool SolidTopAndBottom;
    //int SolidLayers;
    float SolidThickness;
    bool Support;
    float SupportWiden;
    bool Skirt;
    float SkirtHeight;
    float SkirtDistance;
    int Skins;
    bool Varslicing;
    int NormalFilltype;
    float NormalFillExtrusion;
    int FullFilltype;
    float FullFillExtrusion;
    int SupportFilltype;
    float SupportExtrusion;
    bool MakeDecor;
    int DecorFilltype;
    int DecorLayers;
    float DecorInfillDistance;
    float DecorInfillRotation;

    float MinLayertime;
    bool FanControl;
    int MinFanSpeed;
    int MaxFanSpeed;

    bool ShellOnly;
    guint ShellCount;
    /* bool EnableAcceleration; */
    //int ShrinkQuality;

    bool BuildSerial;
    //float SerialBuildHeight;

    float ShellOffset;
    //float Optimization;

    guint FirstLayersNum;
    float FirstLayersSpeed;
    float FirstLayersInfillDist;
    float FirstLayerHeight;
    //void GetAltInfillLayers(std::vector<int>& layers, guint layerCount) const;
  };
  SlicingSettings Slicing;

  struct MiscSettings {
    bool ShapeAutoplace;
    bool FileLoggingEnabled;
    bool TempReadingEnabled;
    bool ClearLogfilesWhenPrintStarts;
    int window_width;
    int window_height;
  };
  MiscSettings Misc;

  class GCodeImpl;
  enum GCodeTextType {
    GCODE_TEXT_START,
    GCODE_TEXT_LAYER,
    GCODE_TEXT_END,
    GCODE_TEXT_TYPE_COUNT
  };
  struct GCodeType {
    GCodeImpl *m_impl;
    std::string getText(GCodeTextType t);
    std::string getStartText() { return getText (GCODE_TEXT_START); }
    std::string getLayerText() { return getText (GCODE_TEXT_LAYER); }
    std::string getEndText()   { return getText (GCODE_TEXT_END);   }
  };
  GCodeType GCode;

  struct DisplaySettings {
    bool DisplayGCode;
    float GCodeDrawStart;
    float GCodeDrawEnd;

    bool DisplayEndpoints;
    bool DisplayNormals;
    bool DisplayBBox;
    bool DisplayWireframe;
    bool DisplayWireframeShaded;
    bool DisplayPolygons;
    bool DisplayAllLayers;
    bool DisplayinFill;
    bool DisplayDebuginFill;
    bool DisplayDebug;
    bool DisplayLayer;
    bool DisplayGCodeBorders;
    bool DisplayGCodeArrows;
    bool DrawVertexNumbers;
    bool DrawLineNumbers;
    bool DrawOutlineNumbers;
    bool DrawCPVertexNumbers;
    bool DrawCPLineNumbers;
    bool DrawCPOutlineNumbers;
    bool DrawMeasures;
    float LayerValue;
    bool LuminanceShowsSpeed;
    bool CommsDebug;
    bool TerminalProgress;

    // Rendering
    Vector4f PolygonRGBA;
    Vector4f WireframeRGBA;
    Vector4f NormalsRGBA;
    Vector4f EndpointsRGBA;
    Vector4f GCodeExtrudeRGBA;
    Vector4f GCodePrintingRGBA;
    Vector4f GCodeMoveRGBA;
    float    Highlight;
    float    NormalsLength;
    float    EndPointSize;
    float    TempUpdateSpeed;
  };
  DisplaySettings Display;

  // Paths we loaded / saved things to last time
  std::string STLPath;
  std::string RFOPath;
  std::string GCodePath;
  std::string SettingsPath;

  std::vector<std::string> CustomButtonGcode;
  std::vector<std::string> CustomButtonLabel;

 private:
  void set_to_gui              (Builder &builder, int i);
  void get_from_gui            (Builder &builder, int i);
  //void set_shrink_to_gui       (Builder &builder);
  //void get_shrink_from_gui     (Builder &builder);
  void set_filltypes_to_gui       (Builder &builder);
  void get_filltypes_from_gui       (Builder &builder);
  void get_port_speed_from_gui (Builder &builder);
  bool get_group_and_key       (int i, Glib::ustring &group, Glib::ustring &key);
  void get_colour_from_gui     (Builder &builder, int i);

  void set_defaults ();
 public:

  Settings ();
  ~Settings();

  Matrix4d getBasicTransformation(Matrix4d T) const;

  // return real mm depending on hardware extrusion width setting
  double GetInfillDistance(double layerthickness, float percent) const;

  // sync changed settings with the GUI eg. used post load
  void set_to_gui (Builder &builder);

  // connect settings to relevant GUI widgets
  void connect_to_ui (Builder &builder);

  void load_settings (Glib::RefPtr<Gio::File> file);
  void save_settings (Glib::RefPtr<Gio::File> file);

  sigc::signal< void > m_signal_visual_settings_changed;
  sigc::signal< void > m_signal_update_settings_gui;
  sigc::signal< void > m_signal_core_settings_changed;
};

#endif // SETTINGS_H
