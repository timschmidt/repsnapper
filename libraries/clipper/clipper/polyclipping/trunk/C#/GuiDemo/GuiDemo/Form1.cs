using System;
using System.Diagnostics;
using System.Text;
using System.Collections.Generic;
using System.Drawing;
using System.Drawing.Drawing2D;
using System.Drawing.Imaging;
using System.IO;
using System.Reflection;
using System.Linq;
using System.Windows.Forms;
using System.Globalization;
using ClipperLib;


namespace WindowsFormsApplication1
{

    using Polygon = List<IntPoint>;
    using Polygons = List<List<IntPoint>>;
    using ExPolygons = List<ExPolygon>;

    public partial class Form1 : Form
    {

        Assembly _assembly;
        Stream polyStream;

        private Bitmap mybitmap;
        private Polygons subjects;
        private Polygons clips;
        private Polygons solution;
        private ExPolygons exSolution;

        //Here we are scaling all coordinates up by 100 when they're passed to Clipper 
        //via Polygon (or Polygons) objects because Clipper no longer accepts floating  
        //point values. Likewise when Clipper returns a solution in a Polygons object, 
        //we need to scale down these returned values by the same amount before displaying.
        private int scale = 100; //or 1 or 10 or 10000 etc for lesser or greater precision.

        //---------------------------------------------------------------------
        //---------------------------------------------------------------------

        //a very simple class that builds an SVG file with any number of 
        //polygons of the specified formats ...
        class SVGBuilder
        {

            public class StyleInfo
            {
                public PolyFillType pft;
                public Color brushClr;
                public Color penClr;
                public double penWidth;
                public int[] dashArray;
                public Boolean showCoords;
                public StyleInfo Clone()
                {
                    StyleInfo si = new StyleInfo();
                    si.pft = this.pft;
                    si.brushClr = this.brushClr;
                    si.dashArray = this.dashArray;
                    si.penClr = this.penClr;
                    si.penWidth = this.penWidth;
                    si.showCoords = this.showCoords;
                    return si;
                }
                public StyleInfo()
                {
                    pft = PolyFillType.pftNonZero;
                    brushClr = Color.AntiqueWhite;
                    dashArray = null;
                    penClr = Color.Black;
                    penWidth = 0.8;
                    showCoords = false;
                }
            }

            public class PolyInfo
            {
                public Polygons polygons;
                public StyleInfo si;
            }

            public StyleInfo style;
            private List<PolyInfo> PolyInfoList;
            const string svg_header = "<?xml version=\"1.0\" standalone=\"no\"?>\n" +
              "<!DOCTYPE svg PUBLIC \"-//W3C//DTD SVG 1.0//EN\"\n" +
              "\"http://www.w3.org/TR/2001/REC-SVG-20010904/DTD/svg10.dtd\">\n\n" +
              "<svg width=\"{0}px\" height=\"{1}px\" viewBox=\"0 0 {2} {3}\" " +
              "version=\"1.1\" xmlns=\"http://www.w3.org/2000/svg\">\n\n";
            const string svg_path_format = "\"\n style=\"fill:{0};" +
                " fill-opacity:{1:f2}; fill-rule:{2}; stroke:{3};" +
                " stroke-opacity:{4:f2}; stroke-width:{5:f2};\"/>\n\n";

            public SVGBuilder()
            {
                PolyInfoList = new List<PolyInfo>();
                style = new StyleInfo();
            }

            public void AddPolygons(Polygons poly)
            {
                if (poly.Count == 0) return;
                PolyInfo pi = new PolyInfo();
                pi.polygons = poly;
                pi.si = style.Clone();
                PolyInfoList.Add(pi);
            }

