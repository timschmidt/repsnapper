unit clipper;

(*******************************************************************************
*                                                                              *
* Author    :  Angus Johnson                                                   *
* Version   :  3.0.2                                                           *
* Date      :  5 February 2011                                                 *
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
  SysUtils, Classes, Math;

type
  PSkipNode = ^TSkipNode;
  TSkipNode = record
    item: pointer;
    level: integer;
    prev: PSkipNode; //ie TSkipNodes form a double linked list
    next: array [0..31] of PSkipNode;
    //nb: we never require (nor allocate memory for) anywhere near 32 levels.
    //The upper array bound depends on (a.) the skip size (currently hardcoded
    //to 4 in TSkipList's constructor), and (b.) the maximum likely number of
    //items in the list. Assuming a skip size of 4 and no more than 10 million
    //items, the maximum levels needed for any item would be 12.
  end;

  TItemsCompare = function (item1, item2: pointer): integer;

  TSkipList = class
  private
    fMaxLevel: integer;
    fSkipFrac: double;
    fCurrentMaxLevel: integer;
    fBase: PSkipNode;
    fCount: integer;
    fCompare: TItemsCompare;
    fLvls: array of PSkipNode;
    procedure CreateBase;
    function NewNode(level: integer; item: pointer): PSkipNode;
  public
    constructor Create(CompareFunc: TItemsCompare);
    destructor Destroy; override;
    procedure Clear;
    //nb: InsertItem fails if a duplicate item is encountered.
    function InsertItem(item: pointer): PSkipNode;
    function FindItem(item: pointer): PSkipNode;
    function DeleteItem(item: pointer): boolean;
    procedure Delete(var node: PSkipNode);
    function PopFirst: pointer;
    function First: PSkipNode;
    function Next(currentNode: PSkipNode): PSkipNode;
    function Prev(currentNode: PSkipNode): PSkipNode;
    function Last: PSkipNode;
    property Count: integer read fCount;
  end;

  TClipType = (ctIntersection, ctUnion, ctDifference, ctXor);
  TPolyType = (ptSubject, ptClip);
  TPolyFillType = (pftEvenOdd, pftNonZero);

  //used internally ...
  TEdgeSide = (esLeft, esRight);
  TIntersectProtect = (ipLeft, ipRight);
  TIntersectProtects = set of TIntersectProtect;

{$IFNDEF USING_GRAPHICS32}
  TFloat = Single;
  TFloatPoint = record X, Y: TFloat; end;
  TArrayOfFloatPoint = array of TFloatPoint;
  TArrayOfArrayOfFloatPoint = array of TArrayOfFloatPoint;
  TFloatRect = record left, top, right, bottom: TFloat; end;
{$ENDIF}
  PDoublePoint = ^TDoublePoint;
  TDoublePoint = record X, Y: double; end;
  TArrayOfDoublePoint = array of TDoublePoint;
  TArrayOfArrayOfDoublePoint = array of TArrayOfDoublePoint;

  PEdge = ^TEdge;
  TEdge = record
    x: double;
    y: double;
    xbot: double;
    ybot: double;
    xtop: double;
    ytop: double;
    dx  : double;
    tmpX:  double;
    nextAtTop: boolean;
    polyType: TPolyType;
    side: TEdgeSide;
    windDelta: integer; //1 or -1 depending on winding direction
    windCnt: integer;
    windCnt2: integer;  //winding count of the opposite polytype
    outIdx: integer;
    next: PEdge;
    prev: PEdge;
    nextInLML: PEdge;
    nextInAEL: PEdge;
    prevInAEL: PEdge;
    nextInSEL: PEdge;
    prevInSEL: PEdge;
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

  //TOrientationFlag:
  //ofClockwise = expected orientation; ofCW = current orientation
  TOrientationFlag = (ofClockwise, ofCW, ofForwardBound, ofTop, ofBottomMinima);
  TOrientationFlags = set of TOrientationFlag;

  PPolyPt = ^TPolyPt;
  TPolyPt = record
    pt: TDoublePoint;
    next: PPolyPt;
    prev: PPolyPt;
    flags: TOrientationFlags;
    dx: double;
  end;

  TJoinRec = record
    pt  : TDoublePoint;
    idx1: integer;
    idx2: integer;
    outPPt: PPolyPt;    //horizontal joins only
  end;

  TArrayOfJoinRec = array of TJoinRec;

  //TClipperBase is the ancestor to the TClipper class. It should not be
  //instantiated directly. This class simply abstracts the conversion of arrays
  //of polygon coords into edge objects that are stored in a LocalMinima list.
  TClipperBase = class
  private
    fList             : TList;
    fLocalMinima      : PLocalMinima;
    fCurrentLM        : PLocalMinima;
    procedure DisposeLocalMinimaList;
  protected
    procedure PopLocalMinima;
    function Reset: boolean;
    property CurrentLM: PLocalMinima read fCurrentLM;
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
    //polygon sets, then Clear circumvents the need to recreate Clipper objects.
    procedure Clear; virtual;
    function GetBounds: TFloatRect;
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
    fClipFillType: TPolyFillType;
    fSubjFillType: TPolyFillType;
    fIntersectTolerance: double;
    fJoins: TArrayOfJoinRec;
    fCurrentHorizontals: TArrayOfJoinRec;
    fIgnoreOrientation: boolean;
    function ResultAsFloatPointArray: TArrayOfArrayOfFloatPoint;
    function ResultAsDoublePointArray: TArrayOfArrayOfDoublePoint;
    function InitializeScanbeam: boolean;
    procedure InsertScanbeam(const y: double);
    function PopScanbeam: double;
    procedure DisposeScanbeamList;
    procedure SetWindingDelta(edge: PEdge);
    procedure SetWindingCount(edge: PEdge);
    function IsNonZeroFillType(edge: PEdge): boolean;
    function IsNonZeroAltFillType(edge: PEdge): boolean;
    procedure InsertLocalMinimaIntoAEL(const botY: double);
    procedure AddEdgeToSEL(edge: PEdge);
    procedure CopyAELToSEL;
    function IsTopHorz(const XPos: double): boolean;
    procedure ProcessHorizontal(horzEdge: PEdge);
    procedure ProcessHorizontals;
    procedure SwapPositionsInAEL(edge1, edge2: PEdge);
    procedure SwapPositionsInSEL(edge1, edge2: PEdge);
    function BubbleSwap(edge: PEdge): PEdge;
    function Process1Before2(Node1, Node2: PIntersectNode): boolean;
    procedure AddIntersectNode(e1, e2: PEdge; const pt: TDoublePoint);
    procedure ProcessIntersections(const topY: double);
    procedure BuildIntersectList(const topY: double);
    function TestIntersections: boolean;
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
    function InsertPolyPtBetween(const pt: TDoublePoint; pp1, pp2: PPolyPt): PPolyPt;
    function AddPolyPt(e: PEdge; const pt: TDoublePoint): PPolyPt;
    procedure AddLocalMaxPoly(e1, e2: PEdge; const pt: TDoublePoint);
    procedure AddLocalMinPoly(e1, e2: PEdge; const pt: TDoublePoint);
    procedure AppendPolygon(e1, e2: PEdge);
    function ExecuteInternal(clipType: TClipType): boolean;
    procedure DisposeAllPolyPts;
    procedure DisposeIntersectNodes;
    procedure MergePolysWithCommonEdges;
    procedure FixOrientation;
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
    //IgnoreOrientation: the Execute method will be approx 60% faster if
    //Clipper doesn't need to calculate the orientation of output polygons ...
    property IgnoreOrientation: boolean
      read fIgnoreOrientation write fIgnoreOrientation;
  end;

  function DoublePoint(const X, Y: double): TDoublePoint; overload;
  function IsClockwise(const pts: TArrayOfDoublePoint): boolean; overload;
  function GetUnitNormal(const pt1, pt2: TDoublePoint): TDoublePoint;

  function MakeArrayOfDoublePoint(const a: TArrayOfFloatPoint): TArrayOfDoublePoint; overload;
  function MakeArrayOfFloatPoint(const a: TArrayOfDoublePoint): TArrayOfFloatPoint; overload;

implementation

type
  TDirection = (dRightToLeft, dLeftToRight);

const
  horizontal: double = -3.4e+38;
  unassigned: double =  3.4e+38;
  tolerance: double = 1.0e-10;
  minimal_tolerance: double = 1.0e-14;
  //precision: defines when adjacent vertices will be considered duplicates
  //and hence ignored. This circumvents edges having indeterminate slope.
  precision: double = 1.0e-6;
  slope_precision: double = 1.0e-3;
  nullRect: TFloatRect =(left:0;top:0;right:0;bottom:0);

resourcestring
  rsSkipListCreate = 'TSkipList create error: no compare function supplied.';
  rsSkipListInsertItem = 'TSkipList - duplicate items not permitted';
  rsMissingRightbound = 'InsertLocalMinimaIntoAEL: missing rightbound';
  rsDoMaxima = 'DoMaxima error';
  rsUpdateEdgeIntoAEL = 'UpdateEdgeIntoAEL error';
  rsProcessEdgesAtTopOfScanbeam = 'ProcessEdgesAtTopOfScanbeam error';
  rsIntersection = 'Intersection error';
  rsInsertPolyPt = 'InsertPolyPtBetween error';
  rsHorizontal = 'ProcessHorizontal error';
  rsCompareForwardAngles = 'CompareForwardAngles error';
  rsFixOrientations = 'FixOrientations error';

//------------------------------------------------------------------------------
// SkipList methods ...
//------------------------------------------------------------------------------

constructor TSkipList.Create(CompareFunc: TItemsCompare);
const
  //defines a likely 'maximum' of 10 million items. The list can still grow
  //beyond this number, but as it does, performance will slowly degrade.
  //nb: available memory starts to be an issue with very large lists where
  //alternative methods of storage should then be considered (eg databases).
  maxItems = 10000000;
  skip = 4;
begin
  Randomize;
  fMaxLevel := trunc( ln(maxItems)/ln(skip) ) +1; //fMaxLevel = 12
  fSkipFrac := 1/skip;
  setlength(fLvls, fMaxLevel);
  if not assigned(CompareFunc) then
    raise exception.Create(rsSkipListCreate);
  fCompare := CompareFunc;
  CreateBase;
end;
//------------------------------------------------------------------------------

destructor TSkipList.Destroy;
begin
  Clear;
  FreeMem(fBase);
  inherited;
end;
//------------------------------------------------------------------------------

procedure TSkipList.CreateBase;
var
  i: integer;
begin
  if assigned(fBase) then exit;
  fBase := NewNode(fMaxLevel-1, nil);

  for i := 0 to fMaxLevel-1 do fBase.next[i] := nil;
  fCurrentMaxLevel := 0;
  fCount := 0;
  fBase.prev := fBase;
end;
//------------------------------------------------------------------------------

procedure TSkipList.Clear;
var
  i: integer;
  tmp, tmp2: PSkipNode;
begin
  tmp := fBase.prev;
  while tmp <> fBase do
  begin
    tmp2 := tmp.prev;
    FreeMem(tmp);
    tmp := tmp2;
  end;

  for i := 0 to fMaxLevel-1 do fBase.next[i] := nil;
  fCurrentMaxLevel := 0;
  fCount := 0;
  fBase.prev := fBase;
end;
//------------------------------------------------------------------------------

function TSkipList.NewNode(level: integer; item: pointer): PSkipNode;
begin
  GetMem(result, sizeof(TSkipNode) + level *sizeof(pointer));
  result.level := level;
  result.item := item;
  inc(fCount);
end;
//------------------------------------------------------------------------------

function TSkipList.First: PSkipNode;
begin
  if fBase.prev = fBase then result := nil else
  result := fBase.next[0];
end;
//------------------------------------------------------------------------------

function TSkipList.Next(currentNode: PSkipNode): PSkipNode;
begin
  if assigned(currentNode) then
    result := currentNode.next[0] else
    result := nil;
end;
//------------------------------------------------------------------------------

function TSkipList.Prev(currentNode: PSkipNode): PSkipNode;
begin
  if assigned(currentNode) then
  begin
    result := currentNode.prev;
    if result = fBase then result := nil;
  end else
    result := nil;
end;
//------------------------------------------------------------------------------

function TSkipList.Last: PSkipNode;
begin
  if fBase.prev = fBase then result := nil else
  result := fBase.prev;
end;
//------------------------------------------------------------------------------

function TSkipList.FindItem(item: pointer): PSkipNode;
var
  i, compareVal: integer;
begin
  result := fBase;
  compareVal := 1;
  for i := fCurrentMaxLevel downto 0 do
  begin
    while assigned(result.next[i]) do
    begin
      if (compareVal > 0) or (result.next[i+1] <> result.next[i]) then
      compareVal := fCompare(result.next[i].item, item);
      if compareVal <= 0 then break
      else result := result.next[i];
    end;
    if (compareVal = 0) then
    begin
      result := result.next[i];
      exit;
    end;
  end;
  result := nil;
end;
//------------------------------------------------------------------------------

function TSkipList.InsertItem(item: pointer): PSkipNode;
var
  i, compareVal, newLevel: integer;
  priorNode: PSkipNode;
begin
  for i := 0 to fMaxLevel -1 do fLvls[i] := fBase;
  priorNode := fBase;
  result := nil;

  compareVal := 1;
  for i := fCurrentMaxLevel downto 0 do
  begin
    while assigned(priorNode.next[i]) do
    begin
      //avoid a few unnecessary compares ...
      if (compareVal > 0) or (priorNode.next[i+1] <> priorNode.next[i]) then
        compareVal := fCompare(priorNode.next[i].item, item);
      if compareVal > 0 then priorNode := priorNode.next[i]
      else break;
    end;
    fLvls[i] := priorNode;
  end;

  //duplicates are NOT allowed ...
  if (compareVal = 0) then
    raise exception.Create(rsSkipListInsertItem);

  //get the level of the new node ...
  newLevel := 0;
  while (newLevel <= fCurrentMaxLevel) and (newLevel < fMaxLevel-1) and
    (Random < fSkipFrac) do inc(newLevel);
  if newLevel > fCurrentMaxLevel then fCurrentMaxLevel := newLevel;

  //create the new node and rearrange links up to newLevel ...
  result := NewNode(newLevel, item);
  if assigned(priorNode.next[0]) then
    priorNode.next[0].prev := result else
    fBase.prev := result; //fBase.prev always points to the last node
  result.prev := priorNode;
  for i := 0 to newLevel do
  begin
    result.next[i] := fLvls[i].next[i];
    fLvls[i].next[i] := result;
  end;
end;
//------------------------------------------------------------------------------

function TSkipList.DeleteItem(item: pointer): boolean;
var
  i, compareVal: integer;
  priorNode, delNode: PSkipNode;
begin
  for i := 0 to fCurrentMaxLevel do fLvls[i] := fBase;
  priorNode := fBase;

  //find the item ...
  compareVal := 1;
  for i := fCurrentMaxLevel downto 0 do
  begin
    while assigned(priorNode.next[i]) do
    begin
      if (compareVal > 0) or (priorNode.next[i+1] <> priorNode.next[i]) then
      compareVal := fCompare(priorNode.next[i].item, item);
      if compareVal > 0 then priorNode := priorNode.next[i]
      else break;
    end;
    fLvls[i] := priorNode;
  end;
  result := compareVal = 0;
  if not result then exit;

  delNode := priorNode.next[0];
  //if this's the only node at fCurrentMaxLevel, decrement fCurrentMaxLevel ...
  with delNode^ do
    if (level > 0) and (level = fCurrentMaxLevel) and
    (fLvls[level] = fBase) and not assigned(next[level]) then
      dec(fCurrentMaxLevel);

  //fix up links before finally deleting the node ...
  for i := 0 to delNode.level do
    fLvls[i].next[i] := delNode.next[i];
  if assigned(delNode.next[0]) then
    delNode.next[0].prev := delNode.prev else
    fBase.prev := delNode.prev;
  FreeMem(delNode);
  dec(fCount);
end;
//------------------------------------------------------------------------------

procedure TSkipList.Delete(var node: PSkipNode);
var
  i,lvl: integer;
  nextNode, priorNode: PSkipNode;
begin
  //this method doesn't require any knowledge of node.item
  //since it doesn't call fCompare() ...
  for i := node.level+1 to fCurrentMaxLevel do fLvls[i] := nil;
  nextNode := node;
  lvl := node.level;
  while assigned(nextNode.next[nextNode.level]) and
    (nextNode.level < fCurrentMaxLevel) do
  begin
    nextNode := nextNode.next[nextNode.level];
    while nextNode.level > lvl do
    begin
      inc(lvl);
      fLvls[lvl] := nextNode;
    end;
  end;
  for i := 0 to node.level do fLvls[i] := node;

  priorNode := fBase;
  for i := fCurrentMaxLevel downto 0 do
  begin
    while assigned(priorNode.next[i]) and (priorNode.next[i] <> fLvls[i]) do
      priorNode := priorNode.next[i];
    fLvls[i] := priorNode;
  end;

  //if this's the only node at fCurrentMaxLevel, decrement fCurrentMaxLevel ...
  with node^ do
    if (level > 0) and (level = fCurrentMaxLevel) and
    (fLvls[level] = fBase) and not assigned(next[level]) then
      dec(fCurrentMaxLevel);

  //fix up links before finally deleting the node ...
  for i := 0 to node.level do
    fLvls[i].next[i] := node.next[i];
  if assigned(node.next[0]) then
    node.next[0].prev := node.prev else
    fBase.prev := node.prev;
  FreeMem(node);
  dec(fCount);
  node := nil;
end;
//------------------------------------------------------------------------------

function TSkipList.PopFirst: pointer;
var
  i, delLevel: integer;
  delNode: PSkipNode;
begin
  result := nil;
  if fCount = 0 then exit;

  delNode := fBase.next[0];
  result := delNode.item;
  delLevel := delNode.level;
  //if this's the only node at fCurrentMaxLevel, decrement fCurrentMaxLevel ...
  if (delLevel > 0) and (delLevel = fCurrentMaxLevel) and
    (fBase.next[delLevel] = delNode) and not assigned(delNode.next[delLevel]) then
      dec(fCurrentMaxLevel);

  //fix up links before finally deleting the node ...
  for i := 0 to delLevel do fBase.next[i] := delNode.next[i];
  if assigned(delNode.next[0]) then
    delNode.next[0].prev := fBase else fBase.prev := fBase;
  FreeMem(delNode);
  dec(fCount);
end;

//------------------------------------------------------------------------------
// Miscellaneous Functions ...
//------------------------------------------------------------------------------

{$IFNDEF USING_GRAPHICS32}
function FloatPoint(X, Y: Single): TFloatPoint;
begin
  Result.X := X;
  Result.Y := Y;
end;
//------------------------------------------------------------------------------
{$ENDIF}

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

function GetR(const pt1,pt2,pt3: TDoublePoint): double;
var
  N1,N2: TDoublePoint;
begin
  //this function is useful when COMPARING angles as it's a little quicker
  //than getting the specific angles using arctan().
  //Return value are between -2 and +2 where -1.99 is an acute angle turning
  //right, +1.99 is an acute angle turn left and 0 when the points are parallel.
  N1 := GetUnitNormal(pt1, pt2);
  N2 := GetUnitNormal(pt2, pt3);
  if (N1.X * N2.Y - N2.X * N1.Y) < 0 then
    result := 1- (N1.X*N2.X + N1.Y*N2.Y) else
    result := (N1.X*N2.X + N1.Y*N2.Y) -1;
end;
//------------------------------------------------------------------------------

function DistanceSqr(const pt1,pt2: TDoublePoint): double;
begin
  result := (pt1.X - pt2.X)*(pt1.X - pt2.X) + (pt1.Y - pt2.Y)*(pt1.Y - pt2.Y);
end;
//------------------------------------------------------------------------------

function MakeArrayOfDoublePoint(const a: TArrayOfFloatPoint): TArrayOfDoublePoint;
var
  i, len: integer;
begin
  len := length(a);
  setlength(result, len);
  for i := 0 to len -1 do
  begin
    result[i].X := a[i].X;
    result[i].Y := a[i].Y;
  end;
end;
//------------------------------------------------------------------------------

function MakeArrayOfFloatPoint(const a: TArrayOfDoublePoint): TArrayOfFloatPoint;
var
  i, len: integer;
begin
  len := length(a);
  setlength(result, len);
  for i := 0 to len -1 do
  begin
    result[i].X := a[i].X;
    result[i].Y := a[i].Y;
  end;
end;
//------------------------------------------------------------------------------

function IsClockwise(const pts: TArrayOfDoublePoint): boolean; overload;
var
  i, highI: integer;
  area: double;
begin
  result := true;
  highI := high(pts);
  if highI < 2 then exit;
  //or ...(x2-x1)(y2+y1)
  area := pts[highI].x * pts[0].y - pts[0].x * pts[highI].y;
  for i := 0 to highI-1 do
    area := area + pts[i].x * pts[i+1].y - pts[i+1].x * pts[i].y;
  //area := area/2;
  result := area > 0; //ie reverse of normal formula because Y axis inverted
end;
//------------------------------------------------------------------------------

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

function RoundToPrecision(const pt: TDoublePoint): TDoublePoint;
begin
  Result.X := Round(pt.X / precision) * precision;
  Result.Y := Round(pt.Y / precision) * precision;
end;
//------------------------------------------------------------------------------

function PointsEqual(const pt1, pt2: TDoublePoint): boolean; overload;
begin
  result := (abs(pt1.X-pt2.X) < precision + tolerance) and
    (abs(pt1.Y-pt2.Y) < precision + tolerance);
end;
//------------------------------------------------------------------------------

function PointsEqual(const pt1x, pt1y, pt2x, pt2y: double): boolean; overload;
begin
  result := (abs(pt1x-pt2x) < precision + tolerance) and
    (abs(pt1y-pt2y) < precision + tolerance);
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

function IsHorizontal(e: PEdge): boolean; overload;
begin
  result := assigned(e) and (e.dx = horizontal);
end;
//------------------------------------------------------------------------------

function IsHorizontal(const pt1, pt2: TDoublePoint): boolean; overload;
begin
  result := (abs(pt1.X - pt2.X) > precision) and
    (abs(pt1.Y - pt2.Y) < precision);
end;
//----------------------------------------------------------------------

function SlopesEqual(const pt1a, pt1b, pt2a, pt2b: TDoublePoint): boolean; overload;
begin
  result := abs((pt1b.Y - pt1a.Y)*(pt2b.X - pt2a.X) -
    (pt1b.X - pt1a.X)*(pt2b.Y - pt2a.Y)) < slope_precision;
end;
//---------------------------------------------------------------------------

function SlopesEqual(const pt1, pt2, pt3: TDoublePoint): boolean; overload;
begin
  result :=
    ((abs(pt1.X-pt2.X) <= slope_precision) and (abs(pt2.X-pt3.X) <= slope_precision)) or
    ((abs(pt1.Y-pt2.Y) <= slope_precision) and (abs(pt2.Y-pt3.Y) <= slope_precision)) or
    (abs((pt2.Y - pt1.Y)*(pt3.X - pt2.X) - (pt2.X - pt1.X)*(pt3.Y - pt2.Y)) < slope_precision);
end;
//---------------------------------------------------------------------------

function SlopesEqual(e1, e2: PEdge): boolean; overload;
begin
  if IsHorizontal(e1) then result := IsHorizontal(e2)
  else if IsHorizontal(e2) then result := false
  else result := abs((e1.ytop - e1.y)*(e2.xtop - e2.x) -
    (e1.xtop - e1.x)*(e2.ytop - e2.y)) < slope_precision;
end;
//---------------------------------------------------------------------------

function FixupOutPolygon(outPoly: PPolyPt; stripPointyEdgesOnly: boolean = false): PPolyPt;
var
  pp, tmp, lastOK: PPolyPt;
begin
  //FixupOutPolygon() - removes duplicate points and simplifies consecutive
  //parallel edges by removing the middle vertex.
  //stripPointyEdgesOnly: removes the middle vertex only when consecutive
  //parallel edges reflect back on themselves ('pointy' edges). However, it
  //doesn't remove the middle vertex when edges are parallel continuations.
  //Given 3 consecutive vertices - o, *, and o ...
  //the form of 'non-pointy' parallel edges is : o--*----------o
  //the form of 'pointy' parallel edges is     : o--o----------*
  //(While merging polygons that share common edges, it's necessary to
  //temporarily retain 'non-pointy' parallel edges.)
  result := outPoly;
  if not assigned(outPoly) then exit;
  lastOK := nil;
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
      (SlopesEqual(pp.prev.pt, pp.pt, pp.next.pt) and (not stripPointyEdgesOnly or
      ((pp.pt.X - pp.prev.pt.X > 0) <> (pp.next.pt.X - pp.pt.X > 0)) or
      ((pp.pt.Y - pp.prev.pt.Y > 0) <> (pp.next.pt.Y - pp.pt.Y > 0)))) then
    begin
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

function FixupOutPolygon2(outPoly: PPolyPt): PPolyPt;
var
  pp, tmp, lastOK: PPolyPt;
begin
  //FixupOutPolygon2 - just removes duplicate points ...
  result := outPoly;
  if not assigned(outPoly) then exit;
  lastOK := nil;
  pp := outPoly;
  while true do
  begin
    if (pp.prev = pp) or (pp.next = pp.prev) then
    begin
      DisposePolyPts(pp);
      result := nil;
      break;
    end
    else if PointsEqual(pp.pt, pp.next.pt) then
    begin
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

procedure DisposePolyPt(var pp: PPolyPt);
var
  tmpPp: PPolyPt;
begin
  pp.next.prev := pp.prev;
  pp.prev.next := pp.next;
  tmpPp := pp;
  pp := pp.next;
  dispose(tmpPp);
end;
//------------------------------------------------------------------------------

function DuplicatePolyPt(polyPt: PPolyPt): PPolyPt;
begin
  new(result);
  result.pt := polyPt.pt;
  result.flags := polyPt.flags;
  result.dx := unassigned;
  result.prev := polyPt;
  result.next := polyPt.next;
  polyPt.next.prev := result;
  polyPt.next := result;
end;
//------------------------------------------------------------------------------

function PtIsAPolyPt(const pt: TDoublePoint; var polyStartPt: PPolyPt): boolean;
var
  p: PPolyPt;
begin
  result := assigned(polyStartPt);
  if not result then exit;
  p := polyStartPt;
  repeat
    if PointsEqual(pt, polyStartPt.pt) then exit;
    polyStartPt := polyStartPt.next;
  until polyStartPt = p;
  result := false;
end;
//------------------------------------------------------------------------------

procedure SetDx(e: PEdge); overload;
var
  dx, dy: double;
begin
  dx := abs(e.x - e.next.x);
  dy := abs(e.y - e.next.y);
  //Very short, nearly horizontal edges can cause problems by very
  //inaccurately determining intermediate X values - see TopX().
  //Therefore treat very short, nearly horizontal edges as horizontal too ...
  if ((dx < 0.1) and  (dy *10 < dx)) or (dy < slope_precision) then
  begin
    e.dx := horizontal;
    if (e.y <> e.next.y) then e.y := e.next.y;
  end else e.dx :=
    (e.x - e.next.x)/(e.y - e.next.y);
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
  else result := edge.x + edge.dx*(currentY - edge.y);
end;
//------------------------------------------------------------------------------

function ShareSamePoly(e1, e2: PEdge): boolean;
begin
  result := assigned(e1) and assigned(e2) and (e1.outIdx = e2.outIdx);
end;
//------------------------------------------------------------------------------

function IntersectPoint(edge1, edge2: PEdge; out ip: TDoublePoint): boolean;
var
  b1,b2: double;
begin
  if (edge1.dx = edge2.dx) then
  begin
    result := false;
    exit;
  end;
  if edge1.dx = 0 then
  begin
    ip.X := edge1.x;
    with edge2^ do b2 := y - x/dx;
    ip.Y := ip.X/edge2.dx + b2;
  end
  else if edge2.dx = 0 then
  begin
    ip.X := edge2.x;
    with edge1^ do b1 := y - x/dx;
    ip.Y := ip.X/edge1.dx + b1;
  end else
  begin
    with edge1^ do b1 := x - y *dx;
    with edge2^ do b2 := x - y *dx;
    ip.Y := (b2-b1)/(edge1.dx - edge2.dx);
    ip.X := edge1.dx * ip.Y + b1;
  end;
  result := (ip.Y > edge1.ytop +tolerance) and (ip.Y > edge2.ytop +tolerance);
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
    area := area + (pt.pt.X * pt.next.pt.Y) - (pt.next.pt.X * pt.pt.Y);
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
  fList := TList.Create;
  fLocalMinima  := nil;
  fCurrentLM    := nil;
end;
//------------------------------------------------------------------------------

destructor TClipperBase.Destroy;
begin
  Clear;
  fList.Free;
  inherited;
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

  //----------------------------------------------------------------------

  procedure InitEdge(e, eNext, ePrev: PEdge; const pt: TDoublePoint);
  begin
    //set up double-link-list linkage and initialize x, y and dx only
    fillChar(e^, sizeof(TEdge), 0);
    e.x := pt.X;
    e.y := pt.Y;
    e.next := eNext;
    e.prev := ePrev;
    SetDx(e);
  end;
  //----------------------------------------------------------------------

  procedure ReInitEdge(e: PEdge; const nextX, nextY: double);
  begin
    if e.y > nextY then
    begin
      e.xbot := e.x;
      e.ybot := e.y;
      e.xtop := nextX;
      e.ytop := nextY;
      e.nextAtTop := true;
    end else
    begin
      //reverse top and bottom ...
      e.xbot := nextX;
      e.ybot := nextY;
      e.xtop := e.x;
      e.ytop := e.y;
      e.x := e.xbot;
      e.y := e.ybot;
      e.nextAtTop := false;
    end;
    e.polyType := polyType;
    e.outIdx := -1;
  end;
  //----------------------------------------------------------------------

  function SlopesEqualInternal(e1, e2: PEdge): boolean;
  begin
    if IsHorizontal(e1) then result := IsHorizontal(e2)
    else if IsHorizontal(e2) then result := false
    else
      //cross product of dy1/dx1 = dy2/dx2 ...
      result := abs((e1.y - e1.next.y)*(e2.x - e2.next.x) -
        (e1.x - e1.next.x)*(e2.y - e2.next.y)) < slope_precision;
  end;
  //----------------------------------------------------------------------

  procedure FixupForDupsAndColinear(const edges: PEdgeArray);
  var
    lastOK: PEdge;
    e: PEdge;
  begin
    e := @edges[0];
    lastOK := nil;
    while true do
    begin
      if (e.next = e.prev) then break
      else if (PointsEqual(e.prev.x, e.prev.y, e.x, e.y) or
        SlopesEqualInternal(e.prev, e)) then
      begin
        lastOK := nil;
        //remove 'e' from the double-linked-list ...
        if (e = @edges[0]) then
        begin
          //move the content of e.next to e before removing e.next from DLL ...
          e.x := e.next.x;
          e.y := e.next.y;
          e.next.next.prev := e;
          e.next := e.next.next;
        end else
        begin
          //remove 'e' from the loop ...
          e.prev.next := e.next;
          e.next.prev := e.prev;
          e := e.prev; //now get back into the loop
        end;
        SetDx(e.prev);
        SetDx(e);
      end else if lastOK = e then break
      else
      begin
        if not assigned(lastOK) then lastOK := e;
        e := e.next;
      end;
    end;
  end;
  //----------------------------------------------------------------------

  procedure SwapX(e: PEdge);
  begin
    //swap horizontal edges' top and bottom x's so they follow the natural
    //progression of the bounds - ie so their xbots will align with the
    //adjoining lower edge. [Helpful in the ProcessHorizontal() method.]
    e.xbot := e.xtop;
    e.xtop := e.x;
    e.x := e.xbot;
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
      if IsHorizontal(e) then
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
    newLm.nextLm := nil;
    newLm.y := e.prev.ybot;
    if IsHorizontal(e) then //horizontal edges never start a left bound
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
      if (e.next.ytop = e.ytop) and not IsHorizontal(e.next) then break;
      e.nextInLML := e.next;
      e := e.next;
      if IsHorizontal(e) and (e.xbot <> e.prev.xtop) then SwapX(e);
    until false;
    result := e.next;
  end;
  //----------------------------------------------------------------------

var
  i, highI: integer;
  edges: PEdgeArray;
  e, eHighest: PEdge;
  pg: TArrayOfDoublePoint;
begin
  {AddPolygon}

  highI := high(polygon);
  setlength(pg, highI +1);
  for i := 0 to highI do pg[i] := RoundToPrecision(polygon[i]);

  while (highI > 1) and PointsEqual(pg[0], pg[highI]) do dec(highI);
  if highI < 2 then exit;

  //make sure this is still a sensible polygon (ie with at least one minima) ...
  i := 1;
  while (i <= highI) and (abs(pg[i].Y - pg[0].Y) < precision) do inc(i);
  if i > highI then exit;

  GetMem(edges, sizeof(TEdge)*(highI+1));
  //convert 'edges' to a double-linked-list and initialize a few of the vars ...
  edges[0].x := pg[0].X;
  edges[0].y := pg[0].Y;
  InitEdge(@edges[highI], @edges[0], @edges[highI-1], pg[highI]);
  for i := highI-1 downto 1 do
    InitEdge(@edges[i], @edges[i+1], @edges[i-1], pg[i]);
  InitEdge(@edges[0], @edges[1], @edges[highI], pg[0]);
  e := @edges[highI];
  while IsHorizontal(e) and (e <> @edges[0]) do
  begin
    if (e.y <> e.next.y) then e.y := e.next.y;
    e := e.prev;
  end;

  //fixup by deleting duplicate points and merging co-linear edges ...
  FixupForDupsAndColinear(edges);

  e := @edges[0];
  if (e.next = e.prev) then
  begin
    //this isn't a valid polygon ...
    dispose(edges);
    exit;
  end;

  fList.Add(edges);

  //now properly re-initialize edges and also find 'eHighest' ...
  e := edges[0].next;
  eHighest := e;
  repeat
    ReInitEdge(e, e.next.x, e.next.y);
    if e.ytop < eHighest.ytop then eHighest := e;
    e := e.next;
  until e = @edges[0];
  if e.next.nextAtTop then
    ReInitEdge(e, e.next.x, e.next.y) else
    ReInitEdge(e, e.next.xtop, e.next.ytop);
  if e.ytop < eHighest.ytop then eHighest := e;

  //make sure eHighest is positioned so the following loop works safely ...
  if eHighest.nextAtTop then eHighest := eHighest.next;
  if IsHorizontal(eHighest) then eHighest := eHighest.next;
  //eHighest is the edge touching the top of the polygon but also
  //just starting the descent ...

  //finally insert each local minima ...
  e := eHighest;
  repeat
    e := AddBoundsToLML(e);
  until (e = eHighest);
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

function TClipperBase.GetBounds: TFloatRect;
var
  e: PEdge;
  lm: PLocalMinima;
begin
  lm := fLocalMinima;
  if not assigned(lm) then
  begin
    result := nullRect;
    exit;
  end;
  result.Left := unassigned;
  result.Top := unassigned;
  result.Right := -unassigned;
  result.Bottom := -unassigned;
  while assigned(lm) do
  begin
    if lm.leftBound.y > result.Bottom then result.Bottom := lm.leftBound.y;
    e := lm.leftBound;
    while assigned(e.nextInLML) do
    begin
      if e.x < result.Left then result.Left := e.x;
      e := e.nextInLML;
    end;
    if e.x < result.Left then result.Left := e.x
    else if e.xtop < result.Left then result.Left := e.xtop;
    if e.ytop < result.Top then result.Top := e.ytop;

    e := lm.rightBound;
    while assigned(e.nextInLML) do
    begin
      if e.x > result.Right then result.Right := e.x;
      e := e.nextInLML;
    end;
    if e.x > result.Right then result.Right := e.x;
    if e.xtop > result.Right then result.Right := e.xtop;

    lm := lm.nextLm;
  end;
end;
//------------------------------------------------------------------------------

function TClipperBase.Reset: boolean;
var
  e: PEdge;
  lm: PLocalMinima;
begin
  //Reset() allows various clipping operations to be executed
  //multiple times on the same polygon sets. (Protected method.)
  fCurrentLM := fLocalMinima;
  result := assigned(fLocalMinima);
  if not result then exit; //ie nothing to process

  //reset all edges ...
  lm := fLocalMinima;
  while assigned(lm) do
  begin
    e := lm.leftBound;
    while assigned(e) do
    begin
      e.xbot := e.x;
      e.ybot := e.y;
      e.side := esLeft;
      e.outIdx := -1;
      e := e.nextInLML;
    end;
    e := lm.rightBound;
    while assigned(e) do
    begin
      e.xbot := e.x;
      e.ybot := e.y;
      e.side := esRight;
      e.outIdx := -1;
      e := e.nextInLML;
    end;
    lm := lm.nextLm;
  end;
end;
//------------------------------------------------------------------------------

procedure TClipperBase.PopLocalMinima;
begin
  if not assigned(fCurrentLM) then exit;
  fCurrentLM := fCurrentLM.nextLm;
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
  fCurrentLM := nil;
end;

//------------------------------------------------------------------------------
// TClipper methods ...
//------------------------------------------------------------------------------

constructor TClipper.Create;
begin
  inherited Create;
  fPolyPtList := TList.Create;
  fIgnoreOrientation := false;
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
  solution := nil;
  if fExecuteLocked then exit;
  try try
    fExecuteLocked := true;
    fSubjFillType := subjFillType;
    fClipFillType := clipFillType;
    result := ExecuteInternal(clipType);
    if result then solution := ResultAsFloatPointArray;
  except
    result := false;
  end;
  finally
    fExecuteLocked := false;
    DisposeAllPolyPts;
  end;
end;
//------------------------------------------------------------------------------

function TClipper.Execute(clipType: TClipType;
  out solution: TArrayOfArrayOfDoublePoint;
  subjFillType: TPolyFillType = pftEvenOdd;
  clipFillType: TPolyFillType = pftEvenOdd): boolean;
begin
  result := false;
  solution := nil;
  if fExecuteLocked then exit;
  try try
    fExecuteLocked := true;
    fSubjFillType := subjFillType;
    fClipFillType := clipFillType;
    result := ExecuteInternal(clipType);
    if result then solution := ResultAsDoublePointArray;
  except
    result := false;
  end;
  finally
    fExecuteLocked := false;
    DisposeAllPolyPts;
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
    fJoins := nil;
    fCurrentHorizontals := nil;
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
begin
  k := 0;
  MergePolysWithCommonEdges;
  for i := 0 to fPolyPtList.Count -1 do
    if assigned(fPolyPtList[i]) then
      fPolyPtList[i] := FixupOutPolygon(fPolyPtList[i]);

  if not fIgnoreOrientation then FixOrientation;

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
      if (cnt < 3) then continue;

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
  p: PPolyPt;
begin
  MergePolysWithCommonEdges;
  for i := 0 to fPolyPtList.Count -1 do
    if assigned(fPolyPtList[i]) then
      fPolyPtList[i] := FixupOutPolygon(fPolyPtList[i]);

  if not fIgnoreOrientation then FixOrientation;

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

      //nb: for those who might want to modify the TPolyPolygon struct to
      //include orientation, p.flags will have the ofClockwise bit set when
      //it's an outer polygon.

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
  result := Reset; //returns false when there are no polygons to process
  if not result then exit;
  //add all the local minima into a fresh fScanbeam list ...
  lm := CurrentLM;
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
  else if edge.nextAtTop then edge.windDelta := 1
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
  case edge.polyType of
    ptSubject: result := fSubjFillType = pftNonZero;
    else result := fClipFillType = pftNonZero;
  end;
end;
//------------------------------------------------------------------------------

function TClipper.IsNonZeroAltFillType(edge: PEdge): boolean;
begin
  case edge.polyType of
    ptSubject: result := fClipFillType = pftNonZero;
    else result := fSubjFillType = pftNonZero;
  end;
end;
//------------------------------------------------------------------------------

function HorizOverlap(const h1a, h1b, h2a, h2b: double): boolean;
var
  min2, max2: double;
begin
  //returns true if (h1a between h2a and h2b) or
  //  (h1a == min2 and h1b > min2) or (h1a == max2 and h1b < max2)
  if h2a < h2b then
  begin
    min2 := h2a; max2 := h2b;
  end else
  begin
    min2 := h2b; max2 := h2a;
  end;
  result := ((h1a > min2 + tolerance) and (h1a < max2 - tolerance)) or
    ((abs(h1a - min2) < tolerance) and (h1b > min2 + tolerance)) or
    ((abs(h1a - max2) < tolerance) and (h1b < max2 - tolerance));
end;
//------------------------------------------------------------------------------

function Edge2InsertsBeforeEdge1(e1,e2: PEdge): boolean;
begin
  if (e2.xbot - tolerance > e1.xbot) then result := false
  else if (e2.xbot + tolerance < e1.xbot) then result := true
  else if IsHorizontal(e2) then result := false
  else result := e2.dx > e1.dx;
end;
//------------------------------------------------------------------------------

procedure TClipper.InsertLocalMinimaIntoAEL(const botY: double);

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
  i,j, hIdx: integer;
  e: PEdge;
  pt, hPt, hPt2: TDoublePoint;
  lb, rb: PEdge;
  p, p2: PPolyPt;
begin
  {InsertLocalMinimaIntoAEL}
  while assigned(CurrentLM) and (CurrentLM.y = botY) do
  begin
    InsertEdgeIntoAEL(CurrentLM.leftBound);
    InsertScanbeam(CurrentLM.leftBound.ytop);
    InsertEdgeIntoAEL(CurrentLM.rightBound);

    lb := CurrentLM.leftBound;
    rb := CurrentLM.rightBound;

    //set edge winding states ...
    SetWindingDelta(lb);
    if IsNonZeroFillType(lb) then
      rb.windDelta := -lb.windDelta else
      rb.windDelta := 1;
    SetWindingCount(lb);
    rb.windCnt := lb.windCnt;
    rb.windCnt2 := lb.windCnt2;

    if IsHorizontal(rb) then
    begin
      //nb: only rbs can have a horizontal bottom edge
      AddEdgeToSEL(rb);
      InsertScanbeam(rb.nextInLML.ytop);
    end else
      InsertScanbeam(rb.ytop);

    if IsContributing(lb) then
      AddLocalMinPoly(lb, rb, DoublePoint(lb.xbot, CurrentLM.y));

    //flag polygons that share colinear edges, so they can be merged later ...
    if (lb.outIdx >= 0) and assigned(lb.prevInAEL) and
      (lb.prevInAEL.outIdx >= 0) and
      (abs(lb.prevInAEL.xbot - lb.x) < tolerance) and
      SlopesEqual(lb, lb.prevInAEL) then
    begin
      pt := DoublePoint(lb.x,lb.y);
      AddPolyPt(lb.prevInAEL, pt);
      i := length(fJoins);
      setlength(fJoins, i+1);
      fJoins[i].idx1 := lb.outIdx;
      fJoins[i].idx2 := lb.prevInAEL.outIdx;
      fJoins[i].pt := pt;
    end;
    if (rb.outIdx >= 0) and IsHorizontal(rb) then
    begin
      //check for overlap with fCurrentHorizontals
      for i := 0 to high(fCurrentHorizontals) do
      begin
        hIdx := fCurrentHorizontals[i].idx1;
        hPt := fCurrentHorizontals[i].outPPt.pt;
        hPt2 := fCurrentHorizontals[i].pt;
        p := fCurrentHorizontals[i].outPPt;
        if IsHorizontal(p.pt, p.prev.pt) and (p.prev.pt.X = hPt2.X) then p2 := p.prev
        else if IsHorizontal(p.pt, p.next.pt) and (p.next.pt.X = hPt2.X) then p2 := p.next
        else continue;

        if HorizOverlap(hPt.X, hPt2.X, rb.x, rb.xtop) then
        begin
          AddPolyPt(rb, hPt);
          j := length(fJoins);
          setlength(fJoins, j+1);
          fJoins[j].idx1 := hIdx;
          fJoins[j].idx2 := rb.outIdx;
          fJoins[j].pt := hPt;
        end
        else if HorizOverlap(rb.x, rb.xtop, hPt.X, hPt2.X) then
        begin
          pt := DoublePoint(rb.x,rb.y);
          j := length(fJoins);
          setlength(fJoins, j+1);
          if not PointsEqual(pt, p.pt) and not PointsEqual(pt, p2.pt) then
            InsertPolyPtBetween(pt, p, p2);
          fJoins[j].idx1 := hIdx;
          fJoins[j].idx2 := rb.outIdx;
          fJoins[j].pt := pt;
        end;
      end;
    end;

    if (lb.nextInAEL <> rb) then
    begin
      e := lb.nextInAEL;
      pt := DoublePoint(lb.xbot,lb.ybot);
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
  fCurrentHorizontals := nil;
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

procedure TClipper.SwapPositionsInSEL(edge1, edge2: PEdge);
var
  prev,next: PEdge;
begin
  if edge1.nextInSEL = edge2 then
  begin
    next    := edge2.nextInSEL;
    if assigned(next) then next.prevInSEL := edge1;
    prev    := edge1.prevInSEL;
    if assigned(prev) then prev.nextInSEL := edge2;
    edge2.prevInSEL := prev;
    edge2.nextInSEL := edge1;
    edge1.prevInSEL := edge2;
    edge1.nextInSEL := next;
  end
  else if edge2.nextInSEL = edge1 then
  begin
    next    := edge1.nextInSEL;
    if assigned(next) then next.prevInSEL := edge2;
    prev    := edge2.prevInSEL;
    if assigned(prev) then prev.nextInSEL := edge1;
    edge1.prevInSEL := prev;
    edge1.nextInSEL := edge2;
    edge2.prevInSEL := edge1;
    edge2.nextInSEL := next;
  end else
  begin
    next    := edge1.nextInSEL;
    prev    := edge1.prevInSEL;
    edge1.nextInSEL := edge2.nextInSEL;
    if assigned(edge1.nextInSEL) then edge1.nextInSEL.prevInSEL := edge1;
    edge1.prevInSEL := edge2.prevInSEL;
    if assigned(edge1.prevInSEL) then edge1.prevInSEL.nextInSEL := edge1;
    edge2.nextInSEL := next;
    if assigned(edge2.nextInSEL) then edge2.nextInSEL.prevInSEL := edge2;
    edge2.prevInSEL := prev;
    if assigned(edge2.prevInSEL) then edge2.prevInSEL.nextInSEL := edge2;
  end;
  if not assigned(edge1.prevInSEL) then fSortedEdges := edge1
  else if not assigned(edge2.prevInSEL) then fSortedEdges := edge2;
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
  result := assigned(e) and
    (abs(e.ytop - Y) < tolerance) and not assigned(e.nextInLML);
end;
//------------------------------------------------------------------------------

function IsIntermediate(e: PEdge; const Y: double): boolean;
begin
  result := (abs(e.ytop - Y) < tolerance) and assigned(e.nextInLML);
end;
//------------------------------------------------------------------------------

function TClipper.GetMaximaPair(e: PEdge): PEdge;
begin
  result := e.next;
  if not IsMaxima(result, e.ytop) or (result.xtop <> e.xtop) then
    result := e.prev;
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
    if not assigned(eNext) then raise exception.Create(rsDoMaxima);
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

function TClipper.IsTopHorz(const XPos: double): boolean;
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
  i: integer;
  e, eNext, eMaxPair: PEdge;
  horzLeft, horzRight: double;
  Direction: TDirection;
  pt: TDoublePoint;
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
    if (e.xbot >= horzLeft -tolerance) and (e.xbot <= horzRight +tolerance) then
    begin
      //ok, so far it looks like we're still in range of the horizontal edge

      if (abs(e.xbot - horzEdge.xtop) < tolerance) and
        assigned(horzEdge.nextInLML) then
      begin
        if SlopesEqual(e, horzEdge.nextInLML) then
        begin
          //we've got 2 colinear edges at the end of the horz. line ...
          if (horzEdge.outIdx >= 0) and (e.outIdx >= 0) then
          begin
            i := length(fJoins);
            setlength(fJoins, i+1);
            pt := DoublePoint(horzEdge.xtop,horzEdge.ytop);
            AddPolyPt(horzEdge, pt);
            fJoins[i].idx1 := horzEdge.outIdx;
            AddPolyPt(e, pt);
            fJoins[i].idx2 := e.outIdx;
            fJoins[i].pt := pt;
          end;
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
          IntersectEdges(horzEdge, e, DoublePoint(e.xbot, horzEdge.ybot)) else
          IntersectEdges(e, horzEdge, DoublePoint(e.xbot, horzEdge.ybot));
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
            ProtectRight[not IsTopHorz(e.xbot)])
        else
          IntersectEdges(e, horzEdge, DoublePoint(e.xbot, horzEdge.ybot),
            ProtectLeft[not IsTopHorz(e.xbot)]);
      end
      else if (Direction = dLeftToRight) then
      begin
        IntersectEdges(horzEdge, e, DoublePoint(e.xbot, horzEdge.ybot),
          ProtectRight[not IsTopHorz(e.xbot)])
      end else
      begin
        IntersectEdges(e, horzEdge, DoublePoint(e.xbot, horzEdge.ybot),
          ProtectLeft[not IsTopHorz(e.xbot)]);
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
      AddPolyPt(horzEdge, DoublePoint(horzEdge.xtop, horzEdge.ytop));
    UpdateEdgeIntoAEL(horzEdge);
  end else
  begin
    if horzEdge.outIdx >= 0 then
      IntersectEdges(horzEdge, eMaxPair,
        DoublePoint(horzEdge.xtop, horzEdge.ybot), [ipLeft,ipRight]);
    if eMaxPair.outIdx >= 0 then raise exception.Create(rsHorizontal);
    DeleteFromAEL(eMaxPair);
    DeleteFromAEL(horzEdge);
  end;
