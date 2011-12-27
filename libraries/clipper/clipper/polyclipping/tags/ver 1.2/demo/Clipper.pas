unit clipper;

(*******************************************************************************
*                                                                              *
* Author    :  Angus Johnson                                                   *
* Version   :  1.2i                                                            *
* Date      :  23 May 2010                                                     *
* Copyright :  Angus Johnson                                                   *
*                                                                              *
* This is an implementation of Bala Vatti's clipping algorithm outlined in:    *
* "A generic solution to polygon clipping"                                     *
* Communications of the ACM, Vol 35, Issue 7 (July 1992) pp 56-63.             *
* http://portal.acm.org/citation.cfm?id=129906                                 *
*                                                                              *
* See also:                                                                    *
* Computer graphics and geometric modeling: implementation and algorithms      *
* By Max K. Agoston                                                            *
* Springer; 1 edition (January 4, 2005)                                        *
* http://www.google.com/search?hl=en&q=vatti+clipping+site:books.google.com    *
*                                                                              *
*******************************************************************************)

(****** BEGIN LICENSE BLOCK ****************************************************
*                                                                              *
* Version: MPL 1.1 or LGPL 2.1 with linking exception                          *
*                                                                              *
* The contents of this file are subject to the Mozilla Public License Version  *
* 1.1 (the "License"); you may not use this file except in compliance with     *
* the License. You may obtain a copy of the License at                         *
* http://www.mozilla.org/MPL/                                                  *
*                                                                              *
* Software distributed under the License is distributed on an "AS IS" basis,   *
* WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License     *
* for the specific language governing rights and limitations under the         *
* License.                                                                     *
*                                                                              *
* Alternatively, the contents of this file may be used under the terms of the  *
* Free Pascal modified version of the GNU Lesser General Public License        *
* Version 2.1 (the "FPC modified LGPL License"), in which case the provisions  *
* of this license are applicable instead of those above.                       *
* Please see the file LICENSE.txt for additional information concerning this   *
* license.                                                                     *
*                                                                              *
* The Original Code is Clipper.pas                                             *
*                                                                              *
* The Initial Developer of the Original Code is                                *
* Angus Johnson (http://www.angusj.com)                                        *
*                                                                              *
******* END LICENSE BLOCK *****************************************************)

//Several type definitions used in this code are defined in the Delphi
//Graphics32 library ( see http://www.graphics32.org/wiki/ ). These type
//definitions are redefined here in case you don't wish to use Graphics32.
{$DEFINE USE_GRAPHICS32}

interface

uses
{$IFDEF USE_GRAPHICS32}
  SysUtils, GR32,
{$ENDIF}
  Classes, Math;

const
  infinite = -MaxSingle;
  tolerance = 0.0000001;
  same_point_tolerance = 0.001;

type
  TClipType = (ctIntersection, ctUnion, ctDifference, ctXor);
  TPolyType = (ptSubject, ptClip);
  TEdgeSide = (esLeft, esRight);
  TDirection = (dRightToLeft, dLeftToRight);
  TIntersectProtect = (ipLeft, ipRight);
  TIntersectProtects = set of TIntersectProtect;

{$IFNDEF USE_GRAPHICS32}
  TFloat = Single;
  TFloatPoint = record X, Y: TFloat; end;
  TArrayOfFloatPoint = array of TFloatPoint;
  TArrayOfArrayOfFloatPoint = array of TArrayOfFloatPoint;
{$ENDIF}
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
    polyIdx: integer;
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

  PPolyPt = ^TPolyPt;
  TPolyPt = record
    pt: TDoublePoint;
    next: PPolyPt;
    prev: PPolyPt;
    isHole: boolean; //(*)
  end;
  //Sometimes simple polygons need to be arranged in 'alternate' orientations -
  //ie where outer polygons are clockwise and inner (hole) polygons are
  //counter clockwise, or vice versa. (There is neither orientation nor 'holes'
  //with complex polygons.) q.v. TClipper ForceAlternateOrientation property.

  TPolyManager = class
  private
    fList             : TList;
    fLocalMinima      : PLocalMinima;
    fRecycledLocMin   : PLocalMinima;
    fRecycledLocMinEnd: PLocalMinima;
    procedure DisposeLocalMinimaList;
  public
    constructor Create; virtual;
    destructor Destroy; override;
    procedure AddPolygon(const polygon: TArrayOfFloatPoint; polyType: TPolyType);
    procedure AddPolyPolygon(const polyPolygon: TArrayOfArrayOfFloatPoint; polyType: TPolyType);
    procedure Clear;
    function Reset: boolean;
    procedure PopLocalMinima;
  property
    LocalMinima: PLocalMinima read fLocalMinima;
  end;

  TClipper = class(TPolyManager)
  private
    fPolyPtList: TList;
    fClipType: TClipType;
    fScanbeam: PScanbeam;
    fActiveEdges: PEdge;
    fSortedEdges: PEdge; //used for both intersection and horizontal edge sorts
    fIntersectNodes: PIntersectNode;
    fExecuteLocked: boolean;
    fForceAlternateOrientation: boolean;
    function ResultAsFloatPointArray: TArrayOfArrayOfFloatPoint;
    function InitializeScanbeam: boolean;
    procedure InsertScanbeam(y: double);
    function PopScanbeam: double;
    procedure DisposeScanbeamList;
    procedure InsertLocalMinimaIntoAEL(botY: double);
    procedure AddHorzEdgeToSEL(edge: PEdge);
    function IsTopHorz(horzEdge: PEdge; XPos: double): boolean;
    procedure ProcessHorizontal(horzEdge: PEdge);
    procedure ProcessHorizontals;
    procedure SwapPositionsInAEL(edge1, edge2: PEdge);
    procedure SwapWithNextInSEL(edge: PEdge);
    function BubbleSwap(edge: PEdge): PEdge;
    procedure AddIntersectNode(e1, e2: PEdge; const pt: TDoublePoint);
    procedure ProcessIntersections(topY: double);
    procedure BuildIntersectList(topY: double);
    procedure ProcessIntersectList(yTop: double);
    procedure IntersectEdges(e1,e2: PEdge;
      const pt: TDoublePoint; protects: TIntersectProtects = []);
    function GetMaximaPair(e: PEdge): PEdge;
    procedure DeleteFromAEL(e: PEdge);
    procedure DeleteFromSEL(e: PEdge);
    procedure DoMaxima(e: PEdge; topY: double);
    procedure UpdateEdgeIntoAEL(var e: PEdge);
    procedure ProcessEdgesAtTopOfScanbeam(topY: double);
    function IsContributing(edge: PEdge; out reverseSides: boolean): boolean;
    function AddPolyPt(idx: integer; const pt: TDoublePoint; ToFront: boolean): integer;
    procedure AddLocalMaxPoly(e1, e2: PEdge; const pt: TDoublePoint);
    procedure AddLocalMinPoly(e1, e2: PEdge; const pt: TDoublePoint);
    procedure AppendPolygon(e1, e2: PEdge);
  public
    function Execute(clipType: TClipType; out solution: TArrayOfArrayOfFloatPoint): boolean;
    constructor Create; override;
    destructor Destroy; override;
  property
    //ForceAlternateOrientation: Ensures that the result of the Execute method
    //will return polygons that have clockwise 'outer' and counter clockwise
    //inner (hole) polygons. (This assumes that these result polygons are
    //simple, not complex polygons). Default = false.
    ForceAlternateOrientation: boolean read
      fForceAlternateOrientation write fForceAlternateOrientation;
  end;

implementation

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

{$IFNDEF USE_GRAPHICS32}
function FloatPoint(X, Y: Single): TFloatPoint;
begin
  Result.X := X;
  Result.Y := Y;
end;
//------------------------------------------------------------------------------
{$ENDIF}

function DoublePoint(X, Y: double): TDoublePoint;
begin
  Result.X := X;
  Result.Y := Y;
end;
//------------------------------------------------------------------------------

function AlmostSamePoint(const pt1, pt2: TFloatPoint): boolean; overload;
begin
  result :=
    (abs(pt1.X - pt2.X) < same_point_tolerance) and
      (abs(pt1.Y - pt2.Y) < same_point_tolerance);
end;
//------------------------------------------------------------------------------

function AlmostSamePoint(const pt1, pt2: TDoublePoint): boolean; overload;
begin
  result :=
    (abs(pt1.X - pt2.X) < same_point_tolerance) and
      (abs(pt1.Y - pt2.Y) < same_point_tolerance);
end;
//------------------------------------------------------------------------------

function AlmostSamePoint(edge: PEdge): boolean; overload;
begin
  result := (abs(edge.xbot - edge.xtop) < same_point_tolerance) and
              (abs(edge.ybot - edge.ytop) < same_point_tolerance);
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

function Slope(pt1, pt2: TFloatPoint): double; overload;
begin
  if abs(pt1.y - pt2.y) < tolerance then
    result:= infinite else
    result := (pt1.x - pt2.x)/(pt1.y - pt2.y);
end;
//------------------------------------------------------------------------------

function Slope(pt1, pt2: TDoublePoint): double; overload;
begin
  if abs(pt1.y - pt2.y) < tolerance then
    result:= infinite else
    result := (pt1.x - pt2.x)/(pt1.y - pt2.y);
end;
//------------------------------------------------------------------------------

function IsHorizontal(e: PEdge): boolean;
begin
  result := assigned(e) and (e.dx < -3.39e+38);
end;
//------------------------------------------------------------------------------

procedure SwapSides(edge1, edge2: PEdge); {inline}
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
  polyIdx: integer;
begin
  polyIdx :=  edge1.polyIdx;
  edge1.polyIdx := edge2.polyIdx;
  edge2.polyIdx := polyIdx;
end;
//------------------------------------------------------------------------------

function TopX(edge: PEdge; currentY: double): double; {inline}
begin
  if currentY = edge.ytop then
    result := edge.xtop else
    result := edge.savedBot.X + edge.dx*(currentY - edge.savedBot.Y);
end;
//------------------------------------------------------------------------------

function EdgesShareSamePoly(e1, e2: PEdge): boolean;
begin
  result := assigned(e1) and assigned(e2) and (e1.polyIdx = e2.polyIdx);
end;
//------------------------------------------------------------------------------

function SameSlope(e1, e2: PEdge): boolean;
begin
  if IsHorizontal(e1) then
    result := IsHorizontal(e2) else
    result := not IsHorizontal(e2) and (abs(e1.dx - e2.dx)*1000 <= abs(e1.dx));
end;
//---------------------------------------------------------------------------

function IntersectPoint(edge1, edge2: PEdge; out ip: TDoublePoint): boolean;
var
  m1,m2,b1,b2: double;
begin
  m1 := 0; m2 := 0; b1 := 0; b2 := 0;
  with edge1^ do
    if dx <> 0 then
    begin
      m1 := 1/dx;
      b1 := savedBot.Y - savedBot.X/dx;
    end;
  with edge2^ do
    if dx <> 0 then
    begin
      m2 := 1/dx;
      b2 := savedBot.Y - savedBot.X/dx;
    end;
  if abs(m1 - m2) < tolerance then
  begin
    result := false;
    exit;
  end;
  if edge1.dx = 0 then
  begin
    ip.X := edge1.savedBot.X;
    ip.Y := m2*ip.X + b2;
  end
  else if edge2.dx = 0 then
  begin
    ip.X := edge2.savedBot.X;
    ip.Y := m1*ip.X + b1;
  end else
  begin
    ip.X := (b2 - b1)/(m1 - m2);
    ip.Y := (m1 * ip.X + b1);
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
  result := IsClockwise <> bottomPt.isHole;
end;

//------------------------------------------------------------------------------
// TPolyManager methods ...
//------------------------------------------------------------------------------

constructor TPolyManager.Create;
begin
  fList := TList.Create;
  fLocalMinima       := nil;
  fRecycledLocMin    := nil;
  fRecycledLocMinEnd := nil;
end;
//------------------------------------------------------------------------------

destructor TPolyManager.Destroy;
begin
  Clear;
  fList.Free;
  inherited;
end;
//------------------------------------------------------------------------------

procedure TPolyManager.AddPolygon(const polygon: TArrayOfFloatPoint; polyType: TPolyType);
var
  i, highI: integer;
  edges: PEdgeArray;
  e, e2: PEdge;

  //----------------------------------------------------------------------

  procedure InitEdge(e: PEdge; const pt1, pt2: TFloatPoint);
  begin
    fillChar(e^, sizeof(TEdge), 0);
    if (pt1.Y > pt2.Y) then
    begin
      e.xbot := pt1.X;
      e.ybot := pt1.Y;
      e.xtop := pt2.X;
      e.ytop := pt2.Y;
    end else
    begin
      e.xbot := pt2.X;
      e.ybot := pt2.Y;
      e.xtop := pt1.X;
      e.ytop := pt1.Y;
    end;
    e.savedBot.X := pt1.X; //temporary (just until duplicates removed)
    e.savedBot.Y := pt1.Y; //temporary (just until duplicates removed)
    e.polyType := polyType;
    e.polyIdx := -1;
    e.dx := Slope(pt1,pt2);
  end;
  //----------------------------------------------------------------------

  procedure ReInitEdge(e: PEdge; const pt1, pt2: TDoublePoint);
  begin
    //nb: preserves linkages
    if (pt1.Y > pt2.Y) then
    begin
      e.xbot := pt1.X;
      e.ybot := pt1.Y;
      e.xtop := pt2.X;
      e.ytop := pt2.Y;
    end else
    begin
      e.xbot := pt2.X;
      e.ybot := pt2.Y;
      e.xtop := pt1.X;
      e.ytop := pt1.Y;
    end;
    e.savedBot := pt1; //temporary (until duplicates etc removed)
    e.dx := Slope(pt1,pt2);
  end;
  //----------------------------------------------------------------------

  function FixupIfDupOrColinear(var e: PEdge): boolean;
  begin
    if (e.next = e.prev) then
    begin
      result := false;
    end
    else if AlmostSamePoint(e) or
      AlmostSamePoint(e.savedBot, e.prev.savedBot) or SameSlope(e.prev, e) then
    begin
      //fixup the previous edge because 'e' is about to come out of the loop ...
      ReInitEdge(e.prev, e.prev.savedBot, e.Next.savedBot);
      //now remove 'e' from the DLL loop by changing the links ...
      e.Next.prev := e.prev;
      e.prev.next := e.Next;
      if (e = @edges[0]) then
      begin
        e^ := e.Next^;
        e.prev.next := e;
        e.Next.prev := e;
      end else
        e := e.prev;
      result := true;
    end else
    begin
      result := false;
      e := e.Next;
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

    with newLm^ do
      if ((e.next <> e2) and (e.xbot < e2.xbot))
        or ((e.next = e2) and (e.dx > e2.dx)) then
      begin
        leftBound := e;
        rightBound := e.next;
        BuildBound(leftBound, esLeft, false);
        if IsHorizontal(rightBound) and (rightBound.xbot <> leftBound.xbot) then
          SwapX(rightBound);
        result := BuildBound(rightBound, esRight, true);
      end else
      begin
        leftBound := e2;
        rightBound := e2.prev;
        if IsHorizontal(rightBound) and (rightBound.xbot <> leftBound.xbot) then
          SwapX(rightBound);
        BuildBound(rightBound, esRight, false);
        result := BuildBound(leftBound, esLeft, true);
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
  while (highI > 1) and AlmostSamePoint(polygon[0],polygon[highI]) do dec(highI);
  if highI < 2 then exit;

  //make sure this is a sensible polygon (ie with at least one minima) ...
  i := 1;
  while (i <= highI) and (polygon[i].Y = polygon[0].Y) do inc(i);
  if i > highI then exit;

  GetMem(edges, sizeof(TEdge)*(highI+1));

  //fill the edge array ...
  InitEdge(@edges[highI], polygon[highI], polygon[0]);
  edges[highI].prev := @edges[highI-1];
  edges[highI].next := @edges[0];
  InitEdge(@edges[0], polygon[0], polygon[1]);
  edges[0].prev := @edges[highI];
  edges[0].next := @edges[1];
  for i := 1 to highI-1 do
  begin
    InitEdge(@edges[i], polygon[i], polygon[i+1]);
    edges[i].prev := @edges[i-1];
    edges[i].next := @edges[i+1];
  end;

  //fixup any co-linear edges or duplicate points ...
  e := @edges[0];
  repeat
    while FixupIfDupOrColinear(e) do ;
  until (e = @edges[0]) or (e.next = e.prev);
  if (e.next <> e.prev) then
    repeat
      e := @edges[0];
    until not FixupIfDupOrColinear(e);

  //make sure we still have a valid polygon ...
  if (e.next = e.prev) then
  begin
    dispose(edges);
    exit; //oops!!
  end;

  fList.Add(edges);

  //once duplicates etc have been removed, we can set edge savedBot to their
  //proper values and also get the starting minima (e2) ...
  e := @edges[0];
  e2 := e;
  repeat
    e.savedBot.X := e.xbot;
    e.savedBot.Y := e.ybot;
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

procedure TPolyManager.AddPolyPolygon(const polyPolygon: TArrayOfArrayOfFloatPoint;
  polyType: TPolyType);
var
  i: integer;
begin
  for i := 0 to high(polyPolygon) do AddPolygon(polyPolygon[i], polyType);
end;
//------------------------------------------------------------------------------

procedure TPolyManager.Clear;
var
  i: Integer;
begin
  DisposeLocalMinimaList;
  for i := 0 to fList.Count -1 do dispose(PEdgeArray(fList[i]));
  fList.Clear;
end;
//------------------------------------------------------------------------------

function TPolyManager.Reset: boolean;
var
  e: PEdge;
  lm: PLocalMinima;
begin
  //this allows clipping operations to be executed multiple times
  //on the same polygons.

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
      e.polyIdx := -1;
      e := e.nextInLML;
    end;
    e := lm.rightBound;
    while assigned(e) do
    begin
      e.xbot := e.savedBot.X;
      e.ybot := e.savedBot.Y;
      e.side := esRight;
      e.polyIdx := -1;
      e := e.nextInLML;
    end;
    lm := lm.nextLm;
  end;
end;
//------------------------------------------------------------------------------

procedure TPolyManager.PopLocalMinima;
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

procedure TPolyManager.DisposeLocalMinimaList;
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
  out solution: TArrayOfArrayOfFloatPoint): boolean;
