unit clipper;

(*******************************************************************************
*                                                                              *
* Author    :  Angus Johnson                                                   *
* Version   :  2.04                                                            *
* Date      :  3 August 2010                                                   *
* Website   :  http://www.angusj.com                                           *
* Copyright :  Angus Johnson 2010                                              *
*                                                                              *
* License:                                                                     *
* Use, modification & distribution is subject to Boost Software License Ver 1. *
* http://www.boost.org/LICENSE_1_0.txt                                         *
* (nb: This library was initially release under dual MPL and LGPL licenses.    *
* This Boost License is much simpler and imposes even fewer restrictions on    *
* the use of this code, yet it still accomplishes the desired purposes.)       *
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
*******************************************************************************)

//Several type definitions used in the code below are defined in the Delphi
//Graphics32 library ( see http://www.graphics32.org/wiki/ ). These type
//definitions are redefined here in case you don't wish to use Graphics32.
{$DEFINE USING_GRAPHICS32}

interface

uses
{$IFDEF USING_GRAPHICS32}
  GR32,
{$ENDIF}
  SysUtils, Classes, Math;

const
  //infinite: simply used to define inverse slope (dx/dy) of horizontal edges
  infinite: double = -3.4e+38;
  //tolerance: ideally this value should vary depending on how big (or small)
  //the supplied polygon coordinate values are. If coordinate values are greater
  //than 1.0E+5 (ie 100,000+) then tolerance should be adjusted up (since the
  //significand of type double is 15 decimal places). However, for the vast
  //majority of uses ... tolerance = 1.0e-10 will be just fine.
  tolerance: double = 1.0e-10;
  //default_dup_pt_tolerance: see TClipperBase.DuplicatePointTolerance property
  default_dup_pt_tolerance: double = 1.0e-6;
type
  TClipType = (ctIntersection, ctUnion, ctDifference, ctXor);
  TPolyType = (ptSubject, ptClip);
  TPolyFillType = (pftEvenOdd, pftNonZero);
  TEdgeSide = (esLeft, esRight);
  TDirection = (dRightToLeft, dLeftToRight);
  TIntersectProtect = (ipLeft, ipRight);
  TIntersectProtects = set of TIntersectProtect;

{$IFNDEF USING_GRAPHICS32}
  TFloat = Single;
  TFloatPoint = record X, Y: TFloat; end;
  TArrayOfFloatPoint = array of TFloatPoint;
  TArrayOfArrayOfFloatPoint = array of TArrayOfFloatPoint;
{$ENDIF}
  PDoublePoint = ^TDoublePoint;
  TDoublePoint = record X, Y: double; end;
  TArrayOfDoublePoint = array of TDoublePoint;
  TArrayOfArrayOfDoublePoint = array of TArrayOfDoublePoint;

  PEdge = ^TEdge;
  TEdge = record
    xbot: double;
    ybot: double;
    xtop: double;
    ytop: double;
    dx  : double;
    tmpX:  double;
    polyType: TPolyType;
    side: TEdgeSide;
    windDelta: integer;
    windCnt: integer;
    windCnt2: integer;  //winding count of polytype <> self.polytype
    outIdx: integer;
    next: PEdge;
    prev: PEdge;
    nextInLML: PEdge;
    nextInAEL: PEdge;
    prevInAEL: PEdge;
    nextInSEL: PEdge;
    prevInSEL: PEdge;
    savedBot: TDoublePoint;
  end;

  PEdgeArray = ^TEdgeArray;
  TEdgeArray = array[0.. MaxInt div sizeof(TEdge) -1] of TEdge;

  PIntersectNode = ^TIntersectNode;
  TIntersectNode = record
    edge1: PEdge;
    edge2: PEdge;
    pt: TDoublePoint;
    next: PIntersectNode;
    prev: PIntersectNode;
  end;

  TIntersectType = (itIgnore, itMax, itMin, itEdge1, itEdge2);

  PLocalMinima = ^TLocalMinima;
  TLocalMinima = record
    y: double;
    leftBound: PEdge;
    rightBound: PEdge;
    nextLm: PLocalMinima;
  end;

  PScanbeam = ^TScanbeam;
  TScanbeam = record
    y: double;
    nextSb: PScanbeam;
  end;

  TriState = (sFalse, sTrue, sUndefined);

  PPolyPt = ^TPolyPt;
  TPolyPt = record
    pt: TDoublePoint;
    next: PPolyPt;
    prev: PPolyPt;
    isHole: TriState; //See TClipper ForceAlternateOrientation property
  end;

  //TClipperBase is the ancestor to the TClipper class. It should not be
  //instantiated directly. This class simply abstracts the conversion of sets of
  //polygon coordinates into edge objects that are stored in a LocalMinima list.
  TClipperBase = class
  private
    fList             : TList;
    fRecycledLocMin   : PLocalMinima;
    fRecycledLocMinEnd: PLocalMinima;
    procedure DisposeLocalMinimaList;
    function GetDuplicatePointTolerance: integer;
    procedure SetDuplicatePointTolerance(const value: integer);
  protected
    fDupPtTolerance: double;
    fLocalMinima      : PLocalMinima;
    procedure PopLocalMinima;
    function Reset: boolean;
  public
    constructor Create; virtual;
    destructor Destroy; override;

    //Any number of subject and clip polygons can be added to the clipping task,
    //either individually via the AddPolygon() method, or as groups via the
    //AddPolyPolygon() method, or even using both methods ...
    procedure AddPolygon(const polygon: TArrayOfFloatPoint; polyType: TPolyType); overload;
    procedure AddPolygon(const polygon: TArrayOfDoublePoint; polyType: TPolyType); overload;
    procedure AddPolyPolygon(const polyPolygon: TArrayOfArrayOfFloatPoint; polyType: TPolyType); overload;
    procedure AddPolyPolygon(const polyPolygon: TArrayOfArrayOfDoublePoint; polyType: TPolyType); overload;

    //Clear: If multiple clipping operations are to be performed on different
    //polygon sets, then Clear avoids the need to create new Clipper objects.
    procedure Clear;

    //DuplicatePointTolerance represents the number of decimal places to which
    //input and output polygon coordinates will be rounded. Any resulting
    //adjacent duplicate vertices will be ignored so as to prevent edges from
    //having indeterminate slope.
    //Valid range: 0 .. 6; Default: 6 (ie round coordinates to 6 decimal places)
    //nb: DuplicatePointTolerance can't be reset once polygons have been added
    //to the Clipper object.
    property DuplicatePointTolerance: integer read GetDuplicatePointTolerance
      write SetDuplicatePointTolerance;
  end;

  TClipper = class(TClipperBase)
  private
    fPolyPtList: TList;
    fClipType: TClipType;
    fScanbeam: PScanbeam;
    fActiveEdges: PEdge;
    fSortedEdges: PEdge; //used for intersection sorts and horizontal edges
    fIntersectNodes: PIntersectNode;
    fExecuteLocked: boolean;
    fForceAlternateOrientation: boolean;
    fClipFillType: TPolyFillType;
    fSubjFillType: TPolyFillType;
    function ResultAsFloatPointArray: TArrayOfArrayOfFloatPoint;
    function ResultAsDoublePointArray: TArrayOfArrayOfDoublePoint;
    function InitializeScanbeam: boolean;
    procedure InsertScanbeam(const y: double);
    function PopScanbeam: double;
    procedure DisposeScanbeamList;
    procedure SetWindingDelta(edge: PEdge);
    procedure SetWindingCount(edge: PEdge);
    function IsNonZeroFillType(edge: PEdge): boolean;
    procedure InsertLocalMinimaIntoAEL(const botY: double);
    procedure AddHorzEdgeToSEL(edge: PEdge);
    function IsTopHorz(horzEdge: PEdge; const XPos: double): boolean;
    procedure ProcessHorizontal(horzEdge: PEdge);
    procedure ProcessHorizontals;
    procedure SwapPositionsInAEL(edge1, edge2: PEdge);
    procedure SwapWithNextInSEL(edge: PEdge);
    function BubbleSwap(edge: PEdge): PEdge;
    procedure AddIntersectNode(e1, e2: PEdge; const pt: TDoublePoint);
    procedure ProcessIntersections(const topY: double);
    procedure BuildIntersectList(const topY: double);
    procedure ProcessIntersectList;
    procedure IntersectEdges(e1,e2: PEdge;
      const pt: TDoublePoint; protects: TIntersectProtects = []);
    function GetMaximaPair(e: PEdge): PEdge;
    procedure DeleteFromAEL(e: PEdge);
    procedure DeleteFromSEL(e: PEdge);
    procedure DoMaxima(e: PEdge; const topY: double);
    procedure UpdateEdgeIntoAEL(var e: PEdge);
    procedure ProcessEdgesAtTopOfScanbeam(const topY: double);
    function IsContributing(edge: PEdge): boolean;
    function AddPolyPt(idx: integer;
      const pt: TDoublePoint; ToFront: boolean): integer;
    procedure AddLocalMaxPoly(e1, e2: PEdge; const pt: TDoublePoint);
    procedure AddLocalMinPoly(e1, e2: PEdge; const pt: TDoublePoint);
    procedure AppendPolygon(e1, e2: PEdge);
    function ExecuteInternal(clipType: TClipType): boolean;
    procedure DisposeAllPolyPts;
  public
    //SavedSolution: TArrayOfArrayOfFloatPoint; //clipper.DLL only

    //The Execute() method performs the specified clipping task on previously
    //assigned subject and clip polygons. This method can be called multiple
    //times (ie to perform different clipping operations) without having to
    //reassign either subject or clip polygons.
    function Execute(clipType: TClipType;
      out solution: TArrayOfArrayOfFloatPoint;
      subjFillType: TPolyFillType = pftEvenOdd;
      clipFillType: TPolyFillType = pftEvenOdd): boolean; overload;
    function Execute(clipType: TClipType;
      out solution: TArrayOfArrayOfDoublePoint;
      subjFillType: TPolyFillType = pftEvenOdd;
      clipFillType: TPolyFillType = pftEvenOdd): boolean; overload;

    constructor Create; override;
    destructor Destroy; override;

    //The ForceAlternateOrientation property is only useful when operating on
    //simple polygons. It ensures that simple polygons that result from
    //TClipper.Execute() calls will have clockwise 'outer' and counter-clockwise
    //'inner' (or 'hole') polygons.If ForceAlternateOrientation = false, then
    //the polygons returned in the solution can have any orientation.<br>
    //There's no danger with leaving this property set to true when operating
    //on complex polygons, but it will cause a minor penalty in execution speed.
    //(Default = true)
    property ForceAlternateOrientation: boolean read
      fForceAlternateOrientation write fForceAlternateOrientation;
  end;

  function DoublePoint(const X, Y: double): TDoublePoint; overload;

