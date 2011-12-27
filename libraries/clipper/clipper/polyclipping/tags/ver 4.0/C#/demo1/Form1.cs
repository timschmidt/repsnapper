using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Drawing;
using System.Drawing.Drawing2D;
using System.Drawing.Imaging;
using System.IO;
using System.Reflection;
using System.Linq;
using System.Windows.Forms;
using clipper;


namespace WindowsFormsApplication1
{

    using Polygon = List<IntPoint>;
    using Polygons = List<List<IntPoint>>;

    public partial class Form1 : Form
    {

        Assembly _assembly;
        Stream polyStream;

        private Bitmap mybitmap;
        private Polygons subjects;
        private Polygons clips;

        //Here we are scaling all coordinates up by 100 when they're passed to Clipper 
        //via Polygon (or Polygons) objects because Clipper no longer accepts floating  
        //point values. Likewise when Clipper returns a solution in a Polygons object, 
        //we need to scale down these returned values by the same amount before displaying.
        //Note: When scaling, keep in mind that integers are only 32bit values, so 
        //there's a limit to the precision possible before range errors will be encountered.
        private int scale = 100; //or 1 or 10 or 10000 etc for lesser or greater precision.

        //---------------------------------------------------------------------
        //---------------------------------------------------------------------

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
            polyStream = _assembly.GetManifestResourceStream("ClipperCSharpDemo1.australia.bin");
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

            for (int i = 0; i < count; ++i)
            {
                int w = pictureBox1.ClientRectangle.Width - 220;
                int h = pictureBox1.ClientRectangle.Height - 140 - statusStrip1.Height;

                pt.X = rand.Next(w) + panel1.Width + 10;
                pt.Y = rand.Next(h) + 30;
                int size = rand.Next(90) + 10;
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
            IntPoint newPt = new IntPoint();
            newPt.X = (rand.Next(r / 10) * 10 + l + 10) * scale;
            newPt.Y = (rand.Next(b / 10) * 10 + t + 10) * scale;
            return newPt;
        }
        //---------------------------------------------------------------------

        private void GenerateRandomPolygon(int count)
        {
            Random rand = new Random();
            int l = 10;
            int t = 0;
            int r = (pictureBox1.ClientRectangle.Width - 20)/10 *10;
            int b = (pictureBox1.ClientRectangle.Height - 20)/10 *10;

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

        ClipType GeClipType()
        {
            if (rbIntersect.Checked) return ClipType.ctIntersection;
            if (rbUnion.Checked) return ClipType.ctUnion;
            if (rbDifference.Checked) return ClipType.ctDifference;
            else return ClipType.ctXor;
        }
        //---------------------------------------------------------------------

        PolyFillType GePolyFillType()
        {
            if (rbNonZero.Checked) return PolyFillType.pftNonZero;
            else return PolyFillType.pftEvenOdd;
        }
        //---------------------------------------------------------------------

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
            newgraphic.Clear(Color.WhiteSmoke);
            Pen myPen = new Pen(Color.LightSlateGray, (float)1.5);
            SolidBrush myBrush = new SolidBrush(Color.FromArgb(16, Color.Blue));
            
            GraphicsPath path = new GraphicsPath();
            if (rbNonZero.Checked) path.FillMode = FillMode.Winding;

            foreach (Polygon pg in subjects)
            {
                PointF[] pts = PolygonToPointFArray(pg, scale);
                path.AddPolygon(pts);
                pts = null;
            }
            newgraphic.FillPath(myBrush, path);
            newgraphic.DrawPath(myPen, path);
            path.Reset();
            if (rbNonZero.Checked) path.FillMode = FillMode.Winding;
            foreach (Polygon pg in clips)
            {
                PointF[] pts = PolygonToPointFArray(pg, scale);
                path.AddPolygon(pts);
                pts = null;
            }
            myPen.Color = Color.LightSalmon;
            myBrush.Color = Color.FromArgb(16, Color.Red);
            newgraphic.FillPath(myBrush, path);
            newgraphic.DrawPath(myPen, path);

            //do the clipping ...
            if ((clips.Count > 0 || subjects.Count > 0) && !rbNone.Checked)
            {
                Polygons solution = new Polygons();
                //Stopwatch sw = new Stopwatch();
                //sw.Start();

                clipper.Clipper c = new clipper.Clipper();
                c.AddPolygons(subjects, PolyType.ptSubject);
                c.AddPolygons(clips, PolyType.ptClip);
                if (!c.Execute(GeClipType(), solution, GePolyFillType(), GePolyFillType()))
                {
                    Console.Beep(1250, 250);
                }
                
                //sw.Stop();
                //TimeSpan ts = sw.Elapsed;
                //this.Text = Convert.ToString(ts.TotalMilliseconds);

                myBrush.Color = Color.Black;
                path.Reset();

                //It really shouldn't matter what FillMode is used for solution
                //polygons because none of the solution polygons overlap. 
                //However, FillMode.Winding will show any orientation errors where 
                //holes will be stroked (outlined) correctly but filled incorrectly  ...
                path.FillMode = FillMode.Winding; 

                //or for something fancy ...
                if (nudOffset.Value != 0)
                    solution = clipper.Clipper.OffsetPolygons(solution, (double)nudOffset.Value * scale);
                foreach (Polygon pg in solution)
                {
                    PointF[] pts = PolygonToPointFArray(pg, scale);
                    if (pts.Count() > 2)
                      path.AddPolygon(pts);
                    pts = null;
                }
                myBrush.Color = Color.FromArgb(128, Color.Yellow);
                myPen.Color = Color.Black;
                newgraphic.FillPath(myBrush, path);
                newgraphic.DrawPath(myPen, path);

                foreach (Polygon pg in solution)
                {
                    PointF[] pts = PolygonToPointFArray(pg, scale);
                    path.Reset();
                    path.AddPolygon(pts);
                }
            }
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
                    MessageBox.Show(this.Text + "\nby Angus Johnson\nCopyright © 2010",
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

    }
}
