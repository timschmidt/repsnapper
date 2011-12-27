/*******************************************************************************
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
*******************************************************************************/

/*******************************************************************************
*                                                                              *
* This is a translation of the Delphi Clipper library and the naming style     *
* used has retained a Delphi flavour.                                          *
*                                                                              *
*******************************************************************************/

using System;
using System.Collections.Generic;

namespace clipper
{
    using Polygon = List<IntPoint>;
    using Polygons = List<List<IntPoint>>;

    public class IntPoint
    {
        public int X { get; set; }
        public int Y { get; set; }
        public IntPoint(int X = 0, int Y = 0)
        {
            this.X = X; this.Y = Y;
        }
    }

    public class IntRect
    {
        public int left { get; set; }
        public int top { get; set; }
        public int right { get; set; }
        public int bottom { get; set; }
        public IntRect(int l = 0, int t = 0, int r = 0, int b = 0)
        {
            this.left = l; this.top = t;
            this.right = r; this.bottom = b;
        }
    }

    public enum ClipType { ctIntersection, ctUnion, ctDifference, ctXor };
    public enum PolyType { ptSubject, ptClip };
    public enum PolyFillType { pftEvenOdd, pftNonZero };


    internal enum EdgeSide { esLeft, esRight };
    internal enum Position { pFirst, pMiddle, pSecond };
    internal enum Direction { dRightToLeft, dLeftToRight };
    [Flags]
    internal enum Protects { ipNone = 0, ipLeft = 1, ipRight = 2, ipBoth = 3 };

    internal class TEdge4 {
        public int xbot;
        public int ybot;
        public int xcurr;
        public int ycurr;
        public int xtop;
        public int ytop;
        public double dx;
        public int tmpX;
        public PolyType polyType;
        public EdgeSide side;
        public int windDelta; //1 or -1 depending on winding direction
        public int windCnt;
        public int windCnt2; //winding count of the opposite polytype
        public int outIdx;
        public TEdge4 next;
        public TEdge4 prev;
        public TEdge4 nextInLML;
        public TEdge4 nextInAEL;
        public TEdge4 prevInAEL;
        public TEdge4 nextInSEL;
        public TEdge4 prevInSEL;
    };

    internal class IntersectNode
    {
        public TEdge4 edge1;
        public TEdge4 edge2;
        public IntPoint pt;
        public IntersectNode next;
    };

    internal class LocalMinima
    {
        public int Y;
        public TEdge4 leftBound;
        public TEdge4 rightBound;
        public LocalMinima next;
    };

    internal class Scanbeam
    {
        public int Y;
        public Scanbeam next;
    };

    internal class PolyPt
    {
        public IntPoint pt;
        public PolyPt next;
        public PolyPt prev;
        public bool isHole;
    };

    internal class JoinRec
    {
        public IntPoint pt1a;
        public IntPoint pt1b;
        public int poly1Idx;
        public IntPoint pt2a;
        public IntPoint pt2b;
        public int poly2Idx;
    };

    internal class HorzJoinRec
    {
        public TEdge4 edge;
        public int savedIdx;
    };

    public class ClipperBase
    {
        protected const double horizontal = -3.4E+38;
        internal LocalMinima m_MinimaList;
        internal LocalMinima m_CurrentLM;
        private List<List<TEdge4>> m_edges = new List<List<TEdge4>>();

        //------------------------------------------------------------------------------

        protected bool PointsEqual(IntPoint pt1, IntPoint pt2)
        {
          return ( pt1.X == pt2.X && pt1.Y == pt2.Y );
        }
        //------------------------------------------------------------------------------

        internal bool SlopesEqual(TEdge4 e1, TEdge4 e2)
        {
          if (e1.ybot == e1.ytop) return (e2.ybot == e2.ytop);
          else if (e2.ybot == e2.ytop) return false;
          else return (Int64)(e1.ytop - e1.ybot)*(e2.xtop - e2.xbot) -
              (Int64)(e1.xtop - e1.xbot)*(e2.ytop - e2.ybot) == 0;
        }
        //------------------------------------------------------------------------------

        protected bool SlopesEqual(IntPoint pt1, IntPoint pt2, IntPoint pt3)
        {
          if (pt1.Y == pt2.Y) return (pt2.Y == pt3.Y);
          else if (pt2.Y == pt3.Y) return false;
          else return
            (Int64)(pt1.Y-pt2.Y)*(pt2.X-pt3.X) - (Int64)(pt1.X-pt2.X)*(pt2.Y-pt3.Y) == 0;
        }
        //------------------------------------------------------------------------------

        internal ClipperBase() //constructor (nb: no external instantiation)
        {
            m_MinimaList = null;
            m_CurrentLM = null;
        }
        //------------------------------------------------------------------------------

        ~ClipperBase() //destructor
        {
            Clear();
        }
        //------------------------------------------------------------------------------

        public virtual void Clear()
        {
            DisposeLocalMinimaList();
            for (int i = 0; i < m_edges.Count; ++i)
            {
                for (int j = 0; j < m_edges[i].Count; ++j) m_edges[i][j] = null;
                m_edges[i].Clear();
            }
            m_edges.Clear();
        }
        //------------------------------------------------------------------------------

        private void DisposeLocalMinimaList()
        {
            while( m_MinimaList != null )
            {
                LocalMinima tmpLm = m_MinimaList.next;
                m_MinimaList = null;
                m_MinimaList = tmpLm;
            }
            m_CurrentLM = null;
        }
        //------------------------------------------------------------------------------

        public void AddPolygons(Polygons ppg, PolyType polyType)
        {
            for (int i = 0; i < ppg.Count; ++i)
                AddPolygon(ppg[i], polyType);
        }
        //------------------------------------------------------------------------------

        public void AddPolygon(Polygon pg, PolyType polyType)
        {
            int len = pg.Count;
            if (len < 3) return;
            Polygon p = new Polygon(len);
            p.Add(new IntPoint(pg[0].X, pg[0].Y));
            int j = 0;
            for (int i = 1; i < len; ++i)
            {
                if (PointsEqual(p[j], pg[i])) continue;
                else if (j > 0 && SlopesEqual(p[j-1], p[j], pg[i]))
                {
                    if (PointsEqual(p[j-1], pg[i])) j--;
                } else j++;
                if (j < p.Count)
                    p[j] = pg[i]; else
                    p.Add(new IntPoint(pg[i].X, pg[i].Y));
            }
            if (j < 2) return;

            len = j+1;
            for (;;)
            {
            //nb: test for point equality before testing slopes ...
            if (PointsEqual(p[j], p[0])) j--;
            else if (PointsEqual(p[0], p[1]) || SlopesEqual(p[j], p[0], p[1]))
                p[0] = p[j--];
            else if (SlopesEqual(p[j-1], p[j], p[0])) j--;
            else if (SlopesEqual(p[0], p[1], p[2]))
            {
                for (int i = 2; i <= j; ++i) p[i-1] = p[i];
                j--;
            }
            //exit loop if nothing is changed or there are too few vertices ...
            if (j == len-1 || j < 2) break;
            len = j +1;
            }
            if (len < 3) return;

            //create a new edge array ...
            List<TEdge4> edges = new List<TEdge4>(len);
            for (int i = 0; i < len; i++) edges.Add(new TEdge4());
            m_edges.Add(edges);

            //convert vertices to a double-linked-list of edges and initialize ...
            edges[0].xcurr = p[0].X;
            edges[0].ycurr = p[0].Y;
            InitEdge(edges[len-1], edges[0], edges[len-2], p[len-1], polyType);
            for (int i = len-2; i > 0; --i)
            InitEdge(edges[i], edges[i+1], edges[i-1], p[i], polyType);
            InitEdge(edges[0], edges[1], edges[len-1], p[0], polyType);

            //reset xcurr & ycurr and find 'eHighest' (given the Y axis coordinates
            //increase downward so the 'highest' edge will have the smallest ytop) ...
            TEdge4 e = edges[0];
            TEdge4 eHighest = e;
            do
            {
            e.xcurr = e.xbot;
            e.ycurr = e.ybot;
            if (e.ytop < eHighest.ytop) eHighest = e;
            e = e.next;
            }
            while ( e != edges[0]);

            //make sure eHighest is positioned so the following loop works safely ...
            if (eHighest.windDelta > 0) eHighest = eHighest.next;
            if (eHighest.dx == horizontal) eHighest = eHighest.next;

            //finally insert each local minima ...
            e = eHighest;
            do {
            e = AddBoundsToLML(e);
            }
            while( e != eHighest );

        }
        //------------------------------------------------------------------------------

        private void InitEdge(TEdge4 e, TEdge4 eNext,
          TEdge4 ePrev, IntPoint pt, PolyType polyType)
        {
          e.next = eNext;
          e.prev = ePrev;
          e.xcurr = pt.X;
          e.ycurr = pt.Y;
          if (e.ycurr >= e.next.ycurr)
          {
            e.xbot = e.xcurr;
            e.ybot = e.ycurr;
            e.xtop = e.next.xcurr;
            e.ytop = e.next.ycurr;
            e.windDelta = 1;
          } else
          {
            e.xtop = e.xcurr;
            e.ytop = e.ycurr;
            e.xbot = e.next.xcurr;
            e.ybot = e.next.ycurr;
            e.windDelta = -1;
          }
          SetDx(e);
          e.polyType = polyType;
          e.outIdx = -1;
        }
        //------------------------------------------------------------------------------

        private void SetDx(TEdge4 e)
        {
          if (e.ybot == e.ytop) e.dx = horizontal;
          else e.dx = (double)(e.xtop - e.xbot)/(e.ytop - e.ybot);
        }
        //---------------------------------------------------------------------------

