#pragma once

#include "flu_tree_browser.h"
#include "stl.h"

class RFO_Transform3D
{
public:
	RFO_Transform3D(){identity();}
	void identity(){transform=Matrix4f::IDENTITY;}
	Matrix4f transform;
	Flu_Tree_Browser::Node* node;
};

class RFO_File
{
public:
	RFO_File(){}
	void Draw();
	RFO_Transform3D transform3D;
	string location;
	string filetype;
	string material;
	STL stl;
	Flu_Tree_Browser::Node* node;
};

class RFO_Object
{
public:
	RFO_Object(){name = "Unnamed object";};
	string name;
	RFO_Transform3D transform3D;
	vector<RFO_File> files;
	Flu_Tree_Browser::Node* node;
};


class RFO
{
public:
	RFO(){version=0.1f;}
	void Draw(ProcessController &PC, float opasity = 1.0f, Flu_Tree_Browser::Node *selected_node=0);
	void Load(string path, ProcessController &PC);
	void BuildBrowser(ProcessController &PC);
	void clear(ProcessController &PC);
	void DeleteSelected(ModelViewController *MVC);
	bool Open(string filename, ProcessController &PC);
	bool Save(string filename, ProcessController &PC);
	Matrix4f &SelectedNodeMatrix(Flu_Tree_Browser::Node *node);
	vector<RFO_Object> Objects;
	RFO_Transform3D transform3D;
	float version;
	string m_filename;
	Flu_Tree_Browser::Node* node;
};
