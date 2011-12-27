//----------------------------------------------------------------------------
// Anti-Grain Geometry - Version 2.4 (Public License)
// Copyright (C) 2002-2005 Maxim Shemanarev (http://www.antigrain.com)
//
// Anti-Grain Geometry - Version 2.4 Release Milano 3 (AggPas 2.4 RM3)
// Pascal Port By: Milan Marusinec alias Milano
//                 milan@marusinec.sk
//                 http://www.aggpas.org
// Copyright (c) 2005-2006
//
// Permission to copy, use, modify, sell and distribute this software
// is granted provided this copyright notice appears in all copies.
// This software is provided "as is" without express or implied
// warranty, and with no claim as to its suitability for any purpose.
//
//----------------------------------------------------------------------------
// Contact: mcseem@antigrain.com
//          mcseemagg@yahoo.com
//          http://www.antigrain.com
//----------------------------------------------------------------------------

unit
 agg_conv_clipper ;

interface

{$I agg_mode.inc }

uses
 clipper ,
 agg_basics ,
 agg_array ,
 agg_vertex_source ;

type
  clipper_op_e = (
    clipper_or ,
    clipper_and ,
    clipper_xor ,
    clipper_a_minus_b ,
    clipper_b_minus_a
  );

  clipper_polyFillType = (
    clipper_evenOdd,
    clipper_nonZero
  );

  status = (status_move_to, status_line_to, status_stop );

 conv_clipper_ptr = ^conv_clipper;
 conv_clipper = object(vertex_source )
   m_src_a ,
   m_src_b : vertex_source_ptr;

   m_status    : status;
   m_vertex    ,
   m_contour   : int;
   m_operation : clipper_op_e;

   m_subjFillType,
   m_clipFillType: clipper_polyFillType;

   m_poly_a ,
   m_poly_b ,
   m_result : TArrayOfArrayOfDoublePoint;

   m_vertex_accumulator: pod_deque;
   clipper: TClipper;

   constructor Construct(a, b : vertex_source_ptr;
     op : clipper_op_e = clipper_or;
     subjFillType: clipper_polyFillType = clipper_evenOdd;
     clipFillType: clipper_polyFillType = clipper_evenOdd);

   destructor  Destruct; virtual;

   procedure set_source1(source : vertex_source_ptr;
     subjFillType: clipper_polyFillType = clipper_evenOdd);
   procedure set_source2(source : vertex_source_ptr;
     clipFillType: clipper_polyFillType = clipper_evenOdd);

   procedure operation(v : clipper_op_e );

  // Vertex Source Interface
   procedure rewind(path_id : unsigned ); virtual;
   function  vertex(x ,y : double_ptr ) : unsigned; virtual;

   function  next_contour : boolean;
   function  next_vertex(x ,y : double_ptr ) : boolean;
   procedure start_extracting;
   procedure start_contour;
   procedure add_vertex_ (x,y: double );
   procedure end_contour(var p: TArrayOfArrayOfDoublePoint);
   procedure add(src : vertex_source_ptr; var p: TArrayOfArrayOfDoublePoint);
  end;

implementation

function pft(cpft: clipper_polyFillType): TPolyFillType;
begin
  if cpft = clipper_evenOdd then
    result := pftEvenOdd else
    result := pftNonZero;
end;

constructor conv_clipper.Construct(a, b: vertex_source_ptr;
     op: clipper_op_e = clipper_or;
     subjFillType: clipper_polyFillType = clipper_evenOdd;
     clipFillType: clipper_polyFillType = clipper_evenOdd);
begin
  m_src_a := a;
  m_src_b := b;
  m_operation := op;

  m_status   := status_move_to;
  m_vertex   := -1;
  m_contour  := -1;

  m_poly_a := nil;
  m_poly_b := nil;
  m_result := nil;
  m_vertex_accumulator.Construct (sizeof(TDoublePoint), 8 );

  m_subjFillType := subjFillType;
  m_clipFillType := clipFillType;
  clipper := TClipper.Create;
  clipper.ForceOrientation := true;

end;
//------------------------------------------------------------------------------

destructor conv_clipper.Destruct;
begin
  clipper.Free;
  m_vertex_accumulator.Destruct;
end;
//------------------------------------------------------------------------------

procedure conv_clipper.set_source1(source : vertex_source_ptr;
  subjFillType: clipper_polyFillType = clipper_evenOdd);
begin
  m_src_a := source;
  m_subjFillType := subjFillType;
end;
//------------------------------------------------------------------------------

procedure conv_clipper.set_source2(source : vertex_source_ptr;
  clipFillType: clipper_polyFillType = clipper_evenOdd);
begin
  m_src_b := source;
  m_clipFillType := clipFillType;
end;
//------------------------------------------------------------------------------

procedure conv_clipper.operation(v : clipper_op_e );
begin
  m_operation := v;
end;
//------------------------------------------------------------------------------

procedure conv_clipper.start_extracting;
begin
  m_status  := status_move_to;
  m_contour := -1;
  m_vertex  := -1;
