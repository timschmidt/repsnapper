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

/*******************************************************************************
*                                                                              *
* This is a translation of the Delphi Clipper library and the naming style     *
* used has retained a very strong Delphi flavour.                              *
*                                                                              *
*******************************************************************************/

using System;
using System.Collections.Generic;

namespace Clipper
{
    using TPolygon = List<TDoublePoint>;
    using TPolyPolygon = List<List<TDoublePoint>>;
    using PolyPtList = List<TPolyPt>;
    using JoinList = List<TJoinRec>;

    public enum TClipType { ctIntersection, ctUnion, ctDifference, ctXor };
    public enum TPolyType { ptSubject, ptClip };
    public enum TPolyFillType { pftEvenOdd, pftNonZero };

    //used internally ...
    enum TEdgeSide { esLeft, esRight };
    enum TDirection { dRightToLeft, dLeftToRight };
    [Flags]
    enum TProtects { ipNone = 0, ipLeft = 1, ipRight = 2, ipBoth = 3 };
    [Flags]
    enum TOrientationFlag {ofEmpty = 0, ofClockwise = 1, ofCW = 2,
        ofForwardBound = 4, ofTop = 8, ofBottomMinima = 16};

    public class TDoublePoint
    {
        public double X { get; set; }
        public double Y { get; set; }
        public TDoublePoint(double X = 0, double Y = 0)
        {
            this.X = X; this.Y = Y;
        }
    };

    public class TDoubleRect
    {
        public double left { get; set; }
        public double top { get; set; }
        public double right { get; set; }
        public double bottom { get; set; }
        public TDoubleRect(double left = 0, double top = 0, double right = 0, double bottom = 0)
        {
            this.left = left; this.top = top; this.right = right; this.bottom = bottom;
        }
    };

    internal class TEdge
    {
        public double x;
        public double y;
        public double xbot;
        public double ybot;
        public double xtop;
        public double ytop;
        public double dx;
        public double tmpX;
        public bool nextAtTop;
        public TPolyType polyType;
        public TEdgeSide side;
        public int windDelta; //1 or -1 depending on winding direction
        public int windCnt;
        public int windCnt2; //winding count of the opposite polytype
        public int outIdx;
        public TEdge next;
        public TEdge prev;
        public TEdge nextInLML;
        public TEdge nextInAEL;
        public TEdge prevInAEL;
        public TEdge nextInSEL;
        public TEdge prevInSEL;
    };

    internal class TIntersectNode
    {
        public TEdge edge1;
        public TEdge edge2;
        public TDoublePoint pt;
        public TIntersectNode next;
        public TIntersectNode prev;
    };

    internal class TLocalMinima
    {
        public double Y;
        public TEdge leftBound;
        public TEdge rightBound;
        public TLocalMinima nextLm;
    };

    internal class TScanbeam
    {
        public double Y;
        public TScanbeam nextSb;
    };

    internal class TPolyPt
    {
        public TDoublePoint pt;
        public TPolyPt next;
        public TPolyPt prev;
        public TOrientationFlag flags;
        public double dx;
    };

    internal class TJoinRec
    {
        public TDoublePoint pt;
        public int idx1;
        public int idx2;
        public TPolyPt outPPt; //horiz joins only
    }

    //------------------------------------------------------------------------------
    //------------------------------------------------------------------------------

    internal class SkipNode<T>
    {
        public T item;
        public int level;
        public SkipNode<T> prev;    //ie SkipNodes form a double linked list
        public SkipNode<T>[] next;
    };

    internal class SkipList<T>
    {
        public delegate int compareFunc(T item1, T item2);

        private int m_MaxLevel;
        private double m_SkipFrac;
        private int m_CurrentMaxLevel;
        private SkipNode<T> m_Base;
        private int m_Count;
        private SkipNode<T>[] m_Lvls;
        private compareFunc m_CompareFunc;
        private Random m_rand;

        public SkipList(compareFunc cf)
        {
            const double maxItems = 10000000; //10 Mill.
            const double skip = 4;
            m_CompareFunc = new  compareFunc(cf);
            m_rand = new Random();

            m_MaxLevel = Convert.ToInt32(Math.Ceiling(Math.Log(maxItems) / Math.Log(skip))); //MaxLevel =12
            m_SkipFrac = 1 / skip;

            //create and initialize the base node ...
            m_Base = new SkipNode<T>();
            m_Base.level = m_MaxLevel;
            m_Base.next = new SkipNode<T>[m_MaxLevel + 1];
            for (int i = 0; i <= m_MaxLevel; ++i) m_Base.next[i] = null;
            m_Base.prev = m_Base;

            m_CurrentMaxLevel = 0;
            m_Count = 0;
            m_Lvls = new SkipNode<T>[m_MaxLevel];
        }
        //------------------------------------------------------------------------------

        ~SkipList() //destructor
        {
            Clear();
            m_Base = null;
        }
        //------------------------------------------------------------------------------

        public SkipNode<T> NewNode(int level, T item)
        {
            SkipNode<T> result = new SkipNode<T>();
            result.next = new SkipNode<T> [level +1];
            result.level = level;
            result.item = item;
            m_Count++;
            return result;
        }
        //------------------------------------------------------------------------------

        public void Clear()
        {
            SkipNode<T> tmp = m_Base.prev;
            SkipNode<T> tmp2;
            if (tmp == null) 
                throw new skiplistException("oops!");

            while (tmp != m_Base)
            {
                tmp2 = tmp.prev;
                tmp = null;
                tmp = tmp2;
            }
            for (int i = 0; i < m_MaxLevel; ++i) m_Base.next[i] = null;
            m_CurrentMaxLevel = 0;
            m_Base.prev = m_Base;
            m_Count = 0;
        }
        //------------------------------------------------------------------------------

        public SkipNode<T> First()
        {
            if (m_Count == 0) return null; else return m_Base.next[0];
        }
        //------------------------------------------------------------------------------


        public SkipNode<T> Next(SkipNode<T> currentNode)
        {
            if (currentNode != null) return currentNode.next[0];
            else return null;
        }
        //------------------------------------------------------------------------------

        public SkipNode<T> Prev(SkipNode<T> currentNode)
        {
            if (currentNode != null)
            {
                if (currentNode.prev == m_Base) return null;
                else return currentNode.prev;
            }
            else return null;
        }
        //------------------------------------------------------------------------------

        public SkipNode<T> Last()
        {
            if (m_Base.prev == m_Base) return null;
            else return m_Base.prev;
        }
        //------------------------------------------------------------------------------

        public int Count()
        {
            return m_Count;
        }
        //------------------------------------------------------------------------------

        public SkipNode<T> InsertItem(T item)
        {
          for (int i = 0; i < m_MaxLevel; ++i) m_Lvls[i] = m_Base;
          SkipNode<T> priorNode = m_Base;

          int compareVal = 1;
          for (int i = m_CurrentMaxLevel; i >= 0; --i)
          {
            while (priorNode.next[i] !=null)
            {
              //avoid a few unnecessary compares ...
              if (compareVal > 0 || priorNode.next[i+1] != priorNode.next[i])
                compareVal = m_CompareFunc(priorNode.next[i].item, item);
              if (compareVal > 0) priorNode = priorNode.next[i];
              else break;
            }
            m_Lvls[i] = priorNode;
          }

          if (compareVal == 0)
            throw new skiplistException("Skiplist error: Duplicate items not allowed.");

          //get the level of the new node ...
          int newLevel = 0;
          while (newLevel <= m_CurrentMaxLevel && newLevel < m_MaxLevel - 1 &&
            m_rand.Next(1000) < m_SkipFrac *1000) newLevel++;
          if (newLevel > m_CurrentMaxLevel) m_CurrentMaxLevel = newLevel;

          //create the new node and rearrange links up to newLevel ...
          SkipNode<T> result = NewNode(newLevel, item);
          if (priorNode.next[0] != null)
            priorNode.next[0].prev = result; else
            m_Base.prev = result; //fBase.prev always points to the last node
          result.prev = priorNode;
          for (int i = 0; i <= newLevel; ++i)
          {
            result.next[i] = m_Lvls[i].next[i];
            m_Lvls[i].next[i] = result;
          }
          return result;
        }
        //------------------------------------------------------------------------------

        public SkipNode<T> FindItem(T item)
        {
          SkipNode<T> result = m_Base;
          int compareVal = 1;
          for (int i = m_CurrentMaxLevel; i >= 0; --i)
          {
            while (result.next[i] !=null)
            {
              if (compareVal > 0 || result.next[i+1] != result.next[i])
                compareVal = m_CompareFunc(result.next[i].item, item);
              if (compareVal <= 0) break;
              else result = result.next[i];
            }
            if (compareVal == 0) return result.next[i];
          }
          return null;
        }
        //------------------------------------------------------------------------------

        public bool DeleteItem(T item)
        {
          for (int i = 0; i <= m_CurrentMaxLevel; ++i) m_Lvls[i] = m_Base;
          SkipNode<T> priorNode = m_Base;

          //find the item ...
          int compareVal = 1;
          for (int i = m_CurrentMaxLevel; i >= 0; --i)
          {
            while (priorNode.next[i] != null)
            {
              if (compareVal > 0 || priorNode.next[i+1] != priorNode.next[i])
                compareVal = m_CompareFunc(priorNode.next[i].item, item);
              if (compareVal > 0) priorNode = priorNode.next[i];
              else break;
            }
            m_Lvls[i] = priorNode;
          }
          if (compareVal != 0) return false;

          SkipNode<T> delNode = priorNode.next[0];
          //if this's the only node at fCurrentMaxLevel, decrement fCurrentMaxLevel ...
          if (delNode.level > 0 && delNode.level == m_CurrentMaxLevel &&
            m_Lvls[delNode.level] == m_Base && delNode.next[delNode.level] == null)
              m_CurrentMaxLevel--;

          //fix up links before finally deleting the node ...
          for (int i = 0; i <= delNode.level; ++i)
            m_Lvls[i].next[i] = delNode.next[i];
          if (delNode.next[0] != null)
            delNode.next[0].prev = delNode.prev; else
            m_Base.prev = delNode.prev;
          delNode = null;
          m_Count--;
          return true;
        }
        //------------------------------------------------------------------------------

        public void Delete(SkipNode<T> delNode)
        {
          //this method doesn't call m_CompareFunc() ...
            for (int i = delNode.level + 1; i <= m_CurrentMaxLevel; ++i) m_Lvls[i] = null;
            SkipNode<T> nextNode = delNode;
            int lvl = delNode.level;
            while (nextNode.next[nextNode.level] != null && nextNode.level < m_CurrentMaxLevel)
            {
                nextNode = nextNode.next[nextNode.level];
                while (nextNode.level > lvl)
                {
                    lvl++;
                    m_Lvls[lvl] = nextNode;
                }
            }
            for (int i = 0; i <= delNode.level; ++i) m_Lvls[i] = delNode;

          SkipNode<T> priorNode = m_Base;
          for (int i = m_CurrentMaxLevel; i >= 0; --i)
          {
            while ( priorNode.next[i] != null && priorNode.next[i] != m_Lvls[i] )
              priorNode = priorNode.next[i];
            m_Lvls[i] = priorNode;
          }

          //if this's the only node at fCurrentMaxLevel, decrement fCurrentMaxLevel ...
          if (delNode.level > 0 && delNode.level == m_CurrentMaxLevel &&
          m_Lvls[delNode.level] == m_Base && delNode.next[delNode.level] == null)
            m_CurrentMaxLevel--;

          //fix up links before finally deleting the node ...
          for (int i = 0; i <= delNode.level; ++i)
              m_Lvls[i].next[i] = delNode.next[i];
          if (delNode.next[0] != null)
              delNode.next[0].prev = delNode.prev;
          else
              m_Base.prev = delNode.prev;
          delNode = null;
          m_Count--;
        }
        //------------------------------------------------------------------------------

        public T PopFirst()
        {
            if (m_Count == 0) return default(T);
            SkipNode<T> delNode = m_Base.next[0];
            T result = delNode.item;

            int delLevel = delNode.level;
            //if this's the only node at fCurrentMaxLevel, decrement fCurrentMaxLevel ...
            if (delLevel > 0 && delLevel == m_CurrentMaxLevel &&
                m_Base.next[delLevel] == delNode && delNode.next[delLevel] == null)
                        m_CurrentMaxLevel--;

            //fix up links before finally deleting the node ...
            for (int i = 0; i <= delLevel; ++i) m_Base.next[i] = delNode.next[i];
            if (delNode.next[0] != null)
                delNode.next[0].prev = m_Base; else m_Base.prev = m_Base;
            delNode = null;
            m_Count--;
            return result;
        }
        //------------------------------------------------------------------------------
        //------------------------------------------------------------------------------
    }

    class skiplistException : Exception
    {
        private string m_description;
        public skiplistException(string description)
        {
            m_description = description;
            Console.WriteLine(m_description);
            throw new Exception(m_description);
        }
    }
    //------------------------------------------------------------------------------
    //------------------------------------------------------------------------------

    public class TClipperBase
    {

        protected const double horizontal = -3.4E+38;
        protected const double unassigned = 3.4E+38;

        //tolerance: is needed because vertices are floating point values and any
        //comparison of floating point values requires a degree of tolerance. 
        protected const double tolerance = 1.0E-10;
        protected const double minimal_tolerance = 1.0E-14;
        //precision: defines when adjacent vertices will be considered duplicates
        //and hence ignored. This circumvents edges having indeterminate slope.
        protected const double precision = 1.0E-6;
        protected const double slope_precision = 1.0E-3;

