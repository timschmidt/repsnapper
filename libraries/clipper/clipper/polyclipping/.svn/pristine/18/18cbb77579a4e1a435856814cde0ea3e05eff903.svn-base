unit GR32_PolygonsEx;

(* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is Vectorial Polygon Rasterizer for Graphics32
 *
 * The Initial Developer of the Original Code is
 * Mattias Andersson <mattias@centaurix.com>
 *
 * Portions created by the Initial Developer are Copyright (C) 2008-2009
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *
 * ***** END LICENSE BLOCK ***** *)

interface

{$I GR32.inc}

{.$DEFINE USEGR32GAMMA}
{.$DEFINE CHANGENOTIFICATIONS}

uses
  GR32, GR32_VPR, GR32_Polygons, GR32_VectorUtils, Types;

type
  TUnpackedFillProc = procedure(Coverage: PSingleArray; AlphaValues: PColor32Array;
    Count: Integer; Color: TColor32);
  TPackedFillProc = procedure(Coverage: Single; AlphaValues: PColor32Array;
    Count: Integer; Color: TColor32);

type
  { TPolygonRenderer }
  TPolygonRenderer = class
  protected
    procedure RenderSpan(const Span: TValueSpan; DstY: Integer); virtual; abstract;
  end;

  { TPolygonRenderer32 }
  TPolygonRenderer32 = class(TPolygonRenderer)
  private
    FBitmap: TBitmap32;
    FFillMode: TPolyFillMode;
    FFillProcPacked: TPackedFillProc;
    FFillProcUnpacked: TUnpackedFillProc;
    FColor: TColor32;
    FFiller: TCustomPolygonFiller;
    procedure SetColor(const Value: TColor32);
    procedure SetFillMode(const Value: TPolyFillMode);
    procedure SetFiller(const Value: TCustomPolygonFiller);
  protected
    procedure RenderSpan(const Span: TValueSpan; DstY: Integer); override;
    procedure RenderSpanLCD(const Span: TValueSpan; DstY: Integer);
    procedure RenderSpanLCD2(const Span: TValueSpan; DstY: Integer);
    procedure FillSpan(const Span: TValueSpan; DstY: Integer);
    procedure UpdateFillProcs;
  public
    constructor Create; virtual;
    property Bitmap: TBitmap32 read FBitmap write FBitmap;
    property FillMode: TPolyFillMode read FFillMode write SetFillMode;
    property Color: TColor32 read FColor write SetColor;
    property Filler: TCustomPolygonFiller read FFiller write SetFiller;
  end;

procedure PolyPolygonFS(Bitmap: TBitmap32; const Points: TArrayOfArrayOfFloatPoint;
  Color: TColor32; FillMode: TPolyFillMode = pfAlternate); overload;
procedure PolygonFS(Bitmap: TBitmap32; const Points: TArrayOfFloatPoint;
  Color: TColor32; FillMode: TPolyFillMode = pfAlternate); overload;
procedure PolyPolygonFS(Bitmap: TBitmap32; const Points: TArrayOfArrayOfFloatPoint;
  Filler: TCustomPolygonFiller; FillMode: TPolyFillMode = pfAlternate); overload;
procedure PolygonFS(Bitmap: TBitmap32; const Points: TArrayOfFloatPoint;
  Filler: TCustomPolygonFiller; FillMode: TPolyFillMode = pfAlternate); overload;
procedure PolyPolygonFS_LCD(Bitmap: TBitmap32; const Points: TArrayOfArrayOfFloatPoint;
  Color: TColor32; FillMode: TPolyFillMode = pfAlternate); overload;
procedure PolygonFS_LCD(Bitmap: TBitmap32; const Points: TArrayOfFloatPoint;
  Color: TColor32; FillMode: TPolyFillMode = pfAlternate);
procedure PolyPolygonFS_LCD2(Bitmap: TBitmap32; const Points: TArrayOfArrayOfFloatPoint;
  Color: TColor32; FillMode: TPolyFillMode = pfAlternate); overload;
procedure PolygonFS_LCD2(Bitmap: TBitmap32; const Points: TArrayOfFloatPoint;
  Color: TColor32; FillMode: TPolyFillMode = pfAlternate);