var
  i: integer;
  yBot, yTop: double;
begin
  result := false;
  solution := nil;
  if fExecuteLocked or not InitializeScanbeam then exit;
  try try
    fExecuteLocked := true;
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

    //build the return polygons ...
    solution := ResultAsFloatPointArray;
    result := true;
  finally
    //clean up ...
    for i := 0 to fPolyPtList.Count -1 do
      if assigned(fPolyPtList[i]) then
        DisposePolyPts(PPolyPt(fPolyPtList[i]));
    fPolyPtList.Clear;
    fExecuteLocked := false;
  end;
  except
    result := false;
  end;
end;
//------------------------------------------------------------------------------

function TClipper.ResultAsFloatPointArray: TArrayOfArrayOfFloatPoint;
var
  i,j,k,cnt: integer;
  pt: PPolyPt;
begin
  k := 0;
  setLength(result, fPolyPtList.Count);
  for i := 0 to fPolyPtList.Count -1 do
    if assigned(fPolyPtList[i]) then
    begin
      cnt := 0;
      pt := PPolyPt(fPolyPtList[i]);
      repeat
        pt := pt.next;
        inc(cnt);
      until (pt = PPolyPt(fPolyPtList[i]));
      if cnt < 2 then continue;

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
  lm := LocalMinima;
  while assigned(lm) do
  begin
    InsertScanbeam(lm.y);
    lm := lm.nextLm;
  end;