        internal TLocalMinima m_localMinimaList;
        internal TLocalMinima m_CurrentLM;
        private List<TEdge> m_edges = new List<TEdge>();

        internal static bool PointsEqual(TDoublePoint pt1, TDoublePoint pt2)
        {
            return (Math.Abs(pt1.X - pt2.X) < precision + tolerance && 
                Math.Abs(pt1.Y - pt2.Y) < precision + tolerance);
        }
        //------------------------------------------------------------------------------

        internal static bool PointsEqual(double pt1x, double pt1y, double pt2x, double pt2y)
        {
            return (Math.Abs(pt1x - pt2x) < precision + tolerance && 
                Math.Abs(pt1y - pt2y) < precision + tolerance);
        }
        //------------------------------------------------------------------------------

        internal static void DisposePolyPts(TPolyPt pp)
        {
            if (pp == null)
                return;
            TPolyPt tmpPp = null;
            pp.prev.next = null;
            while (pp != null)
            {
                tmpPp = pp;
                pp = pp.next;
                tmpPp = null;
            }
        }
        //------------------------------------------------------------------------------

        internal static void ReversePolyPtLinks(TPolyPt pp)
        {
            TPolyPt pp1;
            TPolyPt pp2;
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

        internal static void SetDx(TEdge e)
        {
            double dx = Math.Abs(e.x - e.next.x);
            double dy = Math.Abs(e.y - e.next.y);
            //Very short, nearly horizontal edges can cause problems by very
            //inaccurately determining intermediate X values - see TopX().
            //Therefore treat very short, nearly horizontal edges as horizontal too ...
            if ((dx < 0.1 && dy * 10 < dx) || dy < slope_precision)
            {
                e.dx = horizontal;
                if (e.y != e.next.y) e.y = e.next.y;
            }
            else 
                e.dx = (e.x - e.next.x) / (e.y - e.next.y);
        }
        //------------------------------------------------------------------------------

        internal static bool IsHorizontal(TEdge e)
        {
            return (e != null) && (e.dx == horizontal);
        }
        //------------------------------------------------------------------------------

        internal static void SwapSides(TEdge edge1, TEdge edge2)
        {
            TEdgeSide side = edge1.side;
            edge1.side = edge2.side;
            edge2.side = side;
        }
        //------------------------------------------------------------------------------

        internal static void SwapPolyIndexes(TEdge edge1, TEdge edge2)
        {
            int outIdx = edge1.outIdx;
            edge1.outIdx = edge2.outIdx;
            edge2.outIdx = outIdx;
        }
        //------------------------------------------------------------------------------

        internal static double TopX(TEdge edge, double currentY)
        {
            if (currentY == edge.ytop)
                return edge.xtop;
            return edge.x + edge.dx * (currentY - edge.y);
        }
        //------------------------------------------------------------------------------

        internal static bool EdgesShareSamePoly(TEdge e1, TEdge e2)
        {
            return (e1 != null) && (e2 != null) && (e1.outIdx == e2.outIdx);
        }
        //------------------------------------------------------------------------------

        internal static bool SlopesEqual(TEdge e1, TEdge e2)
        {
            if (IsHorizontal(e1))
                return IsHorizontal(e2);
            if (IsHorizontal(e2))
                return false;
            return Math.Abs((e1.ytop - e1.y) * (e2.xtop - e2.x) - 
                (e1.xtop - e1.x) * (e2.ytop - e2.y)) < slope_precision;
        }
        //------------------------------------------------------------------------------

        internal static bool IntersectPoint(TEdge edge1, TEdge edge2, out TDoublePoint ip)
        {
            ip = new TDoublePoint();
            double b1, b2;
            if (edge1.dx == 0)
            {
                ip.X = edge1.x;
                b2 = edge2.y - edge2.x / edge2.dx;
                ip.Y = ip.X / edge2.dx + b2;
            }
            else if (edge2.dx == 0)
            {
                ip.X = edge2.x;
                b1 = edge1.y - edge1.x / edge1.dx;
                ip.Y = ip.X / edge1.dx + b1;
            }
            else
            {
                b1 = edge1.x - edge1.y * edge1.dx;
                b2 = edge2.x - edge2.y * edge2.dx;
                ip.Y = (b2 - b1) / (edge1.dx - edge2.dx);
                ip.X = edge1.dx * ip.Y + b1;
            }
            return (ip.Y > edge1.ytop + tolerance) && (ip.Y > edge2.ytop + tolerance);
        }
        //------------------------------------------------------------------------------

        internal static void InitEdge(TEdge e, TEdge eNext, TEdge ePrev, TDoublePoint pt)
        {
            e.x = pt.X;
            e.y = pt.Y;
            e.next = eNext;
            e.prev = ePrev;
            SetDx(e);
        }
        //------------------------------------------------------------------------------

        internal static void ReInitEdge(TEdge e, double nextX, double nextY, TPolyType polyType)
        {
            if (e.y > nextY)
            {
                e.xbot = e.x;
                e.ybot = e.y;
                e.xtop = nextX;
                e.ytop = nextY;
                e.nextAtTop = true;
            }
            else
            {
                e.xbot = nextX;
                e.ybot = nextY;
                e.xtop = e.x;
                e.ytop = e.y;
                e.x = e.xbot;
                e.y = e.ybot;
                e.nextAtTop = false;
            }
            e.polyType = polyType;
            e.outIdx = -1;
        }
        //------------------------------------------------------------------------------

        internal static bool SlopesEqualInternal(TEdge e1, TEdge e2)
        {
            if (IsHorizontal(e1))
                return IsHorizontal(e2);
            if (IsHorizontal(e2)) 
                return false;
            return Math.Abs((e1.y - e1.next.y) * (e2.x - e2.next.x) - 
                (e1.x - e1.next.x) * (e2.y - e2.next.y)) < slope_precision;
        }
        //------------------------------------------------------------------------------

        internal static void FixupForDupsAndColinear(TEdge edges)
        {
            TEdge lastOK = null;
            TEdge e = edges;
            for (;;)
            {
                if (e.next == e.prev) break;
                else if (PointsEqual(e.prev.x, e.prev.y, e.x, e.y) || SlopesEqualInternal(e.prev, e))
                {
                    lastOK = null;
                    //remove 'e' from the double-linked-list ...
                    if (e == edges)
                    {
                        //move the content of e.next to e before removing e.next from DLL ...
                        e.x = e.next.x;
                        e.y = e.next.y;
                        e.next.next.prev = e;
                        e.next = e.next.next;
                    }
                    else
                    {
                        //remove 'e' from the loop ...
                        e.prev.next = e.next;
                        e.next.prev = e.prev;
                        e = e.prev; //ie get back into the loop
                    }
                    SetDx(e.prev);
                    SetDx(e);
                }
                else if (lastOK == e) break;
                else
                {
                    if (lastOK == null) lastOK = e;
                    e = e.next;
                }
            }
        }
        //------------------------------------------------------------------------------

        internal static void SwapX(TEdge e)
        {
            //swap horizontal edges' top and bottom x's so they follow the natural
            //progression of the bounds - ie so their xbots will align with the
            //adjoining lower edge. [Helpful in the ProcessHorizontal() method.]
            e.xbot = e.xtop;
            e.xtop = e.x;
            e.x = e.xbot;
            e.nextAtTop = !e.nextAtTop; //but really redundant for horizontals
        }
        //------------------------------------------------------------------------------

        public TClipperBase() //constructor
        {
            m_localMinimaList = null;
            m_CurrentLM = null;
        }
        //------------------------------------------------------------------------------

        ~TClipperBase() //destructor
        {
            Clear();
        }
        //------------------------------------------------------------------------------

        void InsertLocalMinima(TLocalMinima newLm)
        {
            //nb: we'll make sure horizontal minima are sorted below other minima 
            //    of equal Y so that windings will be properly calculated ...
            if (m_localMinimaList == null)
                m_localMinimaList = newLm;
            else if (newLm.Y >= m_localMinimaList.Y) 
            {
                newLm.nextLm = m_localMinimaList;
                m_localMinimaList = newLm;
            }
            else
            {
                TLocalMinima tmpLm = m_localMinimaList;
                while (tmpLm.nextLm != null && (newLm.Y < tmpLm.nextLm.Y))
                    tmpLm = tmpLm.nextLm;
                newLm.nextLm = tmpLm.nextLm;
                tmpLm.nextLm = newLm;
            }
        }
        //------------------------------------------------------------------------------
        
        TEdge AddBoundsToLML(TEdge e)
        {
            //Starting at the top of one bound we progress to the bottom where there's
            //a local minima. We then go to the top of the next bound. These two bounds
            //form the left and right (or right and left) bounds of the local minima.
            e.nextInLML = null;
            e = e.next;
            for (; ; )
            {
                if (IsHorizontal(e))
                {
                    //nb: proceed through horizontals when approaching from their right,
                    //    but break on horizontal minima if approaching from their left.
                    //    This ensures 'local minima' are always on the left of horizontals.
                    if (e.next.ytop < e.ytop && e.next.xbot > e.prev.xbot) 
                        break;
                    if (e.xtop != e.prev.xbot)
                        SwapX(e);
                    e.nextInLML = e.prev;
                }
                else if (e.ybot == e.prev.ybot)
                    break;
                else e.nextInLML = e.prev;
                e = e.next;
            }

            //e and e.prev are now at a local minima ...
            TLocalMinima newLm = new TLocalMinima { nextLm = null, Y = e.prev.ybot };

            if (IsHorizontal(e)) //horizontal edges never start a left bound
            {
                if (e.xbot != e.prev.xbot)
                    SwapX(e);
                newLm.leftBound = e.prev;
                newLm.rightBound = e;
            }
            else if (e.dx < e.prev.dx)
            {
                newLm.leftBound = e.prev;
                newLm.rightBound = e;
            }
            else
            {
                newLm.leftBound = e;
                newLm.rightBound = e.prev;
            }
            newLm.leftBound.side = TEdgeSide.esLeft;
            newLm.rightBound.side = TEdgeSide.esRight;
            InsertLocalMinima(newLm);

            for (; ; )
            {
                if (e.next.ytop == e.ytop && !IsHorizontal(e.next)) 
                    break;
                e.nextInLML = e.next;
                e = e.next;
                if (IsHorizontal(e) && e.xbot != e.prev.xtop)
                    SwapX(e);
            }
            return e.next;
        }
        //------------------------------------------------------------------------------

        internal static TDoublePoint RoundToPrecision(TDoublePoint pt)
        {
            TDoublePoint result = new TDoublePoint();
            result.X = (pt.X >= 0.0) ?
              (Math.Floor(pt.X / precision + 0.5) * precision) :
              (Math.Ceiling(pt.X / precision + 0.5) * precision);
            result.Y = (pt.Y >= 0.0) ?
              (Math.Floor(pt.Y / precision + 0.5) * precision) :
              (Math.Ceiling(pt.Y / precision + 0.5) * precision);
            return result;
        }
        //------------------------------------------------------------------------------

        public virtual void AddPolygon(TPolygon pg, TPolyType polyType)
        {
            int highI = pg.Count - 1;
            TPolygon p = new TPolygon(highI + 1);
            for (int i = 0; i <= highI; ++i)
                p.Add(RoundToPrecision(pg[i]));
            while ((highI > 1) && PointsEqual(p[0], p[highI])) 
                highI--;
            if (highI < 2) 
                return;

            //make sure this is still a sensible polygon (ie with at least one minima) ...
            int j = 1;
            while (j <= highI && Math.Abs(p[j].Y - p[0].Y) < precision)
                j++;

            if (j > highI)
                return;

            //create a new edge array ...
            List<TEdge> edges = new List<TEdge>();
            for (int i = 0; i < highI + 1; i++)
                edges.Add(new TEdge());
            m_edges.AddRange(edges);

            //convert 'edges' to a double-linked-list and initialize a few of the vars ...
            edges[0].x = p[0].X;
            edges[0].y = p[0].Y;


            TEdge edgeRef = edges[highI];
            InitEdge(edgeRef, edges[0], edges[highI - 1], p[highI]);
            for (int i = highI - 1; i > 0; --i)
            {
                edgeRef = edges[i];
                InitEdge(edgeRef, edges[i + 1], edges[i - 1], p[i]);
            }
            TEdge e = edges[highI];
            while (IsHorizontal(e) && e != edges[0])
            {
                if (e.y != e.next.y) e.y = e.next.y;
                e = e.prev;
            }

            edgeRef = edges[0];
            InitEdge(edgeRef, edges[1], edges[highI], p[0]);

            //fixup by deleting any duplicate points and amalgamating co-linear edges ...
            FixupForDupsAndColinear(edges[0]);
            e = edges[0];

            //make sure we still have a valid polygon ...
            if (e.next == e.prev)
            {
                m_edges.RemoveAt(m_edges.Count - 1);
                return;
            }

            //now properly re-initialize edges and also find 'eHighest' ...
            e = edges[0].next;
            TEdge eHighest = e;
            do
            {
                ReInitEdge(e, e.next.x, e.next.y, polyType);
                if (e.ytop < eHighest.ytop) eHighest = e;
                e = e.next;
            } while (e != edges[0]);

            if (e.next.nextAtTop)
                ReInitEdge(e, e.next.x, e.next.y, polyType);
            else
                ReInitEdge(e, e.next.xtop, e.next.ytop, polyType);
            if (e.ytop < eHighest.ytop) eHighest = e;

            //make sure eHighest is positioned so the following loop works safely ...
            if (eHighest.nextAtTop) eHighest = eHighest.next;
            if (IsHorizontal(eHighest))
                eHighest = eHighest.next;

            //finally insert each local minima ...
            e = eHighest;
            do
            {
                e = AddBoundsToLML(e);
            } while (e != eHighest);

        }
        //------------------------------------------------------------------------------

        public virtual void AddPolyPolygon(TPolyPolygon ppg, TPolyType polyType)
        {
            for (int i = 0; i < ppg.Count; ++i)
                AddPolygon(ppg[i], polyType);
        }
        //------------------------------------------------------------------------------
    
        public void Clear()
        {
            DisposeLocalMinimaList();
            m_edges.Clear();
        }
        //------------------------------------------------------------------------------

        internal bool Reset()
        {
            m_CurrentLM = m_localMinimaList;
            if (m_CurrentLM == null)
                return false; //ie nothing to process

            //reset all edges ...
            TLocalMinima lm = m_localMinimaList;
            while (lm != null)
            {
                TEdge e = lm.leftBound;
                while (e != null)
                {
                    e.xbot = e.x;
                    e.ybot = e.y;
                    e.side = TEdgeSide.esLeft;
                    e.outIdx = -1;
                    e = e.nextInLML;
                }
                e = lm.rightBound;
                while (e != null)
                {
                    e.xbot = e.x;
                    e.ybot = e.y;
                    e.side = TEdgeSide.esRight;
                    e.outIdx = -1;
                    e = e.nextInLML;
                }
                lm = lm.nextLm;
            }
            return true;
        }
        //------------------------------------------------------------------------------
        
        internal void PopLocalMinima()
        {
            if (m_CurrentLM == null)
                return;
            m_CurrentLM = m_CurrentLM.nextLm;
        }
        //------------------------------------------------------------------------------
        
        void DisposeLocalMinimaList()
        {
            while (m_localMinimaList != null)
            {
                TLocalMinima tmpLm = m_localMinimaList.nextLm;
                m_localMinimaList = null;
                m_localMinimaList = tmpLm;
            }
            m_CurrentLM = null; 
        }
        //------------------------------------------------------------------------------

        public TDoubleRect GetBounds()
        {
            TLocalMinima lm = m_localMinimaList;
            if (lm == null) return new TDoubleRect(0, 0, 0, 0);

            TDoubleRect result = new TDoubleRect(unassigned, unassigned, -unassigned, -unassigned);
            while (lm != null)
            {
                if (lm.leftBound.y > result.bottom) result.bottom = lm.leftBound.y;
                TEdge e = lm.leftBound;
                while (e.nextInLML != null)
                {
                    if (e.x < result.left) result.left = e.x;
                    e = e.nextInLML;
                }
                if (e.x < result.left) result.left = e.x;
                else if (e.xtop < result.left) result.left = e.xtop;
                if (e.ytop < result.top) result.top = e.ytop;

                e = lm.rightBound;
                while (e.nextInLML != null)
                {
                    if (e.x > result.right) result.right = e.x;
                    e = e.nextInLML;
                }
                if (e.x > result.right) result.right = e.x;
                else if (e.xtop > result.right) result.right = e.xtop;

                lm = lm.nextLm;
            }
            return result;
        }
        //------------------------------------------------------------------------------

    }
    //------------------------------------------------------------------------------
    //------------------------------------------------------------------------------