implementation

resourcestring
  rsDupToleranceRange = 'DuplicatePointTolerance: range 0 .. 6 (decimal places)';
  rsDupToleranceEmpty = 'Clear polygons before setting DuplicatePointTolerance';
  rsMissingRightbound = 'InsertLocalMinimaIntoAEL: missing rightbound';
  rsDoMaxima = 'DoMaxima error';
  rsUpdateEdgeIntoAEL = 'UpdateEdgeIntoAEL error';
  rsProcessEdgesAtTopOfScanbeam = 'ProcessEdgesAtTopOfScanbeam: Broken AEL order';
  rsAppendPolygon = 'AppendPolygon error';

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

{$IFNDEF USING_GRAPHICS32}
function FloatPoint(X, Y: Single): TFloatPoint;
begin
  Result.X := X;
  Result.Y := Y;
end;
//------------------------------------------------------------------------------
{$ENDIF}

function DoublePoint(const X, Y: double): TDoublePoint;
begin
  Result.X := X;
  Result.Y := Y;
end;
//------------------------------------------------------------------------------

function AFloatPt2ADoublePt(const pts: TArrayOfFloatPoint): TArrayOfDoublePoint;
var
  i: integer;
begin
  setlength(result, length(pts));
  for i := 0 to high(pts) do
  begin
    result[i].X := pts[i].X;
    result[i].Y := pts[i].Y;
  end;
end;
//------------------------------------------------------------------------------

function AAFloatPt2AADoublePt(const pts: TArrayOfArrayOfFloatPoint): TArrayOfArrayOfDoublePoint;
var
  i,j: integer;
begin
  setlength(result, length(pts));
  for i := 0 to high(pts) do
  begin
    setlength(result[i], length(pts[i]));
    for j := 0 to high(pts[i]) do
    begin
      result[i][j].X := pts[i][j].X;
      result[i][j].Y := pts[i][j].Y;
    end;
  end;
end;
//------------------------------------------------------------------------------

function RoundToTolerance(const number, epsilon: double): double;
begin
  Result := Round(number / epsilon) * epsilon;
end;
//------------------------------------------------------------------------------

function PointsEqual(const pt1, pt2: TDoublePoint;
  const epsilon: double): boolean; overload;
begin
  result := (abs(pt1.X-pt2.X) < epsilon) and (abs(pt1.Y-pt2.Y) < epsilon);
end;
//------------------------------------------------------------------------------

function PointsEqual(edge: PEdge; const epsilon: double): boolean; overload;
begin
  result := (abs(edge.xbot - edge.xtop) < epsilon) and
    (abs(edge.ybot - edge.ytop) < epsilon);
end;
//------------------------------------------------------------------------------

procedure FixupSolutionColinears(list: TList; idx: integer; const epsilon: double);
var
  pp, tmp: PPolyPt;
begin
	//fixup those occasional overlapping colinear edges (ie empty protrusions) ...
  pp := PPolyPt(list[idx]);
  repeat
    if pp.prev = pp then exit;
    if abs((pp.pt.Y - pp.prev.pt.Y)*(pp.next.pt.X - pp.pt.X) -
        (pp.pt.X - pp.prev.pt.X)*(pp.next.pt.Y - pp.pt.Y)) < epsilon then
    begin
      pp.prev.next := pp.next;
      pp.next.prev := pp.prev;
      tmp := pp;
      if list[idx] = pp then
      begin
        list[idx] := pp.prev;
        pp := pp.next;
      end else
        pp := pp.prev;
      dispose(tmp);
    end else
      pp := pp.next;
  until (pp = list[idx]);
end;
//------------------------------------------------------------------------------

procedure DisposePolyPts(pp: PPolyPt);
var
  tmpPp: PPolyPt;
begin
  pp.prev.next := nil;
  while assigned(pp) do
  begin
    tmpPp := pp;
    pp := pp.next;
    dispose(tmpPp);
  end;
end;
//------------------------------------------------------------------------------

procedure ReversePolyPtLinks(pp: PPolyPt);
var
  pp1,pp2: PPolyPt;
begin
  pp1 := pp;
  repeat
    pp2:= pp1.next;
    pp1.next := pp1.prev;
    pp1.prev := pp2;
    pp1 := pp2;
  until pp1 = pp;
end;
//------------------------------------------------------------------------------

procedure SetDx(e: PEdge; const epsilon: double);
begin
  if abs(e.ybot - e.ytop) < epsilon then
    e.dx := infinite else
    e.dx := (e.xbot - e.xtop)/(e.ybot - e.ytop);
end;
//------------------------------------------------------------------------------

function IsHorizontal(e: PEdge): boolean;
begin
  result := assigned(e) and (e.dx < -3.39e+38);
end;
//------------------------------------------------------------------------------

procedure SwapSides(edge1, edge2: PEdge);
var
  side: TEdgeSide;
begin
  side :=  edge1.side;
  edge1.side := edge2.side;
  edge2.side := side;
end;
//------------------------------------------------------------------------------

procedure SwapPolyIndexes(edge1, edge2: PEdge);
var
  outIdx: integer;
begin
  outIdx :=  edge1.outIdx;
  edge1.outIdx := edge2.outIdx;
  edge2.outIdx := outIdx;
end;
//------------------------------------------------------------------------------

function TopX(edge: PEdge; const currentY: double): double;
begin
  if currentY = edge.ytop then result := edge.xtop
  else result := edge.savedBot.X + edge.dx*(currentY - edge.savedBot.Y);
end;
//------------------------------------------------------------------------------

function ShareSamePoly(e1, e2: PEdge): boolean;
begin
  result := assigned(e1) and assigned(e2) and (e1.outIdx = e2.outIdx);
end;
//------------------------------------------------------------------------------

function SlopesEqual(e1, e2: PEdge; const epsilon: double): boolean;
begin
  if IsHorizontal(e1) then
    result := IsHorizontal(e2)
  else if IsHorizontal(e2) then
    result := false
  else
    result := abs((e1.ytop - e1.savedBot.Y)*(e2.xtop - e2.savedBot.X) -
      (e1.xtop - e1.savedBot.X)*(e2.ytop - e2.savedBot.Y)) < epsilon;
end;
//---------------------------------------------------------------------------

function NextEdgeIsAbove(edge:PEdge): boolean;
begin
  result := ((edge.next.xbot = edge.xtop) and (edge.next.ybot = edge.ytop)) or
      ((edge.next.xtop = edge.xtop) and (edge.next.ytop = edge.ytop));
end;
//---------------------------------------------------------------------------

function IntersectPoint(edge1, edge2: PEdge; out ip: TDoublePoint): boolean;
var
  b1,b2: double;
begin
  if edge1.dx = 0 then
  begin
    ip.X := edge1.savedBot.X;
    with edge2^ do b2 := savedBot.Y - savedBot.X/dx;
    ip.Y := ip.X/edge2.dx + b2;
  end
  else if edge2.dx = 0 then
  begin
    ip.X := edge2.savedBot.X;
    with edge1^ do b1 := savedBot.Y - savedBot.X/dx;
    ip.Y := ip.X/edge1.dx + b1;
  end else
  begin
    with edge1^ do b1 := savedBot.X - savedBot.Y *dx;
    with edge2^ do b2 := savedBot.X - savedBot.Y *dx;
    ip.Y := (b2-b1)/(edge1.dx - edge2.dx);
    ip.X := edge1.dx * ip.Y + b1;
  end;
  result := (ip.Y > edge1.ytop + tolerance) and (ip.Y > edge2.ytop + tolerance);
end;
//------------------------------------------------------------------------------

function GetUnitNormal(const pt1, pt2: TDoublePoint): TDoublePoint;
var
  dx, dy, f: single;
begin
  dx := (pt2.X - pt1.X);
  dy := (pt2.Y - pt1.Y);

  if (dx = 0) and (dy = 0) then
  begin
    result := DoublePoint(0,0);
  end else
  begin
    f := 1 / Hypot(dx, dy);
    dx := dx * f;
    dy := dy * f;
  end;
  Result.X := dy;
  Result.Y := -dx;
end;
//------------------------------------------------------------------------------

function ValidateOrientation(pt: PPolyPt): boolean;
var
  ptStart, bottomPt, ptPrev, ptNext: PPolyPt;
  N1, N2: TDoublePoint;
  IsClockwise: boolean;
begin
  //compares the orientation (clockwise vs counter-clockwise) of a *simple*
  //polygon with its hole status (ie test whether an inner or outer polygon).
  //nb: complex polygons have indeterminate orientations.
  bottomPt := pt;
  ptStart := pt;
  pt := pt.next;
  while (pt <> ptStart) do
  begin
    if (pt.pt.Y > bottomPt.pt.Y) or
      ((pt.pt.Y = bottomPt.pt.Y) and (pt.pt.X > bottomPt.pt.X)) then
        bottomPt := pt;
    pt := pt.next;
  end;

  ptPrev := bottomPt.prev;
  ptNext := bottomPt.next;
  N1 := GetUnitNormal(ptPrev.pt, bottomPt.pt);
  N2 := GetUnitNormal(bottomPt.pt, ptNext.pt);
  //(N1.X * N2.Y - N2.X * N1.Y) == unit normal "cross product" == sin(angle)
  IsClockwise := (N1.X * N2.Y - N2.X * N1.Y) > 0; //ie angle > 180deg.

  while (bottomPt.isHole = sUndefined) and
    (bottomPt.next.pt.Y >= bottomPt.pt.Y) do bottomPt := bottomPt.next;
  while (bottomPt.isHole = sUndefined) and
    (bottomPt.prev.pt.Y >= bottomPt.pt.Y) do bottomPt := bottomPt.prev;
  result := IsClockwise <> (bottomPt.isHole = sTrue);
end;

//------------------------------------------------------------------------------
// TClipperBase methods ...
//------------------------------------------------------------------------------

constructor TClipperBase.Create;
begin
  fList := TList.Create;
  fLocalMinima       := nil;
  fRecycledLocMin    := nil;
  fRecycledLocMinEnd := nil;
  fDupPtTolerance := default_dup_pt_tolerance;
