// Retained to allow incremental porting to gtk+

#include "ui.h"
#include "modelviewcontroller.h"

void GUI::cb_OpenStl_i(Fl_Menu_*, void*) {
  FileChooser::ioDialog (MVC, FileChooser::OPEN, FileChooser::STL, true);
}
void GUI::cb_OpenStl(Fl_Menu_* o, void* v) {
  ((GUI*)(o->parent()->user_data()))->cb_OpenStl_i(o,v);
}

void GUI::cb_OpenGCode_i(Fl_Menu_*, void*) {
  FileChooser::ioDialog (MVC, FileChooser::OPEN, FileChooser::GCODE);
}
void GUI::cb_OpenGCode(Fl_Menu_* o, void* v) {
  ((GUI*)(o->parent()->user_data()))->cb_OpenGCode_i(o,v);
}

void GUI::cb_LoadFrom_i(Fl_Menu_*, void*) {
  Fl_File_Chooser chooser(MVC->ProcessControl.SettingsPath.c_str(), "*.conf", Fl_File_Chooser::SINGLE, "Load settings as...");
chooser.show();
while (chooser.shown())
Fl::wait();
if(chooser.value() == 0)
return;
std::string dir(chooser.value());


if(dir.length())
{
MVC->ProcessControl.SettingsPath = dir;
MVC->ProcessControl.LoadConfig(dir);
MVC->CopySettingsToGUI();
};
}
void GUI::cb_LoadFrom(Fl_Menu_* o, void* v) {
  ((GUI*)(o->parent()->user_data()))->cb_LoadFrom_i(o,v);
}

void GUI::cb_Save_i(Fl_Menu_*, void*) {
  MVC->ProcessControl.SaveSettings();
}
void GUI::cb_Save(Fl_Menu_* o, void* v) {
  ((GUI*)(o->parent()->user_data()))->cb_Save_i(o,v);
}

void GUI::cb_SaveAs_i(Fl_Menu_*, void*) {
  Fl_File_Chooser chooser(MVC->ProcessControl.SettingsPath.c_str(), "*.conf", Fl_File_Chooser::SINGLE|Fl_File_Chooser::CREATE, "Save Settings As...");
chooser.show();
while (chooser.shown())
Fl::wait();
if(chooser.value() == 0)
return;
std::string dir(chooser.value());


if(dir.length())
{
MVC->ProcessControl.SettingsPath = dir;
MVC->ProcessControl.SaveSettingsAs(dir);
};
}
void GUI::cb_SaveAs(Fl_Menu_* o, void* v) {
  ((GUI*)(o->parent()->user_data()))->cb_SaveAs_i(o,v);
}

void GUI::cb_PrinterSettings_i(Fl_Menu_*, void*) {
  printerSettingsWindow->show();
}
void GUI::cb_PrinterSettings(Fl_Menu_* o, void* v) {
  ((GUI*)(o->parent()->user_data()))->cb_PrinterSettings_i(o,v);
}

void GUI::cb_RaftSettings_i(Fl_Menu_*, void*) {
  raftSettingsWindow->show();
}
void GUI::cb_RaftSettings(Fl_Menu_* o, void* v) {
  ((GUI*)(o->parent()->user_data()))->cb_RaftSettings_i(o,v);
}

void GUI::cb_DisplaySettings_i(Fl_Menu_*, void*) {
  DisplaySettingsWindow->show();
}
void GUI::cb_DisplaySettings(Fl_Menu_* o, void* v) {
  ((GUI*)(o->parent()->user_data()))->cb_DisplaySettings_i(o,v);
}

void GUI::cb_CustomButtonSettings_i(Fl_Menu_*, void*) {
  CustomButtonsSetupWindow->show();
}
void GUI::cb_CustomButtonSettings(Fl_Menu_* o, void* v) {
  ((GUI*)(o->parent()->user_data()))->cb_CustomButtonSettings_i(o,v);
}

void GUI::cb_CalibrateSettings_i(Fl_Menu_*, void*) {
  PrinterCalibrationWindow->show();
}
void GUI::cb_CalibrateSettings(Fl_Menu_* o, void* v) {
  ((GUI*)(o->parent()->user_data()))->cb_CalibrateSettings_i(o,v);
}

void GUI::cb_SimpleAdvancedToggle_i(Fl_Menu_*, void*) {
  MVC->SimpleAdvancedToggle();
}
void GUI::cb_SimpleAdvancedToggle(Fl_Menu_* o, void* v) {
  ((GUI*)(o->parent()->user_data()))->cb_SimpleAdvancedToggle_i(o,v);
}

Fl_Menu_Item GUI::menu_MainMenuBar[] = {
 {"File", 0,  0, 0, 64, FL_NORMAL_LABEL, 0, 14, 0},
 {"Open Stl...", 0,  (Fl_Callback*)GUI::cb_OpenStl, 0, 0, FL_NORMAL_LABEL, 0, 14, 0},
 {"Open GCode...", 0,  (Fl_Callback*)GUI::cb_OpenGCode, 0, 0, FL_NORMAL_LABEL, 0, 14, 0},
 {"Preferences", 0,  0, 0, 64, FL_NORMAL_LABEL, 0, 14, 0},
 {"Load From...", 0,  (Fl_Callback*)GUI::cb_LoadFrom, 0, 0, FL_NORMAL_LABEL, 0, 14, 0},
 {"Save", 0,  (Fl_Callback*)GUI::cb_Save, 0, 0, FL_NORMAL_LABEL, 0, 14, 0},
 {"Save As...", 0,  (Fl_Callback*)GUI::cb_SaveAs, 0, 0, FL_NORMAL_LABEL, 0, 14, 0},
 {0,0,0,0,0,0,0,0,0},
 {0,0,0,0,0,0,0,0,0},
 {"Edit", 0,  0, 0, 64, FL_NORMAL_LABEL, 0, 14, 0},
 {"Preferences...", 0,  0, 0, 17, FL_NORMAL_LABEL, 0, 14, 0},
 {"Printer Settings...", 0,  (Fl_Callback*)GUI::cb_PrinterSettings, 0, 0, FL_NORMAL_LABEL, 0, 14, 0},
 {"Raft Settings...", 0,  (Fl_Callback*)GUI::cb_RaftSettings, 0, 0, FL_NORMAL_LABEL, 0, 14, 0},
 {"Display Settings...", 0,  (Fl_Callback*)GUI::cb_DisplaySettings, 0, 0, FL_NORMAL_LABEL, 0, 14, 0},
 {"Custom Button Setup...", 0,  (Fl_Callback*)GUI::cb_CustomButtonSettings, 0, 0, FL_NORMAL_LABEL, 0, 14, 0},
 {"Calibrate", 0,  (Fl_Callback*)GUI::cb_CalibrateSettings, 0, 0, FL_NORMAL_LABEL, 0, 14, 0},
 {0,0,0,0,0,0,0,0,0},
 {"Options", 0,  0, 0, 81, FL_NORMAL_LABEL, 0, 14, 0},
 {"Toggle Simple or Advanced", 0,  (Fl_Callback*)GUI::cb_SimpleAdvancedToggle, 0, 0, FL_NORMAL_LABEL, 0, 14, 0},
 {0,0,0,0,0,0,0,0,0},
 {0,0,0,0,0,0,0,0,0}
};
Fl_Menu_Item* GUI::Filemenu = GUI::menu_MainMenuBar + 0;
Fl_Menu_Item* GUI::OpenStl = GUI::menu_MainMenuBar + 1;
Fl_Menu_Item* GUI::OpenGCode = GUI::menu_MainMenuBar + 2;
Fl_Menu_Item* GUI::Preferences = GUI::menu_MainMenuBar + 3;
Fl_Menu_Item* GUI::LoadFrom = GUI::menu_MainMenuBar + 4;
Fl_Menu_Item* GUI::Save = GUI::menu_MainMenuBar + 5;
Fl_Menu_Item* GUI::SaveAs = GUI::menu_MainMenuBar + 6;
Fl_Menu_Item* GUI::EditMenu = GUI::menu_MainMenuBar + 9;
Fl_Menu_Item* GUI::PrinterSettings = GUI::menu_MainMenuBar + 11;
Fl_Menu_Item* GUI::RaftSettings = GUI::menu_MainMenuBar + 12;
Fl_Menu_Item* GUI::DisplaySettings = GUI::menu_MainMenuBar + 13;
Fl_Menu_Item* GUI::CustomButtonSettings = GUI::menu_MainMenuBar + 14;
Fl_Menu_Item* GUI::CalibrateSettings = GUI::menu_MainMenuBar + 15;
Fl_Menu_Item* GUI::OptionsMenu = GUI::menu_MainMenuBar + 17;
Fl_Menu_Item* GUI::SimpleAdvancedToggle = GUI::menu_MainMenuBar + 18;

void GUI::cb_Tabs_i(Fl_Tabs*, void*) {
  // here;
}
void GUI::cb_Tabs(Fl_Tabs* o, void* v) {
  ((GUI*)(o->parent()->user_data()))->cb_Tabs_i(o,v);
}

void GUI::cb_SerialSpeedInputSimple_i(Fl_Value_Input* o, void*) {
  MVC->setSerialSpeed(o->value());
}
void GUI::cb_SerialSpeedInputSimple(Fl_Value_Input* o, void* v) {
  ((GUI*)(o->parent()->parent()->parent()->parent()->user_data()))->cb_SerialSpeedInputSimple_i(o,v);
}

void GUI::cb_ConnectToPrinterSimpleButton_i(Fl_Light_Button* o, void*) {
MVC->ConnectToPrinter(o->value());
}
void GUI::cb_ConnectToPrinterSimpleButton(Fl_Light_Button* o, void* v) {
  ((GUI*)(o->parent()->parent()->parent()->parent()->user_data()))->cb_ConnectToPrinterSimpleButton_i(o,v);
}

void GUI::cb_portInputSimple_i(Fl_Input_Choice* o, void*) {
  MVC->setPort(std::string(o->input()->value()));
}
void GUI::cb_portInputSimple(Fl_Input_Choice* o, void* v) {
  ((GUI*)(o->parent()->parent()->parent()->parent()->user_data()))->cb_portInputSimple_i(o,v);
}

Fl_Menu_Item GUI::menu_portInputSimple[] = {
 {"COM1", 0,  0, (void*)(1), 4, FL_NORMAL_LABEL, 0, 14, 0},
 {"COM2", 0,  0, (void*)(2), 4, FL_NORMAL_LABEL, 0, 14, 0},
 {"COM3", 0,  0, (void*)(3), 4, FL_NORMAL_LABEL, 0, 14, 0},
 {"COM4", 0,  0, (void*)(4), 4, FL_NORMAL_LABEL, 0, 14, 0},
 {"COM5", 0,  0, (void*)(5), 4, FL_NORMAL_LABEL, 0, 14, 0},
 {"COM6", 0,  0, (void*)(6), 4, FL_NORMAL_LABEL, 0, 14, 0},
 {"COM7", 0,  0, (void*)(7), 4, FL_NORMAL_LABEL, 0, 14, 0},
 {"COM8", 0,  0, (void*)(8), 4, FL_NORMAL_LABEL, 0, 14, 0},
 {"COM9", 0,  0, (void*)(8), 4, FL_NORMAL_LABEL, 0, 14, 0},
 {0,0,0,0,0,0,0,0,0}
};

void GUI::cb_Load_i(Fl_Button*, void*) {
}
void GUI::cb_Load(Fl_Button* o, void* v) {
  ((GUI*)(o->parent()->parent()->parent()->parent()->user_data()))->cb_Load_i(o,v);
}

void GUI::cb_Convert_i(Fl_Button*, void*) {
  MVC->ConvertToGCode();
}
void GUI::cb_Convert(Fl_Button* o, void* v) {
  ((GUI*)(o->parent()->parent()->parent()->parent()->user_data()))->cb_Convert_i(o,v);
}


void GUI::cb_FixSTLerrorsButton_i(Fl_Light_Button*, void*) {
  MVC->redraw();
}
void GUI::cb_FixSTLerrorsButton(Fl_Light_Button* o, void* v) {
  ((GUI*)(o->parent()->parent()->parent()->user_data()))->cb_FixSTLerrorsButton_i(o,v);
}

void GUI::cb_Save1_i(Fl_Button*, void*) {
  FileChooser::ioDialog (MVC, FileChooser::SAVE, FileChooser::STL);
}
void GUI::cb_Save1(Fl_Button* o, void* v) {
  ((GUI*)(o->parent()->parent()->parent()->user_data()))->cb_Save1_i(o,v);
}

void GUI::cb_AutoRotateButton_i(Fl_Button*, void*) {
  MVC->OptimizeRotation();
MVC->CalcBoundingBoxAndCenter();
}
void GUI::cb_AutoRotateButton(Fl_Button* o, void* v) {
  ((GUI*)(o->parent()->parent()->parent()->parent()->user_data()))->cb_AutoRotateButton_i(o,v);
}

void GUI::cb_RFP_Browser_i(Flu_Tree_Browser*, void*) {
  MVC->redraw();
}
void GUI::cb_RFP_Browser(Flu_Tree_Browser* o, void* v) {
  ((GUI*)(o->parent()->parent()->parent()->user_data()))->cb_RFP_Browser_i(o,v);
}

void GUI::cb_TranslateX_i(Fl_Value_Input* o, void*) {
  MVC->Translate("X", o->value());
}
void GUI::cb_TranslateX(Fl_Value_Input* o, void* v) {
  ((GUI*)(o->parent()->parent()->parent()->parent()->user_data()))->cb_TranslateX_i(o,v);
}

void GUI::cb_TranslateY_i(Fl_Value_Input* o, void*) {
  MVC->Translate("Y", o->value());
}
void GUI::cb_TranslateY(Fl_Value_Input* o, void* v) {
  ((GUI*)(o->parent()->parent()->parent()->parent()->user_data()))->cb_TranslateY_i(o,v);
}

void GUI::cb_TranslateZ_i(Fl_Value_Input* o, void*) {
  MVC->Translate("Z", o->value());
}
void GUI::cb_TranslateZ(Fl_Value_Input* o, void* v) {
  ((GUI*)(o->parent()->parent()->parent()->parent()->user_data()))->cb_TranslateZ_i(o,v);
}

void GUI::cb_RotateX_i(Fl_Value_Input* o, void*) {
}
void GUI::cb_RotateX(Fl_Value_Input* o, void* v) {
  ((GUI*)(o->parent()->parent()->parent()->parent()->user_data()))->cb_RotateX_i(o,v);
}

void GUI::cb_RotateY_i(Fl_Value_Input* o, void*) {
}
void GUI::cb_RotateY(Fl_Value_Input* o, void* v) {
  ((GUI*)(o->parent()->parent()->parent()->parent()->user_data()))->cb_RotateY_i(o,v);
}

void GUI::cb_RotateZ_i(Fl_Value_Input* o, void*) {
}
void GUI::cb_RotateZ(Fl_Value_Input* o, void* v) {
  ((GUI*)(o->parent()->parent()->parent()->parent()->user_data()))->cb_RotateZ_i(o,v);
}

void GUI::cb_ScaleX_i(Fl_Value_Input* o, void*) {
  MVC->Scale("X", o->value());
}
void GUI::cb_ScaleX(Fl_Value_Input* o, void* v) {
  ((GUI*)(o->parent()->parent()->parent()->parent()->user_data()))->cb_ScaleX_i(o,v);
}

void GUI::cb_ScaleY_i(Fl_Value_Input* o, void*) {
  MVC->Scale("Y", o->value());
}
void GUI::cb_ScaleY(Fl_Value_Input* o, void* v) {
  ((GUI*)(o->parent()->parent()->parent()->parent()->user_data()))->cb_ScaleY_i(o,v);
}

void GUI::cb_ScaleZ_i(Fl_Value_Input* o, void*) {
  MVC->Scale("Z", o->value());
}
void GUI::cb_ScaleZ(Fl_Value_Input* o, void* v) {
  ((GUI*)(o->parent()->parent()->parent()->parent()->user_data()))->cb_ScaleZ_i(o,v);
}

void GUI::cb_FileLocationInput_i(Fl_Input* o, void*) {
  MVC->setFileLocation(o->value());
}
void GUI::cb_FileLocationInput(Fl_Input* o, void* v) {
  ((GUI*)(o->parent()->parent()->parent()->user_data()))->cb_FileLocationInput_i(o,v);
}

void GUI::cb_FileTypeInput_i(Fl_Input* o, void*) {
  MVC->setFileType(o->value());
}
void GUI::cb_FileTypeInput(Fl_Input* o, void* v) {
  ((GUI*)(o->parent()->parent()->parent()->user_data()))->cb_FileTypeInput_i(o,v);
}

void GUI::cb_FileMaterialInput_i(Fl_Input* o, void*) {
  MVC->setFileMaterial(o->value());
}
void GUI::cb_FileMaterialInput(Fl_Input* o, void* v) {
  ((GUI*)(o->parent()->parent()->parent()->user_data()))->cb_FileMaterialInput_i(o,v);
}

void GUI::cb_ObjectNameInput_i(Fl_Input* o, void*) {
  MVC->setObjectname(o->value());
}
void GUI::cb_ObjectNameInput(Fl_Input* o, void* v) {
  ((GUI*)(o->parent()->parent()->parent()->user_data()))->cb_ObjectNameInput_i(o,v);
}

void GUI::cb_Delete_i(Fl_Button*, void*) {
  MVC->ProcessControl.rfo.DeleteSelected(MVC);
}
void GUI::cb_Delete(Fl_Button* o, void* v) {
  ((GUI*)(o->parent()->parent()->parent()->user_data()))->cb_Delete_i(o,v);
}

void GUI::cb_Duplicate_i(Fl_Button*, void*) {
  MVC->Duplicate();
}
void GUI::cb_Duplicate(Fl_Button* o, void* v) {
  ((GUI*)(o->parent()->parent()->parent()->user_data()))->cb_Duplicate_i(o,v);
}

void GUI::cb_Load3_i(Fl_Button*, void*) {
  FileChooser::ioDialog (MVC, FileChooser::OPEN, FileChooser::GCODE);
}
void GUI::cb_Load3(Fl_Button* o, void* v) {
  ((GUI*)(o->parent()->parent()->parent()->user_data()))->cb_Load3_i(o,v);
}

void GUI::cb_Convert1_i(Fl_Button*, void*) {
  MVC->ConvertToGCode();
}
void GUI::cb_Convert1(Fl_Button* o, void* v) {
  ((GUI*)(o->parent()->parent()->parent()->user_data()))->cb_Convert1_i(o,v);
}

void GUI::cb_GCodeInput_i(Fl_Input* o, void*) {
  MVC->SendNow(o->value());
}
void GUI::cb_GCodeInput(Fl_Input* o, void* v) {
  ((GUI*)(o->parent()->parent()->parent()->user_data()))->cb_GCodeInput_i(o,v);
}

void GUI::cb_Home_i(Fl_Button*, void*) {
  MVC->Home("X");
}
void GUI::cb_Home(Fl_Button* o, void* v) {
  ((GUI*)(o->parent()->parent()->parent()->parent()->user_data()))->cb_Home_i(o,v);
}

void GUI::cb_Home1_i(Fl_Button*, void*) {
  MVC->Home("Y");
}
void GUI::cb_Home1(Fl_Button* o, void* v) {
  ((GUI*)(o->parent()->parent()->parent()->parent()->user_data()))->cb_Home1_i(o,v);
}

void GUI::cb_Home2_i(Fl_Button*, void*) {
  MVC->Home("Z");
}
void GUI::cb_Home2(Fl_Button* o, void* v) {
  ((GUI*)(o->parent()->parent()->parent()->parent()->user_data()))->cb_Home2_i(o,v);
}

void GUI::cb_Home3_i(Fl_Button*, void*) {
  MVC->Home("ALL");
}
void GUI::cb_Home3(Fl_Button* o, void* v) {
  ((GUI*)(o->parent()->parent()->parent()->parent()->user_data()))->cb_Home3_i(o,v);
}

void GUI::cb_10_i(Fl_Button*, void*) {
  MVC->Move("X", -10);
}
void GUI::cb_10(Fl_Button* o, void* v) {
  ((GUI*)(o->parent()->parent()->parent()->parent()->user_data()))->cb_10_i(o,v);
}

void GUI::cb_1_i(Fl_Button*, void*) {
  MVC->Move("X", -1);
}
void GUI::cb_1(Fl_Button* o, void* v) {
  ((GUI*)(o->parent()->parent()->parent()->parent()->user_data()))->cb_1_i(o,v);
}

void GUI::cb_11_i(Fl_Button*, void*) {
  MVC->Move("X", -0.1f);
}
void GUI::cb_11(Fl_Button* o, void* v) {
  ((GUI*)(o->parent()->parent()->parent()->parent()->user_data()))->cb_11_i(o,v);
}

void GUI::cb_12_i(Fl_Button*, void*) {
  MVC->Move("X", 0.1f);
}
void GUI::cb_12(Fl_Button* o, void* v) {
  ((GUI*)(o->parent()->parent()->parent()->parent()->user_data()))->cb_12_i(o,v);
}

void GUI::cb_13_i(Fl_Button*, void*) {
  MVC->Move("X", 1);
}
void GUI::cb_13(Fl_Button* o, void* v) {
  ((GUI*)(o->parent()->parent()->parent()->parent()->user_data()))->cb_13_i(o,v);
}

void GUI::cb_101_i(Fl_Button*, void*) {
  MVC->Move("X", 10);
}
void GUI::cb_101(Fl_Button* o, void* v) {
  ((GUI*)(o->parent()->parent()->parent()->parent()->user_data()))->cb_101_i(o,v);
}

void GUI::cb_100_i(Fl_Button*, void*) {
  MVC->Move("X", -100);
}
void GUI::cb_100(Fl_Button* o, void* v) {
  ((GUI*)(o->parent()->parent()->parent()->parent()->user_data()))->cb_100_i(o,v);
}

void GUI::cb_1001_i(Fl_Button*, void*) {
  MVC->Move("X", 100);
}
void GUI::cb_1001(Fl_Button* o, void* v) {
  ((GUI*)(o->parent()->parent()->parent()->parent()->user_data()))->cb_1001_i(o,v);
}

void GUI::cb_102_i(Fl_Button*, void*) {
  MVC->Move("Y", -10);
}
void GUI::cb_102(Fl_Button* o, void* v) {
  ((GUI*)(o->parent()->parent()->parent()->parent()->user_data()))->cb_102_i(o,v);
}

void GUI::cb_14_i(Fl_Button*, void*) {
  MVC->Move("Y", -1);
}
void GUI::cb_14(Fl_Button* o, void* v) {
  ((GUI*)(o->parent()->parent()->parent()->parent()->user_data()))->cb_14_i(o,v);
}

void GUI::cb_15_i(Fl_Button*, void*) {
  MVC->Move("Y", -0.1f);
}
void GUI::cb_15(Fl_Button* o, void* v) {
  ((GUI*)(o->parent()->parent()->parent()->parent()->user_data()))->cb_15_i(o,v);
}