        TEdge4 AddBoundsToLML(TEdge4 e)
        {
          //Starting at the top of one bound we progress to the bottom where there's
          //a local minima. We then go to the top of the next bound. These two bounds
          //form the left and right (or right and left) bounds of the local minima.
          e.nextInLML = null;
          e = e.next;
          for (;;)
          {
            if ( e.dx == horizontal )
            {
              //nb: proceed through horizontals when approaching from their right,
              //    but break on horizontal minima if approaching from their left.
              //    This ensures 'local minima' are always on the left of horizontals.
              if (e.next.ytop < e.ytop && e.next.xbot > e.prev.xbot) break;
              if (e.xtop != e.prev.xbot) SwapX(e);
              e.nextInLML = e.prev;
            }
            else if (e.ycurr == e.prev.ycurr) break;
            else e.nextInLML = e.prev;
            e = e.next;
          }

          //e and e.prev are now at a local minima ...
          LocalMinima newLm = new LocalMinima();
          newLm.next = null;
          newLm.Y = e.prev.ybot;

          if ( e.dx == horizontal ) //horizontal edges never start a left bound
          {
            if (e.xbot != e.prev.xbot) SwapX(e);
            newLm.leftBound = e.prev;
            newLm.rightBound = e;
          } else if (e.dx < e.prev.dx)
          {
            newLm.leftBound = e.prev;
            newLm.rightBound = e;
          } else
          {
            newLm.leftBound = e;
            newLm.rightBound = e.prev;
          }
          newLm.leftBound.side = EdgeSide.esLeft;
          newLm.rightBound.side = EdgeSide.esRight;
          InsertLocalMinima( newLm );

          for (;;)
          {
            if ( e.next.ytop == e.ytop && e.next.dx != horizontal ) break;
            e.nextInLML = e.next;
            e = e.next;
            if ( e.dx == horizontal && e.xbot != e.prev.xtop) SwapX(e);
          }
          return e.next;
        }
        //------------------------------------------------------------------------------

        private void InsertLocalMinima(LocalMinima newLm)
        {
          if( m_MinimaList == null )
          {
            m_MinimaList = newLm;
          }
          else if( newLm.Y >= m_MinimaList.Y )
          {
            newLm.next = m_MinimaList;
            m_MinimaList = newLm;
          } else
          {
            LocalMinima tmpLm = m_MinimaList;
            while( tmpLm.next != null  && ( newLm.Y < tmpLm.next.Y ) )
              tmpLm = tmpLm.next;
            newLm.next = tmpLm.next;
            tmpLm.next = newLm;
          }
        }
        //------------------------------------------------------------------------------

        protected void PopLocalMinima()
        {
            if (m_CurrentLM == null) return;
            m_CurrentLM = m_CurrentLM.next;
        }
        //------------------------------------------------------------------------------

        private void SwapX(TEdge4 e)
        {
          //swap horizontal edges' top and bottom x's so they follow the natural
          //progression of the bounds - ie so their xbots will align with the
          //adjoining lower edge. [Helpful in the ProcessHorizontal() method.]
          e.xcurr = e.xtop;
          e.xtop = e.xbot;
          e.xbot = e.xcurr;
        }
        //------------------------------------------------------------------------------

        protected virtual bool Reset()
        {
            m_CurrentLM = m_MinimaList;
            if ( m_CurrentLM == null ) return false; //ie nothing to process

            //reset all edges ...
            LocalMinima lm = m_MinimaList;
            while (lm != null)
            {
                TEdge4 e = lm.leftBound;
                while (e != null)
                {
                    e.xcurr = e.xbot;
                    e.ycurr = e.ybot;
                    e.side = EdgeSide.esLeft;
                    e.outIdx = -1;
                    e = e.nextInLML;
                }
                e = lm.rightBound;
                while (e != null)
                {
                    e.xcurr = e.xbot;
                    e.ycurr = e.ybot;
                    e.side = EdgeSide.esRight;
                    e.outIdx = -1;
                    e = e.nextInLML;
                }
                lm = lm.next;
            }
            return true;
        }
        //------------------------------------------------------------------------------

        public IntRect GetBounds()
        {
          IntRect result = new IntRect();
          LocalMinima lm = m_MinimaList;
          if (lm == null) return result;
          result.left = lm.leftBound.xbot;
          result.top = lm.leftBound.ybot;
          result.right = lm.leftBound.xbot;
          result.bottom = lm.leftBound.ybot;
          while (lm != null)
          {
            if (lm.leftBound.ybot > result.bottom)
              result.bottom = lm.leftBound.ybot;
            TEdge4 e = lm.leftBound;
            for (;;) {
              while (e.nextInLML != null)
              {
                if (e.xbot < result.left) result.left = e.xbot;
                if (e.xbot > result.right) result.right = e.xbot;
                e = e.nextInLML;
              }
              if (e.xbot < result.left) result.left = e.xbot;
              if (e.xbot > result.right) result.right = e.xbot;
              if (e.xtop < result.left) result.left = e.xtop;
              if (e.xtop > result.right) result.right = e.xtop;
              if (e.ytop < result.top) result.top = e.ytop;

              if (e == lm.leftBound) e = lm.rightBound;
              else break;
            }
            lm = lm.next;
          }
          return result;
        }

    } //ClipperBase

    public class Clipper : ClipperBase
    {
    
        private List<PolyPt> m_PolyPts;
        private ClipType m_ClipType;
        private Scanbeam m_Scanbeam;
        private TEdge4 m_ActiveEdges;
        private TEdge4 m_SortedEdges;
        private IntersectNode m_IntersectNodes;
        private bool m_ExecuteLocked;
        private PolyFillType m_ClipFillType;
        private PolyFillType m_SubjFillType;
        private List<JoinRec> m_Joins;
        private List<HorzJoinRec> m_HorizJoins;

        public Clipper()
        {
            m_Scanbeam = null;
            m_ActiveEdges = null;
            m_SortedEdges = null;
            m_IntersectNodes = null;
            m_ExecuteLocked = false;
            m_PolyPts = new List<PolyPt>();
            m_Joins = new List<JoinRec>();
            m_HorizJoins = new List<HorzJoinRec>();
        }
        //------------------------------------------------------------------------------

        ~Clipper() //destructor
        {
            DisposeScanbeamList();
        }

        void DisposeScanbeamList()
        {
          while ( m_Scanbeam != null ) {
          Scanbeam sb2 = m_Scanbeam.next;
          m_Scanbeam = null;
          m_Scanbeam = sb2;
          }
        }
        //------------------------------------------------------------------------------

        protected override bool Reset() 
        {
          if ( !base.Reset() ) return false;
          m_Scanbeam = null;
          m_ActiveEdges = null;
          m_SortedEdges = null;
          LocalMinima lm = m_MinimaList;
          while (lm != null)
          {
            InsertScanbeam(lm.Y);
            InsertScanbeam(lm.leftBound.ytop);
            lm = lm.next;
          }
          return true;
        }
        //------------------------------------------------------------------------------

        private void InsertScanbeam(int Y)
        {
          if( m_Scanbeam == null )
          {
            m_Scanbeam = new Scanbeam();
            m_Scanbeam.next = null;
            m_Scanbeam.Y = Y;
          }
          else if(  Y > m_Scanbeam.Y )
          {
            Scanbeam newSb = new Scanbeam();
            newSb.Y = Y;
            newSb.next = m_Scanbeam;
            m_Scanbeam = newSb;
          } else
          {
            Scanbeam sb2 = m_Scanbeam;
            while( sb2.next != null  && ( Y <= sb2.next.Y ) ) sb2 = sb2.next;
            if(  Y == sb2.Y ) return; //ie ignores duplicates
            Scanbeam newSb = new Scanbeam();
            newSb.Y = Y;
            newSb.next = sb2.next;
            sb2.next = newSb;
          }
        }
        //------------------------------------------------------------------------------

        public bool Execute(ClipType clipType, Polygons solution,
            PolyFillType subjFillType, PolyFillType clipFillType)
        {
          if( m_ExecuteLocked ) return false;
          bool succeeded;
          try {
            m_ExecuteLocked = true;
            solution.Clear();
            if ( !Reset() )
            {
              m_ExecuteLocked = false;
              return false;
            }
            m_SubjFillType = subjFillType;
            m_ClipFillType = clipFillType;
            m_ClipType = clipType;

            int botY = PopScanbeam();
            do {
              InsertLocalMinimaIntoAEL(botY);
              m_HorizJoins.Clear();
              ProcessHorizontals();
              int topY = PopScanbeam();
              succeeded = ProcessIntersections(topY);
              if (succeeded) ProcessEdgesAtTopOfScanbeam(topY);
              botY = topY;
            } while( succeeded && m_Scanbeam != null );

            //build the return polygons ...
            if (succeeded) BuildResult(solution);
          }
          catch {
              m_Joins.Clear();
              m_HorizJoins.Clear();
              solution.Clear();
              succeeded = false;
          }
          m_Joins.Clear();
          m_HorizJoins.Clear();
          DisposeAllPolyPts();
          m_ExecuteLocked = false;
          return succeeded;
        }
        //------------------------------------------------------------------------------

        private int PopScanbeam()
        {
          int Y = m_Scanbeam.Y;
          Scanbeam sb2 = m_Scanbeam;
          m_Scanbeam = m_Scanbeam.next;
          sb2 = null;
          return Y;
        }
        //------------------------------------------------------------------------------

        private void DisposeAllPolyPts(){
          for (int i = 0; i < m_PolyPts.Count; ++i)
            DisposePolyPts(m_PolyPts[i]);
          m_PolyPts.Clear();
        }
        //------------------------------------------------------------------------------

        private void DisposePolyPts(PolyPt pp)
        {
            if (pp == null) return;
            PolyPt tmpPp = null;
            pp.prev.next = null;
            while (pp != null)
            {
                tmpPp = pp;
                pp = pp.next;
                tmpPp = null;
            }
        }
        //------------------------------------------------------------------------------

        private void AddJoin(TEdge4 e1, TEdge4 e2, int e1OutIdx = -1)
        {
            JoinRec jr = new JoinRec();
            if (e1OutIdx >= 0)
                jr.poly1Idx = e1OutIdx; else
            jr.poly1Idx = e1.outIdx;
            jr.pt1a = new IntPoint(e1.xbot, e1.ybot);
            jr.pt1b = new IntPoint(e1.xtop, e1.ytop);
            jr.poly2Idx = e2.outIdx;
            jr.pt2a = new IntPoint(e2.xbot, e2.ybot);
            jr.pt2b = new IntPoint(e2.xtop, e2.ytop);
            m_Joins.Add(jr);
        }
        //------------------------------------------------------------------------------

        private void AddHorzJoin(TEdge4 e, int idx)
        {
            HorzJoinRec hj = new HorzJoinRec();
            hj.edge = e;
            hj.savedIdx = idx;
            m_HorizJoins.Add(hj);
        }
        //------------------------------------------------------------------------------

