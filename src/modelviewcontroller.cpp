/* -------------------------------------------------------- *
*
* modelviewcontroller.cpp
*
* Copyright 2009+ Michael Holm - www.kulitorum.com
*
* This file is part of RepSnapper and is made available under
* the terms of the GNU General Public License, version 2, or at your
* option, any later version, incorporated herein by reference.
*
* ------------------------------------------------------------------------- */
#include "stdafx.h"
#include <vector>
#include <string>
#include "modelviewcontroller.h"
#include "RFO.h"

#ifndef WIN32

/**
 * C++ version 0.4 char* style "itoa":
 * Written by Lukás Chmela
 * Released under GPLv3.
 */
char* itoa(int value, char* result, int base) {
	// check that the base if valid
	if (base < 2 || base > 36) { *result = '\0'; return result; }

	char* ptr = result, *ptr1 = result, tmp_char;
	int tmp_value;

	do {
		tmp_value = value;
		value /= base;
		*ptr++ = "zyxwvutsrqponmlkjihgfedcba9876543210123456789abcdefghijklmnopqrstuvwxyz" [35 + (tmp_value - value * base)];
	} while ( value );

	// Apply negative sign
	if (tmp_value < 0) *ptr++ = '-';
	*ptr-- = '\0';
	while(ptr1 < ptr) {
		tmp_char = *ptr;
		*ptr--= *ptr1;
		*ptr1++ = tmp_char;
	}
	return result;
}

#endif


void tree_callback( Fl_Widget* w, void *_gui )
{
	Flu_Tree_Browser *t = (Flu_Tree_Browser*)w;
	int reason = t->callback_reason();
	GUI *gui = (GUI *)_gui;
	
	Flu_Tree_Browser::Node *n = t->callback_node();

	Matrix4f &transform = gui->MVC->SelectedNodeMatrix();
	Vector3f translate = transform.getTranslation();
	RFO_Object *selectedObject=0;
	RFO_File *selectedFile=0;
	gui->MVC->GetSelectedRFO(&selectedObject, &selectedFile);

	switch( reason )
	{
	case FLU_HILIGHTED:
		printf( "%s hilighted\n", n->label() );
		break;

	case FLU_UNHILIGHTED:
		printf( "%s unhilighted\n", n->label() );
		break;

	case FLU_SELECTED:
		gui->TranslateX->value(translate.x);
		gui->TranslateY->value(translate.y);
		gui->TranslateZ->value(translate.z);

		if(selectedObject)
			gui->ObjectNameInput->value(selectedObject->name.c_str());
		else
			gui->ObjectNameInput->value("no selection");
		if(selectedFile)
		{
			gui->FileLocationInput->value(selectedFile->location.c_str());
			gui->FileTypeInput->value(selectedFile->filetype.c_str());
			gui->FileMaterialInput->value(selectedFile->material.c_str());
		}
		else
		{
			gui->FileLocationInput->value("no file selected");
			gui->FileTypeInput->value("no file selected");
			gui->FileMaterialInput->value("no file selected");
		}

		printf( "%s selected\n", n->label() );
		//transform
		break;

	case FLU_UNSELECTED:
		printf( "%s unselected\n", n->label() );
		break;

	case FLU_OPENED:
		printf( "%s opened\n", n->label() );
		break;

	case FLU_CLOSED:
		printf( "%s closed\n", n->label() );
		break;

	case FLU_DOUBLE_CLICK:
		printf( "%s double-clicked\n", n->label() );
		break;

	case FLU_WIDGET_CALLBACK:
		printf( "%s widget callback\n", n->label() );
		break;

	case FLU_MOVED_NODE:
		printf( "%s moved\n", n->label() );
		break;

	case FLU_NEW_NODE:
		printf( "node '%s' added to the tree\n", n->label() );
		break;
	}
	gui->MVC->redraw();
}

ModelViewController::~ModelViewController()
{
	delete serial;
}

ModelViewController::ModelViewController(int x,int y,int w,int h,const char *l) : Fl_Gl_Window(x,y,w,h,l)
{
	serial = new RepRapSerial();

	gui = 0;
	zoom = 100.0f;

	ArcBall = new ArcBallT((GLfloat)w, (GLfloat)h);				                // NEW: ArcBall Instance

	Transform.M[0] = 1.0f;	Transform.M[1] = 0.0f; Transform.M[2] = 0.0f;Transform.M[3] = 0.0f,				// NEW: Final Transform
	Transform.M[4] = 0.0f;	Transform.M[5] = 1.0f; Transform.M[6] = 0.0f;Transform.M[7] = 0.0f,				// NEW: Final Transform
	Transform.M[8] = 0.0f;	Transform.M[9] = 0.0f; Transform.M[10] = 1.0f;Transform.M[11] = 0.0f,				// NEW: Final Transform
	Transform.M[12] = 0.0f;	Transform.M[13] = 0.0f; Transform.M[14] = 0.0f;Transform.M[15] = 1.0f,				// NEW: Final Transform

	LastRot.M[0]=1.0f;LastRot.M[1]=0.0f;LastRot.M[2]=0.0f;					// NEW: Last Rotation
	LastRot.M[3]=0.0f;LastRot.M[4]=1.0f;LastRot.M[5]=0.0f;					// NEW: Last Rotation
	LastRot.M[6]=0.0f;LastRot.M[7]=0.0f;LastRot.M[8]=1.0f;					// NEW: Last Rotation
	ThisRot.M[0]=1.0f;ThisRot.M[1]=0.0f;ThisRot.M[2]=0.0f;					// NEW: Last Rotation
	ThisRot.M[3]=0.0f;ThisRot.M[4]=1.0f;ThisRot.M[5]=0.0f;					// NEW: Last Rotation
	ThisRot.M[6]=0.0f;ThisRot.M[7]=0.0f;ThisRot.M[8]=1.0f;					// NEW: Last Rotation

	m_bExtruderDirection = true;
	m_iExtruderSpeed = 3000;
	m_iExtruderLength = 150;
	m_fTargetTemp = 63.0f;
}

void ModelViewController::Init(GUI *_gui)
{
	gui = _gui;
	DetectComPorts (true);
	ProcessControl.LoadXML();
	serial->SetReceivingBufferSize(ProcessControl.ReceivingBufferSize);
	serial->SetValidateConnection(ProcessControl.m_bValidateConnection);
	CopySettingsToGUI();
	Fl::add_timeout(0.25, Static_Timer_CB, (void*)this);
}

void ModelViewController::Static_Timer_CB(void *userdata) {
    ModelViewController *o = (ModelViewController*)userdata;
    o->Timer_CB();
    Fl::repeat_timeout(0.25, Static_Timer_CB, userdata);
}

/* Called every 250ms (0.25 of a second) */
void ModelViewController::Timer_CB()
{
	if( !serial->isConnected() && gui->printerSettingsWindow->visible() != 0 )
	{
	  static uint count = 0;
	  if ((count++ % 4) == 0) /* every second */
		DetectComPorts();
	}
	if( gui->Tabs->value() == gui->PrintTab )
	{
		gui->PrintTab->redraw();
	}
}

void ModelViewController::resize(int x,int y, int width, int height)					// Reshape The Window When It's Moved Or Resized
{
	Fl_Gl_Window::resize(x,y,width,height);
}