            public Boolean SaveToFile(string filename, double scale = 1.0, int margin = 10)
            {
                if (scale == 0) scale = 1.0;
                if (margin < 0) margin = 0;

                //calculate the bounding rect ...
                int i = 0, j = 0;
                while (i < PolyInfoList.Count)
                {
                    j = 0;
                    while (j < PolyInfoList[i].polygons.Count &&
                        PolyInfoList[i].polygons[j].Count == 0) j++;
                    if (j < PolyInfoList[i].polygons.Count) break;
                    i++;
                }
                if (i == PolyInfoList.Count) return false;
                IntRect rec = new IntRect();
                rec.left = PolyInfoList[i].polygons[j][0].X;
                rec.right = rec.left;
                rec.top = PolyInfoList[0].polygons[j][0].Y;
                rec.bottom = rec.top;

                for (; i < PolyInfoList.Count; i++)
                {
                    foreach (Polygon pg in PolyInfoList[i].polygons)
                        foreach (IntPoint pt in pg)
                        {
                            if (pt.X < rec.left) rec.left = pt.X;
                            else if (pt.X > rec.right) rec.right = pt.X;
                            if (pt.Y < rec.top) rec.top = pt.Y;
                            else if (pt.Y > rec.bottom) rec.bottom = pt.Y;
                        }
                }

                rec.left = (Int64)((double)rec.left * scale);
                rec.top = (Int64)((double)rec.top * scale);
                rec.right = (Int64)((double)rec.right * scale);
                rec.bottom = (Int64)((double)rec.bottom * scale);
                Int64 offsetX = -rec.left + margin;
                Int64 offsetY = -rec.top + margin;

                StreamWriter writer = new StreamWriter(filename);
                if (writer == null) return false;
                writer.Write(svg_header,
                    (rec.right - rec.left) + margin * 2,
                    (rec.bottom - rec.top) + margin * 2,
                    (rec.right - rec.left) + margin * 2,
                    (rec.bottom - rec.top) + margin * 2);

                foreach (PolyInfo pi in PolyInfoList)
                {
                    writer.Write(" <path d=\"");
                    foreach (Polygon p in pi.polygons)
                    {
                        if (p.Count < 3) continue;
                        writer.Write(String.Format(NumberFormatInfo.InvariantInfo, " M {0:f2} {1:f2}",
                            (double)((double)p[0].X * scale + offsetX),
                            (double)((double)p[0].Y * scale + offsetY)));
                        for (int k = 1; k < p.Count; k++)
                        {
                            writer.Write(String.Format(NumberFormatInfo.InvariantInfo, " L {0:f2} {1:f2}",
                            (double)((double)p[k].X * scale + offsetX),
                            (double)((double)p[k].Y * scale + offsetY)));
                        }
                        writer.Write(" z");
                    }

                    writer.Write(String.Format(NumberFormatInfo.InvariantInfo, svg_path_format,
                    ColorTranslator.ToHtml(pi.si.brushClr),
                    (float)pi.si.brushClr.A / 255,
                    (pi.si.pft == PolyFillType.pftEvenOdd ? "evenodd" : "nonzero"),
                    ColorTranslator.ToHtml(pi.si.penClr),
                    (float)pi.si.penClr.A / 255,
                    pi.si.penWidth));

                    if (pi.si.showCoords)
                    {
                        writer.Write("<g font-family=\"Verdana\" font-size=\"11\" fill=\"black\">\n\n");
                        foreach (Polygon p in pi.polygons)
                        {
                            foreach (IntPoint pt in p)
                            {
                                Int64 x = pt.X;
                                Int64 y = pt.Y;
                                writer.Write(String.Format(
                                    "<text x=\"{0}\" y=\"{1}\">{2},{3}</text>\n",
                                    (int)(x * scale + offsetX), (int)(y * scale + offsetY), x, y));

                            }
                            writer.Write("\n");
                        }
                        writer.Write("</g>\n");
                    }
                }
                writer.Write("</svg>\n");
                writer.Close();
                return true;
            }
        }

        //------------------------------------------------------------------------------
        //------------------------------------------------------------------------------

        static private System.Drawing.PointF[] PolygonToPointFArray(Polygon pg, int scale)
        {
            System.Drawing.PointF[] result = new System.Drawing.PointF[pg.Count];
            for (int i = 0; i < pg.Count; ++i)
            {
                result[i].X = (float)pg[i].X / scale;
                result[i].Y = (float)pg[i].Y / scale;
            }
            return result;
        }
        //---------------------------------------------------------------------

        public Form1()
        {
            InitializeComponent();
            this.MouseWheel += new MouseEventHandler(Form1_MouseWheel);
        }
        //---------------------------------------------------------------------

        private void Form1_MouseWheel(object sender, MouseEventArgs e)
        {
            if (e.Delta > 0 && nudOffset.Value < 10) nudOffset.Value += (decimal)0.5;
            else if (e.Delta < 0 && nudOffset.Value > -10) nudOffset.Value -= (decimal)0.5;
        }
        //---------------------------------------------------------------------

        private void bRefresh_Click(object sender, EventArgs e)
        {
            DrawBitmap();
        }
        //---------------------------------------------------------------------

