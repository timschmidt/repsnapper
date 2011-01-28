#!c:/perl/bin/perl.exe -w
##############################################################################
### =file
### win32-developer.pl
###
### =location
### http://svn.xxxx.org/svn/trunk/xxxxx/xxxx/xxxx/win32-developer.pl
###
### =description
### Tool for automating setup of Msys/Mingw builds on MS Windows XP (and compatible)
### based heavily on win32-packager from the MythTv project 
### ( by the same author)  
###
### =examples
### win32-developer.pl -h
###      => Print usage
### win32-developer.pl
###      => based on latest "tested" SVN trunk (ie a known-good win32 build)
### win32-developer.pl -r head
###      => based on trunk head
### win32-developer.pl -b
###      => based on release-021-fixes branch
### win32-developer.pl -b -t
###      => include some patches which are not accepted, but needed for Win32
### win32-developer.pl -b -t -k
###      =>Same but package and create setup.exe at the end
###
### =revision
### $Id$
###
### =author
### David Bussenschutt
##############################################################################

use strict;
use LWP::UserAgent;
use IO::File;
use Data::Dumper; 
use File::Copy qw(cp);
use Getopt::Std;
use Digest::MD5;


$SIG{INT} = sub { die "Interrupted\n"; };
$| = 1; # autoflush stdout;

# this script was last tested to work with this version, on other versions YMMV.
#my $SVNRELEASE = '23137'; # Recent trunk
#my $SVNRELEASE = 'HEAD'; # If you are game, go forth and test the latest!


# We allow SourceForge to tell us which server to download from,
# rather than assuming specific server/s
my $sourceforge = 'downloads.sourceforge.net';     # auto-redirect to a
                                                   # mirror of SF's choosing,
                                                   # hopefully close to you
# alternatively you can choose your own mirror:
#my $sourceforge = 'optusnet.dl.sourceforge.net';  # Australia
#my $sourceforge = 'internap.dl.sourceforge.net';  # USA,California
#my $sourceforge = 'easynews.dl.sourceforge.net';  # USA,Arizona,Phoenix,
#my $sourceforge = 'jaist.dl.sourceforge.net';     # Japan
#my $sourceforge = 'mesh.dl.sourceforge.net';      # Germany
#my $sourceforge = 'transact.dl.sourceforge.net';  # Germany

# Set this to the empty string for no-proxy:
my $proxy = '';
#my $proxy = 'http://enter.your.proxy.here:8080';
# Subversion proxy settings are configured in %APPDATA%\Subversion\servers

my $NOISY   = 1;            # Set to 0 for less output to the screen
my $package = 0;            # Create a Win32 Distribution package? 1 for yes
my $compile_type = "profile"; # compile options: debug, profile or release
my $tickets = 0;            # Apply specific win32 tickets -
                            #  usually those not merged into SVN
my $dbconf = 0;             # Configure MySQL as part of the build process.
                            #  Required only for testing
my $makeclean = 0;          # Flag to make clean
#my $svnlocation = "trunk";  # defaults to trunk unless -b specified
my $qtver = 4;              # default to 4 until we can test otherwise
my $continuous = 0 ;        # by default the app pauses to notify you what
                            #  it's about to do, -y overrides for batch usage.
                            
my $GitUrl = "https://github.com/timschmidt/repsnapper.git";

##############################################################################
# get command line options
my $opt_string = 'vhogkp:r:c:tldby';
my %opt = ();
getopts( "$opt_string", \%opt );
usage() if $opt{h};

$package    = 1       if defined $opt{k};
$NOISY      = 1       if defined $opt{v};
$tickets    = 1       if defined $opt{t};
$proxy      = $opt{p} if defined $opt{p};
#$SVNRELEASE = $opt{r} if defined $opt{r};
$dbconf     = 1       if defined $opt{d};
$makeclean  = 1       if defined $opt{l};
$continuous  = 1       if defined $opt{y};


if (defined $opt{c}) {
    $compile_type = $opt{c} if ($opt{c} eq "release") ;
    $compile_type = $opt{c} if ($opt{c} eq "profile") ;
}

#if (defined $opt{b}) {
  #  my @num = split /\./, $version;
    #$svnlocation = "branches/release-$num[0]-$num[1]-fixes";
    # 
    # Releases like 0.23.1 are actually tags, and use a different location: 
    #if ($version =~ m/-\d$/) { 
    #    $svnlocation = "tags/release-$num[0]-$num[1]-$num[2]"; 
    #}
#} else {
   # $svnlocation = "trunk";
#}

# Try to use parallel make
my $numCPU = $ENV{'NUMBER_OF_PROCESSORS'} or 1;
my $parallelMake = 'make';
if ( $numCPU gt 1 ) {
    # Pre-queue one extra job to keep the pipeline full:
    $parallelMake = 'make -j '. ($numCPU + 1);
}

# "Config:\n\tQT version: $qtver\n\tDLLs will be labeled as: $version\n";
if ( $numCPU gt 1 ) {
    print "\tBuilding with ", $numCPU, " processors\n";
}
print "\n\tWelcome to the Win32 MSYS/MINGW helper script for repsnapper!\n ".
			  "\tWe are about to try to install everything for you! \n".
			"\n\tHere are the relevant paths were thing will be installed:\n\n";
#print "\tComponents to build: ";
#foreach my $comp( @components ) { print "$comp " }
#print "\n\nPress [enter] to continue, or [ctrl]-c to exit now....\n";
#getc() unless $continuous;

# this will be used to test if we the same
# basic build type/layout as previously.
#my $is_same = "$compile_type-$svnlocation-$qtver.same";
#$is_same =~ s#/#-#g; # don't put dir slashes in a filename!


# TODO - we should try to autodetect these paths, rather than assuming
#        the defaults - perhaps from environment variables like this:
#  die "must have env variable SOURCES pointing to your sources folder"
#      unless $ENV{SOURCES};
#  my $sources = $ENV{SOURCES};
# TODO - although theoretically possible to change these paths,
#        it has NOT been tested much,
#        and will with HIGH PROBABILITY fail somewhere.
#      - Only $mingw is tested and most likely is safe to change.

# Perl compatible paths. DOS style, but forward slashes, and must end in slash:
# TIP: using paths with spaces in them is NOT supported, and will break.
#      patches welcome.
my $msys    = 'C:/MSys/1.0/';
my $sources = 'C:/MSys/1.0/sources/';
my $mingw   = 'C:/MinGW/';
my $approot  = 'C:/repsnapper/';       # this is where the entire SVN checkout lives
                                  # so c:/app/app/ is the main codebase.
my $build   = 'C:/repsnapper/repsnapper/src/'; # where 'make install' installs into

# Where is the users home?
#my $doshome = '';
#if ( ! exists $ENV{'HOMEPATH'} || $ENV{'HOMEPATH'} eq '\\' ) {
#  $doshome = $ENV{'USERPROFILE'};
#} else {
#  $doshome = $ENV{HOMEDRIVE}.$ENV{HOMEPATH};
#}
#my $home = $doshome;
#$home =~ s#\\#/#g;
#$home =~ s/ /\\ /g;
#$home .= '/'; # all paths should end in a slash