void ModelViewController::DetectComPorts(bool init)
{
	bool bDirty = init;
	vector<std::string> currentComports;

#ifdef WIN32
	int highestCom = 0;
	for(int i = 1; i <=9 ; i++ )
	{
	        TCHAR strPort[32] = {0};
	        _stprintf(strPort, _T("COM%d"), i);

	        DWORD dwSize = 0;
	        LPCOMMCONFIG lpCC = (LPCOMMCONFIG) new BYTE[1];
	        GetDefaultCommConfig(strPort, lpCC, &dwSize);
		int r = GetLastError();
	        delete [] lpCC;

	        lpCC = (LPCOMMCONFIG) new BYTE[dwSize];
	        GetDefaultCommConfig(strPort, lpCC, &dwSize);
	        delete [] lpCC;

		if( r != 87 )
		{
			ToolkitLock guard;

			highestCom = i;
			if( this ) // oups extremely ugly, should move this code to a static method and a callback
			{
				const_cast<Fl_Menu_Item*>(gui->portInput->menu())[i-1].activate();
				const_cast<Fl_Menu_Item*>(gui->portInputSimple->menu())[i-1].activate();
			}
		}
		else
		{
			ToolkitLock guard;

			if( this ) // oups extremely ugly, should move this code to a static method and a callback
			{
				const_cast<Fl_Menu_Item*>(gui->portInput->menu())[i-1].deactivate();
				const_cast<Fl_Menu_Item*>(gui->portInputSimple->menu())[i-1].deactivate();
			}
		}
	}
	currentComports.push_back(string("COM"+highestCom));

#elif defined(__APPLE__)
	const char *ttyPattern = "tty.";

#else // Linux
	const char *ttyPattern = "ttyUSB";
#endif

#ifndef WIN32
	DIR *d = opendir ("/dev");
	if (d) { // failed
		struct	dirent *e;
		while ((e = readdir (d))) {
			//fprintf (stderr, "name '%s'\n", e->d_name);
			if (strstr(e->d_name,ttyPattern)) {
				string device = string("/dev/") + e->d_name;
				currentComports.push_back(device);
			}
		}
		closedir(d);

		if ( currentComports.size() != this->comports.size() )
			bDirty = true;
	}

	if ( bDirty && gui) {
		ToolkitLock guard;

		static Fl_Menu_Item emptyList[] = {
			{0,0,0,0,0,0,0,0,0}
		};

		bool bWasEmpty = !comports.size();

		gui->portInputSimple->menu(emptyList);
		gui->portInput->menu(emptyList);
		comports.clear();

		for (size_t indx = 0; indx < currentComports.size(); ++indx) {
			string menuLabel = string(currentComports[indx]);
			gui->portInput->add(strdup(menuLabel.c_str()));
			gui->portInputSimple->add(strdup(menuLabel.c_str()));
			comports.push_back(currentComports[indx]);
		}

		// auto-select a new com-port
		if (bWasEmpty && comports.size()) {
			ProcessControl.m_sPortName = ValidateComPort(comports[0]);
			CopySettingsToGUI();
		}
	}
#endif
}

string ModelViewController::ValidateComPort (const string &port)
{
	DetectComPorts();

	// is it a valid port ?
	for (uint i = 0; i < comports.size(); i++) {
		if (port == comports[i])
			return port;
	}
	       
	if (comports.size())
		return comports[0];
	else
		return "No ports found";
}

void ModelViewController::setSerialSpeed(int s )
{
	ProcessControl.m_iSerialSpeed = s;
	CopySettingsToGUI();
}

void ModelViewController::setPort(string s)
{
	ProcessControl.m_sPortName = s;
	CopySettingsToGUI();
}

void ModelViewController::SetValidateConnection(bool validate)
{
	ProcessControl.m_bValidateConnection = validate;
	serial->SetValidateConnection(ProcessControl.m_bValidateConnection);
	CopySettingsToGUI();
}

void ModelViewController::CenterView()
{
	glTranslatef(-ProcessControl.Center.x-ProcessControl.printOffset.x, -ProcessControl.Center.y-ProcessControl.printOffset.y, -ProcessControl.Center.z);
}

void ModelViewController::draw()
{
    if (!valid())
		{
		gui->RFP_Browser->callback( &tree_callback, gui );
		gui->RFP_Browser->redraw();

		glLoadIdentity();
		glViewport (0, 0, w(),h());											// Reset The Current Viewport
		glMatrixMode (GL_PROJECTION);										// Select The Projection Matrix
		glLoadIdentity ();													// Reset The Projection Matrix
		gluPerspective (45.0f, (float)w()/(float)h(),1.0f, 1000000.0f);						// Calculate The Aspect Ratio Of The Window
		glMatrixMode (GL_MODELVIEW);										// Select The Modelview Matrix
		glLoadIdentity ();													// Reset The Modelview Matrix
		ArcBall->setBounds((GLfloat)w(), (GLfloat)h());                 //*NEW* Update mouse bounds for arcball
		glEnable(GL_LIGHTING);
		for(int i=0;i<4;i++)
		{
			lights[i].Init ( (GLenum)(GL_LIGHT0+i) );
			//		lights[i].SetPosition ( 0, 0, 0, 0 );
			lights[i].SetAmbientColor ( 0.2f, 0.2f, 0.2f, 1.0f );
			lights[i].SetDiffuseColor ( 1.0f, 1.0f, 1.0f, 1.0f );
			lights[i].SetSpecular ( 1.0f, 1.0f, 1.0f, 1.0f );
			lights[i].SetValues ( );
			lights[i].TurnOff ( );
		}

		lights[0].SetPosition ( -100, 100, 200, 0 );
		lights[1].SetPosition ( 100, 100, 200, 0 );
		lights[2].SetPosition ( 100, -100, 200, 0 );
		lights[3].SetPosition ( 100, -100, 200, 0 );

		for(int i=0;i<4;i++)
			lights[i].SetValues ( );

		lights[0].TurnOn();
		lights[3].TurnOn();

/*
		float params[4];

		for(int i=0;i<8;i++)
		{
			GLenum lightNr = (GLenum)(GL_LIGHT0+i);
			cout << " Light" << i << "\n";
			glGetLightfv( (GLenum)lightNr, GL_DIFFUSE, &params[0] );
			cout << "GL_DIFFUSE " << params[0] << " " << params[1] << " " << params[2];
			glGetLightfv( (GLenum)lightNr, GL_POSITION, &params[0] );
			cout << "GL_POSITION " << params[0] << " " << params[1] << " " << params[2];
			cout << "\n";
		}
*/

		// enable lighting
		glDisable ( GL_LIGHTING);

		glDepthFunc (GL_LEQUAL);										// The Type Of Depth Testing (Less Or Equal)
		glEnable (GL_DEPTH_TEST);										// Enable Depth Testing
		glHint (GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);				// Set Perspective Calculations To Most Accurate

		quadratic=gluNewQuadric();										// Create A Pointer To The Quadric Object
		gluQuadricNormals(quadratic, GLU_SMOOTH);						// Create Smooth Normals
		gluQuadricTexture(quadratic, GL_TRUE);							// Create Texture Coords
	}
//    glEnable(GL_BLEND);
//    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);				// Clear Screen And Depth Buffer
	glLoadIdentity();												// Reset The Current Modelview Matrix
	glTranslatef(0.0f,0.0f,-zoom*2);								// Move Left 1.5 Units And Into The Screen 6.0

	glMultMatrixf(Transform.M);										// NEW: Apply Dynamic Transform
	CenterView();

    glPushMatrix();													// NEW: Prepare Dynamic Transform
	glColor3f(0.75f,0.75f,1.0f);


	/*--------------- Draw Grid and Axis ---------------*/

	DrawGridAndAxis();

	/*--------------- Draw models ------------------*/

	glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, 1);

	Flu_Tree_Browser::Node *node = gui->RFP_Browser->get_selected( 1 );
	ProcessControl.Draw(node);

	/*--------------- Exit -----------------*/
	glPopMatrix();													// NEW: Unapply Dynamic Transform
	glFlush ();														// Flush The GL Rendering Pipeline