end;
//------------------------------------------------------------------------------

function TClipper.InsertPolyPtBetween(const pt: TDoublePoint; pp1, pp2: PPolyPt): PPolyPt;
begin
  new(result);
  result.pt := pt;
  if pp2 = pp1.next then
  begin
    result.next := pp2;
    result.prev := pp1;
    pp1.next := result;
    pp2.prev := result;
  end else if pp1 = pp2.next then
  begin
    result.next := pp1;
    result.prev := pp2;
    pp2.next := result;
    pp1.prev := result;
  end else
    raise exception.Create(rsInsertPolyPt);
end;
//------------------------------------------------------------------------------

function TClipper.AddPolyPt(e: PEdge; const pt: TDoublePoint): PPolyPt;
var
  fp: PPolyPt;
  ToFront: boolean;
begin
  ToFront := e.side = esLeft;
  if e.outIdx < 0 then
  begin
    new(result);
    result.pt := pt;
    e.outIdx := fPolyPtList.Add(result);
    result.next := result;
    result.prev := result;
    result.flags := [];
    result.dx := unassigned;
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
    result.flags := [];
    result.dx := unassigned;
    fp.prev := result;
    if ToFront then fPolyPtList[e.outIdx] := result;
  end;
end;
//------------------------------------------------------------------------------