void GUI::cb_16_i(Fl_Button*, void*) {
  MVC->Move("Y", 0.1f);
}
void GUI::cb_16(Fl_Button* o, void* v) {
  ((GUI*)(o->parent()->parent()->parent()->parent()->user_data()))->cb_16_i(o,v);
}

void GUI::cb_17_i(Fl_Button*, void*) {
  MVC->Move("Y", 1);
}
void GUI::cb_17(Fl_Button* o, void* v) {
  ((GUI*)(o->parent()->parent()->parent()->parent()->user_data()))->cb_17_i(o,v);
}

void GUI::cb_103_i(Fl_Button*, void*) {
  MVC->Move("Y", 10);
}
void GUI::cb_103(Fl_Button* o, void* v) {
  ((GUI*)(o->parent()->parent()->parent()->parent()->user_data()))->cb_103_i(o,v);
}

void GUI::cb_1002_i(Fl_Button*, void*) {
  MVC->Move("Y", -100);
}
void GUI::cb_1002(Fl_Button* o, void* v) {
  ((GUI*)(o->parent()->parent()->parent()->parent()->user_data()))->cb_1002_i(o,v);
}

void GUI::cb_1003_i(Fl_Button*, void*) {
  MVC->Move("Y", 100);
}
void GUI::cb_1003(Fl_Button* o, void* v) {
  ((GUI*)(o->parent()->parent()->parent()->parent()->user_data()))->cb_1003_i(o,v);
}

void GUI::cb_104_i(Fl_Button*, void*) {
  MVC->Move("Z", -10);
}
void GUI::cb_104(Fl_Button* o, void* v) {
  ((GUI*)(o->parent()->parent()->parent()->parent()->user_data()))->cb_104_i(o,v);
}

void GUI::cb_18_i(Fl_Button*, void*) {
  MVC->Move("Z", -1);
}
void GUI::cb_18(Fl_Button* o, void* v) {
  ((GUI*)(o->parent()->parent()->parent()->parent()->user_data()))->cb_18_i(o,v);
}

void GUI::cb_19_i(Fl_Button*, void*) {
  MVC->Move("Z", -0.1f);
}
void GUI::cb_19(Fl_Button* o, void* v) {
  ((GUI*)(o->parent()->parent()->parent()->parent()->user_data()))->cb_19_i(o,v);
}

void GUI::cb_1a_i(Fl_Button*, void*) {
  MVC->Move("Z", 0.1f);
}
void GUI::cb_1a(Fl_Button* o, void* v) {
  ((GUI*)(o->parent()->parent()->parent()->parent()->user_data()))->cb_1a_i(o,v);
}

void GUI::cb_1b_i(Fl_Button*, void*) {
  MVC->Move("Z", 1);
}
void GUI::cb_1b(Fl_Button* o, void* v) {
  ((GUI*)(o->parent()->parent()->parent()->parent()->user_data()))->cb_1b_i(o,v);
}

void GUI::cb_105_i(Fl_Button*, void*) {
  MVC->Move("Z", 10);
}
void GUI::cb_105(Fl_Button* o, void* v) {
  ((GUI*)(o->parent()->parent()->parent()->parent()->user_data()))->cb_105_i(o,v);
}

void GUI::cb_1004_i(Fl_Button*, void*) {
  MVC->Move("Z", -100);
}
void GUI::cb_1004(Fl_Button* o, void* v) {
  ((GUI*)(o->parent()->parent()->parent()->parent()->user_data()))->cb_1004_i(o,v);
}

void GUI::cb_1005_i(Fl_Button*, void*) {
  MVC->Move("Z", 100);
}
void GUI::cb_1005(Fl_Button* o, void* v) {
  ((GUI*)(o->parent()->parent()->parent()->parent()->user_data()))->cb_1005_i(o,v);
}

void GUI::cb_XposText_i(Fl_Value_Input* o, void*) {
  MVC->Goto("X", o->value());
}
void GUI::cb_XposText(Fl_Value_Input* o, void* v) {
  ((GUI*)(o->parent()->parent()->parent()->parent()->user_data()))->cb_XposText_i(o,v);
}

void GUI::cb_YposText_i(Fl_Value_Input* o, void*) {
  MVC->Goto("Y", o->value());
}
void GUI::cb_YposText(Fl_Value_Input* o, void* v) {
  ((GUI*)(o->parent()->parent()->parent()->parent()->user_data()))->cb_YposText_i(o,v);
}

void GUI::cb_ZposText_i(Fl_Value_Input* o, void*) {
  MVC->Goto("Z", o->value());
}
void GUI::cb_ZposText(Fl_Value_Input* o, void* v) {
  ((GUI*)(o->parent()->parent()->parent()->parent()->user_data()))->cb_ZposText_i(o,v);
}

void GUI::cb_CustomButton1_i(Fl_Button*, void*) {
  MVC->SendCustomButton(1);
}
void GUI::cb_CustomButton1(Fl_Button* o, void* v) {
  ((GUI*)(o->parent()->parent()->parent()->parent()->user_data()))->cb_CustomButton1_i(o,v);
}

void GUI::cb_CustomButton3_i(Fl_Button*, void*) {
  MVC->SendCustomButton(3);
}
void GUI::cb_CustomButton3(Fl_Button* o, void* v) {
  ((GUI*)(o->parent()->parent()->parent()->parent()->user_data()))->cb_CustomButton3_i(o,v);
}

void GUI::cb_CustomButton2_i(Fl_Button*, void*) {
  MVC->SendCustomButton(2);
}
void GUI::cb_CustomButton2(Fl_Button* o, void* v) {
  ((GUI*)(o->parent()->parent()->parent()->parent()->user_data()))->cb_CustomButton2_i(o,v);
}

void GUI::cb_CustomButton4_i(Fl_Button*, void*) {
  MVC->SendCustomButton(4);
}
void GUI::cb_CustomButton4(Fl_Button* o, void* v) {
  ((GUI*)(o->parent()->parent()->parent()->parent()->user_data()))->cb_CustomButton4_i(o,v);
}

void GUI::cb_CustomButton5_i(Fl_Button*, void*) {
  MVC->SendCustomButton(5);
}
void GUI::cb_CustomButton5(Fl_Button* o, void* v) {
  ((GUI*)(o->parent()->parent()->parent()->parent()->user_data()))->cb_CustomButton5_i(o,v);
}

void GUI::cb_CustomButton7_i(Fl_Button*, void*) {
  MVC->SendCustomButton(7);
}
void GUI::cb_CustomButton7(Fl_Button* o, void* v) {
  ((GUI*)(o->parent()->parent()->parent()->parent()->user_data()))->cb_CustomButton7_i(o,v);
}

void GUI::cb_CustomButton6_i(Fl_Button*, void*) {
  MVC->SendCustomButton(6);
}
void GUI::cb_CustomButton6(Fl_Button* o, void* v) {
  ((GUI*)(o->parent()->parent()->parent()->parent()->user_data()))->cb_CustomButton6_i(o,v);
}

void GUI::cb_CustomButton8_i(Fl_Button*, void*) {
  MVC->SendCustomButton(8);
}
void GUI::cb_CustomButton8(Fl_Button* o, void* v) {
  ((GUI*)(o->parent()->parent()->parent()->parent()->user_data()))->cb_CustomButton8_i(o,v);
}

void GUI::cb_CustomButton9_i(Fl_Button*, void*) {
  MVC->SendCustomButton(9);
}
void GUI::cb_CustomButton9(Fl_Button* o, void* v) {
  ((GUI*)(o->parent()->parent()->parent()->parent()->user_data()))->cb_CustomButton9_i(o,v);
}

void GUI::cb_CustomButton10_i(Fl_Button*, void*) {
  MVC->SendCustomButton(10);
}
void GUI::cb_CustomButton10(Fl_Button* o, void* v) {
  ((GUI*)(o->parent()->parent()->parent()->parent()->user_data()))->cb_CustomButton10_i(o,v);
}

void GUI::cb_CustomButton11_i(Fl_Button*, void*) {
  MVC->SendCustomButton(11);
}
void GUI::cb_CustomButton11(Fl_Button* o, void* v) {
  ((GUI*)(o->parent()->parent()->parent()->parent()->user_data()))->cb_CustomButton11_i(o,v);
}

void GUI::cb_CustomButton13_i(Fl_Button*, void*) {
  MVC->SendCustomButton(13);
}
void GUI::cb_CustomButton13(Fl_Button* o, void* v) {
  ((GUI*)(o->parent()->parent()->parent()->parent()->user_data()))->cb_CustomButton13_i(o,v);
}

void GUI::cb_CustomButton12_i(Fl_Button*, void*) {
  MVC->SendCustomButton(12);
}
void GUI::cb_CustomButton12(Fl_Button* o, void* v) {
  ((GUI*)(o->parent()->parent()->parent()->parent()->user_data()))->cb_CustomButton12_i(o,v);
}

void GUI::cb_CustomButton14_i(Fl_Button*, void*) {
  MVC->SendCustomButton(14);
}
void GUI::cb_CustomButton14(Fl_Button* o, void* v) {
  ((GUI*)(o->parent()->parent()->parent()->parent()->user_data()))->cb_CustomButton14_i(o,v);
}

void GUI::cb_CustomButton15_i(Fl_Button*, void*) {
  MVC->SendCustomButton(15);
}
void GUI::cb_CustomButton15(Fl_Button* o, void* v) {
  ((GUI*)(o->parent()->parent()->parent()->parent()->user_data()))->cb_CustomButton15_i(o,v);
}

void GUI::cb_CustomButton17_i(Fl_Button*, void*) {
  MVC->SendCustomButton(17);
}
void GUI::cb_CustomButton17(Fl_Button* o, void* v) {
  ((GUI*)(o->parent()->parent()->parent()->parent()->user_data()))->cb_CustomButton17_i(o,v);
}

void GUI::cb_CustomButton16_i(Fl_Button*, void*) {
  MVC->SendCustomButton(16);
}
void GUI::cb_CustomButton16(Fl_Button* o, void* v) {
  ((GUI*)(o->parent()->parent()->parent()->parent()->user_data()))->cb_CustomButton16_i(o,v);
}

void GUI::cb_CustomButton18_i(Fl_Button*, void*) {
  MVC->SendCustomButton(18);
}
void GUI::cb_CustomButton18(Fl_Button* o, void* v) {
  ((GUI*)(o->parent()->parent()->parent()->parent()->user_data()))->cb_CustomButton18_i(o,v);
}

void GUI::cb_CustomButton19_i(Fl_Button*, void*) {
  MVC->SendCustomButton(19);
}
void GUI::cb_CustomButton19(Fl_Button* o, void* v) {
  ((GUI*)(o->parent()->parent()->parent()->parent()->user_data()))->cb_CustomButton19_i(o,v);
}

void GUI::cb_CustomButton20_i(Fl_Button*, void*) {
  MVC->SendCustomButton(20);
}
void GUI::cb_CustomButton20(Fl_Button* o, void* v) {
  ((GUI*)(o->parent()->parent()->parent()->parent()->user_data()))->cb_CustomButton20_i(o,v);
}

void GUI::cb_Errors_i(Fl_Light_Button* o, void*) {
  MVC->serial->SetDebugMask(DEBUG_ERRORS, (bool)o->value());
}
void GUI::cb_Errors(Fl_Light_Button* o, void* v) {
  ((GUI*)(o->parent()->parent()->parent()->parent()->parent()->user_data()))->cb_Errors_i(o,v);
}

void GUI::cb_Info_i(Fl_Light_Button* o, void*) {
  MVC->serial->SetDebugMask(DEBUG_INFO, (bool)o->value());
}
void GUI::cb_Info(Fl_Light_Button* o, void* v) {
  ((GUI*)(o->parent()->parent()->parent()->parent()->parent()->user_data()))->cb_Info_i(o,v);
}

void GUI::cb_Echo_i(Fl_Light_Button* o, void*) {
  MVC->serial->SetDebugMask(DEBUG_ECHO, (bool)o->value());
}
void GUI::cb_Echo(Fl_Light_Button* o, void* v) {
  ((GUI*)(o->parent()->parent()->parent()->parent()->parent()->user_data()))->cb_Echo_i(o,v);
}

void GUI::cb_FanOnButton_i(Fl_Light_Button* o, void*) {
  MVC->SetFan(o->value() ? FanPowerSlider->value() : 0);
}
void GUI::cb_FanOnButton(Fl_Light_Button* o, void* v) {
  ((GUI*)(o->parent()->parent()->parent()->parent()->parent()->user_data()))->cb_FanOnButton_i(o,v);
}

void GUI::cb_FanPowerSlider_i(Fl_Value_Slider* o, void*) {
  FanOnButton->value(true);
MVC->SetFan(o->value()*(255.0f/12.0f));
}
void GUI::cb_FanPowerSlider(Fl_Value_Slider* o, void* v) {
  ((GUI*)(o->parent()->parent()->parent()->parent()->parent()->user_data()))->cb_FanPowerSlider_i(o,v);
}

void GUI::cb_RunExtruderButton_i(Fl_Light_Button*, void*) {
  MVC->RunExtruder();
}
void GUI::cb_RunExtruderButton(Fl_Light_Button* o, void* v) {
  ((GUI*)(o->parent()->parent()->parent()->parent()->parent()->user_data()))->cb_RunExtruderButton_i(o,v);
}

void GUI::cb_SetExtruderDirectionButton_i(Fl_Light_Button* o, void*) {
  MVC->SetExtruderDirection(o->value());
}
void GUI::cb_SetExtruderDirectionButton(Fl_Light_Button* o, void* v) {
  ((GUI*)(o->parent()->parent()->parent()->parent()->parent()->user_data()))->cb_SetExtruderDirectionButton_i(o,v);
}

void GUI::cb_Speed_i(Fl_Value_Slider* o, void*) {
  MVC->SetExtruderSpeed(o->value());
}
void GUI::cb_Speed(Fl_Value_Slider* o, void* v) {
  ((GUI*)(o->parent()->parent()->parent()->parent()->parent()->user_data()))->cb_Speed_i(o,v);
}

void GUI::cb_Length_i(Fl_Value_Slider* o, void*) {
  MVC->SetExtruderLength(o->value());
}
void GUI::cb_Length(Fl_Value_Slider* o, void* v) {
  ((GUI*)(o->parent()->parent()->parent()->parent()->parent()->user_data()))->cb_Length_i(o,v);
}

void GUI::cb_TempUpdateSpeedSlider_i(Fl_Value_Slider* o, void*) {
  MVC->SetTempUpdateSpeed(o->value());
}
void GUI::cb_TempUpdateSpeedSlider(Fl_Value_Slider* o, void* v) {
  ((GUI*)(o->parent()->parent()->parent()->parent()->parent()->user_data()))->cb_TempUpdateSpeedSlider_i(o,v);
}

void GUI::cb_TempReadingEnabledButton_i(Fl_Light_Button* o, void*) {
  MVC->EnableTempReading(o->value());
}
void GUI::cb_TempReadingEnabledButton(Fl_Light_Button* o, void* v) {
  ((GUI*)(o->parent()->parent()->parent()->parent()->parent()->user_data()))->cb_TempReadingEnabledButton_i(o,v);
}

void GUI::cb_SwitchHeatOnButton_i(Fl_Light_Button* o, void*) {
  MVC->SwitchHeat(o->value(), TargetTempText->value());
}
void GUI::cb_SwitchHeatOnButton(Fl_Light_Button* o, void* v) {
  ((GUI*)(o->parent()->parent()->parent()->parent()->parent()->parent()->user_data()))->cb_SwitchHeatOnButton_i(o,v);
}

void GUI::cb_TargetTempText_i(Fl_Value_Input* o, void*) {
  MVC->SetTargetTemp(o->value());
}
void GUI::cb_TargetTempText(Fl_Value_Input* o, void* v) {
  ((GUI*)(o->parent()->parent()->parent()->parent()->parent()->parent()->user_data()))->cb_TargetTempText_i(o,v);
}

void GUI::cb_BedHeatOnButton_i(Fl_Light_Button* o, void*) {
  MVC->SwitchBedHeat(o->value(), TargetBedTempText->value());
}
void GUI::cb_BedHeatOnButton(Fl_Light_Button* o, void* v) {
  ((GUI*)(o->parent()->parent()->parent()->parent()->parent()->parent()->user_data()))->cb_BedHeatOnButton_i(o,v);
}

void GUI::cb_TargetBedTempText_i(Fl_Value_Input* o, void*) {
  MVC->SetBedTargetTemp(o->value());
}
void GUI::cb_TargetBedTempText(Fl_Value_Input* o, void* v) {
  ((GUI*)(o->parent()->parent()->parent()->parent()->parent()->parent()->user_data()))->cb_TargetBedTempText_i(o,v);
}

void GUI::cb_LinesToKeepSlider_i(Fl_Value_Slider* o, void*) {
  MVC->SetKeepLines(o->value());
}
void GUI::cb_LinesToKeepSlider(Fl_Value_Slider* o, void* v) {
  ((GUI*)(o->parent()->parent()->parent()->parent()->user_data()))->cb_LinesToKeepSlider_i(o,v);
}

void GUI::cb_FileLogginEnabledButton_i(Fl_Light_Button* o, void*) {
  MVC->SetFileLogging(o->value());
}
void GUI::cb_FileLogginEnabledButton(Fl_Light_Button* o, void* v) {
  ((GUI*)(o->parent()->parent()->parent()->parent()->user_data()))->cb_FileLogginEnabledButton_i(o,v);
}

void GUI::cb_ClearLogfilesWhenPrintStartsButton_i(Fl_Light_Button* o, void*) {
  MVC->SetLogFileClear(o->value());
}
void GUI::cb_ClearLogfilesWhenPrintStartsButton(Fl_Light_Button* o, void* v) {
  ((GUI*)(o->parent()->parent()->parent()->parent()->user_data()))->cb_ClearLogfilesWhenPrintStartsButton_i(o,v);
}

void GUI::cb_Clear_i(Fl_Button*, void*) {
  MVC->ClearLogs();
}
void GUI::cb_Clear(Fl_Button* o, void* v) {
  ((GUI*)(o->parent()->parent()->parent()->parent()->user_data()))->cb_Clear_i(o,v);
}

void GUI::cb_ConnectToPrinterButton_i(Fl_Light_Button* o, void*) {
MVC->ConnectToPrinter(o->value());
}
void GUI::cb_ConnectToPrinterButton(Fl_Light_Button* o, void* v) {
  ((GUI*)(o->parent()->parent()->parent()->parent()->user_data()))->cb_ConnectToPrinterButton_i(o,v);
}

void GUI::cb_PrintButton_i(Fl_Light_Button*, void*) {
  MVC->PrintButton();
}
void GUI::cb_PrintButton(Fl_Light_Button* o, void* v) {
  ((GUI*)(o->parent()->parent()->parent()->parent()->user_data()))->cb_PrintButton_i(o,v);
}

void GUI::cb_Power_i(Fl_Light_Button* o, void*) {
  MVC->SwitchPower(o->value());
}
void GUI::cb_Power(Fl_Light_Button* o, void* v) {
  ((GUI*)(o->parent()->parent()->parent()->parent()->user_data()))->cb_Power_i(o,v);
}

void GUI::cb_KickButton_i(Fl_Button*, void*) {
  MVC->Continue();
}
void GUI::cb_KickButton(Fl_Button* o, void* v) {
  ((GUI*)(o->parent()->parent()->parent()->parent()->user_data()))->cb_KickButton_i(o,v);
}

void GUI::cb_ContinueButton_i(Fl_Light_Button*, void*) {
  MVC->ContinuePauseButton();
}
void GUI::cb_ContinueButton(Fl_Light_Button* o, void* v) {
  ((GUI*)(o->parent()->parent()->parent()->parent()->user_data()))->cb_ContinueButton_i(o,v);
}

void GUI::cb_RunLuaButton_i(Fl_Button*, void*) {
  MVC->RunLua(LuaScriptEditor->buffer()->text());
}
void GUI::cb_RunLuaButton(Fl_Button* o, void* v) {
  ((GUI*)(o->parent()->parent()->parent()->parent()->user_data()))->cb_RunLuaButton_i(o,v);
}

void GUI::cb_MarginX_i(Fl_Value_Input* o, void*) {
  MVC->SetPrintMargin("X", o->value());
}
void GUI::cb_MarginX(Fl_Value_Input* o, void* v) {
  ((GUI*)(o->parent()->parent()->user_data()))->cb_MarginX_i(o,v);
}

void GUI::cb_MarginY_i(Fl_Value_Input* o, void*) {
  MVC->SetPrintMargin("Y", o->value());
}
void GUI::cb_MarginY(Fl_Value_Input* o, void* v) {
  ((GUI*)(o->parent()->parent()->user_data()))->cb_MarginY_i(o,v);
}

void GUI::cb_MarginZ_i(Fl_Value_Input* o, void*) {
  MVC->SetPrintMargin("Z", o->value());
}
void GUI::cb_MarginZ(Fl_Value_Input* o, void* v) {
  ((GUI*)(o->parent()->parent()->user_data()))->cb_MarginZ_i(o,v);
}

void GUI::cb_distanceToReachFullSpeedSlider_i(Fl_Value_Slider* o, void*) {
  MVC->SetDistanceToReachFullSpeed(o->value());
}
void GUI::cb_distanceToReachFullSpeedSlider(Fl_Value_Slider* o, void* v) {
  ((GUI*)(o->parent()->parent()->user_data()))->cb_distanceToReachFullSpeedSlider_i(o,v);
}

void GUI::cb_EnableAccelerationButton_i(Fl_Light_Button* o, void*) {
  MVC->SetEnableAcceleration(o->value());
if(o->value())
{
distanceToReachFullSpeedSlider->activate();
}
else
{
distanceToReachFullSpeedSlider->deactivate();
};
}
void GUI::cb_EnableAccelerationButton(Fl_Light_Button* o, void* v) {
  ((GUI*)(o->parent()->parent()->user_data()))->cb_EnableAccelerationButton_i(o,v);
}

void GUI::cb_MaxPrintSpeedXYSlider_i(Fl_Value_Slider* o, void*) {
  MVC->SetMaxPrintSpeedXY(o->value());
}
void GUI::cb_MaxPrintSpeedXYSlider(Fl_Value_Slider* o, void* v) {
  ((GUI*)(o->parent()->parent()->user_data()))->cb_MaxPrintSpeedXYSlider_i(o,v);
}

void GUI::cb_MinPrintSpeedZSlider_i(Fl_Value_Slider* o, void*) {
  MVC->SetMinPrintSpeedZ(o->value());
}
void GUI::cb_MinPrintSpeedZSlider(Fl_Value_Slider* o, void* v) {
  ((GUI*)(o->parent()->parent()->user_data()))->cb_MinPrintSpeedZSlider_i(o,v);
}

void GUI::cb_MinPrintSpeedXYSlider_i(Fl_Value_Slider* o, void*) {
  MVC->SetMinPrintSpeedXY(o->value());
}
void GUI::cb_MinPrintSpeedXYSlider(Fl_Value_Slider* o, void* v) {
  ((GUI*)(o->parent()->parent()->user_data()))->cb_MinPrintSpeedXYSlider_i(o,v);
}