procedure PolyPolylineFS(Bitmap: TBitmap32; const Points: TArrayOfArrayOfFloatPoint;
  Color: TColor32; Closed: Boolean = False; StrokeWidth: TFloat = 1.0;
  JoinStyle: TJoinStyle = jsMiter; EndStyle: TEndStyle = esButt;
  MiterLimit: TFloat = 4.0);
procedure PolylineFS(Bitmap: TBitmap32; const Points: TArrayOfFloatPoint;
  Color: TColor32; Closed: Boolean = False; StrokeWidth: TFloat = 1.0;
  JoinStyle: TJoinStyle = jsMiter; EndStyle: TEndStyle = esButt;
  MiterLimit: TFloat = 4.0);

implementation

uses
  GR32_LowLevel, GR32_Blend, Math, SysUtils;

type
  TBitmap32Access = class(TBitmap32);

{$IFDEF COMPILER2006}
var
  bias_ptr: Pointer;
{$ENDIF}

// routines for color filling:

procedure MakeAlphaNonZeroUP(Coverage: PSingleArray; AlphaValues: PColor32Array;
  Count: Integer; Color: TColor32);
var
  I: Integer;
  M, V, C: Cardinal;
  Last: TFloat;
begin
  M := Color shr 24 * $101;
  C := Color and $00ffffff;

  Last := Infinity;
  V := 0;
  for I := 0 to Count - 1 do
  begin
    if PInteger(@Last)^ <> PInteger(@Coverage[I])^ then
    begin
      Last := Coverage[I];
      V := Abs(Round(Last * $10000));
      if V > $10000 then V := $10000;
{$IFDEF USEGR32GAMMA}
      V := GAMMA_TABLE[V * M shr 24] shl 24 or C;
{$ELSE}
      V := (V * M and $ff000000) or C;
{$ENDIF}
    end;
    AlphaValues[I] := V;
  end;
end;

(*
procedure MakeAlphaNonZeroUP(Coverage: PSingleArray; AlphaValues: PColor32Array;
  Count: Integer; Color: TColor32);
var
  I: Integer;
  M, V, C: Cardinal;
begin
  M := Color shr 24 * $101;
  C := Color and $00ffffff;
  for I := 0 to Count - 1 do
  begin
    V := Abs(Round(Coverage[I] * $10000));
    if V > $10000 then V := $10000;
{$IFDEF USEGR32GAMMA}
    V := GAMMA_TABLE[V * M shr 24];
    AlphaValues[I] := (V shl 24) or C;
{$ELSE}
    AlphaValues[I] := (V * M and $ff000000) or C;
{$ENDIF}
  end;
end;
*)

procedure MakeAlphaEvenOddUP(Coverage: PSingleArray; AlphaValues: PColor32Array;
  Count: Integer; Color: TColor32);
var
  I: Integer;
  M, V, C: Cardinal;
  Last: TFloat;
begin
  M := Color shr 24 * $101;
  C := Color and $00ffffff;
  Last := Infinity;
  V := 0;
  for I := 0 to Count - 1 do
  begin
    if PInteger(@Last)^ <> PInteger(@Coverage[I])^ then
    begin
      Last := Coverage[I];
      V := Abs(Round(Coverage[I] * $10000));
      V := V and $01ffff;
      if V >= $10000 then V := V xor $1ffff;
{$IFDEF USEGR32GAMMA}
      V := GAMMA_TABLE[V * M shr 24] shl 24 or C;
{$ELSE}
      V := (V * M and $ff000000) or C;
{$ENDIF}
    end;
    AlphaValues[I] := V;
  end;
end;

procedure MakeAlphaNonZeroP(Value: Single; AlphaValues: PColor32Array;
  Count: Integer; Color: TColor32);
var
  M, V, C: Cardinal;
begin
  M := Color shr 24 * $101;
  C := Color and $00ffffff;
  V := Abs(Round(Value * $10000));
  if V > $10000 then V := $10000;
{$IFDEF USEGR32GAMMA}
  V := GAMMA_TABLE[V * M shr 24];
  V := V shl 24 or C;
{$ELSE}
  V := (V * M and $ff000000) or C;
{$ENDIF}
  FillLongWord(AlphaValues[0], Count, V);
end;

