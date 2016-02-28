#!/usr/bin/env bash

# From https://news.ycombinator.com/item?id=11187198
# Self contained Linux app generation script
#
# This script needs improvement, and improvements specific to this project,
# although it does nearly everything I want.
#
# TODO
#	* Options should be passed to the script to determine which
#	directory things should go to, and what application to copy
#	* There should be a way to indicate what target the application
#	is built for (eg. x86, x86-64, ARMv7, etc).
#
# 	LIBLISP SPECIFIC
#		* Copy lisp code to a sensible directory and make it so
#		that the interpreter finds the scripts
#		* The lisp interpreter should have an "App mode" where it
#		stores its history in the APPDIR/data directory
#		* Plugins should go in their own separate directory
#

set -e
set -u
shopt -s nullglob

BUILDDIR=.
APP=lisp
APPDIR=AppDir
DOCS=(${BUILDDIR}/*.pdf ${BUILDDIR}/{*.htm,*.html} ${BUILDDIR}/*.[1-9])
VERBOSE=-v

echo "This script creates an application with all dependencies needed to run it for Linux."

mkdir ${VERBOSE} -p ${APPDIR}
mkdir ${VERBOSE} -p ${APPDIR}/bin
mkdir ${VERBOSE} -p ${APPDIR}/data
mkdir ${VERBOSE} -p ${APPDIR}/docs

cp ${VERBOSE} ${BUILDDIR}/${APP} AppDir/bin/${APP}
cp ${VERBOSE} `ldd ${APPDIR}/bin/${APP} | grep -o '\W/[^ ]*'` ${APPDIR}/bin

if [ -n DOCS ]; then
	echo "Copying documentation \"${DOCS[@]}\":"
	cp ${VERBOSE} ${DOCS[@]} ${APPDIR}/docs 
fi

if [ -d ${BUILDDIR}/data ]; then
	echo "Copying data directory:"
	cp ${VERBOSE} -r ${BUILDDIR}/data ${APPDIR}
fi

# Copy dependencies of any plugins, and the plugins
for i in *.so; do
	echo "Processing plugin: ${i}"
	cp ${VERBOSE} ${BUILDDIR}/${i} ${APPDIR}/bin/
	cp ${VERBOSE} `ldd ${i} | grep -o '\W/[^ ]*'` ${APPDIR}/bin
done

cat <<EOF > ${APPDIR}/app
#!/bin/bash
SCRIPT_PATH=\$(dirname \$(readlink -f \$0))
\$SCRIPT_PATH/bin/ld-*.so.2 --library-path \$SCRIPT_PATH/bin \$SCRIPT_PATH/bin/${APP} "\$@" 
EOF

chmod ${VERBOSE} 700 ${APPDIR}/app
chmod ${VERBOSE} 700 ${APPDIR}/bin/*

echo "Done"