//	swap_buffers();
}

int ModelViewController::handle(int event)
{
	Vector2f    Clickpoint;												// NEW: Current Mouse Point
	static float Click_y;
	static float old_zoom;

	Clickpoint.x =  (GLfloat)Fl::event_x();;
	Clickpoint.y =  (GLfloat)Fl::event_y();;

	switch(event) {
		case FL_PUSH:	//mouse down event position in Fl::event_x() and Fl::event_y()
			{
				switch(Fl::event_button())	{
				case FL_LEFT_MOUSE:
					MousePt.T[0] = (GLfloat)Clickpoint.x;
					MousePt.T[1] = (GLfloat)Clickpoint.y;
					ArcBall->click(&MousePt);								// Update Start Vector And Prepare For Dragging
					break;
				case FL_MIDDLE_MOUSE:
					downPoint = Clickpoint;
					/*
					Matrix3fSetIdentity(&LastRot);								// Reset Rotation
					Matrix3fSetIdentity(&ThisRot);								// Reset Rotation
					Matrix4fSetRotationFromMatrix3f(&Transform, &ThisRot);		// Reset Rotation
					*/
					break;
				case FL_RIGHT_MOUSE:
					Click_y = Clickpoint.y;
					old_zoom = zoom;
					break;
				}
				LastRot = ThisRot;										// Set Last Static Rotation To Last Dynamic One
				redraw();
				return 1;
			}
		case FL_DRAG:	//mouse moved while down event ...
			switch(Fl::event_button())
				{
				case FL_LEFT_MOUSE:
					Quat4fT     ThisQuat;

					MousePt.T[0] = Clickpoint.x;
					MousePt.T[1] = Clickpoint.y;

					ArcBall->drag(&MousePt, &ThisQuat);						// Update End Vector And Get Rotation As Quaternion
					Matrix3fSetRotationFromQuat4f(&ThisRot, &ThisQuat);		// Convert Quaternion Into Matrix3fT
					Matrix3fMulMatrix3f(&ThisRot, &LastRot);				// Accumulate Last Rotation Into This One
					Matrix4fSetRotationFromMatrix3f(&Transform, &ThisRot);	// Set Our Final Transform's Rotation From This One
					redraw();
					break;
				case FL_MIDDLE_MOUSE:
					{
					Vector2f    dragp = Clickpoint;
					Vector2f delta = downPoint-dragp;
					Matrix4f matrix;
					memcpy(&matrix.m00, &Transform.M[0], sizeof(Matrix4f));
					Vector3f X(delta.x,0,0);
					X = matrix * X;
					Vector3f Y(0,-delta.y,0);
					Y = matrix * Y;
					ProcessControl.Center += X*delta.length()*0.01f;
					ProcessControl.Center += Y*delta.length()*0.01f;
					redraw();
					downPoint=Clickpoint;
					}

					break;
				case FL_RIGHT_MOUSE:
					float y = 	Click_y - Clickpoint.y;
					zoom = old_zoom + y*0.1;
					redraw();
					break;
				}
			return 1;
		case FL_RELEASE: //mouse up event ...
			MousePt.T[0] = (GLfloat)Fl::event_x();
			MousePt.T[1] = (GLfloat)Fl::event_y();
			redraw();
			return 1;
		case FL_MOUSEWHEEL: { //mouse scroll event
			int mwscrolled = Fl::event_dy();
			zoom += mwscrolled*1;
			redraw();
			return 1;
			}
		case FL_FOCUS :
		case FL_UNFOCUS : // Return 1 if you want keyboard events, 0 otherwise
			return 0;
		case FL_KEYBOARD: //keypress, key is in Fl::event_key(), ascii in Fl::event_text() .. Return 1 if you understand/use the keyboard event, 0 otherwise...
			return 1;
		case FL_SHORTCUT: // shortcut, key is in Fl::event_key(), ascii in Fl::event_text() ... Return 1 if you understand/use the shortcut event, 0 otherwise...
			return 1;
		case FL_CLOSE:
			if (fl_choice("Save settings ?", "Exit", "Save then exit", NULL))
			{
//				int a=0;
//				ProcessControl.SaveXML();
			}
			break;
		default:
			break;
	}
	// pass other events to the base class...
	return Fl_Gl_Window::handle(event);
}

void ModelViewController::DrawGridAndAxis()
{
	//Grid
}

void ModelViewController::ConvertToGCode()
{
	string GcodeTxt;

	Fl_Text_Buffer* buffer = gui->GCodeResult->buffer();
	buffer->remove(0, buffer->length());

	buffer = gui->GCodeStart->buffer();
	char* pText = buffer->text();
	string GCodeStart(pText);
	buffer = gui->GCodeLayer->buffer();
	free(pText);
	pText = buffer->text();
	string GCodeLayer(pText);
	buffer = gui->GCodeEnd->buffer();
	free(pText);
	pText = buffer->text();
	string GCodeEnd(pText);
	free(pText);

	ProcessControl.ConvertToGCode(GcodeTxt, GCodeStart, GCodeLayer, GCodeEnd);
	buffer = gui->GCodeResult->buffer();

	GcodeTxt += "\0";
	buffer->append( GcodeTxt.c_str() );
	redraw();
}

void ModelViewController::init()
{

	Fl_Text_Buffer* buffer = gui->GCodeStart->buffer();
	buffer->text(ProcessControl.GCodeStartText.c_str());
	buffer = gui->GCodeLayer->buffer();
	buffer->text(ProcessControl.GCodeLayerText.c_str());
	buffer = gui->GCodeEnd->buffer();
	buffer->text(ProcessControl.GCodeEndText.c_str());
    //buffer->text(ProcessControl.Notes.c_str());

	buffer = gui->LuaScriptEditor->buffer();
	buffer->text("--Clear existing gcode\nbase:ClearGcode()\n-- Set start speed for acceleration firmware\nbase:AddText(\"G1 F2600\\n\")\n\n	 z=0.0\n	 e=0;\n	oldx = 0;\n	oldy=0;\n	while(z<47) do\n	angle=0\n		while (angle < math.pi*2) do\n	x=(50*math.cos(z/30)*math.sin(angle))+70\n		y=(50*math.cos(z/30)*math.cos(angle))+70\n\n		--how much to extrude\n\n		dx=oldx-x\n		dy=oldy-y\n		if not (angle==0) then\n			e = e+math.sqrt(dx*dx+dy*dy)\n			end\n\n			-- Make gcode string\n\n			s=string.format(\"G1 X%f Y%f Z%f F2600 E%f\\n\", x,y,z,e)\n			if(angle == 0) then\n				s=string.format(\"G1 X%f Y%f Z%f F2600 E%f\\n\", x,y,z,e)\n				end\n				-- Add gcode to gcode result\nbase:AddText(s)\n	 angle=angle+0.2\n	 oldx=x\n	 oldy=y\n	 end\n	 z=z+0.4\n	 end\n	 ");

	//	buffer = gui->CommunationsLogText->buffer();
//	buffer->text("Dump");
}


