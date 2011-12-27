/*******************************************************************************
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
* Pages 98 - 106.                                                              *
* http://books.google.com/books?q=vatti+clipping+agoston                       *
*                                                                              *
*******************************************************************************/

using System;
using System.Collections.Generic;
using Clipper;

namespace Clipper
{

    using TPolygon = List<TDoublePoint>;
    using TPolyPolygon = List<List<TDoublePoint>>;

    public class TClipperMisc
    {

        public static double Area(TPolygon poly)
        {
            int highI = poly.Count - 1;
            if (highI < 2) return 0;
            double result = 0;
            for (int i = 0; i < highI; ++i)
                result += (poly[i].X + poly[i + 1].X) * (poly[i].Y - poly[i + 1].Y);
            result += (poly[highI].X + poly[0].X) * (poly[highI].Y - poly[0].Y);
            result = result / 2;
            return result;
        }
        //------------------------------------------------------------------------------

        private static TPolygon BuildArc(TDoublePoint pt, double a1, double a2, double r)
        {
            int steps = (int)Math.Max(6, Math.Sqrt(Math.Abs(r)) * Math.Abs(a2 - a1));
            TPolygon result = new TPolygon();
            result.Capacity = steps;
            int n = steps - 1;
            double da = (a2 - a1) / n;
            double a = a1;
            for (int i = 0; i <= n; ++i)
            {
                double dy = Math.Sin(a) * r;
                double dx = Math.Cos(a) * r;
                result.Add(new TDoublePoint(pt.X + dx, pt.Y + dy));
                a = a + da;
            }
            return result;
        }
        //------------------------------------------------------------------------------

        public static TPolyPolygon OffsetPolygons(TPolyPolygon pts, double delta)
        {

            //A positive delta will offset each polygon edge towards its left, so
            //polygons orientated clockwise (ie outer polygons) will expand but
            //inner polyons (holes) will shrink. Conversely, negative deltas will
            //offset polygon edges towards their right so outer polygons will shrink
            //and inner polygons will expand.

            double deltaSq = delta * delta;
            TPolyPolygon result = new TPolyPolygon(pts.Count);
            for (int j = 0; j < pts.Count; ++j)
            {

                int highI = pts[j].Count - 1;

                TPolygon pg = new TPolygon(highI * 2 + 2);
                result.Add(pg);

                //to minimize artefacts, strip out those polygons where
                //it's shrinking and where its area < Sqr(delta) ...
                double area = Area(pts[j]);
                if (delta < 0) { if (area > 0 && area < deltaSq) highI = 0; }
                else if (area < 0 && -area < deltaSq) highI = 0; //nb: a hole if area < 0

                if (highI < 2) continue;

                TPolygon normals = new TPolygon(highI + 1);

                normals.Add(TClipper.GetUnitNormal(pts[j][highI], pts[j][0]));
                for (int i = 1; i <= highI; ++i)
                    normals.Add(TClipper.GetUnitNormal(pts[j][i - 1], pts[j][i]));
                for (int i = 0; i < highI; ++i)
                {
                    pg.Add(new TDoublePoint(pts[j][i].X + delta * normals[i].X,
                          pts[j][i].Y + delta * normals[i].Y));
                    pg.Add(new TDoublePoint(pts[j][i].X + delta * normals[i + 1].X,
                          pts[j][i].Y + delta * normals[i + 1].Y));
                }
                pg.Add(new TDoublePoint(pts[j][highI].X + delta * normals[highI].X,
                        pts[j][highI].Y + delta * normals[highI].Y));
                pg.Add(new TDoublePoint(pts[j][highI].X + delta * normals[0].X,
                        pts[j][highI].Y + delta * normals[0].Y));

                //round off reflex angles (ie > 180 deg) unless it's almost flat (ie < 10deg angle) ...
                //cross product normals < 0 . reflex angle; dot product normals == 1 . no angle
                if ((normals[highI].X * normals[0].Y - normals[0].X * normals[highI].Y) * delta > 0 &&
                  (normals[0].X * normals[highI].X + normals[0].Y * normals[highI].Y) < 0.985)
                {
                    double a1 = Math.Atan2(normals[highI].Y, normals[highI].X);
                    double a2 = Math.Atan2(normals[0].Y, normals[0].X);
                    if (delta > 0 && a2 < a1) a2 = a2 + Math.PI * 2;
                    else if (delta < 0 && a2 > a1) a2 = a2 - Math.PI * 2;
                    TPolygon arc = BuildArc(pts[j][highI], a1, a2, delta);
                    pg.InsertRange(highI * 2 + 1, arc);
                }
                for (int i = highI; i > 0; --i)
                    if ((normals[i - 1].X * normals[i].Y - normals[i].X * normals[i - 1].Y) * delta > 0 &&
                    (normals[i].X * normals[i - 1].X + normals[i].Y * normals[i - 1].Y) < 0.985)
                    {
                        double a1 = Math.Atan2(normals[i - 1].Y, normals[i - 1].X);
                        double a2 = Math.Atan2(normals[i].Y, normals[i].X);
                        if (delta > 0 && a2 < a1) a2 = a2 + Math.PI * 2;
                        else if (delta < 0 && a2 > a1) a2 = a2 - Math.PI * 2;
                        TPolygon arc = BuildArc(pts[j][i - 1], a1, a2, delta);
                        pg.InsertRange((i - 1) * 2 + 1, arc);
                    }
            }

            //finally, clean up untidy corners ...
            TClipper c = new TClipper();
            c.AddPolyPolygon(result, TPolyType.ptSubject);
            if (delta > 0)
            {
                if (!c.Execute(TClipType.ctUnion, result,
                    TPolyFillType.pftNonZero, TPolyFillType.pftNonZero)) result.Clear();
            }
            else
            {
                TDoubleRect r = c.GetBounds();
                TPolygon outer = new TPolygon(4);
                outer.Add(new TDoublePoint(r.left - 10, r.bottom + 10));
                outer.Add(new TDoublePoint(r.right + 10, r.bottom + 10));
                outer.Add(new TDoublePoint(r.right + 10, r.top - 10));
                outer.Add(new TDoublePoint(r.left - 10, r.top - 10));
                c.AddPolygon(outer, TPolyType.ptSubject);
                if (c.Execute(TClipType.ctUnion, result, TPolyFillType.pftNonZero, TPolyFillType.pftNonZero))
                    result.RemoveAt(0);
                else
                    result.Clear();

            }
            return result;
        }
        //------------------------------------------------------------------------------


    }
}