end;
//------------------------------------------------------------------------------

destructor TClipperBase.Destroy;
begin
  Clear;
  fList.Free;
  inherited;
end;
//------------------------------------------------------------------------------

function TClipperBase.GetDuplicatePointTolerance: integer;
begin
  result := - round(log10(fDupPtTolerance));
end;
//------------------------------------------------------------------------------

procedure TClipperBase.SetDuplicatePointTolerance(const value: integer);
begin
  if (value < 0) or (value > 6) then raise exception.Create(rsDupToleranceRange);
  if fList.Count > 0 then raise exception.Create(rsDupToleranceEmpty);
  fDupPtTolerance := power(10, -value);
end;
//------------------------------------------------------------------------------

procedure TClipperBase.AddPolygon(const polygon: TArrayOfFloatPoint; polyType: TPolyType);
var
  dblPts: TArrayOfDoublePoint;
begin
  dblPts := AFloatPt2ADoublePt(polygon);
  AddPolygon(dblPts, polyType);
end;
//------------------------------------------------------------------------------

procedure TClipperBase.AddPolygon(const polygon: TArrayOfDoublePoint; polyType: TPolyType);
var
  i, highI: integer;
  edges: PEdgeArray;
  e, e2: PEdge;
  polyg: TArrayOfDoublePoint;

  //----------------------------------------------------------------------

  procedure InitEdge(e, eNext, ePrev: PEdge; const pt: TDoublePoint);
  begin
    fillChar(e^, sizeof(TEdge), 0);
    e.savedBot := pt;
    e.xtop := eNext.savedBot.X;
    e.ytop := eNext.savedBot.Y;
    e.polyType := polyType;
    e.outIdx := -1;
    e.next := eNext;
    e.prev := ePrev;
  end;
  //----------------------------------------------------------------------

  procedure ReInitEdge(e: PEdge);
  begin
    if e.savedBot.Y > e.ytop then
    begin
      e.xbot := e.savedBot.X;
      e.ybot := e.savedBot.Y;
    end else
    begin
      e.xbot := e.xtop;
      e.ybot := e.ytop;
      e.xtop := e.savedBot.X;
      e.ytop := e.savedBot.Y;
      e.savedBot.X := e.xbot;
      e.savedBot.Y := e.ybot;
    end;
    SetDx(e, fDupPtTolerance);
  end;
  //----------------------------------------------------------------------

  function SlopesEqualInternal(e1, e2: PEdge): boolean;
  begin
    //cross product of dy1/dx1 = dy2/dx2 ...
    result := abs((e1.ytop-e1.savedBot.Y)*(e2.xtop-e2.savedBot.X) -
      (e1.xtop-e1.savedBot.X)*(e2.ytop-e2.savedBot.Y)) < fDupPtTolerance;
  end;
  //----------------------------------------------------------------------

  function FixupForDupsAndColinear(var e: PEdge): boolean;
  begin
    result := false;
    if (e.next <> e.prev) and
      (PointsEqual(e.prev.savedBot, e.savedBot, fDupPtTolerance) or
      SlopesEqualInternal(e.prev, e)) then
    begin
      //prepare to remove 'e' from the loop ...
      e.prev.xtop := e.next.savedBot.X;
      e.prev.ytop := e.next.savedBot.Y;
      if (e = @edges[0]) then
      begin
        //move the content of e.next to e, then remove e.next from the loop ...
        e.savedBot := e.next.savedBot;
        e.xtop := e.next.xtop;
        e.ytop := e.next.ytop;
        e.next.next.prev := e;
        e.next := e.next.next;
      end else
      begin
        //remove 'e' from the loop ...
        e.prev.next := e.next;
        e.next.prev := e.prev;
        e := e.prev; //ie get back into the loop
      end;
      result := true;
    end;
  end;
  //----------------------------------------------------------------------

  procedure SwapX(e: PEdge);
  begin
    e.xbot := e.xtop;
    e.xtop := e.savedBot.X;
    e.savedBot.X := e.xbot;
  end;
  //----------------------------------------------------------------------

  function BuildBound(e: PEdge; s: TEdgeSide; buildForward: boolean): PEdge;
  var
    eNext,eNextNext: PEdge;
  begin
    repeat
      e.side := s;
      if buildForward then
        eNext := e.next else
        eNext := e.prev;
      if IsHorizontal(eNext) then
      begin
        if buildForward then
          eNextNext := eNext.next else
          eNextNext := eNext.prev;
        if (eNextNext.ytop < eNext.ytop) then
        begin
          //eNext is an intermediate horizontal.
          //All horizontals have their xbot aligned with the adjoining lower edge
          if eNext.xbot <> e.xtop then swapX(eNext);
        end else if buildForward then
        begin
          //to avoid duplicating top bounds, stop if this is a
          //horizontal edge at the top of a going forward bound ...
          e.nextInLML := nil;
          result := eNext;
          break;
        end;
      end
      else if e.ytop = eNext.ytop then
      begin
        e.nextInLML := nil;
        result := eNext;
        break;
      end;
      e.nextInLML := eNext;
      e := eNext;
    until false;
  end;
  //----------------------------------------------------------------------

  procedure InsertLocalMinima(newLm: PLocalMinima);
  var
    tmpLm: PLocalMinima;
  begin
    if not assigned(fLocalMinima) then
    begin
      fLocalMinima := newLm;
    end
    else if (newLm.y >= fLocalMinima.y) then
    begin
      newLm.nextLm := fLocalMinima;
      fLocalMinima := newLm;
    end else
    begin
      tmpLm := fLocalMinima;
      while assigned(tmpLm.nextLm) and (newLm.y < tmpLm.nextLm.y) do
        tmpLm := tmpLm.nextLm;
      newLm.nextLm := tmpLm.nextLm;
      tmpLm.nextLm := newLm;
    end;
  end;
  //----------------------------------------------------------------------

  function AddLML(e: PEdge): PEdge;
  var
    newLm: PLocalMinima;
    e2: PEdge;
  begin
    new(newLm);
    newLm.nextLm := nil;
    newLm.y := e.ybot;
    e2 := e;
    repeat
      e2 := e2.next;
    until e2.ytop <> e.ybot;
    if IsHorizontal(e) then e := e.prev;

    if ((e.next <> e2) and (e.xbot < e2.xbot))
      or ((e.next = e2) and (e.dx > e2.dx)) then
    begin
      newLm.leftBound := e;
      newLm.rightBound := e.next;
      BuildBound(newLm.leftBound, esLeft, false);
      with newLm^ do
        if IsHorizontal(rightBound) and (rightBound.xbot <> leftBound.xbot) then
          SwapX(rightBound);
      result := BuildBound(newLm.rightBound, esRight, true);
    end else
    begin
      newLm.leftBound := e2;
      newLm.rightBound := e2.prev;
      with newLm^ do
        if IsHorizontal(rightBound) and (rightBound.xbot <> leftBound.xbot) then
          SwapX(rightBound);
      BuildBound(newLm.rightBound, esRight, false);
      result := BuildBound(newLm.leftBound, esLeft, true);
    end;
    InsertLocalMinima(newLm);
  end;
  //----------------------------------------------------------------------

  function NextMin(e: PEdge): PEdge;
  begin
    while e.next.ytop >= e.ybot do e := e.next;
    while e.ytop = e.ybot do e := e.prev;
    result := e;
  end;
  //----------------------------------------------------------------------

begin
  {AddPolygon}

  highI := high(polygon);
  setlength(polyg, highI +1);
  for i := 0 to highI do
  begin
    polyg[i].X := RoundToTolerance(polygon[i].X, fDupPtTolerance);
    polyg[i].Y := RoundToTolerance(polygon[i].Y, fDupPtTolerance);
  end;

  while (highI > 1) and
    PointsEqual(polyg[0], polyg[highI], fDupPtTolerance) do dec(highI);
  if highI < 2 then exit;

  //make sure this is a sensible polygon (ie with at least one minima) ...
  i := 1;
  while (i <= highI) and (polyg[i].Y = polyg[0].Y) do inc(i);
  if i > highI then exit;

  GetMem(edges, sizeof(TEdge)*(highI+1));
  //convert 'edges' to double-linked-list and initialize some of the vars ...
  edges[0].savedBot := polyg[0];
  InitEdge(@edges[highI], @edges[0], @edges[highI-1], polyg[highI]);
  for i := highI-1 downto 1 do
    InitEdge(@edges[i], @edges[i+1], @edges[i-1], polyg[i]);
  InitEdge(@edges[0], @edges[1], @edges[highI], polyg[0]);

  //fixup any duplicate points and co-linear edges ...
  e := @edges[0];
  repeat
    FixupForDupsAndColinear(e);
    e := e.next;
  until (e = @edges[0]);
  if FixupForDupsAndColinear(e) then
  begin
    e := e.prev;
    FixupForDupsAndColinear(e);
  end;

  //make sure we still have a valid polygon ...
  if (e.next = e.prev) then
  begin
    dispose(edges);
    exit; //oops!!
  end;

  fList.Add(edges);

  //now properly reinitialize edges and also get the starting minima (e2) ...
  e := @edges[0];
  e2 := e;
  repeat
    ReInitEdge(e);
    if e.ybot > e2.ybot then e2 := e;
    e := e.next;
  until e = @edges[0];

  //to avoid endless loops, make sure e2 will line up with subsequ. NextMin.
  if (e2.prev.ybot = e2.ybot) and ((e2.prev.xbot = e2.xbot) or
    (IsHorizontal(e2) and (e2.prev.xbot = e2.xtop))) then
  begin
    e2 := e2.prev;
    if IsHorizontal(e2) then e2 := e2.prev;
  end;
  //finally insert each local minima ...
  e := e2;
  repeat
    e2 := AddLML(e2);
    e2 := NextMin(e2);
  until (e2 = e);
end;
//------------------------------------------------------------------------------

procedure TClipperBase.AddPolyPolygon(const polyPolygon: TArrayOfArrayOfFloatPoint;
  polyType: TPolyType);
var
  dblPts: TArrayOfArrayOfDoublePoint;
begin
  dblPts := AAFloatPt2AADoublePt(polyPolygon);
  AddPolyPolygon(dblPts, polyType);
end;
//------------------------------------------------------------------------------

procedure TClipperBase.AddPolyPolygon(const polyPolygon: TArrayOfArrayOfDoublePoint;
  polyType: TPolyType);