procedure TClipper.ProcessIntersections(const topY: double);
begin
  if not assigned(fActiveEdges) then exit;
  try
    fIntersectTolerance := tolerance;
    BuildIntersectList(topY);
    if not assigned(fIntersectNodes) then exit;
    //Test the pending intersections for errors and, if any are found, redo
    //BuildIntersectList (twice if necessary) with adjusted tolerances ...
    if not TestIntersections then
    begin
      fIntersectTolerance := minimal_tolerance;
      DisposeIntersectNodes;
      BuildIntersectList(topY);
      if not TestIntersections then
      begin
        fIntersectTolerance := slope_precision;
        DisposeIntersectNodes;
        BuildIntersectList(topY);
        if not TestIntersections then
          //try eliminating near duplicate points in the input polygons
          //eg by adjusting precision ... to say 0.1;
          raise Exception.Create(rsIntersection);
      end;                                  
    end;
    ProcessIntersectList;
  finally
    //if there's been an error, clean up the mess ...
    DisposeIntersectNodes;
  end;
end;
//------------------------------------------------------------------------------

procedure TClipper.DisposeIntersectNodes;
var
  iNode: PIntersectNode;
begin
  while assigned(fIntersectNodes) do
  begin
    iNode := fIntersectNodes.next;
    dispose(fIntersectNodes);
    fIntersectNodes := iNode;
  end;
