#!/bin/sh

if [ -z "$BUILD_FLAVOUR" ]; then exit 0; fi

export TARGET=target.$BUILD_FLAVOUR
export CHECKOUT=checkout.$BUILD_FLAVOUR

jhbuild --file=gtk+-win32.jhbuildrc "$@" || exit 1

if [ ! -d "$TARGET/lib/gdk-pixbuf-2.0/2.10.0/" ]; then
  mkdir -p "$TARGET/lib/gdk-pixbuf-2.0/2.10.0/"
fi

# FIXME: gdk-pixbuf-query-loaders.exe needs to be run on the target, by the installer or so
# if [ -f "$TARGET/bin/gdk-pixbuf-query-loaders.exe" ]; then
#   wine "$TARGET/bin/gdk-pixbuf-query-loaders.exe" > "$TARGET/lib/gdk-pixbuf-2.0/2.10.0/loaders.cache"
# fi

# Copy GLUT files into place
GLUTVER=3.7.6

cd "$CHECKOUT" && wget -c http://user.xmission.com/~nate/glut/glut-$GLUTVER-bin.zip -O glut-$GLUTVER-bin.zip &&
unzip -u glut-$GLUTVER-bin.zip && cd .. || exit 1

mkdir -p "$TARGET"/include/GL/
cp "$CHECKOUT"/glut-$GLUTVER-bin/glut.h "$TARGET"/include/GL/ || exit 1
cp "$CHECKOUT"/glut-$GLUTVER-bin/glut32.dll "$TARGET"/bin/ || exit 1
cp "$CHECKOUT"/glut-$GLUTVER-bin/glut32.lib "$TARGET"/lib/ || exit 1

# Build repsnapper
if [ ! -d "$CHECKOUT/repsnapper" ]; then
  mkdir "$CHECKOUT"/repsnapper
fi
cd "$CHECKOUT"/repsnapper
CPPFLAGS=-I../../"$TARGET"/include PKG_CONFIG_PATH=../../"$TARGET"/lib/pkgconfig ../../../configure --prefix="`pwd`/../../$TARGET" --host=i586-mingw32msvc && make -j4 && make install || exit 1
cd ../..

# Finally, copy appropriate libgcc into place
GCCPATH=`i586-mingw32msvc-gcc -print-libgcc-file-name`
GCCPATH=`dirname "$GCCPATH"`
for f in $GCCPATH/libgcc*dll; do
  cp "$f" "$TARGET/bin"
done

if [ "$BUILD_FLAVOUR" = "rls" ]; then
  echo "Building installer..."
  # run NSIS installer script for release build
  makensis -V2 -NOCD "$CHECKOUT"/repsnapper/win32/repsnapper.nsi || exit 2
fi

echo "All done..."