end;
//------------------------------------------------------------------------------

procedure TClipper.InsertScanbeam(y: double);
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

procedure TClipper.InsertLocalMinimaIntoAEL(botY: double);

  //----------------------------------------------------------------------
  function Edge2InsertsBeforeEdge1(e1,e2: PEdge): boolean;
  begin
    if (e2.xbot - tolerance > e1.xbot) then result := false
    else if (e2.xbot + tolerance < e1.xbot) then result := true
    else if IsHorizontal(e2) then result := false
    else if SameSlope(e1, e2) then result := false
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
  reverseSides: boolean;
  e: PEdge;
  pt: TDoublePoint;
begin
  {InsertLocalMinimaIntoAEL}
  while assigned(LocalMinima) and (LocalMinima.y = botY) do
  begin
    InsertEdgeIntoAEL(LocalMinima.leftBound);
    InsertScanbeam(LocalMinima.leftBound.ytop);
    InsertEdgeIntoAEL(LocalMinima.rightBound);

    e := LocalMinima.leftBound.nextInAEL;
    if IsHorizontal(LocalMinima.rightBound) then
    begin
      //nb: only rightbounds can have a horizontal bottom edge
      AddHorzEdgeToSEL(LocalMinima.rightBound);
      InsertScanbeam(LocalMinima.rightBound.nextInLML.ytop);
    end else
      InsertScanbeam(LocalMinima.rightBound.ytop);

    with LocalMinima^ do
    begin
      if IsContributing(leftBound, reverseSides) then
      begin
        AddLocalMinPoly(leftBound, rightBound, DoublePoint(leftBound.xbot, y));
        if reverseSides and (leftBound.nextInAEL = rightBound) then
        begin
          leftBound.side := esRight;
          rightBound.side := esLeft;
        end;
      end;
      if (leftBound.nextInAEL <> rightBound) then
      begin
        e := leftBound.nextInAEL;
        pt := DoublePoint(leftBound.xbot,leftBound.ybot);
        while e <> rightBound do
        begin
          if not assigned(e) then raise exception.Create('missing rightbound!');
          IntersectEdges(rightBound, e, pt);
          e := e.nextInAEL;
          if not assigned(e) then
            raise exception.Create('AddLocalMinima: intersect with unexpected maxima');
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

