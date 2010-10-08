
#include "stdafx.h"
#include "ui.h"
#include "rfo.h"
#include "flu_pixmaps.h"
#include <exception>
#include <stdexcept>
#include "processcontroller.h"

Fl_Pixmap plus((char * const*)plus_xpm),
minus((char * const*)minus_xpm),
folder_save((char * const*)folder_save_xpm),
folder_load((char * const*)folder_load_xpm),
reload((char * const*)reload_xpm),
home((char * const*)home_xpm),
trash((char * const*)trash_xpm),
arrow_closed((char * const*)arrow_closed_xpm),
arrow_open((char * const*)arrow_open_xpm),
bluedot((char * const*)bluedot_xpm),
book((char * const*)book_xpm),
cd_drive((char * const*)cd_drive_xpm),
computer((char * const*)computer_xpm),
ram_drive((char * const*)ram_drive_xpm),
network_drive((char * const*)network_drive_xpm),
floppy_drive((char * const*)floppy_drive_xpm),
removable_drive((char * const*)removable_drive_xpm),
disk_drive((char * const*)disk_drive_xpm),
filled_folder((char * const*)filled_folder_xpm),
folder_up((char * const*)folder_up_xpm),
folder_closed((char * const*)folder_closed_xpm),
folder_open((char * const*)folder_open_xpm),
greendot((char * const*)greendot_xpm),
purpledot((char * const*)purpledot_xpm),
question_book((char * const*)question_book_xpm),
reddot((char * const*)reddot_xpm),
tealdot((char * const*)tealdot_xpm),
textdoc((char * const*)textdoc_xpm),
yellowdot((char * const*)yellowdot_xpm),
cdrom((char * const*)cdrom_xpm),
big_folder_new((char * const*)big_folder_new_xpm),
big_folder_up((char * const*)big_folder_up_xpm);

void RFO::Draw(ProcessController &PC, float opasity, Flu_Tree_Browser::Node *selected_node)
{
	glPushMatrix();
	glMultMatrixf(&transform3D.transform.array[0]);
	for(UINT o=0;o<Objects.size();o++)
	{
		glPushMatrix();
		glMultMatrixf(&Objects[o].transform3D.transform.array[0]);
		for(UINT f=0;f<Objects[o].files.size();f++)
		{
			glPushMatrix();
			glMultMatrixf(&Objects[o].files[f].transform3D.transform.array[0]);
			if(Objects[o].files[f].node == selected_node)
			{
				PC.PolygonHue+= 0.5f;
				PC.WireframeHue+= 0.5f;
				Objects[o].files[f].stl.draw(PC, opasity);
				PC.PolygonHue-= 0.5f;
				PC.WireframeHue-= 0.5f;
			}
			else 
				Objects[o].files[f].stl.draw(PC, opasity);
			glPopMatrix();
		}
		glPopMatrix();
	}
	glPopMatrix();
}

void RFO::BuildBrowser(ProcessController &PC)
{
	vector<Flu_Tree_Browser::Node*>tree;
	PC.gui->RFP_Browser->clear();
	if(PC.gui)
	{
		if(m_filename.length() == 0)
			m_filename = "Unsaved file";
		size_t found=m_filename.find_last_of("/\\");
		if(found != string::npos)
			PC.gui->RFP_Browser->label( m_filename.substr(found+1).c_str() );
		else
			PC.gui->RFP_Browser->label( m_filename.c_str() );

		Flu_Tree_Browser::Node* parent = PC.gui->RFP_Browser->get_root();
		if( parent ) PC.gui->RFP_Browser->branch_icons( &folder_closed, &folder_open );
		PC.gui->RFP_Browser->all_branches_always_open (true);
		PC.gui->RFP_Browser->selection_drag_mode (FLU_DRAG_TO_SELECT);	//FLU_DRAG_TO_MOVE
		PC.gui->RFP_Browser->selection_color(2);
		tree.push_back(parent);
		for(UINT o=0;o<Objects.size();o++)
		{
			string namex(Objects[o].name.c_str());
			stringstream oss;
			oss << o;
			namex+=oss.str();
			node = PC.gui->RFP_Browser->add_branch(tree.back(), namex.c_str());
			Objects[o].node = node;
			tree.push_back(node);
			for(UINT f=0;f<Objects[o].files.size();f++)
			{
				node = PC.gui->RFP_Browser->add_leaf(tree.back(), Objects[o].files[f].location.c_str());
				Objects[o].files[f].node = node;
				if(Objects[o].files[f].stl.triangles.size() == 0)
					node->leaf_icon(&reddot);
				else
					node->leaf_icon(&greendot);
			}
			tree.pop_back();
		}
		tree.pop_back();
	}
	assert(tree.size() == 0);
	PC.gui->RFP_Browser->redraw();
	PC.gui->RFP_Browser->redraw();
}


