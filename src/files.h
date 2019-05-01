/*
    This file is a part of the RepSnapper project.
    Copyright (C) 2010  Kulitorum
    Copyright (C) 2011-12  martin.dieringer@gmx.de

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

#pragma once

#include "stdafx.h"
//#include <glibmm/stringutils.h>

#include "triangle.h"
#include <QFile>
#include <QFileInfo>

#define ENABLE_AMF 0

void save_locales();
void set_locales(const char * loc);
void reset_locales();


//using namespace Glib;

enum filetype_t{
    ASCII_STL,
    BINARY_STL,
    NONE_STL,
    VRML,
    SVG,
    AMF,
    UNKNOWN_TYPE
};


class File
{
public:
  File(){}
  File(QFile *file);
  virtual ~File(){}

  QFile *_file;
  QFileInfo _fileInfo;

  filetype_t getFileType();

  void loadTriangles(vector< vector<Triangle> > &triangles,
                     vector<QString> &names,
                     uint max_triangles=0);


  bool load_asciiSTL(vector< vector<Triangle> > &triangles,
                     vector<QString> &names,
                     uint max_triangles=0, bool readnormals=false);

  bool load_binarySTL(vector<Triangle> &triangles,
                      uint max_triangles=0, bool readnormals=false);

  bool load_VRML(vector<Triangle> &triangles, uint max_triangles=0);

// does not cross-compile ....
#ifdef WIN32
#define ENABLE_AMF 0
#else
#define ENABLE_AMF 0
#endif

#if ENABLE_AMF
  bool load_AMF (vector< vector<Triangle> > &triangles,
                 vector<QString> &names,
                 uint max_triangles=0);

  static bool save_AMF (QString filename,
                        const vector< vector<Triangle> > &triangles,
                        const vector<QString> &names,
                        bool compressed = true);
#endif

  static bool parseSTLtriangles_ascii(istream &text,
                                      uint max_triangles, bool readnormals,
                                      vector<Triangle> &triangles,
                                      QString &name);


  /* static bool loadVRMLtriangles(ustring filename, */
  /* 				uint max_triangles, */
  /* 				vector<Triangle> &triangles); */



  static bool saveBinarySTL(QString filename, const vector<Triangle> &triangles,
                            const Matrix4d &T);

};