function IsMaxima(e: PEdge; Y: double): boolean;
begin
  result := assigned(e) and (e.ytop = Y) and not assigned(e.nextInLML);
end;
//------------------------------------------------------------------------------

function IsIntermediate(e: PEdge; Y: double): boolean;
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

procedure TClipper.DoMaxima(e: PEdge; topY: double);
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
  if (e.polyIdx < 0) and (eMaxPair.polyIdx < 0) then
  begin
    DeleteFromAEL(e);
    DeleteFromAEL(eMaxPair);
  end
  else if (e.polyIdx >= 0) and (eMaxPair.polyIdx >= 0) then
  begin
    IntersectEdges(e, eMaxPair, DoublePoint(X, topY));
  end
  else raise exception.Create('DoMaxima error');
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

function TClipper.IsTopHorz(horzEdge: PEdge; XPos: double): boolean;
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
* are processed doesn't seem to matter. HEs intersect with other HE xbots only *
* [o], and with other non-horizontal edges [*]. Once these intersections are   *
* processed, intermediate HEs then 'promote' the edge above (nextInLML) into   *
* the AEL. These 'promoted' edges may in turn intersect with other HEs.        *
*******************************************************************************)

(*******************************************************************************
*           \                         \ /                      /          /    *
*            \                         •                      /          /     *
*             \                       / \         ·——————————*——————————·  (3) *
*              ·——————————·  (2)     /   \        ·         /                  *
* horizontal intersects   ·         /     \       ·        /                   *
*         ·———————————————o————————*———————*——————o———————·  (1)               *
*        /                 \      /         \    /                             *
*******************************************************************************)

  with horzEdge^ do
  begin
    if xbot < xtop then
    begin
      horzLeft := xbot; horzRight := xtop;
      Direction := dLeftToRight;
    end else
    begin
      horzLeft := xtop; horzRight := xbot;
      Direction := dRightToLeft;
    end;

    if assigned(nextInLML) then
      eMaxPair := nil else
      eMaxPair := GetMaximaPair(horzEdge);

    e := GetNextInAEL(horzEdge, Direction);
    while assigned(e) do
    begin
      eNext := GetNextInAEL(e, Direction);
      if (e.xbot >= horzLeft - tolerance) and (e.xbot <= horzRight + tolerance) then
      begin
        //ok, we seem to be in range of the horizontal edge ...
        if (e.xbot = xtop) and assigned(nextInLML) and (e.dx < nextInLML.dx) then
        begin
          //we've gone past the end of an intermediate horz edge so quit.
          //nb: More -ve slopes follow more +ve slopes *above* the horizontal.
          break;
        end
        else if (e = eMaxPair) then
        begin
          //horzEdge is evidently a maxima horizontal and we've arrived at its end.
          IntersectEdges(e, horzEdge, DoublePoint(e.xbot, ybot),[ipRight]);
          break;
        end
        else if IsHorizontal(e) and not IsMinima(e) and not (e.xbot > e.xtop) then
        begin
          //An overlapping horizontal edge. Overlapping horizontal edges are
          //processed as if layered with the current horizontal edge (horizEdge)
          //being infinitesimally lower that the next (e). Therfore, we
          //intersect with e only if e.xbot is within the bounds of horzEdge ...
          if Direction = dLeftToRight then
            IntersectEdges(horzEdge, e, DoublePoint(e.xbot, ybot),
              ProtectRight[not IsTopHorz(horzEdge, e.xbot)])
          else
            IntersectEdges(e, horzEdge, DoublePoint(e.xbot, ybot),
              ProtectLeft[not IsTopHorz(horzEdge, e.xbot)]);
        end
        else if (Direction = dLeftToRight) then
        begin
          IntersectEdges(horzEdge, e, DoublePoint(e.xbot, ybot),
            ProtectRight[not IsTopHorz(horzEdge, e.xbot)])
        end else
        begin
          IntersectEdges(e, horzEdge, DoublePoint(e.xbot, ybot),
            ProtectLeft[not IsTopHorz(horzEdge, e.xbot)]);
        end;
        SwapPositionsInAEL(horzEdge, e);
      end
      else if (Direction = dLeftToRight) and
        (e.xbot > horzRight + tolerance) and not assigned(nextInSEL) then
          break
      else if (Direction = dRightToLeft) and
        (e.xbot < horzLeft - tolerance) and not assigned(nextInSEL) then
          break;
      e := eNext;
    end;

    if assigned(nextInLML) then
    begin
      if (polyIdx >= 0) then AddPolyPt(polyIdx, DoublePoint(xtop, ytop), side = esLeft);
      UpdateEdgeIntoAEL(horzEdge);
    end else
      DeleteFromAEL(horzEdge);
  end;