void RFO::clear(ProcessController &PC)
{
	if(PC.gui)
		PC.gui->RFP_Browser->clear();
	Objects.clear();
	version=0.0f;
	m_filename="";
	transform3D.identity();
}

Matrix4f &RFO::SelectedNodeMatrix(Flu_Tree_Browser::Node *node)
{
	for(UINT o=0;o<Objects.size();o++)
	{
		if(Objects[o].node == node)
			return Objects[o].transform3D.transform;
		for(UINT f=0;f<Objects[o].files.size();f++)
		{
			if(Objects[o].files[f].node == node)
				return Objects[o].files[f].transform3D.transform;
		}
	}
	return transform3D.transform;
}

#include "stdafx.h"
#include "rfo.h"

void RFO_Transform3D::SaveXML(XMLElement* x)
{
	XMLElement* e = x->FindElementZ("transform3D", true);
	for(UINT i=0;i<4;i++)
	{
		XMLElement* r = e->AddElement(new XMLElement(e, "row"));
		for(UINT j=0;j<4;j++)
		{
			stringstream oss, val;
			oss << "m" << i << j;
			val << transform.m[i][j];
			r->AddVariable(new XMLVariable(oss.str().c_str(),val.str().c_str()));
		}
		
	}

}

bool RFO::Save(string filename, ProcessController &PC)
{
	if(filename.find( ".xml", 0) == string::npos)	// has XML extension
		string filename = filename+".xml";

	vector<XMLElement *> tree;

	XML* xml = new XML(filename.c_str()); 

	XMLElement* e = xml->GetRootElement();
	e->SetElementName("reprap-fab-at-home-build");
	stringstream oss;
	oss << version;
	e->AddVariable(new XMLVariable("version", oss.str().c_str()));

	tree.push_back(e);
	for(UINT o=0;o<Objects.size();o++)
	{
		XMLElement* x = e->AddElement(new XMLElement(e, "object"));
		x->AddVariable(new XMLVariable("name", Objects[o].name.c_str()));
		XMLElement* y = x->AddElement(new XMLElement(x, "files"));
		tree.push_back(e);
		for(UINT f=0;f<Objects[o].files.size();f++)
		{
			XMLElement* z = y->AddElement(new XMLElement(y, "file"));
			z->AddVariable( new XMLVariable("location", Objects[o].files[f].location.c_str()));
			z->AddVariable( new XMLVariable("filetype", Objects[o].files[f].filetype.c_str()));
			z->AddVariable( new XMLVariable("material", Objects[o].files[f].material.c_str()));
			Objects[o].files[f].transform3D.SaveXML(z);
		}
		tree.pop_back();
		Objects[o].transform3D.SaveXML(x);
	}
	tree.pop_back();
	transform3D.SaveXML(e);

	if (xml->IntegrityTest())
		xml->Save(); // Saves back to file
	return true;
}