        private void InsertLocalMinimaIntoAEL(int botY)
        {
          while(  m_CurrentLM != null  && ( m_CurrentLM.Y == botY ) )
          {
            TEdge4 lb = m_CurrentLM.leftBound;
            TEdge4 rb = m_CurrentLM.rightBound;

            InsertEdgeIntoAEL( lb );
            InsertScanbeam( lb.ytop );
            InsertEdgeIntoAEL( rb );

            if ( IsNonZeroFillType( lb) )
              rb.windDelta = -lb.windDelta;
            else
            {
              lb.windDelta = 1;
              rb.windDelta = 1;
            }
            SetWindingCount( lb );
            rb.windCnt = lb.windCnt;
            rb.windCnt2 = lb.windCnt2;

            if(  rb.dx == horizontal )
            {
              //nb: only rightbounds can have a horizontal bottom edge
              AddEdgeToSEL( rb );
              InsertScanbeam( rb.nextInLML.ytop );
            }
            else
              InsertScanbeam( rb.ytop );

            if( IsContributing(lb) )
                AddLocalMinPoly(lb, rb, new IntPoint(lb.xcurr, m_CurrentLM.Y));


            //if output polygons share an edge, they'll need joining later ...
            if (lb.outIdx >= 0 && lb.prevInAEL != null &&
              lb.prevInAEL.outIdx >= 0 && lb.prevInAEL.xcurr == lb.xbot &&
               SlopesEqual(lb, lb.prevInAEL))
                AddJoin(lb, lb.prevInAEL);

            //if any output polygons share an edge, they'll need joining later ...
            if (rb.outIdx >= 0)
            {
                if (rb.dx == horizontal)
                {
                    for (int i = 0; i < m_HorizJoins.Count; i++)
                    {
                        IntPoint pt = new IntPoint(), pt2 = new IntPoint(); //used as dummy params.
                        HorzJoinRec hj = m_HorizJoins[i];
                        //if horizontals rb and hj.edge overlap, flag for joining later ...
                        if (GetOverlapSegment(new IntPoint(hj.edge.xbot, hj.edge.ybot),
                            new IntPoint(hj.edge.xtop, hj.edge.ytop),
                            new IntPoint(rb.xbot, rb.ybot),
                            new IntPoint(rb.xtop, rb.ytop), 
                            ref pt, ref pt2))
                                AddJoin(hj.edge, rb, hj.savedIdx);
                    }
                }
            }


            if( lb.nextInAEL != rb )
            {
              TEdge4 e = lb.nextInAEL;
              IntPoint pt = new IntPoint(lb.xcurr, lb.ycurr);
              while( e != rb )
              {
                if(e == null) 
                    throw new ClipperException("InsertLocalMinimaIntoAEL: missing rightbound!");
                //nb: For calculating winding counts etc, IntersectEdges() assumes
                //that param1 will be to the right of param2 ABOVE the intersection ...
                IntersectEdges( rb , e , pt , Protects.ipNone); //order important here
                e = e.nextInAEL;
              }
            }
            PopLocalMinima();
          }
        }
        //------------------------------------------------------------------------------

        private void InsertEdgeIntoAEL(TEdge4 edge)
        {
          edge.prevInAEL = null;
          edge.nextInAEL = null;
          if (m_ActiveEdges == null)
          {
            m_ActiveEdges = edge;
          }
          else if( E2InsertsBeforeE1(m_ActiveEdges, edge) )
          {
            edge.nextInAEL = m_ActiveEdges;
            m_ActiveEdges.prevInAEL = edge;
            m_ActiveEdges = edge;
          } else
          {
            TEdge4 e = m_ActiveEdges;
            while (e.nextInAEL != null && !E2InsertsBeforeE1(e.nextInAEL, edge))
              e = e.nextInAEL;
            edge.nextInAEL = e.nextInAEL;
            if (e.nextInAEL != null) e.nextInAEL.prevInAEL = edge;
            edge.prevInAEL = e;
            e.nextInAEL = edge;
          }
        }
        //----------------------------------------------------------------------

        private bool E2InsertsBeforeE1(TEdge4 e1, TEdge4 e2)
        {
          if (e2.xcurr == e1.xcurr) return e2.dx > e1.dx;
          else return e2.xcurr < e1.xcurr;
        }
        //------------------------------------------------------------------------------

        private bool IsNonZeroFillType(TEdge4 edge) 
        {
          if (edge.polyType == PolyType.ptSubject)
            return m_SubjFillType == PolyFillType.pftNonZero; else
            return m_ClipFillType == PolyFillType.pftNonZero;
        }
        //------------------------------------------------------------------------------

        private bool IsNonZeroAltFillType(TEdge4 edge) 
        {
          if (edge.polyType == PolyType.ptSubject)
            return m_ClipFillType == PolyFillType.pftNonZero; else
            return m_SubjFillType == PolyFillType.pftNonZero;
        }
        //------------------------------------------------------------------------------

        private bool IsContributing(TEdge4 edge)
        {
            switch (m_ClipType)
            {
                case ClipType.ctIntersection:
                    if (edge.polyType == PolyType.ptSubject)
                        return Math.Abs(edge.windCnt) == 1 && edge.windCnt2 != 0;
                    else
                        return Math.Abs(edge.windCnt2) > 0 && Math.Abs(edge.windCnt) == 1;
                case ClipType.ctUnion:
                    return Math.Abs(edge.windCnt) == 1 && edge.windCnt2 == 0;
                case ClipType.ctDifference:
                    if (edge.polyType == PolyType.ptSubject)
                        return Math.Abs(edge.windCnt) == 1 && edge.windCnt2 == 0;
                    else
                        return Math.Abs(edge.windCnt) == 1 && edge.windCnt2 != 0;
                default: //case ctXor:
                    return Math.Abs(edge.windCnt) == 1;
            }
        }
        //------------------------------------------------------------------------------

        private void SetWindingCount(TEdge4 edge)
        {
            TEdge4 e = edge.prevInAEL;
            //find the edge of the same polytype that immediately preceeds 'edge' in AEL
            while (e != null && e.polyType != edge.polyType)
                e = e.prevInAEL;
            if (e == null)
            {
                edge.windCnt = edge.windDelta;
                edge.windCnt2 = 0;
                e = m_ActiveEdges; //ie get ready to calc windCnt2
            }
            else if (IsNonZeroFillType(edge))
            {
                //nonZero filling ...
                if (e.windCnt * e.windDelta < 0)
                {
                    if (Math.Abs(e.windCnt) > 1)
                    {
                        if (e.windDelta * edge.windDelta < 0)
                            edge.windCnt = e.windCnt;
                        else
                            edge.windCnt = e.windCnt + edge.windDelta;
                    }
                    else
                        edge.windCnt = e.windCnt + e.windDelta + edge.windDelta;
                }
                else
                {
                    if (Math.Abs(e.windCnt) > 1 && e.windDelta * edge.windDelta < 0)
                        edge.windCnt = e.windCnt;
                    else if (e.windCnt + edge.windDelta == 0)
                        edge.windCnt = e.windCnt;
                    else
                        edge.windCnt = e.windCnt + edge.windDelta;
                }
                edge.windCnt2 = e.windCnt2;
                e = e.nextInAEL; //ie get ready to calc windCnt2
            }
            else
            {
                //even-odd filling ...
                edge.windCnt = 1;
                edge.windCnt2 = e.windCnt2;
                e = e.nextInAEL; //ie get ready to calc windCnt2
            }

            //update windCnt2 ...
            if (IsNonZeroAltFillType(edge))
            {
                //nonZero filling ...
                while (e != edge)
                {
                    edge.windCnt2 += e.windDelta;
                    e = e.nextInAEL;
                }
            }
            else
            {
                //even-odd filling ...
                while (e != edge)
                {
                    edge.windCnt2 = (edge.windCnt2 == 0) ? 1 : 0;
                    e = e.nextInAEL;
                }
            }
        }
        //------------------------------------------------------------------------------

        private void AddEdgeToSEL(TEdge4 edge)
        {
            //SEL pointers in PEdge are reused to build a list of horizontal edges.
            //However, we don't need to worry about order with horizontal edge processing.
            if (m_SortedEdges == null)
            {
                m_SortedEdges = edge;
                edge.prevInSEL = null;
                edge.nextInSEL = null;
            }
            else
            {
                edge.nextInSEL = m_SortedEdges;
                edge.prevInSEL = null;
                m_SortedEdges.prevInSEL = edge;
                m_SortedEdges = edge;
            }
        }
        //------------------------------------------------------------------------------

        private void CopyAELToSEL()
        {
            TEdge4 e = m_ActiveEdges;
            m_SortedEdges = e;
            if (m_ActiveEdges == null)
                return;
            m_SortedEdges.prevInSEL = null;
            e = e.nextInAEL;
            while (e != null)
            {
                e.prevInSEL = e.prevInAEL;
                e.prevInSEL.nextInSEL = e;
                e.nextInSEL = null;
                e = e.nextInAEL;
            }
        }
        //------------------------------------------------------------------------------

        private void SwapPositionsInAEL(TEdge4 edge1, TEdge4 edge2)
        {
            if (edge1.nextInAEL == null && edge1.prevInAEL == null)
                return;
            if (edge2.nextInAEL == null && edge2.prevInAEL == null)
                return;

            if (edge1.nextInAEL == edge2)
            {
                TEdge4 next = edge2.nextInAEL;
                if (next != null)
                    next.prevInAEL = edge1;
                TEdge4 prev = edge1.prevInAEL;
                if (prev != null)
                    prev.nextInAEL = edge2;
                edge2.prevInAEL = prev;
                edge2.nextInAEL = edge1;
                edge1.prevInAEL = edge2;
                edge1.nextInAEL = next;
            }
            else if (edge2.nextInAEL == edge1)
            {
                TEdge4 next = edge1.nextInAEL;
                if (next != null)
                    next.prevInAEL = edge2;
                TEdge4 prev = edge2.prevInAEL;
                if (prev != null)
                    prev.nextInAEL = edge1;
                edge1.prevInAEL = prev;
                edge1.nextInAEL = edge2;
                edge2.prevInAEL = edge1;
                edge2.nextInAEL = next;
            }
            else
            {
                TEdge4 next = edge1.nextInAEL;
                TEdge4 prev = edge1.prevInAEL;
                edge1.nextInAEL = edge2.nextInAEL;
                if (edge1.nextInAEL != null)
                    edge1.nextInAEL.prevInAEL = edge1;
                edge1.prevInAEL = edge2.prevInAEL;
                if (edge1.prevInAEL != null)
                    edge1.prevInAEL.nextInAEL = edge1;
                edge2.nextInAEL = next;
                if (edge2.nextInAEL != null)
                    edge2.nextInAEL.prevInAEL = edge2;
                edge2.prevInAEL = prev;
                if (edge2.prevInAEL != null)
                    edge2.prevInAEL.nextInAEL = edge2;
            }

            if (edge1.prevInAEL == null)
                m_ActiveEdges = edge1;
            else if (edge2.prevInAEL == null)
                m_ActiveEdges = edge2;
        }
        //------------------------------------------------------------------------------