end;
//------------------------------------------------------------------------------

function TClipper.AddPolyPt(idx: integer; const pt: TDoublePoint; ToFront: boolean): integer;
var
  fp, newPolyPt: PPolyPt;
  e: PEdge;
begin
  new(newPolyPt);
  newPolyPt.pt := pt;
  if idx < 0 then
  begin
    result := fPolyPtList.Add(newPolyPt);
    newPolyPt.next := newPolyPt;
    newPolyPt.prev := newPolyPt;

    newPolyPt.isHole := false;
    if fForceAlternateOrientation then
    begin
      e := fActiveEdges;
      while assigned(e) and (e.xbot < pt.X) do
      begin
        if (e.polyIdx >= 0) then newPolyPt.isHole := not newPolyPt.isHole;
        e := e.nextInAEL;
      end;
    end;
    
  end else
  begin
    result := idx;
    fp := PPolyPt(fPolyPtList[idx]);

    if (ToFront and AlmostSamePoint(pt, fp.pt)) or
      (not ToFront and AlmostSamePoint(pt, fp.prev.pt)) then
    begin
      dispose(newPolyPt);
      exit;
    end;

    newPolyPt.isHole := fp.isHole;
    newPolyPt.next := fp;
    newPolyPt.prev := fp.prev;
    newPolyPt.prev.next := newPolyPt;
    fp.prev := newPolyPt;
    if ToFront then fPolyPtList[idx] := newPolyPt;
  end;
