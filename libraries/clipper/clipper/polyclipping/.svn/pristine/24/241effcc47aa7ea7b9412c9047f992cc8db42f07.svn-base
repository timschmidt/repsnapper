library clipper;

uses
  Windows, SysUtils, Classes, Gr32, Clipper2;

{$R *.res}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

function GetInt(s: PSingle; out int: integer): boolean;
begin
  if assigned(s) then
  begin
    int := round(s^);
    result := (abs(int - s^) < 0.001);
  end else
    result := false;
end;
//------------------------------------------------------------------------------

function GetDoublePoint(s: PSingle): TDoublePoint;
begin
  result.X := s^;
  result.Y := PSingle(pchar(s)+ sizeof(single))^;
end;
//------------------------------------------------------------------------------

function ASingleToAADoublePoint(arrSingle: PSingle): TArrayOfArrayOfDoublePoint;
var
  i, j, polyCnt, vertexCnt: integer;
  s: PSingle;
label Error;
begin
  result := nil;
  s := arrSingle;
  if not GetInt(s, polyCnt) or (polyCnt < 1) then exit;
  inc(s);
  SetLength(Result, polyCnt);
  for i := 0 to polyCnt -1 do
  begin
    if not GetInt(s, vertexCnt) or (vertexCnt < 3) then goto error;
    inc(s);
    SetLength(Result[i], vertexCnt);
    for j := 0 to vertexCnt -1 do
    begin
      Result[i][j] := GetDoublePoint(s);
      inc(s, 2);
    end;
  end;
  exit; //OK and all finished to get here

  error: result := nil;
end;
//------------------------------------------------------------------------------

procedure AAFloatPointToASingle(fp: TArrayOfArrayOfFloatPoint;
  s: PSingle; cnt: integer);
var
  i, j, polyCnt, vertexCnt: integer;
begin
  polyCnt := length(fp);
  if cnt < polyCnt then exit;
  s^ := polyCnt;
  inc(s);
  dec(cnt);
  for i := 0 to polyCnt -1 do
  begin
    if cnt = 0 then exit;
    vertexCnt := length(fp[i]);
    s^ := vertexCnt;
    inc(s);
    dec(cnt);
    for j := 0 to vertexCnt -1 do
    begin
      if cnt < 2 then exit;
      s^ := fp[i][j].X;
      inc(s);
      s^ := fp[i][j].Y;
      inc(s);
      dec(cnt, 2);
    end;
  end;
end;
//------------------------------------------------------------------------------

function ClipperCreate: Pointer; export; stdcall;
begin
  result := TClipper.Create;
  TClipper(result).ForceAlternateOrientation := true;
end;
//------------------------------------------------------------------------------

function ClipperAddPolygon(clipper: Pointer;
  poly: PSingle; polyType: TPolyType): boolean; export; stdcall;
var
  aafp: TArrayOfArrayOfDoublePoint;
begin
  result := assigned(clipper);
  if not result then exit;
  try
    aafp := ASingleToAADoublePoint(poly);
    TClipper(clipper).AddPolyPolygon(aafp, polyType);
  except
    result := false;
  end;
end;
//------------------------------------------------------------------------------

function ClipperExecute(clipper: Pointer; clipType: TClipType;
  out SolutionCount: integer): boolean; export; stdcall;
var
  i: integer;
begin
  SolutionCount := 0;
  result := assigned(clipper);
  if result then
  try
    with TClipper(clipper) do
    begin
      Solution := nil;
      result := Execute(clipType, Solution) and assigned(Solution);
      if not result then exit;
      SolutionCount := 1 + length(Solution);
      for i := 0 to length(Solution) -1 do
        inc(SolutionCount, length(Solution[i])*2);
    end;
  except
    result := false;
  end;
end;
//------------------------------------------------------------------------------

function ClipperSolution(clipper: Pointer;
  solution: PSingle; solutionCount: integer): boolean; export; stdcall;
begin
  result := assigned(clipper);
  if result then
  try
    AAFloatPointToASingle(TClipper(clipper).Solution, solution, solutionCount);
  except
    result := false;
  end;
end;
//------------------------------------------------------------------------------

function ClipperClear(clipper: Pointer): boolean; export; stdcall;
begin
  result := assigned(clipper);
  try
    TClipper(clipper).Clear;
    TClipper(clipper).Solution := nil;
  except
    result := false;
  end;
end;
//------------------------------------------------------------------------------

function ClipperFree(clipper: Pointer): boolean; export; stdcall;
begin
  result := assigned(clipper);
  if result then
  try
    TClipper(clipper).Free;
  except
    result := false;
  end;
end;
//------------------------------------------------------------------------------

exports
  ClipperCreate,
  ClipperAddPolygon,
  ClipperExecute,
  ClipperSolution,
  ClipperClear,
  ClipperFree;

begin
end.