procedure MakeAlphaEvenOddP(Value: Single; AlphaValues: PColor32Array;
  Count: Integer; Color: TColor32);
var
  M, V, C: Cardinal;
begin
  M := Color shr 24 * $101;
  C := Color and $00ffffff;
  V := Abs(Round(Value * $10000));
  V := V and $01ffff;
  if V > $10000 then V := V xor $1ffff;
{$IFDEF USEGR32GAMMA}
  V := GAMMA_TABLE[V * M shr 24];
  V := V shl 24 or C;
{$ELSE}
  V := (V * M and $ff000000) or C;
{$ENDIF}
  FillLongWord(AlphaValues[0], Count, V);
end;


// polygon filler routines (extract alpha only):

procedure MakeAlphaNonZeroUPF(Coverage: PSingleArray; AlphaValues: PColor32Array;
  Count: Integer; Color: TColor32);
var
  I: Integer;
  V: Integer;
begin
  for I := 0 to Count - 1 do
  begin
    V := Clamp(Round(Abs(Coverage[I]) * 256));
{$IFDEF USEGR32GAMMA}
    V := GAMMA_TABLE[V];
{$ENDIF}
    AlphaValues[I] := V;
  end;
end;

procedure MakeAlphaEvenOddUPF(Coverage: PSingleArray; AlphaValues: PColor32Array;
  Count: Integer; Color: TColor32);
var
  I: Integer;
  V: Integer;
begin
  for I := 0 to Count - 1 do
  begin
    V := Round(Abs(Coverage[I]) * 256);
    V := V and $000001ff;
    if V >= $100 then V := V xor $1ff;
{$IFDEF USEGR32GAMMA}
    V := GAMMA_TABLE[V];
{$ENDIF}
    AlphaValues[I] := V;
  end;
end;

procedure MakeAlphaNonZeroPF(Value: Single; AlphaValues: PColor32Array;
  Count: Integer; Color: TColor32);
var
  V: Integer;
begin
  V := Clamp(Round(Abs(Value) * 256));
{$IFDEF USEGR32GAMMA}
    V := GAMMA_TABLE[V];
{$ENDIF}
  FillLongWord(AlphaValues[0], Count, V);
end;

procedure MakeAlphaEvenOddPF(Value: Single; AlphaValues: PColor32Array;
  Count: Integer; Color: TColor32);
var
  V: Integer;
begin
  V := Round(Abs(Value) * 256);
  V := V and $000001ff;
  if V >= $100 then V := V xor $1ff;
{$IFDEF USEGR32GAMMA}
    V := GAMMA_TABLE[V];
{$ENDIF}
  FillLongWord(AlphaValues[0], Count, V);
end;

procedure PolyPolygonFS(Bitmap: TBitmap32; const Points: TArrayOfArrayOfFloatPoint;
  Color: TColor32; FillMode: TPolyFillMode);
var
  Renderer: TPolygonRenderer32;
{$IFDEF CHANGENOTIFICATIONS}
  I: Integer;
{$ENDIF}
begin
  Renderer := TPolygonRenderer32.Create;
  try
    Renderer.Bitmap := Bitmap;
    Renderer.Color := Color;
    Renderer.FillMode := FillMode;
    RenderPolyPolygon(Points, FloatRect(Bitmap.ClipRect), Renderer.RenderSpan);
{$IFDEF CHANGENOTIFICATIONS}
    if TBitmap32Access(Bitmap).UpdateCount = 0 then
      for I := 0 to High(Points) do
        Bitmap.Changed(MakeRect(PolygonBounds(Points[I])));
{$ENDIF}
  finally
    Renderer.Free;
  end;
end;

procedure PolygonFS(Bitmap: TBitmap32; const Points: TArrayOfFloatPoint;
  Color: TColor32; FillMode: TPolyFillMode);
var
  Renderer: TPolygonRenderer32;
begin
  Renderer := TPolygonRenderer32.Create;
  try
    Renderer.Bitmap := Bitmap;
    Renderer.Color := Color;
    Renderer.FillMode := FillMode;
    RenderPolygon(Points, FloatRect(Bitmap.ClipRect), Renderer.RenderSpan);
{$IFDEF CHANGENOTIFICATIONS}
    if TBitmap32Access(Bitmap).UpdateCount = 0 then
      Bitmap.Changed(MakeRect(PolygonBounds(Points)));
{$ENDIF}
  finally
    Renderer.Free;
  end;