end;
//------------------------------------------------------------------------------

procedure conv_clipper.rewind(path_id : unsigned );
begin
  m_src_a.rewind(path_id );
  m_src_b.rewind(path_id );

  add(m_src_a, m_poly_a );
  add(m_src_b, m_poly_b );
  m_result := nil;

  with clipper do
  begin
    clear;
    case m_operation of
      clipper_or :
       begin
         AddPolyPolygon(m_poly_a, ptSubject);
         AddPolyPolygon(m_poly_b, ptClip);
         Execute(ctUnion, m_result, pft(m_subjFillType), pft(m_clipFillType));
       end;
      clipper_and :
       begin
         AddPolyPolygon(m_poly_a, ptSubject);
         AddPolyPolygon(m_poly_b, ptClip);
         Execute(ctIntersection, m_result, pft(m_subjFillType), pft(m_clipFillType));
       end;
      clipper_xor :
       begin
         AddPolyPolygon(m_poly_a, ptSubject);
         AddPolyPolygon(m_poly_b, ptClip);
         Execute(ctXor, m_result, pft(m_subjFillType), pft(m_clipFillType));
       end;
      clipper_a_minus_b :
       begin
         AddPolyPolygon(m_poly_a, ptSubject);
         AddPolyPolygon(m_poly_b, ptClip);
         Execute(ctDifference, m_result, pft(m_subjFillType), pft(m_clipFillType));
       end;
      clipper_b_minus_a :
       begin
         AddPolyPolygon(m_poly_b, ptSubject);
         AddPolyPolygon(m_poly_a, ptClip);
         Execute(ctDifference, m_result, pft(m_subjFillType), pft(m_clipFillType));
       end;
    end;
  end;
  start_extracting;
end;
//------------------------------------------------------------------------------

procedure conv_clipper.start_contour;
begin
  m_vertex_accumulator.remove_all;
end;
//------------------------------------------------------------------------------

procedure conv_clipper.end_contour(var p: TArrayOfArrayOfDoublePoint);
var
  i, len: integer;
begin
  if m_vertex_accumulator.size < 3 then exit;
  len := length(p);
  setLength(p, len+1);
  setLength(p[len], m_vertex_accumulator.size);
  for i := 0 to m_vertex_accumulator.size -1 do
    p[len][i] := PDoublePoint(m_vertex_accumulator.array_operator(i))^;
end;
//------------------------------------------------------------------------------

procedure conv_clipper.add_vertex_ (x,y: double );
var
 v : TDoublePoint;
begin
  v.x := x;
  v.y := y;
  m_vertex_accumulator.add(@v );
end;
//------------------------------------------------------------------------------

function conv_clipper.next_contour;
begin
  result:=false;
  inc(m_contour );
  if m_contour >= length(m_result) then exit;
  m_vertex:=-1;
  result:=true;
end;
//------------------------------------------------------------------------------

function conv_clipper.next_vertex;
begin
  result:=false;
  inc(m_vertex);
  if m_vertex >= length(m_result[m_contour]) then exit;
  x^ := m_result[m_contour][m_vertex].X;
  y^ := m_result[m_contour][m_vertex].Y;
  result := true;
end;
//------------------------------------------------------------------------------

function conv_clipper.vertex(x ,y : double_ptr ) : unsigned;
begin
  if m_status = status_move_to then
  begin
    if next_contour then
    begin
      if next_vertex(x ,y ) then
      begin
        m_status:=status_line_to;
        result := path_cmd_move_to;
      end else
      begin
        m_status := status_stop;
        result := path_cmd_end_poly or path_flags_close;
      end;
    end else
      result := path_cmd_stop;
  end else
  begin
    if next_vertex(x ,y ) then
    begin
      result := path_cmd_line_to;
    end else
    begin
      m_status := status_move_to;
      result := path_cmd_end_poly or path_flags_close;
    end;
  end;
end;
//------------------------------------------------------------------------------

procedure conv_clipper.add(src : vertex_source_ptr; var p: TArrayOfArrayOfDoublePoint);
var
  cmd: unsigned;
  x, y, start_x ,start_y: double;
  starting_first_line : boolean;
begin
  start_x := 0.0;
  start_y := 0.0;
  starting_first_line := true;
  p := nil;

  cmd := src.vertex(@x, @y );
  while not is_stop(cmd) do
  begin
    if is_vertex(cmd) then
    begin
      if is_move_to(cmd ) then
      begin
        if not starting_first_line then end_contour(p);
        start_contour;
        start_x := x;
        start_y := y;
      end;
      add_vertex_(x ,y );
      starting_first_line := false;
    end
    else if is_end_poly(cmd ) then
    begin
      if not starting_first_line and is_closed(cmd ) then
        add_vertex_(start_x ,start_y );
    end;
    cmd := src.vertex(@x ,@y );
  end;
  end_contour(p);
end;
//------------------------------------------------------------------------------

end.