void GUI::cb_MaxPrintSpeedZSlider_i(Fl_Value_Slider* o, void*) {
  MVC->SetMaxPrintSpeedZ(o->value());
}
void GUI::cb_MaxPrintSpeedZSlider(Fl_Value_Slider* o, void* v) {
  ((GUI*)(o->parent()->parent()->user_data()))->cb_MaxPrintSpeedZSlider_i(o,v);
}

void GUI::cb_ExtrudedMaterialWidthSlider_i(Fl_Value_Slider* o, void*) {
  MVC->SetExtrudedMaterialWidth(o->value());
}
void GUI::cb_ExtrudedMaterialWidthSlider(Fl_Value_Slider* o, void* v) {
  ((GUI*)(o->parent()->parent()->user_data()))->cb_ExtrudedMaterialWidthSlider_i(o,v);
}

void GUI::cb_extrusionFactorSlider_i(Fl_Value_Slider* o, void*) {
  MVC->SetExtrusionFactor(o->value());
}
void GUI::cb_extrusionFactorSlider(Fl_Value_Slider* o, void* v) {
  ((GUI*)(o->parent()->parent()->user_data()))->cb_extrusionFactorSlider_i(o,v);
}

void GUI::cb_UseIncrementalEcodeButton_i(Fl_Light_Button* o, void*) {
  MVC->SetUseIncrementalEcode(o->value());
}
void GUI::cb_UseIncrementalEcodeButton(Fl_Light_Button* o, void* v) {
  ((GUI*)(o->parent()->parent()->user_data()))->cb_UseIncrementalEcodeButton_i(o,v);
}

void GUI::cb_LayerThicknessSlider_i(Fl_Value_Slider* o, void*) {
  MVC->SetLayerThickness(o->value());
}
void GUI::cb_LayerThicknessSlider(Fl_Value_Slider* o, void* v) {
  ((GUI*)(o->parent()->parent()->user_data()))->cb_LayerThicknessSlider_i(o,v);
}

void GUI::cb_Use3DGcodeButton_i(Fl_Light_Button* o, void*) {
  MVC->SetUse3DGcode(o->value());
}
void GUI::cb_Use3DGcodeButton(Fl_Light_Button* o, void* v) {
  ((GUI*)(o->parent()->parent()->user_data()))->cb_Use3DGcodeButton_i(o,v);
}

void GUI::cb_antioozeDistanceSlider_i(Fl_Value_Slider* o, void*) {
  MVC->SetAntioozeDistance(o->value());
}
void GUI::cb_antioozeDistanceSlider(Fl_Value_Slider* o, void* v) {
  ((GUI*)(o->parent()->parent()->user_data()))->cb_antioozeDistanceSlider_i(o,v);
}

void GUI::cb_EnableAntioozeButton_i(Fl_Light_Button* o, void*) {
  MVC->SetEnableAntiooze(o->value());
if(o->value())
{
antioozeDistanceSlider->activate();
antioozeSpeedSlider->activate();
}
else
{
antioozeDistanceSlider->deactivate();
antioozeSpeedSlider->deactivate();
};
}
void GUI::cb_EnableAntioozeButton(Fl_Light_Button* o, void* v) {
  ((GUI*)(o->parent()->parent()->user_data()))->cb_EnableAntioozeButton_i(o,v);
}

void GUI::cb_antioozeSpeedSlider_i(Fl_Value_Slider* o, void*) {
  MVC->SetAntioozeSpeed(o->value());
}
void GUI::cb_antioozeSpeedSlider(Fl_Value_Slider* o, void* v) {
  ((GUI*)(o->parent()->parent()->user_data()))->cb_antioozeSpeedSlider_i(o,v);
}

void GUI::cb_SerialSpeedInput_i(Fl_Value_Input* o, void*) {
  MVC->setSerialSpeed(o->value());
}
void GUI::cb_SerialSpeedInput(Fl_Value_Input* o, void* v) {
  ((GUI*)(o->parent()->parent()->user_data()))->cb_SerialSpeedInput_i(o,v);
}

void GUI::cb_portInput_i(Fl_Input_Choice* o, void*) {
  MVC->setPort(std::string(o->input()->value()));
}
void GUI::cb_portInput(Fl_Input_Choice* o, void* v) {
  ((GUI*)(o->parent()->parent()->user_data()))->cb_portInput_i(o,v);
}

Fl_Menu_Item GUI::menu_portInput[] = {
 {"COM1", 0,  0, (void*)(1), 4, FL_NORMAL_LABEL, 0, 14, 0},
 {"COM2", 0,  0, (void*)(2), 4, FL_NORMAL_LABEL, 0, 14, 0},
 {"COM3", 0,  0, (void*)(3), 4, FL_NORMAL_LABEL, 0, 14, 0},
 {"COM4", 0,  0, (void*)(4), 4, FL_NORMAL_LABEL, 0, 14, 0},
 {"COM5", 0,  0, (void*)(5), 4, FL_NORMAL_LABEL, 0, 14, 0},
 {"COM6", 0,  0, (void*)(6), 4, FL_NORMAL_LABEL, 0, 14, 0},
 {"COM7", 0,  0, (void*)(7), 4, FL_NORMAL_LABEL, 0, 14, 0},
 {"COM8", 0,  0, (void*)(8), 4, FL_NORMAL_LABEL, 0, 14, 0},
 {"COM9", 0,  0, (void*)(8), 4, FL_NORMAL_LABEL, 0, 14, 0},
 {0,0,0,0,0,0,0,0,0}
};

void GUI::cb_ValidateConnection_i(Fl_Light_Button* o, void*) {
  MVC->SetValidateConnection(o->value());
}
void GUI::cb_ValidateConnection(Fl_Light_Button* o, void* v) {
  ((GUI*)(o->parent()->parent()->user_data()))->cb_ValidateConnection_i(o,v);
}

void GUI::cb_ReceivingBufferSizeSlider_i(Fl_Value_Slider* o, void*) {
  MVC->SetReceivingBufferSize(o->value());
}
void GUI::cb_ReceivingBufferSizeSlider(Fl_Value_Slider* o, void* v) {
  ((GUI*)(o->parent()->parent()->user_data()))->cb_ReceivingBufferSizeSlider_i(o,v);
}

void GUI::cb_Ok_i(Fl_Button*, void*) {
  printerSettingsWindow->hide();
}
void GUI::cb_Ok(Fl_Button* o, void* v) {
  ((GUI*)(o->parent()->user_data()))->cb_Ok_i(o,v);
}

void GUI::cb_InfillRotationSlider_i(Fl_Value_Slider* o, void*) {
  MVC->SetInfillRotation(o->value());
}
void GUI::cb_InfillRotationSlider(Fl_Value_Slider* o, void* v) {
  ((GUI*)(o->parent()->parent()->user_data()))->cb_InfillRotationSlider_i(o,v);
}

void GUI::cb_InfillRotationPrLayerSlider_i(Fl_Value_Slider* o, void*) {
  MVC->SetInfillRotationPrLayer(o->value());
}
void GUI::cb_InfillRotationPrLayerSlider(Fl_Value_Slider* o, void* v) {
  ((GUI*)(o->parent()->parent()->user_data()))->cb_InfillRotationPrLayerSlider_i(o,v);
}

void GUI::cb_InfillDistanceSlider_i(Fl_Value_Slider* o, void*) {
  MVC->SetInfillDistance(o->value());
}
void GUI::cb_InfillDistanceSlider(Fl_Value_Slider* o, void* v) {
  ((GUI*)(o->parent()->parent()->user_data()))->cb_InfillDistanceSlider_i(o,v);
}

void GUI::cb_ShellOnlyButton_i(Fl_Light_Button* o, void*) {
  MVC->SetShellOnly(o->value());
}
void GUI::cb_ShellOnlyButton(Fl_Light_Button* o, void* v) {
  ((GUI*)(o->parent()->parent()->user_data()))->cb_ShellOnlyButton_i(o,v);
}

void GUI::cb_ShellCountSlider_i(Fl_Value_Slider* o, void*) {
  MVC->SetShellCount(o->value());
}
void GUI::cb_ShellCountSlider(Fl_Value_Slider* o, void* v) {
  ((GUI*)(o->parent()->parent()->user_data()))->cb_ShellCountSlider_i(o,v);
}

void GUI::cb_Fast_i(Fl_Menu_*, void*) {
  MVC->SetShrinkQuality("Fast");
MVC->redraw();
}
void GUI::cb_Fast(Fl_Menu_* o, void* v) {
  ((GUI*)(o->parent()->parent()->user_data()))->cb_Fast_i(o,v);
}

void GUI::cb_Logick_i(Fl_Menu_*, void*) {
  MVC->SetShrinkQuality("Logick");
MVC->redraw();
}
void GUI::cb_Logick(Fl_Menu_* o, void* v) {
  ((GUI*)(o->parent()->parent()->user_data()))->cb_Logick_i(o,v);
}

Fl_Menu_Item GUI::menu_shrinkAlgorithm[] = {
 {"Fast, can have errors", 0,  (Fl_Callback*)GUI::cb_Fast, 0, 0, FL_NORMAL_LABEL, 0, 14, 0},
 {"Logick\'s Algorithm", 0,  (Fl_Callback*)GUI::cb_Logick, 0, 0, FL_NORMAL_LABEL, 0, 14, 0},
 {0,0,0,0,0,0,0,0,0}
};

void GUI::cb_OptimizationSlider_i(Fl_Value_Slider* o, void*) {
  MVC->SetOptimization(o->value());
}
void GUI::cb_OptimizationSlider(Fl_Value_Slider* o, void* v) {
  ((GUI*)(o->parent()->parent()->user_data()))->cb_OptimizationSlider_i(o,v);
}

void GUI::cb_AltInfillLayersInput_i(Fl_Input* o, void*) {
  MVC->SetAltInfillLayersText(o->value());
}
void GUI::cb_AltInfillLayersInput(Fl_Input* o, void* v) {
  ((GUI*)(o->parent()->parent()->user_data()))->cb_AltInfillLayersInput_i(o,v);
}

void GUI::cb_AltInfillDistanceSlider_i(Fl_Value_Slider* o, void*) {
  MVC->SetAltInfillDistance(o->value());
}
void GUI::cb_AltInfillDistanceSlider(Fl_Value_Slider* o, void* v) {
  ((GUI*)(o->parent()->parent()->user_data()))->cb_AltInfillDistanceSlider_i(o,v);
}

void GUI::cb_RaftEnableButton_i(Fl_Light_Button* o, void*) {
  MVC->SetRaftEnable(o->value());
MVC->redraw();
}
void GUI::cb_RaftEnableButton(Fl_Light_Button* o, void* v) {
  ((GUI*)(o->parent()->user_data()))->cb_RaftEnableButton_i(o,v);
}

void GUI::cb_RaftMaterialPrDistanceRatioSlider_i(Fl_Value_Slider* o, void*) {
  MVC->SetRaftMaterialPrDistanceRatio(o->value());
}
void GUI::cb_RaftMaterialPrDistanceRatioSlider(Fl_Value_Slider* o, void* v) {
  ((GUI*)(o->parent()->parent()->user_data()))->cb_RaftMaterialPrDistanceRatioSlider_i(o,v);
}

void GUI::cb_RaftRotationSlider_i(Fl_Value_Slider* o, void*) {
  MVC->SetRaftRotation(o->value());
}
void GUI::cb_RaftRotationSlider(Fl_Value_Slider* o, void* v) {
  ((GUI*)(o->parent()->parent()->user_data()))->cb_RaftRotationSlider_i(o,v);
}

void GUI::cb_RaftBaseDistanceSlider_i(Fl_Value_Slider* o, void*) {
  MVC->SetRaftBaseDistance(o->value());
}
void GUI::cb_RaftBaseDistanceSlider(Fl_Value_Slider* o, void* v) {
  ((GUI*)(o->parent()->parent()->user_data()))->cb_RaftBaseDistanceSlider_i(o,v);
}

void GUI::cb_RaftBaseThicknessSlider_i(Fl_Value_Slider* o, void*) {
  MVC->SetRaftBaseThickness(o->value());
}
void GUI::cb_RaftBaseThicknessSlider(Fl_Value_Slider* o, void* v) {
  ((GUI*)(o->parent()->parent()->user_data()))->cb_RaftBaseThicknessSlider_i(o,v);
}

void GUI::cb_RaftBaseLayerCountSlider_i(Fl_Value_Slider* o, void*) {
  MVC->SetRaftBaseLayerCount(o->value());
}
void GUI::cb_RaftBaseLayerCountSlider(Fl_Value_Slider* o, void* v) {
  ((GUI*)(o->parent()->parent()->user_data()))->cb_RaftBaseLayerCountSlider_i(o,v);
}

void GUI::cb_RaftBaseTemperatureSlider_i(Fl_Value_Slider* o, void*) {
  MVC->SetRaftBaseTemperature(o->value());
}
void GUI::cb_RaftBaseTemperatureSlider(Fl_Value_Slider* o, void* v) {
  ((GUI*)(o->parent()->parent()->user_data()))->cb_RaftBaseTemperatureSlider_i(o,v);
}

void GUI::cb_RaftSizeSlider_i(Fl_Value_Slider* o, void*) {
  MVC->SetRaftSize(o->value());
}
void GUI::cb_RaftSizeSlider(Fl_Value_Slider* o, void* v) {
  ((GUI*)(o->parent()->parent()->user_data()))->cb_RaftSizeSlider_i(o,v);
}

void GUI::cb_RaftInterfaceMaterialPrDistanceRatioSlider_i(Fl_Value_Slider* o, void*) {
  MVC->SetRaftInterfaceMaterialPrDistanceRatio(o->value());
}
void GUI::cb_RaftInterfaceMaterialPrDistanceRatioSlider(Fl_Value_Slider* o, void* v) {
  ((GUI*)(o->parent()->parent()->user_data()))->cb_RaftInterfaceMaterialPrDistanceRatioSlider_i(o,v);
}

void GUI::cb_RaftRotationPrLayerSlider_i(Fl_Value_Slider* o, void*) {
  MVC->SetRaftRotationPrLayer(o->value());
}
void GUI::cb_RaftRotationPrLayerSlider(Fl_Value_Slider* o, void* v) {
  ((GUI*)(o->parent()->parent()->user_data()))->cb_RaftRotationPrLayerSlider_i(o,v);
}

void GUI::cb_RaftInterfaceDistanceSlider_i(Fl_Value_Slider* o, void*) {
  MVC->SetRaftInterfaceDistance(o->value());
}
void GUI::cb_RaftInterfaceDistanceSlider(Fl_Value_Slider* o, void* v) {
  ((GUI*)(o->parent()->parent()->user_data()))->cb_RaftInterfaceDistanceSlider_i(o,v);
}

void GUI::cb_RaftInterfaceThicknessSlider_i(Fl_Value_Slider* o, void*) {
  MVC->SetRaftBaseThickness(o->value());
}
void GUI::cb_RaftInterfaceThicknessSlider(Fl_Value_Slider* o, void* v) {
  ((GUI*)(o->parent()->parent()->user_data()))->cb_RaftInterfaceThicknessSlider_i(o,v);
}

void GUI::cb_RaftInterfaceLayerCountSlider_i(Fl_Value_Slider* o, void*) {
  MVC->SetRaftInterfaceLayerCount(o->value());
}
void GUI::cb_RaftInterfaceLayerCountSlider(Fl_Value_Slider* o, void* v) {
  ((GUI*)(o->parent()->parent()->user_data()))->cb_RaftInterfaceLayerCountSlider_i(o,v);
}

void GUI::cb_RaftInterfaceTemperatureSlider_i(Fl_Value_Slider* o, void*) {
  MVC->SetRaftInterfaceTemperature(o->value());
}
void GUI::cb_RaftInterfaceTemperatureSlider(Fl_Value_Slider* o, void* v) {
  ((GUI*)(o->parent()->parent()->user_data()))->cb_RaftInterfaceTemperatureSlider_i(o,v);
}

void GUI::cb_Ok1_i(Fl_Button*, void*) {
  raftSettingsWindow->hide();
}
void GUI::cb_Ok1(Fl_Button* o, void* v) {
  ((GUI*)(o->parent()->user_data()))->cb_Ok1_i(o,v);
}

void GUI::cb_DisplayPolygonsButton_i(Fl_Light_Button* o, void*) {
  MVC->SetDisplayPolygons(o->value());
}
void GUI::cb_DisplayPolygonsButton(Fl_Light_Button* o, void* v) {
  ((GUI*)(o->parent()->parent()->user_data()))->cb_DisplayPolygonsButton_i(o,v);
}

void GUI::cb_DisplayWireframeButton_i(Fl_Light_Button* o, void*) {
  MVC->SetDisplayWireframe(o->value());
}
void GUI::cb_DisplayWireframeButton(Fl_Light_Button* o, void* v) {
  ((GUI*)(o->parent()->parent()->user_data()))->cb_DisplayWireframeButton_i(o,v);
}

void GUI::cb_DisplayNormalsButton_i(Fl_Light_Button* o, void*) {
  MVC->SetDisplayNormals(o->value());
}
void GUI::cb_DisplayNormalsButton(Fl_Light_Button* o, void* v) {
  ((GUI*)(o->parent()->parent()->user_data()))->cb_DisplayNormalsButton_i(o,v);
}

void GUI::cb_DisplayEndpointsButton_i(Fl_Light_Button* o, void*) {
  MVC->SetDisplayEndpoints(o->value());
}
void GUI::cb_DisplayEndpointsButton(Fl_Light_Button* o, void* v) {
  ((GUI*)(o->parent()->parent()->user_data()))->cb_DisplayEndpointsButton_i(o,v);
}

void GUI::cb_PolygonValSlider_i(Fl_Value_Slider* o, void*) {
  MVC->SetPolygonVal(o->value());
MVC->redraw();
}
void GUI::cb_PolygonValSlider(Fl_Value_Slider* o, void* v) {
  ((GUI*)(o->parent()->parent()->user_data()))->cb_PolygonValSlider_i(o,v);
}

void GUI::cb_PolygonSatSlider_i(Fl_Value_Slider* o, void*) {
  MVC->SetPolygonSat(o->value());
MVC->redraw();
}
void GUI::cb_PolygonSatSlider(Fl_Value_Slider* o, void* v) {
  ((GUI*)(o->parent()->parent()->user_data()))->cb_PolygonSatSlider_i(o,v);
}

void GUI::cb_PolygonHueSlider_i(Fl_Value_Slider* o, void*) {
  MVC->SetPolygonHue(o->value());
MVC->redraw();
}
void GUI::cb_PolygonHueSlider(Fl_Value_Slider* o, void* v) {
  ((GUI*)(o->parent()->parent()->user_data()))->cb_PolygonHueSlider_i(o,v);
}

void GUI::cb_WireframeValSlider_i(Fl_Value_Slider* o, void*) {
  MVC->SetWireframeVal(o->value());
MVC->redraw();
}
void GUI::cb_WireframeValSlider(Fl_Value_Slider* o, void* v) {
  ((GUI*)(o->parent()->parent()->user_data()))->cb_WireframeValSlider_i(o,v);
}

void GUI::cb_WireframeSatSlider_i(Fl_Value_Slider* o, void*) {
  MVC->SetWireframeSat(o->value());
MVC->redraw();
}
void GUI::cb_WireframeSatSlider(Fl_Value_Slider* o, void* v) {
  ((GUI*)(o->parent()->parent()->user_data()))->cb_WireframeSatSlider_i(o,v);
}

void GUI::cb_WireframeHueSlider_i(Fl_Value_Slider* o, void*) {
  MVC->SetWireframeHue(o->value());
MVC->redraw();
}
void GUI::cb_WireframeHueSlider(Fl_Value_Slider* o, void* v) {
  ((GUI*)(o->parent()->parent()->user_data()))->cb_WireframeHueSlider_i(o,v);
}

void GUI::cb_NormalValSlider_i(Fl_Value_Slider* o, void*) {
  MVC->SetNormalsVal(o->value());
MVC->redraw();
}
void GUI::cb_NormalValSlider(Fl_Value_Slider* o, void* v) {
  ((GUI*)(o->parent()->parent()->user_data()))->cb_NormalValSlider_i(o,v);
}

void GUI::cb_NormalSatSlider_i(Fl_Value_Slider* o, void*) {
  MVC->SetNormalsSat(o->value());
MVC->redraw();
}
void GUI::cb_NormalSatSlider(Fl_Value_Slider* o, void* v) {
  ((GUI*)(o->parent()->parent()->user_data()))->cb_NormalSatSlider_i(o,v);
}

void GUI::cb_NormalHueSlider_i(Fl_Value_Slider* o, void*) {
  MVC->SetNormalsHue(o->value());
MVC->redraw();
}
void GUI::cb_NormalHueSlider(Fl_Value_Slider* o, void* v) {
  ((GUI*)(o->parent()->parent()->user_data()))->cb_NormalHueSlider_i(o,v);
}

void GUI::cb_EndpointValSlider_i(Fl_Value_Slider* o, void*) {
  MVC->SetEndpointsVal(o->value());
MVC->redraw();
}
void GUI::cb_EndpointValSlider(Fl_Value_Slider* o, void* v) {
  ((GUI*)(o->parent()->parent()->user_data()))->cb_EndpointValSlider_i(o,v);
}

void GUI::cb_EndpointSatSlider_i(Fl_Value_Slider* o, void*) {
  MVC->SetEndpointsSat(o->value());
MVC->redraw();
}
void GUI::cb_EndpointSatSlider(Fl_Value_Slider* o, void* v) {
  ((GUI*)(o->parent()->parent()->user_data()))->cb_EndpointSatSlider_i(o,v);
}

void GUI::cb_EndpointHueSlider_i(Fl_Value_Slider* o, void*) {
  MVC->SetEndpointsHue(o->value());
MVC->redraw();
}
void GUI::cb_EndpointHueSlider(Fl_Value_Slider* o, void* v) {
  ((GUI*)(o->parent()->parent()->user_data()))->cb_EndpointHueSlider_i(o,v);
}

void GUI::cb_DisplayBBoxButton_i(Fl_Light_Button* o, void*) {
  MVC->SetDisplayBBox(o->value());
}
void GUI::cb_DisplayBBoxButton(Fl_Light_Button* o, void* v) {
  ((GUI*)(o->parent()->parent()->user_data()))->cb_DisplayBBoxButton_i(o,v);
}

void GUI::cb_HighlightSlider_i(Fl_Value_Slider* o, void*) {
  MVC->SetHighlight(o->value());
MVC->redraw();
}
void GUI::cb_HighlightSlider(Fl_Value_Slider* o, void* v) {
  ((GUI*)(o->parent()->parent()->user_data()))->cb_HighlightSlider_i(o,v);
}

void GUI::cb_NormalLengthSlider_i(Fl_Value_Slider* o, void*) {
  MVC->SetNormalsLength(o->value());
MVC->redraw();
}
void GUI::cb_NormalLengthSlider(Fl_Value_Slider* o, void* v) {
  ((GUI*)(o->parent()->parent()->user_data()))->cb_NormalLengthSlider_i(o,v);
}

