/*
    This file is a part of the RepSnapper project.
    Copyright (C) 2010  Kulitorum

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/
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
		Flu_Tree_Browser::Node **selecteds = new Flu_Tree_Browser::Node*[toDelete];
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
		delete selecteds;
		return;
	}
}