        private void GenerateAustPlusRandomEllipses(int count)
        {
            subjects.Clear();
            //load map of Australia from resource ...
            _assembly = Assembly.GetExecutingAssembly();
            polyStream = _assembly.GetManifestResourceStream("GuiDemo.aust.bin");
            int len = (int)polyStream.Length;
            byte[] b = new byte[len];
            polyStream.Read(b, 0, len);
            int polyCnt = BitConverter.ToInt32(b, 0);
            int k = 4;
            for (int i = 0; i < polyCnt; ++i)
            {
                int vertCnt = BitConverter.ToInt32(b, k);
                k += 4;
                Polygon pg = new Polygon(vertCnt);
                for (int j = 0; j < vertCnt; ++j)
                {
                    float x = BitConverter.ToSingle(b, k) * scale;
                    float y = BitConverter.ToSingle(b, k + 4) * scale;
                    k += 8;
                    pg.Add(new IntPoint((int)x, (int)y));
                }
                subjects.Add(pg);
            }

            clips.Clear();
            Random rand = new Random();
            GraphicsPath path = new GraphicsPath();
            Point pt = new Point();

            const int ellipse_size = 100, margin = 10;
            for (int i = 0; i < count; ++i)
            {
                int w = pictureBox1.ClientRectangle.Width - ellipse_size - margin *2;
                int h = pictureBox1.ClientRectangle.Height - ellipse_size - margin * 2 - statusStrip1.Height;

                pt.X = rand.Next(w) + margin;
                pt.Y = rand.Next(h) + margin;
                int size = rand.Next(ellipse_size - 20) + 20;
                path.Reset();
                path.AddEllipse(pt.X, pt.Y, size, size);
                path.Flatten();
                Polygon clip = new Polygon(path.PathPoints.Count());
                foreach (PointF p in path.PathPoints)
                    clip.Add(new IntPoint((int)(p.X * scale), (int)(p.Y * scale)));
                clips.Add(clip);
            }
        }
        //---------------------------------------------------------------------

        private IntPoint GenerateRandomPoint(int l, int t, int r, int b, Random rand)
        {
            int Q = 10;
            IntPoint newPt = new IntPoint();
            newPt.X = (rand.Next(r / Q) * Q + l + 10) * scale;
            newPt.Y = (rand.Next(b / Q) * Q + t + 10) * scale;
            return newPt;
        }
        //---------------------------------------------------------------------

        private void GenerateRandomPolygon(int count)
        {
            int Q = 10;
            Random rand = new Random();
            int l = 10;
            int t = 10;
            int r = (pictureBox1.ClientRectangle.Width - 20) / Q * Q;
            int b = (pictureBox1.ClientRectangle.Height - 20) / Q * Q;

            subjects.Clear();
            clips.Clear();

            Polygon subj = new Polygon(count);
            for (int i = 0; i < count; ++i)
                subj.Add(GenerateRandomPoint(l, t, r, b, rand));
            subjects.Add(subj);

            Polygon clip = new Polygon(count);
            for (int i = 0; i < count; ++i)
                clip.Add(GenerateRandomPoint(l, t, r, b, rand));
            clips.Add(clip);
        }
        //---------------------------------------------------------------------

        ClipType GetClipType()
        {
            if (rbIntersect.Checked) return ClipType.ctIntersection;
            if (rbUnion.Checked) return ClipType.ctUnion;
            if (rbDifference.Checked) return ClipType.ctDifference;
            else return ClipType.ctXor;
        }
        //---------------------------------------------------------------------

        PolyFillType GetPolyFillType()
        {
            if (rbNonZero.Checked) return PolyFillType.pftNonZero;
            else return PolyFillType.pftEvenOdd;
        }
        //---------------------------------------------------------------------

