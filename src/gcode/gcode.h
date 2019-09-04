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
#pragma once

#include <vector>

#include <iostream>
#include <fstream>
#include <string>
#include <sstream>

#include <QTextDocument>
#include <QTextCursor>
class QPlainTextEdit;

#include "command.h"



class GCodeIter
{

    size_t currentCommand;
    const vector<Command> &commands;
    Settings * settings;

    bool speedalways, useTcommand, relativeE;
    vector<QChar> eLetters;
    uint currextruder;

    ViewProgress * progress;
    size_t progress_steps;

    bool paused;

public:
    GCodeIter (const vector<Command> &commands, Settings * settings,
               ViewProgress * progress);

    double lastE, lastF;
    char E_letter;
    Vector3d lastPos;

    double duration;

    int currentLayer;
    double currentLayerZ;

    size_t currentIndex() const { return currentCommand; }
    const Command *getCurrentCommand() const {return &commands[currentCommand];}
    QString get_line(bool calc_duration = false);
    QString get_line_stripped(bool calc_duration = false);
    bool finished();
    void setPaused(bool value);
    bool isPaused() const;
};

class GCode : public QObject
{
    Q_OBJECT

public:
    GCode();
    ~GCode();

  void Read  (QTextDocument *doc, ViewProgress *progress, string filename);
  //void Write (Model *model, string filename);
  void draw  (Settings *settings,
              int layer=-1, bool liveprinting=false,
              uint linewidth=3);
  void drawCommands(Settings *settings, ulong start, ulong end,
                    bool liveprinting, uint linewidth, bool arrows, bool boundary=false,
                    bool onlyZChange = false);
  bool MakeText(Settings *settings, ViewProgress * progress);

  //bool append_text (const std::string &line);
  QString get_text(Settings *settings, ViewProgress *progress);
  void clear();

  std::vector<Command> commands;
  uint size() const { return uint(commands.size()); }

  Vector3d Min, Max, Center;

  void translate(const Vector3d &trans);

  QTextDocument *buffer = nullptr;
  GCodeIter *get_iter (Settings * settings, ViewProgress *progress = nullptr) const;

  double GetTotalExtruded(bool relativeEcode) const;
  void calcTimeEstimation(const Vector3d &from);
  long totalTime;
  void findLayerChanges();

  Vector3d currentCursorWhere;
  Vector3d currentCursorFrom;
  Command currentCursorCommand;

  vector<unsigned long> layerchanges;
  int getLayerNo(const double z) const;
  int getLayerNo(const unsigned long commandno) const;
  unsigned long getLayerStart(const uint layerno) const;
  unsigned long getLayerEnd(const uint layerno) const;

  void clearGlList();
private:
  GLuint gl_List;

  unsigned long unconfirmed_blocks;
signals:
  void gcode_changed();


};