end;
//------------------------------------------------------------------------------

function TClipper.Process1Before2(Node1, Node2: PIntersectNode): boolean;

  function E1PrecedesE2inAEL(e1, e2: PEdge): boolean;
  begin
    result := true;
    while assigned(e1) do
      if e1 = e2 then exit else e1 := e1.nextInAEL;
    result := false;
  end;

begin
  if (abs(Node1.pt.Y - Node2.pt.Y) < fIntersectTolerance) then
  begin
    if (abs(Node1.pt.X - Node2.pt.X) > precision) then
    begin
      result := Node1.pt.X < Node2.pt.X;
      exit;
    end;
    //a complex intersection (with more than 2 edges intersecting) ...
    if (Node1.edge1 = Node2.edge1) or
      SlopesEqual(Node1.edge1, Node2.edge1) then
    begin
      if Node1.edge2 = Node2.edge2 then
        //(N1.E1 & N2.E1 are co-linear) and (N1.E2 == N2.E2)  ...
        result := not E1PrecedesE2inAEL(Node1.edge1, Node2.edge1)
      else if SlopesEqual(Node1.edge2, Node2.edge2) then
        //(N1.E1 == N2.E1) and (N1.E2 & N2.E2 are co-linear) ...
        result := E1PrecedesE2inAEL(Node1.edge2, Node2.edge2)
      else if //check if minima **
        ((abs(Node1.edge2.y - Node1.pt.Y) < slope_precision) or
        (abs(Node2.edge2.y - Node2.pt.Y) < slope_precision)) and
        ((Node1.edge2.next = Node2.edge2) or (Node1.edge2.prev = Node2.edge2)) then
      begin
        if Node1.edge1.dx < 0 then
          result := Node1.edge2.dx > Node2.edge2.dx else
          result := Node1.edge2.dx < Node2.edge2.dx;
      end else if (Node1.edge2.dx - Node2.edge2.dx) < precision then
        result := E1PrecedesE2inAEL(Node1.edge2, Node2.edge2)
      else
        result := (Node1.edge2.dx < Node2.edge2.dx);

    end else if (Node1.edge2 = Node2.edge2) and //check if maxima ***
      ((abs(Node1.edge1.ytop - Node1.pt.Y) < slope_precision) or
      (abs(Node2.edge1.ytop - Node2.pt.Y) < slope_precision)) then
        result := (Node1.edge1.dx > Node2.edge1.dx)
    else
      result := (Node1.edge1.dx < Node2.edge1.dx);
  end else
    result := (Node1.pt.Y > Node2.pt.Y);
  //**a minima that very slightly overlaps an edge can appear like
  //a complex intersection but it's not. (Minima can't have parallel edges.)
  //***a maxima that very slightly overlaps an edge can appear like
  //a complex intersection but it's not. (Maxima can't have parallel edges.)
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
  else if Process1Before2(IntersectNode, fIntersectNodes) then
  begin
    IntersectNode.next := fIntersectNodes;
    fIntersectNodes.prev := IntersectNode;
    fIntersectNodes := IntersectNode;
  end else
  begin
    iNode := fIntersectNodes;
    while assigned(iNode.next) and
      Process1Before2(iNode.next, IntersectNode) do
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
  isModified: boolean;
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
    isModified := true;
    while isModified and assigned(fSortedEdges) do
    begin
      isModified := false;
      e := fSortedEdges;
      while assigned(e.nextInSEL) do
      begin
        eNext := e.nextInSEL;
        if (e.tmpX > eNext.tmpX + tolerance) and
          IntersectPoint(e, eNext, pt) then
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