    public class TClipper : TClipperBase
    {

        public static TDoubleRect GetBounds(TPolygon poly)
        {
            if (poly.Count == 0) return new TDoubleRect(0, 0, 0, 0);
            
            TDoubleRect result = new TDoubleRect(poly[0].X, poly[0].Y, poly[0].X, poly[0].Y);
            for (int i = 1; i < poly.Count; ++i)
            {
                if (poly[i].X < result.left) result.left = poly[i].X;
                else if (poly[i].X > result.right) result.right = poly[i].X;
                if (poly[i].Y < result.top) result.top = poly[i].Y;
                else if (poly[i].Y > result.bottom) result.bottom = poly[i].Y;
            }
            return result;
        }
        //------------------------------------------------------------------------------

        public static bool IsClockwise(TPolygon poly)
        {
          int highI = poly.Count -1;
          if (highI < 2) return false;
          double area = poly[highI].X * poly[0].Y - poly[0].X * poly[highI].Y;
          for (int i = 0; i < highI; ++i)
            area += poly[i].X * poly[i+1].Y - poly[i+1].X * poly[i].Y;
          //area := area/2;
          return area > 0; //ie reverse of normal formula because Y axis inverted
        }
        //------------------------------------------------------------------------------
        //------------------------------------------------------------------------------

        private PolyPtList m_PolyPts;
        private JoinList m_Joins;
        private JoinList m_CurrentHorizontals;
        private TClipType m_ClipType;
        private TScanbeam m_Scanbeam;
        private TEdge m_ActiveEdges;
        private TEdge m_SortedEdges;
        private TIntersectNode m_IntersectNodes;
        private bool m_ExecuteLocked;
        private TPolyFillType m_ClipFillType;
        private TPolyFillType m_SubjFillType;
        private double m_IntersectTolerance;
        private bool m_IgnoreOrientation;
        public bool IgnoreOrientation { get { return m_IgnoreOrientation; } set { m_IgnoreOrientation = value; } }
        //------------------------------------------------------------------------------

        public TClipper()
        {
            m_Scanbeam = null;
            m_ActiveEdges = null;
            m_SortedEdges = null;
            m_IntersectNodes = null;
            m_ExecuteLocked = false;
            m_IgnoreOrientation = false;
            m_PolyPts = new PolyPtList();
            m_Joins = new JoinList();
            m_CurrentHorizontals = new JoinList();
        }
        //------------------------------------------------------------------------------
        
        private void DisposeAllPolyPts()
        {
            for (int i = 0; i < m_PolyPts.Count; ++i)
                DisposePolyPts(m_PolyPts[i]);
            m_PolyPts.Clear();
        }
        //------------------------------------------------------------------------------

        private void DisposeScanbeamList()
        {
            while (m_Scanbeam != null)
            {
                TScanbeam sb2 = m_Scanbeam.nextSb;
                m_Scanbeam = null;
                m_Scanbeam = sb2;
            }
        }
        //------------------------------------------------------------------------------
        
        public override void AddPolygon(TPolygon pg, TPolyType polyType)
        {
            base.AddPolygon(pg, polyType);
        }
        //------------------------------------------------------------------------------
        
        public override void AddPolyPolygon(TPolyPolygon ppg, TPolyType polyType)
        {
            base.AddPolyPolygon(ppg, polyType);
        }
        //------------------------------------------------------------------------------

        public static TDoublePoint GetUnitNormal(TDoublePoint pt1, TDoublePoint pt2)
        {
            double dx = (pt2.X - pt1.X);
            double dy = (pt2.Y - pt1.Y);
            if ((dx == 0) && (dy == 0))
                return new TDoublePoint(0, 0);

            //double f = 1 *1.0/ hypot( dx , dy );
            double f = 1 * 1.0 / Math.Sqrt(dx * dx + dy * dy);
            dx = dx * f;
            dy = dy * f;
            return new TDoublePoint(dy, -dx);
        }
        //------------------------------------------------------------------------------

        private bool IsClockwise(TPolyPt pt)
        {
            double area = 0;
            TPolyPt startPt = pt;
            do
            {
                area = area + (pt.pt.X * pt.next.pt.Y) - (pt.next.pt.X * pt.pt.Y);
                pt = pt.next;
            }
            while (pt != startPt);
            //area = area /2;
            return area > 0; //ie reverse of normal formula because Y axis inverted
        }
        //------------------------------------------------------------------------------

        private bool InitializeScanbeam()
        {
            DisposeScanbeamList();
            if (!Reset())
                return false;
            //add all the local minima into a fresh fScanbeam list ...
            TLocalMinima lm = m_CurrentLM;
            while (lm != null)
            {
                InsertScanbeam(lm.Y);
                InsertScanbeam(lm.leftBound.ytop); //this is necessary too!
                lm = lm.nextLm;
            }
            return true;
        }
        //------------------------------------------------------------------------------

        private void InsertScanbeam(double Y)
        {
            if (m_Scanbeam == null)
            {
                m_Scanbeam = new TScanbeam { Y = Y, nextSb = null };
            }
            else if (Y > m_Scanbeam.Y)
            {
                m_Scanbeam = new TScanbeam { Y = Y, nextSb = m_Scanbeam };
            }
            else
            {
                TScanbeam sb2 = m_Scanbeam;
                while (sb2.nextSb != null && (Y <= sb2.nextSb.Y))
                    sb2 = sb2.nextSb;
                if (Y == sb2.Y)
                    return; //ie ignores duplicates
                TScanbeam newSb = new TScanbeam { Y = Y, nextSb = sb2.nextSb };
                sb2.nextSb = newSb;
            }
        }
        //------------------------------------------------------------------------------

        private double PopScanbeam()
        {
            double Y = m_Scanbeam.Y;
            TScanbeam sb2 = m_Scanbeam;
            m_Scanbeam = m_Scanbeam.nextSb;
            return Y;
        }
        //------------------------------------------------------------------------------

        private void SetWindingDelta(TEdge edge)
        {
            if (!IsNonZeroFillType(edge))
                edge.windDelta = 1;
            else if (edge.nextAtTop)
                edge.windDelta = 1;
            else 
                edge.windDelta = -1;
        }
        //------------------------------------------------------------------------------

