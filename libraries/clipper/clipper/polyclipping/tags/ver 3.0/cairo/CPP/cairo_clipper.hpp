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

#ifndef CLIPPER_CAIRO_CLIPPER_HPP
#define CLIPPER_CAIRO_CLIPPER_HPP

#include "clipper.hpp"

typedef struct _cairo cairo_t;

namespace clipper {
  namespace cairo {

    enum Transform {
      tNone,
      tUserToDevice,
      tDeviceToUser
    };

    void cairo_to_clipper(cairo_t* cr,
                          clipper::TPolyPolygon &pg,
                          Transform transform = tNone,
                          double tolerance = 0.0);

    void clipper_to_cairo(const clipper::TPolyPolygon &pg,
                          cairo_t* cr,
                          Transform transform = tNone);
  }
}

#endif