end;

procedure PolyPolygonFS(Bitmap: TBitmap32; const Points: TArrayOfArrayOfFloatPoint;
  Filler: TCustomPolygonFiller; FillMode: TPolyFillMode);
var
  Renderer: TPolygonRenderer32;
{$IFDEF CHANGENOTIFICATIONS}
  I: Integer;
{$ENDIF}
begin
  Renderer := TPolygonRenderer32.Create;
  try
    Renderer.Bitmap := Bitmap;
    Renderer.Filler := Filler;
    Renderer.FillMode := FillMode;
    RenderPolyPolygon(Points, FloatRect(Bitmap.ClipRect), Renderer.FillSpan);
{$IFDEF CHANGENOTIFICATIONS}
    if TBitmap32Access(Bitmap).UpdateCount = 0 then
      for I := 0 to High(Points) do
        Bitmap.Changed(MakeRect(PolygonBounds(Points[I])));
{$ENDIF}
  finally
    Renderer.Free;
  end;
end;

procedure PolygonFS(Bitmap: TBitmap32; const Points: TArrayOfFloatPoint;
  Filler: TCustomPolygonFiller; FillMode: TPolyFillMode);
var
  Renderer: TPolygonRenderer32;
begin
  Renderer := TPolygonRenderer32.Create;
  try
    Renderer.Bitmap := Bitmap;
    Renderer.Filler := Filler;
    Renderer.FillMode := FillMode;
    RenderPolygon(Points, FloatRect(Bitmap.ClipRect), Renderer.FillSpan);
{$IFDEF CHANGENOTIFICATIONS}
    if TBitmap32Access(Bitmap).UpdateCount = 0 then
      Bitmap.Changed(MakeRect(PolygonBounds(Points)));
{$ENDIF}
  finally
    Renderer.Free;
  end;
end;

procedure PolygonFS_LCD(Bitmap: TBitmap32; const Points: TArrayOfFloatPoint;
  Color: TColor32; FillMode: TPolyFillMode = pfAlternate);
var
  Renderer: TPolygonRenderer32;
  ClipRect: TFloatRect;
  NewPoints: TArrayOfFloatPoint;
begin
  Renderer := TPolygonRenderer32.Create;
  try
    Renderer.Bitmap := Bitmap;
    Renderer.FillMode := FillMode;
    Renderer.Color := Color;
    with Bitmap.ClipRect do
      ClipRect := FloatRect(Left * 3, Top, Right * 3, Bottom);

    NewPoints := ScalePolygon(Points, 3, 1);
    RenderPolygon(NewPoints, ClipRect, Renderer.RenderSpanLCD);
{$IFDEF CHANGENOTIFICATIONS}
    if TBitmap32Access(Bitmap).UpdateCount = 0 then
      Bitmap.Changed(MakeRect(PolygonBounds(Points)));
{$ENDIF}
  finally
    Renderer.Free;
  end;
end;

procedure PolyPolygonFS_LCD(Bitmap: TBitmap32; const Points: TArrayOfArrayOfFloatPoint;
  Color: TColor32; FillMode: TPolyFillMode = pfAlternate); overload;
var
  Renderer: TPolygonRenderer32;
  ClipRect: TFloatRect;
  NewPoints: TArrayOfArrayOfFloatPoint;
{$IFDEF CHANGENOTIFICATIONS}
  I: Integer;
{$ENDIF}
begin
  Renderer := TPolygonRenderer32.Create;
  try
    Renderer.Bitmap := Bitmap;
    Renderer.FillMode := FillMode;
    Renderer.Color := Color;
    with Bitmap.ClipRect do
      ClipRect := FloatRect(Left * 3, Top, Right * 3, Bottom);

    NewPoints := ScalePolyPolygon(Points, 3, 1);
    RenderPolyPolygon(NewPoints, ClipRect, Renderer.RenderSpanLCD);
{$IFDEF CHANGENOTIFICATIONS}
    if TBitmap32Access(Bitmap).UpdateCount = 0 then
      for I := 0 to High(Points) do
        Bitmap.Changed(MakeRect(PolygonBounds(Points[I])));
{$ENDIF}
  finally
    Renderer.Free;
  end;