void ModelViewController::WriteGCode (string filename)
{
  Fl_Text_Buffer* buffer = gui->GCodeResult->buffer();
  int result = buffer->savefile (filename.c_str());

  switch (result)
    {
    case 0: // Succes
      break;
    case 1: // Open for write failed
      fl_alert ("Error saving GCode file, error creating file.");
      break;
    case 2: // Partially saved file
      fl_alert ("Error saving GCode file, while writing file, is the disk full?.");
      break;
    }
}


//Make the remaining buttons work
//implement acceleration

void ModelViewController::CopySettingsToGUI()
{
	if(gui == 0)
		return;
	Fl_Text_Buffer* buffer = gui->GCodeStart->buffer();
	buffer->text(ProcessControl.GCodeStartText.c_str());
	buffer = gui->GCodeLayer->buffer();
	buffer->text(ProcessControl.GCodeLayerText.c_str());
	buffer = gui->GCodeEnd->buffer();
	buffer->text(ProcessControl.GCodeEndText.c_str());
    //buffer->text(ProcessControl.Notes.c_str());

	gui->RaftEnableButton->value(ProcessControl.RaftEnable);
	gui->RaftSizeSlider->value(ProcessControl.RaftSize);
	gui->RaftBaseLayerCountSlider->value(ProcessControl.RaftBaseLayerCount);
	gui->RaftMaterialPrDistanceRatioSlider->value(ProcessControl.RaftMaterialPrDistanceRatio);
	gui->RaftRotationSlider->value(ProcessControl.RaftRotation);
	gui->RaftBaseDistanceSlider->value(ProcessControl.RaftBaseDistance);
	gui->RaftBaseThicknessSlider->value(ProcessControl.RaftBaseThickness);
	gui->RaftBaseTemperatureSlider->value(ProcessControl.RaftBaseTemperature);
	gui->RaftInterfaceLayerCountSlider->value(ProcessControl.RaftInterfaceLayerCount);
	gui->RaftInterfaceMaterialPrDistanceRatioSlider->value(ProcessControl.RaftInterfaceMaterialPrDistanceRatio);
	gui->RaftRotationPrLayerSlider->value(ProcessControl.RaftRotationPrLayer);
	gui->RaftInterfaceDistanceSlider->value(ProcessControl.RaftInterfaceDistance);
	gui->RaftInterfaceThicknessSlider->value(ProcessControl.RaftInterfaceThickness);
	gui->RaftInterfaceTemperatureSlider->value(ProcessControl.RaftInterfaceTemperature);

	gui->ValidateConnection->value(ProcessControl.m_bValidateConnection);
	gui->portInput->value(ProcessControl.m_sPortName.c_str());
	gui->portInputSimple->value(ProcessControl.m_sPortName.c_str());
	gui->SerialSpeedInput->value(ProcessControl.m_iSerialSpeed);
	gui->SerialSpeedInputSimple->value(ProcessControl.m_iSerialSpeed);

	// GCode
	gui->GCodeDrawStartSlider->value(ProcessControl.GCodeDrawStart);
	gui->GCodeDrawEndSlider->value(ProcessControl.GCodeDrawEnd);
	gui->MinPrintSpeedXYSlider->value(ProcessControl.MinPrintSpeedXY);
	gui->MaxPrintSpeedXYSlider->value(ProcessControl.MaxPrintSpeedXY);
	gui->MinPrintSpeedZSlider->value(ProcessControl.MinPrintSpeedZ);
	gui->MaxPrintSpeedZSlider->value(ProcessControl.MaxPrintSpeedZ);

	gui->distanceToReachFullSpeedSlider->value(ProcessControl.DistanceToReachFullSpeed);
//	gui->UseFirmwareAccelerationButton->value(ProcessControl.UseFirmwareAcceleration);
	gui->extrusionFactorSlider->value(ProcessControl.extrusionFactor);
	gui->UseIncrementalEcodeButton->value(ProcessControl.UseIncrementalEcode);
	gui->Use3DGcodeButton->value(ProcessControl.Use3DGcode);

	gui->antioozeDistanceSlider->value(ProcessControl.AntioozeDistance);
	gui->EnableAntioozeButton->value(ProcessControl.EnableAntiooze);
	gui->antioozeSpeedSlider->value(ProcessControl.AntioozeSpeed);

	// Printer
	gui->VolumeX->value(ProcessControl.m_fVolume.x);
	gui->VolumeY->value(ProcessControl.m_fVolume.y);
	gui->VolumeZ->value(ProcessControl.m_fVolume.z);
	gui->MarginX->value(ProcessControl.PrintMargin.x);
	gui->MarginY->value(ProcessControl.PrintMargin.y);
	gui->MarginZ->value(ProcessControl.PrintMargin.z);

	gui->ExtrudedMaterialWidthSlider->value(ProcessControl.ExtrudedMaterialWidth);

	// STL
	gui->LayerThicknessSlider->value(ProcessControl.LayerThickness);
	gui->CuttingPlaneValueSlider->value(ProcessControl.CuttingPlaneValue);
	gui->PolygonOpasitySlider->value(ProcessControl.PolygonOpasity);
	// CuttingPlane
	gui->InfillDistanceSlider->value(ProcessControl.InfillDistance);
	gui->InfillRotationSlider->value(ProcessControl.InfillRotation);
	gui->InfillRotationPrLayerSlider->value(ProcessControl.InfillRotationPrLayer);
	gui->AltInfillLayersInput->value(ProcessControl.AltInfillLayersText.c_str());
	gui->AltInfillDistanceSlider->value(ProcessControl.AltInfillDistance);
	gui->ExamineSlider->value(ProcessControl.Examine);

	gui->ShellOnlyButton->value(ProcessControl.ShellOnly);
	gui->ShellCountSlider->value(ProcessControl.ShellCount);

	gui->EnableAccelerationButton->value(ProcessControl.EnableAcceleration);

	gui->FileLogginEnabledButton->value(ProcessControl.FileLogginEnabled);
	gui->TempReadingEnabledButton->value(ProcessControl.TempReadingEnabled);
	gui->ClearLogfilesWhenPrintStartsButton->value(ProcessControl.ClearLogfilesWhenPrintStarts);

	// GUI GUI
	gui->DisplayEndpointsButton->value(ProcessControl.DisplayEndpoints);
	gui->DisplayNormalsButton->value(ProcessControl.DisplayNormals);
	gui->DisplayWireframeButton->value(ProcessControl.DisplayWireframe);
	gui->DisplayWireframeShadedButton->value(ProcessControl.DisplayWireframeShaded);
	gui->DisplayPolygonsButton->value(ProcessControl.DisplayPolygons);
	gui->DisplayAllLayersButton->value(ProcessControl.DisplayAllLayers);
	gui->DisplayinFillButton->value(ProcessControl.DisplayinFill);
	gui->DisplayDebuginFillButton->value(ProcessControl.DisplayDebuginFill);
	gui->DisplayDebugButton->value(ProcessControl.DisplayDebug);
	gui->DisplayCuttingPlaneButton->value(ProcessControl.DisplayCuttingPlane);
	gui->DrawVertexNumbersButton->value(ProcessControl.DrawVertexNumbers);
	gui->DrawLineNumbersButton->value(ProcessControl.DrawLineNumbers);

	// Rendering
	gui->PolygonValSlider->value(ProcessControl.PolygonVal);
	gui->PolygonSatSlider->value(ProcessControl.PolygonSat);
	gui->PolygonHueSlider->value(ProcessControl.PolygonHue);
	gui->WireframeValSlider->value(ProcessControl.WireframeVal);
	gui->WireframeSatSlider->value(ProcessControl.WireframeSat);
	gui->WireframeHueSlider->value(ProcessControl.WireframeHue);
	gui->NormalSatSlider->value(ProcessControl.NormalsSat);
	gui->NormalValSlider->value(ProcessControl.NormalsVal);
	gui->NormalHueSlider->value(ProcessControl.NormalsHue);
	gui->EndpointSatSlider->value(ProcessControl.EndpointsSat);
	gui->EndpointValSlider->value(ProcessControl.EndpointsVal);
	gui->EndpointHueSlider->value(ProcessControl.EndpointsHue);
	gui->GCodeExtrudeHueSlider->value(ProcessControl.GCodeExtrudeHue);
	gui->GCodeExtrudeSatSlider->value(ProcessControl.GCodeExtrudeSat);
	gui->GCodeExtrudeValSlider->value(ProcessControl.GCodeExtrudeVal);
	gui->GCodeMoveHueSlider->value(ProcessControl.GCodeMoveHue);
	gui->GCodeMoveSatSlider->value(ProcessControl.GCodeMoveSat);
	gui->GCodeMoveValSlider->value(ProcessControl.GCodeMoveVal);
	gui->HighlightSlider->value(ProcessControl.Highlight);

	gui->NormalLengthSlider->value(ProcessControl.NormalsLength);
	gui->EndpointSizeSlider->value(ProcessControl.EndPointSize);
	gui->TempUpdateSpeedSlider->value(ProcessControl.TempUpdateSpeed);

	gui->DisplayGCodeButton->value(ProcessControl.DisplayGCode);
	gui->LuminanceShowsSpeedButton->value(ProcessControl.LuminanceShowsSpeed);

	switch(ProcessControl.m_ShrinkQuality)
	{
	case SHRINK_FAST:
		gui->shrinkAlgorithm->value(0);
		break;
	case SHRINK_NICE:
		gui->shrinkAlgorithm->value(1);
		break;
	case SHRINK_LOGICK:
		gui->shrinkAlgorithm->value(2);
		break;
	}
	gui->OptimizationSlider->value(ProcessControl.Optimization);
	gui->ReceivingBufferSizeSlider->value(ProcessControl.ReceivingBufferSize);

	RefreshCustomButtonLabels();
	GetCustomButtonText(0);
}

