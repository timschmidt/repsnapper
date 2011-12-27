/*******************************************************************************
*                                                                              *
* Author    :  Angus Johnson                                                   *
* Version   :  4.1.0                                                           *
* Date      :  5 April 2011                                                    *
* Website   :  http://www.angusj.com                                           *
* Copyright :  Angus Johnson 2010-2011                                         *
*                                                                              *
* License:                                                                     *
* Use, modification & distribution is subject to Boost Software License Ver 1. *
* http://www.boost.org/LICENSE_1_0.txt                                         *
*                                                                              *
* Attributions:                                                                *
* The code in this library is an extension of Bala Vatti's clipping algorithm: *
* "A generic solution to polygon clipping"                                     *
* Communications of the ACM, Vol 35, Issue 7 (July 1992) pp 56-63.             *
* http://portal.acm.org/citation.cfm?id=129906                                 *
*                                                                              *
* Computer graphics and geometric modeling: implementation and algorithms      *
* By Max K. Agoston                                                            *
* Springer; 1 edition (January 4, 2005)                                        *
* http://books.google.com/books?q=vatti+clipping+agoston                       *
*                                                                              *
*******************************************************************************/

#ifndef clipper_hpp
#define clipper_hpp

#include <vector>
#include <stdexcept>
#include <cstring>
#include <cstdlib>