var
  i: integer;
begin
  for i := 0 to high(polyPolygon) do AddPolygon(polyPolygon[i], polyType);
end;
//------------------------------------------------------------------------------

procedure TClipperBase.Clear;
var
  i: Integer;
begin
  DisposeLocalMinimaList;
  for i := 0 to fList.Count -1 do dispose(PEdgeArray(fList[i]));
  fList.Clear;
end;
//------------------------------------------------------------------------------

function TClipperBase.Reset: boolean;
var
  e: PEdge;
  lm: PLocalMinima;
begin
  //Reset() allows various clipping operations to be executed
  //multiple times on the same polygon sets. (Protected method.)

  while assigned(fLocalMinima) do PopLocalMinima;
  if assigned(fRecycledLocMin) then fLocalMinima := fRecycledLocMin;
  fRecycledLocMin := nil;
  fRecycledLocMinEnd := nil;
  result := assigned(fLocalMinima);
  if not result then exit; //ie nothing to process

  //reset all edges ...
  lm := fLocalMinima;
  while assigned(lm) do
  begin
    e := lm.leftBound;
    while assigned(e) do
    begin
      e.xbot := e.savedBot.X;
      e.ybot := e.savedBot.Y;
      e.side := esLeft;
      e.outIdx := -1;
      e := e.nextInLML;
    end;
    e := lm.rightBound;
    while assigned(e) do
    begin
      e.xbot := e.savedBot.X;
      e.ybot := e.savedBot.Y;
      e.side := esRight;
      e.outIdx := -1;
      e := e.nextInLML;
    end;
    lm := lm.nextLm;
  end;
end;
//------------------------------------------------------------------------------

procedure TClipperBase.PopLocalMinima;
var
  tmpLm: PLocalMinima;
begin
  if not assigned(fLocalMinima) then
    exit
  else if not assigned(fRecycledLocMin) then
  begin
    fRecycledLocMinEnd := fLocalMinima;
    fRecycledLocMin := fLocalMinima;
    fLocalMinima := fLocalMinima.nextLm;
    fRecycledLocMin.nextLm := nil;
  end else
  begin
    tmpLm := fLocalMinima.nextLm;
    fLocalMinima.nextLm := nil;
    fRecycledLocMinEnd.nextLm := fLocalMinima;
    fRecycledLocMinEnd := fLocalMinima;
    fLocalMinima := tmpLm;
  end;
end;
//------------------------------------------------------------------------------

procedure TClipperBase.DisposeLocalMinimaList;
var
  tmpLm: PLocalMinima;
begin
  while assigned(fLocalMinima) do
  begin
    tmpLm := fLocalMinima.nextLm;
    Dispose(fLocalMinima);
    fLocalMinima := tmpLm;
  end;
  while assigned(fRecycledLocMin) do
  begin
    tmpLm := fRecycledLocMin.nextLm;
    Dispose(fRecycledLocMin);
    fRecycledLocMin := tmpLm;
  end;
  fRecycledLocMinEnd := nil;
end;

//------------------------------------------------------------------------------
// TClipper methods ...
//------------------------------------------------------------------------------

constructor TClipper.Create;
begin
  inherited;
  fPolyPtList := TList.Create;
  fForceAlternateOrientation := true;
end;
//------------------------------------------------------------------------------

destructor TClipper.Destroy;
begin
  DisposeScanbeamList;
  fPolyPtList.Free;
  inherited;
end;
//------------------------------------------------------------------------------

function TClipper.Execute(clipType: TClipType;
  out solution: TArrayOfArrayOfFloatPoint;
  subjFillType: TPolyFillType = pftEvenOdd;
  clipFillType: TPolyFillType = pftEvenOdd): boolean;
begin
  result := false;
  if fExecuteLocked then exit;
  try
    fExecuteLocked := true;
    fSubjFillType := subjFillType;
    fClipFillType := clipFillType;
    result := ExecuteInternal(clipType);
    if result then solution := ResultAsFloatPointArray;
  finally
    DisposeAllPolyPts;
    fExecuteLocked := false;
  end;
end;
//------------------------------------------------------------------------------

function TClipper.Execute(clipType: TClipType;
  out solution: TArrayOfArrayOfDoublePoint;
  subjFillType: TPolyFillType = pftEvenOdd;
  clipFillType: TPolyFillType = pftEvenOdd): boolean;
begin
  result := false;
  if fExecuteLocked then exit;
  try
    fExecuteLocked := true;
    fSubjFillType := subjFillType;
    fClipFillType := clipFillType;
    result := ExecuteInternal(clipType);
    if result then solution := ResultAsDoublePointArray;
  finally
    DisposeAllPolyPts;
    fExecuteLocked := false;
  end;
end;
//------------------------------------------------------------------------------

procedure TClipper.DisposeAllPolyPts;
var
  i: integer;
begin
  for i := 0 to fPolyPtList.Count -1 do
    if assigned(fPolyPtList[i]) then
      DisposePolyPts(PPolyPt(fPolyPtList[i]));
  fPolyPtList.Clear;
end;
//------------------------------------------------------------------------------

function TClipper.ExecuteInternal(clipType: TClipType): boolean;
var
  yBot, yTop: double;
begin
  result := false;
  try
    if not InitializeScanbeam then exit;
    fActiveEdges := nil;
    fSortedEdges := nil;
    fClipType := clipType;
    yBot := PopScanbeam;
    repeat
      InsertLocalMinimaIntoAEL(yBot);
      ProcessHorizontals;
      yTop := PopScanbeam;
      ProcessIntersections(yTop);
      ProcessEdgesAtTopOfScanbeam(yTop);
      yBot := yTop;
    until not assigned(fScanbeam);
    result := true;
  except
    //result := false;
  end;
end;
//------------------------------------------------------------------------------

function TClipper.ResultAsFloatPointArray: TArrayOfArrayOfFloatPoint;
var
  i,j,k,cnt: integer;
  pt: PPolyPt;
  y: double;
  isHorizontalOnly: boolean;
begin
  k := 0;
  setLength(result, fPolyPtList.Count);
  for i := 0 to fPolyPtList.Count -1 do
    if assigned(fPolyPtList[i]) then
    begin
      FixupSolutionColinears(fPolyPtList, i, fDupPtTolerance);

      cnt := 0;
      pt := PPolyPt(fPolyPtList[i]);
      isHorizontalOnly := true;
      y := pt.pt.Y;
      repeat
        pt := pt.next;
        if isHorizontalOnly and (abs(pt.pt.Y - y) > fDupPtTolerance) then
          isHorizontalOnly := false;
        inc(cnt);
      until (pt = PPolyPt(fPolyPtList[i]));
      if (cnt < 3) or isHorizontalOnly then continue;

      //optionally validate the orientation of simple polygons ...
      pt := PPolyPt(fPolyPtList[i]);
      if fForceAlternateOrientation and
        not ValidateOrientation(pt) then ReversePolyPtLinks(pt);

      setLength(result[k], cnt);
      for j := 0 to cnt -1 do
      begin
        result[k][j].X := pt.pt.X;
        result[k][j].Y := pt.pt.Y;
        pt := pt.next;
      end;
      inc(k);
    end;
  setLength(result, k);
end;
//------------------------------------------------------------------------------

function TClipper.ResultAsDoublePointArray: TArrayOfArrayOfDoublePoint;
var
  i,j,k,cnt: integer;
  pt: PPolyPt;
  y: double;
  isHorizontalOnly: boolean;
begin
  k := 0;
  setLength(result, fPolyPtList.Count);
  for i := 0 to fPolyPtList.Count -1 do
    if assigned(fPolyPtList[i]) then
    begin
      FixupSolutionColinears(fPolyPtList, i, fDupPtTolerance);

      cnt := 0;
      pt := PPolyPt(fPolyPtList[i]);
      isHorizontalOnly := true;
      y := pt.pt.Y;
      repeat
        pt := pt.next;
        if isHorizontalOnly and (abs(pt.pt.Y - y) > fDupPtTolerance) then
          isHorizontalOnly := false;
        inc(cnt);
      until (pt = PPolyPt(fPolyPtList[i]));
      if (cnt < 3) or isHorizontalOnly then continue;

      //optionally validate the orientation of simple polygons ...
      pt := PPolyPt(fPolyPtList[i]);
      if fForceAlternateOrientation and
        not ValidateOrientation(pt) then ReversePolyPtLinks(pt);

      setLength(result[k], cnt);
      for j := 0 to cnt -1 do
      begin
        result[k][j].X := pt.pt.X;
        result[k][j].Y := pt.pt.Y;
        pt := pt.next;
      end;
      inc(k);
    end;
  setLength(result, k);
end;
//------------------------------------------------------------------------------

procedure TClipper.DisposeScanbeamList;
var
  sb2: PScanbeam;
begin
  while assigned(fScanbeam) do
  begin
    sb2 := fScanbeam.nextSb;
    Dispose(fScanbeam);
    fScanbeam := sb2;
  end;
end;
//------------------------------------------------------------------------------

function TClipper.InitializeScanbeam: boolean;
var
  lm: PLocalMinima;
begin
  DisposeScanbeamList;
  result := Reset; //returns false when no polygons to process
  if not result then exit;
  //add all the local minima into a fresh fScanbeam list ...
  lm := fLocalMinima;
  while assigned(lm) do
  begin
    InsertScanbeam(lm.y);
    InsertScanbeam(lm.leftBound.ytop); //this is necessary too!
    lm := lm.nextLm;
  end;
end;
//------------------------------------------------------------------------------

procedure TClipper.InsertScanbeam(const y: double);
var
  newSb, sb2: PScanbeam;
begin
  if not assigned(fScanbeam) then
  begin
    new(fScanbeam);
    fScanbeam.nextSb := nil;
    fScanbeam.y := y;
  end else if y > fScanbeam.y then
  begin
    new(newSb);
    newSb.y := y;
    newSb.nextSb := fScanbeam;
    fScanbeam := newSb;
  end else
  begin
    sb2 := fScanbeam;
    while assigned(sb2.nextSb) and (y <= sb2.nextSb.y) do sb2 := sb2.nextSb;
    if y = sb2.y then exit; //ie ignores duplicates
    new(newSb);
    newSb.y := y;
    newSb.nextSb := sb2.nextSb;
    sb2.nextSb := newSb;
  end;
end;
//------------------------------------------------------------------------------