end;

procedure PolygonFS_LCD2(Bitmap: TBitmap32; const Points: TArrayOfFloatPoint;
  Color: TColor32; FillMode: TPolyFillMode = pfAlternate);
var
  Renderer: TPolygonRenderer32;
  ClipRect: TFloatRect;
  NewPoints: TArrayOfFloatPoint;
begin
  Renderer := TPolygonRenderer32.Create;
  try
    Renderer.Bitmap := Bitmap;
    Renderer.FillMode := FillMode;
    Renderer.Color := Color;
    with Bitmap.ClipRect do
      ClipRect := FloatRect(Left * 3, Top, Right * 3, Bottom);

    NewPoints := ScalePolygon(Points, 3, 1);
    RenderPolygon(NewPoints, ClipRect, Renderer.RenderSpanLCD2);
{$IFDEF CHANGENOTIFICATIONS}
    if TBitmap32Access(Bitmap).UpdateCount = 0 then
      Bitmap.Changed(MakeRect(PolygonBounds(Points)));
{$ENDIF}
  finally
    Renderer.Free;
  end;
end;

procedure PolyPolygonFS_LCD2(Bitmap: TBitmap32; const Points: TArrayOfArrayOfFloatPoint;
  Color: TColor32; FillMode: TPolyFillMode = pfAlternate); overload;
var
  Renderer: TPolygonRenderer32;
  ClipRect: TFloatRect;
  NewPoints: TArrayOfArrayOfFloatPoint;
{$IFDEF CHANGENOTIFICATIONS}
  I: Integer;
{$ENDIF}
begin
  Renderer := TPolygonRenderer32.Create;
  try
    Renderer.Bitmap := Bitmap;
    Renderer.FillMode := FillMode;
    Renderer.Color := Color;
    with Bitmap.ClipRect do
      ClipRect := FloatRect(Left * 3, Top, Right * 3, Bottom);

    NewPoints := ScalePolyPolygon(Points, 3, 1);
    RenderPolyPolygon(NewPoints, ClipRect, Renderer.RenderSpanLCD2);
{$IFDEF CHANGENOTIFICATIONS}
    if TBitmap32Access(Bitmap).UpdateCount = 0 then
      for I := 0 to High(Points) do
        Bitmap.Changed(MakeRect(PolygonBounds(Points[I])));
{$ENDIF}
  finally
    Renderer.Free;
  end;
end;

procedure PolyPolylineFS(Bitmap: TBitmap32; const Points: TArrayOfArrayOfFloatPoint;
  Color: TColor32; Closed: Boolean = False; StrokeWidth: TFloat = 1.0;
  JoinStyle: TJoinStyle = jsMiter; EndStyle: TEndStyle = esButt;
  MiterLimit: TFloat = 4.0);
var
  I: Integer;
  P1, P2: TArrayOfFloatPoint;
  Dst: TArrayOfArrayOfFloatPoint;
  Normals: TArrayOfFloatPoint;
begin
  if Closed then
  begin
    SetLength(Dst, Length(Points)*2);
    for I := 0 to High(Points) do
    begin
      Normals := BuildNormals(Points[I]);
      P1 := Grow(Points[I], Normals, StrokeWidth * 0.5, JoinStyle, true, MiterLimit);
      P2 := Grow(Points[I], Normals, -StrokeWidth * 0.5, JoinStyle, true, MiterLimit);
      Dst[I * 2] := P1;
      Dst[I * 2 + 1] := ReversePolygon(P2);
    end;
  end
  else
  begin
    SetLength(Dst, Length(Points));
    for I := 0 to High(Points) do
      Dst[I] := BuildPolyline(Points[I], StrokeWidth, JoinStyle, EndStyle);
  end;

  PolyPolygonFS(Bitmap, Dst, Color, pfWinding);
end;

procedure PolylineFS(Bitmap: TBitmap32; const Points: TArrayOfFloatPoint;
  Color: TColor32; Closed: Boolean = False; StrokeWidth: TFloat = 1.0;
  JoinStyle: TJoinStyle = jsMiter; EndStyle: TEndStyle = esButt;
  MiterLimit: TFloat = 4.0);
var
  P: TArrayOfArrayOfFloatPoint;
