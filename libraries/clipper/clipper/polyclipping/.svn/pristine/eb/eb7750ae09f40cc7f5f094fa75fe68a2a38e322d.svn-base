//---------------------------------------------------------------------------

#pragma hdrstop

#include <cmath>
#include <stdlib>
#include <vector>
#include <iostream>
#include <string>
#include <stdio.h>
#include "clipper.hpp"

//---------------------------------------------------------------------------

using namespace clipper;
using namespace std;

const scaling = 10000;

inline int Round(double val)
{
  if ((val < 0)) return (int)(val - 0.5); else return (int)(val + 0.5);
}
//------------------------------------------------------------------------------

bool LoadFromFile(char* filename, clipper::Polygons &pp){
  char buffer[10];
  FILE *f = fopen(filename, "r");
  if (f == 0) return false;
  int polyCnt, vertCnt, i, j;
  double X, Y;
  pp.clear();
  if (fscanf(f, "%d", &polyCnt) != 1 || polyCnt == 0) return false;
  pp.resize(polyCnt);
  for (i = 0; i < polyCnt; i++) {
    if (fscanf(f, "%d", &vertCnt) != 1) return false;
    pp[i].resize(vertCnt);
    for (j = 0; j < vertCnt; j++){
      if (fscanf(f, "%lf, %lf", &X, &Y) != 2) return false;
      pp[i][j].X = Round(X *scaling); pp[i][j].Y = Round(Y *scaling);
      fgets(buffer, 10, f); //gobble any trailing commas
    }
  }
  fclose(f);
  return true;
}
//---------------------------------------------------------------------------

void SaveToConsole(const string name, const clipper::Polygons &pp)
{
  cout << '\n' << name << ":\n"
    << pp.size() << '\n';
  for (unsigned i = 0; i < pp.size(); ++i)
  {
    cout << pp[i].size() << '\n';
    for (unsigned j = 0; j < pp[i].size(); ++j)
      cout << pp[i][j].X /scaling << ", " << pp[i][j].Y /scaling << ",\n";
  }
  cout << "\n";
}
//---------------------------------------------------------------------------

void SaveToFile(char *filename, clipper::Polygons &pp)
{
  FILE *f = fopen(filename, "w");
  fprintf(f, "%d\n", pp.size());
  for (unsigned i = 0; i < pp.size(); ++i)
  {
    fprintf(f, "%d\n", pp[i].size());
    for (unsigned j = 0; j < pp[i].size(); ++j)
      fprintf(f, "%.4lf, %.4lf\n",
        (double)pp[i][j].X /scaling, (double)pp[i][j].Y /scaling);
  }
  fclose(f);
}
//---------------------------------------------------------------------------

#pragma argsused
int _tmain(int argc, _TCHAR* argv[])
{

  if (argc < 3)
  {
    cout << "\nUSAGE:\n"
      << "clipper.exe subject_file clip_file "
      << "[INTERSECTION | UNION | DIFFERENCE | XOR] "
      << "[EVENODD | NONZERO] [EVENODD | NONZERO]\n";
    cout << "\nINPUT AND OUTPUT FILE FORMAT ([optional] {comments}):\n"
      << "Polygon Count\n"
      << "Vertex Count {first polygon}\n"
      << "X, Y[,] {first vertex}\n"
      << "X, Y[,] {next vertex}\n"
      << "{etc.}\n"
      << "Vertex Count {second polygon, if there is one}\n"
      << "X, Y[,] {first vertex of second polygon}\n"
      << "{etc.}\n\n";
    return 1;
  }

  Polygons subject, clip;
  if (!LoadFromFile(argv[1], subject))
  {
    cerr << "\nCan't open the file " << argv[1]
      << " or the file format is invalid.\n";
    return 1;
  }
  if (!LoadFromFile(argv[2], clip))
  {
    cerr << "\nCan't open the file " << argv[2]
      << " or the file format is invalid.\n";
    return 1;
  }

  ClipType clipType = ctIntersection;
  const string sClipType[] = {"INTERSECTION", "UNION", "DIFFERENCE", "XOR"};

  if (argc > 3)
  {
    if (stricmp(argv[3], "XOR") == 0) clipType = ctXor;
    else if (stricmp(argv[3], "UNION") == 0) clipType = ctUnion;
    else if (stricmp(argv[3], "DIFFERENCE") == 0) clipType = ctDifference;
    else clipType = ctIntersection;
  }

  PolyFillType subj_pft = pftNonZero, clip_pft = pftNonZero;
  if (argc == 6)
  {
    if (stricmp(argv[4], "EVENODD") == 0) subj_pft = pftEvenOdd;
    if (stricmp(argv[5], "EVENODD") == 0) clip_pft = pftEvenOdd;
  }

  Clipper c;
  c.AddPolygons(subject, ptSubject);
  c.AddPolygons(clip, ptClip);
  Polygons solution;
  c.Execute(clipType, solution, subj_pft, clip_pft);

  string s = "Subjects (";
  s += (subj_pft == pftEvenOdd ? "EVENODD)" : "NONZERO)");
  SaveToConsole(s, subject);
  s = "Clips (";
  s += (clip_pft == pftEvenOdd ? "EVENODD)" : "NONZERO)");
  SaveToConsole(s, clip);
  s = "Solution (using " + sClipType[clipType] + ")";
  SaveToConsole(s, solution);
  SaveToFile("solution.txt", solution);
  return 0;
}
//---------------------------------------------------------------------------