        private void SetWindingCount(TEdge edge)
        {
            TEdge e = edge.prevInAEL;
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

        private bool IsNonZeroFillType(TEdge edge)
        {
            switch (edge.polyType)
            {
                case TPolyType.ptSubject:
                    return m_SubjFillType == TPolyFillType.pftNonZero;
                default:
                    return m_ClipFillType == TPolyFillType.pftNonZero;
            }
        }
        //------------------------------------------------------------------------------

        private bool IsNonZeroAltFillType(TEdge edge)
        {
            switch (edge.polyType)
            {
                case TPolyType.ptSubject:
                    return m_ClipFillType == TPolyFillType.pftNonZero;
                default: 
                    return m_SubjFillType == TPolyFillType.pftNonZero;
            }
        }
        //------------------------------------------------------------------------------

        private bool Edge2InsertsBeforeEdge1(TEdge e1, TEdge e2)
        {
            if (e2.xbot - tolerance > e1.xbot) return false;
            if (e2.xbot + tolerance < e1.xbot) return true;
            if (IsHorizontal(e2)) return false;
            return (e2.dx > e1.dx);
        }
        //------------------------------------------------------------------------------

        private void InsertEdgeIntoAEL(TEdge edge)
        {
            edge.prevInAEL = null;
            edge.nextInAEL = null;
            if (m_ActiveEdges == null)
            {
                m_ActiveEdges = edge;
            }
            else if (Edge2InsertsBeforeEdge1(m_ActiveEdges, edge))
            {
                edge.nextInAEL = m_ActiveEdges;
                m_ActiveEdges.prevInAEL = edge;
                m_ActiveEdges = edge;
            }
            else
            {
                TEdge e = m_ActiveEdges;
                while (e.nextInAEL != null && !Edge2InsertsBeforeEdge1(e.nextInAEL, edge))
                    e = e.nextInAEL;
                edge.nextInAEL = e.nextInAEL;
                if (e.nextInAEL != null)
                    e.nextInAEL.prevInAEL = edge;
                edge.prevInAEL = e;
                e.nextInAEL = edge;
            }
        }
        //------------------------------------------------------------------------------

        bool HorizOverlap(double h1a, double h1b, double h2a, double h2b)
        {
            //returns true if (h1a between h2a and h2b) or
            //  (h1a == min2 and h1b > min2) or (h1a == max2 and h1b < max2)
            double min2, max2;
            if (h2a < h2b)
            {
                min2 = h2a;
                max2 = h2b;
            }
            else
            {
                min2 = h2b;
                max2 = h2a;
            }
            return (h1a > min2 + tolerance && h1a < max2 - tolerance) ||
              (Math.Abs(h1a - min2) < tolerance && h1b > min2 + tolerance) ||
              (Math.Abs(h1a - max2) < tolerance && h1b < max2 - tolerance);
        }
        //------------------------------------------------------------------------------

        private void InsertLocalMinimaIntoAEL(double botY)
        {
            while (m_CurrentLM != null && m_CurrentLM.Y == botY)            
            {
                InsertEdgeIntoAEL(m_CurrentLM.leftBound);
                InsertScanbeam(m_CurrentLM.leftBound.ytop);
                InsertEdgeIntoAEL(m_CurrentLM.rightBound);

                SetWindingDelta(m_CurrentLM.leftBound);
                if (IsNonZeroFillType(m_CurrentLM.leftBound))
                    m_CurrentLM.rightBound.windDelta =
                      -m_CurrentLM.leftBound.windDelta;
                else
                    m_CurrentLM.rightBound.windDelta = 1;

                SetWindingCount(m_CurrentLM.leftBound);
                m_CurrentLM.rightBound.windCnt =
                  m_CurrentLM.leftBound.windCnt;
                m_CurrentLM.rightBound.windCnt2 =
                  m_CurrentLM.leftBound.windCnt2;

                if (IsHorizontal(m_CurrentLM.rightBound))
                {
                    //nb: only rightbounds can have a horizontal bottom edge
                    AddEdgeToSEL(m_CurrentLM.rightBound);
                    InsertScanbeam(m_CurrentLM.rightBound.nextInLML.ytop);
                }
                else
                    InsertScanbeam(m_CurrentLM.rightBound.ytop);

                TLocalMinima lm = m_CurrentLM;
                if (IsContributing(lm.leftBound))
                    AddLocalMinPoly(lm.leftBound, lm.rightBound, new TDoublePoint(lm.leftBound.xbot, lm.Y));

                //flag polygons that share colinear edges, so they can be merged later ...
                if (lm.leftBound.outIdx >= 0 && lm.leftBound.prevInAEL != null &&
                  lm.leftBound.prevInAEL.outIdx >= 0 &&
                  Math.Abs(lm.leftBound.prevInAEL.xbot - lm.leftBound.x) < tolerance &&
                  SlopesEqual(lm.leftBound, lm.leftBound.prevInAEL))
                {
                    TDoublePoint pt = new TDoublePoint(lm.leftBound.x, lm.leftBound.y);
                    TJoinRec polyPtRec = new TJoinRec();
                    AddPolyPt(lm.leftBound.prevInAEL, pt);
                    polyPtRec.idx1 = lm.leftBound.outIdx;
                    polyPtRec.idx2 = lm.leftBound.prevInAEL.outIdx;
                    polyPtRec.pt = pt;
                    m_Joins.Add(polyPtRec);
                }

                if (lm.rightBound.outIdx >= 0 && IsHorizontal(lm.rightBound))
                {
                    //check for overlap with m_CurrentHorizontals
                    for (int i = 0; i < m_CurrentHorizontals.Count; ++i)
                    {
                        int hIdx = m_CurrentHorizontals[i].idx1;
                        TDoublePoint hPt = m_CurrentHorizontals[i].outPPt.pt;
                        TDoublePoint hPt2 = m_CurrentHorizontals[i].pt;
                        TPolyPt p = m_CurrentHorizontals[i].outPPt;

                        TPolyPt p2;
                        if (IsHorizontal(p, p.prev) && (p.prev.pt.X == hPt2.X)) p2 = p.prev;
                        else if (IsHorizontal(p, p.next) && (p.next.pt.X == hPt2.X)) p2 = p.next;
                        else continue;

                        if (HorizOverlap(hPt.X, p2.pt.X, lm.rightBound.x, lm.rightBound.xtop))
                        {
                            AddPolyPt(lm.rightBound, hPt);
                            TJoinRec polyPtRec = new TJoinRec();
                            polyPtRec.idx1 = hIdx;
                            polyPtRec.idx2 = lm.rightBound.outIdx;
                            polyPtRec.pt = hPt;
                            m_Joins.Add(polyPtRec);
                        }
                        else if (HorizOverlap(lm.rightBound.x, lm.rightBound.xtop, hPt.X, hPt2.X))
                        {
                            TDoublePoint pt = new TDoublePoint(lm.rightBound.x, lm.rightBound.y);
                            TJoinRec polyPtRec = new TJoinRec();
                            if (!PointsEqual(pt, p.pt) && !PointsEqual(pt, p2.pt))
                                InsertPolyPtBetween(pt, p, p2);
                            polyPtRec.idx1 = hIdx;
                            polyPtRec.idx2 = lm.rightBound.outIdx;
                            polyPtRec.pt = pt;
                            m_Joins.Add(polyPtRec);                            
                        }
                    }
                }


                if (lm.leftBound.nextInAEL != lm.rightBound)
                {
                    TEdge e = lm.leftBound.nextInAEL;
                    TDoublePoint pt = new TDoublePoint(lm.leftBound.xbot, lm.leftBound.ybot);
                    while (e != lm.rightBound)
                    {
                        if (e == null)
                            throw new clipperException("InsertLocalMinimaIntoAEL: missing rightbound!");
                        //nb: For calculating winding counts etc, IntersectEdges() assumes
                        //that param1 will be to the right of param2 ABOVE the intersection ...
                        TEdge edgeRef = lm.rightBound;
                        IntersectEdges(edgeRef, e, pt, 0); //order important here
                        e = e.nextInAEL;
                    }
                }
                PopLocalMinima();
            }
            m_CurrentHorizontals.Clear();
        }
        //------------------------------------------------------------------------------

        private void AddEdgeToSEL(TEdge edge)
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
            TEdge e = m_ActiveEdges;
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

        private void SwapPositionsInAEL(TEdge edge1, TEdge edge2)
        {
            if (edge1.nextInAEL == null && edge1.prevInAEL == null)
                return;
            if (edge2.nextInAEL == null && edge2.prevInAEL == null)
                return;

            if (edge1.nextInAEL == edge2)
            {
                TEdge next = edge2.nextInAEL;
                if (next != null)
                    next.prevInAEL = edge1;
                TEdge prev = edge1.prevInAEL;
                if (prev != null)
                    prev.nextInAEL = edge2;
                edge2.prevInAEL = prev;
                edge2.nextInAEL = edge1;
                edge1.prevInAEL = edge2;
                edge1.nextInAEL = next;
            }
            else if (edge2.nextInAEL == edge1)
            {
                TEdge next = edge1.nextInAEL;
                if (next != null)
                    next.prevInAEL = edge2;
                TEdge prev = edge2.prevInAEL;
                if (prev != null)
                    prev.nextInAEL = edge1;
                edge1.prevInAEL = prev;
                edge1.nextInAEL = edge2;
                edge2.prevInAEL = edge1;
                edge2.nextInAEL = next;
            }
            else
            {
                TEdge next = edge1.nextInAEL;
                TEdge prev = edge1.prevInAEL;
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
        
        private void SwapPositionsInSEL(TEdge edge1, TEdge edge2)
        {
            if (edge1.nextInSEL == null && edge1.prevInSEL == null)
                return;
            if (edge2.nextInSEL == null && edge2.prevInSEL == null)
                return;

            if (edge1.nextInSEL == edge2)
            {
                TEdge next = edge2.nextInSEL;
                if (next != null)
                    next.prevInSEL = edge1;
                TEdge prev = edge1.prevInSEL;
                if (prev != null)
                    prev.nextInSEL = edge2;
                edge2.prevInSEL = prev;
                edge2.nextInSEL = edge1;
                edge1.prevInSEL = edge2;
                edge1.nextInSEL = next;
            }
            else if (edge2.nextInSEL == edge1)
            {
                TEdge next = edge1.nextInSEL;
                if (next != null)
                    next.prevInSEL = edge2;
                TEdge prev = edge2.prevInSEL;
                if (prev != null)
                    prev.nextInSEL = edge1;
                edge1.prevInSEL = prev;
                edge1.nextInSEL = edge2;
                edge2.prevInSEL = edge1;
                edge2.nextInSEL = next;
            }
            else
            {
                TEdge next = edge1.nextInSEL;
                TEdge prev = edge1.prevInSEL;
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

        private TEdge GetNextInAEL(TEdge e, TDirection Direction)
        {
            if (Direction == TDirection.dLeftToRight)
                return e.nextInAEL;
            else
                return e.prevInAEL;
        }
        //------------------------------------------------------------------------------

        private TEdge GetPrevInAEL(TEdge e, TDirection Direction)
        {
            if (Direction == TDirection.dLeftToRight)
                return e.prevInAEL;
            else return e.nextInAEL;
        }
        //------------------------------------------------------------------------------

        private bool IsMinima(TEdge e)
        {
            return e != null && (e.prev.nextInLML != e) && (e.next.nextInLML != e);
        }
        //------------------------------------------------------------------------------

        private bool IsMaxima(TEdge e, double Y)
        {
            return e != null && Math.Abs(e.ytop - Y) < tolerance && e.nextInLML == null;
        }
        //------------------------------------------------------------------------------

        private bool IsIntermediate(TEdge e, double Y)
        {
            return Math.Abs(e.ytop - Y) < tolerance && e.nextInLML != null;
        }
        //------------------------------------------------------------------------------

        private TEdge GetMaximaPair(TEdge e)
        {
            if (!IsMaxima(e.next, e.ytop) || (e.next.xtop != e.xtop))
                return e.prev;
            else
                return e.next;
        }
        //------------------------------------------------------------------------------

        private void DoMaxima(TEdge e, double topY)
        {
            TEdge eMaxPair = GetMaximaPair(e);
            double X = e.xtop;
            TEdge eNext = e.nextInAEL;
            while (eNext != eMaxPair)
            {
                if (eNext == null) throw new clipperException("DoMaxima error");
                IntersectEdges(e, eNext, new TDoublePoint(X, topY), TProtects.ipBoth);
                eNext = eNext.nextInAEL;
            }
            if ((e.outIdx < 0) && (eMaxPair.outIdx < 0))
            {
                DeleteFromAEL(e);
                DeleteFromAEL(eMaxPair);
            }
            else if ((e.outIdx >= 0) && (eMaxPair.outIdx >= 0))
            {
                IntersectEdges(e, eMaxPair, new TDoublePoint(X, topY), 0);
            }
            else 
                throw new clipperException("DoMaxima error");
        }
        //------------------------------------------------------------------------------

        private void ProcessHorizontals()
        {
            TEdge horzEdge = m_SortedEdges;
            while (horzEdge != null)
            {
                DeleteFromSEL(horzEdge);
                ProcessHorizontal(horzEdge);
                horzEdge = m_SortedEdges;
            }
        }
        //------------------------------------------------------------------------------

        bool IsHorizontal(TPolyPt pp1, TPolyPt pp2)
        {
            return (Math.Abs(pp1.pt.X - pp2.pt.X) > precision &&
              Math.Abs(pp1.pt.Y - pp2.pt.Y) < precision);
        }
        //------------------------------------------------------------------------------

        private bool IsTopHorz(TEdge horzEdge, double XPos)
        {
            TEdge e = m_SortedEdges;
            while (e != null)
            {
                if ((XPos >= Math.Min(e.xbot, e.xtop)) && (XPos <= Math.Max(e.xbot, e.xtop)))
                    return false;
                e = e.nextInSEL;
            }
            return true;
        }
        //------------------------------------------------------------------------------

        private void ProcessHorizontal(TEdge horzEdge)
        {
            TDirection Direction;
            double horzLeft, horzRight;

            if (horzEdge.xbot < horzEdge.xtop)
            {
                horzLeft = horzEdge.xbot;
                horzRight = horzEdge.xtop;
                Direction = TDirection.dLeftToRight;
            }
            else
            {
                horzLeft = horzEdge.xtop;
                horzRight = horzEdge.xbot;
                Direction = TDirection.dRightToLeft;
            }

            TEdge eMaxPair;
            if (horzEdge.nextInLML != null)
                eMaxPair = null;
            else
                eMaxPair = GetMaximaPair(horzEdge);

            TEdge e = GetNextInAEL(horzEdge, Direction);
            while (e != null)
            {
                TEdge eNext = GetNextInAEL(e, Direction);
                if ((e.xbot >= horzLeft - tolerance) && (e.xbot <= horzRight + tolerance))
                {
                    //ok, so far it looks like we're still in range of the horizontal edge
                    if (Math.Abs(e.xbot - horzEdge.xtop) < tolerance && horzEdge.nextInLML != null)
                    {
                        if (SlopesEqual(e, horzEdge.nextInLML))
                        {
                            //we've got 2 colinear edges at the end of the horz. line ...
                            if (horzEdge.outIdx >= 0 && e.outIdx >= 0)
                            {
                                TDoublePoint pt = new TDoublePoint(horzEdge.xtop, horzEdge.ytop);
                                TJoinRec polyPtRec = new TJoinRec();
                                AddPolyPt(horzEdge, pt);
                                AddPolyPt(e, pt);
                                polyPtRec.idx1 = horzEdge.outIdx;
                                polyPtRec.idx2 = e.outIdx;
                                polyPtRec.pt = pt;
                                m_Joins.Add(polyPtRec);                            
                            }
                            break; //we've reached the end of the horizontal line
                        }
                        else if (e.dx < horzEdge.nextInLML.dx)
                            break; //we've reached the end of the horizontal line
                    }
                        
                    if (e == eMaxPair)
                    {
                        //horzEdge is evidently a maxima horizontal and we've arrived at its end.
                        if (Direction == TDirection.dLeftToRight)
                            IntersectEdges(horzEdge, e, new TDoublePoint(e.xbot, horzEdge.ybot), 0);
                        else
                            IntersectEdges(e, horzEdge, new TDoublePoint(e.xbot, horzEdge.ybot), 0);
                        return;
                    }
                    else if (IsHorizontal(e) && !IsMinima(e) && !(e.xbot > e.xtop))
                    {
                        if (Direction == TDirection.dLeftToRight)
                            IntersectEdges(horzEdge, e, new TDoublePoint(e.xbot, horzEdge.ybot),
                              (IsTopHorz(horzEdge, e.xbot)) ? TProtects.ipLeft : TProtects.ipBoth);
                        else
                            IntersectEdges(e, horzEdge, new TDoublePoint(e.xbot, horzEdge.ybot),
                              (IsTopHorz(horzEdge, e.xbot)) ? TProtects.ipRight : TProtects.ipBoth);
                    }
                    else if (Direction == TDirection.dLeftToRight)
                    {
                        IntersectEdges(horzEdge, e, new TDoublePoint(e.xbot, horzEdge.ybot),
                          (IsTopHorz(horzEdge, e.xbot)) ? TProtects.ipLeft : TProtects.ipBoth);
                    }
                    else
                    {
                        IntersectEdges(e, horzEdge, new TDoublePoint(e.xbot, horzEdge.ybot),
                          (IsTopHorz(horzEdge, e.xbot)) ? TProtects.ipRight : TProtects.ipBoth);
                    }
                    SwapPositionsInAEL(horzEdge, e);
                }
                else if ((Direction == TDirection.dLeftToRight) &&
                  (e.xbot > horzRight + tolerance) && horzEdge.nextInSEL == null) 
                    break;
                else if ((Direction == TDirection.dRightToLeft) &&
                  (e.xbot < horzLeft - tolerance) && horzEdge.nextInSEL == null) 
                    break;
                e = eNext;
            } //end while ( e )

            if (horzEdge.nextInLML != null)
            {
                if (horzEdge.outIdx >= 0)
                    AddPolyPt(horzEdge, new TDoublePoint(horzEdge.xtop, horzEdge.ytop));
                UpdateEdgeIntoAEL(ref horzEdge);
            }
            else
            {
                if (horzEdge.outIdx >= 0)
                    IntersectEdges(horzEdge, eMaxPair,
                      new TDoublePoint(horzEdge.xtop, horzEdge.ybot), TProtects.ipBoth);
                DeleteFromAEL(eMaxPair);
                DeleteFromAEL(horzEdge);
            }
        }
        //------------------------------------------------------------------------------

        TPolyPt InsertPolyPtBetween(TDoublePoint pt, TPolyPt pp1, TPolyPt pp2)
        {
            TPolyPt pp = new TPolyPt();
            pp.pt = pt;
            if (pp2 == pp1.next)
            {
                pp.next = pp2;
                pp.prev = pp1;
                pp1.next = pp;
                pp2.prev = pp;
            }
            else if (pp1 == pp2.next)
            {
                pp.next = pp1;
                pp.prev = pp2;
                pp2.next = pp;
                pp1.prev = pp;
            }
            else throw new clipperException("InsertPolyPtBetween error");
            return pp;
        }
        //------------------------------------------------------------------------------

        private TPolyPt AddPolyPt(TEdge e, TDoublePoint pt)
        {
            bool ToFront = (e.side == TEdgeSide.esLeft);
            if (e.outIdx < 0)
            {
                TPolyPt newPolyPt = new TPolyPt();
                newPolyPt.pt = pt;
                m_PolyPts.Add(newPolyPt);
                newPolyPt.next = newPolyPt;
                newPolyPt.prev = newPolyPt;
                e.outIdx = m_PolyPts.Count - 1;
                newPolyPt.flags = TOrientationFlag.ofEmpty;
                newPolyPt.dx = unassigned;
                return newPolyPt;
            }
            else
            {
                TPolyPt pp = m_PolyPts[e.outIdx];
                if (ToFront && PointsEqual(pt, pp.pt)) return pp;
                if (!ToFront && PointsEqual(pt, pp.prev.pt)) return pp.prev;
                TPolyPt newPolyPt = new TPolyPt();
                newPolyPt.pt = pt;
                newPolyPt.next = pp;
                newPolyPt.prev = pp.prev;
                newPolyPt.prev.next = newPolyPt;
                pp.prev = newPolyPt;
                newPolyPt.flags = TOrientationFlag.ofEmpty;
                newPolyPt.dx = unassigned;
                if (ToFront) m_PolyPts[e.outIdx] = newPolyPt;
                return newPolyPt;
            }
        }
        //------------------------------------------------------------------------------

        private void ProcessIntersections(double topY)
        {
          if( m_ActiveEdges == null)
                return;
          try {
            m_IntersectTolerance = tolerance;
            BuildIntersectList( topY );
            if (m_IntersectNodes == null)
                return;
            //Test pending intersections for errors and, if any are found, redo
            //BuildIntersectList (twice if necessary) with adjusted tolerances.
            //While this adds ~2% extra to processing time, I believe this is justified
            //by further halving of the algorithm's failure rate, though admittedly
            //failures were already extremely rare ...
            if ( !TestIntersections() )
            {
              m_IntersectTolerance = minimal_tolerance;
              DisposeIntersectNodes();
              BuildIntersectList( topY );
              if ( !TestIntersections() )
              {
                m_IntersectTolerance = slope_precision;
                DisposeIntersectNodes();
                BuildIntersectList( topY );
                if (!TestIntersections()) 
                    throw new clipperException("Intersection error");
              }
            }
            ProcessIntersectList();
          }
          catch {
            m_SortedEdges = null;
            DisposeIntersectNodes();
            throw new clipperException("ProcessIntersections error");
          }
        }
        //------------------------------------------------------------------------------

        private void DisposeIntersectNodes()
        {
            while (m_IntersectNodes != null)
            {
                TIntersectNode iNode = m_IntersectNodes.next;
                m_IntersectNodes = null;
                m_IntersectNodes = iNode;
            }
        }
        //------------------------------------------------------------------------------

        private bool E1PrecedesE2inAEL(TEdge e1, TEdge e2)
        {
            while (e1 != null)
            {
                if (e1 == e2)
                    return true;
                else
                    e1 = e1.nextInAEL;
            }
            return false;
        }
        //------------------------------------------------------------------------------

        private bool Process1Before2(TIntersectNode Node1, TIntersectNode Node2)
        {
            if (Math.Abs(Node1.pt.Y - Node2.pt.Y) < m_IntersectTolerance)
            {
                if (Math.Abs(Node1.pt.X - Node2.pt.X) > precision)
                    return Node1.pt.X < Node2.pt.X;
                //a complex intersection (with more than 2 edges intersecting) ...
                if (Node1.edge1 == Node2.edge1 || SlopesEqual(Node1.edge1, Node2.edge1))
                {
                    if (Node1.edge2 == Node2.edge2)
                        //(N1.E1 & N2.E1 are co-linear) and (N1.E2 == N2.E2)  ...
                        return !E1PrecedesE2inAEL(Node1.edge1, Node2.edge1);
                    else if (SlopesEqual(Node1.edge2, Node2.edge2))
                        //(N1.E1 == N2.E1) and (N1.E2 & N2.E2 are co-linear) ...
                        return E1PrecedesE2inAEL(Node1.edge2, Node2.edge2);
                    else if //check if minima **
                      ((Math.Abs(Node1.edge2.y - Node1.pt.Y) < slope_precision ||
                      Math.Abs(Node2.edge2.y - Node2.pt.Y) < slope_precision) &&
                      (Node1.edge2.next == Node2.edge2 || Node1.edge2.prev == Node2.edge2))
                    {
                        if (Node1.edge1.dx < 0) return Node1.edge2.dx > Node2.edge2.dx;
                        else return Node1.edge2.dx < Node2.edge2.dx;
                    }
                    else if ((Node1.edge2.dx - Node2.edge2.dx) < precision)
                        return E1PrecedesE2inAEL(Node1.edge2, Node2.edge2);
                    else
                        return (Node1.edge2.dx < Node2.edge2.dx);

                }
                else if (Node1.edge2 == Node2.edge2 && //check if maxima ***
                  (Math.Abs(Node1.edge1.ytop - Node1.pt.Y) < slope_precision ||
                  Math.Abs(Node2.edge1.ytop - Node2.pt.Y) < slope_precision))
                    return (Node1.edge1.dx > Node2.edge1.dx);
                else
                    return (Node1.edge1.dx < Node2.edge1.dx);
            }
            else
                return (Node1.pt.Y > Node2.pt.Y);
            //**a minima that very slightly overlaps an edge can appear like
            //a complex intersection but it's not. (Minima can't have parallel edges.)
            //***a maxima that very slightly overlaps an edge can appear like
            //a complex intersection but it's not. (Maxima can't have parallel edges.)
        }
        //------------------------------------------------------------------------------
        
        private void AddIntersectNode(TEdge e1, TEdge e2, TDoublePoint pt)
        {
            TIntersectNode IntersectNode = 
                new TIntersectNode { edge1 = e1, edge2 = e2, pt = pt, next = null, prev = null };
            if (m_IntersectNodes == null)
                m_IntersectNodes = IntersectNode;
            else if (Process1Before2(IntersectNode, m_IntersectNodes))
            {
                IntersectNode.next = m_IntersectNodes;
                m_IntersectNodes.prev = IntersectNode;
                m_IntersectNodes = IntersectNode;
            }
            else
            {
                TIntersectNode iNode = m_IntersectNodes;
                while (iNode.next != null && Process1Before2(iNode.next, IntersectNode))
                    iNode = iNode.next;
                if (iNode.next != null)
                    iNode.next.prev = IntersectNode;
                IntersectNode.next = iNode.next;
                IntersectNode.prev = iNode;
                iNode.next = IntersectNode;
            }
        }
        //------------------------------------------------------------------------------

        private void BuildIntersectList(double topY)
        {
            //prepare for sorting ...
            TEdge e = m_ActiveEdges;
            e.tmpX = TopX(e, topY);
            m_SortedEdges = e;
            m_SortedEdges.prevInSEL = null;
            e = e.nextInAEL;
            while (e != null)
            {
                e.prevInSEL = e.prevInAEL;
                e.prevInSEL.nextInSEL = e;
                e.nextInSEL = null;
                e.tmpX = TopX(e, topY);
                e = e.nextInAEL;
            }

            //bubblesort ...
            bool isModified = true;
            while (isModified && m_SortedEdges != null)
            {
                isModified = false;
                e = m_SortedEdges;
                while (e.nextInSEL != null)
                {
                    TEdge eNext = e.nextInSEL;
                    TDoublePoint pt;
                    if ((e.tmpX > eNext.tmpX + tolerance) && IntersectPoint(e, eNext, out pt))
                    {
                        AddIntersectNode(e, eNext, pt);
                        SwapPositionsInSEL(e, eNext);
                        isModified = true;
                    }
                    else
                        e = eNext;
                }
                if (e.prevInSEL != null)
                    e.prevInSEL.nextInSEL = null;
                else break;
            }
            m_SortedEdges = null;
        }
        //------------------------------------------------------------------------------
        
        private bool TestIntersections()
        {
            if (m_IntersectNodes == null)
                return true;
            //do the test sort using SEL ...
            CopyAELToSEL();
            TIntersectNode iNode = m_IntersectNodes;
            while (iNode != null)
            {
                SwapPositionsInSEL(iNode.edge1, iNode.edge2);
                iNode = iNode.next;
            }
            //now check that tmpXs are in the right order ...
            TEdge e = m_SortedEdges;
            while (e.nextInSEL != null)
            {
                if (e.nextInSEL.tmpX < e.tmpX - precision) return false;
                e = e.nextInSEL;
            }
            m_SortedEdges = null;
            return true;
        }
        //------------------------------------------------------------------------------

        private void ProcessIntersectList()
        {
            while (m_IntersectNodes != null)
            {
                TIntersectNode iNode = m_IntersectNodes.next;
                {
                    IntersectEdges(m_IntersectNodes.edge1,
                      m_IntersectNodes.edge2, m_IntersectNodes.pt, TProtects.ipBoth);
                    SwapPositionsInAEL(m_IntersectNodes.edge1, m_IntersectNodes.edge2);
                }
                m_IntersectNodes = iNode;
            }
        }

        void DoEdge1(TEdge edge1, TEdge edge2, TDoublePoint pt)
        {
            AddPolyPt(edge1, pt);
            SwapSides(edge1, edge2);
            SwapPolyIndexes(edge1, edge2);
        }
        //------------------------------------------------------------------------------

        void DoEdge2(TEdge edge1, TEdge edge2, TDoublePoint pt)
        {
            AddPolyPt(edge2, pt);
            SwapSides(edge1, edge2);
            SwapPolyIndexes(edge1, edge2);
        }
        //------------------------------------------------------------------------------

        private void DoBothEdges(TEdge edge1, TEdge edge2, TDoublePoint pt)
        {
            AddPolyPt(edge1, pt);
            AddPolyPt(edge2, pt);
            SwapSides(edge1, edge2);
            SwapPolyIndexes(edge1, edge2);
        }
        //------------------------------------------------------------------------------

        private void IntersectEdges(TEdge e1, TEdge e2, TDoublePoint pt, TProtects protects)
        {
            //e1 will be to the left of e2 BELOW the intersection. Therefore e1 is before
            //e2 in AEL except when e1 is being inserted at the intersection point ...

            bool e1stops = (TProtects.ipLeft & protects) == 0 && e1.nextInLML == null &&
              (Math.Abs(e1.xtop - pt.X) < tolerance) && //nb: not precision
              (Math.Abs(e1.ytop - pt.Y) < tolerance);
            bool e2stops = (TProtects.ipRight & protects) == 0 && e2.nextInLML == null &&
              (Math.Abs(e2.xtop - pt.X) < tolerance) && //nb: not precision
              (Math.Abs(e2.ytop - pt.Y) < tolerance);
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
                  (e1.polyType != e2.polyType && m_ClipType != TClipType.ctXor))
                    AddLocalMaxPoly( e1, e2, pt);
                else
                    DoBothEdges(e1, e2, pt);
            }
            else if (e1Contributing)
            {
                if (m_ClipType == TClipType.ctIntersection)
                {
                    if ((e2.polyType == TPolyType.ptSubject || e2.windCnt2 != 0) &&
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
                if (m_ClipType == TClipType.ctIntersection)
                {
                    if ((e1.polyType == TPolyType.ptSubject || e1.windCnt2 != 0) &&
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
                        case TClipType.ctIntersection:
                            {
                                if (Math.Abs(e1.windCnt2) > 0 && Math.Abs(e2.windCnt2) > 0)
                                    AddLocalMinPoly(e1, e2, pt);
                                break;
                            }
                        case TClipType.ctUnion:
                            {
                                if (e1.windCnt2 == 0 && e2.windCnt2 == 0)
                                    AddLocalMinPoly(e1, e2, pt);
                                break;
                            }
                        case TClipType.ctDifference:
                            {
                                if ((e1.polyType == TPolyType.ptClip && e2.polyType == TPolyType.ptClip &&
                              e1.windCnt2 != 0 && e2.windCnt2 != 0) ||
                              (e1.polyType == TPolyType.ptSubject && e2.polyType == TPolyType.ptSubject &&
                              e1.windCnt2 == 0 && e2.windCnt2 == 0))
                                    AddLocalMinPoly(e1, e2, pt);
                                break;
                            }
                        case TClipType.ctXor:
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

        private void DeleteFromAEL(TEdge e)
        {
            TEdge AelPrev = e.prevInAEL;
            TEdge AelNext = e.nextInAEL;
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

        private void DeleteFromSEL(TEdge e)
        {
            TEdge SelPrev = e.prevInSEL;
            TEdge SelNext = e.nextInSEL;
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

        private void UpdateEdgeIntoAEL(ref TEdge e)
        {
            if (e.nextInLML == null)
                throw new clipperException("UpdateEdgeIntoAEL: invalid call");
            TEdge AelPrev = e.prevInAEL;
            TEdge AelNext = e.nextInAEL;
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
            if (!IsHorizontal(e))
            {
                InsertScanbeam(e.ytop);

                //if output polygons share an edge, they'll need joining later ...
                if (e.outIdx >= 0 && AelPrev != null && AelPrev.outIdx >= 0 &&
                  Math.Abs(AelPrev.xbot - e.x) < tolerance && SlopesEqual(e, AelPrev))
                {
                    TDoublePoint pt = new TDoublePoint(e.x, e.y);
                    TJoinRec polyPtRec = new TJoinRec();
                    AddPolyPt(AelPrev, pt);
                    AddPolyPt(e, pt);
                    polyPtRec.idx1 = AelPrev.outIdx;
                    polyPtRec.idx2 = e.outIdx;
                    polyPtRec.pt = pt;
                    m_Joins.Add(polyPtRec);                            
                }
            }
        }
        //------------------------------------------------------------------------------
        
        private bool IsContributing(TEdge edge)
        {
            switch (m_ClipType)
            {
                case TClipType.ctIntersection:
                    if (edge.polyType == TPolyType.ptSubject)
                        return Math.Abs(edge.windCnt) == 1 && edge.windCnt2 != 0;
                    else
                        return Math.Abs(edge.windCnt2) > 0 && Math.Abs(edge.windCnt) == 1;
                case TClipType.ctUnion:
                    return Math.Abs(edge.windCnt) == 1 && edge.windCnt2 == 0;
                case TClipType.ctDifference:
                    if (edge.polyType == TPolyType.ptSubject)
                        return Math.Abs(edge.windCnt) == 1 && edge.windCnt2 == 0;
                    else
                        return Math.Abs(edge.windCnt) == 1 && edge.windCnt2 != 0;
                default: //case ctXor:
                    return Math.Abs(edge.windCnt) == 1;
            }
        }
        //------------------------------------------------------------------------------

        public bool Execute(TClipType clipType, TPolyPolygon solution, 
            TPolyFillType subjFillType, TPolyFillType clipFillType)
        {
            m_SubjFillType = subjFillType;
            m_ClipFillType = clipFillType;

            solution.Clear();
            if (m_ExecuteLocked || !InitializeScanbeam())
                return false;
            try
            {
                m_ExecuteLocked = true;
                m_ActiveEdges = null;
                m_SortedEdges = null;
                m_ClipType = clipType;
                m_Joins.Clear();
                m_CurrentHorizontals.Clear();
                double ybot = PopScanbeam();
                do
                {
                    InsertLocalMinimaIntoAEL(ybot);
                    ProcessHorizontals();
                    double ytop = PopScanbeam();
                    ProcessIntersections(ytop);
                    ProcessEdgesAtTopOfScanbeam(ytop);
                    ybot = ytop;
                } while (m_Scanbeam != null);

                //build the return polygons ...
                BuildResult(solution);
            }
            catch
            {
                return false;
            }
            finally
            {
                m_Joins.Clear();
                DisposeAllPolyPts();
                m_ExecuteLocked = false;
            }
            return true;
        }
        //------------------------------------------------------------------------------

        private TPolyPt FixupOutPolygon(TPolyPt p, bool stripPointyEdgesOnly = false)
        {
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

            if (p == null) return null;
            TPolyPt pp = p, result = p, lastOK = null;
            for (; ; )
            {
                if (pp.prev == pp || pp.prev == pp.next)
                {
                    DisposePolyPts(pp);
                    return null;
                }
                //test for duplicate points and for same slope (cross-product) ...
                if (PointsEqual(pp.pt, pp.next.pt) ||
                    (Math.Abs((pp.pt.Y - pp.prev.pt.Y) * (pp.next.pt.X - pp.pt.X) -
                    (pp.pt.X - pp.prev.pt.X) * (pp.next.pt.Y - pp.pt.Y)) < precision &&
                    (!stripPointyEdgesOnly ||
                    ((pp.pt.X - pp.prev.pt.X > 0) != (pp.next.pt.X - pp.pt.X > 0)) ||
                    ((pp.pt.Y - pp.prev.pt.Y > 0) != (pp.next.pt.Y - pp.pt.Y > 0)))))
                {
                    lastOK = null;
                    pp.prev.next = pp.next;
                    pp.next.prev = pp.prev;
                    TPolyPt tmp = pp;
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

        private TPolyPt FixupOutPolygon2(TPolyPt p, bool stripPointyEdgesOnly = false)
        {
            //FixupOutPolygon2() - just removes duplicates

            if (p == null) return null;
            TPolyPt pp = p, result = p, lastOK = null;
            for (; ; )
            {
                if (pp.prev == pp || pp.prev == pp.next)
                {
                    DisposePolyPts(pp);
                    return null;
                }
                //test for duplicate points and for same slope (cross-product) ...
                if (PointsEqual(pp.pt, pp.next.pt) )
                {
                    lastOK = null;
                    pp.prev.next = pp.next;
                    pp.next.prev = pp.prev;
                    TPolyPt tmp = pp;
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

        private void BuildResult(TPolyPolygon polypoly)
        {
            MergePolysWithCommonEdges();
            for (int i = 0; i < m_PolyPts.Count; ++i)
              if (m_PolyPts[i] != null)
                m_PolyPts[i] = FixupOutPolygon(m_PolyPts[i]);

            if (!m_IgnoreOrientation) FixOrientation();

            polypoly.Clear();
            polypoly.Capacity = m_PolyPts.Count;
            for (int i = 0; i < m_PolyPts.Count; ++i)
            {
                if (m_PolyPts[i] != null)
                {
                    TPolyPt p = m_PolyPts[i];
                    TPolygon pg = new TPolygon();
                    do
                    {
                        pg.Add(new TDoublePoint(p.pt.X, p.pt.Y));
                        p = p.next;
                    }
                    while (p != m_PolyPts[i]);
                    if (pg.Count < 3)
                        pg = null;
                    else
                        polypoly.Add(pg);
                }
            }
        }
        //------------------------------------------------------------------------------

        private TEdge BubbleSwap(TEdge edge)
        {
            int n = 1;
            TEdge result = edge.nextInAEL;
            while (result != null && (Math.Abs(result.xbot - edge.xbot) <= tolerance))
            {
                ++n;
                result = result.nextInAEL;
            }
            //if more than 2 edges intersect at a given point then there are multiple
            //intersections at this point between all the edges. Therefore ...
            //let n = no. edges intersecting at a given point.
            //given f(n) = no. intersections between n edges at a given point, & f(0) = 0
            //then f(n) = f(n-1) + n-1;
            //therefore 1 edge -> 0 intersections; 2 -> 1; 3 -> 3; 4 -> 6; 5 -> 10 etc.
            //nb: coincident edges will cause unexpected f(n) values.
            if (n > 2)
            {
                //create the sort list ...
                try
                {
                    m_SortedEdges = edge;
                    edge.prevInSEL = null;
                    TEdge e = edge.nextInAEL;
                    for (int i = 2; i <= n; ++i)
                    {
                        e.prevInSEL = e.prevInAEL;
                        e.prevInSEL.nextInSEL = e;
                        if (i == n) e.nextInSEL = null;
                        e = e.nextInAEL;
                    }
                    while (m_SortedEdges != null && m_SortedEdges.nextInSEL != null)
                    {
                        e = m_SortedEdges;
                        while (e.nextInSEL != null)
                        {
                            if (e.nextInSEL.dx > e.dx)
                            {
                                IntersectEdges(e, e.nextInSEL,  //param order important here
                                    new TDoublePoint(e.xbot, e.ybot), TProtects.ipBoth);
                                SwapPositionsInAEL(e, e.nextInSEL);
                                SwapPositionsInSEL(e, e.nextInSEL);
                            }
                            else
                                e = e.nextInSEL;
                        }
                        e.prevInSEL.nextInSEL = null; //removes 'e' from SEL
                    }
                }
                catch
                {
                    m_SortedEdges = null;
                    throw new clipperException("BubbleSwap error");
                }
                m_SortedEdges = null;
            }
            return result;
        }
        //------------------------------------------------------------------------------

        private void ProcessEdgesAtTopOfScanbeam(double topY)
        {
            TEdge e = m_ActiveEdges;
            while (e != null)
            {
                //1. process maxima, treating them as if they're 'bent' horizontal edges,
                //   but exclude maxima with horizontal edges. nb: e can't be a horizontal.
                if (IsMaxima(e, topY) && !IsHorizontal(GetMaximaPair(e)))
                {
                    //'e' might be removed from AEL, as may any following edges so ...
                    TEdge ePrior = e.prevInAEL;
                    DoMaxima(e, topY);
                    if (ePrior == null)
                        e = m_ActiveEdges;
                    else
                        e = ePrior.nextInAEL;
                }
                else
                {
                    //2. promote horizontal edges, otherwise update xbot and ybot ...
                    if (IsIntermediate(e, topY) && IsHorizontal(e.nextInLML))
                    {
                        if (e.outIdx >= 0)
                        {
                            TPolyPt pp = AddPolyPt(e, new TDoublePoint(e.xtop, e.ytop));
                            //add the polyPt to a list that later checks for overlaps with
                            //contributing horizontal minima since they'll need joining...
                            TJoinRec ch = new TJoinRec();
                            ch.idx1 = e.outIdx;
                            ch.pt = new TDoublePoint(e.nextInLML.xtop, e.nextInLML.ytop);
                            ch.outPPt = pp;
                            m_CurrentHorizontals.Add(ch);
                        }

                        //very rarely an edge just below a horizontal edge in a contour
                        //intersects with another edge at the very top of a scanbeam.
                        //If this happens that intersection must be managed first ...
                        if (e.prevInAEL != null && e.prevInAEL.xbot > e.xtop + tolerance)
                        {
                            IntersectEdges(e.prevInAEL, e, new TDoublePoint(e.prevInAEL.xbot,
                              e.prevInAEL.ybot), TProtects.ipBoth);
                            SwapPositionsInAEL(e.prevInAEL, e);
                            UpdateEdgeIntoAEL(ref e);
                            AddEdgeToSEL(e);
                            e = e.nextInAEL;
                            UpdateEdgeIntoAEL(ref e);
                            AddEdgeToSEL(e);
                        }
                        else if (e.nextInAEL != null && e.xtop > TopX(e.nextInAEL, topY) + tolerance)
                        {
                            e.nextInAEL.xbot = TopX(e.nextInAEL, topY);
                            e.nextInAEL.ybot = topY;
                            IntersectEdges(e, e.nextInAEL, new TDoublePoint(e.nextInAEL.xbot,
                              e.nextInAEL.ybot), TProtects.ipBoth);
                            SwapPositionsInAEL(e, e.nextInAEL);
                            UpdateEdgeIntoAEL(ref e);
                            AddEdgeToSEL(e);
                        }
                        else
                        {
                            UpdateEdgeIntoAEL(ref e);
                            AddEdgeToSEL(e);
                        }
                    }
                    else
                    {
                        //this just simplifies horizontal processing ...
                        e.xbot = TopX(e, topY);
                        e.ybot = topY;
                    }
                    e = e.nextInAEL;
                }
            }

            //3. Process horizontals at the top of the scanbeam ...
            ProcessHorizontals();

            //4. Promote intermediate vertices ...
            e = m_ActiveEdges;
            while (e != null)
            {
                if (IsIntermediate(e, topY))
                {
                    if (e.outIdx >= 0)
                        AddPolyPt(e, new TDoublePoint(e.xtop, e.ytop));
                    UpdateEdgeIntoAEL(ref e);
                }
                e = e.nextInAEL;
            }

            //5. Process (non-horizontal) intersections at the top of the scanbeam ...
            e = m_ActiveEdges;
            if (e !=null && e.nextInAEL == null)
                throw new clipperException("ProcessEdgesAtTopOfScanbeam() error");
            while (e != null)
            {
                if (e.nextInAEL == null)
                    break;
                if (e.nextInAEL.xbot < e.xbot - precision)
                    throw new clipperException("ProcessEdgesAtTopOfScanbeam() error");
                if (e.nextInAEL.xbot > e.xbot + tolerance)
                    e = e.nextInAEL;
                else
                    e = BubbleSwap(e);
            }
        }
        //------------------------------------------------------------------------------

        private void AddLocalMaxPoly(TEdge e1, TEdge e2, TDoublePoint pt)
        {
            AddPolyPt(e1, pt);
            if (EdgesShareSamePoly(e1, e2))
            {
                e1.outIdx = -1;
                e2.outIdx = -1;
            }
            else AppendPolygon(e1, e2);
        }

        private void AddLocalMinPoly(TEdge e1, TEdge e2, TDoublePoint pt)
        {
            AddPolyPt(e1, pt);
            e2.outIdx = e1.outIdx;

            if (IsHorizontal(e2) || (e1.dx > e2.dx))
            {
                e1.side = TEdgeSide.esLeft;
                e2.side = TEdgeSide.esRight;
            }
            else
            {
                e1.side = TEdgeSide.esRight;
                e2.side = TEdgeSide.esLeft;
            }
        }
        //------------------------------------------------------------------------------

        private void AppendPolygon(TEdge e1, TEdge e2)
        {
            //get the start and ends of both output polygons ...
            TPolyPt p1_lft = m_PolyPts[e1.outIdx];
            TPolyPt p1_rt = p1_lft.prev;
            TPolyPt p2_lft = m_PolyPts[e2.outIdx];
            TPolyPt p2_rt = p2_lft.prev;
            TEdgeSide side;

            //join e2 poly onto e1 poly and delete pointers to e2 ...
            if (e1.side == TEdgeSide.esLeft)
            {
                if (e2.side == TEdgeSide.esLeft)
                {
                    //z y x a b c
                    ReversePolyPtLinks(p2_lft);
                    p2_lft.next = p1_lft;
                    p1_lft.prev = p2_lft;
                    p1_rt.next = p2_rt;
                    p2_rt.prev = p1_rt;
                    m_PolyPts[e1.outIdx] = p2_rt;
                }
                else
                {
                    //x y z a b c
                    p2_rt.next = p1_lft;
                    p1_lft.prev = p2_rt;
                    p2_lft.prev = p1_rt;
                    p1_rt.next = p2_lft;
                    m_PolyPts[e1.outIdx] = p2_lft;
                }
                side = TEdgeSide.esLeft;
            }
            else
            {
                if (e2.side == TEdgeSide.esRight)
                {
                    //a b c z y x
                    ReversePolyPtLinks(p2_lft);
                    p1_rt.next = p2_rt;
                    p2_rt.prev = p1_rt;
                    p2_lft.next = p1_lft;
                    p1_lft.prev = p2_lft;
                }
                else
                {
                    //a b c x y z
                    p1_rt.next = p2_lft;
                    p2_lft.prev = p1_rt;
                    p1_lft.prev = p2_rt;
                    p2_rt.next = p1_lft;
                }
                side = TEdgeSide.esRight;
            }

            int OKIdx = e1.outIdx;
            int ObsoleteIdx = e2.outIdx;
            m_PolyPts[ObsoleteIdx] = null;

            for (int i = 0; i < m_Joins.Count; ++i)
            {
                if (m_Joins[i].idx1 == ObsoleteIdx) m_Joins[i].idx1 = OKIdx;
                if (m_Joins[i].idx2 == ObsoleteIdx) m_Joins[i].idx2 = OKIdx;
            }
            for (int i = 0; i < m_CurrentHorizontals.Count; ++i)
                if (m_CurrentHorizontals[i].idx1 == ObsoleteIdx)
                    m_CurrentHorizontals[i].idx1 = OKIdx;

            e1.outIdx = -1;
            e2.outIdx = -1;
            TEdge e = m_ActiveEdges;
            while (e != null)
            {
                if (e.outIdx == ObsoleteIdx)
                {
                    e.outIdx = OKIdx;
                    e.side = side;
                    break;
                }
                e = e.nextInAEL;
            }
        }
        //------------------------------------------------------------------------------

        private TPolyPt InsertPolyPt(TPolyPt afterPolyPt, TDoublePoint pt)
        {
          TPolyPt polyPt = new TPolyPt();
          polyPt.pt = pt;
          polyPt.prev = afterPolyPt;
          polyPt.next = afterPolyPt.next;
          afterPolyPt.next.prev = polyPt;
          afterPolyPt.next = polyPt;
          return polyPt;
        }
        //------------------------------------------------------------------------------

        TPolyPt DuplicatePolyPt(TPolyPt polyPt)
        {
          TPolyPt result = new TPolyPt();
          result.pt = polyPt.pt;
          result.flags = polyPt.flags;
          result.dx = unassigned;
          result.prev = polyPt;
          result.next = polyPt.next;
          polyPt.next.prev = result;
          polyPt.next = result;
          return result;
        }
        //------------------------------------------------------------------------------

        bool PtIsAPolyPt(TDoublePoint pt, TPolyPt polyStartPt)
        {
          if (polyStartPt == null) return false;
          TPolyPt p = polyStartPt;
          do {
            if (PointsEqual(pt, p.pt)) return true;
            p = p.next;
          }
          while (p !=polyStartPt);
          return false;
        }
        //------------------------------------------------------------------------------

        internal static bool SlopesEqual(TDoublePoint pt1a, TDoublePoint pt1b, 
            TDoublePoint pt2a, TDoublePoint pt2b)
        {
          return Math.Abs((pt1b.Y - pt1a.Y) * (pt2b.X - pt2a.X) - 
              (pt1b.X - pt1a.X) * (pt2b.Y - pt2a.Y)) < slope_precision;
        }
        //------------------------------------------------------------------------------

        private void MergePolysWithCommonEdges()
        {
          TPolyPt p1, p2;
          for (int i = 0; i < m_Joins.Count; ++i)
          {
            if (m_Joins[i].idx1 == m_Joins[i].idx2)
            {
              p1 = m_PolyPts[m_Joins[i].idx1];
              p1 = FixupOutPolygon(p1, true);
              m_PolyPts[m_Joins[i].idx1] = p1;
              if (!PtIsAPolyPt(m_Joins[i].pt, p1)) continue;
              p2 = p1.next; //ie we don't want the same point as p1
              if (!PtIsAPolyPt(m_Joins[i].pt, p2) || p2 == p1) continue;
            } else
            {
              p1 = m_PolyPts[m_Joins[i].idx1];
              p1 = FixupOutPolygon(p1, true);
              m_PolyPts[m_Joins[i].idx1] = p1;
              //check that fJoins[i].pt is in the polygon and also update p1 so
              //that p1.pt == fJoins[i].pt ...
              if (!PtIsAPolyPt(m_Joins[i].pt, p1)) continue;

              p2 = m_PolyPts[m_Joins[i].idx2];
              p2 = FixupOutPolygon(p2, true);
              m_PolyPts[m_Joins[i].idx2] = p2;
              if (!PtIsAPolyPt(m_Joins[i].pt, p2)) continue;
            }

            if (((p1.next.pt.X > p1.pt.X && p2.next.pt.X > p2.pt.X) ||
              (p1.next.pt.X < p1.pt.X && p2.next.pt.X < p2.pt.X) ||
              (p1.next.pt.Y > p1.pt.Y && p2.next.pt.Y > p2.pt.Y) ||
              (p1.next.pt.Y < p1.pt.Y && p2.next.pt.Y < p2.pt.Y)) &&
              SlopesEqual(p1.pt, p1.next.pt, p2.pt, p2.next.pt))
            {
              if (m_Joins[i].idx1 == m_Joins[i].idx2) continue;
              TPolyPt pp1 = DuplicatePolyPt(p1);
              TPolyPt pp2 = DuplicatePolyPt(p2);
              ReversePolyPtLinks( p2 );
              pp1.prev = pp2;
              pp2.next = pp1;
              p1.next = p2;
              p2.prev = p1;
            }
            else if (((p1.next.pt.X > p1.pt.X && p2.prev.pt.X > p2.pt.X) ||
              (p1.next.pt.X < p1.pt.X && p2.prev.pt.X < p2.pt.X) ||
              (p1.next.pt.Y > p1.pt.Y && p2.prev.pt.Y > p2.pt.Y) ||
              (p1.next.pt.Y < p1.pt.Y && p2.prev.pt.Y < p2.pt.Y)) &&
              SlopesEqual(p1.pt, p1.next.pt, p2.pt, p2.prev.pt))
            {
              TPolyPt pp1 = DuplicatePolyPt(p1);
              TPolyPt pp2 = DuplicatePolyPt(p2);
              p1.next = pp2;
              pp2.prev = p1;
              p2.next = pp1;
              pp1.prev = p2;
            }
            else if (((p1.prev.pt.X > p1.pt.X && p2.next.pt.X > p2.pt.X) ||
              (p1.prev.pt.X < p1.pt.X && p2.next.pt.X < p2.pt.X) ||
              (p1.prev.pt.Y > p1.pt.Y && p2.next.pt.Y > p2.pt.Y) ||
              (p1.prev.pt.Y < p1.pt.Y && p2.next.pt.Y < p2.pt.Y)) &&
              SlopesEqual(p1.pt, p1.prev.pt, p2.pt, p2.next.pt))
            {
              TPolyPt pp1 = DuplicatePolyPt(p1);
              TPolyPt pp2 = DuplicatePolyPt(p2);
              p1.next = pp2;
              pp2.prev = p1;
              pp1.prev = p2;
              p2.next = pp1;
            }
            else if (((p1.prev.pt.X > p1.pt.X && p2.prev.pt.X > p2.pt.X) ||
              (p1.prev.pt.X < p1.pt.X && p2.prev.pt.X < p2.pt.X) ||
              (p1.prev.pt.Y > p1.pt.Y && p2.prev.pt.Y > p2.pt.Y) ||
              (p1.prev.pt.Y < p1.pt.Y && p2.prev.pt.Y < p2.pt.Y)) &&
              SlopesEqual(p1.pt, p1.prev.pt, p2.pt, p2.prev.pt))
            {
              if (m_Joins[i].idx1 == m_Joins[i].idx2) continue;
              TPolyPt pp1 = DuplicatePolyPt(p1);
              TPolyPt pp2 = DuplicatePolyPt(p2);
              ReversePolyPtLinks(pp2);
              pp1.prev = pp2;
              pp2.next = pp1;
              p1.next = p2;
              p2.prev = p1;
            }
            else
              continue;

            if (m_Joins[i].idx1 == m_Joins[i].idx2)
            {
              //When an edge join occurs within the same polygon, then
              //that polygon effectively splits into 2 polygons ...
              p1 = FixupOutPolygon(p1, true);
              m_PolyPts[m_Joins[i].idx1] = p1;
              p2 = FixupOutPolygon(p2, true);
              int newIdx = m_PolyPts.Count;
              m_PolyPts.Add(p2);
              for (int j = i+1; j < m_Joins.Count; ++j)
              {
                if (m_Joins[j].idx1 == m_Joins[i].idx1 &&
                  PtIsAPolyPt(m_Joins[j].pt, p2))
                    m_Joins[j].idx1 = newIdx;
                if (m_Joins[j].idx2 == m_Joins[i].idx1 &&
                  PtIsAPolyPt(m_Joins[j].pt, p2))
                    m_Joins[j].idx1 = newIdx;
              }
            } else
            {
              //When 2 polygons are merged (joined), pointers referencing the
              //'deleted' polygon must now point to the 'merged' polygon ...
              m_PolyPts[m_Joins[i].idx2] = null;
              for (int j = i+1; j < m_Joins.Count; ++j)
              {
                if (m_Joins[j].idx1 == m_Joins[i].idx2)
                  m_Joins[j].idx1 = m_Joins[i].idx1;
                if (m_Joins[j].idx2 == m_Joins[i].idx2)
                  m_Joins[j].idx2 = m_Joins[i].idx1;
              }
            }
          }
        }
        //------------------------------------------------------------------------------

        internal static double SetDx(TPolyPt pp)
        {
          if (pp.dx == unassigned)
          {
            TPolyPt pp2;
            if ((TOrientationFlag.ofForwardBound & pp.flags) != 0) pp2 = pp.next; else pp2 = pp.prev;
            double dx = Math.Abs(pp.pt.X - pp2.pt.X);
            double dy = Math.Abs(pp.pt.Y - pp2.pt.Y);
            if ((dx < 0.1 && dy *10 < dx) || dy < precision)
              pp.dx = horizontal; else
              pp.dx = (pp.pt.X - pp2.pt.X)/(pp.pt.Y - pp2.pt.Y);
          }
          return pp.dx;
        }
        //------------------------------------------------------------------------------

        internal static void NextPoint(ref TPolyPt p, bool goingForward)
        {
          if (goingForward)
          {
            while (PointsEqual(p.pt, p.next.pt)) p = p.next;
            p = p.next;
          } else
          {
            while (PointsEqual(p.pt, p.prev.pt)) p = p.prev;
            p = p.prev;
          }
        }
        //------------------------------------------------------------------------------

        internal static double GetR(TDoublePoint pt1, TDoublePoint pt2, TDoublePoint pt3)
        {
          //this function is useful when COMPARING angles as it's a little quicker
          //than getting the specific angles using arctan().
          //Return value are between -2 and +2 where -1.99 is an acute angle turning
          //right, +1.99 is an acute angle turn left and 0 when the points are parallel.
          TDoublePoint N1 = GetUnitNormal(pt1, pt2);
          TDoublePoint N2 = GetUnitNormal(pt2, pt3);
          if (N1.X * N2.Y - N2.X * N1.Y < 0)
            return 1- (N1.X*N2.X + N1.Y*N2.Y); else
            return (N1.X*N2.X + N1.Y*N2.Y) -1;
        }
        //------------------------------------------------------------------------------

        internal static double DistanceSqr(TDoublePoint pt1, TDoublePoint pt2)
        {
          return (pt1.X - pt2.X)*(pt1.X - pt2.X) + (pt1.Y - pt2.Y)*(pt1.Y - pt2.Y);
        }
        //------------------------------------------------------------------------------

        internal static int CompareForwardAngles(TPolyPt p1, TPolyPt p2)
        {
          //preconditions:
          //1. p1a == p2a
          //2. p1.p1nextInBound is colinear with p2.p2nextInBound

          bool p1Forward = (TOrientationFlag.ofForwardBound & p1.flags) !=0;
          bool p2Forward = (TOrientationFlag.ofForwardBound & p2.flags) != 0;
          TPolyPt pTmp = null;
          TDoublePoint p1a, p1b, p1c,  p2a, p2b, p2c;
          p1b = p1c = p1.pt;
          p2b = p2c = p2.pt;
          do
          {
            p1a = p1b; p2a = p2b; p1b = p1c; p2b = p2c;
            NextPoint(ref p1, p1Forward);
            NextPoint(ref p2, p2Forward);

            //the following avoids a very rare endless loop where the
            //p1 & p2 polys are almost identical except for their orientations ...
            if (pTmp == null) 
                pTmp = p1;
            else if (pTmp == p1)
            {
              if (PointsEqual(p1c, p2c)) return 1;
              break;
            }
            p1c = p1.pt; p2c = p2.pt;
          }
          while (PointsEqual(p1c, p2c));

          //and now ... p1c != p2c ...
          if (PointsEqual(p1a, p1b) ||
            PointsEqual(GetUnitNormal(p1b, p1c),GetUnitNormal(p2b, p2c)))
          {
            //we have parallel edges of unequal length ...
            if (DistanceSqr(p1b, p1c) < DistanceSqr(p2b, p2c))
            {
              p1a = p1b; p1b = p1c;
              NextPoint(ref p1, p1Forward);
              double r1 = GetR(p1a, p1b, p1.pt);
              if (r1 > tolerance) return 1;
              else if (r1 < -tolerance) return -1;
              else throw new clipperException("CompareForwardAngles error");
            } else
            {
              p2a = p2b; p2b = p2c;
              NextPoint(ref p2, p2Forward);
              double r2 = GetR(p2a, p2b, p2.pt);
              if (r2 > tolerance) return -1;
              else if (r2 < -tolerance) return 1;
              else throw new clipperException("CompareForwardAngles error");
            }
          } else
          {
            double r1 = GetR(p1a, p1b, p1c);
            double r2 = GetR(p2a, p2b, p2c);
            if (r1 > r2 + tolerance) return 1;
            else if (r1 < r2 - tolerance) return -1;
            else throw new clipperException("CompareForwardAngles error");
          }
        }
        //------------------------------------------------------------------------------

        int blCompare(TPolyPt pp1, TPolyPt pp2)
        {
            if (pp2.pt.Y > pp1.pt.Y + precision) return -1;
            else if (pp2.pt.Y < pp1.pt.Y - precision) return 1;
            else if (pp2.pt.X < pp1.pt.X - precision) return -1;
            else if (pp2.pt.X > pp1.pt.X + precision) return 1;
            else if (pp1 == pp2) return 0;
            else
            {
                double dx1 = SetDx(pp1);
                double dx2 = SetDx(pp2);
                if (dx1 < dx2 - precision) return -1;
                else if (dx1 > dx2 + precision) return 1;
                else return CompareForwardAngles(pp1, pp2);
            }
        }
        //------------------------------------------------------------------------------

        int wlCompare(TPolyPt pp1, TPolyPt pp2)
        {
          //nb: 1. when item1 < item2, result = 1; when item2 < item1, result = -1
          //    2. wlCompare is only ever used for insertions into the skiplist.
          //    3. item2 is always the item being inserted.
          TPolyPt pp1Next, pp2Next;
          if ((TOrientationFlag.ofForwardBound & pp1.flags) != 0)
            pp1Next = pp1.next; else
            pp1Next = pp1.prev;

          if (pp1 == pp2) return 0;
          else if (pp1.pt.X < pp2.pt.X - tolerance &&
            pp1Next.pt.X < pp2.pt.X + tolerance) return 1;
          else if (pp1.pt.X > pp2.pt.X + tolerance &&
            pp1Next.pt.X > pp2.pt.X - tolerance) return -1;
          else if (PointsEqual(pp1.pt, pp2.pt))
          {
            double dx1 = SetDx(pp1); double dx2 = SetDx(pp2);
            //dx1 should never be horizontal, but if dx2 is horizontal ...
            if (dx2 == horizontal)
            {
                if ((TOrientationFlag.ofForwardBound & pp2.flags) != 0)
                    pp2Next = pp2.next; else pp2Next = pp2.prev;
                if (pp2Next.pt.X < pp1.pt.X) return -1; else return 1;
            }
            else if (dx1 < dx2 - precision) return -1;
            else if (dx1 > dx2 + precision) return 1;
            else return CompareForwardAngles(pp1, pp2);
          } else
          {
            SetDx(pp1);
            if (pp1.dx == horizontal) {
                if ((TOrientationFlag.ofForwardBound & pp1.flags) != 0)
                    pp2 = pp1.next; else pp2 = pp1.prev;
              if (pp2.pt.X > pp1.pt.X) return -1; else return 1;
            }
            double pp1X = pp1.pt.X + (pp2.pt.Y - pp1.pt.Y) * pp1.dx;
            if (pp1X < pp2.pt.X - precision) return 1;
            else if (pp1X > pp2.pt.X + precision) return -1;
            else
            {
                if ((TOrientationFlag.ofForwardBound & pp2.flags) != 0)
                    pp2Next = pp2.next; else
                    pp2Next = pp2.prev;
                pp1Next = pp2Next;
                do {
                    double r = GetR(pp1.pt, pp2.pt, pp2Next.pt);
                    if (Math.Abs(r) < precision) {
                        if ((TOrientationFlag.ofForwardBound & pp2.flags) != 0)
                            pp2Next = pp2.next; else
                            pp2Next = pp2.prev;
                        continue;
                    }
                    else if (r > 0) return -1;
                    else return 1;
                }
                while (pp2Next != pp1Next); //ie avoids a very rare endless loop
                return 1;
            }
          }
        }
        //------------------------------------------------------------------------------

        bool NextIsBottom(TPolyPt p)
        {
            TPolyPt pp = p.next;
            while (pp.next.pt.Y == pp.pt.Y) pp = pp.next;
            if (pp.next.pt.Y > pp.pt.Y) return false;
            else return true;
        }
        //------------------------------------------------------------------------------

        void UpdateBounds(SkipList<TPolyPt> sl, SkipNode<TPolyPt> sn, double Y)
        {
            TPolyPt pp = sn.item;
            for (;;)
            {
                TPolyPt pp2;
                if ((TOrientationFlag.ofForwardBound & pp.flags) != 0)
                    pp2 = pp.next;
                else pp2 = pp.prev;
                if (pp2.pt.Y < Y - tolerance) break;
                pp = pp2;
                if ((TOrientationFlag.ofTop & pp.flags) != 0) break;
            }

            //nb: DeleteItem() isn't safe here because of wCompare function
            if ((TOrientationFlag.ofTop & pp.flags) != 0)
                sl.Delete(sn);
            else
                sn.item = pp;
        }
        //------------------------------------------------------------------------------

        void FixOrientation()
        {
          //Preconditions:
          //1. All output polygons are simple polygons (ie no self-intersecting edges)
          //2. While output polygons may touch, none of them overlap other polygons.
          if (m_PolyPts.Count == 0) return;
          SkipList<TPolyPt> queue = new SkipList<TPolyPt>(blCompare);

          for (int i = 0; i < m_PolyPts.Count; ++i)
            if (m_PolyPts[i] != null)
            {
              //first, find the lowest left most PPolyPt for each polygon ...
              TPolyPt p = m_PolyPts[i];

              TPolyPt lowestP = p;
              p = p.next;
              do
              {
                if (p.pt.Y > lowestP.pt.Y) lowestP = p;
                else if (p.pt.Y == lowestP.pt.Y &&
                  p.pt.X <= lowestP.pt.X) lowestP = p;
                p = p.next;
              }
              while ( p != m_PolyPts[i] );

              //dispose of any invalid polygons here ...
              p = lowestP;
              if (p.next == p || p.prev == p.next)
              {
                DisposePolyPts(p);
                m_PolyPts[i] = null;
                continue;
              }
              m_PolyPts[i] = lowestP;
              
              TOrientationFlag tmpFlag;
              if (IsClockwise(lowestP)) tmpFlag = TOrientationFlag.ofCW; else tmpFlag = TOrientationFlag.ofEmpty;
              lowestP.flags = tmpFlag;
              bool lowestPending = true;

              //loop around the polygon, build 'bounds' for each polygon
              //and add them to the queue ...
              do
              {
                while (p.next.pt.Y == p.pt.Y) p = p.next; //ignore horizontals

                p.flags = tmpFlag | TOrientationFlag.ofForwardBound;
                if (lowestPending && (TOrientationFlag.ofCW & lowestP.flags) != 0)
                {
                    p.flags = p.flags | TOrientationFlag.ofBottomMinima;
                    lowestPending = false;
                }

                queue.InsertItem(p);
                //go up the bound ...
                while (p.next.pt.Y <= p.pt.Y)
                {
                    p.next.flags = tmpFlag | TOrientationFlag.ofForwardBound;
                    p = p.next;
                }
                p.flags = p.flags | TOrientationFlag.ofTop;
                //now add the reverse bound (also reversing the bound direction) ...
                while (!NextIsBottom(p))
                {
                  p.next.flags = tmpFlag;
                  p = p.next;
                }

                if (p.next.pt.Y == p.next.next.pt.Y)
                {
                  p = p.next;
                  p.flags = tmpFlag;
                  if ((TOrientationFlag.ofCW & lowestP.flags) == 0 && 
                      PointsEqual(p.pt, lowestP.pt) && lowestPending )
                  {
                      p.flags = p.flags | TOrientationFlag.ofBottomMinima;
                      lowestPending = false;
                  }
                  queue.InsertItem(p);
                  while (p != lowestP && p.next.pt.Y == p.pt.Y) p = p.next;
                } else
                {
                  p = DuplicatePolyPt(p);
                  p.pt = p.next.pt;
                  p.flags = tmpFlag;
                  if ((TOrientationFlag.ofCW & lowestP.flags) == 0 && PointsEqual(p.pt, lowestP.pt))
                      p.flags = p.flags | TOrientationFlag.ofBottomMinima;
                  queue.InsertItem(p);
                  p = p.next;
                  while (p != lowestP && p.next.pt.Y == p.prev.pt.Y) p = p.next;
                }
              }
              while (p != lowestP);
            }

            if (queue.Count() == 0) return;
            SkipList<TPolyPt> workList = new SkipList<TPolyPt>(wlCompare);

            TPolyPt pp = queue.PopFirst();
            workList.InsertItem(pp);
            pp.flags = pp.flags | TOrientationFlag.ofClockwise;

            for (;;)
            {
              TPolyPt p = queue.PopFirst();
              if (p == null) break;

              SkipNode<TPolyPt> sn = workList.First();
              while (sn != null)
              {
                //get the next item in workList in case sn is about to be removed ...
                  SkipNode<TPolyPt> sn2 = sn.next[0];
                //update each bound, keeping them level with the new bound 'p'
                //and removing bounds that are no longer in scope ...
                //nb: Bounds never intersect other bounds so UpdateBounds() should
                //not upset the order of the bounds in worklist.
                UpdateBounds(workList, sn, p.pt.Y);
                sn = sn2;
              }

              //insert the new bound into WorkList ...
              sn = workList.InsertItem(p);

              //if this is the bottom bound of a polyon,
              //then calculate the polygon's true orientation ...
              if ((TOrientationFlag.ofBottomMinima & p.flags) != 0)
              {
                SkipNode<TPolyPt> sn2 = workList.First();
                bool isCW = true;
                while (sn2 != sn)
                {
                  isCW = !isCW;
                  sn2 = sn2.next[0];
                }
                if (isCW) p.flags = p.flags | TOrientationFlag.ofClockwise;
              }
            }

          for (int i = 0; i < m_PolyPts.Count; ++i)
            if (m_PolyPts[i] != null)
            {
                TPolyPt p = (TPolyPt)m_PolyPts[i];
                do
                {
                    if ((TOrientationFlag.ofBottomMinima & p.flags) != 0) break;
                    p = p.next;
                }
                while (p != m_PolyPts[i]);
                if ((TOrientationFlag.ofBottomMinima & p.flags) == 0)
                    throw new clipperException("FixOrientation error");
                if ((p.flags & (TOrientationFlag.ofCW | TOrientationFlag.ofClockwise)) == TOrientationFlag.ofCW ||
                     (p.flags & (TOrientationFlag.ofCW | TOrientationFlag.ofClockwise)) == TOrientationFlag.ofClockwise)
                      ReversePolyPtLinks(p);
                p = FixupOutPolygon2(p);
                m_PolyPts[i] = p;
            }
          queue = null;
          workList = null;
        }
        //------------------------------------------------------------------------------
    }

    //------------------------------------------------------------------------------
    //------------------------------------------------------------------------------
    
    class clipperException : Exception
    {
        private string m_description;
        public clipperException(string description)
        {
            m_description = description;
            Console.WriteLine(m_description);
            throw new Exception(m_description);
        }
    }
    //------------------------------------------------------------------------------

}

 