        bool LoadFromFile(string filename, Polygons ppg, double scale = 0,
          int xOffset = 0, int yOffset = 0)
        {
            double scaling = Math.Pow(10, scale);
            ppg.Clear();
            if (!File.Exists(filename)) return false;
            StreamReader sr = new StreamReader(filename);
            if (sr == null) return false;
            string line;
            if ((line = sr.ReadLine()) == null) return false;
            int polyCnt, vertCnt;
            if (!Int32.TryParse(line, out polyCnt) || polyCnt < 0) return false;
            ppg.Capacity = polyCnt;
            for (int i = 0; i < polyCnt; i++)
            {
                if ((line = sr.ReadLine()) == null) return false;
                if (!Int32.TryParse(line, out vertCnt) || vertCnt < 0) return false;
                Polygon pg = new Polygon(vertCnt);
                ppg.Add(pg);
                for (int j = 0; j < vertCnt; j++)
                {
                    double x, y;
                    if ((line = sr.ReadLine()) == null) return false;
                    char[] delimiters = new char[] { ',', ' ' };
                    string [] vals = line.Split(delimiters);
                    if (vals.Length < 2) return false;
                    if (!double.TryParse(vals[0], out x)) return false;
                    if (!double.TryParse(vals[1], out y))
                        if (vals.Length < 2 || !double.TryParse(vals[2], out y)) return false;
                    x = x * scaling + xOffset;
                    y = y * scaling + yOffset;
                    pg.Add(new IntPoint((int)Math.Round(x), (int)Math.Round(y)));
                }
            }
            return true;
        }
        //------------------------------------------------------------------------------

        void SaveToFile(string filename, Polygons ppg, int scale = 0)
        {
            double scaling = Math.Pow(10, scale);
            StreamWriter writer = new StreamWriter(filename);
            if (writer == null) return;
            writer.Write("{0}\n", ppg.Count);
            foreach (Polygon pg in ppg)
            {
                writer.Write("{0}\n", pg.Count);
                foreach (IntPoint ip in pg)
                    writer.Write("{0:0.0000}, {1:0.0000}\n", 
                        (double)ip.X/scaling, (double)ip.Y/scaling);
            }
            writer.Close();
        }
        //---------------------------------------------------------------------------