function TClipper.TestIntersections: boolean;
var
  e: PEdge;
  iNode: PIntersectNode;
begin
  result := true;
  if not assigned(fIntersectNodes) then exit;
  try
    //do the test sort using SEL ...
    CopyAELToSEL;
    iNode := fIntersectNodes;
    while assigned(iNode) do
    begin
      SwapPositionsInSEL(iNode.edge1, iNode.edge2);
      iNode := iNode.next;
    end;
    //now check that tmpXs are in the right order ...
    e := fSortedEdges;
    while assigned(e.nextInSEL) do
    begin
      if e.nextInSEL.tmpX < e.tmpX - precision then
      begin
        result := false;
        exit;
      end;
      e := e.nextInSEL;
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
      IntersectEdges(edge1, edge2, pt, [ipLeft,ipRight]);
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
    (abs(e1.xtop - pt.x) < tolerance) and //nb: not precision
    (abs(e1.ytop - pt.y) < tolerance);
  e2stops := not (ipRight in protects) and not assigned(e2.nextInLML) and
    (abs(e2.xtop - pt.x) < tolerance) and //nb: not precision
    (abs(e2.ytop - pt.y) < tolerance);
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
  i: integer;
  pt: TDoublePoint;
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
  if not IsHorizontal(e) then
  begin
    InsertScanbeam(e.ytop);

    //if output polygons share an edge, they'll need joining later ...
    if (e.outIdx >= 0) and assigned(AelPrev) and (AelPrev.outIdx >= 0) and
      (abs(AelPrev.xbot - e.x) < tolerance) and SlopesEqual(e, AelPrev) then
    begin
      i := length(fJoins);
      setlength(fJoins, i+1);
      pt := DoublePoint(e.x,e.y);
      AddPolyPt(AelPrev, pt);
      fJoins[i].idx1 := AelPrev.outIdx;
      AddPolyPt(e, pt);
      fJoins[i].idx2 := e.outIdx;
      fJoins[i].pt := pt;
    end;
  end;