        private void SwapPositionsInSEL(TEdge4 edge1, TEdge4 edge2)
        {
            if (edge1.nextInSEL == null && edge1.prevInSEL == null)
                return;
            if (edge2.nextInSEL == null && edge2.prevInSEL == null)
                return;

            if (edge1.nextInSEL == edge2)
            {
                TEdge4 next = edge2.nextInSEL;
                if (next != null)
                    next.prevInSEL = edge1;
                TEdge4 prev = edge1.prevInSEL;
                if (prev != null)
                    prev.nextInSEL = edge2;
                edge2.prevInSEL = prev;
                edge2.nextInSEL = edge1;
                edge1.prevInSEL = edge2;
                edge1.nextInSEL = next;
            }
            else if (edge2.nextInSEL == edge1)
            {
                TEdge4 next = edge1.nextInSEL;
                if (next != null)
                    next.prevInSEL = edge2;
                TEdge4 prev = edge2.prevInSEL;
                if (prev != null)
                    prev.nextInSEL = edge1;
                edge1.prevInSEL = prev;
                edge1.nextInSEL = edge2;
                edge2.prevInSEL = edge1;
                edge2.nextInSEL = next;
            }
            else
            {
                TEdge4 next = edge1.nextInSEL;
                TEdge4 prev = edge1.prevInSEL;
                edge1.nextInSEL = edge2.nextInSEL;
                if (edge1.nextInSEL != null)
                    edge1.nextInSEL.prevInSEL = edge1;
                edge1.prevInSEL = edge2.prevInSEL;
                if (edge1.prevInSEL != null)
                    edge1.prevInSEL.nextInSEL = edge1;
                edge2.nextInSEL = next;
                if (edge2.nextInSEL != null)
                    edge2.nextInSEL.prevInSEL = edge2;
                edge2.prevInSEL = prev;
                if (edge2.prevInSEL != null)
                    edge2.prevInSEL.nextInSEL = edge2;
            }

            if (edge1.prevInSEL == null)
                m_SortedEdges = edge1;
            else if (edge2.prevInSEL == null)
                m_SortedEdges = edge2;
        }
        //------------------------------------------------------------------------------


        private void AddLocalMaxPoly(TEdge4 e1, TEdge4 e2, IntPoint pt)
        {
            AddPolyPt(e1, pt);
            if (e1.outIdx == e2.outIdx)
            {
                e1.outIdx = -1;
                e2.outIdx = -1;
            }
            else AppendPolygon(e1, e2);
        }
        //------------------------------------------------------------------------------

        private void AddLocalMinPoly(TEdge4 e1, TEdge4 e2, IntPoint pt)
        {
            if (e2.dx == horizontal || (e1.dx > e2.dx))
            {
                AddPolyPt(e1, pt);
                e2.outIdx = e1.outIdx;
                e1.side = EdgeSide.esLeft;
                e2.side = EdgeSide.esRight;
            }
            else
            {
                AddPolyPt(e2, pt);
                e1.outIdx = e2.outIdx;
                e1.side = EdgeSide.esRight;
                e2.side = EdgeSide.esLeft;
            }
        }
        //------------------------------------------------------------------------------

        private PolyPt AddPolyPt(TEdge4 e, IntPoint pt)
        {
          bool ToFront = (e.side == EdgeSide.esLeft);
          if(  e.outIdx < 0 )
          {
            PolyPt newPolyPt = new PolyPt();
            newPolyPt.pt = pt;
            newPolyPt.isHole = IsHole(e);
            m_PolyPts.Add(newPolyPt);
            newPolyPt.next = newPolyPt;
            newPolyPt.prev = newPolyPt;
            e.outIdx = m_PolyPts.Count-1;
            return newPolyPt;
          } else
          {
            PolyPt pp = m_PolyPts[e.outIdx];
            if (ToFront && PointsEqual(pt, pp.pt)) return pp;
            if (!ToFront && PointsEqual(pt, pp.prev.pt)) return pp.prev;

            PolyPt newPolyPt = new PolyPt();
            newPolyPt.pt = pt;
            newPolyPt.isHole = pp.isHole;
            newPolyPt.next = pp;
            newPolyPt.prev = pp.prev;
            newPolyPt.prev.next = newPolyPt;
            pp.prev = newPolyPt;
            if (ToFront) m_PolyPts[e.outIdx] = newPolyPt;
            return newPolyPt;
          }
        }
        //------------------------------------------------------------------------------


        internal void SwapPoints(ref IntPoint pt1, ref IntPoint pt2)
        {
            IntPoint tmp = pt1;
            pt1 = pt2;
            pt2 = tmp;
        }
        //------------------------------------------------------------------------------

        private bool GetOverlapSegment(IntPoint pt1a, IntPoint pt1b, IntPoint pt2a,
            IntPoint pt2b, ref IntPoint pt1, ref IntPoint pt2)
        {
            //precondition: segments are colinear.
            if ( pt1a.Y == pt1b.Y || Math.Abs((pt1a.X - pt1b.X)/(pt1a.Y - pt1b.Y)) > 1 )
            {
            if (pt1a.X > pt1b.X) SwapPoints(ref pt1a, ref pt1b);
            if (pt2a.X > pt2b.X) SwapPoints(ref pt2a, ref pt2b);
            if (pt1a.X > pt2a.X) pt1 = pt1a; else pt1 = pt2a;
            if (pt1b.X < pt2b.X) pt2 = pt1b; else pt2 = pt2b;
            return pt1.X < pt2.X;
            } else
            {
            if (pt1a.Y < pt1b.Y) SwapPoints(ref pt1a, ref pt1b);
            if (pt2a.Y < pt2b.Y) SwapPoints(ref pt2a, ref pt2b);
            if (pt1a.Y < pt2a.Y) pt1 = pt1a; else pt1 = pt2a;
            if (pt1b.Y > pt2b.Y) pt2 = pt1b; else pt2 = pt2b;
            return pt1.Y > pt2.Y;
            }
        }
        //------------------------------------------------------------------------------

        private PolyPt PolygonBottom(PolyPt pp)
        {
            PolyPt p = pp.next;
            PolyPt result = pp;
            while (p != pp)
            {
            if (p.pt.Y > result.pt.Y) result = p;
            else if (p.pt.Y == result.pt.Y && p.pt.X < result.pt.X) result = p;
            p = p.next;
            }
            return result;
        }
        //------------------------------------------------------------------------------

        private bool FindSegment(ref PolyPt pp, IntPoint pt1, IntPoint pt2)
        {
            if (pp == null) return false;
            PolyPt pp2 = pp;
            do
            {
            if (PointsEqual(pp.pt, pt1) &&
                (PointsEqual(pp.next.pt, pt2) || PointsEqual(pp.prev.pt, pt2)))
                return true;
            pp = pp.next;
            }
            while (pp != pp2);
            return false;
        }
        //------------------------------------------------------------------------------

        internal Position GetPosition(IntPoint pt1, IntPoint pt2, IntPoint pt)
        {
            if (PointsEqual(pt1, pt)) return Position.pFirst;
            else if (PointsEqual(pt2, pt)) return Position.pSecond;
            else return Position.pMiddle;
        }
        //------------------------------------------------------------------------------

        internal bool Pt3IsBetweenPt1AndPt2(IntPoint pt1, IntPoint pt2, IntPoint pt3)
        {
            if (PointsEqual(pt1, pt3) || PointsEqual(pt2, pt3)) return true;
            else if (pt1.X != pt2.X) return (pt1.X < pt3.X) == (pt3.X < pt2.X);
            else return (pt1.Y < pt3.Y) == (pt3.Y < pt2.Y);
        }
        //------------------------------------------------------------------------------

        private PolyPt InsertPolyPtBetween(PolyPt p1, PolyPt p2, IntPoint pt)
        {
            PolyPt result = new PolyPt();
            result.pt = pt;
            result.isHole = p1.isHole;
            if (p2 == p1.next)
            {
            p1.next = result;
            p2.prev = result;
            result.next = p2;
            result.prev = p1;
            } else
            {
            p2.next = result;
            p1.prev = result;
            result.next = p1;
            result.prev = p2;
            }
            return result;
        }
        //------------------------------------------------------------------------------

        private void AppendPolygon(TEdge4 e1, TEdge4 e2)
        {
          //get the start and ends of both output polygons ...
          PolyPt p1_lft = m_PolyPts[e1.outIdx];
          PolyPt p1_rt = p1_lft.prev;
          PolyPt p2_lft = m_PolyPts[e2.outIdx];
          PolyPt p2_rt = p2_lft.prev;

          //fixup orientation (hole) flag if necessary ...
          if (p1_lft.isHole != p2_lft.isHole)
          {
            PolyPt p, pp;
            PolyPt bottom1 = PolygonBottom(p1_lft);
            PolyPt bottom2 = PolygonBottom(p2_lft);
            if (bottom1.pt.Y > bottom2.pt.Y) p = p2_lft;
            else if (bottom1.pt.Y < bottom2.pt.Y) p = p1_lft;
            else if (bottom1.pt.X < bottom2.pt.X) p = p2_lft;
            else if (bottom1.pt.X > bottom2.pt.X) p = p1_lft;
            //todo - the following line still isn't 100% ...
            else if (bottom1.isHole) p = p1_lft; else p = p2_lft;

            bool hole = !p.isHole;
            pp = p;
            do
            {
              pp.isHole = hole;
              pp = pp.next;
            }
            while (pp != p);
          }

            EdgeSide side;
          //join e2 poly onto e1 poly and delete pointers to e2 ...
          if(  e1.side == EdgeSide.esLeft )
          {
            if (e2.side == EdgeSide.esLeft)
            {
              //z y x a b c
              ReversePolyPtLinks(p2_lft);
              p2_lft.next = p1_lft;
              p1_lft.prev = p2_lft;
              p1_rt.next = p2_rt;
              p2_rt.prev = p1_rt;
              m_PolyPts[e1.outIdx] = p2_rt;
            } else
            {
              //x y z a b c
              p2_rt.next = p1_lft;
              p1_lft.prev = p2_rt;
              p2_lft.prev = p1_rt;
              p1_rt.next = p2_lft;
              m_PolyPts[e1.outIdx] = p2_lft;
            }
            side = EdgeSide.esLeft;
          } else
          {
            if (e2.side == EdgeSide.esRight)
            {
              //a b c z y x
              ReversePolyPtLinks( p2_lft );
              p1_rt.next = p2_rt;
              p2_rt.prev = p1_rt;
              p2_lft.next = p1_lft;
              p1_lft.prev = p2_lft;
            } else
            {
              //a b c x y z
              p1_rt.next = p2_lft;
              p2_lft.prev = p1_rt;
              p1_lft.prev = p2_rt;
              p2_rt.next = p1_lft;
            }
            side = EdgeSide.esRight;
          }

          int OKIdx = e1.outIdx;
          int ObsoleteIdx = e2.outIdx;
          m_PolyPts[ObsoleteIdx] = null;

          e1.outIdx = -1; //nb: safe because we only get here via AddLocalMaxPoly
          e2.outIdx = -1;

          TEdge4 e = m_ActiveEdges;
          while( e != null )
          {
            if( e.outIdx == ObsoleteIdx )
            {
              e.outIdx = OKIdx;
              e.side = side;
              break;
            }
            e = e.nextInAEL;
          }
        }
        //------------------------------------------------------------------------------

