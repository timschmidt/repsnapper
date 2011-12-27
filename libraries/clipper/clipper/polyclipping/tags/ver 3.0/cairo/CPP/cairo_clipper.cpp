/*******************************************************************************
*                                                                              *
* Author    :  Angus Johnson                                                   *
* Version   :  0.6                                                             *
* Date      :  6 December 2010                                                 *
* Copyright :  Angus Johnson                                                   *
*                                                                              *
* License:                                                                     *
* Use, modification & distribution is subject to Boost Software License Ver 1. *
* http://www.boost.org/LICENSE_1_0.txt                                         *
*                                                                              *
* Modified by Mike Owens to support coordinate transformation and tolerance    *
*******************************************************************************/

#include <cairo/cairo.h>
#include "clipper.hpp"
#include "cairo_clipper.hpp"

namespace clipper {
  namespace cairo {

    namespace {
      inline void transform_point(cairo_t* pen,
                                  Transform transform,
                                  double* x, double* y)
      {
        switch (transform) 
        {
          case tDeviceToUser:
            cairo_device_to_user(pen, x, y);
            break;
          case tUserToDevice:
            cairo_user_to_device(pen, x, y);
            break;
          default:
            ;
        }
      }
    }

    void cairo_to_clipper(cairo_t* cr,
                          TPolyPolygon &pg,
                          Transform transform,
                          double tolerance)
    {
      pg.clear();

      /* Avoid a push/pop if not required. */
      cairo_path_t *path = 0;
      if (tolerance != 0.0) {
        cairo_save(cr);
        cairo_set_tolerance(cr, tolerance);
        path = cairo_copy_path_flat(cr);
        cairo_restore(cr);
      } else {
        path = cairo_copy_path_flat(cr);
      }

      int poly_count = 0;
      for (int i = 0; i < path->num_data; i += path->data[i].header.length) {
        if( path->data[i].header.type == CAIRO_PATH_CLOSE_PATH) {
          poly_count++;
        }
      }

      pg.resize(poly_count);
      int i = 0, pc = 0;
      while (pc < poly_count)
      {
        int vert_count = 1;
        int j = i;
        while(j < path->num_data &&
          path->data[j].header.type != CAIRO_PATH_CLOSE_PATH)
        {
          if (path->data[j].header.type == CAIRO_PATH_LINE_TO)
            vert_count++;
          j += path->data[j].header.length;
        }
        pg[pc].resize(vert_count);
        if (path->data[i].header.type != CAIRO_PATH_MOVE_TO) {
          pg.resize(pc);
          break;
        }
        pg[pc][0].X = path->data[i+1].point.x;
        pg[pc][0].Y = path->data[i+1].point.y;
        transform_point(cr, transform, &pg[pc][0].X, &pg[pc][0].Y);

        i += path->data[i].header.length;

        j = 1;
        while (j < vert_count && i < path->num_data &&
          path->data[i].header.type == CAIRO_PATH_LINE_TO) {
          pg[pc][j].X = path->data[i+1].point.x;
          pg[pc][j].Y = path->data[i+1].point.y;
          transform_point(cr, transform, &pg[pc][j].X, &pg[pc][j].Y);
          j++;
          i += path->data[i].header.length;
        }
        pc++;
        i += path->data[i].header.length;
      }
      cairo_path_destroy(path);
    }
    //--------------------------------------------------------------------------

    void clipper_to_cairo(const TPolyPolygon &pg,
                          cairo_t* cr,
                          Transform transform)
    {
      for (size_t i = 0; i < pg.size(); ++i)
      {
        size_t sz = pg[i].size();
        if (sz < 3)
          continue;
        cairo_new_sub_path(cr);
        for (size_t j = 0; j < sz; ++j) {
          double x = pg[i][j].X,
                 y = pg[i][j].Y;
          transform_point(cr, transform, &x, &y);
          cairo_line_to(cr, x, y);
        }
        cairo_close_path(cr);
      }
    }
    //--------------------------------------------------------------------------

  }
}