begin
  SetLength(P, 1);
  P[0] := Points;
  PolyPolylineFS(Bitmap, P, Color, Closed, StrokeWidth, JoinStyle, EndStyle, MiterLimit);
end;



{ LCD sub-pixel rendering (see http://www.grc.com/cttech.htm) }

type
  PRGBTriple = ^TRGBTriple;
  TRGBTriple = packed record
    B, G, R: Byte;
  end;

  PRGBTripleArray = ^TRGBTripleArray;
  TRGBTripleArray = array [0..0] of TRGBTriple;

  TMakeAlphaProcLCD = procedure(Coverage: PSingleArray; AlphaValues: SysUtils.PByteArray;
    Count: Integer; Color: TColor32);

procedure MakeAlphaNonZeroLCD(Coverage: PSingleArray; AlphaValues: SysUtils.PByteArray;
  Count: Integer; Color: TColor32);
var
  I: Integer;
  M, V: Cardinal;
  Last: TFloat;
begin
  M := Color shr 24 * 86;  // 86 = 258 / 3

  Last := Infinity;
  V := 0;
  AlphaValues[0] := 0;
  AlphaValues[1] := 0;
  for I := 0 to Count - 1 do
  begin
    if PInteger(@Last)^ <> PInteger(@Coverage[I])^ then
    begin
      Last := Coverage[I];
      V := Abs(Round(Last * $10000));
      if V > $10000 then V := $10000;
      V := V * M shr 24;
    end;
    Inc(AlphaValues[I], V);
{$IFDEF USEGR32GAMMA}
    AlphaValues[I] := GAMMA_TABLE[AlphaValues[I]];
{$ENDIF}
    Inc(AlphaValues[I + 1], V);
    AlphaValues[I + 2] := V;
  end;
  AlphaValues[Count + 2] := 0;
  AlphaValues[Count + 3] := 0;
end;

procedure MakeAlphaEvenOddLCD(Coverage: PSingleArray; AlphaValues: SysUtils.PByteArray;
  Count: Integer; Color: TColor32);
var
  I: Integer;
  M, V: Cardinal;
  Last: TFloat;
begin
  M := Color shr 24 * 86;  // 86 = 258 / 3

  Last := Infinity;
  V := 0;
  AlphaValues[0] := 0;
  AlphaValues[1] := 0;
  for I := 0 to Count - 1 do
  begin
    if PInteger(@Last)^ <> PInteger(@Coverage[I])^ then
    begin
      Last := Coverage[I];
      V := Abs(Round(Coverage[I] * $10000));
      V := V and $01ffff;
      if V >= $10000 then V := V xor $1ffff;
      V := V * M shr 24;
    end;
    Inc(AlphaValues[I], V);
{$IFDEF USEGR32GAMMA}
    AlphaValues[I] := GAMMA_TABLE[AlphaValues[I]];
{$ENDIF}
    Inc(AlphaValues[I + 1], V);
    AlphaValues[I + 2] := V;
  end;
  AlphaValues[Count + 2] := 0;
  AlphaValues[Count + 3] := 0;
end;

procedure MakeAlphaNonZeroLCD2(Coverage: PSingleArray; AlphaValues: SysUtils.PByteArray;
  Count: Integer; Color: TColor32);
var
  I: Integer;
begin
  MakeAlphaNonZeroLCD(Coverage, AlphaValues, Count, Color);
  AlphaValues[Count + 2] := (AlphaValues[Count] + AlphaValues[Count + 1]) div 3;
  AlphaValues[Count + 3] := AlphaValues[Count + 1] div 3;
  for I := Count + 1 downto 2 do
  begin
    AlphaValues[I] := (AlphaValues[I] + AlphaValues[I - 1] + AlphaValues[I - 2]) div 3;
  end;
  AlphaValues[1] := (AlphaValues[0] + AlphaValues[1]) div 3;
  AlphaValues[0] := AlphaValues[0] div 3;
end;

procedure MakeAlphaEvenOddLCD2(Coverage: PSingleArray; AlphaValues: SysUtils.PByteArray;
  Count: Integer; Color: TColor32);
var
  I: Integer;
begin
  MakeAlphaEvenOddLCD(Coverage, AlphaValues, Count, Color);
  AlphaValues[Count + 2] := (AlphaValues[Count] + AlphaValues[Count + 1]) div 3;
  AlphaValues[Count + 3] := AlphaValues[Count + 1] div 3;
  for I := Count + 1 downto 2 do
  begin
    AlphaValues[I] := (AlphaValues[I] + AlphaValues[I - 1] + AlphaValues[I - 2]) div 3;
  end;
  AlphaValues[1] := (AlphaValues[0] + AlphaValues[1]) div 3;
  AlphaValues[0] := AlphaValues[0] div 3;
end;

function DivMod(Dividend, Divisor: Integer; var Remainder: Integer): Integer;
asm
        push      edx
        cdq
        idiv      dword ptr [esp]
        add       esp,$04
        mov       dword ptr [ecx], edx
end;

//procedure BlendRGB_MMX(F, W: TColor32; var B: TColor32);
//asm
//        PXOR      MM2,MM2
//        MOVD      MM0,EAX
//        PUNPCKLBW MM0,MM2
//        MOVD      MM1,[ECX]
//        PUNPCKLBW MM1,MM2
//        BSWAP     EDX
//        PSUBW     MM0,MM1
//        MOVD      MM3,EDX
//        PUNPCKLBW MM3,MM2
//        PMULLW    MM0,MM3
//        MOV       EAX,bias_ptr
//        PSLLW     MM1,8
//        PADDW     MM1,[EAX]
//        PADDW     MM1,MM0
//        PSRLW     MM1,8
//        PACKUSWB  MM1,MM2
//        MOVD      [ECX],MM1
//        MOV       ECX,bias_ptr
//end;

procedure CombineLineLCD(Weights: PRGBTripleArray; Dst: PColor32Array; Color: TColor32; Count: Integer);
var
  I: Integer;
  C: TColor32Entry absolute Color;
  W: PRGBTriple;
begin
  for I := 0 to Count - 1 do
  begin

    W := @Weights[I];
    with PColor32Entry(@Dst[I])^ do
    begin
      R := (C.R - R) * W.R div 255 + R;
      G := (C.G - G) * W.G div 255 + G;
      B := (C.B - B) * W.B div 255 + B;
    end;

    //BlendRGB_MMX(Color, PColor32(@Weights[I])^, Dst[I]);
  end;
  EMMS;
end;

{ TPolygonRenderer32 }

constructor TPolygonRenderer32.Create;
begin
  FillMode := pfAlternate;
  UpdateFillProcs;
end;

{$O-}
procedure TPolygonRenderer32.FillSpan(const Span: TValueSpan;
  DstY: Integer);
var
  AlphaValues: PColor32Array;
  Count: Integer;
begin
  Count := Span.X2 - Span.X1 + 1;
  AlphaValues := StackAlloc(Count * SizeOf(TColor32));
  FFillProcUnpacked(Span.Values, AlphaValues, Count, FColor);
  FFiller.FillLine(@Bitmap.ScanLine[DstY][Span.X1], Span.X1, DstY, Count, PColor32(AlphaValues));
  EMMS;
  StackFree(AlphaValues);
end;

procedure TPolygonRenderer32.RenderSpan(const Span: TValueSpan; DstY: Integer);
var
  AlphaValues: PColor32Array;
  Count: Integer;
begin
  Count := Span.X2 - Span.X1 + 1;
  AlphaValues := StackAlloc(Count * SizeOf(TColor32));
  FFillProcUnpacked(Span.Values, AlphaValues, Count, FColor);
  BlendLine(@AlphaValues[0], @Bitmap.ScanLine[DstY][Span.X1], Count);
  EMMS;
  StackFree(AlphaValues);
end;

procedure TPolygonRenderer32.RenderSpanLCD(const Span: TValueSpan;
  DstY: Integer);
const
  PADDING = 5;
var
  AlphaValues: SysUtils.PByteArray;
  Count: Integer;
  X1, Offset: Integer;
const
  MakeAlpha: array [TPolyFillMode] of TMakeAlphaProcLCD = (MakeAlphaEvenOddLCD, MakeAlphaNonZeroLCD);
begin
  Count := Span.X2 - Span.X1 + 1;
  X1 := DivMod(Span.X1, 3, Offset);

  // Left Padding + Right Padding + Filter Width = 2 + 2 + 2 = 6
  AlphaValues := StackAlloc((Count + 6 + PADDING) * SizeOf(Byte));
  AlphaValues[0] := 0;
  AlphaValues[1] := 0;
  if (X1 > 0) then
  begin
    Dec(X1);
    Inc(Offset, 3);
    AlphaValues[2] := 0;
    AlphaValues[3] := 0;
    AlphaValues[4] := 0;
  end;

  //if not PtInRect(Bitmap.BoundsRect, Point(X1 + (Count + Offset + 2) div 3 - 1, DstY)) then
  //  Sleep(0);

  MakeAlpha[FFillMode](Span.Values, PByteArray(@AlphaValues[PADDING]), Count, FColor);
  CombineLineLCD(@AlphaValues[PADDING - Offset], PColor32Array(@Bitmap.ScanLine[DstY][X1]), FColor, (Count + Offset + 2) div 3);

  StackFree(AlphaValues);
end;

procedure TPolygonRenderer32.RenderSpanLCD2(const Span: TValueSpan;
  DstY: Integer);
const
  PADDING = 5;
var
  AlphaValues: SysUtils.PByteArray;
  Count: Integer;
  X1, Offset: Integer;
const
  MakeAlpha: array [TPolyFillMode] of TMakeAlphaProcLCD = (MakeAlphaEvenOddLCD2, MakeAlphaNonZeroLCD2);
begin
  Count := Span.X2 - Span.X1 + 1;
  X1 := DivMod(Span.X1, 3, Offset);

  // Left Padding + Right Padding + Filter Width = 2 + 2 + 2 = 6
  AlphaValues := StackAlloc((Count + 6 + PADDING) * SizeOf(Byte));
  AlphaValues[0] := 0;
  AlphaValues[1] := 0;
  if (X1 > 0) then
  begin
    Dec(X1);
    Inc(Offset, 3);
    AlphaValues[2] := 0;
    AlphaValues[3] := 0;
    AlphaValues[4] := 0;
  end;

  Dec(Offset, 1);
  MakeAlpha[FFillMode](Span.Values, PByteArray(@AlphaValues[PADDING]), Count, FColor);
  Inc(Count);
  CombineLineLCD(@AlphaValues[PADDING - Offset], PColor32Array(@Bitmap.ScanLine[DstY][X1]), FColor, (Count + Offset + 2) div 3);

  StackFree(AlphaValues);
end;


{$O+}

procedure TPolygonRenderer32.SetColor(const Value: TColor32);
begin
  FColor := Value;
end;

procedure TPolygonRenderer32.SetFiller(const Value: TCustomPolygonFiller);
begin
  if FFiller <> Value then
  begin
    FFiller := Value;
    UpdateFillProcs;
  end;
end;

procedure TPolygonRenderer32.SetFillMode(const Value: TPolyFillMode);
begin
  if FFillMode <> Value then
  begin
    FFillMode := Value;
    UpdateFillProcs;
  end;
end;

procedure TPolygonRenderer32.UpdateFillProcs;
const
  FillProcsPacked: array [TPolyFillMode] of TPackedFillProc =
    (MakeAlphaEvenOddP, MakeAlphaNonZeroP);
  FillProcsUnpacked: array [TPolyFillMode] of TUnpackedFillProc =
    (MakeAlphaEvenOddUP, MakeAlphaNonZeroUP);
  FillProcsPackedF: array [TPolyFillMode] of TPackedFillProc =
    (MakeAlphaEvenOddPF, MakeAlphaNonZeroPF);
  FillProcsUnpackedF: array [TPolyFillMode] of TUnpackedFillProc =
    (MakeAlphaEvenOddUPF, MakeAlphaNonZeroUPF);
begin
  if Assigned(FFiller) then
  begin
    FFillProcPacked := FillProcsPackedF[FillMode];
    FFillProcUnpacked := FillProcsUnpackedF[FillMode];
  end
  else
  begin
    FFillProcPacked := FillProcsPacked[FillMode];
    FFillProcUnpacked := FillProcsUnpacked[FillMode];
  end;
end;

{$IFDEF COMPILER2006}
initialization
  bias_ptr := GR32_Blend.bias_ptr;
{$ENDIF}

end.