end;
//------------------------------------------------------------------------------

function TClipper.BubbleSwap(edge: PEdge): PEdge;
var
  i, n: integer;
  e: PEdge;
begin
  n := 1; //no. edges intersecting at the same point.
  result := edge.nextInAEL;
  while assigned(result) and (abs(result.xbot - edge.xbot) <= tolerance) do
  begin
    inc(n);
    result := Result.nextInAEL;
  end;
  //if more than 2 edges intersect at a given point then there are multiple
  //intersections at this point between all the edges. Therefore ...
  //let n = no. edges intersecting at a given point.
  //given f(n) = no. intersections between n edges at a given point, & f(0) = 0
  //then f(n) = f(n-1) + n-1;
  //therefore 1 edge -> 0 intersections; 2 -> 1; 3 -> 3; 4 -> 6; 5 -> 10 etc.
  //nb: coincident edges will cause unexpected f(n) values.
  if n > 2 then
  begin
    try
      //create the sort list ...
      fSortedEdges := edge;
      edge.prevInSEL := nil;
      e := edge.nextInAEL;
      for i := 2 to n do
      begin
        e.prevInSEL := e.prevInAEL;
        e.prevInSEL.nextInSEL := e;
        if i = n then e.nextInSEL := nil;
        e := e.nextInAEL;
      end;

      //fSortedEdges now contains the sort list. Bubble sort this list,
      //processing intersections and dropping the last edge on each pass
      //until the list contains fewer than two edges.
      while assigned(fSortedEdges) and
        assigned(fSortedEdges.nextInSEL) do
      begin
        e := fSortedEdges;
        while assigned(e.nextInSEL) do
        begin
          if (e.nextInSEL.dx > e.dx) then
          begin
            IntersectEdges(e, e.nextInSEL, //param order is important here
              DoublePoint(e.xbot,e.ybot), [ipLeft,ipRight]);
            SwapPositionsInAEL(e, e.nextInSEL);
            SwapPositionsInSEL(e, e.nextInSEL);
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
  i: integer;
  e, ePrior: PEdge;
  pp: PPolyPt;
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
        begin
          pp := AddPolyPt(e, DoublePoint(e.xtop, e.ytop));

          //add the polyPt to a list that later checks for overlaps with
          //contributing horizontal minima since they'll need joining.
          i := length(fCurrentHorizontals);
          setlength(fCurrentHorizontals, i+1);
          fCurrentHorizontals[i].idx1 := e.outIdx;
          with e.nextInLML^ do
            fCurrentHorizontals[i].pt := DoublePoint(xtop,ytop);
          fCurrentHorizontals[i].outPPt := pp;
        end;
        //very rarely an edge just below a horizontal edge in a contour
        //intersects with another edge at the very top of a scanbeam.
        //If this happens that intersection must be managed first ...
        if assigned(e.prevInAEL) and (e.prevInAEL.xbot > e.xtop + tolerance) then
        begin
          IntersectEdges(e.prevInAEL, e,
            DoublePoint(e.prevInAEL.xbot,e.prevInAEL.ybot), [ipLeft,ipRight]);
          SwapPositionsInAEL(e.prevInAEL, e);
          UpdateEdgeIntoAEL(e);
          AddEdgeToSEL(e);
          e := e.nextInAEL;
          UpdateEdgeIntoAEL(e);
          AddEdgeToSEL(e);
        end
        else if assigned(e.nextInAEL) and
          (e.xtop > topX(e.nextInAEL, topY) + tolerance) then
        begin
          e.nextInAEL.xbot := TopX(e.nextInAEL, topY);
          e.nextInAEL.ybot := topY;
          IntersectEdges(e, e.nextInAEL,
            DoublePoint(e.nextInAEL.xbot,e.nextInAEL.ybot), [ipLeft,ipRight]);
          SwapPositionsInAEL(e, e.nextInAEL);
          UpdateEdgeIntoAEL(e);
          AddEdgeToSEL(e);
        end else
        begin
          UpdateEdgeIntoAEL(e);
          AddEdgeToSEL(e);
        end;
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
      if (e.outIdx >= 0) then AddPolyPt(e, DoublePoint(e.xtop, e.ytop));
      UpdateEdgeIntoAEL(e);
    end;
    e := e.nextInAEL;
  end;

  //5. Process (non-horizontal) intersections at the top of the scanbeam ...
  e := fActiveEdges;
  if assigned(e) and (e.nextInAEL = nil) then
    raise Exception.Create(rsProcessEdgesAtTopOfScanbeam);
  while assigned(e) do
  begin
    if not assigned(e.nextInAEL) then break;
    if e.nextInAEL.xbot < e.xbot - precision then
      raise Exception.Create(rsProcessEdgesAtTopOfScanbeam);
    if e.nextInAEL.xbot > e.xbot + tolerance then
      e := e.nextInAEL else
      e := BubbleSwap(e);
  end;
end;
//------------------------------------------------------------------------------

procedure TClipper.AddLocalMaxPoly(e1, e2: PEdge; const pt: TDoublePoint);
begin
  AddPolyPt(e1, pt);
  if ShareSamePoly(e1, e2) then
  begin
    e1.outIdx := -1;
    e2.outIdx := -1;
  end else
    AppendPolygon(e1, e2);
end;
//------------------------------------------------------------------------------

procedure TClipper.AddLocalMinPoly(e1, e2: PEdge; const pt: TDoublePoint);
begin
  AddPolyPt(e1, pt);
  e2.outIdx := e1.outIdx;

  if IsHorizontal(e2) or (e1.dx > e2.dx) then
  begin
    e1.side := esLeft;
    e2.side := esRight;
  end else
  begin
    e1.side := esRight;
    e2.side := esLeft;
  end;
end;
//------------------------------------------------------------------------------

procedure TClipper.AppendPolygon(e1, e2: PEdge);
var
  p1_lft, p1_rt, p2_lft, p2_rt: PPolyPt;
  side: TEdgeSide;
  e: PEdge;
  i, OKIdx, ObsoleteIdx: integer;