namespace clipper {

enum ClipType { ctIntersection, ctUnion, ctDifference, ctXor };
enum PolyType { ptSubject, ptClip };
enum PolyFillType { pftEvenOdd, pftNonZero };

struct IntPoint {
  int X;
  int Y;
  IntPoint(int x = 0, int y = 0): X(x), Y(y) {};
};

typedef std::vector< IntPoint > Polygon;
typedef std::vector< Polygon > Polygons;

typedef signed long long long64;

bool IsClockwise(const Polygon &poly);
double Area(const Polygon &poly);
Polygons OffsetPolygons(const Polygons &pts, const float &delta);

//used internally ...
enum EdgeSide { esLeft, esRight };
enum IntersectProtects { ipNone = 0, ipLeft = 1, ipRight = 2, ipBoth = 3 };

struct TEdge4 {
  int xbot;
  int ybot;
  int xcurr;
  int ycurr;
  int xtop;
  int ytop;
  double dx;
  int tmpX;
  PolyType polyType;
  EdgeSide side;
  int windDelta; //1 or -1 depending on winding direction
  int windCnt;
  int windCnt2; //winding count of the opposite polytype
  int outIdx;
  TEdge4 *next;
  TEdge4 *prev;
  TEdge4 *nextInLML;
  TEdge4 *nextInAEL;
  TEdge4 *prevInAEL;
  TEdge4 *nextInSEL;
  TEdge4 *prevInSEL;
};

struct IntersectNode {
  TEdge4          *edge1;
  TEdge4          *edge2;
  IntPoint        pt;
  IntersectNode  *next;
};

struct LocalMinima {
  int           Y;
  TEdge4        *leftBound;
  TEdge4        *rightBound;
  LocalMinima  *next;
};

struct Scanbeam {
  int       Y;
  Scanbeam *next;
};

struct PolyPt {
  IntPoint pt;
  PolyPt  *next;
  PolyPt  *prev;
  bool     isHole;
};

struct JoinRec {
  IntPoint  pt1a;
  IntPoint  pt1b;
  int       poly1Idx;
  IntPoint  pt2a;
  IntPoint  pt2b;
  int       poly2Idx;
};

struct HorzJoinRec {
  TEdge4   *edge;
  int       savedIdx;
};

struct IntRect { int left; int top; int right; int bottom; };

typedef std::vector < PolyPt* > PolyPtList;
typedef std::vector < TEdge4* > EdgeList;
typedef std::vector < JoinRec* > JoinList;
typedef std::vector < HorzJoinRec* > HorzJoinList;

//ClipperBase is the ancestor to the Clipper class. It should not be
//instantiated directly. This class simply abstracts the conversion of sets of
//polygon coordinates into edge objects that are stored in a LocalMinima list.
class ClipperBase
{
public:
  ClipperBase();
  virtual ~ClipperBase();
  void AddPolygon(const Polygon &pg, PolyType polyType);
  void AddPolygons( const Polygons &ppg, PolyType polyType);
  virtual void Clear();
  IntRect GetBounds();
protected:
  void DisposeLocalMinimaList();
  TEdge4* AddBoundsToLML(TEdge4 *e);
  void PopLocalMinima();
  virtual bool Reset();
  void InsertLocalMinima(LocalMinima *newLm);
  LocalMinima           *m_CurrentLM;
  LocalMinima           *m_MinimaList;
private:
  EdgeList               m_edges;
};

class Clipper : public virtual ClipperBase
{
public:
  Clipper();
  ~Clipper();
  bool Execute(ClipType clipType,
    Polygons &solution,
    PolyFillType subjFillType = pftEvenOdd,
    PolyFillType clipFillType = pftEvenOdd);
protected:
  bool Reset();
private:
  PolyPtList        m_PolyPts;
  JoinList          m_Joins;
  HorzJoinList      m_HorizJoins;
  ClipType          m_ClipType;
  Scanbeam         *m_Scanbeam;
  TEdge4           *m_ActiveEdges;
  TEdge4           *m_SortedEdges;
  IntersectNode    *m_IntersectNodes;
  bool              m_ExecuteLocked;
  PolyFillType      m_ClipFillType;
  PolyFillType      m_SubjFillType;
  void DisposeScanbeamList();
  void SetWindingCount(TEdge4& edge);
  bool IsNonZeroFillType(const TEdge4& edge) const;
  bool IsNonZeroAltFillType(const TEdge4& edge) const;
  void InsertScanbeam(const int Y);
  int PopScanbeam();
  void InsertLocalMinimaIntoAEL(const int botY);
  void InsertEdgeIntoAEL(TEdge4 *edge);
  void AddEdgeToSEL(TEdge4 *edge);
  void CopyAELToSEL();
  void DeleteFromSEL(TEdge4 *e);
  void DeleteFromAEL(TEdge4 *e);
  void UpdateEdgeIntoAEL(TEdge4 *&e);
  void SwapPositionsInSEL(TEdge4 *edge1, TEdge4 *edge2);
  bool IsContributing(const TEdge4& edge) const;
  bool IsTopHorz(const int XPos);
  void SwapPositionsInAEL(TEdge4 *edge1, TEdge4 *edge2);
  void DoMaxima(TEdge4 *e, int topY);
  void ProcessHorizontals();
  void ProcessHorizontal(TEdge4 *horzEdge);
  void AddLocalMaxPoly(TEdge4 *e1, TEdge4 *e2, const IntPoint &pt);
  void AddLocalMinPoly(TEdge4 *e1, TEdge4 *e2, const IntPoint &pt);
  void AppendPolygon(TEdge4 *e1, TEdge4 *e2);
  void DoEdge1(TEdge4 *edge1, TEdge4 *edge2, const IntPoint &pt);
  void DoEdge2(TEdge4 *edge1, TEdge4 *edge2, const IntPoint &pt);
  void DoBothEdges(TEdge4 *edge1, TEdge4 *edge2, const IntPoint &pt);
  void IntersectEdges(TEdge4 *e1, TEdge4 *e2,
    const IntPoint &pt, IntersectProtects protects);
  PolyPt* AddPolyPt(TEdge4 *e, const IntPoint &pt);
  void DisposeAllPolyPts();
  bool ProcessIntersections( const int topY);
  void AddIntersectNode(TEdge4 *e1, TEdge4 *e2, const IntPoint &pt);
  void BuildIntersectList(const int topY);
  void ProcessIntersectList();
  void ProcessEdgesAtTopOfScanbeam(const int topY);
  void BuildResult(Polygons& polypoly);
  void DisposeIntersectNodes();
  bool FixupIntersections();
  bool IsHole(TEdge4 *e);
  void AddJoin(TEdge4 *e1, TEdge4 *e2, int e1OutIdx = -1);
  void ClearJoins();
  void AddHorzJoin(TEdge4 *e, int idx);
  void ClearHorzJoins();
  void JoinCommonEdges();

};

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

class clipperException : public std::exception
{
  public:
    clipperException(const char* description)
      throw(): std::exception(), m_description (description) {}
    virtual ~clipperException() throw() {}
    virtual const char* what() const throw() {return m_description.c_str();}
  private:
    std::string m_description;
};
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

} //clipper namespace
#endif //clipper_hpp


