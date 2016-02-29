#!/usr/bin/env bash
set -e
set -u
shopt -s nullglob

################################################################################
# Originally from https://news.ycombinator.com/item?id=11187198
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
#	* Add options for including source (possibly) to the APPDIR
#	* Elaborate on help
#	* Multiple verboseness levels
#	* It would be nice to test this under Cygwin, and get a Mac OSX
#	version working
# 	LIBLISP SPECIFIC
#		* Copy lisp code to a sensible directory and make it so
#		that the interpreter finds the scripts
#		* The lisp interpreter should have an "App mode" where it
#		stores its history in the APPDIR/data directory
#		* Plugins should go in their own separate directory
#
################################################################################

##  helper functions ###########################################################

function die {
	echo "error: ${1}"
	echo
	echo "${HELP}"
	exit 1
}

function verbose {
	if [ "-v" = "${VERBOSE}" ]; then
		echo "${1}"
	fi
}

## variable set up #############################################################

EXTRA=""
VERBOSE=""
BUILDDIR="./"
APP=""
OS=$(uname -s)
MACHINE=$(uname -m)
TARGET=$(echo "${OS}-${MACHINE}" | tr '[:upper:]' '[:lower:]')
PLUGINS_ON="false"

read -r -d '' HELP <<EOF || true
This script creates an application with all dependencies needed to run it for
Linux. 

Options:
	-a	specify application to process (including path to it)
	-h	print out help message and exit
	-p	process plugins as well
	-v	verbose mode on
	-e	stop processing arguments and add arguments after the "-e"
		to the script that calls your application

When passing in variables to be passed to your executable, you might want to
use the \${SCRIPT_PATH} variable

This script expects the application directory to have a specific structure, 
the user should provide an executable ELF file to be processed, within the
directory that the ELF is there should be various optional directories and
files.

	* Documentation (manpages, PDFs, HTML files) should be at the
	same level as the executable
	* Shared objects should be at the same level as the excutable
	* Plugins should be at the same level as the executable
	* The can optionally be a data folder, containing arbitrary
	data, files and folders, which will be copied.

An example directory structure is:

	$ ls -1 /path/to/app/directory/exe
	exe	<= application we want to process
	exe_dependency.so <= a direct dependency of exe
	plugin1.so	<= a plugin that can be loaded by exe
	plugin2.so	<= another plugin
	help.html	<= help document
	man_page.1	<= another help document
	data/		<= arbitrary data

To run this script on the example app:

	$ ${0} -pa /path/to/app/directory/exe 

This will produce a packaged app directory that like this (in the current
working directory):

	$ ls -1R exe-${TARGET}
	exe-${TARGET}/:
	bin/ 
	data/ 
	docs/ 
	exe	<= this script will run the executable ELF file

	exe-${TARGET}/bin:
	exe	<= the executable ELF file
	ld-linux-machine.so.2  
	libc.so.6  
	libdl.so.2  
	plugin1.so
	plugin2.so
	plugin1_dependency.so
	exe_dependency.so
	library_needed_by_exe_dependency.so

	exe-${TARGET}/data:
	whatever.file
	whatever2.jpg
	...

	exe-${TARGET}/docs:

Which should be all that is needed to run that application on another
${MACHINE} machine, running ${OS}.
EOF

## argument processing #########################################################

if [ 0 -eq "${#}" ]; then
	die "no application provided"
fi

# To do
#	* Process multiple apps
#	* Set app directory name
#	* Clean up directory
#	* Default command list for application
while getopts "ehpva:" opt; do
	case ${opt} in
		a)
			APP="${OPTARG##*/}"
			BUILDDIR="${OPTARG%/*}"
			if [ "${APP}" == "${BUILDDIR}" ]; then
				BUILDDIR='./'
			fi
			verbose "app '${APP}' selected in '${BUILDDIR}'"
			;;
		h)
			echo "${HELP}"
			exit 0
			;;
		v)
			echo "verbose mode on"
			VERBOSE="-v"
			;;
		p)	
			PLUGINS_ON="true"
			verbose "plugin processing turned on"
			;;
		e)
			EXTRA=${@:${OPTIND}}
			verbose "adding arguments to app script: ${EXTRA}"
			break
			;;
		\?)
			die "invalid option: -${OPTARG}" >&2
			;;
		:)
			die "option -${OPTARG} requires an argument" >&2
			;;
	esac
done

if [ -z "${APP}" ]; then
	die "a path to an application needs to be provided"
fi

APPDIR="${APP}-${TARGET}"
DOCS=("${BUILDDIR}"/*.pdf "${BUILDDIR}"/{*.htm,*.html} "${BUILDDIR}"/*.[1-9])

## Process application #########################################################

mkdir ${VERBOSE} -p "${APPDIR}"
mkdir ${VERBOSE} -p "${APPDIR}"/bin
mkdir ${VERBOSE} -p "${APPDIR}"/data
mkdir ${VERBOSE} -p "${APPDIR}"/docs

verbose "processing application: ${APP}"
cp ${VERBOSE} "${BUILDDIR}"/${APP} "${APPDIR}"/bin/${APP}
cp ${VERBOSE} `ldd "${APPDIR}"/bin/${APP} | grep -o '\W/[^ ]*'` "${APPDIR}"/bin

if [ -n "${DOCS}" ]; then
	verbose "copying documentation ${DOCS[@]}:"
	cp ${VERBOSE} ${DOCS[@]} "${APPDIR}"/docs 
fi

if [ -d "${BUILDDIR}/data" ]; then
	verbose "copying data directory:"
	cp ${VERBOSE} -r "${BUILDDIR}"/data "${APPDIR}"
fi

# Copy dependencies of any plugins, and the plugins
if [ "true" = ${PLUGINS_ON} ]; then
	for i in "${BUILDDIR}"/*.so; do
		verbose "processing plugin: ${i}"
		cp ${VERBOSE} "${i}" "${APPDIR}"/bin/
		cp ${VERBOSE} `ldd "${i}" | grep -o '\W/[^ ]*'` "${APPDIR}"/bin
	done
fi

cat <<EOF > "${APPDIR}"/"${APP}"
#!/usr/bin/env bash
set -e
set -u
SCRIPT_PATH=\$(dirname \$(readlink -f \${0}))
"\${SCRIPT_PATH}"/bin/ld-*.so.2 --library-path "\${SCRIPT_PATH}"/bin "\${SCRIPT_PATH}"/bin/"${APP}" ${EXTRA} "\${@}" 
EOF
# To do print generated script

chmod ${VERBOSE} 700 "${APPDIR}"/"${APP}"
chmod ${VERBOSE} 700 "${APPDIR}"/bin/*

verbose "done"

### End ########################################################################