begin
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

  OKIdx := e1.outIdx;
  ObsoleteIdx := e2.outIdx;
  fPolyPtList[ObsoleteIdx] := nil;

  for i := 0 to high(fJoins) do
  begin
    if fJoins[i].idx1 = ObsoleteIdx then fJoins[i].idx1 := OKIdx;
    if fJoins[i].idx2 = ObsoleteIdx then fJoins[i].idx2 := OKIdx;
  end;
  for i := 0 to high(fCurrentHorizontals) do
    if fCurrentHorizontals[i].idx1 = ObsoleteIdx then
      fCurrentHorizontals[i].idx1 := OKIdx;

  e1.outIdx := -1; //nb: safe because we only get here via AddLocalMaxPoly
  e2.outIdx := -1;

  e := fActiveEdges;
  while assigned(e) do
  begin
    if (e.outIdx = ObsoleteIdx) then
    begin
      e.outIdx := OKIdx;
      e.side := side;
      break;
    end;
    e := e.nextInAEL;
  end;

end;
//------------------------------------------------------------------------------

procedure TClipper.MergePolysWithCommonEdges;
var
  i, j, newIdx: integer;
  p1, p2, pp1, pp2: PPolyPt;
begin
  for i := 0 to high(fJoins) do
  begin
    if fJoins[i].idx1 = fJoins[i].idx2 then
    begin
      p1 := fPolyPtList[fJoins[i].idx1];
      p1 := FixupOutPolygon(p1, true);
      fPolyPtList[fJoins[i].idx1] := p1;
      if not PtIsAPolyPt(fJoins[i].pt, p1) then continue;
      p2 := p1.next; //ie we don't want the same point as p1
      if not PtIsAPolyPt(fJoins[i].pt, p2) or (p2 = p1) then continue;
    end else
    begin
      p1 := fPolyPtList[fJoins[i].idx1];
      p1 := FixupOutPolygon(p1, true);
      fPolyPtList[fJoins[i].idx1] := p1;
      //check that fJoins[i].pt is in the polygon and also update p1 so
      //that p1.pt == fJoins[i].pt ...
      if not PtIsAPolyPt(fJoins[i].pt, p1) then continue;

      p2 := fPolyPtList[fJoins[i].idx2];
      p2 := FixupOutPolygon(p2, true);
      fPolyPtList[fJoins[i].idx2] := p2;
      if not PtIsAPolyPt(fJoins[i].pt, p2) then continue;
    end;

    if (((p1.next.pt.X > p1.pt.X) and (p2.next.pt.X > p2.pt.X)) or
      ((p1.next.pt.X < p1.pt.X) and (p2.next.pt.X < p2.pt.X)) or
      ((p1.next.pt.Y > p1.pt.Y) and (p2.next.pt.Y > p2.pt.Y)) or
      ((p1.next.pt.Y < p1.pt.Y) and (p2.next.pt.Y < p2.pt.Y))) and
      SlopesEqual(p1.pt, p1.next.pt, p2.pt, p2.next.pt) then
    begin
      if fJoins[i].idx1 = fJoins[i].idx2 then continue;
      pp1 := DuplicatePolyPt(p1);
      pp2 := DuplicatePolyPt(p2);
      ReversePolyPtLinks(p2);
      pp1.prev := pp2;
      pp2.next := pp1;
      p1.next := p2;
      p2.prev := p1;
    end
    else if (((p1.next.pt.X > p1.pt.X) and (p2.prev.pt.X > p2.pt.X)) or
      ((p1.next.pt.X < p1.pt.X) and (p2.prev.pt.X < p2.pt.X)) or
      ((p1.next.pt.Y > p1.pt.Y) and (p2.prev.pt.Y > p2.pt.Y)) or
      ((p1.next.pt.Y < p1.pt.Y) and (p2.prev.pt.Y < p2.pt.Y))) and
      SlopesEqual(p1.pt, p1.next.pt, p2.pt, p2.prev.pt) then
    begin
      pp1 := DuplicatePolyPt(p1);
      pp2 := DuplicatePolyPt(p2);
      p1.next := pp2;
      pp2.prev := p1;
      p2.next := pp1;
      pp1.prev := p2;
    end
    else if (((p1.prev.pt.X > p1.pt.X) and (p2.next.pt.X > p2.pt.X)) or
      ((p1.prev.pt.X < p1.pt.X) and (p2.next.pt.X < p2.pt.X)) or
      ((p1.prev.pt.Y > p1.pt.Y) and (p2.next.pt.Y > p2.pt.Y)) or
      ((p1.prev.pt.Y < p1.pt.Y) and (p2.next.pt.Y < p2.pt.Y))) and
      SlopesEqual(p1.pt, p1.prev.pt, p2.pt, p2.next.pt) then
    begin
      pp1 := DuplicatePolyPt(p1);
      pp2 := DuplicatePolyPt(p2);
      p1.next := pp2;
      pp2.prev := p1;
      pp1.prev := p2;
      p2.next := pp1;
    end
    else if (((p1.prev.pt.X > p1.pt.X) and (p2.prev.pt.X > p2.pt.X)) or
      ((p1.prev.pt.X < p1.pt.X) and (p2.prev.pt.X < p2.pt.X)) or
      ((p1.prev.pt.Y > p1.pt.Y) and (p2.prev.pt.Y > p2.pt.Y)) or
      ((p1.prev.pt.Y < p1.pt.Y) and (p2.prev.pt.Y < p2.pt.Y))) and
      SlopesEqual(p1.pt, p1.prev.pt, p2.pt, p2.prev.pt) then
    begin
      if fJoins[i].idx1 = fJoins[i].idx2 then continue;
      pp1 := DuplicatePolyPt(p1);
      pp2 := DuplicatePolyPt(p2);
      ReversePolyPtLinks(pp2);
      pp1.prev := pp2;
      pp2.next := pp1;
      p1.next := p2;
      p2.prev := p1;
    end
    else
      continue;

    if fJoins[i].idx1 = fJoins[i].idx2 then
    begin
      //When an edge join occurs within the same polygon, then
      //that polygon effectively splits into 2 polygons ...
      p1 := FixupOutPolygon(p1, true);
      fPolyPtList[fJoins[i].idx1] := p1;
      p2 := FixupOutPolygon(p2, true);
      newIdx := fPolyPtList.Add(p2);
      for j := i+1 to high(fJoins) do
      begin
        if (fJoins[j].idx1 = fJoins[i].idx1) and
          PtIsAPolyPt(fJoins[j].pt, p2) then
            fJoins[j].idx1 := newIdx;
        if (fJoins[j].idx2 = fJoins[i].idx1) and
          PtIsAPolyPt(fJoins[j].pt, p2) then
            fJoins[j].idx1 := newIdx;
      end;
    end else
    begin
      //When 2 polygons are merged (joined), pointers referencing the
      //'deleted' polygon must now point to the 'merged' polygon ...
      fPolyPtList[fJoins[i].idx2] := nil;
      for j := i+1 to high(fJoins) do
      begin
        if (fJoins[j].idx1 = fJoins[i].idx2) then
          fJoins[j].idx1 := fJoins[i].idx1;
        if (fJoins[j].idx2 = fJoins[i].idx2) then
          fJoins[j].idx2 := fJoins[i].idx1;
      end;
    end;
  end;
end;
//------------------------------------------------------------------------------

function SetDx(pp: PPolyPt): double; overload;
var
  dx, dy: double;
  pp2: PPolyPt;
begin
  if pp.dx = unassigned then
  begin
    if ofForwardBound in pp.flags then pp2 := pp.next else pp2 := pp.prev;
    dx := abs(pp.pt.X - pp2.pt.X);
    dy := abs(pp.pt.Y - pp2.pt.Y);
    if ((dx < 0.1) and  (dy *10 < dx)) or (dy < precision) then
      pp.dx := horizontal else
      pp.dx := (pp.pt.X - pp2.pt.X)/(pp.pt.Y - pp2.pt.Y);
  end;
  result := pp.dx;
end;
//------------------------------------------------------------------------------

function CompareForwardAngles(p1, p2: PPolyPt): integer;
var
  p1a, p1b, p1c, p2a, p2b, p2c: TDoublePoint;
  r1, r2: double;
  p1Forward, p2Forward: boolean;
  pTmp: PPolyPt;

  procedure NextPoint(var p: PPolyPt; goingForward: boolean);
  begin
    if goingForward then
    begin
      while PointsEqual(p.pt, p.next.pt) do p := p.next;
      p := p.next;
    end else
    begin
      while PointsEqual(p.pt, p.prev.pt) do p := p.prev;
      p := p.prev;
    end;
  end;

begin
  //preconditions:
  //1. p1a == p2a
  //2. p1->p1nextInBound is colinear with p2->p2nextInBound

  p1Forward := (ofForwardBound in p1.flags);
  p2Forward := (ofForwardBound in p2.flags);
  pTmp := nil;
  p1c := p1.pt; p2c := p2.pt; p1b := p1c; p2b := p2c;
  repeat
    p1a := p1b; p2a := p2b; p1b := p1c; p2b := p2c;
    NextPoint(p1, p1Forward);
    NextPoint(p2, p2Forward);

    //the following avoids a very rare endless loop where the
    //p1 & p2 polys are almost identical except for their orientations ...
    if pTmp = nil then pTmp := p1
    else if (pTmp = p1) then
    begin
      if PointsEqual(p1c, p2c) then
      begin
        result := 1;
        exit;
      end else break;
    end;

    p1c := p1.pt; p2c := p2.pt;
  until not PointsEqual(p1c, p2c);

  //and now ... p1c != p2c ...
  if PointsEqual(p1a, p1b) or
    PointsEqual(GetUnitNormal(p1b, p1c),GetUnitNormal(p2b, p2c)) then
  begin
    //we have parallel edges of unequal length ...
    if DistanceSqr(p1b, p1c) < DistanceSqr(p2b, p2c) then
    begin
      p1a := p1b; p1b := p1c;
      NextPoint(p1, p1Forward);
      r1 := GetR(p1a, p1b, p1.pt);
      if r1 > tolerance then result := 1
      else if r1 < -tolerance then result := -1
      else raise exception.Create(rsCompareForwardAngles);
    end else
    begin
      p2a := p2b; p2b := p2c;
      NextPoint(p2, p2Forward);
      r2 := GetR(p2a, p2b, p2.pt);
      if r2 > tolerance then result := -1
      else if r2 < -tolerance then result := 1
      else raise exception.Create(rsCompareForwardAngles);
    end;
  end else
  begin
    r1 := GetR(p1a, p1b, p1c);
    r2 := GetR(p2a, p2b, p2c);
    if r1 > r2 + tolerance then result := 1
    else if r1 < r2 - tolerance then result := -1
    else raise exception.Create(rsCompareForwardAngles);
  end;