# Where are program files (32-bit)?
my $dosprogramfiles = '';
if ( $ENV{'ProgramFiles(x86)'} ) {
  $dosprogramfiles = $ENV{'ProgramFiles(x86)'};
} else {
  $dosprogramfiles = $ENV{'ProgramFiles'};
}
my $programfiles = $dosprogramfiles;
$programfiles =~ s#\\#/#g;


# DOS executable CMD.exe versions of the paths (for when we shell to DOS mode):
my $dosmsys    = perl2dos($msys);
my $dossources = perl2dos($sources);
my $dosmingw   = perl2dos($mingw);
my $dosapp  = perl2dos($approot);

# Unix/MSys equiv. versions of the paths (for when we shell to MSYS/UNIX mode):
my $unixmsys  = '/';       # MSys root is always mounted here,
                           # irrespective of where DOS says it really is.
my $unixmingw = '/mingw/'; # MinGW is always mounted here under unix,
                           # if you setup mingw right in msys,
                           # so we will usually just say /mingw in the code,
                           # not '.$unixmingw.' or similar (see /etc/fstab)
my $unixsources      = perl2unix($sources);
my $unixapp       = perl2unix($approot);
#my $unixhome         = perl2unix($home);
my $unixprogramfiles = perl2unix($programfiles);
my $unixbuild        = perl2unix($build);

# The installer for MinGW:
my $MinGWinstaller = 'MinGW-5.1.6.exe';
my $installMinGW   = $dossources.$MinGWinstaller;

# Qt4 directory
#my $qt4dir     = 'C:/qt/4.5.3/';
#my $dosqt4dir  = perl2dos($qt4dir);
#my $unixqt4dir = perl2unix($qt4dir);

#NOTE: IT'S IMPORTANT that the PATHS use the correct SLASH-ing method for
#the type of action:
#      for [exec] actions, use standard DOS paths, with single BACK-SLASHES
#      '\' (unless in double quotes, then double the backslashes)
#      for [shell] actions, use standard UNIX paths, with single
#      FORWARD-SLASHES '/'
#
#NOTE: when referring to variables in paths, try to keep them out of double
#quotes, or the slashing can get confused:
#      [exec]   actions should always refer to  $dosXXX path variables
#      [shell]  actions should always refer to $unixXXX path  variables
#      [dir],[file],[mkdirs],[archive] actions should always refer to
#      default perl compatible paths
# NOTE:  The architecture of this script is based on cause-and-event.
#        There are a number of "causes" (or expectations) that can trigger
#        an event/action.
#        There are a number of different actions that can be taken.
#
# eg: [ dir  => "c:/MinGW", exec => $dossources.'MinGW-5.1.4.exe' ],
#
# means: expect there to be a dir called "c:/MinGW", and if there isn't
# execute the file MinGW-5.1.4.exe. (clearly there needs to be a file
# MinGW-5.1.4.exe on disk for that to work, so there is an earlier
# declaration to 'fetch' it)


#build expectations (causes) :
#  missing a file (given an expected path)                           [file]
#  missing folder                                                    [dir]
#  missing source archive (version of 'file' to fetch from the web)  [archive]
#  apply a perl pattern match and if it DOESNT match execute action  [grep]
#  - this 'cause' actually needs two parameters in an array
#    [ pattern, file].  If the file is absent, the pattern
#    is assumed to not match (and emits a warning).
#  test the file/s are totally the same (by size and mtime)          [filesame]
#  - if first file is non-existant then that's permitted,
#    it causes the action to trigger.
#  test the first file is newer(mtime) than the second one           [newer]
#  - if given 2 existing files, not necessarily same size/content,
#    and the first one isn't newer, execute the action!
#    If the first file is ABSENT, run the action too.
#  execute the action only if a file or directory exists             [exists]
#  stop the run, useful for script debugging                         [stop]
#  pause the run, await a enter                                      [pause]
#  always execute the action  (try to minimise the use of this!)     [always]

#build actions (events) are:
#  fetch a file from the web (to a location)                         [fetch]
#  execute a DOS/Win32 exe/command and wait to complete              [exec]
#  execute a MSYS/Unix script/command in bash and wait to complete   [shell]
#  - this 'effect' actually accepts many parameters in an array
#    [ cmd1, cmd2, etc ]
#  extract a .tar .tar.gz or .tar.bz2 or ,zip file ( to a location)  [extract]
#  - (note that .gz and .bz2 are thought equivalent)
#  write a small patch/config/script file directly to disk           [write]
#  make directory tree upto the path specified                       [mkdirs]
#  copy a new version of a file, set mtime to the original           [copy]

#TODO:
#  copy a set of files (path/filespec,  destination)               not-yet-impl
#   =>  use exec => 'copy /Y xxx.* yyy'
#  apply a diff                                                    not-yet-impl
#   =>   use shell => 'patch -p0 < blah.patch'
#  search-replace text in a file                                   not-yet-impl
#   =>   use grep => ['pattern',subject],
#                     exec => shell 'patch < etc to replace it'


# NOTES on specific actions:
# 'extract' now requires all paths to be perl compatible (like all other
# commands) If not supplied, it extracts into the folder the .tar.gz is in.
# 'exec' actually runs all your commands inside a single cmd.exe
# command-line. To link commands use '&&'
# 'shell' actually runs all your commands inside a bash shell with -c "(
# cmd;cmd;cmd )" so be careful about quoting.


#------------------------------------------------------------------------------
#------------------------------------------------------------------------------
# DEFINE OUR EXPECTATIONS and the related ACTIONS:
#  - THIS IS THE GUTS OF THE APPLICATION!
#  - A SET OF DECLARATIONS THAT SHOULD RESULT IN A WORKING WIN32 INSTALATION
#------------------------------------------------------------------------------
#------------------------------------------------------------------------------

my $expect;

push @{$expect},

[ dir => [$sources] ,
  mkdirs  => [$sources],
  comment => 'We will download all the files from the web, and save them here:'],

[ pause => '.... press [enter] to continue !'],

[ archive => $sources.$MinGWinstaller,
  'fetch' => 'http://'.$sourceforge.'/sourceforge/mingw/'.$MinGWinstaller,
  comment => 'Get mingw and addons first, or we cant do [shell] requests!' ],
[ archive => $sources.'mingw-utils-0.3.tar.gz',
  'fetch' => 'http://'.$sourceforge.
              '/sourceforge/mingw/mingw-utils-0.3.tar.gz' ],
# ffmpeg complains if gcc < 4.2, so download a newer version
# As of 20090626 MinGW gcc >= 4.3 do NOT work with ffmpeg (MinGW bug #2812588)
 [ archive => $sources.'gcc-4.2.4-tdm-1-core.tar.gz',
   'fetch' => 'http://'.$sourceforge.
              '/sourceforge/tdm-gcc/gcc-4.2.4-tdm-1-core.tar.gz' ],
 [ archive => $sources.'gcc-4.2.4-tdm-1-g++.tar.gz',
   'fetch' => 'http://'.$sourceforge.
              '/sourceforge/tdm-gcc/gcc-4.2.4-tdm-1-g++.tar.gz' ],