end;
//------------------------------------------------------------------------------

procedure TClipper.ProcessIntersections(topY: double);
var
  iNode: PIntersectNode;
begin
  if not assigned(fActiveEdges) then exit;
  try try
    BuildIntersectList(topY);
    ProcessIntersectList(topY);
  finally
    //if there's been a problem, clean up the mess ...
    while assigned(fIntersectNodes) do
    begin
      iNode := fIntersectNodes.next;
      dispose(fIntersectNodes);
      fIntersectNodes := iNode;
    end;
  end;
  except
    raise;
  end;
end;
//------------------------------------------------------------------------------

function Node1IsBelow2(Node1, Node2: PIntersectNode): boolean;

  function Edge2BeforeEdge1InAEL(e1,e2: PEdge): boolean;
  begin
    result := true;
    while assigned(e2) do
      if e2 = e1 then exit
      else e2 := e2.nextInAEL;
    result := false;
  end;

begin
  if (abs(Node1.pt.Y - Node2.pt.Y) < tolerance) then
  begin
    if (Node1.edge1.dx = Node2.edge1.dx) then
    begin
      if SameSlope(Node1.edge2, Node2.edge2) then
      begin
        if Node1.edge2 = Node2.edge2 then
        begin
          //e2=e2 ...
          if Node1.edge2.dx > Node1.edge1.dx then
            result := Edge2BeforeEdge1InAEL(Node1.edge1, Node2.edge1) else
            result := Edge2BeforeEdge1InAEL(Node2.edge1, Node1.edge1);
        end else
        begin
          //e1=e1 ...
          if Node1.edge1.dx > Node1.edge2.dx then
            result := Edge2BeforeEdge1InAEL(Node1.edge2, Node2.edge2) else
            result := Edge2BeforeEdge1InAEL(Node2.edge2, Node1.edge2);
        end;
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
  else if Node1IsBelow2(IntersectNode, fIntersectNodes) then
  begin
    IntersectNode.next := fIntersectNodes;
    fIntersectNodes.prev := IntersectNode;
    fIntersectNodes := IntersectNode;
  end else
  begin
    iNode := fIntersectNodes;
    while assigned(iNode.next) and Node1IsBelow2(iNode.next, IntersectNode) do
      iNode := iNode.next;
    if assigned(iNode.next) then iNode.next.prev := IntersectNode;
    IntersectNode.next := iNode.next;
    IntersectNode.prev := iNode;
    iNode.next := IntersectNode;
  end;
