unit clipper_misc;

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
*******************************************************************************)

{$DEFINE USING_GRAPHICS32}

interface

uses
{$IFDEF USING_GRAPHICS32}
  GR32,
{$ENDIF}
  SysUtils, Classes, Math, clipper;


//Area() ...
//The return value will be invalid if the supplied polygon is self-intersecting.
function Area(const pts: TArrayOfDoublePoint): double;

//OffsetPolygons() ...
//A positive delta will offset each polygon edge towards its left, so polygons
//orientated clockwise (ie outer polygons) will expand but inner polyons (holes)
//will shrink. Conversely, negative deltas will offset polygon edges towards
//their right so outer polygons will shrink and inner polygons will expand.
function OffsetPolygons(const pts: TArrayOfArrayOfDoublePoint;
  const delta: double): TArrayOfArrayOfDoublePoint; overload;
function OffsetPolygons(const pts: TArrayOfArrayOfFloatPoint;
  const delta: double): TArrayOfArrayOfFloatPoint; overload;

implementation

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

function Area(const pts: TArrayOfDoublePoint): double;
var
  i, highI: integer;
begin
  result := 0;
  highI := high(pts);
  if highI < 2 then exit;
  result := pts[highI].x * pts[0].y - pts[0].x * pts[highI].y;
  for i := 0 to highI-1 do
    result := result + pts[i].x * pts[i+1].y - pts[i+1].x * pts[i].y;
  result := result/2;
end;
//------------------------------------------------------------------------------

function BuildArc(const pt: TDoublePoint;
  a1, a2, r: single): TArrayOfDoublePoint; overload;
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
    Result[i].X := pt.X + (C * r);
    Result[i].Y := pt.Y + (S * r);
    a := a + da;
  end;
end;
//------------------------------------------------------------------------------

function InsertPoints(const existingPts, newPts:
  TArrayOfDoublePoint; position: integer): TArrayOfDoublePoint; overload;
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
    result[position+lenN],(lenE-position)*sizeof(TDoublePoint));
  Move(newPts[0], result[position], lenN*sizeof(TDoublePoint));
end;
//------------------------------------------------------------------------------

function OffsetPolygons(const pts: TArrayOfArrayOfDoublePoint;
  const delta: double): TArrayOfArrayOfDoublePoint;
var
  j, i, highI: integer;
  normals: TArrayOfDoublePoint;
  a1,a2, deltaSq: double;
  arc, outer: TArrayOfDoublePoint;
  r: TFloatRect;
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
      result[j][i*2].X := pts[j][i].X +delta *normals[i].X;
      result[j][i*2].Y := pts[j][i].Y +delta *normals[i].Y;
      result[j][i*2+1].X := pts[j][i].X +delta *normals[i+1].X;
      result[j][i*2+1].Y := pts[j][i].Y +delta *normals[i+1].Y;
    end;
    result[j][highI*2].X := pts[j][highI].X +delta *normals[highI].X;
    result[j][highI*2].Y := pts[j][highI].Y +delta *normals[highI].Y;
    result[j][highI*2+1].X := pts[j][highI].X +delta *normals[0].X;
    result[j][highI*2+1].Y := pts[j][highI].Y +delta *normals[0].Y;

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
    c.AddPolyPolygon(result, ptSubject);
    if delta > 0 then
    begin
      if not c.Execute(ctUnion, result, pftNonZero, pftNonZero) then
        result := nil;
    end else
    begin
      r := c.GetBounds;
      setlength(outer, 4);
      outer[0] := DoublePoint(r.left-10, r.bottom+10);
      outer[1] := DoublePoint(r.right+10, r.bottom+10);
      outer[2] := DoublePoint(r.right+10, r.top-10);
      outer[3] := DoublePoint(r.left-10, r.top-10);
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

function OffsetPolygons(const pts: TArrayOfArrayOfFloatPoint;
  const delta: double): TArrayOfArrayOfFloatPoint;
var
  i, len: integer;
  dblPts: TArrayOfArrayOfDoublePoint;
begin
  len := length(pts);
  setlength(dblPts, len);
  for i := 0 to len -1 do
    dblPts[i] := MakeArrayOfDoublePoint(pts[i]);
  dblPts := OffsetPolygons(dblPts, delta);
  len := length(dblPts);
  setlength(result, len);
  for i := 0 to len -1 do
    Result[i] := MakeArrayOfFloatPoint(dblPts[i]);
end;
//------------------------------------------------------------------------------

end.
