#!/bin/sh
#
# Sets up the loadpng package for building with the specified compiler.
#

proc_help()
{
   echo
   echo "Usage: $0 <platform> [--quick|--dtou|--utod]"
   echo
   echo "Where platform is one of: djgpp, mingw32, unix, macosx"
   echo "The --quick parameter turns off text file conversion, --dtou converts from"
   echo "DOS/Win32 format to Unix, --utod converts from Unix to DOS/Win32 format."
   echo "If no parameter is specified --quick is assumed."
   echo
}

proc_fix()
{
   echo "Configuring loadpng for $1 ..."

   echo "# generated by fix.sh" > Makefile
   echo "MAKEFILE_INC=$2"      >> Makefile
   echo "include Makefile.all" >> Makefile
}

proc_fix_msvc()
{
   echo "Configuring loadpng for $1 ..."

   echo "# generated by fix.sh" > Makefile
   echo "include Makefile.vc"  >> Makefile
}

proc_filelist()
{
   # common files.
   AL_FILELIST=`find . -type f "(" ! -path "*/.*" ")" -a "(" \
      -name "*.c" -o -name "*.cfg" -o -name "*.cpp" -o -name "*.def" -o \
      -name "*.h" -o -name "*.hin" -o -name "*.in" -o -name "*.inc" -o \
      -name "*.m" -o -name "*.m4" -o -name "*.mft" -o -name "*.s" -o \
      -name "*.rc" -o -name "*.rh" -o -name "*.spec" -o -name "*.pl" -o \
      -name "*.txt" -o -name "*._tx" -o -name "Makefile*" -o \
      -name "*.inl" -o -name "configure" -o -name "CHANGES" -o \
      -name "AUTHORS" -o -name "THANKS" ")" \
   `

   # touch unix shell scripts?
   if [ "$1" != "omit_sh" ]; then
      AL_FILELIST="$AL_FILELIST `find . -type f -name '*.sh'`"
   fi

   # touch DOS batch files?
   if [ "$1" != "omit_bat" ]; then
      AL_FILELIST="$AL_FILELIST `find . -type f -name '*.bat'`"
   fi
}

proc_utod()
{
   echo "Converting files from Unix to DOS/Win32 ..."
   proc_filelist "omit_sh"
   /bin/sh ../../misc/utod.sh $AL_FILELIST
}

proc_dtou()
{
   echo "Converting files from DOS/Win32 to Unix ..."
   proc_filelist "omit_bat"
   /bin/sh ../../misc/dtou.sh $AL_FILELIST
}

# prepare loadpng for the given platform.

if [ -z "$1" ]; then
   proc_help
   exit 0
fi

case "$1" in
   "djgpp"   ) proc_fix "DOS (djgpp)"       "Makefile.dj";;
   "mingw"   ) proc_fix "Windows (MinGW)"   "Makefile.mgw";;
   "mingw32" ) proc_fix "Windows (MinGW)"   "Makefile.mgw";;
   "unix"    ) proc_fix "Unix"              "Makefile.unx";;
   "macosx"  ) proc_fix "Mac OS X"          "Makefile.osx";;
   # used only by allegro's zipup.sh in packaging process
   "msvc"    ) proc_fix_msvc "Windows (MSVC)";;
   "help"    ) proc_help; exit 0;;
   *         ) echo "Platform not supported by loadpng."; exit 0;;
esac

# convert all text-file line endings.

case "$2" in
   "--utod"  ) proc_utod "$1";;
   "--dtou"  ) proc_dtou "$1";;
esac

# set execute permissions just in case.

chmod +x *.sh
