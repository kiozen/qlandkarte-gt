# <pre>
# @(#)Makefile	8.8
# This file is in the public domain, so clarified as of
# 2009-05-17 by Arthur David Olson.
#
# ...and remains in the PD, even though I changed it...
# (Konrad Rosenbaum: July 2010)

# This is a shorted version that only regenerates the zone data
# using the local installation of tzcode
#
# The original can be found in tzcode.
#
# It also assumes that the tzdata files have been checked before being released.
# All compile and logic check rules have been removed from this version.

# Change the line below for your time zone (after finding the zone you want in
# the time zone files, or adding it to a time zone file).
# Alternately, if you discover you've got the wrong time zone, you can just
#	zic -l rightzone
# to correct things.
# Use the command
#	make zonenames
# to get a list of the values you can use for LOCALTIME.

LOCALTIME=	Factory

# If you want something other than Eastern United States time as a template
# for handling POSIX-style time zone environment variables,
# change the line below (after finding the zone you want in the
# time zone files, or adding it to a time zone file).
# (When a POSIX-style environment variable is handled, the rules in the
# template file are used to determine "spring forward" and "fall back" days and
# times; the environment variable itself specifies UTC offsets of standard and
# summer time.)
# Alternately, if you discover you've got the wrong time zone, you can just
#	zic -p rightzone
# to correct things.
# Use the command
#	make zonenames
# to get a list of the values you can use for POSIXRULES.
# If you want POSIX compatibility, use "America/New_York".

POSIXRULES=	America/New_York

# Also see TZDEFRULESTRING below, which takes effect only
# if the time zone files cannot be accessed.

# Everything gets put in subdirectories of. . .

TOPDIR=		..

# "Compiled" time zone information is placed in the "TZDIR" directory
# (and subdirectories).
# Use an absolute path name for TZDIR unless you're just testing the software.

TZDIR=		$(TOPDIR)/zoneinfo

# If you always want time values interpreted as "seconds since the epoch
# (not counting leap seconds)", use
#	REDO=		posix_only
# below.  If you always want right time values interpreted as "seconds since
# the epoch" (counting leap seconds)", use
#	REDO=		right_only
# below.  If you want both sets of data available, with leap seconds not
# counted normally, use
#	REDO=		posix_right
# below.  If you want both sets of data available, with leap seconds counted
# normally, use
#	REDO=		right_posix
# below.
# POSIX mandates that leap seconds not be counted; for compatibility with it,
# use either "posix_only" or "posix_right".

REDO=		posix_only

# Since "." may not be in PATH...

YEARISTYPE=	./yearistype

# If you want zic's -s option used when installing, uncomment the next line
# ZFLAGS=	-s

zic=		zic
ZIC=		$(zic) $(ZFLAGS)

# The name of a Posix-compliant `awk' on your system.
AWK=		nawk

###############################################################################

PRIMARY_YDATA=	africa antarctica asia australasia \
		europe northamerica southamerica
YDATA=		$(PRIMARY_YDATA) pacificnew etcetera factory backward
NDATA=		systemv
SDATA=		solar87 solar88 solar89
TDATA=		$(YDATA) $(NDATA) $(SDATA)
TABDATA=	iso3166.tab zone.tab
DATA=		$(YDATA) $(NDATA) $(SDATA) $(TABDATA) leapseconds yearistype.sh

# And for the benefit of csh users on systems that assume the user
# shell should be used to handle commands in Makefiles. . .

SHELL=		/bin/sh

all:		zones

posix_only:	$(TDATA)
		$(ZIC) -y $(YEARISTYPE) -d $(TZDIR) -L /dev/null $(TDATA)

right_only:	leapseconds $(TDATA)
		$(ZIC) -y $(YEARISTYPE) -d $(TZDIR) -L leapseconds $(TDATA)

# In earlier versions of this makefile, the other two directories were
# subdirectories of $(TZDIR).  However, this led to configuration errors.
# For example, with posix_right under the earlier scheme,
# TZ='right/Australia/Adelaide' got you localtime with leap seconds,
# but gmtime without leap seconds, which led to problems with applications
# like sendmail that subtract gmtime from localtime.
# Therefore, the other two directories are now siblings of $(TZDIR).
# You must replace all of $(TZDIR) to switch from not using leap seconds
# to using them, or vice versa.
other_two:	leapseconds $(TDATA)
		$(ZIC) -y $(YEARISTYPE) -d $(TZDIR)-posix -L /dev/null $(TDATA)
		$(ZIC) -y $(YEARISTYPE) \
			-d $(TZDIR)-leaps -L leapseconds $(TDATA)

posix_right:	posix_only other_two

right_posix:	right_only other_two

zones:		yearistype $(REDO)

yearistype:	yearistype.sh
	cp $^ $@
	chmod +x $@

clean:
		rm -f core *.o *.out

maintainer-clean: clean
