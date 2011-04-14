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
#ifndef TYPES_H
#define TYPES_H

// avoid having to re-type forward definitions and enums a lot
enum FileType { TYPE_STL, TYPE_RFO, TYPE_GCODE, TYPE_AUTO };

// try to avoid compile time explosion by reducing includes
class GUI;
class Poly;
class View;
class GCode;
class Model;
class Render;
class Command;
class Point2f;
class Printer;
class Settings;
class Triangle;
class Settings;
class Segment2f;
class AsyncSerial;
class RepRapSerial;
class CuttingPlane;
class PrintInhibitor;
class ProcessController;

class RFO;
class RFO_File;
class RFO_Object;
class RFO_Transform3D;


enum TempType { TEMP_NOZZLE, TEMP_BED, TEMP_LAST };
enum SerialState { SERIAL_DISCONNECTED, SERIAL_DISCONNECTING,
		   SERIAL_CONNECTED, SERIAL_CONNECTING };

#endif // ENUMS_H