function TClipper.PopScanbeam: double;
var
  sb2: PScanbeam;
begin
  result := fScanbeam.y;
  sb2 := fScanbeam;
  fScanbeam := fScanbeam.nextSb;
  dispose(sb2);
end;
//------------------------------------------------------------------------------

procedure TClipper.SetWindingDelta(edge: PEdge);
begin
  if not IsNonZeroFillType(edge) then edge.windDelta := 1
  else if NextEdgeIsAbove(edge) then edge.windDelta := 1
  else edge.windDelta := -1;
end;
//------------------------------------------------------------------------------

procedure TClipper.SetWindingCount(edge: PEdge);
var
  e: PEdge;
begin
  e := edge.prevInAEL;
  //find the edge of the same polytype that immediately preceeds 'edge' in AEL
  while assigned(e) and (e.polyType <> edge.polyType) do e := e.prevInAEL;
  if not assigned(e) then
  begin
    edge.windCnt := edge.windDelta;
    edge.windCnt2 := 0;
    e := fActiveEdges; //ie get ready to calc windCnt2
  end else if IsNonZeroFillType(edge) then
  begin
    //nonZero filling ...
    if e.windCnt * e.windDelta < 0 then
    begin
      if (abs(e.windCnt) > 1) then
      begin
        if (e.windDelta * edge.windDelta < 0) then edge.windCnt := e.windCnt
        else edge.windCnt := e.windCnt + edge.windDelta;
      end else
        edge.windCnt := e.windCnt + e.windDelta + edge.windDelta;
    end else
    begin
      if (abs(e.windCnt) > 1) and (e.windDelta * edge.windDelta < 0) then
        edge.windCnt := e.windCnt
      else if e.windCnt + edge.windDelta = 0 then
        edge.windCnt := e.windCnt
      else edge.windCnt := e.windCnt + edge.windDelta;
    end;
    edge.windCnt2 := e.windCnt2;
    e := e.nextInAEL; //ie get ready to calc windCnt2
  end else
  begin
    //even-odd filling ...
    edge.windCnt := 1;
    edge.windCnt2 := e.windCnt2;
    e := e.nextInAEL; //ie get ready to calc windCnt2
  end;

  //update windCnt2 ...
  if (edge.polyType = ptSubject) and (fClipFillType = pftNonZero) or
    (edge.polyType = ptClip) and (fSubjFillType = pftNonZero) then
  begin
    //nonZero filling ...
    while (e <> edge) do
    begin
      inc(edge.windCnt2, e.windDelta);
      e := e.nextInAEL;
    end;
  end else
  begin
    //even-odd filling ...
    while (e <> edge) do
    begin
      if edge.windCnt2 = 0 then edge.windCnt2 := 1 else edge.windCnt2 := 0;
      e := e.nextInAEL;
    end;
  end;
end;
//------------------------------------------------------------------------------

function TClipper.IsNonZeroFillType(edge: PEdge): boolean;
begin
  result := (edge.polyType = ptSubject) and (fSubjFillType = pftNonZero) or
    (edge.polyType = ptClip) and (fClipFillType = pftNonZero);
end;
//------------------------------------------------------------------------------

procedure TClipper.InsertLocalMinimaIntoAEL(const botY: double);

  //----------------------------------------------------------------------
  function Edge2InsertsBeforeEdge1(e1,e2: PEdge): boolean;
  begin
    if (e2.xbot - tolerance > e1.xbot) then result := false
    else if (e2.xbot + tolerance < e1.xbot) then result := true
    else if IsHorizontal(e2) then result := false
    else if SlopesEqual(e1, e2, fDupPtTolerance) then result := false
    else result := e2.dx > e1.dx;
  end;
  //----------------------------------------------------------------------

  procedure InsertEdgeIntoAEL(edge: PEdge);
  var
    e: PEdge;
  begin
    edge.prevInAEL := nil;
    edge.nextInAEL := nil;
    if not assigned(fActiveEdges) then
    begin
      fActiveEdges := edge;
    end else if Edge2InsertsBeforeEdge1(fActiveEdges, edge) then
    begin
      edge.nextInAEL := fActiveEdges;
      fActiveEdges.prevInAEL := edge;
      fActiveEdges := edge;
    end else
    begin
      e := fActiveEdges;
      while assigned(e.nextInAEL) and
        not Edge2InsertsBeforeEdge1(e.nextInAEL, edge) do e := e.nextInAEL;
      edge.nextInAEL := e.nextInAEL;
      if assigned(e.nextInAEL) then e.nextInAEL.prevInAEL := edge;
      edge.prevInAEL := e;
      e.nextInAEL := edge;
    end;
  end;
  //----------------------------------------------------------------------

var
  e: PEdge;
  pt: TDoublePoint;
begin
  {InsertLocalMinimaIntoAEL}
  while assigned(fLocalMinima) and (fLocalMinima.y = botY) do
  begin
    InsertEdgeIntoAEL(fLocalMinima.leftBound);
    InsertScanbeam(fLocalMinima.leftBound.ytop);
    InsertEdgeIntoAEL(fLocalMinima.rightBound);

    //set edge winding states ...
    with fLocalMinima^ do
    begin
      SetWindingDelta(leftBound);
      if IsNonZeroFillType(leftBound) then
        rightBound.windDelta := -leftBound.windDelta else
        rightBound.windDelta := 1;
      SetWindingCount(leftBound);
      rightBound.windCnt := leftBound.windCnt;
      rightBound.windCnt2 := leftBound.windCnt2;

      e := leftBound.nextInAEL;
      if IsHorizontal(rightBound) then
      begin
        //nb: only rightbounds can have a horizontal bottom edge
        AddHorzEdgeToSEL(rightBound);
        InsertScanbeam(rightBound.nextInLML.ytop);
      end else
        InsertScanbeam(rightBound.ytop);

      if IsContributing(leftBound) then
        AddLocalMinPoly(leftBound, rightBound, DoublePoint(leftBound.xbot, y));
      if (leftBound.nextInAEL <> rightBound) then
      begin
        e := leftBound.nextInAEL;
        pt := DoublePoint(leftBound.xbot,leftBound.ybot);
        while e <> rightBound do
        begin
          if not assigned(e) then raise exception.Create(rsMissingRightbound);
          IntersectEdges(rightBound, e, pt);
          e := e.nextInAEL;
        end;
      end;
    end;

    PopLocalMinima;
  end;
end;
//------------------------------------------------------------------------------

procedure TClipper.AddHorzEdgeToSEL(edge: PEdge);
begin
  //SEL pointers in PEdge are reused to build a list of horizontal edges.
  //Also, we don't need to worry about order with horizontal edge processing ...
  if not assigned(fSortedEdges) then
  begin
    fSortedEdges := edge;
    edge.prevInSEL := nil;
    edge.nextInSEL := nil;
  end else
  begin
    edge.nextInSEL := fSortedEdges;
    edge.prevInSEL := nil;
    fSortedEdges.prevInSEL := edge;
    fSortedEdges := edge;
  end;
end;
//------------------------------------------------------------------------------

procedure TClipper.SwapPositionsInAEL(edge1, edge2: PEdge);
var
  prev,next: PEdge;
begin
  with edge1^ do if not assigned(nextInAEL) and not assigned(prevInAEL) then exit;
  with edge2^ do if not assigned(nextInAEL) and not assigned(prevInAEL) then exit;

  if edge1.nextInAEL = edge2 then
  begin
    next    := edge2.nextInAEL;
    if assigned(next) then next.prevInAEL := edge1;
    prev    := edge1.prevInAEL;
    if assigned(prev) then prev.nextInAEL := edge2;
    edge2.prevInAEL := prev;
    edge2.nextInAEL := edge1;
    edge1.prevInAEL := edge2;
    edge1.nextInAEL := next;
  end
  else if edge2.nextInAEL = edge1 then
  begin
    next    := edge1.nextInAEL;
    if assigned(next) then next.prevInAEL := edge2;
    prev    := edge2.prevInAEL;
    if assigned(prev) then prev.nextInAEL := edge1;
    edge1.prevInAEL := prev;
    edge1.nextInAEL := edge2;
    edge2.prevInAEL := edge1;
    edge2.nextInAEL := next;
  end else
  begin
    next    := edge1.nextInAEL;
    prev    := edge1.prevInAEL;
    edge1.nextInAEL := edge2.nextInAEL;
    if assigned(edge1.nextInAEL) then edge1.nextInAEL.prevInAEL := edge1;
    edge1.prevInAEL := edge2.prevInAEL;
    if assigned(edge1.prevInAEL) then edge1.prevInAEL.nextInAEL := edge1;
    edge2.nextInAEL := next;
    if assigned(edge2.nextInAEL) then edge2.nextInAEL.prevInAEL := edge2;
    edge2.prevInAEL := prev;
    if assigned(edge2.prevInAEL) then edge2.prevInAEL.nextInAEL := edge2;
  end;
  if not assigned(edge1.prevInAEL) then fActiveEdges := edge1
  else if not assigned(edge2.prevInAEL) then fActiveEdges := edge2;
end;
//------------------------------------------------------------------------------

function GetNextInAEL(e: PEdge; Direction: TDirection): PEdge;
begin
  if Direction = dLeftToRight then
    result := e.nextInAEL else
    result := e.prevInAEL;
end;
//------------------------------------------------------------------------------

function GetPrevInAEL(e: PEdge; Direction: TDirection): PEdge;
begin
  if Direction = dLeftToRight then
    result := e.prevInAEL else
    result := e.nextInAEL;
end;
//------------------------------------------------------------------------------

function IsMinima(e: PEdge): boolean;
begin
  result := assigned(e) and (e.prev.nextInLML <> e) and (e.next.nextInLML <> e);
end;
//------------------------------------------------------------------------------

function IsMaxima(e: PEdge; const Y: double): boolean;
begin
  result := assigned(e) and (e.ytop = Y) and not assigned(e.nextInLML);
end;
//------------------------------------------------------------------------------

function IsIntermediate(e: PEdge; const Y: double): boolean;
begin
  result := (e.ytop = Y) and assigned(e.nextInLML);
end;
//------------------------------------------------------------------------------

function TClipper.GetMaximaPair(e: PEdge): PEdge;
begin
  result := e.next;
  if not IsMaxima(result, e.ytop) or (result.xtop <> e.xtop) then result := e.prev;
end;
//------------------------------------------------------------------------------