[ dir     => $mingw,
  exec    => $installMinGW,
  comment => 'install MinGW (be sure to install g++, g77 and ming '.
             'make too. Leave the default directory at c:\mingw ) '.
             '- it will require user interaction, '.
             'but once completed, will return control to us....' ],

# interactive, supposed to install g++ and ming make too,
# but people forget to select them?

[ file    => $mingw."bin/gcc.exe",
  exec    => $installMinGW,
  comment => 'unable to find a gcc.exe where expected, '.
             'rerunning MinGW installer!' ],

[ archive => $sources.'MSYS-1.0.11.exe',
  'fetch' => 'http://'.$sourceforge.'/sourceforge/mingw/MSYS-1.0.11.exe',
  comment => 'Get the MSYS and addons:' ] ,
[ archive => $sources.'bash-3.1-MSYS-1.0.11-1.tar.bz2',
  'fetch' => 'http://'.$sourceforge.
             '/sourceforge/mingw/bash-3.1-MSYS-1.0.11-1.tar.bz2' ] ,
[ archive => $sources.'zlib-1.2.3-MSYS-1.0.11.tar.bz2',
  'fetch' => 'http://easynews.dl.sourceforge.net'.
             '/sourceforge/mingw/zlib-1.2.3-MSYS-1.0.11.tar.bz2' ] ,
[ archive => $sources.'coreutils-5.97-MSYS-1.0.11-snapshot.tar.bz2',
  'fetch' => 'http://'.$sourceforge.'/sourceforge/mingw'.
             '/coreutils-5.97-MSYS-1.0.11-snapshot.tar.bz2' ] ,
[ archive => $sources.'mktemp-1.5-MSYS.tar.bz2',
  'fetch' => 'http://'.$sourceforge.'/sourceforge/mingw'.
             '/mktemp-1.5-MSYS.tar.bz2' ] ,

# install MSYS, it supplies the 'tar' executable, among others:
[ file    => $msys.'bin/tar.exe',
  exec    => $dossources.'MSYS-1.0.11.exe',
  comment => 'Install MSYS, it supplies the tar executable, among others. You '.
             'should follow prompts, AND do post-install in DOS box.' ] ,

#  don't use the [shell] or [copy] actions here,
# as neither are avail til bash is installed!
[ file    => $msys.'bin/sh2.exe',
  exec    => 'copy /Y '.$dosmsys.'bin\sh.exe '.$dosmsys.'bin\sh2.exe',
  comment => 'make a copy of the sh.exe so that we can '.
             'utilise it when we extract later stuff' ],

# prior to this point you can't use the 'extract' 'copy' or 'shell' features!

# if you did a default-install of MingW, then you need to try again, as we
# really need g++ and mingw32-make, and g77 is needed for fftw
[ file    => $mingw.'bin/mingw32-make.exe',
  exec    => $installMinGW,
  comment => 'Seriously?  You must have done a default install of MinGW.  '.
             'go try again! You MUST choose the custom installed and select '.
             'the mingw make, g++ and g77 optional packages.' ],
[ file    => $mingw.'bin/g++.exe',
  exec    => $installMinGW,
  comment => 'Seriously?  You must have done a default install of MinGW.  '.
             'go try again! You MUST choose the custom installed and select '.
             'the mingw make, g++ and g77 optional packages.' ],
[ file    => $mingw.'bin/g77.exe',
  exec    => $installMinGW,
   comment => 'Seriously?  You must have done a default install of MinGW.  '.
              'go try again! You MUST choose the custom installed and select '.
              'the mingw make, g++ and g77 optional packages.' ],

#[ file => 'C:/MinGW/bin/mingw32-make.exe',  extract =>
#$sources.'mingw32-make-3.81-2.tar',"C:/MinGW" ], - optionally we could get
#mingw32-make from here

# now that we have the 'extract' feature, we can finish ...
[ file    => $mingw.'/bin/reimp.exe',
  extract => [$sources.'mingw-utils-0.3.tar', $mingw],
  comment => 'Now we can finish all the mingw and msys addons:' ],
[ file    => $msys.'bin/bash.exe',
  extract => [$sources.'bash-3.1-MSYS-1.0.11-1.tar', $msys] ],
[ file    => $mingw.'bin/mingw32-gcc-4.2.4.exe',
  extract => [$sources.'gcc-4.2.4-tdm-1-core.tar', $mingw] ],
[ file    => $mingw.'include/c++/4.2.4/cstdlib',
  extract => [$sources.'gcc-4.2.4-tdm-1-g++.tar', $mingw] ],
[ dir     => $sources.'coreutils-5.97',
  extract => [$sources.'coreutils-5.97-MSYS-1.0.11-snapshot.tar'] ],
[ file    => $msys.'bin/pr.exe',
  shell   => ["cd ".$unixsources."coreutils-5.97","cp -r * / "] ],
[ file    => $msys.'bin/mktemp.exe',
  extract => [$sources.'mktemp-1.5-MSYS.tar', $msys] ],

[ dir     => $msys."lib" ,  mkdirs => $msys.'lib' ],
[ dir     => $msys."include" ,  mkdirs => $msys.'include' ],

#get gdb
[ archive => $sources.'gdb-6.8-mingw-3.tar.bz2',
  'fetch' => 'http://'.$sourceforge.
             '/sourceforge/mingw/gdb-6.8-mingw-3.tar.bz2',
  comment => 'Get gdb for possible debugging later' ],
[ file    => $msys.'bin/gdb.exe',
  extract => [$sources.'gdb-6.8-mingw-3.tar.bz2', $msys] ],


# (alternate would be from the gnuwin32 project,
#  which is actually from same source)
#  run it into a 'unzip' folder, because it doesn't extract to a folder:
[ dir     => $sources."unzip",
  mkdirs  => $sources.'unzip',
  comment => 'unzip.exe - Get a precompiled '.
             'native Win32 version from InfoZip' ],
[ archive => $sources.'unzip/unz552xN.exe',
  'fetch' => 'ftp://ftp.info-zip.org/pub/infozip/win32/unz552xn.exe'],
[ file    => $sources.'unzip/unzip.exe',
  exec    => 'chdir '.$dossources.'unzip && '.
             $dossources.'unzip/unz552xN.exe' ],
# we could probably put the unzip.exe into the path...

# get GIT from the msys git project
[ archive => $sources.'Git-1.7.3.1-preview20101002.exe',
  'fetch' => 'http://msysgit.googlecode.com/files/Git-1.7.3.1-preview20101002.exe',
  comment => 'Get GIT for use later.' ],

# install GIT exe, to C:\Program Files\Git :
[ file    => 'C:/Progra~1/Git/bin/git.exe',
  exec    => $dossources.'Git-1.7.3.1-preview20101002.exe',
  comment => 'Install GIT. You should follow the DEFAULT prompts ... dont change them! ...' ] ,

#[ pause => '.... press [enter] to continue !'],

# :
[ dir     => $sources."zlib",
  mkdirs  => $sources.'zlib',
  comment => 'the zlib download is a bit messed-up, and needs some TLC '.
             'to put everything in the right place' ],
[ dir     => $sources."zlib/usr",
  extract => [$sources.'zlib-1.2.3-MSYS-1.0.11.tar', $sources."zlib"] ],