end;
//------------------------------------------------------------------------------

procedure TClipper.BuildIntersectList(topY: double);
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

procedure TClipper.ProcessIntersectList(yTop: double);
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

  //----------------------------------------------------------------------
  procedure AdjustLocalMinSides(edge1, edge2: PEdge);
  begin
    if not IsHorizontal(edge2) and (edge1.dx > edge2.dx) then
    begin
      edge1.side := esLeft;
      edge2.side := esRight;
    end else
    begin
      edge1.side := esRight;
      edge2.side := esLeft;
    end;
  end;
  //----------------------------------------------------------------------

  procedure DoEdge1(edge1, edge2: PEdge);
  begin
    AddPolyPt(e1.polyIdx, pt, e1.side = esLeft);
    SwapSides(e1, e2);
    SwapPolyIndexes(e1, e2);
  end;
  //----------------------------------------------------------------------

  procedure DoEdge2(edge1, edge2: PEdge);
  begin
    AddPolyPt(e2.polyIdx, pt, e2.side = esLeft);
    SwapSides(e1, e2);
    SwapPolyIndexes(e1, e2);
  end;
  //----------------------------------------------------------------------

  procedure DoBothEdges(edge1, edge2: PEdge);
  begin
    AddPolyPt(e1.polyIdx, pt, e1.side = esLeft);
    AddPolyPt(e2.polyIdx, pt, e2.side = esLeft);
    SwapSides(e1, e2);
    SwapPolyIndexes(e1, e2);
  end;
  //----------------------------------------------------------------------

var
  e1stops, e2stops: boolean;
  e1Contributing, e2contributing: boolean;
begin
  {IntersectEdges}
  e1stops := not (ipLeft in protects) and not assigned(e1.nextInLML) and
    (abs(e1.xtop - pt.x) < tolerance) and (abs(e1.ytop - pt.y) < tolerance);
  e2stops := not (ipRight in protects) and not assigned(e2.nextInLML) and
    (abs(e2.xtop - pt.x) < tolerance) and (abs(e2.ytop - pt.y) < tolerance);
  e1Contributing := (e1.polyidx >= 0);
  e2contributing := (e2.polyidx >= 0);

  case Self.fClipType of
    ctIntersection, ctUnion, ctDifference:
      begin
        if e1Contributing and e2contributing then
        begin
          if e1stops or e2stops or (e1.polytype <> e2.polytype) then
              AddLocalMaxPoly(e1, e2, pt) else
              DoBothEdges(e1,e2);
        end
          else if e1Contributing then
          DoEdge1(e1,e2)
        else if e2contributing then
          DoEdge2(e1,e2)
        else
        begin
          //neither edge contributing
          if (e1.polytype <> e2.polytype) and not e1stops and not e2stops then
          begin
            AddLocalMinPoly(e1, e2, pt);
            AdjustLocalMinSides(e1, e2);
          end
            else swapsides(e1,e2);
        end;
      end;
    ctXor:
      begin
        //edges can be momentarily non-contributing at complex intersections ...
        if e1Contributing and e2contributing then
        begin
          if e1stops or e2stops then
            AddLocalMaxPoly(e1, e2, pt) else
            DoBothEdges(e1,e2);
        end else if e1Contributing then
          AddPolyPt(e1.polyIdx, pt, e1.side = esLeft)
        else if e2Contributing then
          AddPolyPt(e2.polyIdx, pt, e2.side = esLeft);
      end;
  end;

  if (e1stops <> e2stops) and
    ((e1stops and (e1.polyIdx >= 0)) or (e2stops and (e2.polyIdx >= 0))) then
  begin
    swapsides(e1,e2);
    SwapPolyIndexes(e1, e2);
  end;

  //finally, delete any non-contrib. maxima edges  ...
  if e1stops then deleteFromAEL(e1);
  if e2stops then deleteFromAEL(e2);
end;
//------------------------------------------------------------------------------

function TClipper.IsContributing(edge: PEdge; out reverseSides: boolean): boolean;
var
  polyType: TPolyType;
begin
  result := true;
  polyType := edge.polyType;
  reverseSides := false;
  case fClipType of
    ctIntersection:
      begin
        result := false;
        while assigned(edge.prevInAEL) do
        begin
          if edge.prevInAEL.polyType = polyType then
            reverseSides := not reverseSides else
            result := not result;
          edge := edge.prevInAEL;
        end;
      end;
    ctUnion:
      begin
        result := true;
        while assigned(edge.prevInAEL) do
        begin
          if edge.prevInAEL.polyType <> polyType then
            result := not result;
          edge := edge.prevInAEL;
        end;
      end;
    ctDifference:
      begin
        result := polyType = ptSubject;
        while assigned(edge.prevInAEL) do
        begin
          if edge.prevInAEL.polyType <> polyType then
            result := not result;
          edge := edge.prevInAEL;
        end;
      end;
    ctXor:
      result := true;
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
  if not assigned(e.nextInLML) then
    raise exception.Create('UpdateEdgeIntoAEL: invalid call');
  AelPrev := e.prevInAEL;
  AelNext := e.nextInAEL;
  e.nextInLML.polyIdx := e.polyIdx;
  if assigned(AelPrev) then
    AelPrev.nextInAEL := e.nextInLML else
    fActiveEdges := e.nextInLML;
  if assigned(AelNext) then
    AelNext.prevInAEL := e.nextInLML;
  e.nextInLML.side := e.side;
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
  //nb: cnt will always be greater than 1 here
  if cnt = 2 then
  begin
    if SameSlope(edge, edge.nextInAEL) then exit;
    IntersectEdges(edge, edge.nextInAEL, DoublePoint(edge.xbot,edge.ybot));
    SwapPositionsInAEL(edge, edge.nextInAEL);
  end else
  begin
    //create the sort list ...
    try
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

