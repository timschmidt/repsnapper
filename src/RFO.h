#pragma once

#include "flu_tree_browser.h"

/*
<!--
the values in this file are made-up and are only used to illustrate structure.  
-->
&#8722;
<reprap-fab-at-home-build version="0.1">
<!-- a build consists of a variety of objects -->
&#8722;
<object name="Corner Bracket">
<!-- each object can contain multiple files. -->
&#8722;
<files>
<!-- each file has a location, filetype, and material -->
&#8722;
<file location="1.stl" filetype="application/sla" material="HDPE">
&#8722;
<!--
you can specify a transform on the file level.  it is applied to this file only. 
-->
&#8722;
<transform3D>
<row m00="1.0" m01="0.0" m02="0.0" m03="2.0"/>
<row m10="0.0" m11="1.0" m12="0.0" m13="2.0"/>
<row m20="0.0" m21="0.0" m22="1.0" m23="2.0"/>
<row m30="0.0" m31="0.0" m32="0.0" m33="1.0"/>
</transform3D>
</file>
&#8722;
<file location="2.stl" material="Silicone">
&#8722;
<transform3D>
<row m00="1.0" m01="0.0" m02="0.0" m03="2.0"/>
<row m10="0.0" m11="1.0" m12="0.0" m13="2.0"/>
<row m20="0.0" m21="0.0" m22="1.0" m23="2.0"/>
<row m30="0.0" m31="0.0" m32="0.0" m33="1.0"/>
</transform3D>
</file>
</files>
&#8722;
<!--
you can also supply a transform on the object level that is applied to every file in the object, after they are all loaded. 
-->
&#8722;
<transform3D>
<row m00="1.0" m01="0.0" m02="0.0" m03="2.0"/>
<row m10="0.0" m11="1.0" m12="0.0" m13="2.0"/>
<row m20="0.0" m21="0.0" m22="1.0" m23="2.0"/>
<row m30="0.0" m31="0.0" m32="0.0" m33="1.0"/>
</transform3D>
</object>
&#8722;
<object name="X Carriage">
&#8722;
<files>
&#8722;
<file location="http://www.example.com/xcarriage.stl" material="HDPE">
&#8722;
<transform3D>
<row m00="1.0" m01="0.0" m02="0.0" m03="2.0"/>
<row m10="0.0" m11="1.0" m12="0.0" m13="2.0"/>
<row m20="0.0" m21="0.0" m22="1.0" m23="2.0"/>
<row m30="0.0" m31="0.0" m32="0.0" m33="1.0"/>
</transform3D>
</file>
&#8722;
<file location="3.stl" material="Polyfilla">
&#8722;
<transform3D>
<row m00="1.0" m01="0.0" m02="0.0" m03="2.0"/>
<row m10="0.0" m11="1.0" m12="0.0" m13="2.0"/>
<row m20="0.0" m21="0.0" m22="1.0" m23="2.0"/>
<row m30="0.0" m31="0.0" m32="0.0" m33="1.0"/>
</transform3D>
</file>
</files>
&#8722;
<transform3D>
<row m00="1.0" m01="0.0" m02="0.0" m03="0.0"/>
<row m10="0.0" m11="1.0" m12="0.0" m13="0.0"/>
<row m20="0.0" m21="0.0" m22="1.0" m23="0.0"/>
<row m30="0.0" m31="0.0" m32="0.0" m33="1.0"/>
</transform3D>
</object>
&#8722;
<!--
you can also supply a transform on the build level that is applied to each and every object, after they are all loaded. 
-->
&#8722;
<transform3D>
<row m00="1.0" m01="0.0" m02="0.0" m03="0.0"/>
<row m10="0.0" m11="1.0" m12="0.0" m13="0.0"/>
<row m20="0.0" m21="0.0" m22="1.0" m23="0.0"/>
<row m30="0.0" m31="0.0" m32="0.0" m33="1.0"/>
</transform3D>
</reprap-fab-at-home-build>
*/
#pragma once
#include "xml/xml.h"
#include "stl.h"

class RFO_Transform3D
{
public:
	RFO_Transform3D(){identity();}
	void identity(){transform=Matrix4f::IDENTITY;}
	void Read(XMLElement* ch);
	void SaveXML(XMLElement* x);
	Matrix4f transform;
	Flu_Tree_Browser::Node* node;
};

class RFO_File
{
public:
	RFO_File(){}
	void Read(XMLElement* e);
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
	void Read(XMLElement* e);
	void ReadFiles(XMLElement* e);
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
	void ReadVersion0x1(XMLElement* e);
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