# install to /usr:
[ file    => $msys.'lib/libz.a',
  exec    => ["copy /Y ".$dossources.'zlib\usr\lib\* '.$dosmsys."lib"] ],
[ file    => $msys.'bin/msys-z.dll',
  exec    => ["copy /Y ".$dossources.'zlib\usr\bin\* '.$dosmsys."bin"] ],
[ file    => $msys.'include/zlib.h',
  exec    => ["copy /Y ".$dossources.'zlib\usr\include\* '.
              $dosmsys."include"] ],
              
# make sure that /mingw is mounted in MSYS properly before trying
# to use the /mingw folder.  this is supposed to happen as part
# of the MSYS post-install, but doesnt always work.
[ file    => $msys.'etc/fstab',
  write   => [$msys.'etc/fstab',
"$mingw /mingw
" ],
  comment => 'correct a MinGW bug that prevents the /etc/fstab from existing'],



# apply sspi.h patch
[ file    => $mingw.'include/sspi_h.patch',
  write   => [$mingw.'include/sspi_h.patch',
'*** sspi.h      Sun Jan 25 17:55:57 2009
--- sspi.h.new  Sun Jan 25 17:55:51 2009
***************
*** 8,13 ****
--- 8,15 ----
  extern "C" {
  #endif
  
+ #include <subauth.h>
+ 
  #define SECPKG_CRED_INBOUND 1
  #define SECPKG_CRED_OUTBOUND 2
  #define SECPKG_CRED_BOTH (SECPKG_CRED_OUTBOUND|SECPKG_CRED_INBOUND)

' ],comment => 'write the patch for the the sspi.h file'],
# apply it!?
[ grep    => ['subauth.h',$mingw.'include/sspi.h'], 
  shell   => ["cd /mingw/include","patch -p0 < sspi_h.patch"],
  comment => 'Apply sspi.h patch file, if not already applied....' ],

#[ pause => 'check  patch.... press [enter] to continue !'],

# apply sched.h patch
[ always    => $mingw.'include/sched_h.patch', 
  write   => [$mingw.'include/sched_h.patch',
"--- include/sched.h.org	Thu Dec  4 12:00:16 2008
+++ include/sched.h	Wed Dec  3 13:42:54 2008
@@ -124,8 +124,17 @@
 typedef int pid_t;
 #endif
 
-/* Thread scheduling policies */
+/* pid_t again! */
+#if defined(__MINGW32__) || defined(_UWIN)
+/* Define to `int' if <sys/types.h> does not define. */
+/* GCC 4.x reportedly defines pid_t. */
+#ifndef _PID_T_
+#define _PID_T_
+#define pid_t int
+#endif
+#endif
 
+/* Thread scheduling policies */
 enum {
   SCHED_OTHER = 0,
   SCHED_FIFO,
" ],comment => 'write the patch for the the sched.h file'],
# apply it!?
[ grep    => ['GCC 4.x reportedly defines pid_t',$mingw.'include/sched.h'], 
  shell   => ["cd /mingw/include","patch -p1 < sched_h.patch"],
  comment => 'Apply sched.h patch file, if not already applied....' ],

#[ pause => 'check  patch.... press [enter] to continue !'],
;

# if packaging is selected, get innosetup
if ($package == 1) {
  push @{$expect},
 [ archive => $sources.'isetup-5.2.3.exe',
     fetch => 'http://files.jrsoftware.org/ispack/ispack-5.2.3.exe',
   comment => 'fetch inno setup setup' ],
 [ file    => $programfiles.'/Inno Setup 5/iscc.exe',
   exec    => $dossources.'isetup-5.2.3.exe', #/silent is broken! 
   comment => 'Install innosetup - install ISTool, '.
              'ISSP, AND encryption support.' ],
 # Get advanced uninstall
 [ archive => $sources.'UninsHs.rar',
     fetch => 'http://www.uninshs.com/down/UninsHs.rar',
   comment => 'fetch uninstall for innosetup' ],
 [ archive => $sources.'unrar-3.4.3-bin.zip',
     fetch => 'http://downloads.sourceforge.net/gnuwin32/unrar-3.4.3-bin.zip',
   comment => 'fetching unrar'],
 [ file    => $sources.'bin/unrar.exe',
  shell    => ["cd ".$sources, "unzip/unzip.exe unrar-3.4.3-bin.zip"]],
 [ file    => $programfiles.'/Inno Setup 5/UninsHs.exe',
  shell    => ['cd "'.$unixprogramfiles.'/inno setup 5"',
              $sources.'bin/unrar.exe e '.$sources.'UninsHs.rar'],
  comment  => 'Install innosetup' ],
 [ archive => $sources.'istool-5.2.1.exe',
     fetch => 'http://downloads.sourceforge.net/sourceforge'.
              '/istool/istool-5.2.1.exe',
   comment => 'fetching istool' ],
 [ file    => $programfiles.'/ISTool/isxdl.dll',
   exec    => $dossources.'istool-5.2.1.exe /silent',
   comment => 'Install istool'],
 [ archive => $sources.'logo_mysql_sun.gif',
     fetch => 'http://www.mysql.com/common/logos/logo_mysql_sun.gif',
   comment => 'Download MySQL logo for an install page in the package' ],
 [ exists  => $approot.'build/package_flag',
    shell  => ["rm ".$unixapp."build/package_flag"],
   comment => '' ],
;
}



#----------------------------------------
# now we do each of the source library dependencies in turn:
# download,extract,build/install
# TODO - ( and just pray that they all work?)  These should really be more
# detailed, and actually check that we got it installed properly.
#
# Most of these look for a Makefile as a sign that the ./configure was
# successful (not necessarily true, but it's a start) but this requires that
# the .tar.gz didn't come with a Makefile in it.

push @{$expect},
#[ archive => $sources.'freetype-2.3.5.tar.gz',  
#  fetch   => 'http://download.savannah.gnu.org'.
#             '/releases-noredirect/freetype/freetype-2.3.5.tar.gz'],
#[ dir     => $sources.'freetype-2.3.5', 
#  extract => $sources.'freetype-2.3.5.tar' ],
## caution... freetype comes with a Makefile in the .tar.gz, so work around it!
#[ file    => $sources.'freetype-2.3.5/Makefile__', 
#  shell   => ["cd $unixsources/freetype-2.3.5",
#              "./configure --prefix=/usr",
#              "touch $unixsources/freetype-2.3.5/Makefile__"],
#  comment => 'building freetype' ],
#              
## here's an example of specifying the make and make install steps separately, 
## for apps that can't be relied on to have the make step work!
#[ file    => $sources.'freetype-2.3.5/objs/.libs/libfreetype.a', 
#  shell   => ["cd $unixsources/freetype-2.3.5",
#              "make"],
#  comment => 'checking freetype' ],
#[ file    => $msys.'lib/libfreetype.a', 
#  shell   => ["cd $unixsources/freetype-2.3.5",
#              "make install"],
#  comment => 'installing freetype' ],
#[ file    => $msys.'bin/libfreetype-6.dll', 
#  shell   => ["cp $unixsources/freetype-2.3.5/objs/.libs/libfreetype-6.dll ".
#              "$msys/bin/"] ],
#
##here's a classic "configure; make; make-install" script, but specifying the PREFIX.
#[ archive => $sources.'lame-398-2.tar.gz',  
#  fetch   => 'http://'.$sourceforge.'/sourceforge/lame/lame-398-2.tar.gz'],
#[ dir     => $sources.'lame-398-2', 
#  extract => $sources.'lame-398-2.tar' ],
#[ file    => $msys.'lib/libmp3lame.a', 
#  shell   => ["cd $unixsources/lame-398-2",
#              "./configure --prefix=/usr",
#              "make",
#              "make install"],
#  comment => 'building and installing: msys lame' ],
#

              
# get fluid binary: from http://www.muquit.com/muquit/software/fluid_hack/fluid.exe              
#[ archive => $sources.'fluid.exe',
#  'fetch' => 'http://www.muquit.com/muquit/software/fluid_hack/fluid.exe',
#  comment => 'Get FLUID for use later.' ],
#[ file    => $msys.'bin/fluid.exe',
#  exec    => 'copy /Y '.$dossources.'fluid.exe '.$dosmsys.'bin\fluid.exe'],
  
  
# get fltk sources to compile against:
# http://ftp.easysw.com/pub/fltk/1.1.10/fltk-1.1.10-source.tar.gz
# install into /mingw as installing to /usr seems to not quite work right? 
[ archive => $sources.'fltk-1.1.10-source.tar.gz',
  'fetch' => 'http://ftp.easysw.com/pub/fltk/1.1.10/fltk-1.1.10-source.tar.gz',
  comment => 'Get FLTK 1.1.10 for use later.' ],
[ dir     => $sources.'fltk-1.1.10', 
  extract => $sources.'fltk-1.1.10-source.tar' ],
[ file    => $mingw.'lib/libfltk.a', 
  shell   => ["cd $unixsources/fltk-1.1.10",
              "./configure --prefix=/mingw",
              "make",
              "make install"],
  comment => 'building and installing: msys fltk-1.1.10-source' ],  
  
# get vmmlib:


[ pause => 'check  patch.... press [enter] to continue !'],
;

#----------------------------------------
# get app sources, if we don't already have them
# download all the files from the web, and save them here:
#----------------------------------------
push @{$expect},
[ dir     => $approot,
  mkdirs  => $approot,
  comment => 'make app dir'],

# if no files , pull from main GIT repo ( no support for alternative repos presently
[ file    => $build.'Makefile', 
  shell   => ['cd '.$approot,
              'C:/Progra~1/Git/bin/git.exe clone '.$GitUrl],
   comment => 'CLONE git repo!'],
   
# NOTE: if there are already files in the folder, we assume you just want to build them as-is
# and we don't muck with them.! 
              
#[ pause => '.... press [enter] to continue !'],


#----------------------------------------
# now we actually build app! 
#----------------------------------------

# 
[ file   => $build.'repsnapper',  # is the binary present?! 
  shell  => ['cd '.$unixbuild,
            'make'],
  comment => 'Build the app! ' ],
;
# make

# check that the date stamp on your target dll's is newenough, or delete it and try again. 
# repeat for each dll ( not shown here )
#[ newer => [$approot."app/libs/libmyth/libmyth-$version.dll",
#            $approot.'app/last_build.txt'],
#  shell => ['rm '.$unixapp."app/libs/libmyth/libmyth-$version.dll",
#            'source '.$unixapp.'qt'.$qtver.'_env.sh',
#            'cd '.$unixapp.'app', $parallelMake],
#  comment => 'libs/libmyth/libmyth-$version.dll - '.
#             'redo make unless all these files exist, '.
#             'and are newer than the last_build.txt identifier' ],



# Archive old build before we create a new one with make install:
#[ exists  => $approot.'build_old',
#  shell   => ['rm -fr '.$unixapp.'build_old'],
#  comment => 'Deleting old build backup'],
#[ exists  => $build,
#  shell   => ['mv '.$unixbuild.' '.$unixapp.'build_old'],
#  comment => 'Renaming build to build_old for backup....'],

# re-install to /c/app/build if we have a newer app build
# ready:
#[ newer   => [$build.'bin/mythfrontend.exe',
#              $approot.'app/programs/mythfrontend/mythfrontend.exe'],
#  shell   => ['source '.$unixapp.'qt'.$qtver.'_env.sh',
#              'cd '.$unixapp.'app',
#              'make install'],
#  comment => 'was the last configure successful? then install app ' ],




# -------------------------------
# Prepare Readme.txt for distribution file - temporary for now
# -------------------------------


push @{$expect},
[ file => $approot.'repsnapper/readme.win32.txt',
 write => [$approot.'repsnapper/readme.win32.txt',
'PLACEHOLDER README for Win32 Installation of XXXX version
=============================================================
The current installation very basic:
 - All exe and dlls will be installed to %PROGRAMFILES% - TODO
','nocheck'],comment => ''],

[ file => $approot.'repsnapper/readme.win32.txt', 
  shell => ['unix2dos '.$unixapp.'repsnapper/readme.win32.txt', 'nocheck'],
  comment => 'Create a WIN32 readme.txt file, if there isnt one' ],
;

#if ($package == 1) {
#    push @{$expect},
#    # Create directories
#    [ dir     => [$approot.'setup'] , #
#      mkdirs => [$approot.'setup'],
#      comment => 'Create Packaging directory'],
#    [ dir     => [$approot.'build/isfiles'] , 
#      mkdirs => [$approot.'build/isfiles'],
#      comment => 'Create Packaging directory'],
#    # Move required files from inno setup to setup directory
#    [ file    => $approot."build/isfiles/UninsHs.exe",
#      exec    => 'copy /Y "'.$dosprogramfiles.'\Inno Setup 5\UninsHs.exe" '.#
#                 $dosapp.'build\isfiles\UninsHs.exe',
#      comment => 'Copy UninsHs to setup directory' ],
#    [ file    => $approot."build/isfiles/isxdl.dll",
#      exec    => 'copy /Y "'.$dosprogramfiles.'\ISTool\isxdl.dll" '.
#                 $dosapp.'build\isfiles\isxdl.dll',
#      comment => 'Copy isxdl.dll to setup directory' ],
#    [ file    => $approot."build/isfiles/WizModernSmallImage-IS.bmp",
#      exec    => 'copy /Y "'.$dosprogramfiles.'\Inno Setup 5'.
#                 '\WizModernSmallImage-IS.bmp" '.
#                 $dosapp.'build\isfiles\WizModernSmallImage-IS.bmp',
#      comment => 'Copy WizModernSmallImage-IS.bmp to setup directory' ],
#    # Copy required files from sources or packaging to setup directory:
#    [ filesame => [$approot.'build/isfiles/appsetup.iss',
#                   $approot.'packaging/win32/build/appsetup.iss'],
#      copy     => [''=>'', 
#      comment  => 'appsetup.iss'] ],
#    [ filesame => [$approot.'build/isfiles/mysql.gif',
#                   $sources.'logo_mysql_sun.gif'],
#      copy     => [''=>'', 
#      comment  => 'mysql.gif'] ],
#    # Create on-the-fly  files required
#    [ file     => $approot.'build/isfiles/configuremysql.vbs',
#      write    => [$approot.'build/isfiles/configuremysql.vbs',
#'WScript.Echo "Currently Unimplemented"
#' ], 
#      comment   => 'Write a VB script to configure MySQL' ],
##    [ always   => [],#
##      write    => [$approot.'build/isfiles/versioninfo.iss', '
##define MyAppName      "MythTv"
##define MyAppVerName   "MythTv '.$version.'(svn_'.$SVNRELEASE .')"
##define MyAppPublisher "Mythtv"
##define MyAppURL       "http://www.app.org"
##define MyAppExeName   "Win32MythTvInstall.exe"
##' ], 
#      comment  => 'write the version information for the setup'],
#    [ file     => $approot.'genfiles.sh', 
#      write    => [$approot.'genfiles.sh','
#cd '.$unixapp.'build
#find . -type f -printf "Source: '.$approot.'build/%h/%f; Destdir: {app}/%h\n" | sed "s/\.\///" | grep -v ".svn" | grep -#v "isfiles" | grep -v "include" > '.$unixapp.'/build/isfiles/files.iss
#',], 
#      comment  => 'write script to generate setup files'], 
#    [ newer    => [$approot.'build/isfiles/files.iss',
#                   $approot.'app/last_build.txt'],
#      shell    => [$unixapp.'genfiles.sh'] ],
## Run setup
##    [ newer   => [$approot.'setup/MythTvSetup.exe',
##                   $approot.'app/last_build.txt'],
##      exec    => ['"'.$dosprogramfiles.'\Inno Setup 5\Compil32.exe" /cc "'.
##                  $dosapp.'build\isfiles\appsetup.iss"' ]],
#    [ newer    => [$approot.'setup/MythTvSetup.exe',
#                    $approot.'app/last_build.txt'],
#      exec     => ['cd '.$dosapp.'build\isfiles && '.
#                   '"'.$dosprogramfiles.'\Inno Setup 5\iscc.exe" "'.
#                   $dosapp.'build\isfiles\appsetup.iss"' ]],
#
    #;
#}


#------------------------------------------------------------------------------

; # END OF CAUSE->ACTION DEFINITIONS

#------------------------------------------------------------------------------

sub _end {
  #  comment("This version of the Win32 Build script ".
  #          "last was last tested on: $SVNRELEASE");

    print << 'END';    
#
# SCRIPT TODO/NOTES:  - further notes on this scripts direction....
END
}

#------------------------------------------------------------------------------

# this is the mainloop that iterates over the above definitions and
# determines what to do:
# cause:
foreach my $dep ( @{$expect} ) { 
    my @dep = @{$dep};

    #print Dumper(\@dep);

    my $causetype = $dep[0];
    my $cause =  $dep[1];
    my $effecttype = $dep[2];
    my $effectparams = $dep[3] || '';
    die "too many parameters in cause->event declaration (".join('|',@dep).")"
        if defined $dep[4] && $dep[4] ne 'comment'; 
    # four pieces: cause => [blah] , effect => [blah]

    my $comment = $dep[5] || '';

    if ( $comment && $NOISY ) {
        comment($comment);
    }

    my @cause;
    if (ref($cause) eq "ARRAY" ) {
        @cause = @{$cause};
    } else { 
        push @cause, $cause ;
    }

    # six pieces: cause => [blah] , effect => [blah] , comment => ''
    die "too many parameters in cause->event declaration (@dep)"
        if defined $dep[6];

    my @effectparams = ();
    if (ref($effectparams) eq "ARRAY" ) {
        @effectparams = @{$effectparams};
    } else { 
        push @effectparams, $effectparams ;
    }
    # if a 'nocheck' parameter is passed through, dont pass it through to
    # the 'effect()', use it to NOT check if the file/dir exists at the end.
    my @nocheckeffectparams = grep { ! /nocheck/i } @effectparams; 
    my $nocheck = 0;
    if ( $#nocheckeffectparams != $#effectparams ) { $nocheck = 1; } 

    if ( $causetype eq 'archive' ) {
        die "archive only supports type fetch ($cause)($effecttype)"
            unless $effecttype eq 'fetch';
        if ( -f $cause[0] ) {print "file exists: $cause[0]\n"; next;}
        # 2nd and 3rd params get squashed into
        # a single array on passing to effect();
        effect($effecttype,$cause[0],@nocheckeffectparams);
        if ( ! -f $cause[0] && $nocheck == 0) {
            die "EFFECT FAILED ($causetype -> $effecttype): unable to ".
                "locate expected file ($cause[0]) that was to be ".
                "fetched from $nocheckeffectparams[0]\n";
        }

    }   elsif ( $causetype eq 'dir' ) {
        if ( -d $cause[0] ) {
            print "directory exists: $causetype,$cause[0]\n"; next;
        }
        effect($effecttype,@nocheckeffectparams);
        if ( ! -d $cause[0] && $nocheck == 0) {
            die "EFFECT FAILED ($causetype -> $effecttype): unable to ".
                "locate expected directory ($cause[0]).\n";
        }

    } elsif ( $causetype eq 'file' ) {
        if ( -f $cause[0] ) {print "file already exists: $cause[0]\n"; next;}
        effect($effecttype,@nocheckeffectparams);
        if ( ! -f $cause[0] && $nocheck == 0) {
            die "EFFECT FAILED ($causetype -> $effecttype): unable to ".
                "locate expected file ($cause[0]).\n";
        }
    } elsif ( $causetype eq 'filesame' ) {
        # NOTE - we check file mtime, byte size, AND MD5 of contents
        #      as without the MD5, the script can break in some circumstances.
        my ( $size,$mtime,$md5)=fileinfo($cause[0]);
        my ( $size2,$mtime2,$md5_2)=fileinfo($cause[1]);
        if ( $mtime != $mtime2 || $size != $size2 || $md5 ne $md5_2 ) {
          if ( ! $nocheckeffectparams[0] ) {
            die "sorry but you can not leave the arguments list empty for ".
                "anything except the 'copy' action (and only when used with ".
                "the 'filesame' cause)" unless $effecttype eq 'copy';
            print "no parameters defined, so applying effect($effecttype) as ".
                  "( 2nd src parameter => 1st src parameter)!\n";
            effect($effecttype,$cause[1],$cause[0]); 
          } else {
            effect($effecttype,@nocheckeffectparams);
            # do something else if the files are not 100% identical?
            if ( $nocheck == 0 ) {
              # now verify the effect was successful
              # at matching the file contents!:
              undef $size; undef $size2;
              undef $mtime; undef $mtime2;
              undef $md5; undef $md5_2;
              ( $size,$mtime,$md5)=fileinfo($cause[0]);
              ( $size2,$mtime2,$md5_2)=fileinfo($cause[1]);
              if ( $mtime != $mtime2 || $size != $size2 || $md5 ne $md5_2 ) {
                die("effect failed, files NOT identical size,mtime, ".
                    "and MD5: ($cause[0] => $cause[1]).\n");
              } else {
                print "files ($cause[0] => $cause[1]) now ".
                      "identical - successful! \n";
              }
            }
          }  
        }else {
           print "effect not required files already up-to-date/identical: ".
                 "($cause[0] => $cause[1]).\n";
        }
        undef $size; undef $size2;
        undef $mtime; undef $mtime2;
        undef $md5; undef $md5_2;
        
    } elsif ( $causetype eq 'newer' ) {
        my $mtime = 0;
        if ( -f $cause[0] ) {
          $mtime = (stat($cause[0]))[9]
                   || warn("$cause[0] could not be stated");
        }
        if (! ( -f $cause[1]) ) {
            die "cause: $causetype requires it's SECOND filename to exist ".
                "for comparison: $cause[1].\n";
        }
        my $mtime2  = (stat($cause[1]))[9];
        if ( $mtime < $mtime2 ) {
          effect($effecttype,@nocheckeffectparams);
          if ( $nocheck == 0 ) {
            # confirm it worked, mtimes should have changed now: 
            my $mtime3 = 0;
            if ( -f $cause[0] ) {
              $mtime3   = (stat($cause[0]))[9];
            }
            my $mtime4  = (stat($cause[1]))[9];
            if ( $mtime3 < $mtime4  ) {
                die "EFFECT FAILED ($causetype -> $effecttype): mtime of file".
                    " ($cause[0]) should be greater than file ($cause[1]).\n".
                    "[$mtime3]  [$mtime4]\n";
            }
          }
        } else {
           print "file ($cause[0]) has same or newer mtime than ".
                 "($cause[1]) already, no action taken\n";
        } 
        undef $mtime;
        undef $mtime2;
        
    } elsif ( $causetype eq 'grep' ) {
        print "grep-ing for pattern($cause[0]) in file($cause[1]):\n"
            if $NOISY >0;
        if ( ! _grep($cause[0],$cause[1]) ) {
# grep actually needs two parameters on the source side of the action
            effect($effecttype,@nocheckeffectparams);   
        } else {
            print "grep - already matched source file($cause[1]), ".
                  "with pattern ($cause[0]) - no action reqd\n";
        }
        if ( (! _grep($cause[0],$cause[1])) && $nocheck == 0 ) { 
           die "EFFECT FAILED ($causetype -> $effecttype): unable to locate regex pattern ($cause[0]) in file ($cause[1])\n";
        }

    } elsif ( $causetype eq 'exists' ) {
        print "testing if '$cause[0]' exists...\n" if $NOISY >0;
        if ( -e $cause[0] ) {
            effect($effecttype,@nocheckeffectparams);
        }
    } elsif ( $causetype eq 'always' ) {
        effect($effecttype,@nocheckeffectparams);
    } elsif ( $causetype eq 'stop' ){
        die "Stop found \n";
    } elsif ( $causetype eq 'pause' ){
        comment("PAUSED! : ".$cause);
        my $temp = getc() unless $continuous;
    } else {
        die " unknown causetype $causetype \n";
    }
}
print "\nwin32-developer all done\n";
_end();

#------------------------------------------------------------------------------
# each cause has an effect, this is where we do them:
sub effect {
    my ( $effecttype, @effectparams ) = @_;

        if ( $effecttype eq 'fetch') {
            # passing two parameters that came in via the array
            _fetch(@effectparams);

        } elsif ( $effecttype eq 'extract') {
            my $tarfile = $effectparams[0];
            my $destdir = $effectparams[1] || '';
            if ($destdir eq '') {
                $destdir = $tarfile;
                # strip off everything after the final forward slash
                $destdir =~ s#[^/]*$##;
            }
        my $t = findtar($tarfile);
        print "found equivalent: ($t) -> ($tarfile)\n" if $t ne $tarfile;
        print "extracttar($t,$destdir);\n";
        extracttar($t,$destdir);

        } elsif ($effecttype eq 'exec') { # execute a DOS command
            my $cmd = shift @effectparams;
            #print `$cmd`;
            print "exec:$cmd\n";
            open F, $cmd." |"  || die "err: $!";
            while (<F>) {
                print;
            }   

        } elsif ($effecttype eq 'shell') {
            shell(@effectparams);
            
        } elsif ($effecttype eq 'copy') {
            die "Can not copy non-existant file ($effectparams[0])\n"
                unless -f $effectparams[0];
            print "copying file ($effectparams[0] => $effectparams[1]) \n";
            cp($effectparams[0],$effectparams[1]);
            # make destn mtime the same as the original for ease of comparison:
            shell("touch --reference='".perl2unix($effectparams[0])."' '"
                                       .perl2unix($effectparams[1])."'");

        } elsif ($effecttype eq 'mkdirs') {
            mkdirs(shift @effectparams);

        } elsif ($effecttype eq 'write') {
            # just dump the requested content from the array to the file.
            my $filename = shift @effectparams;
            my $fh = new IO::File ("> $filename")
                || die "error opening $filename for writing: $!\n";
            $fh->binmode();
            $fh->print(join('',@effectparams));
            $fh->close();

        } else {
            die " unknown effecttype $effecttype from cause 'dir'\n";
        }
        return; # which ever one we actioned,
                # we don't want to action anything else 
}

#------------------------------------------------------------------------------
# get info from a file for comparisons
sub fileinfo {
    # filename passed in should be perl compatible path
    # using single FORWARD slashes
    my $filename = shift;

    my ( $size,$mtime,$md5)=(0,0,0);
    
    if ( -f $filename ) {
        $size = (stat($filename))[7];
        $mtime  = (stat($filename))[9];
        my $md5obj = Digest::MD5->new();
        my $fileh = IO::File->new($filename);
        binmode($fileh);
        $md5obj->addfile($fileh);
        $md5 = $md5obj->digest();
    } else {
        warn(" invalid file name provided for testing: ($filename)\n");
        $size=rand(99);
        $mtime=rand(999);
        $md5=rand(999);
        }

    #print "compared: $size,$mtime,$md5\n";
    return ($size,$mtime,$md5);
}

#------------------------------------------------------------------------------
# kinda like a directory search for blah.tar* but faster/easier.
#  only finds .tar.gz, .tar.bz2, .zip
sub findtar {
    my $t = shift;
    return "$t.gz" if -f "$t.gz";
    return "$t.bz2" if -f "$t.bz2";

    if ( -f "$t.zip" || $t =~ m/\.zip$/ ) {
        die "no unzip.exe found ! - yet" unless -f $sources."unzip/unzip.exe";
        # TODO - a bit of a special test, should be fixed better.
        return "$t.zip" if  -f "$t.zip";
        return $t if -f $t;
    }
    return $t if -f $t;
    die "findtar failed to match a file from:($t)\n";
}

#------------------------------------------------------------------------------
# given a ($t) .tar.gz, .tar.bz2, .zip extract it to the directory ( $d)
# changes current directory to $d too
sub extracttar {
    my ( $t, $d) = @_;

    # both $t and $d at this point should be perl-compatible-forward-slashed
    die "extracttar expected forward-slashes only in pathnames ($t,$d)"
        if $t =~ m#\\# || $d =~ m#\\#;

    unless ( $t =~ m/zip/ ) {
        # the unzip tool need the full DOS path,
        # the msys commands need that stripped off.
        $t =~ s#^$msys#/#i;
    }

    print "extracting to: $d\n";

    # unzipping happens in DOS as it's a dos utility:
    if ( $t =~ /\.zip$/ ) {
        #$d =~ s#/#\\#g;  # the chdir command MUST have paths with backslashes,
                          # not forward slashes. 
        $d = perl2dos($d);
        $t = perl2dos($t);
        my $cmd = 'chdir '.$d.' && '.$dossources.'unzip\unzip.exe -o '.$t;
        print "extracttar:$cmd\n";
        open F, "$cmd |"  || die "err: $!";
        while (<F>) {
            print;
        }
        return; 
    }

    $d = perl2unix($d);
    $t = perl2unix($t);
    # untarring/gzipping/bunzipping happens in unix/msys mode:
    die "unable to locate sh2.exe" unless -f $dosmsys.'bin/sh2.exe';
    my $cmd = $dosmsys.
              'bin\sh2.exe -c "( export PATH=/bin:/mingw/bin:$PATH ; ';
    $cmd .= "cd $d;";
    if ( $t =~ /\.gz$/ ) {
        $cmd .= $unixmsys."bin/tar.exe -zxvpf $t";
    } elsif ( $t =~ /\.bz2$/ ) {
        $cmd .= $unixmsys."bin/tar.exe -jxvpf $t";
    } elsif ( $t =~ /\.tar$/ ) {
        $cmd .= $unixmsys."bin/tar.exe -xvpf $t";
    } else {
        die  "extract tar failed on ($t,$d)\n";
    }
    $cmd .= ')"'; # end-off the brackets around the shell commands.

    # execute the cmd, and capture the output!  
    # this is a glorified version of "print `$cmd`;"
    # except it doesn't buffer the output, if $|=1; is set.
    # $t should be a msys compatible path ie /sources/etc
    print "extracttar:$cmd\n";
    open F, "$cmd |"  || die "err: $!";
    while (<F>) {
        print;
    }   
}

#------------------------------------------------------------------------------
# get the $url (typically a .tar.gz or similar) , and save it to $file
sub _fetch {
    my ( $file,$url ) = @_;

    #$file =~ s#/#\\\\#g;
    print "already exists: $file \n" if -f $file;
    return undef if -f $file;

    print "fetching $url to $file (please wait)...\n";
    my $ua = LWP::UserAgent->new;
    $ua->proxy(['http', 'ftp'], $proxy);

    my $res = $ua->request(HTTP::Request->new(GET => $url),
      sub {
          if (! -f $file) {
              open(FILE, ">$file") || die "_fetch can't open $file: $!\n";
              binmode FILE;
          }
          print FILE $_[0] or die "_fetch can't write to $file: $!\n";
      }
    );
    close(FILE) || print "_fetch can't close $file: $!\n";
    if (my $mtime = $res->last_modified) {
        utime time, $mtime, $file;
    }
    if ($res->header("X-Died") || !$res->is_success) {
        unlink($file) && print "Transfer failed.  File deleted.\n";
    }

    if ( ! -s $file ) {
      die ('ERR: Unable to automatically fetch file!\nPerhaps manually '.
           'downloading from the URL to the filename (both listed above) '.
           'might work, or you might want to choose your own SF mirror '.
           '(edit this script for instructions), or perhaps this version'.
           ' of the file is no longer available.');
    }
}

#------------------------------------------------------------------------------
# execute a sequence of commands in a bash shell.
# we explicitly add the /bin and /mingw/bin to the path
# because at this point they aren't likely to be there
# (cause we are in the process of installing them)
sub shell {
    my @cmds = @_;
    my $cmd = $dosmsys.'bin\bash.exe -c "( export PATH=/bin:/mingw/bin:$PATH;'.
              join(';',@cmds).') 2>&1 "';
    print "shell:$cmd\n";
    # execute the cmd, and capture the output!  this is a glorified version
    # of "print `$cmd`;" except it doesn't buffer the output if $|=1; is set.
    open F, "$cmd |"  || die "err: $!";
    while (<F>) {
        if (! $NOISY )  {
          # skip known spurious messages from going to the screen unnecessarily
          next if /redeclared without dllimport attribute after being referenced with dllimpo/;
          next if /declared as dllimport: attribute ignored/;
          next if /warning: overriding commands for target `\.\'/;
          next if /warning: ignoring old commands for target `\.\'/;
          next if /Nothing to be done for `all\'/;
          next if /^cd .* \&\& \\/;
          next if /^make -f Makefile/;
        }
        print;
    }
}

#------------------------------------------------------------------------------
# recursively make folders, requires perl-compatible folder separators
# (ie forward slashes)
# 
sub mkdirs {
    my $path = shift;
    die "backslash in foldername not allowed in mkdirs function:($path)\n"
        if $path =~ m#\\#;
    $path = perl2dos($path);
    print "mkdir $path\n";
    # reduce path to just windows,
    # incase we have a rogue unix mkdir command elsewhere!
    print `set PATH=C:\\WINDOWS\\system32;c:\\WINDOWS;C:\\WINDOWS\\System32\\Wbem && mkdir $path`;

}

#------------------------------------------------------------------------------
# unix compatible  versions of the perl paths (for when we [shell] to unix/bash mode):
sub perl2unix  {
    my $p = shift;
    print "perl2unix: $p\n";
    $p =~ s#$msys#/#i;  # remove superfluous msys folders if they are there

    #change c:/ into /c  (or a D:)   so c:/msys becomes /c/msys etc.
    $p =~ s#^([A-Z]):#/$1#ig;
    $p =~ s#//#/#ig; # reduce any double forward slashes to single ones.
    return $p;
}

#------------------------------------------------------------------------------
# DOS executable CMD.exe versions of the paths (for when we [exec] to DOS mode):
sub perl2dos {
    my $p = shift;
    $p =~ s#/#\\#g; # single forward to single backward
    return $p;
}

#------------------------------------------------------------------------------
# find a pattern in a file and return if it was found or not.
# Absent file assumes pattern was not found.
sub _grep {
    my ($pattern,$file) = @_;
    #$pattern = qw($pattern);
    
    my $fh = IO::File->new("< $file");
    unless ( $fh) {
        print "WARNING: Unable to read file ($file) when searching for ".
              "pattern:($pattern), assumed to NOT match pattern\n";
        return 0;
    }
    my $found = 0;
    while ( my $contents = <$fh> ) {
        if ( $contents =~ m/$pattern/ ) { $found= 1; }
    }
    $fh->close();
    return $found;
}

#------------------------------------------------------------------------------
# where is this script? 
sub scriptpath {
  return "" if ($0 eq "");
  my @path = split /\\/, $0;
  
  pop @path;
  return join '\\', @path;
}

#------------------------------------------------------------------------------
# display stuff to the user in a manner that unclutters it from
# the other compilation messages that will be scrolling past! 
sub comment {
    my $comment = shift;
    print "\nCOMMENTS:";
    print "-"x30;
    print "\n";
    print "COMMENTS:$comment\nCOMMENTS:";
    print "-"x30;
    print "\n";
    print "\n";
}

#------------------------------------------------------------------------------
# how? what the?   oh! like that!  I get it now, I think.
sub usage {
    print << 'END_USAGE';

-h             This message
-v             Verbose output
-k             Package win32 distribution
-p proxy:port  Your proxy
-c debug|release|profile
END_USAGE
    exit;
}

#------------------------------------------------------------------------------