void GUI::cb_EndpointSizeSlider_i(Fl_Value_Slider* o, void*) {
  MVC->SetEndPointSize(o->value());
MVC->redraw();
}
void GUI::cb_EndpointSizeSlider(Fl_Value_Slider* o, void* v) {
  ((GUI*)(o->parent()->parent()->user_data()))->cb_EndpointSizeSlider_i(o,v);
}

void GUI::cb_DisplayWireframeShadedButton_i(Fl_Light_Button* o, void*) {
  MVC->SetDisplayWireframeShaded(o->value());
}
void GUI::cb_DisplayWireframeShadedButton(Fl_Light_Button* o, void* v) {
  ((GUI*)(o->parent()->parent()->user_data()))->cb_DisplayWireframeShadedButton_i(o,v);
}

void GUI::cb_PolygonOpasitySlider_i(Fl_Value_Slider* o, void*) {
  MVC->SetPolygonOpasity(o->value());
MVC->redraw();
}
void GUI::cb_PolygonOpasitySlider(Fl_Value_Slider* o, void* v) {
  ((GUI*)(o->parent()->parent()->user_data()))->cb_PolygonOpasitySlider_i(o,v);
}

void GUI::cb_DisplayGCodeButton_i(Fl_Light_Button* o, void*) {
  MVC->SetDisplayGCode(o->value());
MVC->redraw();
}
void GUI::cb_DisplayGCodeButton(Fl_Light_Button* o, void* v) {
  ((GUI*)(o->parent()->parent()->user_data()))->cb_DisplayGCodeButton_i(o,v);
}

void GUI::cb_GCodeExtrudeValSlider_i(Fl_Value_Slider* o, void*) {
  MVC->SetGCodeExtrudeVal(o->value());
MVC->redraw();
}
void GUI::cb_GCodeExtrudeValSlider(Fl_Value_Slider* o, void* v) {
  ((GUI*)(o->parent()->parent()->user_data()))->cb_GCodeExtrudeValSlider_i(o,v);
}

void GUI::cb_GCodeExtrudeSatSlider_i(Fl_Value_Slider* o, void*) {
  MVC->SetGCodeExtrudeSat(o->value());
MVC->redraw();
}
void GUI::cb_GCodeExtrudeSatSlider(Fl_Value_Slider* o, void* v) {
  ((GUI*)(o->parent()->parent()->user_data()))->cb_GCodeExtrudeSatSlider_i(o,v);
}

void GUI::cb_GCodeExtrudeHueSlider_i(Fl_Value_Slider* o, void*) {
  MVC->SetGCodeExtrudeHue(o->value());
MVC->redraw();
}
void GUI::cb_GCodeExtrudeHueSlider(Fl_Value_Slider* o, void* v) {
  ((GUI*)(o->parent()->parent()->user_data()))->cb_GCodeExtrudeHueSlider_i(o,v);
}

void GUI::cb_LuminanceShowsSpeedButton_i(Fl_Light_Button* o, void*) {
  MVC->SetLuminanceShowsSpeed(o->value());
MVC->redraw();
}
void GUI::cb_LuminanceShowsSpeedButton(Fl_Light_Button* o, void* v) {
  ((GUI*)(o->parent()->parent()->user_data()))->cb_LuminanceShowsSpeedButton_i(o,v);
}

void GUI::cb_Crop_i(Fl_Button*, void*) {
  float start = GCodeDrawStartSlider->value();
float end = GCodeDrawEndSlider->value();
GCodeDrawStartSlider->range(start,end);
GCodeDrawEndSlider->range(start,end);
GCodeDrawStartSlider->redraw();
GCodeDrawEndSlider->redraw();
}
void GUI::cb_Crop(Fl_Button* o, void* v) {
  ((GUI*)(o->parent()->parent()->user_data()))->cb_Crop_i(o,v);
}

void GUI::cb_Reset_i(Fl_Button*, void*) {
  GCodeDrawStartSlider->range(0,1);
GCodeDrawEndSlider->range(0,1);
GCodeDrawStartSlider->redraw();
GCodeDrawEndSlider->redraw();
}
void GUI::cb_Reset(Fl_Button* o, void* v) {
  ((GUI*)(o->parent()->parent()->user_data()))->cb_Reset_i(o,v);
}

void GUI::cb_GCodeDrawStartSlider_i(Fl_Slider* o, void*) {
  MVC->SetGCodeDrawStart(o->value());
}
void GUI::cb_GCodeDrawStartSlider(Fl_Slider* o, void* v) {
  ((GUI*)(o->parent()->parent()->user_data()))->cb_GCodeDrawStartSlider_i(o,v);
}

void GUI::cb_GCodeDrawEndSlider_i(Fl_Slider* o, void*) {
  MVC->SetGCodeDrawEnd(o->value());
}
void GUI::cb_GCodeDrawEndSlider(Fl_Slider* o, void* v) {
  ((GUI*)(o->parent()->parent()->user_data()))->cb_GCodeDrawEndSlider_i(o,v);
}

void GUI::cb_GCodeMoveValSlider_i(Fl_Value_Slider* o, void*) {
  MVC->SetGCodeMoveVal(o->value());
MVC->redraw();
}
void GUI::cb_GCodeMoveValSlider(Fl_Value_Slider* o, void* v) {
  ((GUI*)(o->parent()->parent()->user_data()))->cb_GCodeMoveValSlider_i(o,v);
}

void GUI::cb_GCodeMoveSatSlider_i(Fl_Value_Slider* o, void*) {
  MVC->SetGCodeMoveSat(o->value());
MVC->redraw();
}
void GUI::cb_GCodeMoveSatSlider(Fl_Value_Slider* o, void* v) {
  ((GUI*)(o->parent()->parent()->user_data()))->cb_GCodeMoveSatSlider_i(o,v);
}

void GUI::cb_GCodeMoveHueSlider_i(Fl_Value_Slider* o, void*) {
  MVC->SetGCodeMoveHue(o->value());
MVC->redraw();
}
void GUI::cb_GCodeMoveHueSlider(Fl_Value_Slider* o, void* v) {
  ((GUI*)(o->parent()->parent()->user_data()))->cb_GCodeMoveHueSlider_i(o,v);
}

void GUI::cb_CuttingPlaneValueSlider_i(Fl_Value_Slider* o, void*) {
  MVC->SetCuttingPlaneValue(o->value());
}
void GUI::cb_CuttingPlaneValueSlider(Fl_Value_Slider* o, void* v) {
  ((GUI*)(o->parent()->parent()->user_data()))->cb_CuttingPlaneValueSlider_i(o,v);
}

void GUI::cb_DisplayCuttingPlaneButton_i(Fl_Light_Button* o, void*) {
  MVC->SetDisplayCuttingPlane(o->value());
}
void GUI::cb_DisplayCuttingPlaneButton(Fl_Light_Button* o, void* v) {
  ((GUI*)(o->parent()->parent()->user_data()))->cb_DisplayCuttingPlaneButton_i(o,v);
}

void GUI::cb_DisplayinFillButton_i(Fl_Light_Button* o, void*) {
  MVC->SetDisplayinFill(o->value());
}
void GUI::cb_DisplayinFillButton(Fl_Light_Button* o, void* v) {
  ((GUI*)(o->parent()->parent()->user_data()))->cb_DisplayinFillButton_i(o,v);
}

void GUI::cb_DisplayAllLayersButton_i(Fl_Light_Button* o, void*) {
  MVC->SetDisplayAllLayers(o->value());
}
void GUI::cb_DisplayAllLayersButton(Fl_Light_Button* o, void* v) {
  ((GUI*)(o->parent()->parent()->user_data()))->cb_DisplayAllLayersButton_i(o,v);
}

void GUI::cb_DrawCuttingPlaneVertexNumbersButton_i(Fl_Light_Button* o, void*) {
  MVC->SetDrawCuttingPlaneVertexNumbers(o->value());
}
void GUI::cb_DrawCuttingPlaneVertexNumbersButton(Fl_Light_Button* o, void* v) {
  ((GUI*)(o->parent()->parent()->user_data()))->cb_DrawCuttingPlaneVertexNumbersButton_i(o,v);
}

void GUI::cb_DrawCuttingPlaneLineNumbersButton_i(Fl_Light_Button* o, void*) {
  MVC->SetDrawCuttingPlaneLineNumbers(o->value());
}
void GUI::cb_DrawCuttingPlaneLineNumbersButton(Fl_Light_Button* o, void* v) {
  ((GUI*)(o->parent()->parent()->user_data()))->cb_DrawCuttingPlaneLineNumbersButton_i(o,v);
}

void GUI::cb_DrawCuttingPlanePolygonNumbersButton_i(Fl_Light_Button* o, void*) {
  MVC->SetDrawCuttingPlanePolyNumbers(o->value());
}
void GUI::cb_DrawCuttingPlanePolygonNumbersButton(Fl_Light_Button* o, void* v) {
  ((GUI*)(o->parent()->parent()->user_data()))->cb_DrawCuttingPlanePolygonNumbersButton_i(o,v);
}

void GUI::cb_Enable_i(Fl_Light_Button* o, void*) {
  //  MVC->SetEnableLight(0, o->value());
}
void GUI::cb_Enable(Fl_Light_Button* o, void* v) {
  ((GUI*)(o->parent()->parent()->user_data()))->cb_Enable_i(o,v);
}

void GUI::cb_Enable1_i(Fl_Light_Button* o, void*) {
  //  MVC->SetEnableLight(1, o->value());
}
void GUI::cb_Enable1(Fl_Light_Button* o, void* v) {
  ((GUI*)(o->parent()->parent()->user_data()))->cb_Enable1_i(o,v);
}

void GUI::cb_Enable2_i(Fl_Light_Button* o, void*) {
  //  MVC->SetEnableLight(2, o->value());
}
void GUI::cb_Enable2(Fl_Light_Button* o, void* v) {
  ((GUI*)(o->parent()->parent()->user_data()))->cb_Enable2_i(o,v);
}

void GUI::cb_Enable3_i(Fl_Light_Button* o, void*) {
  //  MVC->SetEnableLight(3, o->value());
}
void GUI::cb_Enable3(Fl_Light_Button* o, void* v) {
  ((GUI*)(o->parent()->parent()->user_data()))->cb_Enable3_i(o,v);
}

void GUI::cb_DisplayDebuginFillButton_i(Fl_Light_Button* o, void*) {
  MVC->SetDisplayDebuginFill(o->value());
}
void GUI::cb_DisplayDebuginFillButton(Fl_Light_Button* o, void* v) {
  ((GUI*)(o->parent()->user_data()))->cb_DisplayDebuginFillButton_i(o,v);
}

void GUI::cb_DisplayDebugButton_i(Fl_Light_Button* o, void*) {
  if(o->value())
{
DisplayDebuginFillButton->show();
ExamineSlider->show();
}
else
{
DisplayDebuginFillButton->hide();
ExamineSlider->hide();
}


MVC->redraw();
}
void GUI::cb_DisplayDebugButton(Fl_Light_Button* o, void* v) {
  ((GUI*)(o->parent()->user_data()))->cb_DisplayDebugButton_i(o,v);
}

void GUI::cb_DrawVertexNumbersButton_i(Fl_Light_Button* o, void*) {
  MVC->SetDrawVertexNumbers(o->value());
}
void GUI::cb_DrawVertexNumbersButton(Fl_Light_Button* o, void* v) {
  ((GUI*)(o->parent()->user_data()))->cb_DrawVertexNumbersButton_i(o,v);
}

void GUI::cb_DrawLineNumbersButton_i(Fl_Light_Button* o, void*) {
  MVC->SetDrawLineNumbers(o->value());
}
void GUI::cb_DrawLineNumbersButton(Fl_Light_Button* o, void* v) {
  ((GUI*)(o->parent()->user_data()))->cb_DrawLineNumbersButton_i(o,v);
}

void GUI::cb_ExamineSlider_i(Fl_Value_Slider* o, void*) {
  MVC->SetExamine(o->value());
}
void GUI::cb_ExamineSlider(Fl_Value_Slider* o, void* v) {
  ((GUI*)(o->parent()->user_data()))->cb_ExamineSlider_i(o,v);
}

void GUI::cb_Ok2_i(Fl_Button*, void*) {
  DisplaySettingsWindow->hide();
}
void GUI::cb_Ok2(Fl_Button* o, void* v) {
  ((GUI*)(o->parent()->user_data()))->cb_Ok2_i(o,v);
}

void GUI::cb_CustomButtonSelectorSlider_i(Fl_Value_Slider* o, void*) {
  MVC->GetCustomButtonText(o->value()-1);
}
void GUI::cb_CustomButtonSelectorSlider(Fl_Value_Slider* o, void* v) {
  ((GUI*)(o->parent()->user_data()))->cb_CustomButtonSelectorSlider_i(o,v);
}

void GUI::cb_Test_i(Fl_Button*, void*) {
  MVC->TestCustomButton();
}
void GUI::cb_Test(Fl_Button* o, void* v) {
  ((GUI*)(o->parent()->user_data()))->cb_Test_i(o,v);
}

void GUI::cb_Save3_i(Fl_Button*, void*) {
  MVC->SaveCustomButton();
}
void GUI::cb_Save3(Fl_Button* o, void* v) {
  ((GUI*)(o->parent()->user_data()))->cb_Save3_i(o,v);
}

void GUI::cb_Ok3_i(Fl_Button*, void*) {
  DisplaySettingsWindow->hide();
}
void GUI::cb_Ok3(Fl_Button* o, void* v) {
  ((GUI*)(o->parent()->user_data()))->cb_Ok3_i(o,v);
}