procedure TClipper.ProcessEdgesAtTopOfScanbeam(topY: double);
var
  e, ePrior: PEdge;
begin
(*******************************************************************************
* Notes: Processing edges at scanline intersections (ie at the top or bottom   *
* of a scanbeam) needs to be done in multiple stages and in the correct order. *
* Firstly, edges forming a 'maxima' need to be processed and then removed.     *
* Next, 'intermediate' and 'maxima' horizontal edges are processed.            *
* Then edges that intersect exactly at the top of the scanbeam are processed.  *
* Finally, new minima are added and any intersects they create are processed.  *
*******************************************************************************)

(*******************************************************************************
*  \                               /    /        \     /                       *
*   \     horiz. minima intersect /    /          \   /                        *
*    ·———————————————————————————#————·            \ /                         *
*      horiz. maxima intersects /                   * scanline intersect       *
*      ·———————————————————————o———————————————————o—o————————·                *
*      |                      /                   /   \        \               *
*      • maxima intersect    /                   /     \        \              *
*     /|\                   /                   /       \        \             *
*    / | \                 /                   /         \        \            *
*******************************************************************************)

  e := fActiveEdges;
  while assigned(e) do
  begin
    //1. process all maxima ...
    //   logic behind code - maxima are treated as if 'bent' horizontal edges
    if IsMaxima(e, topY) and not IsHorizontal(GetMaximaPair(e)) then
    begin
      //'e' might be removed from AEL, as may any following so ...
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
        if (e.polyIdx >= 0) then
          AddPolyPt(e.polyIdx, DoublePoint(e.xtop, e.ytop), e.side = esLeft);
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
      if (e.polyIdx >= 0) then
        AddPolyPt(e.polyIdx, DoublePoint(e.xtop, e.ytop), e.side = esLeft);
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
      raise Exception.Create('ProcessEdgesAtTopOfScanbeam: Broken AEL order');
    if e.nextInAEL.xbot > e.xbot + tolerance then
      e := e.nextInAEL else
      e := BubbleSwap(e);
  end;
end;
//------------------------------------------------------------------------------

procedure TClipper.AddLocalMaxPoly(e1, e2: PEdge; const pt: TDoublePoint);
begin
  AddPolyPt(e1.polyIdx, pt, e1.side = esLeft);
  if EdgesShareSamePoly(e1,e2) then
  begin
    e1.polyIdx := -1;
    e2.polyIdx := -1;
  end else
    AppendPolygon(e1, e2);
end;
//------------------------------------------------------------------------------

procedure TClipper.AddLocalMinPoly(e1, e2: PEdge; const pt: TDoublePoint);
begin
  e1.polyIdx := AddPolyPt(e1.polyIdx, pt, true);
  e2.polyIdx := e1.polyIdx;
end;
//------------------------------------------------------------------------------

procedure TClipper.AppendPolygon(e1, e2: PEdge);
var
  p1_lft, p1_rt, p2_lft, p2_rt: PPolyPt;
  side: TEdgeSide;
  e: PEdge;
  ObsoleteIdx: integer;
begin
  if (e1.polyIdx < 0) or (e2.polyIdx < 0) then
    raise Exception.Create('AppendPolygon error');

  //get the start and ends of both output polygons ...
  p1_lft := PPolyPt(fPolyPtList[e1.polyIdx]);
  p1_rt := p1_lft.prev;
  p2_lft := PPolyPt(fPolyPtList[e2.polyIdx]);
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
      fPolyPtList[e1.polyIdx] := p2_rt;
    end else
    begin
      //x y z a b c
      p2_rt.next := p1_lft;
      p1_lft.prev := p2_rt;
      p2_lft.prev := p1_rt;
      p1_rt.next := p2_lft;
      fPolyPtList[e1.polyIdx] := p2_lft;
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

  ObsoleteIdx := e2.polyIdx;
  e2.polyIdx := -1;
  e := fActiveEdges;
  while assigned(e) do
  begin
    if (e.polyIdx = ObsoleteIdx) then
    begin
      e.polyIdx := e1.polyIdx;
      e.side := side;
      break;
    end;
    e := e.nextInAEL;
  end;
  e1.polyIdx := -1;
  fPolyPtList[ObsoleteIdx] := nil;
end;
//------------------------------------------------------------------------------

end.