end;
//------------------------------------------------------------------------------

function blCompare(item1, item2: pointer): integer;
var
  pp1, pp2: PPolyPt;
  dx1, dx2: double;
begin
  pp1 := PPolyPt(item1);
  pp2 := PPolyPt(item2);
  if pp2.pt.Y > pp1.pt.Y + precision then result := -1
  else if pp2.pt.Y < pp1.pt.Y - precision  then result := +1
  else if pp2.pt.X < pp1.pt.X - precision  then result := -1
  else if pp2.pt.X > pp1.pt.X + precision  then result := +1
  else if item1 = item2 then result := 0
  else
  begin
    dx1 := SetDx(pp1); dx2 := SetDx(pp2);
    if (dx1 < dx2 - precision) then result := -1
    else if (dx1 > dx2 + precision) then result := +1
    else result := CompareForwardAngles(pp1, pp2);
  end;
end;
//------------------------------------------------------------------------------

function wlCompare(item1, item2: pointer): integer;
var
  pp1, pp2, pp1Next, pp2Next: PPolyPt;
  r, dx1, dx2, pp1X: double;
begin
  //nb: 1. when item1 < item2, result = 1; when item2 < item1, result = -1
  //    2. wlCompare is only ever used for insertions into the skiplist.
  //    3. item2 is always the item being inserted.
  pp1 := PPolyPt(item1);
  pp2 := PPolyPt(item2);
  if (ofForwardBound in pp1.flags) then
    pp1Next := pp1.next else
    pp1Next := pp1.prev;

  if item1 = item2 then result := 0
  //given pp1 and pp1Next represent the p1 edge at Y coordinate pp2.pt.Y,
  //if the x coords of this edge are both before or both after pp2's x coord,
  //then this edge (and its polygon) must respectively precede or follow pp2 ...
  else if (pp1.pt.X < pp2.pt.X - tolerance) and
    (pp1Next.pt.X < pp2.pt.X + tolerance) then result := 1
  else if (pp1.pt.X > pp2.pt.X + tolerance) and
    (pp1Next.pt.X > pp2.pt.X - tolerance) then result := -1
  else if PointsEqual(pp1.pt, pp2.pt) then
  begin
    //compare slopes of edge pp1->pp1Next & edge pp2->pp2Next ...

    dx1 := SetDx(pp1); dx2 := SetDx(pp2);
    //dx1 should never be horizontal, but if dx2 is horizontal ...
    if dx2 = horizontal then
    begin
      if (ofForwardBound in pp2.flags) then
        pp2Next := pp2.next else
        pp2Next := pp2.prev;
      if pp2Next.pt.X < pp1.pt.X then result := -1 else result := 1;
    end
    else if (dx1 < dx2 - precision) then result := -1
    else if (dx1 > dx2 + precision) then result := 1
    //if these slopes are both equal then ...
    else result := CompareForwardAngles(pp1, pp2);
  end else
  begin
    SetDx(pp1);
    if pp1.dx = horizontal then
    begin
      if ofForwardBound in pp1.flags then pp2 := pp1.next else pp2 := pp1.prev;
      if pp2.pt.X > pp1.pt.X then result := -1 else result := 1;
      exit;
    end;
    pp1X := pp1.pt.X + (pp2.pt.Y - pp1.pt.Y) * pp1.dx;
    if (pp1X < pp2.pt.X - precision) then result := 1
    else if (pp1X > pp2.pt.X + precision) then result := -1
    else
    begin
      //presumably pp1 is below pp2 to get here and the edge bounded by
      //pp1->pp1Next touches pp2.pt.
      if (ofForwardBound in pp2.flags) then
        pp2Next := pp2.next else
        pp2Next := pp2.prev;

      result := 1;
      pp1Next := pp2Next;
      //given that edge pp2->pp2Next can't intersect pp1->pp1Next, if the angle
      //made by pp1->pp2 & pp2->pp2Next turns to the right (ie GetR > 0) then
      //the polygon owning pp2 must be to the right of the polygon owning pp1.
      repeat
        r := GetR(pp1.pt, pp2.pt, pp2Next.pt);
        if abs(r) < precision then //ie parallel
        begin
         if (ofForwardBound in pp2.flags) then
          pp2Next := pp2Next.next else
          pp2Next := pp2Next.prev;
          continue;
        end
        else if r > 0 then result := -1
        else result := 1;
        break;
      until pp2Next = pp1Next; //ie avoids a very rare endless loop
    end;
  end;
end;
//------------------------------------------------------------------------------

procedure TClipper.FixOrientation;
var
  i: integer;
  p, lowestP: PPolyPt;
  worklist, queue: TSkipList;
  sn, sn2: PSkipNode;
  tmpFlag: TOrientationFlags;
  isCW, lowestPending: boolean;

  function NextIsBottom(p: PPolyPt): boolean;
  var
    pp: PPolyPt;
  begin
    pp := p.next;
    while (pp.next.pt.Y = pp.pt.Y) do pp := pp.next;
    if pp.next.pt.Y > pp.pt.Y then result := false
    else result := true;
  end;

  procedure UpdateBounds(sn: PSkipNode; const Y: double);
  var
    pp, pp2: PPolyPt;
  begin
    pp := PPolyPt(sn.item);
    while true do
    begin
      if ofForwardBound in pp.flags then
        pp2 := pp.next else pp2 := pp.prev;
      if (pp2.pt.Y < Y - tolerance) then break;
      pp := pp2;
      if (ofTop in pp.flags) then break;
    end;
    //if we've reached the top of this bound then remove it from worklist ...
    if (ofTop in pp.flags) then
      //nb: DeleteItem() isn't safe here because of how
      //wCompare function works. Hence we use Delete() instead ...
      worklist.Delete(sn) else
      sn.item := pp;
  end;

begin
  //Preconditions:
  //1. All output polygons are simple polygons (ie no self-intersecting edges)
  //2. While output polygons may touch, none of them overlap other polygons.
  queue := TSkipList.Create(blCompare);
  worklist := TSkipList.Create(wlCompare);
  try
    for i := 0 to fPolyPtList.Count -1 do
      if assigned(fPolyPtList[i]) then
      begin
        //first, find the lowest left most PPolyPt for each polygon ...
        p := PPolyPt(fPolyPtList[i]);

        lowestP := p;
        p := p.next;
        repeat
          if p.pt.Y > lowestP.pt.Y then lowestP := p
          else if (p.pt.Y = lowestP.pt.Y) and
            (p.pt.X <= lowestP.pt.X) then lowestP := p;
          p := p.next;
        until (p = fPolyPtList[i]);

        //dispose of any invalid polygons here ...
        p := lowestP;
        if (p.next = p) or (p.prev = p.next) then
        begin
          DisposePolyPts(p);
          fPolyPtList[i] := nil;
          continue;
        end;
        fPolyPtList[i] := lowestP;
        if IsClockwise(lowestP) then tmpFlag := [ofCW] else tmpFlag := [];
        lowestP.flags := tmpFlag;
        lowestPending := true;

        //loop around the polygon, build 'bounds' for each polygon
        //and add them to the queue ...
        repeat
          while p.next.pt.Y = p.pt.Y do p := p.next; //ignore horizontals
          p.flags := tmpFlag + [ofForwardBound];
          if lowestPending and (ofCW in lowestP.flags) then
          begin
            include(p.flags, ofBottomMinima);
            lowestPending := false;
          end;

          queue.InsertItem(p);
          //go up the bound ...
          while (p.next.pt.Y <= p.pt.Y) do
          begin
            p.next.flags := tmpFlag + [ofForwardBound];
            p := p.next;
          end;
          Include(p.flags, ofTop);
          //now add the reverse bound (also reversing the bound direction) ...
          while not NextIsBottom(p) do
          begin
            p.next.flags := tmpFlag;
            p := p.next;
          end;

          if (p.next.pt.Y = p.next.next.pt.Y) then
          begin
            p := p.next;
            p.flags := tmpFlag;
            if not (ofCW in lowestP.flags) and PointsEqual(p.pt, lowestP.pt) and
              lowestPending then
            begin
              include(p.flags, ofBottomMinima);
              lowestPending := false;
            end;
            queue.InsertItem(p);
            while (p <> lowestP) and (p.next.pt.Y = p.pt.Y) do
              p := p.next;
          end else
          begin
            p := DuplicatePolyPt(p);
            p.pt := p.next.pt;
            p.flags := tmpFlag;
            if not (ofCW in lowestP.flags) and PointsEqual(p.pt, lowestP.pt) then
              include(p.flags, ofBottomMinima);
            queue.InsertItem(p);
            p := p.next;
            while (p <> lowestP) and (p.next.pt.Y = p.prev.pt.Y) do
              p := p.next;
          end;
        until (p = lowestP);
      end;

      if queue.Count = 0 then exit;
      p := PPolyPt(queue.PopFirst);

      worklist.InsertItem(p);
      include(p.flags, ofClockwise);

      while true do
      begin
        p := PPolyPt(queue.PopFirst);
        if not assigned(p) then break;

        sn := worklist.first;
        while assigned(sn) do
        begin
          //get the next item in workList in case sn is about to be removed ...
          sn2 := sn.next[0];
          //update each bound, keeping them level with the new bound 'p'
          //and removing bounds that are no longer in scope ...
          //nb: Bounds never intersect other bounds so UpdateBounds() should
          //not upset the order of the bounds in worklist.
          UpdateBounds(sn, p.pt.Y);
          sn := sn2;
        end;

        //insert the new bound into worklist ...
        sn := worklist.InsertItem(p);

        //if this is the bottom bound of a polyon,
        //then calculate the polygon's true orientation ...
        if (ofBottomMinima in p.flags) then
        begin
          sn2 := worklist.First;
          isCW := true;
          while sn2 <> sn do
          begin
            isCW := not isCW;
            sn2 := sn2.next[0];
          end;
          if isCW then include(p.flags, ofClockwise);
        end;
      end;
  finally
    queue.Free;
    worklist.Free;
  end;

  for i := 0 to fPolyPtList.Count -1 do
    if assigned(fPolyPtList[i]) then
    begin
      p := PPolyPt(fPolyPtList[i]);
      repeat
        if ofBottomMinima in p.flags then break;
        p := p.next;
      until p = PPolyPt(fPolyPtList[i]);
      if not (ofBottomMinima in p.flags) then
        raise exception.Create(rsFixOrientations);
      if not (p.flags * [ofCW,ofClockwise] = [ofCW,ofClockwise]) and not
        (p.flags * [ofCW,ofClockwise] = []) then
          ReversePolyPtLinks(PPolyPt(fPolyPtList[i]));
      p := FixupOutPolygon2(p);
      fPolyPtList[i] := p;
    end;
end;
//------------------------------------------------------------------------------

end.
