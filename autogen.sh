:
    eval 'exec perl -S $0 ${1+"$@"}'
        if 0;

use strict;

sub clean()
{
    system ("rm -Rf autom4te.cache");
    system ("rm -f missing install-sh mkinstalldirs libtool ltmain.sh");
    print "cleaned the build tree\n";
}

my $aclocal;

# check we have various vital tools
sub sanity_checks($)
{
    my $system = shift;
    my @path = split (':', $ENV{'PATH'});
    my %required =
      (
       'pkg-config' => "pkg-config is required to be installed",
       'autoconf'   => "autoconf is required",
       $aclocal     => "$aclocal is required",
      );

    for my $elem (@path) {
        for my $app (keys %required) {
            if (-f "$elem/$app") {
                delete $required{$app};
            }
        }
    }
    if ((keys %required) > 0) {
        print ("Various low-level dependencies are missing, please install them:\n");
        for my $app (keys %required) {
            print "\t $app: " . $required{$app} . "\n";
        }
        exit (1);
    }
}

# one argument per line
sub read_args($)
{
    my $file = shift;
    my $fh;
    my @lst;
    open ($fh, $file) || die "can't open file: $file";
    while (<$fh>) {
        chomp();
        # migrate from the old system
        if ( substr($_, 0, 1) eq "'" ) {
            print "Migrating options from the old autogen.lastrun format, using:\n";
            my @opts;
            @opts = split(/'/);
            foreach my $opt (@opts) {
                if ( substr($opt, 0, 1) eq "-" ) {
                    push @lst, $opt;
                    print "  $opt\n";
                }
            }
        } elsif ( substr($_, 0, 1) eq "#" ) {
            # comment
        } else {
            push @lst, $_;
        }
    }
    close ($fh);
    # print "read args from file '$file': @lst\n";
    return @lst;
}

my @cmdline_args = ();
if (!@ARGV) {
    my $lastrun = "autogen.lastrun";
    @cmdline_args = read_args ($lastrun) if (-f $lastrun);
} else {
    @cmdline_args = @ARGV;
}

my @args;
for my $arg (@cmdline_args) {
    if ($arg eq '--clean') {
        clean();
    } else {
        push @args, $arg;
    }
}
for my $arg (@args) {
    if ($arg =~ /^([A-Z]+)=(.*)/) {
        $ENV{$1} = $2;
    }
}

# Alloc $ACLOCAL to specify which aclocal to use
$aclocal = $ENV{ACLOCAL} ? $ENV{ACLOCAL} : 'aclocal';

my $system = `uname -s`;
chomp $system;

sanity_checks ($system);# unless($system eq 'Darwin');

my $aclocal_flags = $ENV{ACLOCAL_FLAGS};

$aclocal_flags = "-I ./m4 -I /opt/homebrew/share/aclocal -I /usr/local" if (($aclocal_flags eq "") && ($system eq 'Darwin'));

print "aclocal_flags : $aclocal_flags \n";

$ENV{AUTOMAKE_EXTRA_FLAGS} = '--warnings=no-portability' if (!($system eq 'Darwin'));

system ("$aclocal $aclocal_flags") && die "Failed to run aclocal";
unlink ("configure");
system ("autoreconf -i -f") && die "Failed to run autoconf";
die "failed to generate configure" if (! -x "configure");
system ("intltoolize --copy --force --automake") && die "Failed to intltoolize";

if (defined $ENV{NOCONFIGURE}) {
    print "Skipping configure process.";
} else {
    # Save autogen.lastrun only if we did get some arguments on the command-line
    if (@ARGV) {
        if ($#cmdline_args > 0) {
            # print "writing args to autogen.lastrun\n";
            my $fh;
            open ($fh, ">autogen.lastrun") || die "can't open autogen.lastrun: $!";
            for my $arg (@cmdline_args) {
                print $fh "$arg\n";
            }
            close ($fh);
        }
    }
    print "running ./configure with '" . join ("' '", @args), "'\n";
    system ("./configure", @args) && die "Error running configure";
}

# Local Variables:
# mode: perl
# cperl-indent-level: 4
# tab-width: 4
# indent-tabs-mode: nil
# End:

# vim:set shiftwidth=4 softtabstop=4 expandtab: #
