unit clipper;

(*******************************************************************************
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
  SysUtils, Types, Classes, Math;

type

  TClipType = (ctIntersection, ctUnion, ctDifference, ctXor);
  TPolyType = (ptSubject, ptClip);
  TPolyFillType = (pftEvenOdd, pftNonZero);

{$IFNDEF USING_GRAPHICS32}
  TFloat = Single;
  TFloatPoint = record X, Y: TFloat; end;
  TArrayOfFloatPoint = array of TFloatPoint;
  TArrayOfArrayOfFloatPoint = array of TArrayOfFloatPoint;
  TFloatRect = record left, top, right, bottom: TFloat; end;
{$ENDIF}

  //used internally ...
  TEdgeSide = (esLeft, esRight);
  TIntersectProtect = (ipLeft, ipRight);
  TIntersectProtects = set of TIntersectProtect;
  TDirection = (dRightToLeft, dLeftToRight);
  TArrayOfPoint = array of TPoint;
  TArrayOfArrayOfPoint = array of TArrayOfPoint;

  PEdge = ^TEdge;
  TEdge = record
    xbot : integer;  //bottom
    ybot : integer;
    xcurr: integer;  //current (ie relative to bottom of current scanbeam)
    ycurr: integer;
    xtop : integer;  //top
    ytop : integer;
    tmpX :  integer;
    dx   : double;   //the inverse of slope
    polyType : TPolyType;
    side     : TEdgeSide;
    windDelta: integer; //1 or -1 depending on winding direction
    windCnt  : integer;
    windCnt2 : integer;  //winding count of the opposite polytype
    outIdx   : integer;
    next     : PEdge;
    prev     : PEdge;
    nextInLML: PEdge;
    prevInAEL: PEdge;
    nextInAEL: PEdge;
    prevInSEL: PEdge;
    nextInSEL: PEdge;
  end;

  PEdgeArray = ^TEdgeArray;
  TEdgeArray = array[0.. MaxInt div sizeof(TEdge) -1] of TEdge;

  PScanbeam = ^TScanbeam;
  TScanbeam = record
    y   : integer;
    next: PScanbeam;
  end;

  PIntersectNode = ^TIntersectNode;
  TIntersectNode = record
    edge1: PEdge;
    edge2: PEdge;
    pt   : TPoint;
    next : PIntersectNode;
  end;

  PLocalMinima = ^TLocalMinima;
  TLocalMinima = record
    y         : integer;
    leftBound : PEdge;
    rightBound: PEdge;
    next      : PLocalMinima;
  end;

  PPolyPt = ^TPolyPt;
  TPolyPt = record
    pt     : TPoint;
    next   : PPolyPt;
    prev   : PPolyPt;
    isHole : boolean;
  end;

  PJoinRec = ^TJoinRec;
  TJoinRec = record
    pt1a     : TPoint;
    pt1b     : TPoint;
    poly1Idx : integer;
    pt2a     : TPoint;
    pt2b     : TPoint;
    poly2Idx : integer;
    next     : PJoinRec;
    prev     : PJoinRec;
  end;

  PHorzRec = ^THorzRec;
  THorzRec = record
    edge     : PEdge;
    savedIdx : integer;
    next     : PHorzRec;
    prev     : PHorzRec;
  end;

type

  TClipperBase = class
  private
    fEdgeList      : TList;
    fLmList        : PLocalMinima; //localMinima list
    fCurrLm        : PLocalMinima; //current localMinima node
    procedure DisposeLocalMinimaList;
  protected
    function Reset: boolean; virtual;
    procedure PopLocalMinima;
    property CurrentLm: PLocalMinima read fCurrLm;
  public
    constructor Create; virtual;
    destructor Destroy; override;
    procedure AddPolygon(const polygon: TArrayOfPoint; polyType: TPolyType);
    procedure AddPolygons(const polygons: TArrayOfArrayOfPoint; polyType: TPolyType);
    procedure Clear; virtual;
  end;

  TClipper = class(TClipperBase)
  private
    fPolyPtList    : TList;
    fClipType      : TClipType;
    fScanbeam      : PScanbeam; //scanbeam list
    fActiveEdges   : PEdge;     //active edge list
    fSortedEdges   : PEdge;     //used for temporary sorting
    fIntersectNodes: PIntersectNode;
    fClipFillType  : TPolyFillType;
    fSubjFillType  : TPolyFillType;
    fExecuteLocked : boolean;
    fJoins         : PJoinRec;
    fHorizJoin     : PHorzRec;
    procedure DisposeScanbeamList;
    procedure InsertScanbeam(const y: integer);
    function PopScanbeam: integer;
    procedure SetWindingCount(edge: PEdge);
    function IsNonZeroFillType(edge: PEdge): boolean;
    function IsNonZeroAltFillType(edge: PEdge): boolean;
    procedure AddEdgeToSEL(edge: PEdge);
    procedure CopyAELToSEL;
    procedure InsertLocalMinimaIntoAEL(const botY: integer);
    procedure SwapPositionsInAEL(e1, e2: PEdge);
    procedure SwapPositionsInSEL(e1, e2: PEdge);
    function IsTopHorz(const XPos: integer): boolean;
    procedure ProcessHorizontal(horzEdge: PEdge);
    procedure ProcessHorizontals;
    procedure AddIntersectNode(e1, e2: PEdge; const pt: TPoint);
    function ProcessIntersections(const topY: integer): boolean;
    procedure BuildIntersectList(const topY: integer);
    procedure ProcessIntersectList;
    procedure DeleteFromAEL(e: PEdge);
    procedure DeleteFromSEL(e: PEdge);
    procedure IntersectEdges(e1,e2: PEdge;
      const pt: TPoint; protects: TIntersectProtects = []);
    procedure DoMaxima(e: PEdge; const topY: integer);
    procedure UpdateEdgeIntoAEL(var e: PEdge);
    function FixupIntersections: boolean;
    procedure SwapIntersectNodes(int1, int2: PIntersectNode);
    procedure ProcessEdgesAtTopOfScanbeam(const topY: integer);
    function IsContributing(edge: PEdge): boolean;
    function AddPolyPt(e: PEdge; const pt: TPoint): PPolyPt;
    procedure AddLocalMaxPoly(e1, e2: PEdge; const pt: TPoint);
    procedure AddLocalMinPoly(e1, e2: PEdge; const pt: TPoint);
    procedure AppendPolygon(e1, e2: PEdge);
    procedure DisposePolyPts(pp: PPolyPt);
    procedure DisposeAllPolyPts;
    procedure DisposeIntersectNodes;
    function GetResult: TArrayOfArrayOfPoint;
    function FixupOutPolygon(outPoly: PPolyPt): PPolyPt;
    function IsHole(e: PEdge): boolean;
    procedure AddJoin(e1, e2: PEdge; e1OutIdx: integer = -1);
    procedure ClearJoins;
    procedure AddHorzJoin(e: PEdge; idx: integer);
    procedure ClearHorzJoins;
    procedure JoinCommonEdges;
  protected
    function Reset: boolean; override;
  public
    function Execute(clipType: TClipType;
      out solution: TArrayOfArrayOfPoint;
      subjFillType: TPolyFillType = pftEvenOdd;
      clipFillType: TPolyFillType = pftEvenOdd): boolean;
    constructor Create; override;
    destructor Destroy; override;
  end;

function IsClockwise(const pts: TArrayOfPoint): boolean;
function Area(const pts: TArrayOfPoint): double;
function OffsetPolygons(const pts: TArrayOfArrayOfPoint;
  const delta: single): TArrayOfArrayOfPoint;
function PointInPolygon(const pt: TPoint; const pts: TArrayOfPoint): Boolean;

function FloatPointsToPoint(const a: TArrayOfFloatPoint;
  decimals: integer = 2): TArrayOfPoint; overload;
function FloatPointsToPoint(const a: TArrayOfArrayOfFloatPoint;
  decimals: integer = 2): TArrayOfArrayOfPoint; overload;
function PointsToFloatPoints(const a: TArrayOfPoint;
  decimals: integer = 2): TArrayOfFloatPoint; overload;
function PointsToFloatPoints(const a: TArrayOfArrayOfPoint;
  decimals: integer = 2): TArrayOfArrayOfFloatPoint; overload;

implementation

type
  PDoublePoint = ^TDoublePoint;
  TDoublePoint = record X, Y: double; end;
  TArrayOfDoublePoint = array of TDoublePoint;

  TPosition = (pFirst, pMiddle, pSecond);

const
  horizontal: double = -3.4e+38;

resourcestring
  rsMissingRightbound = 'InsertLocalMinimaIntoAEL: missing rightbound';
  rsDoMaxima = 'DoMaxima error';
  rsUpdateEdgeIntoAEL = 'UpdateEdgeIntoAEL error';
  rsHorizontal = 'ProcessHorizontal error';

//------------------------------------------------------------------------------
// Miscellaneous Functions ...
//------------------------------------------------------------------------------

{$IFDEF USING_GRAPHICS32}
function FloatPointsToPoint(const a: TArrayOfFloatPoint;
  decimals: integer = 2): TArrayOfPoint; overload;
var
  i,decScale: integer;
begin
  decScale := round(power(10,decimals));
  setlength(result, length(a));
  for i := 0 to high(a) do
  begin
    result[i].X := round(a[i].X *decScale);
    result[i].Y := round(a[i].Y *decScale);
  end;
end;
//------------------------------------------------------------------------------

function FloatPointsToPoint(const a: TArrayOfArrayOfFloatPoint;
  decimals: integer = 2): TArrayOfArrayOfPoint; overload;
var
  i,j,decScale: integer;
begin
  decScale := round(power(10,decimals));
  setlength(result, length(a));
  for i := 0 to high(a) do
  begin
    setlength(result[i], length(a[i]));
    for j := 0 to high(a[i]) do
    begin
      result[i][j].X := round(a[i][j].X *decScale);
      result[i][j].Y := round(a[i][j].Y *decScale);
    end;
  end;
end;
//------------------------------------------------------------------------------

function PointsToFloatPoints(const a: TArrayOfPoint;
  decimals: integer = 2): TArrayOfFloatPoint; overload;
var
  i,decScale: integer;
begin
  decScale := round(power(10,decimals));
  setlength(result, length(a));
  for i := 0 to high(a) do
  begin
    result[i].X := a[i].X /decScale;
    result[i].Y := a[i].Y /decScale;
  end;
end;
//------------------------------------------------------------------------------

function PointsToFloatPoints(const a: TArrayOfArrayOfPoint;
  decimals: integer = 2): TArrayOfArrayOfFloatPoint; overload;
var
  i,j,decScale: integer;
begin
  decScale := round(power(10,decimals));
  setlength(result, length(a));
  for i := 0 to high(a) do
  begin
    setlength(result[i], length(a[i]));
    for j := 0 to high(a[i]) do
    begin
      result[i][j].X := a[i][j].X /decScale;
      result[i][j].Y := a[i][j].Y /decScale;
    end;
  end;
end;
//------------------------------------------------------------------------------
{$ENDIF}

function GetAngle(const pt1, pt2: TPoint): double;
begin
  if (pt1.Y = pt2.Y) then
  begin
    if pt1.X > pt2.X then result := pi/2
    else result := -pi/2;
  end
  else
    result := ArcTan2(pt1.X-pt2.X, pt1.Y-pt2.Y);
end;
//---------------------------------------------------------------------------

function IsClockwise(const pts: TArrayOfPoint): boolean; overload;
var
  i, highI: integer;
  area: double;
begin
  result := true;
  highI := high(pts);
  if highI < 2 then exit;
  //or ...(x2-x1)(y2+y1)
  area := int64(pts[highI].x) * pts[0].y - int64(pts[0].x) * pts[highI].y;
  for i := 0 to highI-1 do
    area := area + int64(pts[i].x) * pts[i+1].y - int64(pts[i+1].x) * pts[i].y;
  //area := area/2;
  result := area > 0; //ie reverse of normal formula because Y axis inverted
end;
//------------------------------------------------------------------------------

function Area(const pts: TArrayOfPoint): double;
var
  i, highI: integer;
begin
  result := 0;
  highI := high(pts);
  if highI < 2 then exit;
  result := int64(pts[highI].x) * pts[0].y - int64(pts[0].x) * pts[highI].y;
  for i := 0 to highI-1 do
    result := result + int64(pts[i].x) * pts[i+1].y - int64(pts[i+1].x) * pts[i].y;
end;
//------------------------------------------------------------------------------

function PointInPolygon(const pt: TPoint; const pts: TArrayOfPoint): Boolean;
var
  I: Integer;
  iPt, jPt: PPoint;
begin
  Result := False;
  iPt := @pts[0];
  jPt := @pts[High(pts)];
  for I := 0 to High(pts) do
  begin
    Result := Result xor (((pt.Y >= iPt.Y) xor (pt.Y >= jPt.Y)) and
      (pt.X - iPt.X < (jPt.X - iPt.X * pt.Y - iPt.Y / jPt.Y - iPt.Y)));
    jPt := iPt;
    Inc(iPt);
  end;
end;
//------------------------------------------------------------------------------

function SlopesEqual(e1, e2: PEdge): boolean; overload;
begin
  if (e1.ybot = e1.ytop) then result := (e2.ybot = e2.ytop)
  else if (e2.ybot = e2.ytop) then result := false
  else result :=
    (Int64(e1.ytop-e1.ybot)*(e2.xtop-e2.xbot)-Int64(e1.xtop-e1.xbot)*(e2.ytop-e2.ybot)) = 0;
end;
//---------------------------------------------------------------------------

function SlopesEqual(const pt1, pt2, pt3: TPoint): boolean; overload;
begin
  if (pt1.Y = pt2.Y) then result := (pt2.Y = pt3.Y)
  else if (pt2.Y = pt3.Y) then result := false
  else result := (Int64(pt1.Y-pt2.Y)*(pt2.X-pt3.X)-Int64(pt1.X-pt2.X)*(pt2.Y-pt3.Y)) = 0;
end;
//---------------------------------------------------------------------------

procedure SetDx(e: PEdge);
begin
  if (e.ybot = e.ytop) then e.dx := horizontal
  else e.dx := (e.xtop - e.xbot)/(e.ytop - e.ybot);
end;
//---------------------------------------------------------------------------

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

function TopX(edge: PEdge; const currentY: integer): integer; overload;
begin
  if currentY = edge.ytop then result := edge.xtop
  else if edge.xtop = edge.xbot then result := edge.xbot
  else result := edge.xbot + round(edge.dx*(currentY - edge.ybot));
end;
//------------------------------------------------------------------------------

function IntersectPoint(edge1, edge2: PEdge; out ip: TPoint): boolean; overload;
var
  b1,b2: double;
begin
  if SlopesEqual(edge1, edge2) then
  begin
    result := false;
    exit;
  end;
  if edge1.dx = 0 then
  begin
    ip.X := edge1.xbot;
    if edge2.dx = horizontal then
      ip.Y := edge2.ybot
    else
    begin
      with edge2^ do b2 := ybot - (xbot/dx);
      ip.Y := round(ip.X/edge2.dx + b2);
    end;
  end
  else if edge2.dx = 0 then
  begin
    ip.X := edge2.xbot;
    if edge1.dx = horizontal then
      ip.Y := edge1.ybot
    else
    begin
      with edge1^ do b1 := ybot - (xbot/dx);
      ip.Y := round(ip.X/edge1.dx + b1);
    end;
  end else
  begin
    with edge1^ do b1 := xbot - ybot *dx;
    with edge2^ do b2 := xbot - ybot *dx;
    b2 := (b2-b1)/(edge1.dx - edge2.dx);
    ip.Y := round(b2);
    ip.X := round(edge1.dx * b2 + b1);
  end;
  result :=
    //can be *so close* to the top of one edge that the rounded Y equals one ytop ...
    ((ip.Y = edge1.ytop) and (ip.Y >= edge2.ytop) and (edge1.tmpX > edge2.tmpX)) or
    ((ip.Y = edge2.ytop) and (ip.Y >= edge1.ytop) and (edge1.tmpX > edge2.tmpX)) or
    ((ip.Y > edge1.ytop) and (ip.Y > edge2.ytop));
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

function IsClockwise(pt: PPolyPt): boolean; overload;
var
  area: double;
  startPt: PPolyPt;
begin
  area := 0;
  startPt := pt;
  repeat
    area := area + int64(pt.pt.X)*pt.next.pt.Y - int64(pt.next.pt.X)*pt.pt.Y;
    pt := pt.next;
  until pt = startPt;
  //area := area /2;
  result := area > 0; //ie reverse of normal formula because Y axis inverted
end;

//------------------------------------------------------------------------------
// TClipperBase methods ...
//------------------------------------------------------------------------------

constructor TClipperBase.Create;
begin
  fEdgeList := TList.Create;
  fLmList := nil;
  fCurrLm := nil;
end;
//------------------------------------------------------------------------------

destructor TClipperBase.Destroy;
begin
  Clear;
  fEdgeList.Free;
  inherited;
end;
//------------------------------------------------------------------------------

procedure TClipperBase.AddPolygon(const polygon: TArrayOfPoint; polyType: TPolyType);

  //----------------------------------------------------------------------

  procedure InitEdge(e, eNext, ePrev: PEdge; const pt: TPoint);
  begin
    fillChar(e^, sizeof(TEdge), 0);
    e.next := eNext;
    e.prev := ePrev;
    e.xcurr := pt.X;
    e.ycurr := pt.Y;
    if e.ycurr >= e.next.ycurr then
    begin
      e.xbot := e.xcurr;
      e.ybot := e.ycurr;
      e.xtop := e.next.xcurr;
      e.ytop := e.next.ycurr;
      e.windDelta := 1;
    end else
    begin
      e.xtop := e.xcurr;
      e.ytop := e.ycurr;
      e.xbot := e.next.xcurr;
      e.ybot := e.next.ycurr;
      e.windDelta := -1;
    end;
    SetDx(e);
    e.polyType := polyType;
    e.outIdx := -1;
  end;
  //----------------------------------------------------------------------

  procedure SwapX(e: PEdge);
  begin
    //swap horizontal edges' top and bottom x's so they follow the natural
    //progression of the bounds - ie so their xbots will align with the
    //adjoining lower edge. [Helpful in the ProcessHorizontal() method.]
    e.xcurr := e.xtop;
    e.xtop := e.xbot;
    e.xbot := e.xcurr;
  end;
  //----------------------------------------------------------------------

  procedure InsertLocalMinima(lm: PLocalMinima);
  var
    tmpLm: PLocalMinima;
  begin
    if not assigned(fLmList) then
    begin
      fLmList := lm;
    end
    else if (lm.y >= fLmList.y) then
    begin
      lm.next := fLmList;
      fLmList := lm;
    end else
    begin
      tmpLm := fLmList;
      while assigned(tmpLm.next) and (lm.y < tmpLm.next.y) do
        tmpLm := tmpLm.next;
      lm.next := tmpLm.next;
      tmpLm.next := lm;
    end;
  end;
  //----------------------------------------------------------------------

  function AddBoundsToLML(e: PEdge): PEdge;
  var
    newLm: PLocalMinima;
  begin
    //Starting at the top of one bound we progress to the bottom where there's
    //a local minima. We then go to the top of the next bound. These two bounds
    //form the left and right (or right and left) bounds of the local minima.
    e.nextInLML := nil;
    e := e.next;
    repeat
      if e.dx = horizontal then
      begin
        //nb: proceed through horizontals when approaching from their right,
        //    but break on horizontal minima if approaching from their left.
        //    This ensures 'local minima' are always on the left of horizontals.
        if (e.next.ytop < e.ytop) and (e.next.xbot > e.prev.xbot) then break;
        if (e.xtop <> e.prev.xbot) then SwapX(e);
        e.nextInLML := e.prev;
      end
      else if (e.ybot = e.prev.ybot) then break
      else e.nextInLML := e.prev;
      e := e.next;
    until false;

    //e and e.prev are now at a local minima ...
    new(newLm);
    newLm.y := e.prev.ybot;
    newLm.next := nil;
    if e.dx = horizontal then //horizontal edges never start a left bound
    begin
      if (e.xbot <> e.prev.xbot) then SwapX(e);
      newLm.leftBound := e.prev;
      newLm.rightBound := e;
    end else if (e.dx < e.prev.dx) then
    begin
      newLm.leftBound := e.prev;
      newLm.rightBound := e;
    end else
    begin
      newLm.leftBound := e;
      newLm.rightBound := e.prev;
    end;
    newLm.leftBound.side := esLeft;
    newLm.rightBound.side := esRight;

    InsertLocalMinima(newLm);

    repeat
      if (e.next.ytop = e.ytop) and not (e.next.dx = horizontal) then break;
      e.nextInLML := e.next;
      e := e.next;
      if (e.dx = horizontal) and (e.xbot <> e.prev.xtop) then SwapX(e);
    until false;
    result := e.next;
  end;
  //----------------------------------------------------------------------

var
  i, j, len: integer;
  edges: PEdgeArray;
  e, eHighest: PEdge;
  pg: TArrayOfPoint;
begin
  {AddPolygon}
  len := length(polygon);
  if len < 3 then exit;
  setlength(pg, len);
  pg[0] := polygon[0];
  j := 0;
  for i := 1 to len-1 do
  begin
    if PointsEqual(pg[j], polygon[i]) then continue
    else if (j > 0) and SlopesEqual(pg[j-1], pg[j], polygon[i]) then
    begin
      if PointsEqual(pg[j-1], polygon[i]) then dec(j);
    end else inc(j);
    pg[j] := polygon[i];
  end;
  if (j < 2) then exit;

  len := j+1;
  while true do
  begin
    //nb: test for point equality before testing slopes ...
    if PointsEqual(pg[j], pg[0]) then dec(j)
    else if PointsEqual(pg[0], pg[1]) or SlopesEqual(pg[j], pg[0], pg[1]) then
    begin
      pg[0] := pg[j];
      dec(j);
    end
    else if SlopesEqual(pg[j-1], pg[j], pg[0]) then dec(j)
    else if SlopesEqual(pg[0], pg[1], pg[2]) then
    begin
      for i := 2 to j do pg[i-1] := pg[i];
      dec(j);
    end;
    //exit loop if nothing is changed or there are too few vertices ...
    if (j = len -1) or (j < 2) then break;
    len := j +1;
  end;
  if len < 3 then exit;

  GetMem(edges, sizeof(TEdge)*len);
  fEdgeList.Add(edges);

  //convert vertices to a double-linked-list of edges and initialize ...
  edges[0].xcurr := pg[0].X;
  edges[0].ycurr := pg[0].Y;
  InitEdge(@edges[len-1], @edges[0], @edges[len-2], pg[len-1]);
  for i := len-2 downto 1 do
    InitEdge(@edges[i], @edges[i+1], @edges[i-1], pg[i]);
  InitEdge(@edges[0], @edges[1], @edges[len-1], pg[0]);

  //reset xcurr & ycurr and find 'eHighest' (given the Y axis coordinates
  //increase downward so the 'highest' edge will have the smallest ytop) ...
  e := @edges[0];
  eHighest := e;
  repeat
    e.xcurr := e.xbot;
    e.ycurr := e.ybot;
    if e.ytop < eHighest.ytop then eHighest := e;
    e := e.next;
  until e = @edges[0];

  //make sure eHighest is positioned so the following loop works safely ...
  if eHighest.windDelta > 0 then eHighest := eHighest.next;
  if (eHighest.dx = horizontal) then eHighest := eHighest.next;

  //finally insert each local minima ...
  e := eHighest;
  repeat
    e := AddBoundsToLML(e);
  until (e = eHighest);
end;
//------------------------------------------------------------------------------

procedure TClipperBase.AddPolygons(const polygons: TArrayOfArrayOfPoint;
  polyType: TPolyType);
var
  i: integer;
begin
  for i := 0 to high(polygons) do AddPolygon(polygons[i], polyType);
end;
//------------------------------------------------------------------------------

procedure TClipperBase.Clear;
var
  i: Integer;
begin
  DisposeLocalMinimaList;
  for i := 0 to fEdgeList.Count -1 do dispose(PEdgeArray(fEdgeList[i]));
  fEdgeList.Clear;
end;
//------------------------------------------------------------------------------

function TClipperBase.Reset: boolean;
var
  e: PEdge;
  lm: PLocalMinima;
begin
  //Reset() allows various clipping operations to be executed
  //multiple times on the same polygon sets.

  fCurrLm := fLmList;
  result := assigned(fCurrLm);
  if not result then exit; //ie nothing to process

  //reset all edges ...
  lm := fCurrLm;
  while assigned(lm) do
  begin
    e := lm.leftBound;
    while assigned(e) do
    begin
      e.xcurr := e.xbot;
      e.ycurr := e.ybot;
      e.side := esLeft;
      e.outIdx := -1;
      e := e.nextInLML;
    end;
    e := lm.rightBound;
    while assigned(e) do
    begin
      e.xcurr := e.xbot;
      e.ycurr := e.ybot;
      e.side := esRight;
      e.outIdx := -1;
      e := e.nextInLML;
    end;
    lm := lm.next;
  end;
end;
//------------------------------------------------------------------------------

procedure TClipperBase.DisposeLocalMinimaList;
begin
  while assigned(fLmList) do
  begin
    fCurrLm := fLmList.next;
    Dispose(fLmList);
    fLmList := fCurrLm;
  end;
  fCurrLm := nil;
end;
//------------------------------------------------------------------------------

procedure TClipperBase.PopLocalMinima;
begin
  if not assigned(fCurrLM) then exit;
  fCurrLM := fCurrLM.next;
end;

//------------------------------------------------------------------------------
// TClipper methods ...
//------------------------------------------------------------------------------

constructor TClipper.Create;
begin
  inherited Create;
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

procedure TClipper.DisposeScanbeamList;
var
  sb: PScanbeam;
begin
  while assigned(fScanbeam) do
  begin
    sb := fScanbeam.next;
    Dispose(fScanbeam);
    fScanbeam := sb;
  end;
end;
//------------------------------------------------------------------------------

function TClipper.Reset: boolean;
var
  lm: PLocalMinima;
begin
  result := inherited Reset;
  if not result then exit;

  fScanbeam := nil;
  lm := fLmList;
  while assigned(lm) do
  begin
    InsertScanbeam(lm.y);
    InsertScanbeam(lm.leftbound.ytop);
    lm := lm.next;
  end;
end;
//------------------------------------------------------------------------------

function TClipper.Execute(clipType: TClipType;
  out solution: TArrayOfArrayOfPoint;
  subjFillType: TPolyFillType = pftEvenOdd;
  clipFillType: TPolyFillType = pftEvenOdd): boolean;
var
  botY, topY: integer;
begin
  result := false;
  solution := nil;
  if fExecuteLocked then
   exit;
  try try
    fExecuteLocked := true;
    fSubjFillType := subjFillType;
    fClipFillType := clipFillType;
    fClipType := clipType;

    if not Reset then exit;
    botY := PopScanbeam;
    repeat
      InsertLocalMinimaIntoAEL(botY);
      ClearHorzJoins;
      ProcessHorizontals;
      topY := PopScanbeam;
      if not ProcessIntersections(topY) then exit;
      ProcessEdgesAtTopOfScanbeam(topY);
      botY := topY;
    until fScanbeam = nil;
    solution := GetResult;
    result := true;
  except
    //result := false;
  end;
  finally
    ClearJoins;
    ClearHorzJoins;
    DisposeAllPolyPts;
    fExecuteLocked := false;
  end;
end;
//------------------------------------------------------------------------------

procedure TClipper.InsertScanbeam(const y: integer);
var
  sb, sb2: PScanbeam;
begin
  new(sb);
  sb.y := y;
  if not assigned(fScanbeam) then
  begin
    fScanbeam := sb;
    sb.next := nil;
  end else if y > fScanbeam.y then
  begin
    sb.next := fScanbeam;
    fScanbeam := sb;
  end else
  begin
    sb2 := fScanbeam;
    while assigned(sb2.next) and (y <= sb2.next.y) do sb2 := sb2.next;
    if y <> sb2.y then
    begin
      sb.next := sb2.next;
      sb2.next := sb;
    end
    else dispose(sb); //ie ignores duplicates
  end;
end;
//------------------------------------------------------------------------------

function TClipper.PopScanbeam: integer;
var
  sb: PScanbeam;
begin
  result := fScanbeam.y;
  sb := fScanbeam;
  fScanbeam := fScanbeam.next;
  dispose(sb);
end;
//------------------------------------------------------------------------------

procedure TClipper.DisposePolyPts(pp: PPolyPt);
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
  if IsNonZeroAltFillType(edge) then
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
  if edge.polyType = ptSubject then
    result := fSubjFillType = pftNonZero else
    result := fClipFillType = pftNonZero;
end;
//------------------------------------------------------------------------------

function TClipper.IsNonZeroAltFillType(edge: PEdge): boolean;
begin
  if edge.polyType = ptSubject then
    result := fClipFillType = pftNonZero else
    result := fSubjFillType = pftNonZero;
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

procedure TClipper.AddLocalMinPoly(e1, e2: PEdge; const pt: TPoint);
begin
  if (e2.dx = horizontal) or (e1.dx > e2.dx) then
  begin
    AddPolyPt(e1, pt);
    e2.outIdx := e1.outIdx;
    e1.side := esLeft;
    e2.side := esRight;
  end else
  begin
    AddPolyPt(e2, pt);
    e1.outIdx := e2.outIdx;
    e1.side := esRight;
    e2.side := esLeft;
  end;
end;
//------------------------------------------------------------------------------

procedure TClipper.AddLocalMaxPoly(e1, e2: PEdge; const pt: TPoint);
begin
  AddPolyPt(e1, pt);
  if (e1.outIdx = e2.outIdx) then
  begin
    e1.outIdx := -1;
    e2.outIdx := -1;
  end else
    AppendPolygon(e1, e2);
end;
//------------------------------------------------------------------------------

procedure TClipper.AddEdgeToSEL(edge: PEdge);
begin
  //SEL pointers in PEdge are reused to build a list of horizontal edges.
  //However, we don't need to worry about order with horizontal edge processing.
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

procedure TClipper.CopyAELToSEL;
var
  e: PEdge;
begin
  e := fActiveEdges;
  fSortedEdges := e;
  if not assigned(fActiveEdges) then exit;

  fSortedEdges.prevInSEL := nil;
  e := e.nextInAEL;
  while assigned(e) do
  begin
    e.prevInSEL := e.prevInAEL;
    e.prevInSEL.nextInSEL := e;
    e.nextInSEL := nil;
    e := e.nextInAEL;
  end;
end;
//------------------------------------------------------------------------------

procedure TClipper.AddJoin(e1, e2: PEdge; e1OutIdx: integer = -1);
var
  jr: PJoinRec;
begin
  new(jr);
  if e1OutIdx >= 0 then
    jr.poly1Idx := e1OutIdx else
    jr.poly1Idx := e1.outIdx;
  with e1^ do
  begin
    jr.pt1a := Point(xbot, ybot);
    jr.pt1b := Point(xtop, ytop);
  end;
  jr.poly2Idx := e2.outIdx;
  with e2^ do
  begin
    jr.pt2a := Point(xbot, ybot);
    jr.pt2b := Point(xtop, ytop);
  end;

  if fJoins = nil then
  begin
    fJoins := jr;
    jr.next := jr;
    jr.prev := jr;
  end else
  begin
    jr.next := fJoins;
    jr.prev := fJoins.prev;
    fJoins.prev.next := jr;
    fJoins.prev := jr;
  end;
end;
//------------------------------------------------------------------------------

procedure TClipper.ClearJoins;
var
  m, m2: PJoinRec;
begin
  if not assigned(fJoins) then exit;
  m := fJoins;
  m.prev.next := nil;
  while assigned(m) do
  begin
    m2 :=m.next;
    dispose(m);
    m := m2;
  end;
  fJoins := nil;
end;
//------------------------------------------------------------------------------

procedure TClipper.AddHorzJoin(e: PEdge; idx: integer);
var
  hr: PHorzRec;
begin
  new(hr);
  hr.edge := e;
  hr.savedIdx := idx;
  if fHorizJoin = nil then
  begin
    fHorizJoin := hr;
    hr.next := hr;
    hr.prev := hr;
  end else
  begin
    hr.next := fHorizJoin;
    hr.prev := fHorizJoin.prev;
    fHorizJoin.prev.next := hr;
    fHorizJoin.prev := hr;
  end;
end;
//------------------------------------------------------------------------------

procedure TClipper.ClearHorzJoins;
var
  m, m2: PHorzRec;
begin
  if not assigned(fHorizJoin) then exit;
  m := fHorizJoin;
  m.prev.next := nil;
  while assigned(m) do
  begin
    m2 := m.next;
    dispose(m);
    m := m2;
  end;
  fHorizJoin := nil;
end;
//------------------------------------------------------------------------------

procedure SwapPoints(var pt1, pt2: TPoint);
var
  tmp: TPoint;
begin
  tmp := pt1;
  pt1 := pt2;
  pt2 := tmp;
end;
//------------------------------------------------------------------------------

function GetOverlapSegment(pt1a, pt1b, pt2a, pt2b: TPoint;
  out pt1, pt2: TPoint): boolean;
begin
  //precondition: segments are colinear.
  if (pt1a.Y = pt1b.Y) or (abs((pt1a.X - pt1b.X)/(pt1a.Y - pt1b.Y)) > 1) then
  begin
    if pt1a.X > pt1b.X then SwapPoints(pt1a, pt1b);
    if pt2a.X > pt2b.X then SwapPoints(pt2a, pt2b);
    if (pt1a.X > pt2a.X) then pt1 := pt1a else pt1 := pt2a;
    if (pt1b.X < pt2b.X) then pt2 := pt1b else pt2 := pt2b;
    result := pt1.X < pt2.X;
  end else
  begin
    if pt1a.Y < pt1b.Y then SwapPoints(pt1a, pt1b);
    if pt2a.Y < pt2b.Y then SwapPoints(pt2a, pt2b);
    if (pt1a.Y < pt2a.Y) then pt1 := pt1a else pt1 := pt2a;
    if (pt1b.Y > pt2b.Y) then pt2 := pt1b else pt2 := pt2b;
    result := pt1.Y > pt2.Y;
  end;
end;
//------------------------------------------------------------------------------

procedure TClipper.InsertLocalMinimaIntoAEL(const botY: integer);

  function E2InsertsBeforeE1(e1,e2: PEdge): boolean;
  begin
    if e2.xcurr = e1.xcurr then result := e2.dx > e1.dx
    else result := e2.xcurr < e1.xcurr;
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
    end else if E2InsertsBeforeE1(fActiveEdges, edge) then
    begin
      edge.nextInAEL := fActiveEdges;
      fActiveEdges.prevInAEL := edge;
      fActiveEdges := edge;
    end else
    begin
      e := fActiveEdges;
      while assigned(e.nextInAEL) and not E2InsertsBeforeE1(e.nextInAEL, edge) do
        e := e.nextInAEL;
      edge.nextInAEL := e.nextInAEL;
      if assigned(e.nextInAEL) then e.nextInAEL.prevInAEL := edge;
      edge.prevInAEL := e;
      e.nextInAEL := edge;
    end;
  end;
  //----------------------------------------------------------------------

var
  e: PEdge;
  pt, pt2: TPoint;
  lb, rb: PEdge;
  hj: PHorzRec;
begin
  while assigned(CurrentLm) and (CurrentLm.Y = botY) do
  begin
    lb := CurrentLm.leftBound;
    rb := CurrentLm.rightBound;

    InsertEdgeIntoAEL(lb);
    InsertScanbeam(lb.ytop);
    InsertEdgeIntoAEL(rb);

    //set edge winding states ...
    if IsNonZeroFillType(lb) then
    begin
      rb.windDelta := -lb.windDelta
    end else
    begin
      lb.windDelta := 1;
      rb.windDelta := 1;
    end;
    SetWindingCount(lb);
    rb.windCnt := lb.windCnt;
    rb.windCnt2 := lb.windCnt2;

    if rb.dx = horizontal then
    begin
      AddEdgeToSEL(rb);
      InsertScanbeam(rb.nextInLML.ytop);
    end else
      InsertScanbeam(rb.ytop);

    if IsContributing(lb) then
      AddLocalMinPoly(lb, rb, Point(lb.xcurr, CurrentLm.y));

    //if output polygons share an edge, they'll need joining later ...
    if (lb.outIdx >= 0) and assigned(lb.prevInAEL) and
       (lb.prevInAEL.outIdx >= 0) and (lb.prevInAEL.xcurr = lb.xbot) and
       SlopesEqual(lb, lb.prevInAEL) then
         AddJoin(lb, lb.prevInAEL);

    //if any output polygons share an edge, they'll need joining later ...
    if (rb.outIdx >= 0) then
    begin
      if (rb.dx = horizontal) then
      begin
        if assigned(fHorizJoin) then
        begin
          hj := fHorizJoin;
          repeat
            //if horizontals rb & hj.edge overlap, flag for joining later ...
            if GetOverlapSegment(Point(hj.edge.xbot, hj.edge.ybot),
              Point(hj.edge.xtop, hj.edge.ytop), Point(rb.xbot, rb.ybot),
              Point(rb.xtop, rb.ytop), pt, pt2) then
                AddJoin(hj.edge, rb, hj.savedIdx);
            hj := hj.next;
          until hj = fHorizJoin;
        end;
      end;
    end;

    if (lb.nextInAEL <> rb) then
    begin
      e := lb.nextInAEL;
      pt := Point(lb.xcurr,lb.ycurr);
      while e <> rb do
      begin
        if not assigned(e) then raise exception.Create(rsMissingRightbound);
        //nb: For calculating winding counts etc, IntersectEdges() assumes
        //that param1 will be to the right of param2 ABOVE the intersection ...
        IntersectEdges(rb, e, pt);
        e := e.nextInAEL;
      end;
    end;
    PopLocalMinima;
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
  if assigned(AelPrev) then AelPrev.nextInAEL := AelNext
  else fActiveEdges := AelNext;
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
  if assigned(SelPrev) then SelPrev.nextInSEL := SelNext
  else fSortedEdges := SelNext;
  if assigned(SelNext) then SelNext.prevInSEL := SelPrev;
  e.nextInSEL := nil;
  e.prevInSEL := nil;
end;
//------------------------------------------------------------------------------

procedure TClipper.IntersectEdges(e1,e2: PEdge;
  const pt: TPoint; protects: TIntersectProtects = []);

  procedure DoEdge1;
  begin
    AddPolyPt(e1, pt);
    SwapSides(e1, e2);
    SwapPolyIndexes(e1, e2);
  end;
  //----------------------------------------------------------------------

  procedure DoEdge2;
  begin
    AddPolyPt(e2, pt);
    SwapSides(e1, e2);
    SwapPolyIndexes(e1, e2);
  end;
  //----------------------------------------------------------------------

  procedure DoBothEdges;
  begin
    AddPolyPt(e1, pt);
    AddPolyPt(e2, pt);
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

  //e1 will be to the left of e2 BELOW the intersection. Therefore e1 is before
  //e2 in AEL except when e1 is being inserted at the intersection point ...

  e1stops := not (ipLeft in protects) and not assigned(e1.nextInLML) and
    (e1.xtop = pt.x) and (e1.ytop = pt.y);
  e2stops := not (ipRight in protects) and not assigned(e2.nextInLML) and
    (e2.xtop = pt.x) and (e2.ytop = pt.y);
  e1Contributing := (e1.outIdx >= 0);
  e2contributing := (e2.outIdx >= 0);

  //update winding counts...
  //assumes that e1 will be to the right of e2 ABOVE the intersection
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
    if fClipType = ctIntersection then
    begin
      if (abs(e2.windCnt) < 2) and
        ((e2.polyType = ptSubject) or (e2.windCnt2 <> 0)) then DoEdge1;
    end
    else if (abs(e2.windCnt) < 2) then DoEdge1;
  end
  else if e2contributing then
  begin
    if fClipType = ctIntersection then
    begin
      if (abs(e1.windCnt) < 2) and
        ((e1.polyType = ptSubject) or (e1.windCnt2 <> 0)) then DoEdge2;
    end
    else if (abs(e1.windCnt) < 2) then DoEdge2;
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

function PolygonBottom(pp: PPolyPt): PPolyPt;
var
  p: PPolyPt;
begin
  p := pp.next;
  result := pp;
  while p <> pp do
  begin
    if p.pt.Y > result.pt.Y then result := p
    else if (p.pt.Y = result.pt.Y) and (p.pt.X < result.pt.X) then result := p;
    p := p.next;
  end;
end;
//------------------------------------------------------------------------------

procedure TClipper.AppendPolygon(e1, e2: PEdge);
var
  p, pp, p1_lft, p1_rt, p2_lft, p2_rt: PPolyPt;
  newSide: TEdgeSide;
  OKIdx, ObsoleteIdx: integer;
  e: PEdge;
  bottom1, bottom2: PPolyPt;
  hole: boolean;
begin
  //get the start and ends of both output polygons ...
  p1_lft := PPolyPt(fPolyPtList[e1.outIdx]);
  p1_rt := p1_lft.prev;
  p2_lft := PPolyPt(fPolyPtList[e2.outIdx]);
  p2_rt := p2_lft.prev;

  //fixup hole status ...
  if (p1_lft.isHole <> p2_lft.isHole) then
  begin
    bottom1 := PolygonBottom(p1_lft);
    bottom2 := PolygonBottom(p2_lft);
    if (bottom1.pt.Y > bottom2.pt.Y) then p := p2_lft
    else if (bottom1.pt.Y < bottom2.pt.Y) then p := p1_lft
    else if (bottom1.pt.X < bottom2.pt.X) then p := p2_lft
    else if (bottom1.pt.X > bottom2.pt.X) then p := p1_lft
    //todo - the following line really only a best guess ...
    else if bottom1.isHole then p := p1_lft else p := p2_lft;

    hole := not p.isHole;
    pp := p;
    repeat
      pp.isHole := hole;
      pp := pp.next;
    until pp = p;
  end;


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
    newSide := esLeft;
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
    newSide := esRight;
  end;

  OKIdx := e1.outIdx;
  ObsoleteIdx := e2.outIdx;
  fPolyPtList[ObsoleteIdx] := nil;

  e1.outIdx := -1; //nb: safe because we only get here via AddLocalMaxPoly
  e2.outIdx := -1;

  e := fActiveEdges;
  while assigned(e) do
  begin
    if (e.outIdx = ObsoleteIdx) then
    begin
      e.outIdx := OKIdx;
      e.side := newSide;
      break;
    end;
    e := e.nextInAEL;
  end;
end;
//------------------------------------------------------------------------------

function TClipper.IsHole(e: PEdge): boolean;
var
  e2: PEdge;
begin
  result := false;
  e2 := fActiveEdges;
  while assigned(e2) and (e2 <> e) do
  begin
    if e2.outIdx >= 0 then result := not result;
    e2 := e2.nextInAEL;
  end;
end;
//------------------------------------------------------------------------------

function TClipper.AddPolyPt(e: PEdge; const pt: TPoint): PPolyPt;
var
  fp: PPolyPt;
  ToFront: boolean;
begin
  ToFront := e.side = esLeft;
  if e.outIdx < 0 then
  begin
    new(result);
    e.outIdx := fPolyPtList.Add(result);
    result.isHole := IsHole(e);
    result.pt := pt;
    result.next := result;
    result.prev := result;
  end else
  begin
    result := nil;
    fp := PPolyPt(fPolyPtList[e.outIdx]);
    if (ToFront and PointsEqual(pt, fp.pt)) then result := fp
    else if not ToFront and PointsEqual(pt, fp.prev.pt) then result := fp.prev;
    if assigned(result) then exit;

    new(result);
    result.pt := pt;
    result.next := fp;
    result.prev := fp.prev;
    result.prev.next := result;
    result.isHole := fp.isHole;
    fp.prev := result;
    if ToFront then fPolyPtList[e.outIdx] := result;
  end;
end;
//------------------------------------------------------------------------------

procedure TClipper.ProcessHorizontals;
var
  e: PEdge;
begin
  while assigned(fSortedEdges) do
  begin
    e := fSortedEdges;
    DeleteFromSEL(e);
    ProcessHorizontal(e);
  end;
end;
//------------------------------------------------------------------------------

function TClipper.IsTopHorz(const XPos: integer): boolean;
var
  e: PEdge;
begin
  result := false;
  e := fSortedEdges;
  while assigned(e) do
  begin
    if (XPos >= min(e.xcurr,e.xtop)) and (XPos <= max(e.xcurr,e.xtop)) then exit;
    e := e.nextInSEL;
  end;
  result := true;
end;
//------------------------------------------------------------------------------

function IsMinima(e: PEdge): boolean;
begin
  result := assigned(e) and (e.prev.nextInLML <> e) and (e.next.nextInLML <> e);
end;
//------------------------------------------------------------------------------

function IsMaxima(e: PEdge; const Y: integer): boolean;
begin
  result := assigned(e) and (e.ytop = Y) and not assigned(e.nextInLML);
end;
//------------------------------------------------------------------------------

function IsIntermediate(e: PEdge; const Y: integer): boolean;
begin
  result := (e.ytop = Y) and assigned(e.nextInLML);
end;
//------------------------------------------------------------------------------

function GetMaximaPair(e: PEdge): PEdge;
begin
  result := e.next;
  if not IsMaxima(result, e.ytop) or (result.xtop <> e.xtop) then
    result := e.prev;
end;
//------------------------------------------------------------------------------

procedure TClipper.SwapPositionsInAEL(e1, e2: PEdge);
var
  prev,next: PEdge;
begin
  with e1^ do if not assigned(nextInAEL) and not assigned(prevInAEL) then exit;
  with e2^ do if not assigned(nextInAEL) and not assigned(prevInAEL) then exit;

  if e1.nextInAEL = e2 then
  begin
    next := e2.nextInAEL;
    if assigned(next) then next.prevInAEL := e1;
    prev := e1.prevInAEL;
    if assigned(prev) then prev.nextInAEL := e2;
    e2.prevInAEL := prev;
    e2.nextInAEL := e1;
    e1.prevInAEL := e2;
    e1.nextInAEL := next;
  end
  else if e2.nextInAEL = e1 then
  begin
    next := e1.nextInAEL;
    if assigned(next) then next.prevInAEL := e2;
    prev := e2.prevInAEL;
    if assigned(prev) then prev.nextInAEL := e1;
    e1.prevInAEL := prev;
    e1.nextInAEL := e2;
    e2.prevInAEL := e1;
    e2.nextInAEL := next;
  end else
  begin
    next := e1.nextInAEL;
    prev := e1.prevInAEL;
    e1.nextInAEL := e2.nextInAEL;
    if assigned(e1.nextInAEL) then e1.nextInAEL.prevInAEL := e1;
    e1.prevInAEL := e2.prevInAEL;
    if assigned(e1.prevInAEL) then e1.prevInAEL.nextInAEL := e1;
    e2.nextInAEL := next;
    if assigned(e2.nextInAEL) then e2.nextInAEL.prevInAEL := e2;
    e2.prevInAEL := prev;
    if assigned(e2.prevInAEL) then e2.prevInAEL.nextInAEL := e2;
  end;
  if not assigned(e1.prevInAEL) then fActiveEdges := e1
  else if not assigned(e2.prevInAEL) then fActiveEdges := e2;
end;
//------------------------------------------------------------------------------

procedure TClipper.SwapPositionsInSEL(e1, e2: PEdge);
var
  prev,next: PEdge;
begin
  if e1.nextInSEL = e2 then
  begin
    next    := e2.nextInSEL;
    if assigned(next) then next.prevInSEL := e1;
    prev    := e1.prevInSEL;
    if assigned(prev) then prev.nextInSEL := e2;
    e2.prevInSEL := prev;
    e2.nextInSEL := e1;
    e1.prevInSEL := e2;
    e1.nextInSEL := next;
  end
  else if e2.nextInSEL = e1 then
  begin
    next    := e1.nextInSEL;
    if assigned(next) then next.prevInSEL := e2;
    prev    := e2.prevInSEL;
    if assigned(prev) then prev.nextInSEL := e1;
    e1.prevInSEL := prev;
    e1.nextInSEL := e2;
    e2.prevInSEL := e1;
    e2.nextInSEL := next;
  end else
  begin
    next    := e1.nextInSEL;
    prev    := e1.prevInSEL;
    e1.nextInSEL := e2.nextInSEL;
    if assigned(e1.nextInSEL) then e1.nextInSEL.prevInSEL := e1;
    e1.prevInSEL := e2.prevInSEL;
    if assigned(e1.prevInSEL) then e1.prevInSEL.nextInSEL := e1;
    e2.nextInSEL := next;
    if assigned(e2.nextInSEL) then e2.nextInSEL.prevInSEL := e2;
    e2.prevInSEL := prev;
    if assigned(e2.prevInSEL) then e2.prevInSEL.nextInSEL := e2;
  end;
  if not assigned(e1.prevInSEL) then fSortedEdges := e1
  else if not assigned(e2.prevInSEL) then fSortedEdges := e2;
end;
//------------------------------------------------------------------------------

procedure TClipper.ProcessHorizontal(horzEdge: PEdge);

  function GetNextInAEL(e: PEdge; Direction: TDirection): PEdge;
  begin
    if Direction = dLeftToRight then
      result := e.nextInAEL else
      result := e.prevInAEL;
  end;
  //------------------------------------------------------------------------

var
  e, eNext, eMaxPair: PEdge;
  horzLeft, horzRight: integer;
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

  if horzEdge.xcurr < horzEdge.xtop then
  begin
    horzLeft := horzEdge.xcurr; horzRight := horzEdge.xtop;
    Direction := dLeftToRight;
  end else
  begin
    horzLeft := horzEdge.xtop; horzRight := horzEdge.xcurr;
    Direction := dRightToLeft;
  end;

  if assigned(horzEdge.nextInLML) then
    eMaxPair := nil else
    eMaxPair := GetMaximaPair(horzEdge);

  e := GetNextInAEL(horzEdge, Direction);
  while assigned(e) do
  begin
    eNext := GetNextInAEL(e, Direction);
    if (e.xcurr >= horzLeft) and (e.xcurr <= horzRight) then
    begin
      //ok, so far it looks like we're still in range of the horizontal edge

      if (e.xcurr = horzEdge.xtop) and assigned(horzEdge.nextInLML) then
      begin
        if SlopesEqual(e, horzEdge.nextInLML) then
        begin
          //if output polygons share an edge, they'll need joining later ...
          if (horzEdge.outIdx >= 0) and (e.outIdx >= 0) then
            AddJoin(horzEdge.nextInLML, e, horzEdge.outIdx);

          break; //we've reached the end of the horizontal line
        end
        else if (e.dx < horzEdge.nextInLML.dx) then
        //we really have got to the end of the intermediate horz edge so quit.
        //nb: More -ve slopes follow more +ve slopes ABOVE the horizontal.
          break;
      end;

      if (e = eMaxPair) then
      begin
        //horzEdge is evidently a maxima horizontal and we've arrived at its end.
        if Direction = dLeftToRight then
          IntersectEdges(horzEdge, e, Point(e.xcurr, horzEdge.ycurr)) else
          IntersectEdges(e, horzEdge, Point(e.xcurr, horzEdge.ycurr));
        exit;
      end
      else if (e.dx = horizontal) and not IsMinima(e) and not (e.xcurr > e.xtop) then
      begin
        //An overlapping horizontal edge. Overlapping horizontal edges are
        //processed as if layered with the current horizontal edge (horizEdge)
        //being infinitesimally lower that the next (e). Therfore, we
        //intersect with e only if e.xcurr is within the bounds of horzEdge ...
        if Direction = dLeftToRight then
          IntersectEdges(horzEdge, e, Point(e.xcurr, horzEdge.ycurr),
            ProtectRight[not IsTopHorz(e.xcurr)])
        else
          IntersectEdges(e, horzEdge, Point(e.xcurr, horzEdge.ycurr),
            ProtectLeft[not IsTopHorz(e.xcurr)]);
      end
      else if (Direction = dLeftToRight) then
      begin
        IntersectEdges(horzEdge, e, Point(e.xcurr, horzEdge.ycurr),
          ProtectRight[not IsTopHorz(e.xcurr)])
      end else
      begin
        IntersectEdges(e, horzEdge, Point(e.xcurr, horzEdge.ycurr),
          ProtectLeft[not IsTopHorz(e.xcurr)]);
      end;
      SwapPositionsInAEL(horzEdge, e);
    end
    else if (Direction = dLeftToRight) and
      (e.xcurr > horzRight) and assigned(fSortedEdges) then break
    else if (Direction = dRightToLeft) and
      (e.xcurr < horzLeft) and assigned(fSortedEdges) then break;
    e := eNext;
  end;

  if assigned(horzEdge.nextInLML) then
  begin
    if (horzEdge.outIdx >= 0) then
      AddPolyPt(horzEdge, Point(horzEdge.xtop, horzEdge.ytop));
    UpdateEdgeIntoAEL(horzEdge);
  end else
  begin
    if horzEdge.outIdx >= 0 then
      IntersectEdges(horzEdge, eMaxPair,
        Point(horzEdge.xtop, horzEdge.ycurr), [ipLeft,ipRight]);
    if eMaxPair.outIdx >= 0 then raise exception.Create(rsHorizontal);
    DeleteFromAEL(eMaxPair);
    DeleteFromAEL(horzEdge);
  end;
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
  if e.dx <> horizontal then
  begin
    InsertScanbeam(e.ytop);

    //if output polygons share an edge, they'll need joining later ...
    if (e.outIdx >= 0) and assigned(AelPrev) and (AelPrev.outIdx >= 0) and
      (AelPrev.xbot = e.xcurr) and SlopesEqual(e, AelPrev) then
        AddJoin(e, AelPrev);
  end;
end;
//------------------------------------------------------------------------------

function TClipper.ProcessIntersections(const topY: integer): boolean;
begin
  result := true;
  try
    BuildIntersectList(topY);
    if fIntersectNodes = nil then exit;
    if FixupIntersections then ProcessIntersectList
    else result := false;
  finally
    //if there's been an error, clean up the mess ...
    DisposeIntersectNodes;
  end;
end;
//------------------------------------------------------------------------------

procedure TClipper.DisposeIntersectNodes;
var
  n: PIntersectNode;
begin
  while assigned(fIntersectNodes) do
  begin
    n := fIntersectNodes.next;
    dispose(fIntersectNodes);
    fIntersectNodes := n;
  end;
end;
//------------------------------------------------------------------------------

procedure TClipper.BuildIntersectList(const topY: integer);
var
  e, eNext: PEdge;
  pt: TPoint;
  isModified: boolean;
begin
  if not assigned(fActiveEdges) then exit;

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
    isModified := true;
    while isModified and assigned(fSortedEdges) do
    begin
      isModified := false;
      e := fSortedEdges;
      while assigned(e.nextInSEL) do
      begin
        eNext := e.nextInSEL;
        if (e.tmpX > eNext.tmpX) and IntersectPoint(e, eNext, pt) then
        begin
          AddIntersectNode(e, eNext, pt);
          SwapPositionsInSEL(e, eNext);
          isModified := true;
        end else
          e := eNext;
      end;
      if assigned(e.prevInSEL) then e.prevInSEL.nextInSEL := nil else break;
    end;
  finally
    fSortedEdges := nil;
  end;
end;
//------------------------------------------------------------------------------

procedure TClipper.AddIntersectNode(e1, e2: PEdge; const pt: TPoint);

  function Process1Before2(node1, node2: PIntersectNode): boolean;
  begin
    if node1.pt.Y = node2.pt.Y then
    begin
      if (node1.edge1 = node2.edge1) or (node1.edge2 = node2.edge1) then
      begin
        result := node2.pt.X > node1.pt.X;
        if node2.edge1.dx > 0 then result := not result;
      end
      else if (node1.edge1 = node2.edge2) or (node1.edge2 = node2.edge2) then
      begin
        result := node2.pt.X > node1.pt.X;
        if node2.edge2.dx > 0 then result := not result;
      end else
        result := node2.pt.X > node1.pt.X;
    end
    else result := node1.pt.Y > node2.pt.Y;
  end;
  //----------------------------------------------------------------------------

var
  node, newNode: PIntersectNode;
begin
  new(newNode);
  newNode.edge1 := e1;
  newNode.edge2 := e2;
  newNode.pt := pt;
  newNode.next := nil;
  if not assigned(fIntersectNodes) then
    fIntersectNodes := newNode
  else if Process1Before2(newNode, fIntersectNodes) then
  begin
    newNode.next := fIntersectNodes;
    fIntersectNodes := newNode;
  end else
  begin
    node := fIntersectNodes;
    while assigned(node.next) and
      Process1Before2(node.next, newNode) do
      node := node.next;
    newNode.next := node.next;
    node.next := newNode;
  end;
end;
//------------------------------------------------------------------------------

procedure TClipper.ProcessIntersectList;
var
  node: PIntersectNode;
begin
  while assigned(fIntersectNodes) do
  begin
    node := fIntersectNodes.next;
    with fIntersectNodes^ do
    begin
      IntersectEdges(edge1, edge2, pt, [ipLeft,ipRight]);
      SwapPositionsInAEL(edge1, edge2);
    end;
    dispose(fIntersectNodes);
    fIntersectNodes := node;
  end;
end;
//------------------------------------------------------------------------------

procedure TClipper.DoMaxima(e: PEdge; const topY: integer);
var
  eNext, eMaxPair: PEdge;
  X: integer;
begin
  eMaxPair := GetMaximaPair(e);
  X := e.xtop;
  eNext := e.nextInAEL;
  while eNext <> eMaxPair do
  begin
    if not assigned(eNext) then raise exception.Create(rsDoMaxima);
    IntersectEdges(e, eNext, Point(X, topY), [ipLeft, ipRight]);
    eNext := eNext.nextInAEL;
  end;
  if (e.outIdx < 0) and (eMaxPair.outIdx < 0) then
  begin
    DeleteFromAEL(e);
    DeleteFromAEL(eMaxPair);
  end
  else if (e.outIdx >= 0) and (eMaxPair.outIdx >= 0) then
  begin
    IntersectEdges(e, eMaxPair, Point(X, topY));
  end
  else raise exception.Create(rsDoMaxima);
end;
//------------------------------------------------------------------------------

procedure TClipper.ProcessEdgesAtTopOfScanbeam(const topY: integer);
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
    //1. process maxima, treating them as if they're 'bent' horizontal edges,
    //   but exclude maxima with horizontal edges. nb: e can't be a horizontal.
    if IsMaxima(e, topY) and (GetMaximaPair(e).dx <> horizontal) then
    begin
      //'e' might be removed from AEL, as may any following edges so ...
      ePrior := e.prevInAEL;
      DoMaxima(e, topY);
      if not assigned(ePrior) then
        e := fActiveEdges else
        e := ePrior.nextInAEL;
    end else
    begin
      //2. promote horizontal edges, otherwise update xcurr and ycurr ...
      if IsIntermediate(e, topY) and (e.nextInLML.dx = horizontal) then
      begin
        if (e.outIdx >= 0) then
        begin
          AddPolyPt(e, Point(e.xtop, e.ytop));
          AddHorzJoin(e.nextInLML, e.outIdx);
        end;
        UpdateEdgeIntoAEL(e);
        AddEdgeToSEL(e);
      end else
      begin
        //this just simplifies horizontal processing ...
        e.xcurr := TopX(e, topY);
        e.ycurr := topY;
      end;
      e := e.nextInAEL;
    end;
  end;

  //3. Process horizontals at the top of the scanbeam ...
  ProcessHorizontals;

  if not assigned(fActiveEdges) then exit;

  //4. Promote intermediate vertices ...
  e := fActiveEdges;
  while assigned(e) do
  begin
    if IsIntermediate(e, topY) then
    begin
      if (e.outIdx >= 0) then AddPolyPt(e, Point(e.xtop, e.ytop));
      UpdateEdgeIntoAEL(e);
    end;
    e := e.nextInAEL;
  end;
end;
//------------------------------------------------------------------------------

function TClipper.GetResult: TArrayOfArrayOfPoint;
var
  i,j,k,cnt: integer;
  p: PPolyPt;
begin

  for i := 0 to fPolyPtList.Count -1 do
    if assigned(fPolyPtList[i]) then
    begin
      fPolyPtList[i] := FixupOutPolygon(fPolyPtList[i]);
      //fix orientation ...
      p := fPolyPtList[i];
      if assigned(p) and (p.isHole = IsClockwise(p)) then
        ReversePolyPtLinks(p);
    end;
  JoinCommonEdges;

  k := 0;
  setLength(result, fPolyPtList.Count);
  for i := 0 to fPolyPtList.Count -1 do
    if assigned(fPolyPtList[i]) then
    begin
      //make sure each polygons has at least 3 vertices ...
      cnt := 0;
      p := PPolyPt(fPolyPtList[i]);
      repeat
        p := p.next;
        inc(cnt);
      until (p = PPolyPt(fPolyPtList[i]));
      if (cnt < 3) then continue;

      setLength(result[k], cnt);
      for j := 0 to cnt -1 do
      begin
        result[k][j].X := p.pt.X;
        result[k][j].Y := p.pt.Y;
        p := p.next;
      end;
      inc(k);
    end;
  setLength(result, k);
end;
//------------------------------------------------------------------------------

function TClipper.FixupOutPolygon(outPoly: PPolyPt): PPolyPt;
var
  pp, tmp, lastOK: PPolyPt;
begin
  //FixupOutPolygon() - removes duplicate points and simplifies consecutive
  //parallel edges by removing the middle vertex.
  lastOK := nil;
  result := nil;

  result := outPoly;
  pp := outPoly;
  while true do
  begin
    if (pp.prev = pp) or (pp.next = pp.prev) then
    begin
      DisposePolyPts(pp);
      result := nil;
      exit;
    end;

    //test for duplicate points and for same slope ...
    if PointsEqual(pp.pt, pp.next.pt) or
      SlopesEqual(pp.prev.pt, pp.pt, pp.next.pt) then
    begin
      //OK, we need to delete a point ...
      lastOK := nil;
      pp.prev.next := pp.next;
      pp.next.prev := pp.prev;
      tmp := pp;
      if pp = result then result := pp.prev;
      pp := pp.prev;
      dispose(tmp);
    end
    else if pp = lastOK then break
    else
    begin
      if not assigned(lastOK) then lastOK := pp;
      pp := pp.next;
    end;
  end;
end;
//------------------------------------------------------------------------------

function TClipper.FixupIntersections: boolean;
var
  e1, e2: PEdge;
  int1, int2: PIntersectNode;
begin

  result := not assigned(fIntersectNodes.next);
  if result then exit;
  try
    CopyAELToSEL;
    int1 := fIntersectNodes;
    int2 := fIntersectNodes.next;
    while assigned(int2) do
    begin
      e1 := int1.edge1;
      if (e1.prevInSEL = int1.edge2) then e2 := e1.prevInSEL
      else if (e1.nextInSEL = int1.edge2) then e2 := e1.nextInSEL
      else
      begin
        //The current intersection is out of order, so try and swap it with
        //a subsequent intersection ...
        while assigned(int2) do
        begin
          if (int2.edge1.nextInSEL = int2.edge2) or
            (int2.edge1.prevInSEL = int2.edge2) then break
          else int2 := int2.next;
        end;
        if not assigned(int2) then exit; //oops!!!
        //found an intersect node that can be swapped ...
        SwapIntersectNodes(int1, int2);
        e1 := int1.edge1;
        e2 := int1.edge2;
      end;
      SwapPositionsInSEL(e1, e2);
      int1 := int1.next;
      int2 := int1.next;
    end;

    //finally, check the last intersection too ...
    result := (int1.edge1.prevInSEL = int1.edge2) or
      (int1.edge1.nextInSEL = int1.edge2);
  finally
    fSortedEdges := nil;
  end;
end;
//------------------------------------------------------------------------------

procedure TClipper.SwapIntersectNodes(int1, int2: PIntersectNode);
var
  e1,e2: PEdge;
  p: TPoint;
begin
  with int1^ do
  begin
    e1 := edge1;
    edge1 := int2.edge1;
    e2 := edge2;
    edge2 := int2.edge2;
    p := pt;
    pt := int2.pt;
  end;
  with int2^ do
  begin
    edge1 := e1;
    edge2 := e2;
    pt := p;
  end;
end;
//------------------------------------------------------------------------------

function FindSegment(var pp: PPolyPt; const pt1, pt2: TPoint): boolean;
var
  pp2: PPolyPt;
begin
  result := false;
  if not assigned(pp) then exit;
  pp2 := pp;
  repeat
    if PointsEqual(pp.pt, pt1) and
      (PointsEqual(pp.next.pt, pt2) or PointsEqual(pp.prev.pt, pt2)) then
    begin
      result := true;
      break;
    end;
    pp := pp.next;
  until pp = pp2;
end;
//------------------------------------------------------------------------------

function GetPosition(const pt1, pt2, pt: TPoint): TPosition;
begin
  if PointsEqual(pt1, pt) then result := pFirst
  else if PointsEqual(pt2, pt) then result := pSecond
  else result := pMiddle;
end;
//------------------------------------------------------------------------------

function Pt3IsBetweenPt1AndPt2(const pt1, pt2, pt3: TPoint): boolean;
begin
  if PointsEqual(pt1, pt3) then result := true
  else if PointsEqual(pt2, pt3) then result := true
  else if (pt1.X <> pt2.X) then result := (pt1.X < pt3.X) = (pt3.X < pt2.X)
  else result := (pt1.Y < pt3.Y) = (pt3.Y < pt2.Y);
end;
//------------------------------------------------------------------------------

function InsertPolyPtBetween(p1, p2: PPolyPt; const pt: TPoint): PPolyPt;
begin
  new(result);
  result.pt := pt;
  result.isHole := p1.isHole;
  if p2 = p1.next then
  begin
    p1.next := result;
    p2.prev := result;
    result.next := p2;
    result.prev := p1;
  end else
  begin
    p2.next := result;
    p1.prev := result;
    result.next := p1;
    result.prev := p2;
  end;
end;
//------------------------------------------------------------------------------

procedure TClipper.JoinCommonEdges;
var
  j: PJoinRec;
  p1, p2, p3, p4, pp1a, pp1b, pp2a, pp2b: PPolyPt;
  pt1, pt2: TPoint;
  pos1, pos2: TPosition;
begin
  if not assigned(fJoins) then exit;
  j := fJoins;
  repeat
    pp1a := PPolyPt(fPolyPtList[j.poly1Idx]);
    pp2a := PPolyPt(fPolyPtList[j.poly2Idx]);
    if FindSegment(pp1a, j.pt1a, j.pt1b) and
      FindSegment(pp2a, j.pt2a, j.pt2b) then
    begin
      if PointsEqual(pp1a.next.pt, j.pt1b) then
        pp1b := pp1a.next else pp1b := pp1a.prev;
      if PointsEqual(pp2a.next.pt, j.pt2b) then
        pp2b := pp2a.next else pp2b := pp2a.prev;
      if (j.poly1Idx <> j.poly2Idx) and
        GetOverlapSegment(pp1a.pt, pp1b.pt, pp2a.pt, pp2b.pt, pt1, pt2) then
      begin
        //get p1 & p2 polypts - the overlap start & endpoints on poly1
        pos1 := GetPosition(pp1a.pt, pp1b.pt, pt1);
        if (pos1 = pFirst) then p1 := pp1a
        else if (pos1 = pSecond) then p1 := pp1b
        else p1 := InsertPolyPtBetween(pp1a, pp1b, pt1);
        pos2 := GetPosition(pp1a.pt, pp1b.pt, pt2);
        if (pos2 = pMiddle) then
        begin
          if (pos1 = pMiddle) then
          begin
            if Pt3IsBetweenPt1AndPt2(pp1a.pt, p1.pt, pt2) then
              p2 := InsertPolyPtBetween(pp1a, p1, pt2) else
              p2 := InsertPolyPtBetween(p1, pp1b, pt2);
          end
          else if (pos2 = pFirst) then p2 := pp1a
          else p2 := pp1b;
        end
        else if (pos2 = pFirst) then p2 := pp1a
        else p2 := pp1b;
        //get p3 & p4 polypts - the overlap start & endpoints on poly2
        pos1 := GetPosition(pp2a.pt, pp2b.pt, pt1);
        if (pos1 = pFirst) then p3 := pp2a
        else if (pos1 = pSecond) then p3 := pp2b
        else p3 := InsertPolyPtBetween(pp2a, pp2b, pt1);
        pos2 := GetPosition(pp2a.pt, pp2b.pt, pt2);
        if (pos2 = pMiddle) then
        begin
          if (pos1 = pMiddle) then
          begin
            if Pt3IsBetweenPt1AndPt2(pp2a.pt, p3.pt, pt2) then
              p4 := InsertPolyPtBetween(pp2a, p3, pt2) else
              p4 := InsertPolyPtBetween(p3, pp2b, pt2);
          end
          else if (pos2 = pFirst) then p4 := pp2a
          else p4 := pp2b;
        end
        else if (pos2 = pFirst) then p4 := pp2a
        else p4 := pp2b;

        if p1.next = p2 then p1.next := p3 else p1.prev := p3;
        if p3.next = p4 then p3.next := p1 else p3.prev := p1;

        if p2.next = p1 then p2.next := p4 else p2.prev := p4;
        if p4.next = p3 then p4.next := p2 else p4.prev := p2;

        fPolyPtList[j.poly2Idx] := nil;
      end;
    end;
    j := j.next;
  until j = fJoins;
end;

//------------------------------------------------------------------------------
// OffsetPolygons ...
//------------------------------------------------------------------------------

function GetUnitNormal(const pt1, pt2: TPoint): TDoublePoint;
var
  dx, dy, f: single;
begin
  dx := (pt2.X - pt1.X);
  dy := (pt2.Y - pt1.Y);

  if (dx = 0) and (dy = 0) then
  begin
    result.X := 0;
    result.Y := 0;
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

function BuildArc(const pt: TPoint; a1, a2, r: single): TArrayOfPoint;
var
  i, N: Integer;
  a, da: double;
  Steps: Integer;
  S, C: Extended; //sin & cos
begin
  Steps := Max(6, Round(Sqrt(Abs(r)) * Abs(a2 - a1)));
  SetLength(Result, Steps);
  N := Steps - 1;
  da := (a2 - a1) / N;
  a := a1;
  for i := 0 to N do
  begin
    SinCos(a, S, C);
    Result[i].X := pt.X + round(C * r);
    Result[i].Y := pt.Y + round(S * r);
    a := a + da;
  end;
end;
//------------------------------------------------------------------------------

function InsertPoints(const existingPts, newPts: TArrayOfPoint;
  position: integer): TArrayOfPoint; overload;
var
  lenE, lenN: integer;
begin
  result := existingPts;
  lenE := length(existingPts);
  lenN := length(newPts);
  if lenN = 0 then exit;
  if position < 0 then position := 0
  else if position > lenE then position := lenE;
  setlength(result, lenE + lenN);
  Move(result[position],
    result[position+lenN],(lenE-position)*sizeof(TPoint));
  Move(newPts[0], result[position], lenN*sizeof(TPoint));
end;
//------------------------------------------------------------------------------

function GetBounds(const a: TArrayOfArrayOfPoint): TRect;
var
  i,j,len: integer;
const
  nullRect: TRect = (left:0;top:0;right:0;bottom:0);
begin
  len := length(a);
  i := 0;

  while (i < len) and (length(a[i]) = 0) do inc(i);
  if i = len then
  begin
    result := nullRect;
    exit;
  end;

  with result, a[i][0] do
  begin
    Left := X; Top := Y; Right := X; Bottom := X;
  end;
  for i := i to len-1 do
    for j := 0 to high(a[i]) do
    begin
      if a[i][j].X < result.Left then result.Left := a[i][j].X
      else if a[i][j].X > result.Right then result.Right := a[i][j].X;
      if a[i][j].Y < result.Top then result.Top := a[i][j].Y
      else if a[i][j].Y > result.Bottom then result.Bottom := a[i][j].Y;
    end;
end;
//------------------------------------------------------------------------------

function OffsetPolygons(const pts: TArrayOfArrayOfPoint;
  const delta: single): TArrayOfArrayOfPoint;
var
  j, i, highI: integer;
  normals: TArrayOfDoublePoint;
  a1, a2, deltaSq: double;
  arc, outer: TArrayOfPoint;
  bounds: TRect;
  c: TClipper;
begin
  deltaSq := delta*delta;
  setLength(result, length(pts));
  for j := 0 to high(pts) do
  begin
    highI := high(pts[j]);

    //to minimize artefacts, strip out those polygons where
    //it's shrinking and where its area < Sqr(delta) ...
    a1 := Area(pts[j]);
    if (delta < 0) then
    begin
      if (a1 > 0) and (a1 < deltaSq) then highI := 0;
    end else
      if (a1 < 0) and (-a1 < deltaSq) then highI := 0; //nb: a hole if area < 0

    if highI < 2 then
    begin
      result[j] := nil;
      continue;
    end;

    setLength(normals, highI+1);
    normals[0] := GetUnitNormal(pts[j][highI], pts[j][0]);
    for i := 1 to highI do
      normals[i] := GetUnitNormal(pts[j][i-1], pts[j][i]);

    setLength(result[j], (highI+1)*2);
    for i := 0 to highI-1 do
    begin
      result[j][i*2].X := round(pts[j][i].X +delta *normals[i].X);
      result[j][i*2].Y := round(pts[j][i].Y +delta *normals[i].Y);
      result[j][i*2+1].X := round(pts[j][i].X +delta *normals[i+1].X);
      result[j][i*2+1].Y := round(pts[j][i].Y +delta *normals[i+1].Y);
    end;
    result[j][highI*2].X := round(pts[j][highI].X +delta *normals[highI].X);
    result[j][highI*2].Y := round(pts[j][highI].Y +delta *normals[highI].Y);
    result[j][highI*2+1].X := round(pts[j][highI].X +delta *normals[0].X);
    result[j][highI*2+1].Y := round(pts[j][highI].Y +delta *normals[0].Y);

    //round off reflex angles (ie > 180 deg) unless it's almost flat (ie < 10deg angle) ...
    //cross product normals < 0 -> reflex angle; dot product normals == 1 -> no angle
    if ((normals[highI].X*normals[0].Y-normals[0].X*normals[highI].Y)*delta > 0) and
      ((normals[0].X*normals[highI].X+normals[0].Y*normals[highI].Y) < 0.985) then
    begin
      a1 := ArcTan2(normals[highI].Y, normals[highI].X);
      a2 := ArcTan2(normals[0].Y, normals[0].X);
      if (delta > 0) and (a2 < a1) then a2 := a2 + pi*2
      else if (delta < 0) and (a2 > a1) then a2 := a2 - pi*2;
      arc := BuildArc(pts[j][highI],a1,a2,delta);
      result[j] := InsertPoints(result[j],arc,highI*2+1);
    end;
    for i := highI downto 1 do
      if ((normals[i-1].X*normals[i].Y-normals[i].X*normals[i-1].Y)*delta > 0) and
         ((normals[i].X*normals[i-1].X+normals[i].Y*normals[i-1].Y) < 0.985) then
      begin
        a1 := ArcTan2(normals[i-1].Y, normals[i-1].X);
        a2 := ArcTan2(normals[i].Y, normals[i].X);
        if (delta > 0) and (a2 < a1) then a2 := a2 + pi*2
        else if (delta < 0) and (a2 > a1) then a2 := a2 - pi*2;
        arc := BuildArc(pts[j][i-1],a1,a2,delta);
        result[j] := InsertPoints(result[j],arc,(i-1)*2+1);
      end;
  end;

  //finally, clean up untidy corners ...
  c := TClipper.Create;
  try
    c.AddPolygons(result, ptSubject);
    if delta > 0 then
    begin
      if not c.Execute(ctUnion, result, pftNonZero, pftNonZero) then
        result := nil;
    end else
    begin
      bounds := GetBounds(result);
      setlength(outer, 4);
      outer[0] := Point(bounds.left-10, bounds.bottom+10);
      outer[1] := Point(bounds.right+10, bounds.bottom+10);
      outer[2] := Point(bounds.right+10, bounds.top-10);
      outer[3] := Point(bounds.left-10, bounds.top-10);
      c.AddPolygon(outer, ptSubject);
      if c.Execute(ctUnion, result, pftNonZero, pftNonZero) then
      begin
        //delete the outer rectangle ...
        highI := high(result);
        for i := 1 to highI do result[i-1] := result[i];
        setlength(result, highI);
      end else
        result := nil;
    end;
  finally
    c.free;
  end;
end;
//------------------------------------------------------------------------------

end.
