#! /bin/sh

# Simple program to make new version numbers for the shell.
# Big deal, but it was getting out of hand to do everything
# in the makefile.  This creates a file named by the -o option,
# otherwise everything is echoed to the standard output.

PROGNAME=`basename $0`
USAGE="$PROGNAME [-b] -d version -p patchlevel [-s status]"

while [ $# -gt 0 ]; do
	case "$1" in
	-o)	shift; OUTFILE=$1; shift ;;
	-b)	shift; inc_build=yes ;;
	-s)	shift; rel_status=$1; shift ;;
	-p)	shift; patch_level=$1; shift ;;
	-d)	shift; dist_version=$1; shift ;;
	*)	echo "$PROGNAME: usage: $USAGE" >&2 ; exit 2 ;;
	esac
done

# Required arguments
if [ -z "$dist_version" ]; then
	echo "${PROGNAME}: required argument -d missing" >&2
	echo "$PROGNAME: usage: $USAGE" >&2
	exit 1
fi

if [ -z "$patch_level" ]; then
	echo "${PROGNAME}: required argument -p missing" >&2
	echo "$PROGNAME: usage: $USAGE" >&2
	exit 1
fi

# Defaults
if [ -z "$rel_status" ]; then
	rel_status="release"
fi

build_ver=
if [ -r .build ]; then
	build_ver=`cat .build`
fi
if [ -z "$build_ver" ]; then
	build_ver=0
fi

# increment the build version if that's what's required

if [ -n "$inc_build" ]; then
	build_ver=`expr $build_ver + 1`
fi

# If we have an output file specified, make it the standard output
if [ -n "$OUTFILE" ]; then
	if exec >$OUTFILE; then
		:
	else
		echo "${PROGNAME}: cannot redirect standard output to $OUTFILE" >&2
		exit 1
	fi
fi

# Output the leading comment.
echo "/* Version control for the shell.  This file gets changed when you say"
echo "   \`make version.h' to the Makefile.  It is created by mkversion. */"

# Output the distribution version
float_dist=`echo $dist_version | awk '{printf "%.2f\n", $1}'`

echo
echo "/* The distribution version number of this shell. */"
echo "#define DISTVERSION \"${float_dist}\""

# Output the patch level
echo
echo "/* The patch level of this version of the shell. */"
echo "#define PATCHLEVEL ${patch_level}"

# Output the build version
echo
echo "/* The last built version of this shell. */"
echo "#define BUILDVERSION ${build_ver}"

# Output the release status
echo
echo "/* The release status of this shell. */"
echo "#define RELSTATUS \"${rel_status}\""

# Output the SCCS version string
sccs_string="${float_dist}.${patch_level}(${build_ver}) ${rel_status} GNU"
echo
echo "/* A version string for use by sccs and the what command. */"
echo "#define SCCSVERSION \"@(#)Bash version ${sccs_string}\""

if [ -n "$inc_build" ]; then
	# Make sure we can write to .build
	if [ -f .build ] && [ ! -w .build ]; then
		echo "$PROGNAME: cannot write to .build, not incrementing build version" >&2
	else
		echo "$build_ver" > .build
	fi
fi
	
exit 0
