/*
    This file is a part of the RepSnapper project.
    Copyright (C) 2011 Michael Meeks

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

/* #ifndef TYPES_H */
/* #define TYPES_H */
#pragma once

// try to avoid compile time explosion by reducing includes


class GUI;
class Poly;
class View;
class GCode;
class Model;
class Render;
class Command;
class Printer;
class Settings;
class Triangle;
class RepRapSerial;
class Layer;
class PrintInhibitor;
class ProcessController;
class ObjectsTree;
class TreeObject;
class Shape;
class Transform3D;
class Infill;
class ViewProgress;


enum SerialState { SERIAL_DISCONNECTED, SERIAL_DISCONNECTING,
		   SERIAL_CONNECTED, SERIAL_CONNECTING };


/* #endif // TYPES_H */