GUI::GUI() {
  { mainWindow = new Fl_Double_Window(1275, 860, "RepSnapper by Kulitorum www.kulitorum.com");
    mainWindow->box(FL_UP_BOX);
    mainWindow->color((Fl_Color)FL_DARK3);
    mainWindow->selection_color((Fl_Color)FL_DARK3);
    mainWindow->labelsize(12);
    mainWindow->user_data((void*)(this));
    { MainMenuBar = new Fl_Menu_Bar(5, 5, 705, 25);
      MainMenuBar->align(FL_ALIGN_TOP_LEFT);
      MainMenuBar->menu(menu_MainMenuBar);
    } // Fl_Menu_Bar* MainMenuBar
    { 
      MVC = NULL;
    } // ModelViewController* MVC
    { Tabs = new Fl_Tabs(710, 0, 565, 852);
      Tabs->callback((Fl_Callback*)cb_Tabs);
      Tabs->align(FL_ALIGN_TOP_LEFT);
      Tabs->when(FL_WHEN_CHANGED);
      { SimpleTab = new Fl_Group(715, 21, 545, 824, "Simple");
        { Fl_Group* o = new Fl_Group(725, 87, 535, 88, "Setup");
          o->box(FL_ENGRAVED_FRAME);
          o->color((Fl_Color)FL_DARK3);
          { SerialSpeedInputSimple = new Fl_Value_Input(1045, 132, 135, 26, "Speed:");
            SerialSpeedInputSimple->callback((Fl_Callback*)cb_SerialSpeedInputSimple);
          } // Fl_Value_Input* SerialSpeedInputSimple
          { ConnectToPrinterSimpleButton = new Fl_Light_Button(740, 99, 160, 25, "Connect to printer");
            ConnectToPrinterSimpleButton->selection_color((Fl_Color)2);
            ConnectToPrinterSimpleButton->callback((Fl_Callback*)cb_ConnectToPrinterSimpleButton);
          } // Fl_Light_Button* ConnectToPrinterSimpleButton
          { Fl_Text_Display* o = new Fl_Text_Display(855, 98, 55, 25, "First lets connect to the printer");
            o->box(FL_NO_BOX);
            o->align(FL_ALIGN_RIGHT);
          } // Fl_Text_Display* o
          { portInputSimple = new Fl_Input_Choice(770, 133, 205, 24, "Port:");
            portInputSimple->callback((Fl_Callback*)cb_portInputSimple);
            portInputSimple->when(FL_WHEN_CHANGED);
            portInputSimple->menu(menu_portInputSimple);
          } // Fl_Input_Choice* portInputSimple
          o->end();
        } // Fl_Group* o
        { Fl_Group* o = new Fl_Group(725, 199, 535, 141, "Model");
          o->box(FL_ENGRAVED_FRAME);
          o->color((Fl_Color)FL_DARK3);
          { Fl_Button* o = new Fl_Button(740, 217, 160, 25, "Load STL");
            o->callback((Fl_Callback*)cb_Load);
          } // Fl_Button* o
          { Fl_Button* o = new Fl_Button(740, 246, 160, 25, "Convert to GCode");
            o->callback((Fl_Callback*)cb_Convert);
          } // Fl_Button* o
          { Fl_Text_Display* o = new Fl_Text_Display(855, 216, 55, 25, "Then you want to load an STL model");
            o->box(FL_NO_BOX);
            o->align(FL_ALIGN_RIGHT);
          } // Fl_Text_Display* o
          { Fl_Text_Display* o = new Fl_Text_Display(855, 246, 60, 24, "And convert it into GCODE");
            o->box(FL_NO_BOX);
            o->align(FL_ALIGN_RIGHT);
          } // Fl_Text_Display* o
          { Fl_Text_Display* o = new Fl_Text_Display(855, 296, 45, 29, "Or just load previously generated GCODE directly");
            o->box(FL_NO_BOX);
            o->align(FL_ALIGN_RIGHT);
          } // Fl_Text_Display* o
          { new Fl_Box(940, 270, 120, 25, "-----------------------------------------------------------------------------\
------------------------");
          } // Fl_Box* o
          o->end();
        } // Fl_Group* o
        { Fl_Group* o = new Fl_Group(725, 363, 535, 62, "Print");
          o->box(FL_ENGRAVED_FRAME);
          o->color((Fl_Color)FL_DARK3);
          { Fl_Text_Display* o = new Fl_Text_Display(865, 378, 45, 25, "Make a print");
            o->box(FL_NO_BOX);
            o->align(FL_ALIGN_RIGHT);
          } // Fl_Text_Display* o
          o->end();
        } // Fl_Group* o
        { VersionText = new Fl_Output(805, 819, 220, 26, "Version");
        } // Fl_Output* VersionText
        SimpleTab->end();
      } // Fl_Group* SimpleTab
      { InputTab = new Fl_Group(710, 19, 560, 832, "Input file");
        InputTab->hide();
        { FixSTLerrorsButton = new Fl_Light_Button(910, 89, 135, 21, "Fix STL errors");
          FixSTLerrorsButton->value(1);
          FixSTLerrorsButton->selection_color((Fl_Color)FL_GREEN);
          FixSTLerrorsButton->callback((Fl_Callback*)cb_FixSTLerrorsButton);
        } // Fl_Light_Button* FixSTLerrorsButton
        { Fl_Button* o = new Fl_Button(715, 126, 135, 21, "Save STL");
          o->callback((Fl_Callback*)cb_Save1);
          o->deactivate();
        } // Fl_Button* o
        { Fl_Group* o = new Fl_Group(715, 710, 535, 50, "Object rotation");
          o->box(FL_FLAT_BOX);
          o->color((Fl_Color)FL_DARK3);
          { AutoRotateButton = new Fl_Button(725, 720, 125, 25, "Auto rotate");
            AutoRotateButton->callback((Fl_Callback*)cb_AutoRotateButton);
          } // Fl_Button* AutoRotateButton
          o->end();
        } // Fl_Group* o
        { RFP_Browser = new Flu_Tree_Browser(720, 156, 355, 450, "RFO file");
          RFP_Browser->box(FL_UP_BOX);
          RFP_Browser->color((Fl_Color)FL_BACKGROUND_COLOR);
          RFP_Browser->selection_color((Fl_Color)FL_BACKGROUND_COLOR);
          RFP_Browser->labeltype(FL_NORMAL_LABEL);
          RFP_Browser->labelfont(0);
          RFP_Browser->labelsize(14);
          RFP_Browser->labelcolor((Fl_Color)FL_FOREGROUND_COLOR);
          RFP_Browser->callback((Fl_Callback*)cb_RFP_Browser);
          RFP_Browser->align(FL_ALIGN_CENTER|FL_ALIGN_INSIDE);
          RFP_Browser->when(FL_WHEN_CHANGED);
          Fl_Group::current()->resizable(RFP_Browser);
        } // Flu_Tree_Browser* RFP_Browser
        { Fl_Group* o = new Fl_Group(1080, 163, 180, 40, "Translate");
          o->box(FL_ENGRAVED_FRAME);
          o->color((Fl_Color)FL_DARK3);
          { TranslateX = new Fl_Value_Input(1095, 174, 45, 23, "X");
            TranslateX->minimum(-300);
            TranslateX->maximum(300);
            TranslateX->step(0.1);
            TranslateX->callback((Fl_Callback*)cb_TranslateX);
          } // Fl_Value_Input* TranslateX
          { TranslateY = new Fl_Value_Input(1155, 175, 45, 23, "Y");
            TranslateY->minimum(-300);
            TranslateY->maximum(300);
            TranslateY->step(0.1);
            TranslateY->callback((Fl_Callback*)cb_TranslateY);
          } // Fl_Value_Input* TranslateY
          { TranslateZ = new Fl_Value_Input(1210, 175, 45, 23, "Z");
            TranslateZ->minimum(-300);
            TranslateZ->maximum(300);
            TranslateZ->step(0.01);
            TranslateZ->callback((Fl_Callback*)cb_TranslateZ);
          } // Fl_Value_Input* TranslateZ
          o->end();
        } // Fl_Group* o
        { Fl_Group* o = new Fl_Group(1080, 226, 180, 40, "Rotate");
          o->box(FL_ENGRAVED_FRAME);
          o->color((Fl_Color)FL_DARK3);
          { RotateX = new Fl_Value_Input(1095, 237, 45, 23, "X");
            RotateX->minimum(-360);
            RotateX->maximum(360);
            RotateX->step(1);
            RotateX->callback((Fl_Callback*)cb_RotateX);
          } // Fl_Value_Input* RotateX
          { RotateY = new Fl_Value_Input(1155, 238, 45, 23, "Y");
            RotateY->minimum(-360);
            RotateY->maximum(360);
            RotateY->step(1);
            RotateY->callback((Fl_Callback*)cb_RotateY);
          } // Fl_Value_Input* RotateY
          { RotateZ = new Fl_Value_Input(1210, 238, 45, 23, "Z");
            RotateZ->minimum(-360);
            RotateZ->maximum(360);
            RotateZ->step(1);
            RotateZ->callback((Fl_Callback*)cb_RotateZ);
          } // Fl_Value_Input* RotateZ
          o->end();
        } // Fl_Group* o
        { Fl_Group* o = new Fl_Group(1080, 289, 180, 68, "Scale");
          o->box(FL_ENGRAVED_FRAME);
          o->color((Fl_Color)FL_DARK3);
          { ScaleX = new Fl_Value_Input(1095, 298, 45, 23, "X");
            ScaleX->minimum(-100);
            ScaleX->maximum(100);
            ScaleX->step(0.01);
            ScaleX->value(1);
            ScaleX->callback((Fl_Callback*)cb_ScaleX);
          } // Fl_Value_Input* ScaleX
          { ScaleY = new Fl_Value_Input(1155, 299, 45, 23, "Y");
            ScaleY->minimum(-100);
            ScaleY->maximum(100);
            ScaleY->step(0.01);
            ScaleY->value(1);
            ScaleY->callback((Fl_Callback*)cb_ScaleY);
          } // Fl_Value_Input* ScaleY
          { ScaleZ = new Fl_Value_Input(1210, 299, 45, 23, "Z");
            ScaleZ->minimum(-100);
            ScaleZ->maximum(100);
            ScaleZ->step(0.01);
            ScaleZ->value(1);
            ScaleZ->callback((Fl_Callback*)cb_ScaleZ);
          } // Fl_Value_Input* ScaleZ
          { ScaleAllAxies = new Fl_Light_Button(1095, 327, 160, 20, "All axis");
            ScaleAllAxies->value(1);
            ScaleAllAxies->selection_color((Fl_Color)2);
          } // Fl_Light_Button* ScaleAllAxies
          o->end();
        } // Fl_Group* o
        { FileLocationInput = new Fl_Input(1080, 428, 180, 24, "File location");
          FileLocationInput->callback((Fl_Callback*)cb_FileLocationInput);
          FileLocationInput->align(FL_ALIGN_TOP_LEFT);
        } // Fl_Input* FileLocationInput
        { FileTypeInput = new Fl_Input(1080, 473, 180, 24, "Filetype");
          FileTypeInput->callback((Fl_Callback*)cb_FileTypeInput);
          FileTypeInput->align(FL_ALIGN_TOP_LEFT);
        } // Fl_Input* FileTypeInput
        { FileMaterialInput = new Fl_Input(1080, 518, 180, 24, "File material");
          FileMaterialInput->callback((Fl_Callback*)cb_FileMaterialInput);
          FileMaterialInput->align(FL_ALIGN_TOP_LEFT);
        } // Fl_Input* FileMaterialInput
        { ObjectNameInput = new Fl_Input(1080, 383, 180, 24, "Object name");
          ObjectNameInput->callback((Fl_Callback*)cb_ObjectNameInput);
          ObjectNameInput->align(FL_ALIGN_TOP_LEFT);
        } // Fl_Input* ObjectNameInput
        { Fl_Button* o = new Fl_Button(860, 127, 135, 25, "Delete");
          o->callback((Fl_Callback*)cb_Delete);
        } // Fl_Button* o
        { Fl_Button* o = new Fl_Button(1005, 127, 135, 25, "Duplicate");
          o->callback((Fl_Callback*)cb_Duplicate);
        } // Fl_Button* o
        InputTab->end();
      } // Fl_Group* InputTab
      { Fl_Group* o = new Fl_Group(710, 20, 560, 829, "GCode");
        o->hide();
        { Fl_Button* o = new Fl_Button(730, 85, 140, 25, "Load Gcode");
          o->callback((Fl_Callback*)cb_Load3);
        } // Fl_Button* o
        { GCodeLengthText = new Fl_Output(870, 86, 110, 24);
        } // Fl_Output* GCodeLengthText
        { Fl_Button* o = new Fl_Button(815, 40, 325, 39, "Convert to GCode");
          o->callback((Fl_Callback*)cb_Convert1);
        } // Fl_Button* o
        { Fl_Tabs* o = new Fl_Tabs(715, 175, 555, 664);
          o->end();
        } // Fl_Tabs* o
        { GCodeInput = new Fl_Input(825, 140, 325, 25, "Send a GCode");
          GCodeInput->callback((Fl_Callback*)cb_GCodeInput);
          GCodeInput->when(FL_WHEN_ENTER_KEY_ALWAYS);
        } // Fl_Input* GCodeInput
        o->end();
      } // Fl_Group* o
      { Interactive = new Fl_Group(710, 19, 565, 833, "Interactive");
        Interactive->align(FL_ALIGN_CENTER);
        Interactive->hide();
        { Fl_Group* o = new Fl_Group(710, 40, 565, 520, "Controls");
          { Fl_Button* o = new Fl_Button(730, 46, 60, 20, "Home");
            o->callback((Fl_Callback*)cb_Home);
          } // Fl_Button* o
          { Fl_Button* o = new Fl_Button(730, 66, 60, 20, "Home");
            o->callback((Fl_Callback*)cb_Home1);
          } // Fl_Button* o
          { Fl_Button* o = new Fl_Button(730, 86, 60, 20, "Home");
            o->callback((Fl_Callback*)cb_Home2);
          } // Fl_Button* o
          { Fl_Button* o = new Fl_Button(831, 115, 370, 25, "Find position in Gcode");
            o->deactivate();
          } // Fl_Button* o
          { Fl_Button* o = new Fl_Button(725, 110, 85, 35, "Home all");
            o->callback((Fl_Callback*)cb_Home3);
          } // Fl_Button* o
          { Fl_Button* o = new Fl_Button(870, 46, 35, 20, "-10");
            o->callback((Fl_Callback*)cb_10);
          } // Fl_Button* o
          { Fl_Button* o = new Fl_Button(910, 46, 35, 20, "-1");
            o->callback((Fl_Callback*)cb_1);
          } // Fl_Button* o
          { Fl_Button* o = new Fl_Button(950, 46, 35, 20, "-.1");
            o->callback((Fl_Callback*)cb_11);
          } // Fl_Button* o
          { Fl_Button* o = new Fl_Button(1040, 46, 35, 20, "+.1");
            o->callback((Fl_Callback*)cb_12);
          } // Fl_Button* o
          { Fl_Button* o = new Fl_Button(1080, 46, 35, 20, "+1");
            o->callback((Fl_Callback*)cb_13);
          } // Fl_Button* o
          { Fl_Button* o = new Fl_Button(1120, 46, 30, 20, "+10");
            o->callback((Fl_Callback*)cb_101);
          } // Fl_Button* o
          { Fl_Button* o = new Fl_Button(820, 46, 45, 20, "-100");
            o->callback((Fl_Callback*)cb_100);
          } // Fl_Button* o
          { Fl_Button* o = new Fl_Button(1155, 46, 45, 20, "+100");
            o->callback((Fl_Callback*)cb_1001);
          } // Fl_Button* o
          { Fl_Button* o = new Fl_Button(870, 66, 35, 20, "-10");
            o->callback((Fl_Callback*)cb_102);
          } // Fl_Button* o
          { Fl_Button* o = new Fl_Button(910, 66, 35, 20, "-1");
            o->callback((Fl_Callback*)cb_14);
          } // Fl_Button* o
          { Fl_Button* o = new Fl_Button(950, 66, 35, 20, "-.1");
            o->callback((Fl_Callback*)cb_15);
          } // Fl_Button* o
          { Fl_Button* o = new Fl_Button(1040, 66, 35, 20, "+.1");
            o->callback((Fl_Callback*)cb_16);
          } // Fl_Button* o
          { Fl_Button* o = new Fl_Button(1080, 66, 35, 20, "+1");
            o->callback((Fl_Callback*)cb_17);
          } // Fl_Button* o
          { Fl_Button* o = new Fl_Button(1120, 66, 30, 20, "+10");
            o->callback((Fl_Callback*)cb_103);
          } // Fl_Button* o
          { Fl_Button* o = new Fl_Button(820, 66, 45, 20, "-100");
            o->callback((Fl_Callback*)cb_1002);
          } // Fl_Button* o
          { Fl_Button* o = new Fl_Button(1155, 66, 45, 20, "+100");
            o->callback((Fl_Callback*)cb_1003);
          } // Fl_Button* o
          { Fl_Button* o = new Fl_Button(870, 86, 35, 20, "-10");
            o->callback((Fl_Callback*)cb_104);
          } // Fl_Button* o
          { Fl_Button* o = new Fl_Button(910, 86, 35, 20, "-1");
            o->callback((Fl_Callback*)cb_18);
          } // Fl_Button* o
          { Fl_Button* o = new Fl_Button(950, 86, 35, 20, "-.1");
            o->callback((Fl_Callback*)cb_19);
          } // Fl_Button* o
          { Fl_Button* o = new Fl_Button(1040, 86, 35, 20, "+.1");
            o->callback((Fl_Callback*)cb_1a);
          } // Fl_Button* o
          { Fl_Button* o = new Fl_Button(1080, 86, 35, 20, "+1");
            o->callback((Fl_Callback*)cb_1b);
          } // Fl_Button* o
          { Fl_Button* o = new Fl_Button(1120, 86, 30, 20, "+10");
            o->callback((Fl_Callback*)cb_105);
          } // Fl_Button* o
          { Fl_Button* o = new Fl_Button(820, 86, 45, 20, "-100");
            o->callback((Fl_Callback*)cb_1004);
          } // Fl_Button* o
          { Fl_Button* o = new Fl_Button(1155, 86, 45, 20, "+100");
            o->callback((Fl_Callback*)cb_1005);
          } // Fl_Button* o
          { XposText = new Fl_Value_Input(990, 46, 45, 20);
            XposText->maximum(500);
            XposText->callback((Fl_Callback*)cb_XposText);
          } // Fl_Value_Input* XposText
          { YposText = new Fl_Value_Input(990, 64, 45, 22);
            YposText->maximum(500);
            YposText->callback((Fl_Callback*)cb_YposText);
          } // Fl_Value_Input* YposText
          { ZposText = new Fl_Value_Input(990, 85, 45, 21);
            ZposText->maximum(500);
            ZposText->callback((Fl_Callback*)cb_ZposText);
          } // Fl_Value_Input* ZposText
          { CustomButton1 = new Fl_Button(720, 414, 265, 20, "Custom button 1");
            CustomButton1->callback((Fl_Callback*)cb_CustomButton1);
          } // Fl_Button* CustomButton1
          { CustomButton3 = new Fl_Button(720, 434, 265, 20, "Custom button 3");
            CustomButton3->callback((Fl_Callback*)cb_CustomButton3);
          } // Fl_Button* CustomButton3
          { CustomButton2 = new Fl_Button(990, 414, 270, 20, "Custom button 2");
            CustomButton2->callback((Fl_Callback*)cb_CustomButton2);
          } // Fl_Button* CustomButton2
          { CustomButton4 = new Fl_Button(990, 434, 270, 20, "Custom button 4");
            CustomButton4->callback((Fl_Callback*)cb_CustomButton4);
          } // Fl_Button* CustomButton4
          { CustomButton5 = new Fl_Button(720, 454, 265, 20, "Custom button 5");
            CustomButton5->callback((Fl_Callback*)cb_CustomButton5);
          } // Fl_Button* CustomButton5
          { CustomButton7 = new Fl_Button(720, 474, 265, 20, "Custom button 7");
            CustomButton7->callback((Fl_Callback*)cb_CustomButton7);
          } // Fl_Button* CustomButton7
          { CustomButton6 = new Fl_Button(990, 454, 270, 20, "Custom button 6");
            CustomButton6->callback((Fl_Callback*)cb_CustomButton6);
          } // Fl_Button* CustomButton6
          { CustomButton8 = new Fl_Button(990, 474, 270, 20, "Custom button 8");
            CustomButton8->callback((Fl_Callback*)cb_CustomButton8);
          } // Fl_Button* CustomButton8
          { CustomButton9 = new Fl_Button(720, 494, 265, 20, "Custom button 9");
            CustomButton9->callback((Fl_Callback*)cb_CustomButton9);
          } // Fl_Button* CustomButton9
          { CustomButton10 = new Fl_Button(990, 494, 270, 20, "Custom button 10");
            CustomButton10->callback((Fl_Callback*)cb_CustomButton10);
          } // Fl_Button* CustomButton10
          { CustomButton11 = new Fl_Button(720, 414, 265, 20, "Custom button 11");
            CustomButton11->callback((Fl_Callback*)cb_CustomButton11);
            CustomButton11->hide();
            CustomButton11->deactivate();
          } // Fl_Button* CustomButton11
          { CustomButton13 = new Fl_Button(720, 434, 265, 20, "Custom button 13");
            CustomButton13->callback((Fl_Callback*)cb_CustomButton13);
            CustomButton13->hide();
            CustomButton13->deactivate();
          } // Fl_Button* CustomButton13
          { CustomButton12 = new Fl_Button(990, 414, 270, 20, "Custom button 12");
            CustomButton12->callback((Fl_Callback*)cb_CustomButton12);
            CustomButton12->hide();
            CustomButton12->deactivate();
          } // Fl_Button* CustomButton12
          { CustomButton14 = new Fl_Button(990, 434, 270, 20, "Custom button 14");
            CustomButton14->callback((Fl_Callback*)cb_CustomButton14);
            CustomButton14->hide();
            CustomButton14->deactivate();
          } // Fl_Button* CustomButton14
          { CustomButton15 = new Fl_Button(720, 454, 265, 20, "Custom button 15");
            CustomButton15->callback((Fl_Callback*)cb_CustomButton15);
            CustomButton15->hide();
            CustomButton15->deactivate();
          } // Fl_Button* CustomButton15
          { CustomButton17 = new Fl_Button(720, 474, 265, 20, "Custom button 17");
            CustomButton17->callback((Fl_Callback*)cb_CustomButton17);
            CustomButton17->hide();
            CustomButton17->deactivate();
          } // Fl_Button* CustomButton17
          { CustomButton16 = new Fl_Button(990, 454, 270, 20, "Custom button 16");
            CustomButton16->callback((Fl_Callback*)cb_CustomButton16);
            CustomButton16->hide();
            CustomButton16->deactivate();
          } // Fl_Button* CustomButton16
          { CustomButton18 = new Fl_Button(990, 474, 270, 20, "Custom button 18");
            CustomButton18->callback((Fl_Callback*)cb_CustomButton18);
            CustomButton18->hide();
            CustomButton18->deactivate();
          } // Fl_Button* CustomButton18
          { CustomButton19 = new Fl_Button(720, 494, 265, 20, "Custom button 19");
            CustomButton19->callback((Fl_Callback*)cb_CustomButton19);
            CustomButton19->hide();
            CustomButton19->deactivate();
          } // Fl_Button* CustomButton19
          { CustomButton20 = new Fl_Button(990, 494, 270, 20, "Custom button 20");
            CustomButton20->callback((Fl_Callback*)cb_CustomButton20);
            CustomButton20->hide();
            CustomButton20->deactivate();
          } // Fl_Button* CustomButton20
          { Fl_Group* o = new Fl_Group(815, 520, 420, 40, "Feedback");
            o->box(FL_ENGRAVED_FRAME);
            o->color((Fl_Color)FL_DARK3);
            o->align(FL_ALIGN_LEFT);
            { Fl_Light_Button* o = new Fl_Light_Button(830, 530, 70, 20, "Errors");
              o->value(1);
              o->selection_color((Fl_Color)2);
              o->callback((Fl_Callback*)cb_Errors);
            } // Fl_Light_Button* o
            { Fl_Light_Button* o = new Fl_Light_Button(995, 530, 65, 20, "Info");
              o->value(1);
              o->selection_color((Fl_Color)2);
              o->callback((Fl_Callback*)cb_Info);
            } // Fl_Light_Button* o
            { Fl_Light_Button* o = new Fl_Light_Button(910, 530, 70, 20, "Echo");
              o->value(1);
              o->selection_color((Fl_Color)2);
              o->callback((Fl_Callback*)cb_Echo);
            } // Fl_Light_Button* o
            o->end();
          } // Fl_Group* o
          { Fl_Group* o = new Fl_Group(775, 368, 495, 40, "Fan");
            o->box(FL_ENGRAVED_FRAME);
            o->color((Fl_Color)FL_DARK3);
            o->align(FL_ALIGN_LEFT);
            { FanOnButton = new Fl_Light_Button(810, 376, 85, 25, "Fan on");
              FanOnButton->selection_color((Fl_Color)2);
              FanOnButton->callback((Fl_Callback*)cb_FanOnButton);
            } // Fl_Light_Button* FanOnButton
            { FanPowerSlider = new Fl_Value_Slider(980, 378, 195, 20, "Voltage");
              FanPowerSlider->type(5);
              FanPowerSlider->maximum(12);
              FanPowerSlider->value(5);
              FanPowerSlider->textsize(14);
              FanPowerSlider->callback((Fl_Callback*)cb_FanPowerSlider);
              FanPowerSlider->align(FL_ALIGN_LEFT);
              FanPowerSlider->when(FL_WHEN_RELEASE);
            } // Fl_Value_Slider* FanPowerSlider
            o->end();
          } // Fl_Group* o
          { Extruder = new Fl_Group(775, 283, 495, 81, "Extruder");
            Extruder->box(FL_ENGRAVED_FRAME);
            Extruder->align(FL_ALIGN_LEFT);
            { RunExtruderButton = new Fl_Light_Button(810, 288, 115, 28, "Run extruder");
              RunExtruderButton->callback((Fl_Callback*)cb_RunExtruderButton);
            } // Fl_Light_Button* RunExtruderButton
            { SetExtruderDirectionButton = new Fl_Light_Button(950, 288, 75, 29, "Reverse");
              SetExtruderDirectionButton->callback((Fl_Callback*)cb_SetExtruderDirectionButton);
            } // Fl_Light_Button* SetExtruderDirectionButton
            { Fl_Value_Slider* o = new Fl_Value_Slider(830, 340, 200, 15, "Speed");
              o->type(5);
              o->minimum(5);
              o->maximum(9999);
              o->step(1);
              o->value(3000);
              o->textsize(14);
              o->callback((Fl_Callback*)cb_Speed);
              o->align(FL_ALIGN_LEFT);
            } // Fl_Value_Slider* o
            { Fl_Value_Slider* o = new Fl_Value_Slider(830, 320, 200, 15, "Length");
              o->type(5);
              o->minimum(10);
              o->maximum(850);
              o->step(1);
              o->value(150);
              o->textsize(14);
              o->callback((Fl_Callback*)cb_Length);
              o->align(FL_ALIGN_LEFT);
            } // Fl_Value_Slider* o
            { DownstreamMultiplierSlider = new Fl_Value_Slider(1030, 340, 235, 15, "Downstream speed multiplier");
              DownstreamMultiplierSlider->type(5);
              DownstreamMultiplierSlider->minimum(0.01);
              DownstreamMultiplierSlider->maximum(25);
              DownstreamMultiplierSlider->value(1);
              DownstreamMultiplierSlider->textsize(14);
              DownstreamMultiplierSlider->align(FL_ALIGN_TOP_LEFT);
            } // Fl_Value_Slider* DownstreamMultiplierSlider
            { DownstreamExtrusionMultiplierSlider = new Fl_Value_Slider(1030, 305, 235, 15, "Downstream extrusion multiplier");
              DownstreamExtrusionMultiplierSlider->type(5);
              DownstreamExtrusionMultiplierSlider->minimum(0.01);
              DownstreamExtrusionMultiplierSlider->maximum(25);
              DownstreamExtrusionMultiplierSlider->value(1);
              DownstreamExtrusionMultiplierSlider->textsize(14);
              DownstreamExtrusionMultiplierSlider->align(FL_ALIGN_TOP_LEFT);
            } // Fl_Value_Slider* DownstreamExtrusionMultiplierSlider
            Extruder->end();
          } // Fl_Group* Extruder
          { Temperatures = new Fl_Group(715, 149, 555, 137);
            { TempUpdateSpeedSlider = new Fl_Value_Slider(990, 167, 205, 18, "Temperature display update interval");
              TempUpdateSpeedSlider->type(5);
              TempUpdateSpeedSlider->minimum(0.1);
              TempUpdateSpeedSlider->maximum(10);
              TempUpdateSpeedSlider->step(0.1);
              TempUpdateSpeedSlider->value(3);
              TempUpdateSpeedSlider->textsize(14);
              TempUpdateSpeedSlider->callback((Fl_Callback*)cb_TempUpdateSpeedSlider);
              TempUpdateSpeedSlider->align(FL_ALIGN_LEFT);
            } // Fl_Value_Slider* TempUpdateSpeedSlider
            { TempReadingEnabledButton = new Fl_Light_Button(1200, 163, 70, 24, "Enable");
              TempReadingEnabledButton->value(1);
              TempReadingEnabledButton->selection_color((Fl_Color)2);
              TempReadingEnabledButton->callback((Fl_Callback*)cb_TempReadingEnabledButton);
            } // Fl_Light_Button* TempReadingEnabledButton
            { Fl_Group* o = new Fl_Group(775, 190, 495, 40, "Nozzle");
              o->box(FL_ENGRAVED_FRAME);
              o->align(FL_ALIGN_LEFT);
              { SwitchHeatOnButton = new Fl_Light_Button(805, 194, 85, 30, "heat on");
                SwitchHeatOnButton->callback((Fl_Callback*)cb_SwitchHeatOnButton);
              } // Fl_Light_Button* SwitchHeatOnButton
              { TargetTempText = new Fl_Value_Input(1175, 200, 90, 24, "Target");
                TargetTempText->maximum(300);
                TargetTempText->value(200);
                TargetTempText->callback((Fl_Callback*)cb_TargetTempText);
              } // Fl_Value_Input* TargetTempText
              { CurrentTempText = new Fl_Output(985, 199, 90, 26, "Current");
              } // Fl_Output* CurrentTempText
              o->end();
            } // Fl_Group* o
            { Fl_Group* o = new Fl_Group(775, 235, 495, 41, "Bed");
              o->box(FL_ENGRAVED_FRAME);
              o->color((Fl_Color)FL_DARK3);
              o->align(FL_ALIGN_LEFT);
              { BedHeatOnButton = new Fl_Light_Button(805, 241, 105, 28, "bed heat on");
                BedHeatOnButton->selection_color((Fl_Color)2);
                BedHeatOnButton->callback((Fl_Callback*)cb_BedHeatOnButton);
              } // Fl_Light_Button* BedHeatOnButton
              { CurrentBedTempText = new Fl_Output(985, 244, 90, 26, "Current");
              } // Fl_Output* CurrentBedTempText
              { TargetBedTempText = new Fl_Value_Input(1175, 246, 90, 24, "Target");
                TargetBedTempText->maximum(300);
                TargetBedTempText->value(200);
                TargetBedTempText->callback((Fl_Callback*)cb_TargetBedTempText);
              } // Fl_Value_Input* TargetBedTempText
              o->end();
            } // Fl_Group* o
            Temperatures->end();
          } // Fl_Group* Temperatures
          o->end();
        } // Fl_Group* o
        { Fl_Group* o = new Fl_Group(720, 557, 550, 291);
          { Fl_Tabs* o = new Fl_Tabs(720, 562, 540, 191);
            { Fl_Group* o = new Fl_Group(720, 585, 530, 158, "Communication Log");
              { CommunationLog = new Fl_Multi_Browser(740, 585, 510, 158, "Errors / warnings");
                CommunationLog->box(FL_NO_BOX);
                CommunationLog->color((Fl_Color)FL_BACKGROUND2_COLOR);
                CommunationLog->selection_color((Fl_Color)31);
                CommunationLog->labeltype(FL_NORMAL_LABEL);
                CommunationLog->labelfont(0);
                CommunationLog->labelsize(14);
                CommunationLog->labelcolor((Fl_Color)FL_FOREGROUND_COLOR);
                CommunationLog->align(FL_ALIGN_CENTER|FL_ALIGN_INSIDE);
                CommunationLog->when(FL_WHEN_RELEASE_ALWAYS);
              } // Fl_Multi_Browser* CommunationLog
              o->end();
            } // Fl_Group* o
            { Fl_Group* o = new Fl_Group(720, 585, 530, 158, "Errors / warnings");
              o->hide();
              { ErrorLog = new Fl_Multi_Browser(740, 585, 505, 158, "Errors / warnings");
                ErrorLog->box(FL_NO_BOX);
                ErrorLog->color((Fl_Color)FL_BACKGROUND2_COLOR);
                ErrorLog->selection_color((Fl_Color)31);
                ErrorLog->labeltype(FL_NORMAL_LABEL);
                ErrorLog->labelfont(0);
                ErrorLog->labelsize(14);
                ErrorLog->labelcolor((Fl_Color)FL_FOREGROUND_COLOR);
                ErrorLog->align(FL_ALIGN_CENTER|FL_ALIGN_INSIDE);
                ErrorLog->when(FL_WHEN_RELEASE_ALWAYS);
              } // Fl_Multi_Browser* ErrorLog
              o->end();
            } // Fl_Group* o
            { Fl_Group* o = new Fl_Group(740, 580, 510, 158, "Echo");
              o->hide();
              { Echo = new Fl_Multi_Browser(740, 580, 510, 158, "Echo");
                Echo->box(FL_NO_BOX);
                Echo->color((Fl_Color)FL_BACKGROUND2_COLOR);
                Echo->selection_color((Fl_Color)31);
                Echo->labeltype(FL_NORMAL_LABEL);
                Echo->labelfont(0);
                Echo->labelsize(14);
                Echo->labelcolor((Fl_Color)FL_FOREGROUND_COLOR);
                Echo->align(FL_ALIGN_CENTER|FL_ALIGN_INSIDE);
                Echo->when(FL_WHEN_RELEASE_ALWAYS);
              } // Fl_Multi_Browser* Echo
              o->end();
            } // Fl_Group* o
            o->end();
          } // Fl_Tabs* o
          { AutoscrollButton = new Fl_Light_Button(725, 753, 95, 25, "Auto scroll");
            AutoscrollButton->value(1);
            AutoscrollButton->selection_color((Fl_Color)2);
          } // Fl_Light_Button* AutoscrollButton
          { LinesToKeepSlider = new Fl_Value_Slider(970, 778, 285, 30, "Keep only the last # lines:");
            LinesToKeepSlider->type(5);
            LinesToKeepSlider->minimum(100);
            LinesToKeepSlider->maximum(100000);
            LinesToKeepSlider->step(1);
            LinesToKeepSlider->value(1000);
            LinesToKeepSlider->textsize(14);
            LinesToKeepSlider->callback((Fl_Callback*)cb_LinesToKeepSlider);
            LinesToKeepSlider->align(FL_ALIGN_TOP_LEFT);
          } // Fl_Value_Slider* LinesToKeepSlider
          { FileLogginEnabledButton = new Fl_Light_Button(825, 753, 115, 25, "LogFiles");
            FileLogginEnabledButton->value(1);
            FileLogginEnabledButton->selection_color((Fl_Color)2);
            FileLogginEnabledButton->callback((Fl_Callback*)cb_FileLogginEnabledButton);
          } // Fl_Light_Button* FileLogginEnabledButton
          { ClearLogfilesWhenPrintStartsButton = new Fl_Light_Button(725, 783, 215, 25, "Clear logs when print starts");
            ClearLogfilesWhenPrintStartsButton->value(1);
            ClearLogfilesWhenPrintStartsButton->selection_color((Fl_Color)2);
            ClearLogfilesWhenPrintStartsButton->callback((Fl_Callback*)cb_ClearLogfilesWhenPrintStartsButton);
          } // Fl_Light_Button* ClearLogfilesWhenPrintStartsButton
          { Fl_Button* o = new Fl_Button(725, 813, 215, 25, "Clear logs now");
            o->callback((Fl_Callback*)cb_Clear);
          } // Fl_Button* o
          o->end();
        } // Fl_Group* o
        Interactive->end();
      } // Fl_Group* Interactive
      { PrintTab = new Fl_Group(715, 19, 550, 493, "Print");
        PrintTab->color((Fl_Color)FL_DARK1);
        PrintTab->hide();
        { Fl_Group* o = new Fl_Group(725, 57, 515, 324, "Print");
          o->box(FL_ENGRAVED_FRAME);
          o->color((Fl_Color)FL_DARK3);
          { ConnectToPrinterButton = new Fl_Light_Button(775, 83, 150, 27, "Connect to printer");
            ConnectToPrinterButton->selection_color((Fl_Color)2);
            ConnectToPrinterButton->callback((Fl_Callback*)cb_ConnectToPrinterButton);
          } // Fl_Light_Button* ConnectToPrinterButton
          { PrintButton = new Fl_Light_Button(775, 200, 360, 50, "Print");
            PrintButton->selection_color((Fl_Color)2);
            PrintButton->callback((Fl_Callback*)cb_PrintButton);
            PrintButton->align(FL_ALIGN_TEXT_OVER_IMAGE|FL_ALIGN_INSIDE);
          } // Fl_Light_Button* PrintButton
          { Fl_Light_Button* o = new Fl_Light_Button(935, 83, 90, 27, "Power on");
            o->selection_color((Fl_Color)2);
            o->callback((Fl_Callback*)cb_Power);
          } // Fl_Light_Button* o
          { KickButton = new Fl_Button(1050, 83, 85, 27, "Kick");
            KickButton->callback((Fl_Callback*)cb_KickButton);
          } // Fl_Button* KickButton
          { ContinueButton = new Fl_Light_Button(775, 140, 360, 40, "Pause");
            ContinueButton->selection_color((Fl_Color)2);
            ContinueButton->callback((Fl_Callback*)cb_ContinueButton);
            ContinueButton->align(FL_ALIGN_CENTER|FL_ALIGN_INSIDE);
            ContinueButton->deactivate();
          } // Fl_Light_Button* ContinueButton
          o->end();
        } // Fl_Group* o
        PrintTab->end();
      } // Fl_Group* PrintTab
      { Fl_Group* o = new Fl_Group(710, 20, 560, 825, "lua");
        o->hide();
        o->deactivate();
        { LuaTab = new Fl_Group(710, 20, 560, 785, "Lua script");
          LuaTab->box(FL_ENGRAVED_FRAME);
          LuaTab->color((Fl_Color)FL_DARK3);
          LuaTab->hide();
          LuaTab->deactivate();
          { Fl_Text_Editor* o = LuaScriptEditor = new Fl_Text_Editor(1135, 55, 130, 735, "LUA script:");
            LuaScriptEditor->align(FL_ALIGN_TOP_LEFT);
            Fl_Text_Buffer *luascript = new Fl_Text_Buffer();
            o->buffer(luascript);
          } // Fl_Text_Editor* LuaScriptEditor
          { RunLuaButton = new Fl_Button(1265, 20, 5, 30, "Run");
            RunLuaButton->callback((Fl_Callback*)cb_RunLuaButton);
          } // Fl_Button* RunLuaButton
          LuaTab->end();
        } // Fl_Group* LuaTab
        o->end();
        Fl_Group::current()->resizable(o);
      } // Fl_Group* o
      Tabs->end();
    } // Fl_Tabs* Tabs
    mainWindow->end();
    mainWindow->resizable(mainWindow);
  } // Fl_Double_Window* mainWindow
  { printerSettingsWindow = new Fl_Double_Window(740, 710, "Printer settings");
    printerSettingsWindow->user_data((void*)(this));
    { Fl_Group* o = new Fl_Group(145, 25, 230, 40, "Build volume (mm)");
      o->box(FL_ENGRAVED_FRAME);
      o->color((Fl_Color)FL_DARK3);
      { VolumeX = new Fl_Value_Input(165, 36, 45, 23, "X");
        VolumeX->maximum(5000);
        VolumeX->step(1);
        VolumeX->value(200);
      } // Fl_Value_Input* VolumeX
      { VolumeY = new Fl_Value_Input(240, 36, 45, 23, "Y");
        VolumeY->maximum(5000);
        VolumeY->value(200);
      } // Fl_Value_Input* VolumeY
      { VolumeZ = new Fl_Value_Input(315, 36, 45, 23, "Z");
        VolumeZ->maximum(5000);
        VolumeZ->value(140);
      } // Fl_Value_Input* VolumeZ
      o->end();
    } // Fl_Group* o
    { Fl_Group* o = new Fl_Group(405, 25, 235, 40, "Print margin  (mm)");
      o->box(FL_ENGRAVED_FRAME);
      o->color((Fl_Color)FL_DARK3);
      { MarginX = new Fl_Value_Input(430, 36, 45, 23, "X");
        MarginX->maximum(100);
        MarginX->step(1);
        MarginX->value(10);
        MarginX->callback((Fl_Callback*)cb_MarginX);
      } // Fl_Value_Input* MarginX
      { MarginY = new Fl_Value_Input(505, 36, 45, 23, "Y");
        MarginY->maximum(100);
        MarginY->step(1);
        MarginY->value(10);
        MarginY->callback((Fl_Callback*)cb_MarginY);
      } // Fl_Value_Input* MarginY
      { MarginZ = new Fl_Value_Input(575, 37, 45, 23, "Z");
        MarginZ->minimum(-50);
        MarginZ->maximum(100);
        MarginZ->step(0.01);
        MarginZ->callback((Fl_Callback*)cb_MarginZ);
      } // Fl_Value_Input* MarginZ
      o->end();
    } // Fl_Group* o
    { Fl_Group* o = new Fl_Group(415, 425, 315, 75, "Acceleration");
      o->box(FL_ENGRAVED_FRAME);
      o->color((Fl_Color)FL_DARK3);
      { distanceToReachFullSpeedSlider = new Fl_Value_Slider(425, 475, 295, 15, "Distance used to read full speed(mm)");
        distanceToReachFullSpeedSlider->type(5);
        distanceToReachFullSpeedSlider->selection_color((Fl_Color)2);
        distanceToReachFullSpeedSlider->maximum(10);
        distanceToReachFullSpeedSlider->value(0.1);
        distanceToReachFullSpeedSlider->textsize(14);
        distanceToReachFullSpeedSlider->callback((Fl_Callback*)cb_distanceToReachFullSpeedSlider);
        distanceToReachFullSpeedSlider->align(FL_ALIGN_TOP_LEFT);
      } // Fl_Value_Slider* distanceToReachFullSpeedSlider
      { EnableAccelerationButton = new Fl_Light_Button(480, 435, 170, 20, "Enable Acceleration");
        EnableAccelerationButton->selection_color((Fl_Color)FL_GREEN);
        EnableAccelerationButton->callback((Fl_Callback*)cb_EnableAccelerationButton);
      } // Fl_Light_Button* EnableAccelerationButton
      o->end();
    } // Fl_Group* o
    { Fl_Group* o = new Fl_Group(15, 295, 390, 90, "Print speeds");
      o->box(FL_ENGRAVED_FRAME);
      o->color((Fl_Color)FL_DARK3);
      { MaxPrintSpeedXYSlider = new Fl_Value_Slider(215, 318, 170, 16, "Max XY (mm/minute)");
        MaxPrintSpeedXYSlider->type(5);
        MaxPrintSpeedXYSlider->selection_color((Fl_Color)2);
        MaxPrintSpeedXYSlider->maximum(8000);
        MaxPrintSpeedXYSlider->step(100);
        MaxPrintSpeedXYSlider->value(4000);
        MaxPrintSpeedXYSlider->textsize(14);
        MaxPrintSpeedXYSlider->callback((Fl_Callback*)cb_MaxPrintSpeedXYSlider);
        MaxPrintSpeedXYSlider->align(FL_ALIGN_TOP_LEFT);
      } // Fl_Value_Slider* MaxPrintSpeedXYSlider
      { MinPrintSpeedZSlider = new Fl_Value_Slider(25, 357, 165, 16, "Min  Z (mm/minute)");
        MinPrintSpeedZSlider->type(5);
        MinPrintSpeedZSlider->selection_color((Fl_Color)2);
        MinPrintSpeedZSlider->minimum(1);
        MinPrintSpeedZSlider->maximum(2500);
        MinPrintSpeedZSlider->step(10);
        MinPrintSpeedZSlider->value(50);
        MinPrintSpeedZSlider->textsize(14);
        MinPrintSpeedZSlider->callback((Fl_Callback*)cb_MinPrintSpeedZSlider);
        MinPrintSpeedZSlider->align(FL_ALIGN_TOP_LEFT);
      } // Fl_Value_Slider* MinPrintSpeedZSlider
      { MinPrintSpeedXYSlider = new Fl_Value_Slider(30, 320, 165, 20, "Min XY (mm/minute)");
        MinPrintSpeedXYSlider->type(5);
        MinPrintSpeedXYSlider->selection_color((Fl_Color)2);
        MinPrintSpeedXYSlider->maximum(8000);
        MinPrintSpeedXYSlider->step(100);
        MinPrintSpeedXYSlider->value(1000);
        MinPrintSpeedXYSlider->textsize(14);
        MinPrintSpeedXYSlider->callback((Fl_Callback*)cb_MinPrintSpeedXYSlider);
        MinPrintSpeedXYSlider->align(FL_ALIGN_TOP_LEFT);
      } // Fl_Value_Slider* MinPrintSpeedXYSlider
      { MaxPrintSpeedZSlider = new Fl_Value_Slider(215, 356, 170, 18, "Max Z (mm/minute)");
        MaxPrintSpeedZSlider->type(5);
        MaxPrintSpeedZSlider->selection_color((Fl_Color)2);
        MaxPrintSpeedZSlider->maximum(2500);
        MaxPrintSpeedZSlider->step(10);
        MaxPrintSpeedZSlider->value(150);
        MaxPrintSpeedZSlider->textsize(14);
        MaxPrintSpeedZSlider->callback((Fl_Callback*)cb_MaxPrintSpeedZSlider);
        MaxPrintSpeedZSlider->align(FL_ALIGN_TOP_LEFT);
      } // Fl_Value_Slider* MaxPrintSpeedZSlider
      o->end();
    } // Fl_Group* o
    { Fl_Group* o = new Fl_Group(15, 85, 390, 190, "Extruder");
      o->box(FL_ENGRAVED_FRAME);
      o->color((Fl_Color)FL_DARK3);
      { ExtrudedMaterialWidthSlider = new Fl_Value_Slider(25, 160, 230, 20, "Extruded material width");
        ExtrudedMaterialWidthSlider->type(5);
        ExtrudedMaterialWidthSlider->selection_color((Fl_Color)2);
        ExtrudedMaterialWidthSlider->maximum(10);
        ExtrudedMaterialWidthSlider->value(0.7);
        ExtrudedMaterialWidthSlider->textsize(14);
        ExtrudedMaterialWidthSlider->callback((Fl_Callback*)cb_ExtrudedMaterialWidthSlider);
        ExtrudedMaterialWidthSlider->align(FL_ALIGN_TOP_LEFT);
      } // Fl_Value_Slider* ExtrudedMaterialWidthSlider
      { extrusionFactorSlider = new Fl_Value_Slider(25, 200, 230, 20, "Extrusion multiplier");
        extrusionFactorSlider->type(5);
        extrusionFactorSlider->selection_color((Fl_Color)2);
        extrusionFactorSlider->maximum(2);
        extrusionFactorSlider->value(1);
        extrusionFactorSlider->textsize(14);
        extrusionFactorSlider->callback((Fl_Callback*)cb_extrusionFactorSlider);
        extrusionFactorSlider->align(FL_ALIGN_TOP_LEFT);
      } // Fl_Value_Slider* extrusionFactorSlider
      { UseIncrementalEcodeButton = new Fl_Light_Button(25, 95, 305, 20, "Incremental ecode (seperate controller)");
        UseIncrementalEcodeButton->selection_color((Fl_Color)FL_GREEN);
        UseIncrementalEcodeButton->callback((Fl_Callback*)cb_UseIncrementalEcodeButton);
      } // Fl_Light_Button* UseIncrementalEcodeButton
      { LayerThicknessSlider = new Fl_Value_Slider(25, 240, 230, 20, "Layer Thickness");
        LayerThicknessSlider->type(5);
        LayerThicknessSlider->selection_color((Fl_Color)2);
        LayerThicknessSlider->minimum(0.1);
        LayerThicknessSlider->maximum(3);
        LayerThicknessSlider->value(0.4);
        LayerThicknessSlider->textsize(14);
        LayerThicknessSlider->callback((Fl_Callback*)cb_LayerThicknessSlider);
        LayerThicknessSlider->align(FL_ALIGN_TOP_LEFT);
      } // Fl_Value_Slider* LayerThicknessSlider
      { Use3DGcodeButton = new Fl_Light_Button(25, 120, 255, 20, "Use 3D Gcode (for DC extruders)");
        Use3DGcodeButton->selection_color((Fl_Color)FL_GREEN);
        Use3DGcodeButton->callback((Fl_Callback*)cb_Use3DGcodeButton);
      } // Fl_Light_Button* Use3DGcodeButton
      o->end();
    } // Fl_Group* o
    { Fl_Group* o = new Fl_Group(415, 525, 315, 125, "Antiooze Retraction");
      o->box(FL_ENGRAVED_FRAME);
      o->color((Fl_Color)FL_DARK3);
      { antioozeDistanceSlider = new Fl_Value_Slider(425, 580, 300, 15, "Distance to retract filament (mm)");
        antioozeDistanceSlider->tooltip("Not the actual distance the filament is moved but the equivalent extrudate");
        antioozeDistanceSlider->type(5);
        antioozeDistanceSlider->selection_color((Fl_Color)2);
        antioozeDistanceSlider->maximum(60);
        antioozeDistanceSlider->step(0.5);
        antioozeDistanceSlider->value(4.5);
        antioozeDistanceSlider->textsize(14);
        antioozeDistanceSlider->callback((Fl_Callback*)cb_antioozeDistanceSlider);
        antioozeDistanceSlider->align(FL_ALIGN_TOP_LEFT);
      } // Fl_Value_Slider* antioozeDistanceSlider
      { EnableAntioozeButton = new Fl_Light_Button(470, 535, 215, 20, "Enable Antiooze Retraction");
        EnableAntioozeButton->selection_color((Fl_Color)FL_GREEN);
        EnableAntioozeButton->callback((Fl_Callback*)cb_EnableAntioozeButton);
      } // Fl_Light_Button* EnableAntioozeButton
      { antioozeSpeedSlider = new Fl_Value_Slider(425, 616, 300, 18, "Speed to retract filament");
        antioozeSpeedSlider->type(5);
        antioozeSpeedSlider->selection_color((Fl_Color)2);
        antioozeSpeedSlider->maximum(16000);
        antioozeSpeedSlider->step(100);
        antioozeSpeedSlider->value(1000);
        antioozeSpeedSlider->textsize(14);
        antioozeSpeedSlider->callback((Fl_Callback*)cb_antioozeSpeedSlider);
        antioozeSpeedSlider->align(FL_ALIGN_TOP_LEFT);
      } // Fl_Value_Slider* antioozeSpeedSlider
      o->end();
    } // Fl_Group* o
    { Fl_Group* o = new Fl_Group(415, 83, 315, 112, "Port");
      o->box(FL_ENGRAVED_FRAME);
      o->color((Fl_Color)FL_DARK3);
      { SerialSpeedInput = new Fl_Value_Input(475, 125, 70, 22, "Speed:");
        SerialSpeedInput->callback((Fl_Callback*)cb_SerialSpeedInput);
      } // Fl_Value_Input* SerialSpeedInput
      { portInput = new Fl_Input_Choice(475, 92, 175, 23, "Port:");
        portInput->callback((Fl_Callback*)cb_portInput);
        portInput->when(FL_WHEN_CHANGED);
        portInput->menu(menu_portInput);
      } // Fl_Input_Choice* portInput
      { ValidateConnection = new Fl_Light_Button(475, 157, 200, 18, "Validate connection");
        ValidateConnection->selection_color((Fl_Color)FL_GREEN);
        ValidateConnection->callback((Fl_Callback*)cb_ValidateConnection);
      } // Fl_Light_Button* ValidateConnection
      o->end();
    } // Fl_Group* o
    { Fl_Group* o = new Fl_Group(415, 225, 315, 50, "Receiving buffer size");
      o->box(FL_ENGRAVED_FRAME);
      o->color((Fl_Color)FL_DARK3);
      { ReceivingBufferSizeSlider = new Fl_Value_Slider(420, 250, 305, 20, "Buffer size on printer, in number of gcodes");
        ReceivingBufferSizeSlider->tooltip("Recommended value for FiveD is 4");
        ReceivingBufferSizeSlider->type(5);
        ReceivingBufferSizeSlider->selection_color((Fl_Color)2);
        ReceivingBufferSizeSlider->minimum(1);
        ReceivingBufferSizeSlider->maximum(100);
        ReceivingBufferSizeSlider->step(1);
        ReceivingBufferSizeSlider->value(4);
        ReceivingBufferSizeSlider->textsize(14);
        ReceivingBufferSizeSlider->callback((Fl_Callback*)cb_ReceivingBufferSizeSlider);
        ReceivingBufferSizeSlider->align(FL_ALIGN_TOP_LEFT);
      } // Fl_Value_Slider* ReceivingBufferSizeSlider
      o->end();
    } // Fl_Group* o
    { Fl_Button* o = new Fl_Button(295, 660, 155, 35, "Ok");
      o->callback((Fl_Callback*)cb_Ok);
    } // Fl_Button* o
    { Fl_Group* o = new Fl_Group(5, 405, 400, 125, "Infill");
      o->box(FL_ENGRAVED_FRAME);
      o->color((Fl_Color)FL_DARK3);
      { InfillRotationSlider = new Fl_Value_Slider(10, 495, 190, 20, "Rotation");
        InfillRotationSlider->type(5);
        InfillRotationSlider->selection_color((Fl_Color)2);
        InfillRotationSlider->maximum(360);
        InfillRotationSlider->step(1);
        InfillRotationSlider->value(45);
        InfillRotationSlider->textsize(14);
        InfillRotationSlider->callback((Fl_Callback*)cb_InfillRotationSlider);
        InfillRotationSlider->align(FL_ALIGN_TOP_LEFT);
      } // Fl_Value_Slider* InfillRotationSlider
      { InfillRotationPrLayerSlider = new Fl_Value_Slider(200, 455, 200, 15, "InFill Rotation pr. Layer");
        InfillRotationPrLayerSlider->type(5);
        InfillRotationPrLayerSlider->selection_color((Fl_Color)2);
        InfillRotationPrLayerSlider->maximum(360);
        InfillRotationPrLayerSlider->step(1);
        InfillRotationPrLayerSlider->value(90);
        InfillRotationPrLayerSlider->textsize(14);
        InfillRotationPrLayerSlider->callback((Fl_Callback*)cb_InfillRotationPrLayerSlider);
        InfillRotationPrLayerSlider->align(FL_ALIGN_TOP_LEFT);
      } // Fl_Value_Slider* InfillRotationPrLayerSlider
      { InfillDistanceSlider = new Fl_Value_Slider(200, 495, 200, 20, "Infill Distance");
        InfillDistanceSlider->type(5);
        InfillDistanceSlider->selection_color((Fl_Color)2);
        InfillDistanceSlider->minimum(0.1);
        InfillDistanceSlider->maximum(10);
        InfillDistanceSlider->step(0.1);
        InfillDistanceSlider->value(2);
        InfillDistanceSlider->textsize(14);
        InfillDistanceSlider->callback((Fl_Callback*)cb_InfillDistanceSlider);
        InfillDistanceSlider->align(FL_ALIGN_TOP_LEFT);
      } // Fl_Value_Slider* InfillDistanceSlider
      { ShellOnlyButton = new Fl_Light_Button(100, 415, 175, 20, "Shell Only - no infill");
        ShellOnlyButton->selection_color((Fl_Color)FL_GREEN);
        ShellOnlyButton->callback((Fl_Callback*)cb_ShellOnlyButton);
      } // Fl_Light_Button* ShellOnlyButton
      { ShellCountSlider = new Fl_Value_Slider(10, 450, 190, 20, "Shell count");
        ShellCountSlider->type(5);
        ShellCountSlider->selection_color((Fl_Color)2);
        ShellCountSlider->minimum(1);
        ShellCountSlider->maximum(10);
        ShellCountSlider->step(1);
        ShellCountSlider->value(1);
        ShellCountSlider->textsize(14);
        ShellCountSlider->callback((Fl_Callback*)cb_ShellCountSlider);
        ShellCountSlider->align(FL_ALIGN_TOP_LEFT);
      } // Fl_Value_Slider* ShellCountSlider
      o->end();
    } // Fl_Group* o
    { Fl_Group* o = new Fl_Group(415, 300, 315, 95, "Shrinking quality");
      o->box(FL_ENGRAVED_FRAME);
      o->color((Fl_Color)2);
      { shrinkAlgorithm = new Fl_Choice(570, 315, 125, 20, "Shrinking Algorithm:");
        shrinkAlgorithm->down_box(FL_BORDER_BOX);
        shrinkAlgorithm->menu(menu_shrinkAlgorithm);
      } // Fl_Choice* shrinkAlgorithm
      { OptimizationSlider = new Fl_Value_Slider(425, 360, 280, 20, "Optimization");
        OptimizationSlider->type(5);
        OptimizationSlider->selection_color((Fl_Color)2);
        OptimizationSlider->minimum(0.001);
        OptimizationSlider->step(0.001);
        OptimizationSlider->value(0.05);
        OptimizationSlider->textsize(14);
        OptimizationSlider->callback((Fl_Callback*)cb_OptimizationSlider);
        OptimizationSlider->align(FL_ALIGN_TOP_LEFT);
      } // Fl_Value_Slider* OptimizationSlider
      o->end();
    } // Fl_Group* o
    { Fl_Group* o = new Fl_Group(0, 550, 405, 100, "Alternate Infill Layers");
      o->box(FL_ENGRAVED_FRAME);
      o->color((Fl_Color)FL_DARK3);
      { AltInfillLayersInput = new Fl_Input(55, 620, 105, 15, "Layers");
        AltInfillLayersInput->callback((Fl_Callback*)cb_AltInfillLayersInput);
      } // Fl_Input* AltInfillLayersInput
      { Fl_Text_Display* o = new Fl_Text_Display(0, 560, 10, 30, "Comma separated layer numbers, 0 is the lowest layer, -1 is the highest");
        o->box(FL_NO_BOX);
        o->labelsize(10);
        o->align(FL_ALIGN_RIGHT);
      } // Fl_Text_Display* o
      { Fl_Text_Display* o = new Fl_Text_Display(30, 580, 20, 20, "(eg. 0,1,-1,-2 means the top and bottom two layers)");
        o->box(FL_NO_BOX);
        o->labelsize(10);
        o->align(FL_ALIGN_RIGHT);
      } // Fl_Text_Display* o
      { AltInfillDistanceSlider = new Fl_Value_Slider(160, 615, 230, 20, "Infill Distance");
        AltInfillDistanceSlider->type(5);
        AltInfillDistanceSlider->selection_color((Fl_Color)2);
        AltInfillDistanceSlider->minimum(0.1);
        AltInfillDistanceSlider->maximum(10);
        AltInfillDistanceSlider->step(0.1);
        AltInfillDistanceSlider->value(2);
        AltInfillDistanceSlider->textsize(14);
        AltInfillDistanceSlider->callback((Fl_Callback*)cb_AltInfillDistanceSlider);
        AltInfillDistanceSlider->align(FL_ALIGN_TOP_LEFT);
      } // Fl_Value_Slider* AltInfillDistanceSlider
      o->end();
    } // Fl_Group* o
    printerSettingsWindow->set_modal();
    printerSettingsWindow->end();
    printerSettingsWindow->resizable(printerSettingsWindow);
  } // Fl_Double_Window* printerSettingsWindow
  { raftSettingsWindow = new Fl_Double_Window(555, 740, "Raft Settings");
    raftSettingsWindow->user_data((void*)(this));
    { RaftEnableButton = new Fl_Light_Button(195, 10, 130, 25, "Raft Enable");
      RaftEnableButton->callback((Fl_Callback*)cb_RaftEnableButton);
    } // Fl_Light_Button* RaftEnableButton
    { Fl_Group* o = new Fl_Group(5, 125, 535, 270, "Base");
      o->box(FL_ENGRAVED_FRAME);
      o->color((Fl_Color)FL_DARK3);
      { RaftMaterialPrDistanceRatioSlider = new Fl_Value_Slider(15, 190, 515, 20, "Material pr. distance ratio");
        RaftMaterialPrDistanceRatioSlider->type(5);
        RaftMaterialPrDistanceRatioSlider->selection_color((Fl_Color)2);
        RaftMaterialPrDistanceRatioSlider->minimum(0.1);
        RaftMaterialPrDistanceRatioSlider->maximum(3);
        RaftMaterialPrDistanceRatioSlider->step(0.1);
        RaftMaterialPrDistanceRatioSlider->value(1.75);
        RaftMaterialPrDistanceRatioSlider->textsize(14);
        RaftMaterialPrDistanceRatioSlider->callback((Fl_Callback*)cb_RaftMaterialPrDistanceRatioSlider);
        RaftMaterialPrDistanceRatioSlider->align(FL_ALIGN_TOP_LEFT);
      } // Fl_Value_Slider* RaftMaterialPrDistanceRatioSlider
      { RaftRotationSlider = new Fl_Value_Slider(15, 230, 515, 20, "Rotation");
        RaftRotationSlider->type(5);
        RaftRotationSlider->selection_color((Fl_Color)2);
        RaftRotationSlider->maximum(360);
        RaftRotationSlider->step(1);
        RaftRotationSlider->value(90);
        RaftRotationSlider->textsize(14);
        RaftRotationSlider->callback((Fl_Callback*)cb_RaftRotationSlider);
        RaftRotationSlider->align(FL_ALIGN_TOP_LEFT);
      } // Fl_Value_Slider* RaftRotationSlider
      { RaftBaseDistanceSlider = new Fl_Value_Slider(15, 275, 515, 20, "Distance between lines");
        RaftBaseDistanceSlider->tooltip("Distance between the base layer\'s lines");
        RaftBaseDistanceSlider->type(5);
        RaftBaseDistanceSlider->selection_color((Fl_Color)2);
        RaftBaseDistanceSlider->minimum(0.1);
        RaftBaseDistanceSlider->maximum(10);
        RaftBaseDistanceSlider->step(0.1);
        RaftBaseDistanceSlider->value(2);
        RaftBaseDistanceSlider->textsize(14);
        RaftBaseDistanceSlider->callback((Fl_Callback*)cb_RaftBaseDistanceSlider);
        RaftBaseDistanceSlider->align(FL_ALIGN_TOP_LEFT);
      } // Fl_Value_Slider* RaftBaseDistanceSlider
      { RaftBaseThicknessSlider = new Fl_Value_Slider(15, 320, 515, 20, "Thinkness ratio");
        RaftBaseThicknessSlider->tooltip("How much thicker or thinner then a normal layer should the raft base be.");
        RaftBaseThicknessSlider->type(5);
        RaftBaseThicknessSlider->selection_color((Fl_Color)2);
        RaftBaseThicknessSlider->minimum(0.1);
        RaftBaseThicknessSlider->maximum(3);
        RaftBaseThicknessSlider->step(0.1);
        RaftBaseThicknessSlider->value(1);
        RaftBaseThicknessSlider->textsize(14);
        RaftBaseThicknessSlider->callback((Fl_Callback*)cb_RaftBaseThicknessSlider);
        RaftBaseThicknessSlider->align(FL_ALIGN_TOP_LEFT);
      } // Fl_Value_Slider* RaftBaseThicknessSlider
      { RaftBaseLayerCountSlider = new Fl_Value_Slider(15, 150, 515, 20, "Number of Base layers");
        RaftBaseLayerCountSlider->type(5);
        RaftBaseLayerCountSlider->selection_color((Fl_Color)2);
        RaftBaseLayerCountSlider->maximum(5);
        RaftBaseLayerCountSlider->step(1);
        RaftBaseLayerCountSlider->value(1);
        RaftBaseLayerCountSlider->textsize(14);
        RaftBaseLayerCountSlider->callback((Fl_Callback*)cb_RaftBaseLayerCountSlider);
        RaftBaseLayerCountSlider->align(FL_ALIGN_TOP_LEFT);
      } // Fl_Value_Slider* RaftBaseLayerCountSlider
      { RaftBaseTemperatureSlider = new Fl_Value_Slider(15, 365, 515, 20, "Temperature ratio");
        RaftBaseTemperatureSlider->tooltip("To make the flow better, you may want to print the base layers hotter then th\
e rest of the print.");
        RaftBaseTemperatureSlider->type(5);
        RaftBaseTemperatureSlider->selection_color((Fl_Color)2);
        RaftBaseTemperatureSlider->minimum(0.1);
        RaftBaseTemperatureSlider->maximum(3);
        RaftBaseTemperatureSlider->value(1.1);
        RaftBaseTemperatureSlider->textsize(14);
        RaftBaseTemperatureSlider->callback((Fl_Callback*)cb_RaftBaseTemperatureSlider);
        RaftBaseTemperatureSlider->align(FL_ALIGN_TOP_LEFT);
      } // Fl_Value_Slider* RaftBaseTemperatureSlider
      o->end();
    } // Fl_Group* o
    { Fl_Group* o = new Fl_Group(5, 55, 535, 55, "Size");
      o->box(FL_ENGRAVED_FRAME);
      o->color((Fl_Color)FL_DARK3);
      { RaftSizeSlider = new Fl_Value_Slider(15, 80, 515, 20, "Larger then objects (mm)");
        RaftSizeSlider->tooltip("How much larget then the print should the Raft be.");
        RaftSizeSlider->type(5);
        RaftSizeSlider->selection_color((Fl_Color)2);
        RaftSizeSlider->minimum(1);
        RaftSizeSlider->maximum(50);
        RaftSizeSlider->value(1.33);
        RaftSizeSlider->textsize(14);
        RaftSizeSlider->callback((Fl_Callback*)cb_RaftSizeSlider);
        RaftSizeSlider->align(FL_ALIGN_TOP_LEFT);
      } // Fl_Value_Slider* RaftSizeSlider
      o->end();
    } // Fl_Group* o
    { Fl_Group* o = new Fl_Group(5, 415, 535, 285, "Interface");
      o->tooltip("This is the layer(s) connecting to the print itself. Unlike the Base layer, t\
hey should be \"nice\" and with a normal extrusion ratio.");
      o->box(FL_ENGRAVED_FRAME);
      o->color((Fl_Color)FL_DARK3);
      { RaftInterfaceMaterialPrDistanceRatioSlider = new Fl_Value_Slider(15, 490, 515, 20, "Material pr. distance ratio");
        RaftInterfaceMaterialPrDistanceRatioSlider->type(5);
        RaftInterfaceMaterialPrDistanceRatioSlider->selection_color((Fl_Color)2);
        RaftInterfaceMaterialPrDistanceRatioSlider->minimum(0.1);
        RaftInterfaceMaterialPrDistanceRatioSlider->maximum(3);
        RaftInterfaceMaterialPrDistanceRatioSlider->step(0.1);
        RaftInterfaceMaterialPrDistanceRatioSlider->value(1);
        RaftInterfaceMaterialPrDistanceRatioSlider->textsize(14);
        RaftInterfaceMaterialPrDistanceRatioSlider->callback((Fl_Callback*)cb_RaftInterfaceMaterialPrDistanceRatioSlider);
        RaftInterfaceMaterialPrDistanceRatioSlider->align(FL_ALIGN_TOP_LEFT);
      } // Fl_Value_Slider* RaftInterfaceMaterialPrDistanceRatioSlider
      { RaftRotationPrLayerSlider = new Fl_Value_Slider(15, 535, 515, 20, "Rotation pr. layer");
        RaftRotationPrLayerSlider->type(5);
        RaftRotationPrLayerSlider->selection_color((Fl_Color)2);
        RaftRotationPrLayerSlider->maximum(360);
        RaftRotationPrLayerSlider->step(1);
        RaftRotationPrLayerSlider->value(90);
        RaftRotationPrLayerSlider->textsize(14);
        RaftRotationPrLayerSlider->callback((Fl_Callback*)cb_RaftRotationPrLayerSlider);
        RaftRotationPrLayerSlider->align(FL_ALIGN_TOP_LEFT);
      } // Fl_Value_Slider* RaftRotationPrLayerSlider
      { RaftInterfaceDistanceSlider = new Fl_Value_Slider(15, 580, 515, 20, "Distance between lines");
        RaftInterfaceDistanceSlider->tooltip("Distance between the base layer\'s lines");
        RaftInterfaceDistanceSlider->type(5);
        RaftInterfaceDistanceSlider->selection_color((Fl_Color)2);
        RaftInterfaceDistanceSlider->minimum(0.1);
        RaftInterfaceDistanceSlider->maximum(10);
        RaftInterfaceDistanceSlider->step(0.1);
        RaftInterfaceDistanceSlider->value(2);
        RaftInterfaceDistanceSlider->textsize(14);
        RaftInterfaceDistanceSlider->callback((Fl_Callback*)cb_RaftInterfaceDistanceSlider);
        RaftInterfaceDistanceSlider->align(FL_ALIGN_TOP_LEFT);
      } // Fl_Value_Slider* RaftInterfaceDistanceSlider
      { RaftInterfaceThicknessSlider = new Fl_Value_Slider(15, 625, 515, 20, "Thinkness ratio");
        RaftInterfaceThicknessSlider->tooltip("How much thicker or thinner then a normal layer should the raft base be.");
        RaftInterfaceThicknessSlider->type(5);
        RaftInterfaceThicknessSlider->selection_color((Fl_Color)2);
        RaftInterfaceThicknessSlider->minimum(0.1);
        RaftInterfaceThicknessSlider->maximum(3);
        RaftInterfaceThicknessSlider->step(0.1);
        RaftInterfaceThicknessSlider->value(1);
        RaftInterfaceThicknessSlider->textsize(14);
        RaftInterfaceThicknessSlider->callback((Fl_Callback*)cb_RaftInterfaceThicknessSlider);
        RaftInterfaceThicknessSlider->align(FL_ALIGN_TOP_LEFT);
      } // Fl_Value_Slider* RaftInterfaceThicknessSlider
      { RaftInterfaceLayerCountSlider = new Fl_Value_Slider(15, 445, 515, 20, "Number of interface layers");
        RaftInterfaceLayerCountSlider->type(5);
        RaftInterfaceLayerCountSlider->selection_color((Fl_Color)2);
        RaftInterfaceLayerCountSlider->maximum(5);
        RaftInterfaceLayerCountSlider->step(1);
        RaftInterfaceLayerCountSlider->value(2);
        RaftInterfaceLayerCountSlider->textsize(14);
        RaftInterfaceLayerCountSlider->callback((Fl_Callback*)cb_RaftInterfaceLayerCountSlider);
        RaftInterfaceLayerCountSlider->align(FL_ALIGN_TOP_LEFT);
      } // Fl_Value_Slider* RaftInterfaceLayerCountSlider
      { RaftInterfaceTemperatureSlider = new Fl_Value_Slider(15, 670, 515, 20, "Temperature ratio");
        RaftInterfaceTemperatureSlider->tooltip("To make the flow better, you may want to print the base layers hotter then th\
e rest of the print.");
        RaftInterfaceTemperatureSlider->type(5);
        RaftInterfaceTemperatureSlider->selection_color((Fl_Color)2);
        RaftInterfaceTemperatureSlider->minimum(0.1);
        RaftInterfaceTemperatureSlider->maximum(3);
        RaftInterfaceTemperatureSlider->value(1.1);
        RaftInterfaceTemperatureSlider->textsize(14);
        RaftInterfaceTemperatureSlider->callback((Fl_Callback*)cb_RaftInterfaceTemperatureSlider);
        RaftInterfaceTemperatureSlider->align(FL_ALIGN_TOP_LEFT);
      } // Fl_Value_Slider* RaftInterfaceTemperatureSlider
      o->end();
    } // Fl_Group* o
    { Fl_Button* o = new Fl_Button(430, 710, 110, 25, "Ok");
      o->callback((Fl_Callback*)cb_Ok1);
    } // Fl_Button* o
    raftSettingsWindow->set_modal();
    raftSettingsWindow->end();
  } // Fl_Double_Window* raftSettingsWindow
  { DisplaySettingsWindow = new Fl_Double_Window(590, 870, "Display Settings");
    DisplaySettingsWindow->user_data((void*)(this));
    { Fl_Group* o = new Fl_Group(0, 25, 575, 305, "STL rendering");
      o->box(FL_ENGRAVED_FRAME);
      o->color((Fl_Color)FL_DARK3);
      { DisplayPolygonsButton = new Fl_Light_Button(40, 35, 230, 20, "Display Polygons");
        DisplayPolygonsButton->value(1);
        DisplayPolygonsButton->selection_color((Fl_Color)FL_GREEN);
        DisplayPolygonsButton->callback((Fl_Callback*)cb_DisplayPolygonsButton);
      } // Fl_Light_Button* DisplayPolygonsButton
      { DisplayWireframeButton = new Fl_Light_Button(315, 35, 165, 20, "Display Wireframe");
        DisplayWireframeButton->selection_color((Fl_Color)FL_GREEN);
        DisplayWireframeButton->callback((Fl_Callback*)cb_DisplayWireframeButton);
      } // Fl_Light_Button* DisplayWireframeButton
      { DisplayNormalsButton = new Fl_Light_Button(45, 160, 225, 20, "Display Normals");
        DisplayNormalsButton->selection_color((Fl_Color)FL_GREEN);
        DisplayNormalsButton->callback((Fl_Callback*)cb_DisplayNormalsButton);
      } // Fl_Light_Button* DisplayNormalsButton
      { DisplayEndpointsButton = new Fl_Light_Button(315, 160, 250, 20, "Display Endpoints");
        DisplayEndpointsButton->selection_color((Fl_Color)FL_GREEN);
        DisplayEndpointsButton->callback((Fl_Callback*)cb_DisplayEndpointsButton);
      } // Fl_Light_Button* DisplayEndpointsButton
      { PolygonValSlider = new Fl_Value_Slider(40, 100, 230, 15, "Val");
        PolygonValSlider->type(5);
        PolygonValSlider->selection_color((Fl_Color)2);
        PolygonValSlider->value(0.5);
        PolygonValSlider->textsize(14);
        PolygonValSlider->callback((Fl_Callback*)cb_PolygonValSlider);
        PolygonValSlider->align(FL_ALIGN_LEFT);
      } // Fl_Value_Slider* PolygonValSlider
      { PolygonSatSlider = new Fl_Value_Slider(40, 80, 230, 20, "Sat");
        PolygonSatSlider->type(5);
        PolygonSatSlider->selection_color((Fl_Color)2);
        PolygonSatSlider->value(0.5);
        PolygonSatSlider->textsize(14);
        PolygonSatSlider->callback((Fl_Callback*)cb_PolygonSatSlider);
        PolygonSatSlider->align(FL_ALIGN_LEFT);
      } // Fl_Value_Slider* PolygonSatSlider
      { PolygonHueSlider = new Fl_Value_Slider(40, 60, 230, 20, "Hue");
        PolygonHueSlider->type(5);
        PolygonHueSlider->selection_color((Fl_Color)2);
        PolygonHueSlider->value(0.5);
        PolygonHueSlider->textsize(14);
        PolygonHueSlider->callback((Fl_Callback*)cb_PolygonHueSlider);
        PolygonHueSlider->align(FL_ALIGN_LEFT);
      } // Fl_Value_Slider* PolygonHueSlider
      { WireframeValSlider = new Fl_Value_Slider(315, 100, 250, 25, "Val");
        WireframeValSlider->type(5);
        WireframeValSlider->selection_color((Fl_Color)2);
        WireframeValSlider->value(0.5);
        WireframeValSlider->textsize(14);
        WireframeValSlider->callback((Fl_Callback*)cb_WireframeValSlider);
        WireframeValSlider->align(FL_ALIGN_LEFT);
      } // Fl_Value_Slider* WireframeValSlider
      { WireframeSatSlider = new Fl_Value_Slider(315, 80, 250, 20, "Sat");
        WireframeSatSlider->type(5);
        WireframeSatSlider->selection_color((Fl_Color)2);
        WireframeSatSlider->value(0.5);
        WireframeSatSlider->textsize(14);
        WireframeSatSlider->callback((Fl_Callback*)cb_WireframeSatSlider);
        WireframeSatSlider->align(FL_ALIGN_LEFT);
      } // Fl_Value_Slider* WireframeSatSlider
      { WireframeHueSlider = new Fl_Value_Slider(315, 60, 250, 20, "Hue");
        WireframeHueSlider->type(5);
        WireframeHueSlider->selection_color((Fl_Color)2);
        WireframeHueSlider->value(0.5);
        WireframeHueSlider->textsize(14);
        WireframeHueSlider->callback((Fl_Callback*)cb_WireframeHueSlider);
        WireframeHueSlider->align(FL_ALIGN_LEFT);
      } // Fl_Value_Slider* WireframeHueSlider
      { NormalValSlider = new Fl_Value_Slider(45, 225, 225, 20, "Val");
        NormalValSlider->type(5);
        NormalValSlider->selection_color((Fl_Color)2);
        NormalValSlider->value(0.5);
        NormalValSlider->textsize(14);
        NormalValSlider->callback((Fl_Callback*)cb_NormalValSlider);
        NormalValSlider->align(FL_ALIGN_LEFT);
      } // Fl_Value_Slider* NormalValSlider
      { NormalSatSlider = new Fl_Value_Slider(45, 205, 225, 20, "Sat");
        NormalSatSlider->type(5);
        NormalSatSlider->selection_color((Fl_Color)2);
        NormalSatSlider->value(0.5);
        NormalSatSlider->textsize(14);
        NormalSatSlider->callback((Fl_Callback*)cb_NormalSatSlider);
        NormalSatSlider->align(FL_ALIGN_LEFT);
      } // Fl_Value_Slider* NormalSatSlider
      { NormalHueSlider = new Fl_Value_Slider(45, 185, 225, 20, "Hue");
        NormalHueSlider->type(5);
        NormalHueSlider->selection_color((Fl_Color)2);
        NormalHueSlider->value(0.5);
        NormalHueSlider->textsize(14);
        NormalHueSlider->callback((Fl_Callback*)cb_NormalHueSlider);
        NormalHueSlider->align(FL_ALIGN_LEFT);
      } // Fl_Value_Slider* NormalHueSlider
      { EndpointValSlider = new Fl_Value_Slider(315, 225, 250, 20, "Val");
        EndpointValSlider->type(5);
        EndpointValSlider->selection_color((Fl_Color)2);
        EndpointValSlider->value(0.5);
        EndpointValSlider->textsize(14);
        EndpointValSlider->callback((Fl_Callback*)cb_EndpointValSlider);
        EndpointValSlider->align(FL_ALIGN_LEFT);
      } // Fl_Value_Slider* EndpointValSlider
      { EndpointSatSlider = new Fl_Value_Slider(315, 205, 250, 20, "Sat");
        EndpointSatSlider->type(5);
        EndpointSatSlider->selection_color((Fl_Color)2);
        EndpointSatSlider->value(0.5);
        EndpointSatSlider->textsize(14);
        EndpointSatSlider->callback((Fl_Callback*)cb_EndpointSatSlider);
        EndpointSatSlider->align(FL_ALIGN_LEFT);
      } // Fl_Value_Slider* EndpointSatSlider
      { EndpointHueSlider = new Fl_Value_Slider(315, 185, 250, 20, "Hue");
        EndpointHueSlider->type(5);
        EndpointHueSlider->selection_color((Fl_Color)2);
        EndpointHueSlider->value(0.5);
        EndpointHueSlider->textsize(14);
        EndpointHueSlider->callback((Fl_Callback*)cb_EndpointHueSlider);
        EndpointHueSlider->align(FL_ALIGN_LEFT);
      } // Fl_Value_Slider* EndpointHueSlider
      { DisplayBBoxButton = new Fl_Light_Button(45, 295, 225, 20, "Display Bounding Box");
        DisplayBBoxButton->selection_color((Fl_Color)FL_GREEN);
        DisplayBBoxButton->callback((Fl_Callback*)cb_DisplayBBoxButton);
      } // Fl_Light_Button* DisplayBBoxButton
      { HighlightSlider = new Fl_Value_Slider(315, 305, 250, 20, "Highlight");
        HighlightSlider->type(5);
        HighlightSlider->selection_color((Fl_Color)2);
        HighlightSlider->value(0.5);
        HighlightSlider->textsize(14);
        HighlightSlider->callback((Fl_Callback*)cb_HighlightSlider);
        HighlightSlider->align(FL_ALIGN_TOP_LEFT);
      } // Fl_Value_Slider* HighlightSlider
      { NormalLengthSlider = new Fl_Value_Slider(45, 265, 225, 25, "Normals length");
        NormalLengthSlider->type(5);
        NormalLengthSlider->selection_color((Fl_Color)2);
        NormalLengthSlider->minimum(1);
        NormalLengthSlider->maximum(30);
        NormalLengthSlider->step(0.1);
        NormalLengthSlider->value(10);
        NormalLengthSlider->textsize(14);
        NormalLengthSlider->callback((Fl_Callback*)cb_NormalLengthSlider);
        NormalLengthSlider->align(FL_ALIGN_TOP_LEFT);
      } // Fl_Value_Slider* NormalLengthSlider
      { EndpointSizeSlider = new Fl_Value_Slider(315, 265, 250, 20, "Endpoints size");
        EndpointSizeSlider->type(5);
        EndpointSizeSlider->selection_color((Fl_Color)2);
        EndpointSizeSlider->minimum(1);
        EndpointSizeSlider->maximum(20);
        EndpointSizeSlider->step(0.1);
        EndpointSizeSlider->value(8);
        EndpointSizeSlider->textsize(14);
        EndpointSizeSlider->callback((Fl_Callback*)cb_EndpointSizeSlider);
        EndpointSizeSlider->align(FL_ALIGN_TOP_LEFT);
      } // Fl_Value_Slider* EndpointSizeSlider
      { DisplayWireframeShadedButton = new Fl_Light_Button(485, 35, 80, 20, "Shaded");
        DisplayWireframeShadedButton->selection_color((Fl_Color)FL_GREEN);
        DisplayWireframeShadedButton->callback((Fl_Callback*)cb_DisplayWireframeShadedButton);
      } // Fl_Light_Button* DisplayWireframeShadedButton
      { PolygonOpasitySlider = new Fl_Value_Slider(40, 120, 230, 25, "Opa");
        PolygonOpasitySlider->type(5);
        PolygonOpasitySlider->selection_color((Fl_Color)2);
        PolygonOpasitySlider->value(0.5);
        PolygonOpasitySlider->textsize(14);
        PolygonOpasitySlider->callback((Fl_Callback*)cb_PolygonOpasitySlider);
        PolygonOpasitySlider->align(FL_ALIGN_LEFT);
      } // Fl_Value_Slider* PolygonOpasitySlider
      o->end();
    } // Fl_Group* o
    { Fl_Group* o = new Fl_Group(0, 345, 575, 175, "GCode rendering");
      o->box(FL_ENGRAVED_FRAME);
      o->color((Fl_Color)FL_DARK3);
      { DisplayGCodeButton = new Fl_Light_Button(50, 355, 195, 20, "Display GCode");
        DisplayGCodeButton->value(1);
        DisplayGCodeButton->selection_color((Fl_Color)FL_GREEN);
        DisplayGCodeButton->callback((Fl_Callback*)cb_DisplayGCodeButton);
      } // Fl_Light_Button* DisplayGCodeButton
      { GCodeExtrudeValSlider = new Fl_Value_Slider(50, 420, 195, 20, "Val");
        GCodeExtrudeValSlider->type(5);
        GCodeExtrudeValSlider->selection_color((Fl_Color)2);
        GCodeExtrudeValSlider->value(0.5);
        GCodeExtrudeValSlider->textsize(14);
        GCodeExtrudeValSlider->callback((Fl_Callback*)cb_GCodeExtrudeValSlider);
        GCodeExtrudeValSlider->align(FL_ALIGN_LEFT);
      } // Fl_Value_Slider* GCodeExtrudeValSlider
      { GCodeExtrudeSatSlider = new Fl_Value_Slider(50, 400, 195, 20, "Sat");
        GCodeExtrudeSatSlider->type(5);
        GCodeExtrudeSatSlider->selection_color((Fl_Color)2);
        GCodeExtrudeSatSlider->value(0.5);
        GCodeExtrudeSatSlider->textsize(14);
        GCodeExtrudeSatSlider->callback((Fl_Callback*)cb_GCodeExtrudeSatSlider);
        GCodeExtrudeSatSlider->align(FL_ALIGN_LEFT);
      } // Fl_Value_Slider* GCodeExtrudeSatSlider
      { GCodeExtrudeHueSlider = new Fl_Value_Slider(50, 380, 195, 20, "Hue");
        GCodeExtrudeHueSlider->type(5);
        GCodeExtrudeHueSlider->selection_color((Fl_Color)2);
        GCodeExtrudeHueSlider->value(0.5);
        GCodeExtrudeHueSlider->textsize(14);
        GCodeExtrudeHueSlider->callback((Fl_Callback*)cb_GCodeExtrudeHueSlider);
        GCodeExtrudeHueSlider->align(FL_ALIGN_LEFT);
      } // Fl_Value_Slider* GCodeExtrudeHueSlider
      { LuminanceShowsSpeedButton = new Fl_Light_Button(315, 355, 260, 20, "Luminance shows speed");
        LuminanceShowsSpeedButton->value(1);
        LuminanceShowsSpeedButton->selection_color((Fl_Color)FL_GREEN);
        LuminanceShowsSpeedButton->callback((Fl_Callback*)cb_LuminanceShowsSpeedButton);
      } // Fl_Light_Button* LuminanceShowsSpeedButton
      { Fl_Button* o = new Fl_Button(50, 495, 220, 20, "Crop Range");
        o->callback((Fl_Callback*)cb_Crop);
      } // Fl_Button* o
      { Fl_Button* o = new Fl_Button(295, 495, 280, 20, "Reset crop range");
        o->callback((Fl_Callback*)cb_Reset);
      } // Fl_Button* o
      { GCodeDrawStartSlider = new Fl_Slider(50, 445, 525, 20, "From");
        GCodeDrawStartSlider->type(5);
        GCodeDrawStartSlider->color((Fl_Color)FL_DARK3);
        GCodeDrawStartSlider->selection_color((Fl_Color)2);
        GCodeDrawStartSlider->callback((Fl_Callback*)cb_GCodeDrawStartSlider);
        GCodeDrawStartSlider->align(FL_ALIGN_LEFT);
      } // Fl_Slider* GCodeDrawStartSlider
      { GCodeDrawEndSlider = new Fl_Slider(50, 470, 525, 20, "To");
        GCodeDrawEndSlider->type(5);
        GCodeDrawEndSlider->color((Fl_Color)FL_DARK3);
        GCodeDrawEndSlider->selection_color((Fl_Color)2);
        GCodeDrawEndSlider->callback((Fl_Callback*)cb_GCodeDrawEndSlider);
        GCodeDrawEndSlider->align(FL_ALIGN_LEFT);
      } // Fl_Slider* GCodeDrawEndSlider
      { GCodeMoveValSlider = new Fl_Value_Slider(315, 420, 260, 20, "Val");
        GCodeMoveValSlider->type(5);
        GCodeMoveValSlider->selection_color((Fl_Color)2);
        GCodeMoveValSlider->value(0.5);
        GCodeMoveValSlider->textsize(14);
        GCodeMoveValSlider->callback((Fl_Callback*)cb_GCodeMoveValSlider);
        GCodeMoveValSlider->align(FL_ALIGN_LEFT);
      } // Fl_Value_Slider* GCodeMoveValSlider
      { GCodeMoveSatSlider = new Fl_Value_Slider(315, 400, 260, 15, "Sat");
        GCodeMoveSatSlider->type(5);
        GCodeMoveSatSlider->selection_color((Fl_Color)2);
        GCodeMoveSatSlider->value(0.5);
        GCodeMoveSatSlider->textsize(14);
        GCodeMoveSatSlider->callback((Fl_Callback*)cb_GCodeMoveSatSlider);
        GCodeMoveSatSlider->align(FL_ALIGN_LEFT);
      } // Fl_Value_Slider* GCodeMoveSatSlider
      { GCodeMoveHueSlider = new Fl_Value_Slider(315, 380, 260, 15, "Hue");
        GCodeMoveHueSlider->type(5);
        GCodeMoveHueSlider->selection_color((Fl_Color)2);
        GCodeMoveHueSlider->value(0.5);
        GCodeMoveHueSlider->textsize(14);
        GCodeMoveHueSlider->callback((Fl_Callback*)cb_GCodeMoveHueSlider);
        GCodeMoveHueSlider->align(FL_ALIGN_LEFT);
      } // Fl_Value_Slider* GCodeMoveHueSlider
      o->end();
    } // Fl_Group* o
    { Fl_Group* o = new Fl_Group(0, 540, 575, 120, "Layer preview");
      o->box(FL_ENGRAVED_FRAME);
      o->color((Fl_Color)FL_DARK3);
      { CuttingPlaneValueSlider = new Fl_Value_Slider(15, 585, 560, 20, "Cuttingplane");
        CuttingPlaneValueSlider->type(5);
        CuttingPlaneValueSlider->value(0.5);
        CuttingPlaneValueSlider->textsize(14);
        CuttingPlaneValueSlider->callback((Fl_Callback*)cb_CuttingPlaneValueSlider);
        CuttingPlaneValueSlider->align(FL_ALIGN_TOP_LEFT);
      } // Fl_Value_Slider* CuttingPlaneValueSlider
      { DisplayCuttingPlaneButton = new Fl_Light_Button(15, 545, 165, 25, "Display CuttingPlane");
        DisplayCuttingPlaneButton->selection_color((Fl_Color)FL_GREEN);
        DisplayCuttingPlaneButton->callback((Fl_Callback*)cb_DisplayCuttingPlaneButton);
      } // Fl_Light_Button* DisplayCuttingPlaneButton
      { DisplayinFillButton = new Fl_Light_Button(190, 545, 170, 25, "Display inFill");
        DisplayinFillButton->selection_color((Fl_Color)FL_GREEN);
        DisplayinFillButton->callback((Fl_Callback*)cb_DisplayinFillButton);
      } // Fl_Light_Button* DisplayinFillButton
      { DisplayAllLayersButton = new Fl_Light_Button(370, 545, 205, 25, "Display all layers");
        DisplayAllLayersButton->selection_color((Fl_Color)FL_GREEN);
        DisplayAllLayersButton->callback((Fl_Callback*)cb_DisplayAllLayersButton);
      } // Fl_Light_Button* DisplayAllLayersButton
      { DrawCuttingPlaneVertexNumbersButton = new Fl_Light_Button(15, 615, 160, 25, "Vertex numbers");
        DrawCuttingPlaneVertexNumbersButton->selection_color((Fl_Color)FL_GREEN);
        DrawCuttingPlaneVertexNumbersButton->callback((Fl_Callback*)cb_DrawCuttingPlaneVertexNumbersButton);
      } // Fl_Light_Button* DrawCuttingPlaneVertexNumbersButton
      { DrawCuttingPlaneLineNumbersButton = new Fl_Light_Button(185, 615, 175, 25, "Line numbers");
        DrawCuttingPlaneLineNumbersButton->selection_color((Fl_Color)FL_GREEN);
        DrawCuttingPlaneLineNumbersButton->callback((Fl_Callback*)cb_DrawCuttingPlaneLineNumbersButton);
      } // Fl_Light_Button* DrawCuttingPlaneLineNumbersButton
      { DrawCuttingPlanePolygonNumbersButton = new Fl_Light_Button(370, 615, 205, 25, "Polygon numbers");
        DrawCuttingPlanePolygonNumbersButton->selection_color((Fl_Color)FL_GREEN);
        DrawCuttingPlanePolygonNumbersButton->callback((Fl_Callback*)cb_DrawCuttingPlanePolygonNumbersButton);
      } // Fl_Light_Button* DrawCuttingPlanePolygonNumbersButton
      o->end();
    } // Fl_Group* o
    { Fl_Group* o = new Fl_Group(0, 675, 575, 60, "Lights");
      o->box(FL_ENGRAVED_FRAME);
      o->color((Fl_Color)FL_DARK3);
      { Fl_Light_Button* o = new Fl_Light_Button(15, 685, 120, 20, "Enable light 0");
        o->value(1);
        o->selection_color((Fl_Color)FL_GREEN);
        o->callback((Fl_Callback*)cb_Enable);
      } // Fl_Light_Button* o
      { Fl_Light_Button* o = new Fl_Light_Button(15, 705, 120, 20, "Enable light 1");
        o->selection_color((Fl_Color)FL_GREEN);
        o->callback((Fl_Callback*)cb_Enable1);
      } // Fl_Light_Button* o
      { Fl_Light_Button* o = new Fl_Light_Button(290, 680, 140, 15, "Enable light 2");
        o->selection_color((Fl_Color)FL_GREEN);
        o->callback((Fl_Callback*)cb_Enable2);
      } // Fl_Light_Button* o
      { Fl_Light_Button* o = new Fl_Light_Button(290, 705, 140, 20, "Enable light 3");
        o->value(1);
        o->selection_color((Fl_Color)FL_GREEN);
        o->callback((Fl_Callback*)cb_Enable3);
      } // Fl_Light_Button* o
      { Fl_Button* o = new Fl_Button(140, 680, 0, 20, "Edit light 0");
        o->deactivate();
      } // Fl_Button* o
      { Fl_Button* o = new Fl_Button(140, 705, 0, 20, "Edit light 1");
        o->deactivate();
      } // Fl_Button* o
      { Fl_Button* o = new Fl_Button(440, 680, 135, 25, "Edit light 2");
        o->deactivate();
      } // Fl_Button* o
      { Fl_Button* o = new Fl_Button(440, 705, 135, 20, "Edit light 3");
        o->deactivate();
      } // Fl_Button* o
      o->end();
    } // Fl_Group* o
    { DisplayDebuginFillButton = new Fl_Light_Button(85, 780, 160, 25, "Display Debug inFill");
      DisplayDebuginFillButton->selection_color((Fl_Color)FL_GREEN);
      DisplayDebuginFillButton->callback((Fl_Callback*)cb_DisplayDebuginFillButton);
    } // Fl_Light_Button* DisplayDebuginFillButton
    { DisplayDebugButton = new Fl_Light_Button(10, 780, 70, 25, "Debug");
      DisplayDebugButton->selection_color((Fl_Color)FL_GREEN);
      DisplayDebugButton->callback((Fl_Callback*)cb_DisplayDebugButton);
    } // Fl_Light_Button* DisplayDebugButton
    { DrawVertexNumbersButton = new Fl_Light_Button(250, 780, 170, 25, "Draw vertex numbers");
      DrawVertexNumbersButton->selection_color((Fl_Color)FL_GREEN);
      DrawVertexNumbersButton->callback((Fl_Callback*)cb_DrawVertexNumbersButton);
    } // Fl_Light_Button* DrawVertexNumbersButton
    { DrawLineNumbersButton = new Fl_Light_Button(425, 780, 160, 25, "Draw line numbers");
      DrawLineNumbersButton->selection_color((Fl_Color)FL_GREEN);
      DrawLineNumbersButton->callback((Fl_Callback*)cb_DrawLineNumbersButton);
    } // Fl_Light_Button* DrawLineNumbersButton
    { ExamineSlider = new Fl_Value_Slider(85, 755, 500, 20, "Examine");
      ExamineSlider->type(1);
      ExamineSlider->step(0.001);
      ExamineSlider->value(0.098);
      ExamineSlider->textsize(14);
      ExamineSlider->callback((Fl_Callback*)cb_ExamineSlider);
      ExamineSlider->align(FL_ALIGN_LEFT);
    } // Fl_Value_Slider* ExamineSlider
    { Fl_Button* o = new Fl_Button(195, 825, 210, 40, "Ok");
      o->callback((Fl_Callback*)cb_Ok2);
    } // Fl_Button* o
    DisplaySettingsWindow->set_modal();
    DisplaySettingsWindow->end();
  } // Fl_Double_Window* DisplaySettingsWindow
  { CustomButtonsSetupWindow = new Fl_Double_Window(455, 410, "CustomButtonsSetup");
    CustomButtonsSetupWindow->user_data((void*)(this));
    { Fl_Text_Editor* o = CustomButtonText = new Fl_Text_Editor(25, 135, 410, 260, "Button gcode");
      CustomButtonText->selection_color((Fl_Color)31);
      Fl_Text_Buffer *CustomButtonBuffer = new Fl_Text_Buffer();
      o->buffer(CustomButtonBuffer);
    } // Fl_Text_Editor* CustomButtonText
    { CustomButtonLabel = new Fl_Input(115, 82, 320, 28, "Button label:");
    } // Fl_Input* CustomButtonLabel
    { CustomButtonSelectorSlider = new Fl_Value_Slider(155, 10, 240, 25, "Button to edit:");
      CustomButtonSelectorSlider->type(1);
      CustomButtonSelectorSlider->minimum(1);
      CustomButtonSelectorSlider->maximum(20);
      CustomButtonSelectorSlider->step(1);
      CustomButtonSelectorSlider->value(1);
      CustomButtonSelectorSlider->textsize(14);
      CustomButtonSelectorSlider->callback((Fl_Callback*)cb_CustomButtonSelectorSlider);
      CustomButtonSelectorSlider->align(FL_ALIGN_LEFT);
    } // Fl_Value_Slider* CustomButtonSelectorSlider
    { Fl_Button* o = new Fl_Button(70, 40, 120, 25, "Test");
      o->callback((Fl_Callback*)cb_Test);
    } // Fl_Button* o
    { Fl_Button* o = new Fl_Button(200, 40, 150, 25, "Save");
      o->callback((Fl_Callback*)cb_Save3);
    } // Fl_Button* o
    CustomButtonsSetupWindow->end();
  } // Fl_Double_Window* CustomButtonsSetupWindow
  { PrinterCalibrationWindow = new Fl_Double_Window(375, 315, "PrinterCalibrationWindow");
    PrinterCalibrationWindow->user_data((void*)(this));
    { Fl_Group* o = new Fl_Group(20, 51, 335, 187, "Calibrate");
      o->box(FL_ENGRAVED_FRAME);
      o->color((Fl_Color)FL_DARK3);
      { Fl_Text_Display* o = new Fl_Text_Display(21, 123, 44, 25, "Makes a test print with various settings. (Disabled)");
        o->box(FL_NO_BOX);
        o->labelsize(10);
        o->align(FL_ALIGN_RIGHT);
      } // Fl_Text_Display* o
      { Fl_Button* o = new Fl_Button(105, 167, 154, 38, "Calibration Print");
        o->box(FL_NO_BOX);
        o->color((Fl_Color)41);
        o->labelfont(2);
      } // Fl_Button* o
      o->end();
    } // Fl_Group* o
    { Fl_Button* o = new Fl_Button(120, 252, 125, 28, "Ok");
      o->callback((Fl_Callback*)cb_Ok3);
    } // Fl_Button* o
    PrinterCalibrationWindow->end();
  } // Fl_Double_Window* PrinterCalibrationWindow
}

void GUI::show(int argc, char **argv) {
  Fl::visual( FL_DOUBLE | FL_RGB);
Fl::scheme("plastic");
MVC->ReadStl("C:/code/printed-parts/z-tensioner_1off.stl");
#ifndef ENABLE_LUA
    Tabs->remove(LuaTab->parent());
#endif
mainWindow->show(argc, argv);
MVC->init();
MVC->CopySettingsToGUI();
MVC->draw();
MVC->redraw();
MVC->redraw();
}
extern GUI* gui;

void TempReadTimer(void *) {
  if(gui->TempReadingEnabledButton->value())
{
if(!gui->MVC->serial->isPrinting())
{
gui->MVC->serial->SendNow("M105");
}
Fl::repeat_timeout(gui->MVC->gui ? gui->MVC->ProcessControl.TempUpdateSpeed: 10.0f , &TempReadTimer);
}
}
