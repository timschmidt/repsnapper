/*
    This file is a part of the RepSnapper project.
    Copyright (C) 2012  martin.dieringer@gmx.de

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#include "flatshape.h"
#include "ui/progress.h"
//#include "settings.h"
#include "slicer/clipping.h"
#include "ui/draw.h"

#include <QRegularExpressionMatch>
//#include "glibmm.h"

#ifdef _OPENMP
#include <omp.h>
#endif


FlatShape::FlatShape()
{
  slow_drawing = false;
  Min.set(0,0,0);
  Max.set(200,200,0);
  CalcBBox();
}


FlatShape::FlatShape(Glib::ustring filename)
{
  slow_drawing = false;
  this->filename = filename;
  loadSVG(filename);
}

// FlatShape::FlatShape(const FlatShape &rhs)
// {
//   slow_drawing = false;
//   polygons = rhs.polygons;
//   scale_factor_x = rhs.scale_factor_x;
//   scale_factor_y = rhs.scale_factor_y;
//   scale_factor = rhs.scale_factor;
//   Min = rhs.Min;
//   Max = rhs.Max;
//   Center = rhs.Center;
// }

QString REMatch(const QRegularExpression &RE,
                const QString &input,
                const QString &name)
{
    QRegularExpressionMatch match = RE.match(input);
    if (match.hasMatch()) {
        return match.captured(name);
    }
    return nullptr;
}

QString REMatch(const QString &regex,
                const QString &input,
                const QString &name)
{
  return REMatch(QRegularExpression(regex), input, name);
}

// int loadSVGold(string filename)
// {
//   Min=Vector3d(1000000,1000000,0);
//   Max=Vector3d(-1000000,-1000000,0);

//   polygons.clear();

//   ifstream file;
//   file.open(filename.c_str());


//   string lines;
//   string line;
//   if (file.is_open()) {
//     while ( file.good() ) {
//       getline (file,line);
//       lines += line;
//     }
//     file.close();

//     Glib::RefPtr<Glib::Regex> polyregex =
//       Glib::Regex::create ("(?ims)<path.*?stroke\\:none.*?\\sd\\=\"(?<POLY>.*?(Z|\"/\\>))");
//     Glib::RefPtr<Glib::Regex> strokeregex =
//       Glib::Regex::create ("(?ims)<path.*?(?<STROKE>fill\\:none.*?\\sd\\=\".*?\"/\\>)");
//     Glib::RefPtr<Glib::Regex> strwidthregex =
//       Glib::Regex::create ("stroke\\-width\\:(?<STRWIDTH>[\\-\\.\\d]+)");
//     Glib::RefPtr<Glib::Regex> pointregex =
//       Glib::Regex::create ("(?<POINT>[LM](\\s+[\\-\\.\\d]+){2})");
//     Glib::RefPtr<Glib::Regex> transregex =
//       Glib::Regex::create ("transform=\"(?<TRANS>.*?)\"");
//     Glib::RefPtr<Glib::Regex> matrixregex =
//       Glib::Regex::create ("matrix\\((?<MATR>([\\-\\.\\d]*?(\\,|\\))){6})");

//     vector<string> strokes = REMatches(strokeregex, lines, "STROKE");
//     for (uint i = 0; i < strokes.size(); i++) {
//       cerr << i << ": "<<strokes[i] << endl;
//       vector<string> strwidth = REMatches(strwidthregex,strokes[i],"STRWIDTH");
//       for (uint j = 0; j < strwidth.size(); j++) {
// 	cerr << "STRW " << strwidth[j] << endl;
//       }
//       vector<string> points = REMatches(pointregex,strokes[i],"POINT");
//       for (uint j = 0; j < points.size(); j++) {
// 	cerr << j << ":: "<<points[j]<< endl;
//       }
//       vector<string> trans = REMatches(transregex,strokes[i],"TRANS");
//       for (uint j = 0; j < trans.size(); j++) {
// 	cerr << j << "trans: "<<trans[j]<< endl;
// 	vector<string> matrix = REMatches(matrixregex,trans[j],"MATR");
// 	for (uint k = 0; k < matrix.size(); k++)
// 	  cerr << k << " matr: "<<matrix[k]<< endl;
//       }
//     }
//     vector<string> polys = REMatches(polyregex, lines, "POLY");
//     for (uint i = 0; i < polys.size(); i++) {
//       vector<string> points = REMatches(pointregex, polys[i], "POINT");
//       Poly poly;
//       //cout << i << ": ";
//       //cout << polys[i] << endl;
//       for (uint j = 0; j < points.size(); j++) {
// 	//cout << j << " - " << points[j]  << endl ;
// 	istringstream is(points[j]);
// 	string type;
// 	is >> type;
// 	//cerr << type<< endl;
// 	if (type=="M" || type == "L") {
// 	  double x,y;
// 	  is >> x;
// 	  is >> y;
// 	  //cout << x << "," << y << endl;
// 	  poly.addVertex(x,y);
// 	  if (x<Min.x()) Min.x() = x;
// 	  if (y<Min.y()) Min.y() = y;
// 	  if (x>Max.x()) Max.x() = x;
// 	  if (y>Max.y()) Max.y() = y;
// 	}
//       }
//       if (poly.size()>0) {
// 	poly.setZ(0);
// 	//cerr << poly.info() << endl;
// 	polygons.push_back(poly);
//       }
//       cout << endl;
//     }
//   }
//   else cerr << _("Error: Unable to open SVG file - ") << filename << endl;
//   Center = (Min+Max)/2;
// }

bool FlatShape::getPolygonsAtZ(const Matrix4d &T, double z,
                   vector<Poly> &polys, double &max_grad,
                   vector<Poly> &supportpolys,
                   double max_supportangle,
                   double thickness) const
{
  max_grad = 0;
  polys = polygons;
  const Matrix4d trans = T * transform3D.transform;
  for (uint i = 0; i < polys.size(); i++) {
    polys[i].setZ(0);
    polys[i].transform(trans);
  }
  return true;
}

void FlatShape::clear()
{
  polygons.clear();
}

void FlatShape::draw_geometry(uint max_polygons) {
  const Matrix4d invT = transform3D.getInverse();
  const Vector3d minT = invT*Min;
  const Vector3d maxT = invT*Max;
  const Vector2d min2d(minT.x(), minT.y());
  const Vector2d max2d(maxT.x(), maxT.y());
  glDrawPolySurfaceRastered(polygons, min2d, max2d, 0, 0.1);
  uint step = 1;
  if (max_polygons > 0) step = polygons.size()/max_polygons;
  for (uint i = 0; i < polygons.size(); i+=step) {
    polygons[i].draw(GL_LINE_LOOP,false);
    // Poly p;
    // p.vertices = simplified(polygons[i].vertices, 0.2);
    // cleandist(p.vertices, 0.2);
    // p.draw_as_surface();

    //polygons[i].draw_as_surface();
  }
}

void FlatShape::CalcBBox()
{
  Min.set(INFTY,INFTY,0);
  Max.set(-INFTY,-INFTY,0);
  for(size_t i = 0; i < polygons.size(); i++)
    for(size_t j = 0; j < polygons[i].size(); j++){
      if ( polygons[i][j].x() < Min.x() ) Min.x() = polygons[i][j].x();
      if ( polygons[i][j].y() < Min.y() ) Min.y() = polygons[i][j].y();
      if ( polygons[i][j].x() > Max.x() ) Max.x() = polygons[i][j].x();
      if ( polygons[i][j].y() > Max.y() ) Max.y() = polygons[i][j].y();
    }
  Min = transform3D.transform*Min;
  Max = transform3D.transform*Max;
  Center = (Max + Min )/2;
}


void FlatShape::invertNormals()
{
  for (uint i = 0; i < polygons.size(); i++)
    polygons[i].reverse();
}

void FlatShape::mirror()
{
  for (uint i = 0; i < polygons.size(); i++)
    polygons[i].mirrorX(Center);
}

// Rotate and adjust for the user - not a pure rotation by any means
void FlatShape::Rotate(const Vector3d & axis, const double & angle)
{
  CalcBBox();
  if (axis.z()==0.) return; // try to only 2D-rotate
  Vector2d center(Center.x(),Center.y());
  for (size_t i=0; i<polygons.size(); i++)
    {
      polygons[i].rotate(center, angle);
    }
  PlaceOnPlatform();
}

void FlatShape::splitshapes(vector<Shape*> &shapes, ViewProgress *progress)
{
  uint count = polygons.size();
  if (progress) progress->start(_("Split Polygons"), count);
  int progress_steps = max(1,int(count/100.));

  for (uint i = 0; i < count; i++) {
    FlatShape *fs  = new FlatShape();
    fs->polygons.push_back(polygons[i]);
    if (progress && i%progress_steps==0 && !progress->update(count)) break;
    shapes.push_back(fs);
  }
  progress->stop("_(Done)");
}


string FlatShape::info() const
{
  ostringstream ostr;
  ostr <<"FlatShape with "<<polygons.size() << " polygons "
       << "min/max/center: "<<Min<<Max <<Center ;
  return ostr.str();
}

////////////////////////////// XML //////////////////////////////////////////

inline double ToDouble(QString s)
{
    std::istringstream i(s.toStdString());
    double x;
    if (!(i >> x))
        return -1;
    return x;
}

const QRegularExpression polyregex("(?ims)<path.*?stroke\\:none.*?\\sd\\=\"(?<POLY>.*?(Z|\"/\\>))");
const QRegularExpression strokeregex("(?ims)<path.*?(?<STROKE>fill\\:none.*?\\sd\\=\".*?\"/\\>)");
const QRegularExpression strwidthregex("stroke\\-width\\:(?<STRWIDTH>[\\-\\.\\d]+)");
const QRegularExpression pointregex("(?<POINT>[LM](\\s+[\\-\\.\\d]+){2})");
const QRegularExpression transregex("transform=\"(?<TRANS>.*?)\"");
const QRegularExpression matrixregex("matrix\\((?<MATR>([\\-\\.\\d]*?(\\,|\\))){6})");


Matrix3d svg_trans(const string &line)
{
  Matrix3d mat;
  QString val = REMatch(matrixregex, QString::fromStdString(line), "MATR");
  if (val != nullptr) {
      QStringList vals = val.split("[\\,\\)]");
      if (vals.size()>5) {
          mat.set_row(0,Vector3d(ToDouble(vals[0]),ToDouble(vals[2]),ToDouble(vals[4])));
          mat.set_row(1,Vector3d(ToDouble(vals[1]),ToDouble(vals[3]),ToDouble(vals[5])));
          mat.set_row(2,Vector3d(0,0,1));
    }
  }
  return mat;
}

QString get_attr(const QString &line, const QString &attrname)
{
    QStringList parts = line.split(";");
    for (QString p : parts) {
        QStringList lr = p.split(":");
        if (lr.size()==2){
            if (lr[0] == attrname) return lr[1];
        } else return "";
    }
    return "";
}

vector<Vector2d> ToVertices(const QString &line)
{
    QString points = REMatch(pointregex, line, "POINT");
    vector<Vector2d> v;
    for (QString p: points) {
        //cout << j << " - " << points[j]  << endl ;
        istringstream is(p.toStdString());
        string type;
        is >> type;
        //cerr << type<< endl;
        if (type=="M" || type == "L") {
            double x,y;
            if (is >> x && is >> y)
                //cout << x << "," << y << endl;
                v.push_back(Vector2d(x,y));
        }
    }
    return v;
}

int FlatShape::svg_addPolygon()
{

  vector<Poly> polys;
  if (svg_cur_style.find("stroke:none") != string::npos) { // polygon
    Poly poly;
    poly.vertices = ToVertices(QString::fromStdString(svg_cur_path));
    poly.setZ(0);
    poly.reverse();
    polys.push_back(poly);
  }
  else if (svg_cur_style.find("fill:none") != string::npos) { // stroke
    // cerr << "stroke " << svg_cur_path << endl;
    // cerr << "\t" << svg_cur_style << endl;
    QString wstr = get_attr(QString::fromStdString(svg_cur_style),"stroke-width");
    double width = ToDouble(wstr);
    // cerr << "\t width " << wstr << " = " << width << endl;
    vector<Vector2d> vertices = ToVertices(QString::fromStdString(svg_cur_path));
    polys = thick_lines(vertices, width);
    // cerr <<"thick "<< polys.size()<<" of " << vertices.size() << endl;
  }
  else if (svg_cur_style!="") {
    cerr << "unknown " << svg_cur_path << " in " << svg_cur_name << endl;
    cerr << "\t style: " << svg_cur_style << endl;
  }

  if (polys.size()>0) {
    if (svg_cur_trans!="") {
      // cerr << svg_cur_trans << endl;
      Matrix3d  mat = svg_trans(svg_cur_trans);
      // cerr << mat << endl;
      for (uint i=0; i < polys.size(); i++) {
    polys[i].setZ(0);
    polys[i].transform(mat);
      }
    }
    polygons.insert(polygons.begin(),polys.begin(),polys.end());
  }
  return polys.size();
}


void FlatShape::xml_handle_node(const xmlpp::Node* node)
{
  //std::cout << std::endl; //Separate nodes by an empty line.

  const xmlpp::ContentNode* nodeContent = dynamic_cast<const xmlpp::ContentNode*>(node);
  const xmlpp::TextNode*       nodeText = dynamic_cast<const xmlpp::TextNode*>(node);
  const xmlpp::CommentNode* nodeComment = dynamic_cast<const xmlpp::CommentNode*>(node);

  // if(nodeText && nodeText->is_white_space()) //Let's ignore the indenting - you don't always want to do this.
  //   return;

  const Glib::ustring nodename = node->get_name();

  if(!nodeText && !nodeComment && !nodename.empty()) //Let's not say "name: text".
  {
    const Glib::ustring namespace_prefix = node->get_namespace_prefix();
    if (svg_cur_name != "") svg_addPolygon();
    svg_cur_name = nodename;
    svg_cur_path = "";
    svg_cur_trans = "";
    svg_cur_style = "";
    // if(namespace_prefix.empty())
    //   std::cout << "Node name = " << nodename << std::endl;
    // else
    //   std::cout << "Node name = " << namespace_prefix << ":" << nodename << std::endl;
  }
  // else if(nodeText) //Let's say when it's text. - e.g. let's say what that white space is.
  // {
  //   std::cout << "Text Node " << nodename << nodeText->get_content()<< std::endl;
  // }

  //Treat the various node types differently:
  if(nodeText)
  {
      //std::cout << "text = \"" << nodeText->get_content() << "\"" << std::endl;
  }
  else if(nodeComment)
  {
      //std::cout << "comment = " << nodeComment->get_content() << std::endl;
  }
  else if(nodeContent)
  {
      //std::cout << "content = " << nodeContent->get_content() << std::endl;
  } else
  if (const xmlpp::Element* nodeElement = dynamic_cast<const xmlpp::Element*>(node)) {
    //A normal Element node:

    //line() works only for ElementNodes.
    // std::cout << "     line = " << node->get_line() << std::endl;

    //attributes:
    const xmlpp::Element::AttributeList& attributes = nodeElement->get_attributes();

    for(xmlpp::Element::AttributeList::const_iterator iter = attributes.begin();
    iter != attributes.end(); ++iter) {
      const xmlpp::Attribute* attribute = *iter;

      const Glib::ustring namespace_prefix = attribute->get_namespace_prefix();
      string attr = attribute->get_name();
      if (attr=="d")
    svg_cur_path  = attribute->get_value();
      else if (attr == "style")
    svg_cur_style = attribute->get_value();
      else if (attr == "transform")
    svg_cur_trans = attribute->get_value();
      else if (svg_cur_name == "svg" ){
    if (attr == "width" || attr == "height") {
      string val = attribute->get_value();
      if (val.find("pt") != string::npos)
        svg_prescale = 0.3527;
    }
      }
      else if (attr=="id") {
      }
      else
    cerr << "unknown Attribute in " << svg_cur_name << " : " << attr << " = " <<attribute->get_value() << endl;
      // if(namespace_prefix.empty())
      //   std::cout << "  Attribute " << attribute->get_name() << " = " <<  << std::endl;
      // else
      //   std::cout << "  Attribute " << namespace_prefix  << ":" << attribute->get_name() << " = " << attribute->get_value() << std::endl;
    }

    const xmlpp::Attribute* attribute = nodeElement->get_attribute("title");
    if(attribute) {
      std::cout << "title found: =" << attribute->get_value() << std::endl;
    }
  }

  if(!nodeContent)
  {
    //Recurse through child nodes:
    xmlpp::Node::NodeList list = node->get_children();
    for(xmlpp::Node::NodeList::iterator iter = list.begin(); iter != list.end(); ++iter)
      {
    xml_handle_node(*iter); //recursive
      }
    // get last bit (?)
    if (svg_cur_name!="") svg_addPolygon();
  }
}


int FlatShape::loadSVG(string filename)
{  // Set the global C++ locale to the user-configured locale,
  // so we can use std::cout with UTF-8, via Glib::ustring, without exceptions.

  #ifdef LIBXMLCPP_EXCEPTIONS_ENABLED
  try
  {
  #endif //LIBXMLCPP_EXCEPTIONS_ENABLED
    xmlpp::DomParser parser;
    //parser.set_validate();
    parser.set_substitute_entities(); //We just want the text to be resolved/unescaped automatically.
    parser.parse_file(filename);
    if(parser)
    {
      polygons.clear();
      svg_cur_name = "";
      svg_cur_path = "";
      svg_cur_trans = "";
      svg_cur_style = "";
      svg_prescale = 1.;
      //Walk the tree:
      const xmlpp::Node* pNode = parser.get_document()->get_root_node(); //deleted by DomParser.
      xml_handle_node(pNode);
    }

    if (svg_prescale!=1)
      for (uint i= 0; i<polygons.size(); i++)
    for (uint j= 0; j<polygons[i].size(); j++)
      polygons[i].vertices[j] *= svg_prescale;
    Clipping clipp;
    clipp.addPolys(polygons, subject);
    polygons = clipp.unite(CL::pftNonZero,CL::pftNegative);
    CalcBBox();
    Vector2d center2(Center.x(),Center.y());
    for (uint i= 0; i<polygons.size(); i++) {
      polygons[i].mirrorX(Center);
      polygons[i].rotate(center2, M_PI);
    }
    CalcBBox();

  #ifdef LIBXMLCPP_EXCEPTIONS_ENABLED
  }
  catch(const std::exception& ex)
  {
    std::cerr << "Exception caught: " << ex.what() << std::endl;
  }
  #endif //LIBXMLCPP_EXCEPTIONS_ENABLED

  return 0;
}


