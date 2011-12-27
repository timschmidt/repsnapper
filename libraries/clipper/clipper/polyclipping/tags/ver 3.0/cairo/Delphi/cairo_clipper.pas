unit cairo_clipper;

(*******************************************************************************
*                                                                              *
* Author    :  Angus Johnson                                                   *
* Version   :  0.50                                                            *
* Date      :  30 October 2010                                                 *
* Website   :  http://www.angusj.com                                           *
* Copyright :  Angus Johnson 2010                                              *
*                                                                              *
* License:                                                                     *
* Use, modification & distribution is subject to Boost Software License Ver 1. *
* http://www.boost.org/LICENSE_1_0.txt                                         *
*                                                                              *
*******************************************************************************)

interface

uses
  SysUtils, Classes, Cairo, clipper;

function DoublePointArrayToCairo(const fpa: TArrayOfArrayOfDoublePoint;
  cairo: Pcairo_t): boolean;
function CairoToDoublePointArray(cairo: Pcairo_t;
  out fpa: TArrayOfArrayOfDoublePoint): boolean;

implementation

type
  PCairoPathDataArray = ^TCairoPathDataArray;
  TCairoPathDataArray =
    array [0.. MAXINT div sizeof(cairo_path_data_t) -1] of cairo_path_data_t;

function DoublePointArrayToCairo(const fpa: TArrayOfArrayOfDoublePoint;
  cairo: Pcairo_t): boolean;
var
  i,j: integer;
begin
  result := assigned(cairo);
  if not result then exit;
  for i := 0 to high(fpa) do
  begin
    cairo_new_sub_path(cairo);
    for j := 0 to high(fpa[i]) do
      with fpa[i][j] do
        cairo_line_to(cairo,X,Y); //does cairo_move_to() on first iteration
    cairo_close_path(cairo);
  end;
end;
//------------------------------------------------------------------------------

function CairoToDoublePointArray(cairo: Pcairo_t;
  out fpa: TArrayOfArrayOfDoublePoint): boolean;
const
  buffLen1: integer = 32;
  buffLen2: integer = 128;
var
  i,currLen1, currLen2: integer;
  pdHdr: cairo_path_data_t;
  path: Pcairo_path_t;
  currPos: TDoublePoint;
begin
  result := false;
  setlength(fpa, buffLen1);
  currLen1 := 1;
  currLen2 := 0;
  currPos := DoublePoint(0,0);
  i := 0;
  path := cairo_copy_path_flat(cairo);
  try
    while i < path.num_data do
    begin
      pdHdr := PCairoPathDataArray(path.data)[i];
      case pdHdr.header._type of
      CAIRO_PATH_CLOSE_PATH:
        begin
          if currLen2 > 1 then //ie: ignore if < 3 points (not a polygon)
          begin
            setlength(fpa[currLen1-1], currLen2);
            setlength(fpa[currLen1], buffLen2);
            inc(currLen1);
          end;
          currLen2 := 0;
          currPos := DoublePoint(0,0);
          result := true;
        end;
      CAIRO_PATH_MOVE_TO, CAIRO_PATH_LINE_TO:
        begin
          result := false;
          if (pdHdr.header._type = CAIRO_PATH_MOVE_TO) and
            (currLen2 > 0) then break; //ie enforce ClosePath for polygons
          if (currLen2 mod buffLen2 = 0) then
            SetLength(fpa[currLen1-1], currLen2 + buffLen2);
          with PCairoPathDataArray(path.data)[i+1].point do
            currPos := DoublePoint(x,y);
          fpa[currLen1-1][currLen2] := currPos;
          inc(currLen2);
        end;
      end;
      inc(i, pdHdr.header.length);
    end;
  finally
    cairo_path_destroy(path);
  end;
  dec(currLen1); //ie enforces a ClosePath
  setlength(fpa, currLen1);
end;
//------------------------------------------------------------------------------

end.