void ModelViewController::Continue()
{
	gui->ContinueButton->label("Pause");
	gui->ContinueButton->value(0);
	gui->PrintButton->value(1);
	gui->PrintButton->label("Print");
	gui->PrintButton->deactivate();
	serial->m_bPrinting = true;
	serial->SendNextLine();
}

void ModelViewController::Restart()
{
	serial->Clear();	// resets line nr and clears buffer
	Print();
}

void ModelViewController::ContinuePauseButton()
{
	if( !strcmp (gui->ContinueButton->label(), "Pause") )
	{
		Pause();
	}
	else
	{
		Continue();
	}
}


void ModelViewController::PrintButton()
{
	if( !strcmp (gui->PrintButton->label(), "Print") )
	{
		Print();
	}
	else
	{
		Restart();
	}
}

void ModelViewController::PrintDone()
{
	serial->Clear();	// resets line nr and buffer
	serial->m_bPrinting = false;
	gui->PrintButton->label("Print");
	gui->PrintButton->value(0);
	gui->PrintButton->activate();
	gui->ContinueButton->value(0);
	gui->ContinueButton->label("Pause");
	gui->ContinueButton->deactivate();
}

void ModelViewController::ConnectToPrinter(char on)
{
	if(on)
	{
		serial->Connect(ProcessControl.m_sPortName, ProcessControl.m_iSerialSpeed);
	}
	else
	{
		serial->DisConnect();
	}
}
bool ModelViewController::IsConnected()
{
	return serial->isConnected();
}

void ModelViewController::SimplePrint()
{
	if( serial->isPrinting() )
	{
		fl_alert ("Already printing.\nCannot start printing");
	}

	if( !serial->isConnected() )
	{
		ConnectToPrinter(true);
		WaitForConnection(5.0);
	}

	Print();
}

void ModelViewController::WaitForConnection(float seconds)
{
	serial->WaitForConnection(seconds*1000);
}

void ModelViewController::serialConnected()
{
	gui->ConnectToPrinterButton->set();
	gui->ConnectToPrinterSimpleButton->set();
}

void ModelViewController::serialConnectionLost()
{
	gui->ConnectToPrinterButton->clear();
	gui->ConnectToPrinterSimpleButton->clear();
}

void ModelViewController::Print()
{
	if( !serial->isConnected() )
	{
		fl_alert ("Not connected to printer.\nCannot start printing");
		return;
	}

	gui->ContinueButton->value(0);
	gui->ContinueButton->activate();
	gui->ContinueButton->label("Pause");
	gui->PrintButton->value(1);
	gui->PrintButton->label("Print");
	gui->PrintButton->deactivate();

	// Snack one line at a time from the Gcode view, and buffer it.
/*
	if(gui->PrintButton->value() == 0)	// Turned off print, cancel buffer and flush
	{
		m_bPrinting = false;
		return;
	}
*/
	serial->Clear();	// resets line nr and buffer
	serial->m_bPrinting = false;
	serial->SetDebugMask();
	serial->SetLineNr(-1);	// Reset LineNr Count
	gui->CommunationLog->clear();
	Fl_Text_Buffer* buffer = gui->GCodeResult->buffer();
	char* pText = buffer->text();
	uint length = buffer->length();

	uint pos = 2;
	while(pos < length)
		{
		char* line = buffer->line_text(pos);
		if(line[0] == ';')
			{
			pos = buffer->line_end(pos)+1;	// skip newline
			continue;
			}
		serial->AddToBuffer(line);
		pos = buffer->line_end(pos)+1;	// find end of line
		}

	gui->ProgressBar->maximum(serial->Length());
	gui->ProgressBar->label("Printing");
	gui->ProgressBar->value(0);
	free(pText);
	serial->StartPrint();
}