bool RFO::Open(string filename, ProcessController &PC)
{
	clear(PC);
	try{

		if(filename.find( ".xml", 0) == string::npos)	// has XML extension
			string filename = filename+".xml";
		m_filename = filename;
		XML* xml = new XML(filename.c_str()); 
		XMLElement* e = xml->GetRootElement();

		XMLVariable* y;
		y=e->FindVariableZ("version", false);
		if(y)	version = y->GetValueFloat();
		else
			throw std::runtime_error("Malformed xml file, version nr missing");
		if(version == 0.1f)
			ReadVersion0x1(e);
		//		else if(version == 0.2f)
		//			return ReadVersion0x2(e);
		else
			throw std::runtime_error("Unknown RFO version number, canceling load.");

	}// try
	catch ( const std::exception& err )
		{
		std::cout << err.what() << std::endl;
		}
	catch ( ... )
		{
		std::cout << " unknown-bad " << std::endl;
		}
	return true;
}


void RFO::ReadVersion0x1(XMLElement* e)
{
	cout << "RFO::ReadVersion0x1";
	int nC = e->GetChildrenNum();
	fprintf(stdout,"Root element has %u children.\r\n",nC);
	for(int i = 0 ; i < nC ; i++)
	{
		XMLElement* ch = e->GetChildren()[i];	// Get child i, one of "object" or "transform3D"
		int nV = ch->GetVariableNum();
		int nMaxElName = ch->GetElementName(0);
		char* n = new char[nMaxElName + 1];
		ch->GetElementName(n);
		// Can be one of "object" or "transform3D"
		fprintf(stdout,"\t Child %u: Variables: %u , Name: %s\r\n",i,nV,n);

		if(string(n) == "object")
		{
			RFO_Object obj;
			XMLVariable* y;
			y=ch->FindVariableZ("name", false);
			if(y)
			{
				size_t length = y->GetValue(0, 1);
				char* val = new char[length+1];
				y->GetValue(val);
				obj.name=val;
				delete[] val;
			}
			obj.Read(ch);
			Objects.push_back(obj);
		}
		else if(string(n) == "transform3D")
		{
			transform3D.Read(ch);
		}
	}
}

void RFO_Object::Read(XMLElement* e)
{
	cout << "RFO_Object::Read";
	int nC = e->GetChildrenNum();
	fprintf(stdout,"Root element has %u children.\r\n",nC);
	for(int i = 0 ; i < nC ; i++)
	{

		XMLElement* ch = e->GetChildren()[i];	// Get child i, one of "object" or "transform3D"
		int nV = ch->GetVariableNum();
		int nMaxElName = ch->GetElementName(0);
		char* n = new char[nMaxElName + 1];
		ch->GetElementName(n);
		// Can be one of "object" or "transform3D"
		fprintf(stdout,"\t Child %u: Variables: %u , Name: %s\r\n",i,nV,n);

		if(string(n) == "files")	// files for this object.
		{
			ReadFiles(ch);
		}
		else
			if(string(n) == "transform3D")
				transform3D.Read(ch);
	}
}

void RFO_Object::ReadFiles(XMLElement* e)
{
	cout << "RFO_Object::ReadFiles";
	int nC = e->GetChildrenNum();
	fprintf(stdout,"Root element has %u children.\r\n",nC);
	for(int i = 0 ; i < nC ; i++)
	{
		XMLElement* ch = e->GetChildren()[i];	// Get child i, one of "object" or "transform3D"
		int nV = ch->GetVariableNum();
		int nMaxElName = ch->GetElementName(0);
		char* n = new char[nMaxElName + 1];
		ch->GetElementName(n);
		// Can be one of "object" or "transform3D"
		fprintf(stdout,"\t Child %u: Variables: %u , Name: %s\r\n",i,nV,n);

		RFO_File file;
		if(string(n) == "file")	// files for this object.
		{
			file.Read(ch);
		}
		else
			if(string(n) == "transform3D")
				transform3D.Read(ch);
		files.push_back(file);
	}
}