        private void ReversePolyPtLinks(PolyPt pp)
        {
            PolyPt pp1;
            PolyPt pp2;
            pp1 = pp;
            do
            {
                pp2 = pp1.next;
                pp1.next = pp1.prev;
                pp1.prev = pp2;
                pp1 = pp2;
            } while (pp1 != pp);
        }
        //------------------------------------------------------------------------------

        private static void SwapSides(TEdge4 edge1, TEdge4 edge2)
        {
            EdgeSide side = edge1.side;
            edge1.side = edge2.side;
            edge2.side = side;
        }
        //------------------------------------------------------------------------------

        private static void SwapPolyIndexes(TEdge4 edge1, TEdge4 edge2)
        {
            int outIdx = edge1.outIdx;
            edge1.outIdx = edge2.outIdx;
            edge2.outIdx = outIdx;
        }
        //------------------------------------------------------------------------------

        private void DoEdge1(TEdge4 edge1, TEdge4 edge2, IntPoint pt)
        {
            AddPolyPt(edge1, pt);
            SwapSides(edge1, edge2);
            SwapPolyIndexes(edge1, edge2);
        }
        //------------------------------------------------------------------------------

        private void DoEdge2(TEdge4 edge1, TEdge4 edge2, IntPoint pt)
        {
            AddPolyPt(edge2, pt);
            SwapSides(edge1, edge2);
            SwapPolyIndexes(edge1, edge2);
        }
        //------------------------------------------------------------------------------

        private void DoBothEdges(TEdge4 edge1, TEdge4 edge2, IntPoint pt)
        {
            AddPolyPt(edge1, pt);
            AddPolyPt(edge2, pt);
            SwapSides(edge1, edge2);
            SwapPolyIndexes(edge1, edge2);
        }
        //------------------------------------------------------------------------------

        private void IntersectEdges(TEdge4 e1, TEdge4 e2, IntPoint pt, Protects protects)
        {
            //e1 will be to the left of e2 BELOW the intersection. Therefore e1 is before
            //e2 in AEL except when e1 is being inserted at the intersection point ...

            bool e1stops = (Protects.ipLeft & protects) == 0 && e1.nextInLML == null &&
              e1.xtop == pt.X && e1.ytop == pt.Y;
            bool e2stops = (Protects.ipRight & protects) == 0 && e2.nextInLML == null &&
              e2.xtop == pt.X && e2.ytop == pt.Y;
            bool e1Contributing = (e1.outIdx >= 0);
            bool e2contributing = (e2.outIdx >= 0);

            //update winding counts...
            //assumes that e1 will be to the right of e2 ABOVE the intersection
            if (e1.polyType == e2.polyType)
            {
                if (IsNonZeroFillType(e1))
                {
                    if (e1.windCnt + e2.windDelta == 0) e1.windCnt = -e1.windCnt;
                    else e1.windCnt += e2.windDelta;
                    if (e2.windCnt - e1.windDelta == 0) e2.windCnt = -e2.windCnt;
                    else e2.windCnt -= e1.windDelta;
                }
                else
                {
                    int oldE1WindCnt = e1.windCnt;
                    e1.windCnt = e2.windCnt;
                    e2.windCnt = oldE1WindCnt;
                }
            }
            else
            {
                if (IsNonZeroFillType(e2)) e1.windCnt2 += e2.windDelta;
                else e1.windCnt2 = (e1.windCnt2 == 0) ? 1 : 0;
                if (IsNonZeroFillType(e1)) e2.windCnt2 -= e1.windDelta;
                else e2.windCnt2 = (e2.windCnt2 == 0) ? 1 : 0;
            }

            if (e1Contributing && e2contributing)
            {
                if (e1stops || e2stops || Math.Abs(e1.windCnt) > 1 ||
                  Math.Abs(e2.windCnt) > 1 ||
                  (e1.polyType != e2.polyType && m_ClipType != ClipType.ctXor))
                    AddLocalMaxPoly(e1, e2, pt);
                else
                    DoBothEdges(e1, e2, pt);
            }
            else if (e1Contributing)
            {
                if (m_ClipType == ClipType.ctIntersection)
                {
                    if ((e2.polyType == PolyType.ptSubject || e2.windCnt2 != 0) &&
                        Math.Abs(e2.windCnt) < 2)
                        DoEdge1(e1, e2, pt);
                }
                else
                {
                    if (Math.Abs(e2.windCnt) < 2)
                        DoEdge1(e1, e2, pt);
                }
            }
            else if (e2contributing)
            {
                if (m_ClipType == ClipType.ctIntersection)
                {
                    if ((e1.polyType == PolyType.ptSubject || e1.windCnt2 != 0) &&
                  Math.Abs(e1.windCnt) < 2) DoEdge2(e1, e2, pt);
                }
                else
                {
                    if (Math.Abs(e1.windCnt) < 2)
                        DoEdge2(e1, e2, pt);
                }
            }
            else if (Math.Abs(e1.windCnt) < 2 && Math.Abs(e2.windCnt) < 2)
            {
                //neither edge is currently contributing ...
                if (e1.polyType != e2.polyType && !e1stops && !e2stops &&
                  Math.Abs(e1.windCnt) < 2 && Math.Abs(e2.windCnt) < 2)
                    AddLocalMinPoly(e1, e2, pt);
                else if (Math.Abs(e1.windCnt) == 1 && Math.Abs(e2.windCnt) == 1)
                    switch (m_ClipType)
                    {
                        case ClipType.ctIntersection:
                            {
                                if (Math.Abs(e1.windCnt2) > 0 && Math.Abs(e2.windCnt2) > 0)
                                    AddLocalMinPoly(e1, e2, pt);
                                break;
                            }
                        case ClipType.ctUnion:
                            {
                                if (e1.windCnt2 == 0 && e2.windCnt2 == 0)
                                    AddLocalMinPoly(e1, e2, pt);
                                break;
                            }
                        case ClipType.ctDifference:
                            {
                                if ((e1.polyType == PolyType.ptClip && e2.polyType == PolyType.ptClip &&
                              e1.windCnt2 != 0 && e2.windCnt2 != 0) ||
                              (e1.polyType == PolyType.ptSubject && e2.polyType == PolyType.ptSubject &&
                              e1.windCnt2 == 0 && e2.windCnt2 == 0))
                                    AddLocalMinPoly(e1, e2, pt);
                                break;
                            }
                        case ClipType.ctXor:
                            {
                                AddLocalMinPoly(e1, e2, pt);
                                break;
                            }
                    }
                else if (Math.Abs(e1.windCnt) < 2 && Math.Abs(e2.windCnt) < 2)
                    SwapSides(e1, e2);
            }

            if ((e1stops != e2stops) &&
              ((e1stops && (e1.outIdx >= 0)) || (e2stops && (e2.outIdx >= 0))))
            {
                SwapSides(e1, e2);
                SwapPolyIndexes(e1, e2);
            }

            //finally, delete any non-contributing maxima edges  ...
            if (e1stops) DeleteFromAEL(e1);
            if (e2stops) DeleteFromAEL(e2);
        }
        //------------------------------------------------------------------------------

        private void DeleteFromAEL(TEdge4 e)
        {
            TEdge4 AelPrev = e.prevInAEL;
            TEdge4 AelNext = e.nextInAEL;
            if (AelPrev == null && AelNext == null && (e != m_ActiveEdges))
                return; //already deleted
            if (AelPrev != null)
                AelPrev.nextInAEL = AelNext;
            else m_ActiveEdges = AelNext;
            if (AelNext != null)
                AelNext.prevInAEL = AelPrev;
            e.nextInAEL = null;
            e.prevInAEL = null;
        }
        //------------------------------------------------------------------------------

        private void DeleteFromSEL(TEdge4 e)
        {
            TEdge4 SelPrev = e.prevInSEL;
            TEdge4 SelNext = e.nextInSEL;
            if (SelPrev == null && SelNext == null && (e != m_SortedEdges))
                return; //already deleted
            if (SelPrev != null)
                SelPrev.nextInSEL = SelNext;
            else m_SortedEdges = SelNext;
            if (SelNext != null)
                SelNext.prevInSEL = SelPrev;
            e.nextInSEL = null;
            e.prevInSEL = null;
        }
        //------------------------------------------------------------------------------

        private void UpdateEdgeIntoAEL(ref TEdge4 e)
        {
            if (e.nextInLML == null)
                throw new ClipperException("UpdateEdgeIntoAEL: invalid call");
            TEdge4 AelPrev = e.prevInAEL;
            TEdge4 AelNext = e.nextInAEL;
            e.nextInLML.outIdx = e.outIdx;
            if (AelPrev != null)
                AelPrev.nextInAEL = e.nextInLML;
            else m_ActiveEdges = e.nextInLML;
            if (AelNext != null)
                AelNext.prevInAEL = e.nextInLML;
            e.nextInLML.side = e.side;
            e.nextInLML.windDelta = e.windDelta;
            e.nextInLML.windCnt = e.windCnt;
            e.nextInLML.windCnt2 = e.windCnt2;
            e = e.nextInLML;
            e.prevInAEL = AelPrev;
            e.nextInAEL = AelNext;
            if (e.dx != horizontal)
            {
                InsertScanbeam(e.ytop);
                //if output polygons share an edge, they'll need joining later ...
                if (e.outIdx >= 0 && AelPrev != null && AelPrev.outIdx >= 0 &&
                  AelPrev.xbot == e.xcurr && SlopesEqual(e, AelPrev))
                    AddJoin(e, AelPrev);
            }
        }
        //------------------------------------------------------------------------------