void ModelViewController::Pause()
{
	if( serial->isPrinting() )
	{
		gui->ContinueButton->value(1);
		gui->ContinueButton->label("Continue");
		gui->PrintButton->value(0);
		gui->PrintButton->activate();
		gui->PrintButton->label("Restart");
		serial->m_bPrinting = false;
	}
}

void ModelViewController::SwitchHeat(bool on, float temp)
{
	std::stringstream oss;
	oss << "M104 S" <<temp;

	if(on)
		serial->SendNow(oss.str());
	else
		serial->SendNow("M104 S0");
}
void ModelViewController::SetTargetTemp(float temp)
{
	m_fTargetTemp = temp;
}
void ModelViewController::SetExtruderSpeed(int speed)
{
	m_iExtruderSpeed = speed;
}
void ModelViewController::SetExtruderLength(int length)
{
	m_iExtruderLength = length;
}
void ModelViewController::SetFileLogging(bool on)
{
	ProcessControl.FileLogginEnabled = on;
}
void ModelViewController::EnableTempReading(bool on)
{
	ProcessControl.TempReadingEnabled = on;
	if(on)
		Fl::add_timeout(1.0f, &TempReadTimer);

}
void ModelViewController::SetLogFileClear(bool on)
{
	ProcessControl.ClearLogfilesWhenPrintStarts = on;
}
void ModelViewController::ClearLogs()
{
	if( gui )
	{
		gui->CommunationLog->clear();
		gui->ErrorLog->clear();
		gui->Echo->clear();
	}
}

void ModelViewController::SwitchPower(bool on)
{
	if(on)
		serial->SendNow("M80");
	else
		serial->SendNow("M81");
}

void ModelViewController::SetFan(int val)
{
	if(val != 0)
	{
		std::stringstream oss;
		oss << "M106 S" << val;
		serial->SendNow(oss.str());
	}
	else
		serial->SendNow("M107");
}

void ModelViewController::RunExtruder()
{
	static bool extruderIsRunning = false;
	if(ProcessControl.Use3DGcode)
	{
		if(extruderIsRunning)
			serial->SendNow("M103");
		else
			serial->SendNow("M101");
		extruderIsRunning = 1-extruderIsRunning;
		return;
	}

	std::stringstream oss;
	string command("G1 F");
	oss << m_iExtruderSpeed;
	command += oss.str();
	serial->SendNow(command);
	oss.str("");

	serial->SendNow("G92 E0");	// set extruder zero
	oss << m_iExtruderLength;
	string command2("G1 E");

	if(!m_bExtruderDirection)	// Forwards
		command2+="-";
	command2+=oss.str();
	serial->SendNow(command2);
	serial->SendNow("G1 F1500.0");

	serial->SendNow("G92 E0");	// set extruder zero
}
void ModelViewController::SetExtruderDirection(bool reverse)
{
	m_bExtruderDirection = !reverse;
}
void ModelViewController::SendNow(string str)
{
	serial->SendNow(str);
}

