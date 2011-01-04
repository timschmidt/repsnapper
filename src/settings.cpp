#include "config.h"
#include <gtkmm.h>
#include "settings.h"
#include <libconfig.h++>

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

  FLOAT_MEMBER (Raft.Size, "RaftSize", 1.33),
#define FLOAT_PHASE_MEMBER(phase, phasestd, member, def_value) \
  { OFFSET (Raft.Phase[Settings::RaftSettings::PHASE_##phase].member), T_FLOAT, \
    #phasestd #member, #phasestd #member, def_value, }

  // Raft Base
  { OFFSET (Raft.Phase[Settings::RaftSettings::PHASE_BASE].LayerCount), T_INT,
    "BaseLayerCount", "BaseLayerCount", 1, },
  FLOAT_PHASE_MEMBER(BASE, Base, MaterialDistanceRatio, 1.8),
  FLOAT_PHASE_MEMBER(BASE, Base, Rotation, 90.0),
  FLOAT_PHASE_MEMBER(BASE, Base, Distance, 2.0),
  FLOAT_PHASE_MEMBER(BASE, Base, Thickness, 1.0),
  FLOAT_PHASE_MEMBER(BASE, Base, Temperature, 1.10),

  // Raft Interface
  { OFFSET (Raft.Phase[Settings::RaftSettings::PHASE_INTERFACE].LayerCount), T_INT,
    "InterfaceLayerCount", "InterfaceLayerCount", 2, },
  FLOAT_PHASE_MEMBER(INTERFACE, Interface, MaterialDistanceRatio, 1.0),
  FLOAT_PHASE_MEMBER(INTERFACE, Interface, Rotation, 90.0),
  FLOAT_PHASE_MEMBER(INTERFACE, Interface, Distance, 2.0),
  FLOAT_PHASE_MEMBER(INTERFACE, Interface, Thickness, 1.0),
  FLOAT_PHASE_MEMBER(INTERFACE, Interface, Temperature, 1.0),

#undef FLOAT_PHASE_MEMBER

  // Hardware
  FLOAT_MEMBER (Hardware.MinPrintSpeedXY, "MinPrintSpeedXY", 1000),
  FLOAT_MEMBER (Hardware.MinPrintSpeedXY, "MaxPrintSpeedXY", 4000),
  FLOAT_MEMBER (Hardware.MinPrintSpeedZ,  "MinPrintSpeedZ",  50),
  FLOAT_MEMBER (Hardware.MinPrintSpeedZ,  "MaxPrintSpeedZ",  150),

  FLOAT_MEMBER (Hardware.DistanceToReachFullSpeed, "DistanceToReachFullSpeed", 1.5),
  FLOAT_MEMBER (Hardware.ExtrusionFactor, "ExtrusionFactor", 1.0),
  FLOAT_MEMBER (Hardware.LayerThickness, "LayerThickness", 0.4),

  // FIXME: Volume, PrintMargin etc. - needs splitting up ...
  //  m_fVolume = Vector3f(200,200,140);
  //  PrintMargin = Vector3f(10,10,0);

  FLOAT_MEMBER  (Hardware.ExtrudedMaterialWidth, "ExtrudedMaterialWidth", 0.7),
  STRING_MEMBER (Hardware.PortName, "msPortName", DEFAULT_COM_PORT),
  INT_MEMBER    (Hardware.SerialSpeed, "miSerialSpeed", 57600),
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

Settings::Settings (Gtk::Builder *builder)
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
  // FIXME: Set the start / end / layer text buffer contents ...
}

void Settings::load_settings (std::string filename)
{
  libconfig::Config cfg;

  set_defaults();

  try {
    cfg.readFile (filename.c_str());
    std::cout << "parsing config from '" << filename << "\n";

    for (uint i = 0; i < G_N_ELEMENTS (settings); i++) {
      const char *name = settings[i].config_name;
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
    std::cout << "Failed to load settings from file '" << filename
	      << "' - error '" << e.what() << "\n";
  }

  // debug !
  save_settings ("/dev/stdout");
}

void Settings::save_settings (std::string filename)
{
  libconfig::Config   cfg;
  libconfig::Setting &root = cfg.getRoot();

  for (uint i = 0; i < G_N_ELEMENTS (settings); i++) {
    libconfig::Setting::Type t;
    const char *name = settings[i].config_name;

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
}