procedure TClipper.DoMaxima(e: PEdge; const topY: double);
var
  eNext, eMaxPair: PEdge;
  X: double;
begin
  eMaxPair := GetMaximaPair(e);
  X := e.xtop;
  eNext := e.nextInAEL;
  while eNext <> eMaxPair do
  begin
    IntersectEdges(e, eNext, DoublePoint(X, topY), [ipLeft, ipRight]);
    eNext := eNext.nextInAEL;
  end;
  if (e.outIdx < 0) and (eMaxPair.outIdx < 0) then
  begin
    DeleteFromAEL(e);
    DeleteFromAEL(eMaxPair);
  end
  else if (e.outIdx >= 0) and (eMaxPair.outIdx >= 0) then
  begin
    IntersectEdges(e, eMaxPair, DoublePoint(X, topY));
  end
  else raise exception.Create(rsDoMaxima);
end;
//------------------------------------------------------------------------------

procedure TClipper.ProcessHorizontals;
var
  horzEdge: PEdge;
begin
  horzEdge := fSortedEdges;
  while assigned(horzEdge) do
  begin
    DeleteFromSEL(horzEdge);
    ProcessHorizontal(horzEdge);
    horzEdge := fSortedEdges;
  end;
end;
//------------------------------------------------------------------------------

function TClipper.IsTopHorz(horzEdge: PEdge; const XPos: double): boolean;
var
  e: PEdge;
begin
  result := false;
  e := fSortedEdges;
  while assigned(e) do
  begin
    if (XPos >= min(e.xbot,e.xtop)) and (XPos <= max(e.xbot,e.xtop)) then exit;
    e := e.nextInSEL;
  end;
  result := true;
end;
//------------------------------------------------------------------------------

procedure TClipper.ProcessHorizontal(horzEdge: PEdge);
var
  e, eNext, eMaxPair: PEdge;
  horzLeft, horzRight: double;
  Direction: TDirection;
const
  ProtectLeft: array[boolean] of TIntersectProtects = ([ipRight], [ipLeft,ipRight]);
  ProtectRight: array[boolean] of TIntersectProtects = ([ipLeft], [ipLeft,ipRight]);
begin
(*******************************************************************************
* Notes: Horizontal edges (HEs) at scanline intersections (ie at the top or    *
* bottom of a scanbeam) are processed as if layered. The order in which HEs    *
* are processed doesn't matter. HEs intersect with other HE xbots only [#],    *
* and with other non-horizontal edges [*]. Once these intersections are        *
* processed, intermediate HEs then 'promote' the edge above (nextInLML) into   *
* the AEL. These 'promoted' edges may in turn intersect [%] with other HEs.    *
*******************************************************************************)