        private void DrawBitmap(bool justClip = false)
        {

            if (!justClip)
            {
                if (rbTest2.Checked)
                    GenerateAustPlusRandomEllipses((int)nudCount.Value);
                else
                    GenerateRandomPolygon((int)nudCount.Value);
            }

            Cursor.Current = Cursors.WaitCursor;
            Graphics newgraphic;
            newgraphic = Graphics.FromImage(mybitmap);
            newgraphic.SmoothingMode = SmoothingMode.AntiAlias;
            newgraphic.Clear(Color.White);
            
            GraphicsPath path = new GraphicsPath();
            if (rbNonZero.Checked) path.FillMode = FillMode.Winding;

            //draw subjects ...
            foreach (Polygon pg in subjects)
            {
                PointF[] pts = PolygonToPointFArray(pg, scale);
                path.AddPolygon(pts);
                pts = null;
            }
            Pen myPen = new Pen(Color.FromArgb(196, 0xC3, 0xC9, 0xCF), (float)0.6);
            SolidBrush myBrush = new SolidBrush(Color.FromArgb(127, 0xDD, 0xDD, 0xF0));
            newgraphic.FillPath(myBrush, path);
            newgraphic.DrawPath(myPen, path);
            path.Reset();

            //draw clips ...
            if (rbNonZero.Checked) path.FillMode = FillMode.Winding;
            foreach (Polygon pg in clips)
            {
                PointF[] pts = PolygonToPointFArray(pg, scale);
                path.AddPolygon(pts);
                pts = null;
            }
            myPen.Color = Color.FromArgb(196, 0xF9, 0xBE, 0xA6);
            myBrush.Color = Color.FromArgb(127, 0xFF, 0xE0, 0xE0);
            newgraphic.FillPath(myBrush, path);
            newgraphic.DrawPath(myPen, path);

            //do the clipping ...
            if ((clips.Count > 0 || subjects.Count > 0) && !rbNone.Checked)
            {                
                Polygons solution2 = new Polygons();
                Clipper c = new Clipper();
                c.AddPolygons(subjects, PolyType.ptSubject);
                c.AddPolygons(clips, PolyType.ptClip);
                exSolution.Clear();
                solution.Clear();
                bool succeeded = c.Execute(GetClipType(), solution, GetPolyFillType(), GetPolyFillType());
                if (succeeded)
                {
                    myBrush.Color = Color.Black;
                    path.Reset();

                    //It really shouldn't matter what FillMode is used for solution
                    //polygons because none of the solution polygons overlap. 
                    //However, FillMode.Winding will show any orientation errors where 
                    //holes will be stroked (outlined) correctly but filled incorrectly  ...
                    path.FillMode = FillMode.Winding;

                    //or for something fancy ...
                    if (nudOffset.Value != 0)
                        solution2 = Clipper.OffsetPolygons(solution, (double)nudOffset.Value * scale, JoinType.jtMiter);
                    else
                        solution2 = new Polygons(solution);
                    foreach (Polygon pg in solution2)
                    {
                        PointF[] pts = PolygonToPointFArray(pg, scale);
                        if (pts.Count() > 2)
                            path.AddPolygon(pts);
                        pts = null;
                    }
                    myBrush.Color = Color.FromArgb(127, 0x66, 0xEF, 0x7F);
                    myPen.Color = Color.FromArgb(255, 0, 0x33, 0);
                    myPen.Width = 1.0f;
                    newgraphic.FillPath(myBrush, path);
                    newgraphic.DrawPath(myPen, path);

                    //now do some fancy testing ...
                    Font f = new Font("Arial", 8);
                    SolidBrush b = new SolidBrush(Color.Navy);
                    double subj_area = 0, clip_area = 0, int_area = 0, union_area = 0;
                    c.Clear();
                    c.AddPolygons(subjects, PolyType.ptSubject);
                    c.Execute(ClipType.ctUnion, solution2, GetPolyFillType(), GetPolyFillType());
                    foreach (Polygon pg in solution2) subj_area += Clipper.Area(pg);
                    c.Clear();
                    c.AddPolygons(clips, PolyType.ptClip);
                    c.Execute(ClipType.ctUnion, solution2, GetPolyFillType(), GetPolyFillType());
                    foreach (Polygon pg in solution2) clip_area += Clipper.Area(pg);
                    c.AddPolygons(subjects, PolyType.ptSubject);
                    c.Execute(ClipType.ctIntersection, solution2, GetPolyFillType(), GetPolyFillType());
                    foreach (Polygon pg in solution2) int_area += Clipper.Area(pg);
                    c.Execute(ClipType.ctUnion, solution2, GetPolyFillType(), GetPolyFillType());
                    foreach (Polygon pg in solution2) union_area += Clipper.Area(pg);

                    StringFormat lftStringFormat = new StringFormat();
                    lftStringFormat.Alignment = StringAlignment.Near;
                    lftStringFormat.LineAlignment = StringAlignment.Near;
                    StringFormat rtStringFormat = new StringFormat();
                    rtStringFormat.Alignment = StringAlignment.Far;
                    rtStringFormat.LineAlignment = StringAlignment.Near;
                    Rectangle rec = new Rectangle(pictureBox1.ClientSize.Width - 108, 
                        pictureBox1.ClientSize.Height - 116, 104, 106);
                    newgraphic.FillRectangle(new SolidBrush(Color.FromArgb(196, Color.WhiteSmoke)), rec);
                    newgraphic.DrawRectangle(myPen, rec);
                    rec.Inflate(new Size(-2, 0));
                    newgraphic.DrawString("Areas", f, b, rec, rtStringFormat);
                    rec.Offset(new Point(0, 14));
                    newgraphic.DrawString("subj: ", f, b, rec, lftStringFormat);
                    newgraphic.DrawString((subj_area / 100000).ToString("0,0"), f, b, rec, rtStringFormat);
                    rec.Offset(new Point(0, 12));
                    newgraphic.DrawString("clip: ", f, b, rec, lftStringFormat);
                    newgraphic.DrawString((clip_area / 100000).ToString("0,0"), f, b, rec, rtStringFormat);
                    rec.Offset(new Point(0, 12));
                    newgraphic.DrawString("intersect: ", f, b, rec, lftStringFormat);
                    newgraphic.DrawString((int_area / 100000).ToString("0,0"), f, b, rec, rtStringFormat);
                    rec.Offset(new Point(0, 12));
                    newgraphic.DrawString("---------", f, b, rec, rtStringFormat);
                    rec.Offset(new Point(0, 10));
                    newgraphic.DrawString("s + c - i: ", f, b, rec, lftStringFormat);
                    newgraphic.DrawString(((subj_area + clip_area - int_area) / 100000).ToString("0,0"), f, b, rec, rtStringFormat);
                    rec.Offset(new Point(0, 10));
                    newgraphic.DrawString("---------", f, b, rec, rtStringFormat);
                    rec.Offset(new Point(0, 10));
                    newgraphic.DrawString("union: ", f, b, rec, lftStringFormat);
                    newgraphic.DrawString((union_area / 100000).ToString("0,0"), f, b, rec, rtStringFormat);
                    rec.Offset(new Point(0, 10));
                    newgraphic.DrawString("---------", f, b, rec, rtStringFormat);
                } //end if succeeded
            } //end if something to clip

            pictureBox1.Image = mybitmap;
            newgraphic.Dispose();
            Cursor.Current = Cursors.Default;
        }
        //---------------------------------------------------------------------