        private void ProcessHorizontals()
        {
            TEdge4 horzEdge = m_SortedEdges;
            while (horzEdge != null)
            {
                DeleteFromSEL(horzEdge);
                ProcessHorizontal(horzEdge);
                horzEdge = m_SortedEdges;
            }
        }
        //------------------------------------------------------------------------------

        private void ProcessHorizontal(TEdge4 horzEdge)
        {
            Direction Direction;
            int horzLeft, horzRight;

            if (horzEdge.xcurr < horzEdge.xtop)
            {
                horzLeft = horzEdge.xcurr;
                horzRight = horzEdge.xtop;
                Direction = Direction.dLeftToRight;
            }
            else
            {
                horzLeft = horzEdge.xtop;
                horzRight = horzEdge.xcurr;
                Direction = Direction.dRightToLeft;
            }

            TEdge4 eMaxPair;
            if (horzEdge.nextInLML != null)
                eMaxPair = null;
            else
                eMaxPair = GetMaximaPair(horzEdge);

            TEdge4 e = GetNextInAEL(horzEdge, Direction);
            while (e != null)
            {
                TEdge4 eNext = GetNextInAEL(e, Direction);
                if (e.xcurr >= horzLeft && e.xcurr <= horzRight)
                {
                    //ok, so far it looks like we're still in range of the horizontal edge
                    if (e.xcurr == horzEdge.xtop && horzEdge.nextInLML != null)
                    {
                        if (SlopesEqual(e, horzEdge.nextInLML))
                        {
                            //if output polygons share an edge, they'll need joining later ...
                            if (horzEdge.outIdx >= 0 && e.outIdx >= 0)
                                AddJoin(horzEdge.nextInLML, e, horzEdge.outIdx);
                            break; //we've reached the end of the horizontal line
                        }
                        else if (e.dx < horzEdge.nextInLML.dx)
                            //we really have got to the end of the intermediate horz edge so quit.
                            //nb: More -ve slopes follow more +ve slopes ABOVE the horizontal.
                            break;
                    }

                    if (e == eMaxPair)
                    {
                        //horzEdge is evidently a maxima horizontal and we've arrived at its end.
                        if (Direction == Direction.dLeftToRight)
                            IntersectEdges(horzEdge, e, new IntPoint(e.xcurr, horzEdge.ycurr), 0);
                        else
                            IntersectEdges(e, horzEdge, new IntPoint(e.xcurr, horzEdge.ycurr), 0);
                        return;
                    }
                    else if (e.dx == horizontal && !IsMinima(e) && !(e.xcurr > e.xtop))
                    {
                        if (Direction == Direction.dLeftToRight)
                            IntersectEdges(horzEdge, e, new IntPoint(e.xcurr, horzEdge.ycurr),
                              (IsTopHorz(horzEdge, e.xcurr)) ? Protects.ipLeft : Protects.ipBoth);
                        else
                            IntersectEdges(e, horzEdge, new IntPoint(e.xcurr, horzEdge.ycurr),
                              (IsTopHorz(horzEdge, e.xcurr)) ? Protects.ipRight : Protects.ipBoth);
                    }
                    else if (Direction == Direction.dLeftToRight)
                    {
                        IntersectEdges(horzEdge, e, new IntPoint(e.xcurr, horzEdge.ycurr),
                          (IsTopHorz(horzEdge, e.xcurr)) ? Protects.ipLeft : Protects.ipBoth);
                    }
                    else
                    {
                        IntersectEdges(e, horzEdge, new IntPoint(e.xcurr, horzEdge.ycurr),
                          (IsTopHorz(horzEdge, e.xcurr)) ? Protects.ipRight : Protects.ipBoth);
                    }
                    SwapPositionsInAEL(horzEdge, e);
                }
                else if (Direction == Direction.dLeftToRight && 
                      e.xcurr > horzRight && horzEdge.nextInSEL == null) break;
                else if ((Direction == Direction.dRightToLeft) &&
                  e.xcurr < horzLeft && horzEdge.nextInSEL == null) break;
                e = eNext;
            } //end while ( e )

            if (horzEdge.nextInLML != null)
            {
                if (horzEdge.outIdx >= 0)
                    AddPolyPt(horzEdge, new IntPoint(horzEdge.xtop, horzEdge.ytop));
                UpdateEdgeIntoAEL(ref horzEdge);
            }
            else
            {
                if (horzEdge.outIdx >= 0)
                    IntersectEdges(horzEdge, eMaxPair, 
                        new IntPoint(horzEdge.xtop, horzEdge.ycurr), Protects.ipBoth);
                DeleteFromAEL(eMaxPair);
                DeleteFromAEL(horzEdge);
            }
        }
        //------------------------------------------------------------------------------

        private bool IsTopHorz(TEdge4 horzEdge, double XPos)
        {
            TEdge4 e = m_SortedEdges;
            while (e != null)
            {
                if ((XPos >= Math.Min(e.xcurr, e.xtop)) && (XPos <= Math.Max(e.xcurr, e.xtop)))
                    return false;
                e = e.nextInSEL;
            }
            return true;
        }
        //------------------------------------------------------------------------------

        private TEdge4 GetNextInAEL(TEdge4 e, Direction Direction)
        {
            if (Direction == Direction.dLeftToRight) return e.nextInAEL;
            else return e.prevInAEL;
        }
        //------------------------------------------------------------------------------

        private bool IsMinima(TEdge4 e)
        {
            return e != null && (e.prev.nextInLML != e) && (e.next.nextInLML != e);
        }
        //------------------------------------------------------------------------------

        private bool IsMaxima(TEdge4 e, double Y)
        {
            return (e != null && e.ytop == Y && e.nextInLML == null);
        }
        //------------------------------------------------------------------------------

        private bool IsIntermediate(TEdge4 e, double Y)
        {
            return (e.ytop == Y && e.nextInLML != null);
        }
        //------------------------------------------------------------------------------

        private TEdge4 GetMaximaPair(TEdge4 e)
        {
            if (!IsMaxima(e.next, e.ytop) || (e.next.xtop != e.xtop))
                return e.prev; else
                return e.next;
        }
        //------------------------------------------------------------------------------

        private bool ProcessIntersections(int topY)
        {
          if( m_ActiveEdges == null ) return true;
          try {
            BuildIntersectList(topY);
            if ( m_IntersectNodes == null) return true;
            if ( FixupIntersections() ) ProcessIntersectList();
            else return false;
          }
          catch {
            m_SortedEdges = null;
            DisposeIntersectNodes();
            throw new ClipperException("ProcessIntersections error");
          }
          return true;
        }
        //------------------------------------------------------------------------------

        private void BuildIntersectList(int topY)
        {
          if ( m_ActiveEdges == null ) return;

          //prepare for sorting ...
          TEdge4 e = m_ActiveEdges;
          e.tmpX = TopX( e, topY );
          m_SortedEdges = e;
          m_SortedEdges.prevInSEL = null;
          e = e.nextInAEL;
          while( e != null )
          {
            e.prevInSEL = e.prevInAEL;
            e.prevInSEL.nextInSEL = e;
            e.nextInSEL = null;
            e.tmpX = TopX( e, topY );
            e = e.nextInAEL;
          }

          //bubblesort ...
          bool isModified = true;
          while( isModified && m_SortedEdges != null )
          {
            isModified = false;
            e = m_SortedEdges;
            while( e.nextInSEL != null )
            {
              TEdge4 eNext = e.nextInSEL;
              IntPoint pt = new IntPoint();
              if(e.tmpX > eNext.tmpX && IntersectPoint(e, eNext, ref pt))
              {
                AddIntersectNode( e, eNext, pt );
                SwapPositionsInSEL(e, eNext);
                isModified = true;
              }
              else
                e = eNext;
            }
            if( e.prevInSEL != null ) e.prevInSEL.nextInSEL = null;
            else break;
          }
          m_SortedEdges = null;
        }
        //------------------------------------------------------------------------------

        private bool FixupIntersections()
        {
          if ( m_IntersectNodes.next == null ) return true;

          CopyAELToSEL();
          IntersectNode int1 = m_IntersectNodes;
          IntersectNode int2 = m_IntersectNodes.next;
          while (int2 != null)
          {
            TEdge4 e1 = int1.edge1;
            TEdge4 e2;
            if (e1.prevInSEL == int1.edge2) e2 = e1.prevInSEL;
            else if (e1.nextInSEL == int1.edge2) e2 = e1.nextInSEL;
            else
            {
              //The current intersection is out of order, so try and swap it with
              //a subsequent intersection ...
              while (int2 != null)
              {
                if (int2.edge1.nextInSEL == int2.edge2 ||
                  int2.edge1.prevInSEL == int2.edge2) break;
                else int2 = int2.next;
              }
              if (int2 == null) return false; //oops!!!

              //found an intersect node that can be swapped ...
              SwapIntersectNodes(int1, int2);
              e1 = int1.edge1;
              e2 = int1.edge2;
            }
            SwapPositionsInSEL(e1, e2);
            int1 = int1.next;
            int2 = int1.next;
          }

          m_SortedEdges = null;

          //finally, check the last intersection too ...
          return (int1.edge1.prevInSEL == int1.edge2 || int1.edge1.nextInSEL == int1.edge2);
        }
        //------------------------------------------------------------------------------

        private void ProcessIntersectList()
        {
          while( m_IntersectNodes != null )
          {
            IntersectNode iNode = m_IntersectNodes.next;
            {
              IntersectEdges( m_IntersectNodes.edge1 ,
                m_IntersectNodes.edge2 , m_IntersectNodes.pt, Protects.ipBoth );
              SwapPositionsInAEL( m_IntersectNodes.edge1 , m_IntersectNodes.edge2 );
            }
            m_IntersectNodes = null;
            m_IntersectNodes = iNode;
          }
        }
        //------------------------------------------------------------------------------

        private static int Round(double value)
        {
            if ((value < 0)) return (int)(value - 0.5); else return (int)(value + 0.5);
        }
        //------------------------------------------------------------------------------

        private static int TopX(TEdge4 edge, int currentY)
        {
            if (currentY == edge.ytop)
                return edge.xtop;
            return edge.xbot + Round(edge.dx *(currentY - edge.ybot));
        }
        //------------------------------------------------------------------------------