(*******************************************************************************
*           \   nb: HE processing order doesn't matter         /          /    *
*            \                                                /          /     *
* { --------  \  -------------------  /  \  - (3) o==========%==========o  - } *
* {            o==========o (2)      /    \       .          .               } *
* {                       .         /      \      .          .               } *
* { ----  o===============#========*========*=====#==========o  (1)  ------- } *
*        /                 \      /          \   /                             *
*******************************************************************************)

  if horzEdge.xbot < horzEdge.xtop then
  begin
    horzLeft := horzEdge.xbot; horzRight := horzEdge.xtop;
    Direction := dLeftToRight;
  end else
  begin
    horzLeft := horzEdge.xtop; horzRight := horzEdge.xbot;
    Direction := dRightToLeft;
  end;

  if assigned(horzEdge.nextInLML) then
    eMaxPair := nil else
    eMaxPair := GetMaximaPair(horzEdge);

  e := GetNextInAEL(horzEdge, Direction);
  while assigned(e) do
  begin
    eNext := GetNextInAEL(e, Direction);
    if (e.xbot >= horzLeft - tolerance) and (e.xbot <= horzRight + tolerance) then
    begin
      //ok, so far it looks like we're still in range of the horizontal edge
      if (abs(e.xbot - horzEdge.xtop) < tolerance) and
        assigned(horzEdge.nextInLML) and
        (SlopesEqual(e, horzEdge.nextInLML, fDupPtTolerance) or
        (e.dx < horzEdge.nextInLML.dx)) then
      begin
        //we really have gone past the end of intermediate horz edge so quit.
        //nb: More -ve slopes follow more +ve slopes *above* the horizontal.
        break;
      end
      else if (e = eMaxPair) then
      begin
        //horzEdge is evidently a maxima horizontal and we've arrived at its end.
        if Direction = dLeftToRight then
          IntersectEdges(horzEdge, e, DoublePoint(e.xbot, horzEdge.ybot),[]) else
          IntersectEdges(e, horzEdge, DoublePoint(e.xbot, horzEdge.ybot),[]);
        exit;
      end
      else if IsHorizontal(e) and not IsMinima(e) and not (e.xbot > e.xtop) then
      begin
        //An overlapping horizontal edge. Overlapping horizontal edges are
        //processed as if layered with the current horizontal edge (horizEdge)
        //being infinitesimally lower that the next (e). Therfore, we
        //intersect with e only if e.xbot is within the bounds of horzEdge ...
        if Direction = dLeftToRight then
          IntersectEdges(horzEdge, e, DoublePoint(e.xbot, horzEdge.ybot),
            ProtectRight[not IsTopHorz(horzEdge, e.xbot)])
        else
          IntersectEdges(e, horzEdge, DoublePoint(e.xbot, horzEdge.ybot),
            ProtectLeft[not IsTopHorz(horzEdge, e.xbot)]);
      end
      else if (Direction = dLeftToRight) then
      begin
        IntersectEdges(horzEdge, e, DoublePoint(e.xbot, horzEdge.ybot),
          ProtectRight[not IsTopHorz(horzEdge, e.xbot)])
      end else
      begin
        IntersectEdges(e, horzEdge, DoublePoint(e.xbot, horzEdge.ybot),
          ProtectLeft[not IsTopHorz(horzEdge, e.xbot)]);
      end;
      SwapPositionsInAEL(horzEdge, e);
    end
    else if (Direction = dLeftToRight) and
      (e.xbot > horzRight + tolerance) and not assigned(horzEdge.nextInSEL) then
        break
    else if (Direction = dRightToLeft) and
      (e.xbot < horzLeft - tolerance) and not assigned(horzEdge.nextInSEL) then
        break;
    e := eNext;
  end;

  if assigned(horzEdge.nextInLML) then
  begin
    if (horzEdge.outIdx >= 0) then
      AddPolyPt(horzEdge.outIdx,
        DoublePoint(horzEdge.xtop, horzEdge.ytop), horzEdge.side = esLeft);
    UpdateEdgeIntoAEL(horzEdge);
  end else
    DeleteFromAEL(horzEdge);
end;
//------------------------------------------------------------------------------

function TClipper.AddPolyPt(idx: integer;
  const pt: TDoublePoint; ToFront: boolean): integer;
var
  fp, newPolyPt: PPolyPt;
begin
  if idx < 0 then
  begin
    new(newPolyPt);
    newPolyPt.pt := pt;
    result := fPolyPtList.Add(newPolyPt);
    newPolyPt.next := newPolyPt;
    newPolyPt.prev := newPolyPt;
    newPolyPt.isHole := sUndefined;
  end else
  begin
    result := idx;
    fp := PPolyPt(fPolyPtList[idx]);
    if (ToFront and PointsEqual(pt, fp.pt, fDupPtTolerance)) or
      (not ToFront and PointsEqual(pt, fp.prev.pt, fDupPtTolerance)) then exit;
    new(newPolyPt);
    newPolyPt.pt := pt;
    newPolyPt.isHole := sUndefined;
    newPolyPt.next := fp;
    newPolyPt.prev := fp.prev;
    newPolyPt.prev.next := newPolyPt;
    fp.prev := newPolyPt;
    if ToFront then fPolyPtList[idx] := newPolyPt;
  end;
end;
//------------------------------------------------------------------------------

procedure TClipper.ProcessIntersections(const topY: double);
var
  iNode: PIntersectNode;
begin
  if not assigned(fActiveEdges) then exit;
  try
    BuildIntersectList(topY);
    ProcessIntersectList;
  finally
    //if there's been an error, clean up the mess ...
    while assigned(fIntersectNodes) do
    begin
      iNode := fIntersectNodes.next;
      dispose(fIntersectNodes);
      fIntersectNodes := iNode;
    end;
  end;
end;
//------------------------------------------------------------------------------

function Process1Before2(Node1, Node2: PIntersectNode; const epsilon: double): boolean;

  function E1PrecedesE2inAEL(e1, e2: PEdge): boolean;
  begin
    result := true;
    while assigned(e1) do
      if e1 = e2 then exit else e1 := e1.nextInAEL;
    result := false;
  end;

begin
  if (abs(Node1.pt.Y - Node2.pt.Y) < tolerance) then
  begin
    if SlopesEqual(Node1.edge1, Node2.edge1, epsilon) then
    begin
      if SlopesEqual(Node1.edge2, Node2.edge2, epsilon) then
      begin
        //nb: probably overlapping co-linear segments to get here
        if Node1.edge2 = Node2.edge2 then
          result := E1PrecedesE2inAEL(Node2.edge1, Node1.edge1) else
          result := E1PrecedesE2inAEL(Node1.edge2, Node2.edge2);
      end else
        result := (Node1.edge2.dx < Node2.edge2.dx)
    end else
      result := (Node1.edge1.dx < Node2.edge1.dx);
  end else
    result := (Node1.pt.Y > Node2.pt.Y);
end;
//------------------------------------------------------------------------------

procedure TClipper.AddIntersectNode(e1, e2: PEdge; const pt: TDoublePoint);
var
  IntersectNode, iNode: PIntersectNode;
begin
  new(IntersectNode);
  IntersectNode.edge1 := e1;
  IntersectNode.edge2 := e2;
  IntersectNode.pt := pt;
  IntersectNode.next := nil;
  IntersectNode.prev := nil;
  if not assigned(fIntersectNodes) then
    fIntersectNodes := IntersectNode
  else if Process1Before2(IntersectNode,fIntersectNodes,fDupPtTolerance) then
  begin
    IntersectNode.next := fIntersectNodes;
    fIntersectNodes.prev := IntersectNode;
    fIntersectNodes := IntersectNode;
  end else
  begin
    iNode := fIntersectNodes;
    while assigned(iNode.next) and
      Process1Before2(iNode.next, IntersectNode, fDupPtTolerance) do
      iNode := iNode.next;
    if assigned(iNode.next) then iNode.next.prev := IntersectNode;
    IntersectNode.next := iNode.next;
    IntersectNode.prev := iNode;
    iNode.next := IntersectNode;
  end;
end;
//------------------------------------------------------------------------------

procedure TClipper.BuildIntersectList(const topY: double);
var
  e, eNext: PEdge;
  pt: TDoublePoint;
begin
  //prepare for sorting ...
  e := fActiveEdges;
  e.tmpX := TopX(e, topY);
  fSortedEdges := e;
  fSortedEdges.prevInSEL := nil;
  e := e.nextInAEL;
  while assigned(e) do
  begin
    e.prevInSEL := e.prevInAEL;
    e.prevInSEL.nextInSEL := e;
    e.nextInSEL := nil;
    e.tmpX := TopX(e, topY);
    e := e.nextInAEL;
  end;

  try
    //bubblesort ...
    while assigned(fSortedEdges) do
    begin
      e := fSortedEdges;
      while assigned(e.nextInSEL) do
      begin
        eNext := e.nextInSEL;
        if (e.tmpX > eNext.tmpX + tolerance) and
          IntersectPoint(e, eNext, pt) then
        begin
          AddIntersectNode(e, eNext, pt);
          SwapWithNextInSEL(e);
        end else
          e := eNext;
      end;
      if assigned(e.prevInSEL) then
        e.prevInSEL.nextInSEL := nil else
        break;
    end;
  finally
    fSortedEdges := nil;
  end;
end;
//------------------------------------------------------------------------------

procedure TClipper.ProcessIntersectList;
var
  iNode: PIntersectNode;
begin
  while assigned(fIntersectNodes) do
  begin
    iNode := fIntersectNodes.next;
    with fIntersectNodes^ do
    begin
      IntersectEdges(edge1, edge2, pt);
      SwapPositionsInAEL(edge1, edge2);
    end;
    dispose(fIntersectNodes);
    fIntersectNodes := iNode;
  end;
end;
//------------------------------------------------------------------------------

procedure TClipper.IntersectEdges(e1,e2: PEdge;
  const pt: TDoublePoint; protects: TIntersectProtects = []);

  procedure DoEdge1;
  begin
    AddPolyPt(e1.outIdx, pt, e1.side = esLeft);
    SwapSides(e1, e2);
    SwapPolyIndexes(e1, e2);
  end;
  //----------------------------------------------------------------------

  procedure DoEdge2;
  begin
    AddPolyPt(e2.outIdx, pt, e2.side = esLeft);
    SwapSides(e1, e2);
    SwapPolyIndexes(e1, e2);
  end;
  //----------------------------------------------------------------------

  procedure DoBothEdges;
  begin
    AddPolyPt(e1.outIdx, pt, e1.side = esLeft);
    AddPolyPt(e2.outIdx, pt, e2.side = esLeft);
    SwapSides(e1, e2);
    SwapPolyIndexes(e1, e2);
  end;
  //----------------------------------------------------------------------

var
  oldE1WindCnt: integer;
  e1stops, e2stops: boolean;
  e1Contributing, e2contributing: boolean;
begin
  {IntersectEdges}

  //nb: e1 always precedes e2 in AEL ...
  e1stops := not (ipLeft in protects) and not assigned(e1.nextInLML) and
    (abs(e1.xtop - pt.x) < tolerance) and (abs(e1.ytop - pt.y) < tolerance);
  e2stops := not (ipRight in protects) and not assigned(e2.nextInLML) and
    (abs(e2.xtop - pt.x) < tolerance) and (abs(e2.ytop - pt.y) < tolerance);
  e1Contributing := (e1.outIdx >= 0);
  e2contributing := (e2.outIdx >= 0);

  //update winding counts ...
  if e1.polyType = e2.polyType then
  begin
    if IsNonZeroFillType(e1) then
    begin
      if e1.windCnt + e2.windDelta = 0 then
        e1.windCnt := -e1.windCnt else
        inc(e1.windCnt, e2.windDelta);
      if e2.windCnt - e1.windDelta = 0 then
        e2.windCnt := -e2.windCnt else
        dec(e2.windCnt, e1.windDelta);
    end else
    begin
      oldE1WindCnt := e1.windCnt;
      e1.windCnt := e2.windCnt;
      e2.windCnt := oldE1WindCnt;
    end;
  end else
  begin
    if IsNonZeroFillType(e2) then inc(e1.windCnt2, e2.windDelta)
    else if e1.windCnt2 = 0 then e1.windCnt2 := 1
    else e1.windCnt2 := 0;
    if IsNonZeroFillType(e1) then dec(e2.windCnt2, e1.windDelta)
    else if e2.windCnt2 = 0 then e2.windCnt2 := 1
    else e2.windCnt2 := 0;
  end;

  if e1Contributing and e2contributing then
  begin
    if e1stops or e2stops or
      (abs(e1.windCnt) > 1) or (abs(e2.windCnt) > 1) or
      ((e1.polytype <> e2.polytype) and (fClipType <> ctXor)) then
        AddLocalMaxPoly(e1, e2, pt) else
        DoBothEdges;
  end
  else if e1Contributing then
  begin
    case fClipType of
      ctIntersection: if (abs(e2.windCnt) < 2) and
        ((e2.polyType = ptSubject) or (e2.windCnt2 <> 0)) then DoEdge1;
      else
        if (abs(e2.windCnt) < 2) then DoEdge1;
    end;
  end
  else if e2contributing then
  begin
    case fClipType of
      ctIntersection: if (abs(e1.windCnt) < 2) and
        ((e1.polyType = ptSubject) or (e1.windCnt2 <> 0)) then DoEdge2;
      else
        if (abs(e1.windCnt) < 2) then DoEdge2;
    end;
  end
  else
  begin
    //neither edge is currently contributing ...
    if (abs(e1.windCnt) > 1) and (abs(e2.windCnt) > 1) then
      // do nothing
    else if (e1.polytype <> e2.polytype) and
      not e1stops and not e2stops and
      (abs(e1.windCnt) < 2) and (abs(e2.windCnt) < 2)then
      AddLocalMinPoly(e1, e2, pt)
    else if (abs(e1.windCnt) = 1) and (abs(e2.windCnt) = 1) then
      case fClipType of
        ctIntersection:
          if (abs(e1.windCnt2) > 0) and (abs(e2.windCnt2) > 0) then
            AddLocalMinPoly(e1, e2, pt);
        ctUnion:
          if (e1.windCnt2 = 0) and (e2.windCnt2 = 0) then
            AddLocalMinPoly(e1, e2, pt);
        ctDifference:
          if ((e1.polyType = ptClip) and (e2.polyType = ptClip) and
            (e1.windCnt2 <> 0) and (e2.windCnt2 <> 0)) or
            ((e1.polyType = ptSubject) and (e2.polyType = ptSubject) and
            (e1.windCnt2 = 0) and (e2.windCnt2 = 0)) then
              AddLocalMinPoly(e1, e2, pt);
        ctXor:
          AddLocalMinPoly(e1, e2, pt);
      end
    else if (abs(e1.windCnt) < 2) and (abs(e2.windCnt) < 2) then
      swapsides(e1,e2);
  end;

  if (e1stops <> e2stops) and
    ((e1stops and (e1.outIdx >= 0)) or (e2stops and (e2.outIdx >= 0))) then
  begin
    swapsides(e1,e2);
    SwapPolyIndexes(e1, e2);
  end;

  //finally, delete any non-contributing maxima edges  ...
  if e1stops then deleteFromAEL(e1);
  if e2stops then deleteFromAEL(e2);
end;
//------------------------------------------------------------------------------

function TClipper.IsContributing(edge: PEdge): boolean;
begin
  result := true;
  case fClipType of
    ctIntersection:
      begin
        if edge.polyType = ptSubject then
          result := (abs(edge.windCnt) = 1) and (edge.windCnt2 <> 0) else
          result := (edge.windCnt2 <> 0) and (abs(edge.windCnt) = 1);
      end;
    ctUnion:
      begin
        result := (abs(edge.windCnt) = 1) and (edge.windCnt2 = 0);
      end;
    ctDifference:
      begin
        if edge.polyType = ptSubject then
        result := (abs(edge.windCnt) = 1) and (edge.windCnt2 = 0) else
        result := (abs(edge.windCnt) = 1) and (edge.windCnt2 <> 0);
      end;
    ctXor:
      result := (abs(edge.windCnt) = 1);
  end;
end;
//------------------------------------------------------------------------------

procedure TClipper.DeleteFromAEL(e: PEdge);
var
  AelPrev, AelNext: PEdge;
begin
  AelPrev := e.prevInAEL;
  AelNext := e.nextInAEL;
  if not assigned(AelPrev) and not assigned(AelNext) and
    (e <> fActiveEdges) then exit; //already deleted
  if assigned(AelPrev) then
    AelPrev.nextInAEL := AelNext else
    fActiveEdges := AelNext;
  if assigned(AelNext) then AelNext.prevInAEL := AelPrev;
  e.nextInAEL := nil;
  e.prevInAEL := nil;
end;
//------------------------------------------------------------------------------

procedure TClipper.DeleteFromSEL(e: PEdge);
var
  SelPrev, SelNext: PEdge;
begin
  SelPrev := e.prevInSEL;
  SelNext := e.nextInSEL;
  if not assigned(SelPrev) and not assigned(SelNext) and
    (e <> fSortedEdges) then exit; //already deleted
  if assigned(SelPrev) then
    SelPrev.nextInSEL := SelNext else
    fSortedEdges := SelNext;
  if assigned(SelNext) then SelNext.prevInSEL := SelPrev;
  e.nextInSEL := nil;
  e.prevInSEL := nil;
end;
//------------------------------------------------------------------------------

procedure TClipper.UpdateEdgeIntoAEL(var e: PEdge);
var
  AelPrev, AelNext: PEdge;
begin
  if not assigned(e.nextInLML) then raise exception.Create(rsUpdateEdgeIntoAEL);
  AelPrev := e.prevInAEL;
  AelNext := e.nextInAEL;
  e.nextInLML.outIdx := e.outIdx;
  if assigned(AelPrev) then
    AelPrev.nextInAEL := e.nextInLML else
    fActiveEdges := e.nextInLML;
  if assigned(AelNext) then
    AelNext.prevInAEL := e.nextInLML;
  e.nextInLML.side := e.side;
  e.nextInLML.windDelta := e.windDelta;
  e.nextInLML.windCnt := e.windCnt;
  e.nextInLML.windCnt2 := e.windCnt2;
  e := e.nextInLML;
  e.prevInAEL := AelPrev;
  e.nextInAEL := AelNext;
  if not IsHorizontal(e) then InsertScanbeam(e.ytop);
end;
//------------------------------------------------------------------------------

procedure TClipper.SwapWithNextInSEL(edge: PEdge);
var
  prev, next, nextNext: PEdge;
begin
  prev := edge.prevInSEL;
  next := edge.nextInSEL;
  nextNext := next.nextInSEL;
  if assigned(prev) then prev.nextInSEL := next;
  if assigned(nextNext) then nextNext.prevInSEL := edge;
  edge.nextInSEL := nextNext;
  edge.prevInSEL := next;
  next.nextInSEL := edge;
  next.prevInSEL := prev;
  if edge = fSortedEdges then fSortedEdges := next;
end;
//------------------------------------------------------------------------------

function TClipper.BubbleSwap(edge: PEdge): PEdge;
var
  i, cnt: integer;
  e: PEdge;
begin
  cnt := 1;
  result := edge.nextInAEL;
  while assigned(result) and (abs(result.xbot - edge.xbot) <= tolerance) do
  begin
    inc(cnt);
    result := Result.nextInAEL;
  end;
  //let e = no edges in a complex intersect (ie >2 edges intersect at same pt).
  //if cnt = no intersection ops between those edges at that intersection
  //then ... when e =1, cnt =0; e =2, cnt =1; e =3, cnt =3; e =4, cnt =6; ...
  //series s (where s = intersections per no edges) ... s = 0,1,3,6,10,15 ...
  //generalising: given i = e-1, and s[0] = 0, then ... cnt = i + s[i-1]
  //example: no. intersect ops required by 4 edges in a complex intersection ...
  //         cnt = 3 + 2 + 1 + 0 = 6 intersection ops
  //nb: parallel edges within intersections will cause unexpected cnt values.
  if cnt > 2 then
  begin
    try
      //create the sort list ...
      fSortedEdges := edge;
      edge.prevInSEL := nil;
      e := edge.nextInAEL;
      for i := 2 to cnt do
      begin
        e.prevInSEL := e.prevInAEL;
        e.prevInSEL.nextInSEL := e;
        if i = cnt then e.nextInSEL := nil;
        e := e.nextInAEL;
      end;

      //fSortedEdges now contains the sort list. Bubble sort this list,
      //processing intersections and dropping the last edge on each pass
      //until the list contains fewer than two edges.
      while assigned(fSortedEdges) and assigned(fSortedEdges.nextInSEL) do
      begin
        e := fSortedEdges;
        while assigned(e.nextInSEL) do
        begin
          if (e.nextInSEL.dx > e.dx) then
          begin
            IntersectEdges(e, e.nextInSEL, DoublePoint(e.xbot,e.ybot));
            SwapPositionsInAEL(e, e.nextInSEL);
            SwapWithNextInSEL(e);
          end else
            e := e.nextInSEL;
        end;
        e.prevInSEL.nextInSEL := nil; //removes 'e' from SEL
      end;

    finally
      fSortedEdges := nil;
    end;
  end;
end;
//------------------------------------------------------------------------------

procedure TClipper.ProcessEdgesAtTopOfScanbeam(const topY: double);
var
  e, ePrior: PEdge;
begin
(*******************************************************************************
* Notes: Processing edges at scanline intersections (ie at the top or bottom   *
* of a scanbeam) needs to be done in multiple stages and in the correct order. *
* Firstly, edges forming a 'maxima' need to be processed and then removed.     *
* Next, 'intermediate' and 'maxima' horizontal edges are processed. Then edges *
* that intersect exactly at the top of the scanbeam are processed [%].         *
* Finally, new minima are added and any intersects they create are processed.  *
*******************************************************************************)

(*******************************************************************************
*     \                          /    /          \   /                         *
*      \   horizontal minima    /    /            \ /                          *
* { --  o======================#====o   --------   .     ------------------- } *
* {       horizontal maxima    .                   %  scanline intersect     } *
* { -- o=======================#===================#========o     ---------- } *
*      |                      /                   / \        \                 *
*      + maxima intersect    /                   /   \        \                *
*     /|\                   /                   /     \        \               *
*    / | \                 /                   /       \        \              *
*******************************************************************************)

  e := fActiveEdges;
  while assigned(e) do
  begin
    //1. process maxima, treating them as if they're 'bent' horizontal edges ...
    if IsMaxima(e, topY) and not IsHorizontal(GetMaximaPair(e)) then
    begin
      //'e' might be removed from AEL, as may any following edges so ...
      ePrior := e.prevInAEL;
      DoMaxima(e, topY);
      if not assigned(ePrior) then
        e := fActiveEdges else
        e := ePrior.nextInAEL;
    end else
    begin
      //2. promote horizontal edges, otherwise update xbot and ybot ...
      if IsIntermediate(e, topY) and IsHorizontal(e.nextInLML) then
      begin
        if (e.outIdx >= 0) then
          AddPolyPt(e.outIdx, DoublePoint(e.xtop, e.ytop), e.side = esLeft);
        UpdateEdgeIntoAEL(e);
        AddHorzEdgeToSEL(e);
      end else
      begin
        //this just simplifies horizontal processing ...
        e.xbot := TopX(e, topY);
        e.ybot := topY;
      end;
      e := e.nextInAEL;
    end;
  end;

  //3. Process horizontals at the top of the scanbeam ...
  ProcessHorizontals;

  //4. Promote intermediate vertices ...
  e := fActiveEdges;
  while assigned(e) do
  begin
    if IsIntermediate(e, topY) then
    begin
      if (e.outIdx >= 0) then
        AddPolyPt(e.outIdx, DoublePoint(e.xtop, e.ytop), e.side = esLeft);
      UpdateEdgeIntoAEL(e);
    end;
    e := e.nextInAEL;
  end;

  //5. Process (non-horizontal) intersections at the top of the scanbeam ...
  e := fActiveEdges;
  while assigned(e) do
  begin
    if not assigned(e.nextInAEL) then break;
    if e.nextInAEL.xbot < e.xbot - tolerance then
      raise Exception.Create(rsProcessEdgesAtTopOfScanbeam);
    if e.nextInAEL.xbot > e.xbot + tolerance then
      e := e.nextInAEL else
      e := BubbleSwap(e);
  end;
end;
//------------------------------------------------------------------------------

procedure TClipper.AddLocalMaxPoly(e1, e2: PEdge; const pt: TDoublePoint);
begin
  AddPolyPt(e1.outIdx, pt, e1.side = esLeft);
  if ShareSamePoly(e1, e2) then
  begin
    e1.outIdx := -1;
    e2.outIdx := -1;
  end else
    AppendPolygon(e1, e2);
end;
//------------------------------------------------------------------------------

procedure TClipper.AddLocalMinPoly(e1, e2: PEdge; const pt: TDoublePoint);
var
  e: PEdge;
  pp: PPolyPt;
  isAHole: boolean;
begin
  e1.outIdx := AddPolyPt(e1.outIdx, pt, true);
  e2.outIdx := e1.outIdx;

  if not IsHorizontal(e2) and (e1.dx > e2.dx) then
  begin
    e1.side := esLeft;
    e2.side := esRight;
  end else
  begin
    e1.side := esRight;
    e2.side := esLeft;
  end;

  if fForceAlternateOrientation then
  begin
    pp := PPolyPt(fPolyPtList[e1.outIdx]);
    isAHole := false;
    e := fActiveEdges;
    while assigned(e) do
    begin
      if (e.outIdx >= 0) and (TopX(e,pp.pt.Y) < pp.pt.X - fDupPtTolerance) then
        isAHole := not isAHole;
      e := e.nextInAEL;
    end;
    if isAHole then pp.isHole := sTrue else pp.isHole := sFalse;
  end;
end;
//------------------------------------------------------------------------------

procedure TClipper.AppendPolygon(e1, e2: PEdge);
var
  p1_lft, p1_rt, p2_lft, p2_rt: PPolyPt;
  side: TEdgeSide;
  e: PEdge;
  ObsoleteIdx: integer;
begin
  if (e1.outIdx < 0) or (e2.outIdx < 0) then
    raise Exception.Create(rsAppendPolygon);

  //get the start and ends of both output polygons ...
  p1_lft := PPolyPt(fPolyPtList[e1.outIdx]);
  p1_rt := p1_lft.prev;
  p2_lft := PPolyPt(fPolyPtList[e2.outIdx]);
  p2_rt := p2_lft.prev;

  //join e2 poly onto e1 poly and delete pointers to e2 ...
  if e1.side = esLeft then
  begin
    if e2.side = esLeft then
    begin
      //z y x a b c
      ReversePolyPtLinks(p2_lft);
      p2_lft.next := p1_lft;
      p1_lft.prev := p2_lft;
      p1_rt.next := p2_rt;
      p2_rt.prev := p1_rt;
      fPolyPtList[e1.outIdx] := p2_rt;
    end else
    begin
      //x y z a b c
      p2_rt.next := p1_lft;
      p1_lft.prev := p2_rt;
      p2_lft.prev := p1_rt;
      p1_rt.next := p2_lft;
      fPolyPtList[e1.outIdx] := p2_lft;
    end;
    side := esLeft;
  end else
  begin
    if e2.side = esRight then
    begin
      //a b c z y x
      ReversePolyPtLinks(p2_lft);
      p1_rt.next := p2_rt;
      p2_rt.prev := p1_rt;
      p2_lft.next := p1_lft;
      p1_lft.prev := p2_lft;
    end else
    begin
      //a b c x y z
      p1_rt.next := p2_lft;
      p2_lft.prev := p1_rt;
      p1_lft.prev := p2_rt;
      p2_rt.next := p1_lft;
    end;
    side := esRight;
  end;

  ObsoleteIdx := e2.outIdx;
  e2.outIdx := -1;
  e := fActiveEdges;
  while assigned(e) do
  begin
    if (e.outIdx = ObsoleteIdx) then
    begin
      e.outIdx := e1.outIdx;
      e.side := side;
      break;
    end;
    e := e.nextInAEL;
  end;
  e1.outIdx := -1;
  fPolyPtList[ObsoleteIdx] := nil;
end;
//------------------------------------------------------------------------------

end.