        private void Form1_Load(object sender, EventArgs e)
        {
            mybitmap = new Bitmap(
                pictureBox1.ClientRectangle.Width,
                pictureBox1.ClientRectangle.Height,
                PixelFormat.Format32bppArgb);

            subjects = new Polygons(); 
            clips = new Polygons();
            solution = new Polygons();
            exSolution = new ExPolygons();

            toolStripStatusLabel1.Text =
                "Tip: Use the mouse-wheel (or +,-,0) to adjust the offset of the solution polygons.";
            DrawBitmap();
        }
        //---------------------------------------------------------------------

        private void bClose_Click(object sender, EventArgs e)
        {
            Close();
        }
        //---------------------------------------------------------------------

        private void Form1_Resize(object sender, EventArgs e)
        {
            if (pictureBox1.ClientRectangle.Width == 0 || 
                pictureBox1.ClientRectangle.Height == 0) return;
            mybitmap.Dispose();
            mybitmap = new Bitmap(
                pictureBox1.ClientRectangle.Width,
                pictureBox1.ClientRectangle.Height,
                PixelFormat.Format32bppArgb);
            pictureBox1.Image = mybitmap;
            DrawBitmap();
        }
        //---------------------------------------------------------------------

        private void rbNonZero_Click(object sender, EventArgs e)
        {
            DrawBitmap(true);
        }
        //---------------------------------------------------------------------

        private void Form1_KeyDown(object sender, KeyEventArgs e)
        {
            switch (e.KeyCode)
            {
                case (Keys)27:
                    this.Close();
                    return;
                case Keys.F1:
                    MessageBox.Show(this.Text + "\nby Angus Johnson\nCopyright © 2010, 2011",
                    this.Text, MessageBoxButtons.OK, MessageBoxIcon.Information);
                    e.Handled = true;
                    return;
                case (Keys)187:
                case Keys.Add:
                    if (nudOffset.Value == 10) return;
                    nudOffset.Value += (decimal)0.5;
                    e.Handled = true;
                    break;
                case (Keys)189:
                case Keys.Subtract:
                    if (nudOffset.Value == -10) return;
                    nudOffset.Value -= (decimal)0.5;
                    e.Handled = true;
                    break;
                case Keys.NumPad0:
                case Keys.D0:
                    if (nudOffset.Value == 0) return;
                    nudOffset.Value = (decimal)0;
                    e.Handled = true;
                    break;
                default: return;
            }
            
        }
        //---------------------------------------------------------------------

        private void nudCount_ValueChanged(object sender, EventArgs e)
        {
            DrawBitmap(true);
        }
        //---------------------------------------------------------------------

        private void rbTest1_Click(object sender, EventArgs e)
        {
            if (rbTest1.Checked)
                lblCount.Text = "Vertex &Count:";
            else
                lblCount.Text = "Ellipse &Count:";
            DrawBitmap();
        }
        //---------------------------------------------------------------------

        private void bSave_Click(object sender, EventArgs e)
        {
            //save to SVG ...
            if (saveFileDialog1.ShowDialog() == DialogResult.OK)
            {
                PolyFillType pft = GetPolyFillType();
                SVGBuilder svg = new SVGBuilder();
                svg.style.brushClr = Color.FromArgb(0x10, 0, 0, 0x9c);
                svg.style.penClr = Color.FromArgb(0xd3, 0xd3, 0xda);
                svg.AddPolygons(subjects);
                svg.style.brushClr = Color.FromArgb(0x10, 0x9c, 0, 0);
                svg.style.penClr = Color.FromArgb(0xff, 0xa0, 0x7a);
                svg.AddPolygons(clips);
                svg.style.brushClr = Color.FromArgb(0xAA, 0x80, 0xff, 0x9c);
                svg.style.penClr = Color.FromArgb(0, 0x33, 0);
                svg.AddPolygons(solution);
                svg.SaveToFile(saveFileDialog1.FileName, 1.0 / scale);
            }
        }
        //---------------------------------------------------------------------

    }
}