void ModelViewController::Home(string axis)
{
	if(serial->isPrinting())
	{
		fl_alert("Can't go home while printing");
		return;
	}
	if(axis == "X" || axis == "Y" || axis == "Z")
	{
		string buffer="G1 F";
		std::stringstream oss;
		if(axis == "Z")
			oss << ProcessControl.MaxPrintSpeedZ;
		else
			oss << ProcessControl.MaxPrintSpeedXY;
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
			oss << ProcessControl.MinPrintSpeedZ;
		else
			oss << ProcessControl.MinPrintSpeedXY;
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
		fl_alert("Home called with unknown axis");
}

void ModelViewController::Move(string axis, float distance)
{
	if(serial->isPrinting())
	{
		fl_alert("Can't move manually while printing");
		return;
	}
	if(axis == "X" || axis == "Y" || axis == "Z")
	{
		SendNow("G91");	// relative positioning
		string buffer="G1 F";
		std::stringstream oss;
		if(axis == "Z")
			oss << ProcessControl.MaxPrintSpeedZ;
		else
			oss << ProcessControl.MaxPrintSpeedXY;
		buffer+= oss.str();
		SendNow(buffer);
		buffer="G1 ";
		buffer += axis;
		oss.str("");
		oss << distance;
		buffer+= oss.str();
		oss.str("");
		if(axis == "Z")
			oss << ProcessControl.MaxPrintSpeedZ;
		else
			oss << ProcessControl.MaxPrintSpeedXY;
		buffer+=" F"+oss.str();
		SendNow(buffer);
		SendNow("G90");	// absolute positioning
	}
	else
		fl_alert("Move called with unknown axis");
}
void ModelViewController::Goto(string axis, float position)
{
	if(serial->isPrinting())
	{
		fl_alert("Can't move manually while printing");
		return;
	}
	if(axis == "X" || axis == "Y" || axis == "Z")
	{
		string buffer="G1 F";
		std::stringstream oss;
		oss << ProcessControl.MaxPrintSpeedXY;
		buffer+= oss.str();
		SendNow(buffer);
		buffer="G1 ";
		buffer += axis;
		oss.str("");
		oss << position;
		buffer+= oss.str();
		oss.str("");
		oss << ProcessControl.MaxPrintSpeedXY;
		buffer+=" F"+oss.str();
		SendNow(buffer);
	}
	else
		fl_alert("Goto called with unknown axis");
}
void ModelViewController::STOP()
{
	SendNow("M112");
	serial->Clear(); // reset buffer
}

void ModelViewController::SetPrintMargin(string Axis, float value)
{
	if(Axis == "X")
		ProcessControl.PrintMargin.x = value;
	else if(Axis == "Y")
		ProcessControl.PrintMargin.y = value;
	else if(Axis == "Z")
		ProcessControl.PrintMargin.z = value;

	cout << "ModelViewController::SetPrintMargin NON-WORKING";
//	ProcessControl.stl.MoveIntoPrintingArea(ProcessControl.PrintMargin);
	redraw();
}


class NumberPrinter {
public:
NumberPrinter(int number) :
m_number(number) {}

void print() {
cout << m_number << endl;
}

private:
int m_number;
};

ProcessController &ModelViewController::getProcessController()
{
	return ProcessControl;
}

struct ResourceManager {
ResourceManager() :
   m_ResourceCount(0) {}

  void loadResource(const string &sFilename) {
    ++m_ResourceCount;
  }
size_t getResourceCount() const {
   return m_ResourceCount;
  }

 size_t m_ResourceCount;
};
#ifdef ENABLE_LUA

void print_hello(int number)
{
	cout << "hello world " << number << endl;
}

void refreshGraphicsView(ModelViewController *MVC)
{
	// Hack: save and load the gcode, to force redraw

	GCode* code = &MVC->ProcessControl.gcode;
	code->Write(MVC,"./tmp.gcode");
	code->Read(MVC,"./tmp.gcode");
}

void ReportErrors(lua_State * L)
{
	fl_beep(FL_BEEP_ERROR);
	std::stringstream oss;
	oss << "Error: " << lua_tostring(L,-1);
	fl_alert(oss.str().c_str());
	lua_pop(L, 1);
}
#endif // ENABLE_LUA

void ModelViewController::RunLua(char* script)
{
#ifdef ENABLE_LUA
	try{

		lua_State *myLuaState = lua_open();				// Create a new lua state

		luabind::open(myLuaState);						// Connect LuaBind to this lua state
		luaL_openlibs(myLuaState);

		// Add our function to the state's global scope
		luabind::module(myLuaState) [
			luabind::def("print_hello", print_hello)
		];


		luabind::module(myLuaState)
			[
			luabind::class_<ModelViewController>("ModelViewController")
			.def("ReadStl", &ModelViewController::ReadStl)			// To use: base:ReadStl("c:/Vertex.stl")
			.def("ConvertToGCode", &ModelViewController::ConvertToGCode)			// To use: base:ConvertToGCode()
			.def("ClearGCode", &ModelViewController::ClearGcode)	// To use: base:ClearGcode()
			.def("AddText", &ModelViewController::AddText)			// To use: base:AddText("text\n")
			.def("Print", &ModelViewController::SimplePrint)			// To use: base:Print()
			];

		luabind::globals(myLuaState)["base"] = this;

		// Now call our function in a lua script, such as "print_hello(5)"
		if luaL_dostring( myLuaState, script)
			ReportErrors(myLuaState);

		lua_close(myLuaState);


	}// try
	catch(const std::exception &TheError)
	{
		cerr << TheError.what() << endl;
	}
	refreshGraphicsView (this);
#endif // ENABLE_LUA
}
void ModelViewController::ReadRFO(string filename)
{
	ProcessControl.rfo.Open(filename, ProcessControl);
	string path;

	size_t found;
	cout << "Splitting: " << filename << endl;
	found=filename.find_last_of("/\\");
	cout << " folder: " << filename.substr(0,found) << endl;
	cout << " file: " << filename.substr(found+1) << endl;

	path=filename.substr(0,found);

	ProcessControl.rfo.Load(path, ProcessControl);
	ProcessControl.CalcBoundingBoxAndZoom();
}

void ModelViewController::GetSelectedRFO(RFO_Object **selectedObject, RFO_File **selectedFile)
{
	Flu_Tree_Browser::Node *node=gui->RFP_Browser->get_selected( 1 );
	if(node==0)
		node = gui->RFP_Browser->get_root();
	// Check for selected file (use the object)
	for(UINT o=0;o<ProcessControl.rfo.Objects.size();o++)
	{
		for(UINT f=0;f<ProcessControl.rfo.Objects[o].files.size();f++)
		{
			if(ProcessControl.rfo.Objects[o].files[f].node == node)
			{
				*selectedObject = &ProcessControl.rfo.Objects[o];
				*selectedFile = &ProcessControl.rfo.Objects[o].files[f];
				return;
			}
		}
	}
	// Check for selected object
	for(UINT o=0;o<ProcessControl.rfo.Objects.size();o++)
	{
		if(ProcessControl.rfo.Objects[o].node == node)
		{
			*selectedObject = &ProcessControl.rfo.Objects[o];
//			*selectedFile = 0;
			return;
		}
	}
	if(ProcessControl.rfo.Objects.size())
	{
		*selectedObject = &ProcessControl.rfo.Objects[0];
//		*selectedFile = 0;
		return;
	}
//	*selectedObject = 0;
//	*selectedFile = 0;
}


RFO_Object* ModelViewController::SelectedParent()
{
	Flu_Tree_Browser::Node *node=gui->RFP_Browser->get_selected( 1 );
	if(node==0)
		node = gui->RFP_Browser->get_root();
	// Check for selected object
	for(UINT o=0;o<ProcessControl.rfo.Objects.size();o++)
	{
		if(ProcessControl.rfo.Objects[o].node == node)
			return &ProcessControl.rfo.Objects[o];
	}
	// Check for selected file (use the object)
	for(UINT o=0;o<ProcessControl.rfo.Objects.size();o++)
	{
		for(UINT f=0;f<ProcessControl.rfo.Objects[o].files.size();f++)
		{
			if(ProcessControl.rfo.Objects[o].files[f].node == node)
				return &ProcessControl.rfo.Objects[o];
		}
	}
	if(ProcessControl.rfo.Objects.size())
		return &ProcessControl.rfo.Objects[0];
	return 0;
}
Matrix4f &ModelViewController::SelectedNodeMatrix(uint objectNr)
{
	Flu_Tree_Browser::Node *node=gui->RFP_Browser->get_selected( objectNr );
	for(UINT o=0;o<ProcessControl.rfo.Objects.size();o++)
	{
		if(ProcessControl.rfo.Objects[o].node == node)
			return ProcessControl.rfo.Objects[o].transform3D.transform;
		for(UINT f=0;f<ProcessControl.rfo.Objects[o].files.size();f++)
		{
			if(ProcessControl.rfo.Objects[o].files[f].node == node)
				return ProcessControl.rfo.Objects[o].files[f].transform3D.transform;
		}
	}
	return ProcessControl.rfo.transform3D.transform;
}
void ModelViewController::SelectedNodeMatrices(vector<Matrix4f *> &result )
{
	result.clear();
	UINT i=1;
	Flu_Tree_Browser::Node *node;
	node=gui->RFP_Browser->get_selected( i++ );
	while(node)
	{
		for(UINT o=0;o<ProcessControl.rfo.Objects.size();o++)
		{
			if(ProcessControl.rfo.Objects[o].node == node)
				result.push_back(&ProcessControl.rfo.Objects[o].transform3D.transform);
			for(UINT f=0;f<ProcessControl.rfo.Objects[o].files.size();f++)
			{
				if(ProcessControl.rfo.Objects[o].files[f].node == node)
					result.push_back(&ProcessControl.rfo.Objects[o].files[f].transform3D.transform);
			}
		}
//		return ProcessControl.rfo.transform3D.transform;
		node=gui->RFP_Browser->get_selected( i++ );	// next selected
	}
}

void ModelViewController::Translate(string axis, float distance)
{
	vector<Matrix4f *> pMatrices;
	SelectedNodeMatrices(pMatrices);

	for(UINT i=0;i<pMatrices.size();i++)
		{
		Vector3f Tr = pMatrices[i]->getTranslation();
		if(axis == "X")
			Tr.x = distance;
		if(axis == "Y")
			Tr.y = distance;
		if(axis == "Z")
			Tr.z = distance;
		pMatrices[i]->setTranslation(Tr);
		}
	ProcessControl.CalcBoundingBoxAndZoom();
	redraw();
}

void ModelViewController::Scale(string axis, float distance)
{

}

void ModelViewController::ReadStl(string filename)
{
	STL stl;
	bool ok = ProcessControl.ReadStl(filename, stl);
	if(ok)
		AddStl(stl, filename);
}


RFO_File* ModelViewController::AddStl(STL stl, string filename)
{
	RFO_Object *parent = SelectedParent();
	if(parent == 0)
	{
		ProcessControl.rfo.Objects.push_back(RFO_Object());
		ProcessControl.rfo.BuildBrowser(ProcessControl);
		parent = SelectedParent();
	}
	assert(parent != 0);

	RFO_File r;
	r.stl = stl;

	size_t found;
	found=filename.find_last_of("/\\");
	r.location = filename.substr(found+1);
	//r.filetype = "";
	//string material;
	r.node = 0;	//???
	parent->files.push_back(r);
	ProcessControl.rfo.BuildBrowser(ProcessControl);
	parent->files[parent->files.size()-1].node->select(true); // select the new stl file.
	ProcessControl.CalcBoundingBoxAndZoom();
	redraw();
	return &parent->files.back();
}


void ModelViewController::Duplicate()
{
	Flu_Tree_Browser::Node *node = gui->RFP_Browser->get_selected( 1 );
	// first check files
	for(uint o=0;o<ProcessControl.rfo.Objects.size();o++)
	{
		for(uint f=0;f<ProcessControl.rfo.Objects[o].files.size();f++)
		{
			if(ProcessControl.rfo.Objects[o].files[f].node == node)
			{
				// Move it, so there's room for it.
				RFO_File* obj = AddStl(ProcessControl.rfo.Objects[o].files[f].stl, ProcessControl.rfo.Objects[o].files[f].location);
				Vector3f p = ProcessControl.rfo.Objects[o].files[f].transform3D.transform.getTranslation();
				Vector3f size = ProcessControl.rfo.Objects[o].files[f].stl.Max - ProcessControl.rfo.Objects[o].files[f].stl.Min;
				p.x += size.x+5.0f;	// 5mm space
				obj->transform3D.transform.setTranslation(p);
				gui->RFP_Browser->set_hilighted(obj->node);
				ProcessControl.CalcBoundingBoxAndZoom();
				redraw();
				return;
			}
		}
	}
}

void ModelViewController::newObject()
{
	ProcessControl.rfo.Objects.push_back(RFO_Object());
	ProcessControl.rfo.BuildBrowser(ProcessControl);
}

void ModelViewController::setObjectname(string name)
{
	RFO_Object *selectedObject=0;
	RFO_File *selectedFile=0;
	GetSelectedRFO(&selectedObject, &selectedFile);
	if(selectedObject)
		selectedObject->name = name;
}
void ModelViewController::setFileMaterial(string material)
{
	RFO_Object *selectedObject=0;
	RFO_File *selectedFile=0;
	GetSelectedRFO(&selectedObject, &selectedFile);
	if(selectedFile)
		selectedFile->material = material;
}
void ModelViewController::setFileType(string type)
{
	RFO_Object *selectedObject=0;
	RFO_File *selectedFile=0;
	GetSelectedRFO(&selectedObject, &selectedFile);
	if(selectedFile)
		selectedFile->filetype = type;
}
void ModelViewController::setFileLocation(string location)
{
	RFO_Object *selectedObject=0;
	RFO_File *selectedFile=0;
	GetSelectedRFO(&selectedObject, &selectedFile);
	if(selectedFile)
		selectedFile->location = location;
}

void ModelViewController::SetShrinkQuality(string quality)
{
	if(quality == "Fast")
		ProcessControl.m_ShrinkQuality = SHRINK_FAST;
	else if(quality == "Logick")
		ProcessControl.m_ShrinkQuality = SHRINK_LOGICK;
	else
		ProcessControl.m_ShrinkQuality = SHRINK_NICE;
}
void ModelViewController::SetReceivingBufferSize(float val)
{
	ProcessControl.ReceivingBufferSize = val; serial->SetReceivingBufferSize(val);
}

void ModelViewController::SendCustomButton(int nr)
{
	nr--;
	string gcode = ProcessControl.CustomButtonGcode[nr];
	serial->SendNow(gcode);
}
void ModelViewController::SaveCustomButton()
{
	int ButtonNr = gui->CustomButtonSelectorSlider->value()-1;

	Fl_Text_Buffer* buffer = gui->CustomButtonText->buffer();
	char* pText = buffer->text();
	string Text = string(pText);
	free(pText);
	ProcessControl.CustomButtonGcode[ButtonNr] = Text;

	const char* text = gui->CustomButtonLabel->value();
	string label(text);
	ProcessControl.CustomButtonLabel[ButtonNr] = label;

	RefreshCustomButtonLabels();
}
void ModelViewController::TestCustomButton()
{
	Fl_Text_Buffer* buffer = gui->CustomButtonText->buffer();
	char* pText = buffer->text();
	serial->SendNow(pText);
}
void ModelViewController::GetCustomButtonText(int nr)
{
	string Text = ProcessControl.CustomButtonGcode[nr];

	Fl_Text_Buffer* buffer = gui->CustomButtonText->buffer();
	buffer->text(Text.c_str());
	gui->CustomButtonLabel->value(ProcessControl.CustomButtonLabel[nr].c_str());
	Fl::check();
}


void ModelViewController::RefreshCustomButtonLabels()
{
	for(UINT i=0;i<ProcessControl.CustomButtonLabel.size();i++)
	switch(i)
		{
		case 0: gui->CustomButton1->label(ProcessControl.CustomButtonLabel[i].c_str());
		case 1: gui->CustomButton2->label(ProcessControl.CustomButtonLabel[i].c_str());
		case 2: gui->CustomButton3->label(ProcessControl.CustomButtonLabel[i].c_str());
		case 3: gui->CustomButton4->label(ProcessControl.CustomButtonLabel[i].c_str());
		case 4: gui->CustomButton5->label(ProcessControl.CustomButtonLabel[i].c_str());
		case 5: gui->CustomButton6->label(ProcessControl.CustomButtonLabel[i].c_str());
		case 6: gui->CustomButton7->label(ProcessControl.CustomButtonLabel[i].c_str());
		case 7: gui->CustomButton8->label(ProcessControl.CustomButtonLabel[i].c_str());
		case 8: gui->CustomButton9->label(ProcessControl.CustomButtonLabel[i].c_str());
		case 9: gui->CustomButton10->label(ProcessControl.CustomButtonLabel[i].c_str());
		case 10: gui->CustomButton11->label(ProcessControl.CustomButtonLabel[i].c_str());
		case 11: gui->CustomButton12->label(ProcessControl.CustomButtonLabel[i].c_str());
		case 12: gui->CustomButton13->label(ProcessControl.CustomButtonLabel[i].c_str());
		case 13: gui->CustomButton14->label(ProcessControl.CustomButtonLabel[i].c_str());
		case 14: gui->CustomButton15->label(ProcessControl.CustomButtonLabel[i].c_str());
		case 15: gui->CustomButton16->label(ProcessControl.CustomButtonLabel[i].c_str());
		case 16: gui->CustomButton17->label(ProcessControl.CustomButtonLabel[i].c_str());
		case 17: gui->CustomButton18->label(ProcessControl.CustomButtonLabel[i].c_str());
		case 18: gui->CustomButton19->label(ProcessControl.CustomButtonLabel[i].c_str());
		case 19: gui->CustomButton20->label(ProcessControl.CustomButtonLabel[i].c_str());
		}
}


// LUA functions

void ModelViewController::ClearGcode()
{
	Fl_Text_Buffer* buffer = gui->GCodeResult->buffer();
	buffer->remove(0, buffer->length());
}
int ModelViewController::GCodeSize()
{
	return gui->GCodeResult->buffer()->length();
}
void ModelViewController::AddText(string line)
{
	gui->GCodeResult->buffer()->append(line.c_str());
}
string ModelViewController::GetText()
{
	return gui->GCodeResult->buffer()->text();
}