        private int TopX(IntPoint pt1, IntPoint pt2, int currentY)
        {
          //preconditions: pt1.Y <> pt2.Y and pt1.Y > pt2.Y
          if (currentY >= pt1.Y) return pt1.X;
          else if (currentY == pt2.Y) return pt2.X;
          else if (pt1.X == pt2.X) return pt1.X;
          else
          {
            double q = (pt1.X-pt2.X)/(pt1.Y-pt2.Y);
            return (int)(pt1.X + (currentY - pt1.Y) *q);
          }
        }
        //------------------------------------------------------------------------------

        private void AddIntersectNode(TEdge4 e1, TEdge4 e2, IntPoint pt)
        {
          IntersectNode newNode = new IntersectNode();
          newNode.edge1 = e1;
          newNode.edge2 = e2;
          newNode.pt = pt;
          newNode.next = null;
          if (m_IntersectNodes == null) m_IntersectNodes = newNode;
          else if( Process1Before2(newNode, m_IntersectNodes) )
          {
            newNode.next = m_IntersectNodes;
            m_IntersectNodes = newNode;
          }
          else
          {
            IntersectNode iNode = m_IntersectNodes;
            while( iNode.next != null  && Process1Before2(iNode.next, newNode) )
                iNode = iNode.next;
            newNode.next = iNode.next;
            iNode.next = newNode;
          }
        }
        //------------------------------------------------------------------------------

        private bool Process1Before2(IntersectNode node1, IntersectNode node2)
        {
          bool result;
          if (node1.pt.Y == node2.pt.Y)
          {
            if (node1.edge1 == node2.edge1 || node1.edge2 == node2.edge1)
            {
              result = node2.pt.X > node1.pt.X;
              if (node2.edge1.dx > 0) return result; else return !result;
            }
            else if (node1.edge1 == node2.edge2 || node1.edge2 == node2.edge2)
            {
              result = node2.pt.X > node1.pt.X;
              if (node2.edge2.dx > 0) return result; else return !result;
            }
            else return node2.pt.X > node1.pt.X;
          }
          else return node1.pt.Y > node2.pt.Y;
        }
        //------------------------------------------------------------------------------

        private void SwapIntersectNodes(IntersectNode int1, IntersectNode int2)
        {
          TEdge4 e1 = int1.edge1;
          TEdge4 e2 = int1.edge2;
          IntPoint p = int1.pt;
          int1.edge1 = int2.edge1;
          int1.edge2 = int2.edge2;
          int1.pt = int2.pt;
          int2.edge1 = e1;
          int2.edge2 = e2;
          int2.pt = p;
        }
        //------------------------------------------------------------------------------

        private bool IntersectPoint(TEdge4 edge1, TEdge4 edge2, ref IntPoint ip)
        {
          double b1, b2;
          if (SlopesEqual(edge1, edge2)) return false;
          else if (edge1.dx == 0)
          {
            ip.X = edge1.xbot;
            if (edge2.dx == horizontal)
            {
              ip.Y = edge2.ybot;
            } else
            {
              b2 = edge2.ybot - (edge2.xbot/edge2.dx);
              ip.Y = Round(ip.X/edge2.dx + b2);
            }
          }
          else if (edge2.dx == 0)
          {
            ip.X = edge2.xbot;
            if (edge1.dx == horizontal)
            {
              ip.Y = edge1.ybot;
            } else
            {
              b1 = edge1.ybot - (edge1.xbot/edge1.dx);
              ip.Y = Round(ip.X/edge1.dx + b1);
            }
          } else
          {
            b1 = edge1.xbot - edge1.ybot * edge1.dx;
            b2 = edge2.xbot - edge2.ybot * edge2.dx;
            b2 = (b2-b1)/(edge1.dx - edge2.dx);
            ip.Y = Round(b2);
            ip.X = Round(edge1.dx * b2 + b1);
          }

          return
            //can be *so close* to the top of one edge that the rounded Y equals one ytop ...
            (ip.Y == edge1.ytop && ip.Y >= edge2.ytop && edge1.tmpX > edge2.tmpX) ||
            (ip.Y == edge2.ytop && ip.Y >= edge1.ytop && edge1.tmpX > edge2.tmpX) ||
            (ip.Y > edge1.ytop && ip.Y > edge2.ytop);
        }
        //------------------------------------------------------------------------------

        private void DisposeIntersectNodes()
        {
          while ( m_IntersectNodes != null )
          {
            IntersectNode iNode = m_IntersectNodes.next;
            m_IntersectNodes = null;
            m_IntersectNodes = iNode;
          }
        }
        //------------------------------------------------------------------------------

        private void ProcessEdgesAtTopOfScanbeam(int topY)
        {
          TEdge4 e = m_ActiveEdges;
          while( e != null )
          {
            //1. process maxima, treating them as if they're 'bent' horizontal edges,
            //   but exclude maxima with horizontal edges. nb: e can't be a horizontal.
            if( IsMaxima(e, topY) && GetMaximaPair(e).dx != horizontal )
            {
              //'e' might be removed from AEL, as may any following edges so ...
              TEdge4 ePrior = e.prevInAEL;
              DoMaxima(e, topY);
              if( ePrior == null ) e = m_ActiveEdges;
              else e = ePrior.nextInAEL;
            }
            else
            {
              //2. promote horizontal edges, otherwise update xcurr and ycurr ...
              if(  IsIntermediate(e, topY) && e.nextInLML.dx == horizontal )
              {
                if (e.outIdx >= 0)
                {
                    AddPolyPt(e, new IntPoint(e.xtop, e.ytop));
                    AddHorzJoin(e.nextInLML, e.outIdx);
                }
                UpdateEdgeIntoAEL(ref e);
                AddEdgeToSEL(e);
              } 
              else
              {
                //this just simplifies horizontal processing ...
                e.xcurr = TopX( e, topY );
                e.ycurr = topY;
              }
              e = e.nextInAEL;
            }
          }

          //3. Process horizontals at the top of the scanbeam ...
          ProcessHorizontals();

          if (m_ActiveEdges == null) return;

          //4. Promote intermediate vertices ...
          e = m_ActiveEdges;
          while( e != null )
          {
            if( IsIntermediate( e, topY ) )
            {
                if (e.outIdx >= 0) AddPolyPt(e, new IntPoint(e.xtop, e.ytop));
              UpdateEdgeIntoAEL(ref e);
            }
            e = e.nextInAEL;
          }
        }
        //------------------------------------------------------------------------------

        private void DoMaxima(TEdge4 e, int topY)
        {
          TEdge4 eMaxPair = GetMaximaPair(e);
          int X = e.xtop;
          TEdge4 eNext = e.nextInAEL;
          while( eNext != eMaxPair )
          {
            if (eNext == null) throw new ClipperException("DoMaxima error");
            IntersectEdges( e, eNext, new IntPoint(X, topY), Protects.ipBoth );
            eNext = eNext.nextInAEL;
          }
          if( e.outIdx < 0 && eMaxPair.outIdx < 0 )
          {
            DeleteFromAEL( e );
            DeleteFromAEL( eMaxPair );
          }
          else if( e.outIdx >= 0 && eMaxPair.outIdx >= 0 )
          {
              IntersectEdges(e, eMaxPair, new IntPoint(X, topY), Protects.ipNone);
          }
          else throw new ClipperException("DoMaxima error");
        }
        //------------------------------------------------------------------------------

        public static bool IsClockwise(Polygon poly)
        {
          int highI = poly.Count -1;
          if (highI < 2) return false;
          double area;
          area = (double)poly[highI].X * poly[0].Y -(double)poly[0].X * poly[highI].Y;
          for (int i = 0; i < highI; ++i)
            area += (double)poly[i].X * poly[i+1].Y -(double)poly[i+1].X * poly[i].Y;
          //area := area/2;
          return area > 0; //reverse of normal formula because assuming Y axis inverted
        }
        //------------------------------------------------------------------------------

        private bool IsClockwise(PolyPt pt)
        {
            Int64 area = 0;
            PolyPt startPt = pt;
            do
            {
                area += (Int64)pt.pt.X * pt.next.pt.Y - (Int64)pt.next.pt.X * pt.pt.Y;
                pt = pt.next;
            }
            while (pt != startPt);
            //area = area /2;
            return area > 0; //reverse of normal formula because assuming Y axis inverted
        }
        //------------------------------------------------------------------------------

        private void BuildResult(Polygons polyg)
        {
          for (int i = 0; i < m_PolyPts.Count; ++i)
              if (m_PolyPts[i] != null)
              {
                  m_PolyPts[i] = FixupOutPolygon(m_PolyPts[i]);
                  //fix orientation ...
                  PolyPt p = m_PolyPts[i];
                  if (p != null && p.isHole == IsClockwise(p))
                      ReversePolyPtLinks(p);
              }
          JoinCommonEdges();

          polyg.Clear();
          polyg.Capacity = m_PolyPts.Count;
          for (int i = 0; i < m_PolyPts.Count; ++i)
          {
            if (m_PolyPts[i] != null) {
                Polygon pg = new Polygon();
                PolyPt p = m_PolyPts[i];

              do {
                pg.Add(new IntPoint(p.pt.X, p.pt.Y));
                p = p.next;
              } while (p != m_PolyPts[i]);
              if (pg.Count > 2) polyg.Add(pg); else pg = null;
            }
          }
        }
        //------------------------------------------------------------------------------

        private PolyPt FixupOutPolygon(PolyPt p, bool stripPointyEdgesOnly = false)
        {
            //FixupOutPolygon() - removes duplicate points and simplifies consecutive
            //parallel edges by removing the middle vertex.

            if (p == null) return null;
            PolyPt pp = p, result = p, lastOK = null;
            for (; ; )
            {
                if (pp.prev == pp || pp.prev == pp.next)
                {
                    DisposePolyPts(pp);
                    return null;
                }
                //test for duplicate points and for same slope (cross-product) ...
                if (PointsEqual(pp.pt, pp.next.pt) ||
                  SlopesEqual(pp.prev.pt, pp.pt, pp.next.pt))
                {
                    lastOK = null;
                    pp.prev.next = pp.next;
                    pp.next.prev = pp.prev;
                    PolyPt tmp = pp;
                    if (pp == result) result = pp.prev;
                    pp = pp.prev;
                    tmp = null;
                }
                else if (pp == lastOK) break;
                else
                {
                    if (lastOK == null) lastOK = pp;
                    pp = pp.next;
                }
            }
            return result;
        }
        //------------------------------------------------------------------------------

        private bool IsHole(TEdge4 e)
        {
            bool hole = false;
            TEdge4 e2 = m_ActiveEdges;
            while (e2 != null && e2 != e)
            {
                if (e2.outIdx >= 0) hole = !hole;
                e2 = e2.nextInAEL;
            }
            return hole;
        }
        //----------------------------------------------------------------------