void RFO_Transform3D::Read(XMLElement* e)
{
	cout << "RFO_Transform3D::Read";
	int nC = e->GetChildrenNum();
	fprintf(stdout,"Root element has %u children.\r\n",nC);
	for(int i = 0 ; i < nC ; i++)
	{
		XMLElement* ch = e->GetChildren()[i];	// Get child i, one of "object" or "transform3D"
		int nMaxElName = ch->GetElementName(0);
		char* n = new char[nMaxElName + 1];
		ch->GetElementName(n);
		// rwo
		if (string(n) != "row")
			throw std::runtime_error("Malformed xml:Transform3D children not named \"row\" ");

		XMLVariable* y;
		for(int x=0;x<4;x++)
		{
			stringstream oss;
			oss << "m" << i << x;
			y=ch->FindVariableZ(oss.str().c_str(), false);
			if(y)	transform.m[i][x] = y->GetValueFloat();
		}
	}
}



void RFO_File::Read(XMLElement* e)
{
	// 3 Variables, one Child
	cout << "RFO_File::Read";
	XMLVariable* y;
	y=e->FindVariableZ("location", false);
	if(y)
	{
		size_t length = y->GetValue(0, 1);
		char* val = new char[length+1];
		y->GetValue(val);
		location=val;
		delete[] val;
	}
	y=e->FindVariableZ("filetype", false);
	if(y)
	{
		size_t length = y->GetValue(0, 1);
		char* val = new char[length+1];
		y->GetValue(val);
		filetype=val;
		delete[] val;
	}
	y=e->FindVariableZ("material", false);
	if(y)
	{
		size_t length = y->GetValue(0, 1);
		char* val = new char[length+1];
		y->GetValue(val);
		material=val;
		delete[] val;
	}

	int nChildren = e->GetChildrenNum();
	if(nChildren)
	{
		assert(nChildren==1);
		XMLElement* child = e->GetChildren()[0];

		int nMaxElName = child->GetElementName(0);
		char* n = new char[nMaxElName + 1];
		child->GetElementName(n);

		assert(string(n) == "transform3D");

		transform3D.Read(child);
	}
}

void RFO::Load(string path, ProcessController &PC)
{
	for(UINT o=0;o<Objects.size();o++)
	{
		for(UINT f=0;f<Objects[o].files.size();f++)
		{
			string fullFilename = path+"/"+Objects[o].files[f].location;
			Objects[o].files[f].stl.Read(fullFilename);
		}
	}

	BuildBrowser(PC);
}

void RFO::DeleteSelected(ModelViewController *MVC)
{
	if (MVC->gui->RFP_Browser->num_selected() == 1)
	{
		Flu_Tree_Browser::Node *node = MVC->gui->RFP_Browser->get_selected(1);
		for(UINT o=0;o<Objects.size();o++)
		{
			if(Objects[o].node == node)
			{
				Objects.erase(Objects.begin()+o);
				BuildBrowser(MVC->ProcessControl);
				MVC->redraw();
				return;
			}
			for(UINT f=0;f<Objects[o].files.size();f++)
			{
				if(Objects[o].files[f].node == node)
				{
					Objects[o].files.erase(Objects[o].files.begin()+f);
					BuildBrowser(MVC->ProcessControl);
					MVC->redraw();
					return;
				}
			}
		}
	}
	else
	{
		int toDelete = MVC->gui->RFP_Browser->num_selected();
		Flu_Tree_Browser::Node *selecteds[toDelete];
		for (int t = 1; t <= toDelete; t++)
		{
			selecteds[t-1] = MVC->gui->RFP_Browser->get_selected(t);
		}
		for (int t = 0; t <= (toDelete - 1); t++)
		{
			for(UINT o=0;o<Objects.size();o++)
			{
				if(Objects[o].node == selecteds[t])
				{
					Objects.erase(Objects.begin()+o);
					
				}
				for(UINT f=0;f<Objects[o].files.size();f++)
				{
					if(Objects[o].files[f].node == selecteds[t])
					{
						Objects[o].files.erase(Objects[o].files.begin()+f);
					}
				}
			}
		}
		BuildBrowser(MVC->ProcessControl);
		MVC->redraw();
		return;
	}
}