        private void JoinCommonEdges()
        {
          for (int i = 0; i < m_Joins.Count; i++)
          {
            PolyPt pp1a, pp1b, pp2a, pp2b;
            IntPoint pt1 = new IntPoint(), pt2 = new IntPoint();

            JoinRec j = m_Joins[i];
            if (j.poly1Idx < 0 || j.poly2Idx < 0) 
                throw new ClipperException("JoinCommonEdges error");
            
            pp1a = m_PolyPts[j.poly1Idx];
            pp2a = m_PolyPts[j.poly2Idx];
            if (FindSegment(ref pp1a, j.pt1a, j.pt1b) &&
              FindSegment(ref pp2a, j.pt2a, j.pt2b))
            {
              if (PointsEqual(pp1a.next.pt, j.pt1b))
                pp1b = pp1a.next; else pp1b = pp1a.prev;
              if (PointsEqual(pp2a.next.pt, j.pt2b))
                pp2b = pp2a.next; else pp2b = pp2a.prev;
              if (j.poly1Idx != j.poly2Idx &&
                GetOverlapSegment(pp1a.pt, pp1b.pt, pp2a.pt, pp2b.pt, ref pt1, ref pt2))
              {
                PolyPt p1, p2, p3, p4;
                //get p1 & p2 polypts - the overlap start & endpoints on poly1
                Position pos1 = GetPosition(pp1a.pt, pp1b.pt, pt1);
                if (pos1 == Position.pFirst) p1 = pp1a;
                else if (pos1 == Position.pSecond) p1 = pp1b;
                else p1 = InsertPolyPtBetween(pp1a, pp1b, pt1);
                Position pos2 = GetPosition(pp1a.pt, pp1b.pt, pt2);
                if (pos2 == Position.pMiddle)
                {
                    if (pos1 == Position.pMiddle)
                  {
                    if (Pt3IsBetweenPt1AndPt2(pp1a.pt, p1.pt, pt2))
                      p2 = InsertPolyPtBetween(pp1a, p1, pt2); else
                      p2 = InsertPolyPtBetween(p1, pp1b, pt2);
                  }
                    else if (pos2 == Position.pFirst) p2 = pp1a;
                  else p2 = pp1b;
                }
                else if (pos2 == Position.pFirst) p2 = pp1a;
                else p2 = pp1b;
                //get p3 & p4 polypts - the overlap start & endpoints on poly2
                pos1 = GetPosition(pp2a.pt, pp2b.pt, pt1);
                if (pos1 == Position.pFirst) p3 = pp2a;
                else if (pos1 == Position.pSecond) p3 = pp2b;
                else p3 = InsertPolyPtBetween(pp2a, pp2b, pt1);
                pos2 = GetPosition(pp2a.pt, pp2b.pt, pt2);
                if (pos2 == Position.pMiddle)
                {
                    if (pos1 == Position.pMiddle)
                  {
                    if (Pt3IsBetweenPt1AndPt2(pp2a.pt, p3.pt, pt2))
                      p4 = InsertPolyPtBetween(pp2a, p3, pt2); else
                      p4 = InsertPolyPtBetween(p3, pp2b, pt2);
                  }
                    else if (pos2 == Position.pFirst) p4 = pp2a;
                  else p4 = pp2b;
                }
                else if (pos2 == Position.pFirst) p4 = pp2a;
                else p4 = pp2b;

                if (p1.next == p2) p1.next = p3; else p1.prev = p3;
                if (p3.next == p4) p3.next = p1; else p3.prev = p1;

                if (p2.next == p1) p2.next = p4; else p2.prev = p4;
                if (p4.next == p3) p4.next = p2; else p4.prev = p2;

                m_PolyPts[j.poly2Idx] = null;
              }
            }
          }
        }

        //------------------------------------------------------------------------------
        // OffsetPolygon functions ...
        //------------------------------------------------------------------------------

        public static double Area(Polygon poly)
        {
          int highI = poly.Count -1;
          if (highI < 2) return 0;
          float area = (Int64)poly[highI].X * poly[0].Y - (Int64)poly[0].X * poly[highI].Y;
          for (int i = 0; i < highI; ++i)
              area += (Int64)poly[i].X * poly[i + 1].Y - (Int64)poly[i + 1].X * poly[i].Y;
          return area/2;
        }
        //------------------------------------------------------------------------------

        internal class DoublePoint
        {
            public double X { get; set; }
            public double Y { get; set; }
            public DoublePoint(double x = 0, double y = 0)
            {
                this.X = x; this.Y = y;
            }
        };
        //------------------------------------------------------------------------------


        internal static Polygon BuildArc(IntPoint pt, double a1, double a2, double r)
        {
          int steps = Math.Max(6, (int)(Math.Sqrt(Math.Abs(r)) * Math.Abs(a2 - a1)));
          Polygon result = new Polygon(steps);
          int n = steps - 1;
          double da = (a2 - a1) / n;
          double a = a1;
          for (int i = 0; i < steps; ++i)
          {
            result.Add(new IntPoint(pt.X + (int)(Math.Cos(a)*r), pt.Y + (int)(Math.Sin(a)*r)));
            a += da;
          }
          return result;
        }
        //------------------------------------------------------------------------------

        internal static DoublePoint GetUnitNormal(IntPoint pt1, IntPoint pt2)
        {
          double dx = ( pt2.X - pt1.X );
          double dy = ( pt2.Y - pt1.Y );
          if(  ( dx == 0 ) && ( dy == 0 ) ) return new DoublePoint();

          double f = 1 *1.0/ Math.Sqrt( dx*dx + dy*dy );
          dx *= f;
          dy *= f;
          return new DoublePoint(dy, -dx);
        }
        //------------------------------------------------------------------------------

        public static Polygons OffsetPolygons(Polygons pts, double delta)
        {
          double deltaSq = delta*delta;
          Polygons result = new Polygons(pts.Count);

          for (int j = 0; j < pts.Count; ++j)
          {
            int highI = pts[j].Count -1;
            //to minimize artefacts, strip out those polygons where
            //it's shrinking and where its area < Sqr(delta) ...
            double a1 = Area(pts[j]);
            if (delta < 0) { if (a1 > 0 && a1 < deltaSq) highI = 0;}
            else if (a1 < 0 && -a1 < deltaSq) highI = 0; //nb: a hole if area < 0

            Polygon pg = new Polygon(highI*2+2);

            if (highI < 2)
            {
              result.Add(pg);
              continue;
            }

            List<DoublePoint> normals = new List<DoublePoint>(highI+1);
            normals.Add(GetUnitNormal(pts[j][highI], pts[j][0]));
            for (int i = 1; i <= highI; ++i)
              normals.Add(GetUnitNormal(pts[j][i-1], pts[j][i]));

            for (int i = 0; i < highI; ++i)
            {
              pg.Add(new IntPoint(Round(pts[j][i].X + delta *normals[i].X),
                (int)(pts[j][i].Y + delta *normals[i].Y)));
              pg.Add(new IntPoint(Round(pts[j][i].X + delta * normals[i + 1].X),
                (int)(pts[j][i].Y + delta *normals[i+1].Y)));
            }
            pg.Add(new IntPoint(Round(pts[j][highI].X + delta * normals[highI].X),
              (int)(pts[j][highI].Y + delta *normals[highI].Y)));
            pg.Add(new IntPoint(Round(pts[j][highI].X + delta * normals[0].X),
              (int)(pts[j][highI].Y + delta *normals[0].Y)));

            //round off reflex angles (ie > 180 deg) unless it's almost flat (ie < 10deg angle) ...
            //cross product normals < 0 . reflex angle; dot product normals == 1 . no angle
            if ((normals[highI].X *normals[0].Y - normals[0].X *normals[highI].Y) *delta > 0 &&
            (normals[0].X *normals[highI].X + normals[0].Y *normals[highI].Y) < 0.985)
            {
              double at1 = Math.Atan2(normals[highI].Y, normals[highI].X);
              double at2 = Math.Atan2(normals[0].Y, normals[0].X);
              if (delta > 0 && at2 < at1) at2 = at2 + Math.PI*2;
              else if (delta < 0 && at2 > at1) at2 = at2 - Math.PI*2;
              Polygon arc = BuildArc(pts[j][highI], at1, at2, delta);
              pg.InsertRange(highI * 2 + 1, arc);
            }
            for (int i = highI; i > 0; --i)
              if ((normals[i-1].X*normals[i].Y - normals[i].X*normals[i-1].Y) *delta > 0 &&
              (normals[i].X*normals[i-1].X + normals[i].Y*normals[i-1].Y) < 0.985)
              {
                double at1 = Math.Atan2(normals[i-1].Y, normals[i-1].X);
                double at2 = Math.Atan2(normals[i].Y, normals[i].X);
                if (delta > 0 && at2 < at1) at2 = at2 + Math.PI*2;
                else if (delta < 0 && at2 > at1) at2 = at2 - Math.PI*2;
                Polygon arc = BuildArc(pts[j][i-1], at1, at2, delta);
                pg.InsertRange((i - 1) * 2 + 1, arc);
              }
            result.Add(pg);
          }

          //finally, clean up untidy corners ...
          Clipper clpr = new Clipper();
          clpr.AddPolygons(result, PolyType.ptSubject);
          if (delta > 0){
            if(!clpr.Execute(ClipType.ctUnion, result, PolyFillType.pftNonZero, PolyFillType.pftNonZero))
              result.Clear();
          }
          else
          {
            IntRect r = clpr.GetBounds();
            Polygon outer = new Polygon(4);
                outer.Add(new IntPoint(r.left - 10, r.bottom + 10));
                outer.Add(new IntPoint(r.right + 10, r.bottom + 10));
                outer.Add(new IntPoint(r.right + 10, r.top - 10));
                outer.Add(new IntPoint(r.left - 10, r.top - 10));
                clpr.AddPolygon(outer, PolyType.ptSubject);
                if (clpr.Execute(ClipType.ctUnion, result, PolyFillType.pftNonZero, PolyFillType.pftNonZero))
                    result.RemoveAt(0);
                else
                    result.Clear();
          }
          return result;
        }
        //------------------------------------------------------------------------------


    } //clipper namespace
  
    class ClipperException : Exception
    {
        private string m_description;
        public ClipperException(string description)
        {
            m_description = description;
            Console.WriteLine(m_description);
            throw new Exception(m_description);
        }
    }
    //------------------------------------------------------------------------------
}
