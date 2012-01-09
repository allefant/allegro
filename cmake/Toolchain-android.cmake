
SET(CMAKE_SYSTEM_NAME Linux)
SET(CMAKE_SYSTEM_VERSION 1)

#set path for android toolchain -- look

set(ANDROID_NDK_TOOLCHAIN_ROOT "$ENV{HOME}/android-toolchain" CACHE PATH "Path to the Android NDK Standalone Toolchain" )

message( WARNING "selected ${ANDROID_NDK_TOOLCHAIN_ROOT} android toolchain" )
if(NOT EXISTS ${ANDROID_NDK_TOOLCHAIN_ROOT})
   set(ANDROID_NDK_TOOLCHAIN_ROOT "/opt/android-toolchain" CACHE PATH "Path to the Android NDK Standalone Toolchain" )
   message( STATUS "Using default path for toolchain ${ANDROID_NDK_TOOLCHAIN_ROOT}")
   message( STATUS "If you prefer to use a different location, please set the ANDROID_NDK_TOOLCHAIN_ROOT cmake variable.")
endif()
   
#set(ANDROID_NDK_TOOLCHAIN_ROOT ${ANDROID_NDK_TOOLCHAIN_ROOT} CACHE PATH
#    "root of the android ndk standalone toolchain" FORCE)
    
if(NOT EXISTS ${ANDROID_NDK_TOOLCHAIN_ROOT})
  message(FATAL_ERROR
  "${ANDROID_NDK_TOOLCHAIN_ROOT} does not exist!
  You should either set an environment variable:
    export ANDROID_NDK_TOOLCHAIN_ROOT=~/my-toolchain
  or put the toolchain in the default path:
    sudo ln -s ~/android-toolchain /opt/android-toolchain
    ")
endif()

find_program(CMAKE_MAKE_PROGRAM make)

# specify the cross compiler
SET(CMAKE_C_COMPILER   
  ${ANDROID_NDK_TOOLCHAIN_ROOT}/bin/arm-linux-androideabi-gcc${CMAKE_EXECUTABLE_SUFFIX} CACHE PATH "gcc" FORCE)
SET(CMAKE_CXX_COMPILER 
  ${ANDROID_NDK_TOOLCHAIN_ROOT}/bin/arm-linux-androideabi-g++${CMAKE_EXECUTABLE_SUFFIX} CACHE PATH "gcc" FORCE)
#there may be a way to make cmake deduce these TODO deduce the rest of the tools
set(CMAKE_AR
 ${ANDROID_NDK_TOOLCHAIN_ROOT}/bin/arm-linux-androideabi-ar${CMAKE_EXECUTABLE_SUFFIX}  CACHE PATH "archive" FORCE)
set(CMAKE_LINKER
 ${ANDROID_NDK_TOOLCHAIN_ROOT}/bin/arm-linux-androideabi-ld${CMAKE_EXECUTABLE_SUFFIX}  CACHE PATH "linker" FORCE)
set(CMAKE_NM
 ${ANDROID_NDK_TOOLCHAIN_ROOT}/bin/arm-linux-androideabi-nm${CMAKE_EXECUTABLE_SUFFIX}  CACHE PATH "nm" FORCE)
set(CMAKE_OBJCOPY
 ${ANDROID_NDK_TOOLCHAIN_ROOT}/bin/arm-linux-androideabi-objcopy${CMAKE_EXECUTABLE_SUFFIX}  CACHE PATH "objcopy" FORCE)
set(CMAKE_OBJDUMP
 ${ANDROID_NDK_TOOLCHAIN_ROOT}/bin/arm-linux-androideabi-objdump${CMAKE_EXECUTABLE_SUFFIX}  CACHE PATH "objdump" FORCE)
set(CMAKE_STRIP
  ${ANDROID_NDK_TOOLCHAIN_ROOT}/bin/arm-linux-androideabi-strip${CMAKE_EXECUTABLE_SUFFIX}  CACHE PATH "strip" FORCE)
set(CMAKE_RANLIB
  ${ANDROID_NDK_TOOLCHAIN_ROOT}/bin/arm-linux-androideabi-ranlib${CMAKE_EXECUTABLE_SUFFIX}  CACHE PATH "ranlib" FORCE)
#setup build targets, mutually exclusive
set(PossibleArmTargets
  "armeabi;armeabi-v7a;armeabi-v7a with NEON")
set(ARM_TARGETS "armeabi-v7a" CACHE STRING 
    "the arm targets for android, recommend armeabi-v7a 
    for floating point support and NEON.")

set_property(CACHE ARM_TARGETS PROPERTY STRINGS ${PossibleArmTargets} )

set(LIBRARY_OUTPUT_PATH_ROOT ${CMAKE_SOURCE_DIR} CACHE PATH 
    "root for library output, set this to change where
    android libs are installed to")
    
#set these flags for client use
if(ARM_TARGETS STREQUAL "armeabi")
  set(ARMEABI true)
  set(LIBRARY_OUTPUT_PATH ${LIBRARY_OUTPUT_PATH_ROOT}/libs/armeabi
      CACHE PATH "path for android libs" FORCE)
  set(CMAKE_INSTALL_PREFIX ${ANDROID_NDK_TOOLCHAIN_ROOT}/user/armeabi
      CACHE STRING "path for installing" FORCE)
  set(NEON false)
else()
  if(ARM_TARGETS STREQUAL "armeabi-v7a with NEON")
    set(NEON true)
  endif()
  set(ARMEABI_V7A true)
  set( LIBRARY_OUTPUT_PATH ${LIBRARY_OUTPUT_PATH_ROOT}/libs/armeabi-v7a 
       CACHE PATH "path for android libs" FORCE)
  set(  CMAKE_INSTALL_PREFIX ${ANDROID_NDK_TOOLCHAIN_ROOT}/user/armeabi-v7a
      CACHE STRING "path for installing" FORCE)
endif()

# where is the target environment 
SET(CMAKE_FIND_ROOT_PATH  ${ANDROID_NDK_TOOLCHAIN_ROOT}/bin ${ANDROID_NDK_TOOLCHAIN_ROOT}/arm-linux-androideabi ${ANDROID_NDK_TOOLCHAIN_ROOT}/sysroot ${CMAKE_INSTALL_PREFIX} ${CMAKE_INSTALL_PREFIX}/share)

#for some reason this is needed? TODO figure out why...
include_directories(${ANDROID_NDK_TOOLCHAIN_ROOT}/arm-linux-androideabi/include/c++/4.4.3/arm-linux-androideabi)

SET(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM ONLY)
# only search for libraries and includes in the ndk toolchain
SET(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
SET(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)


#It is recommended to use the -mthumb compiler flag to force the generation
#of 16-bit Thumb-1 instructions (the default being 32-bit ARM ones).
SET(CMAKE_CXX_FLAGS "-DGL_GLEXT_PROTOTYPES -fPIC -DANDROID -mthumb -Wno-psabi")
SET(CMAKE_C_FLAGS "-DGL_GLEXT_PROTOTYPES -fPIC -DANDROID -mthumb -Wno-psabi")

#set(LIBCPP_LINK_DIR  ${ANDROID_NDK_TOOLCHAIN_ROOT}/user/lib/thumb)
if(ARMEABI_V7A)  
  #these are required flags for android armv7-a
  SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=armv7-a -mfloat-abi=softfp")
  SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -march=armv7-a -mfloat-abi=softfp")
  if(NEON)
      SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mfpu=neon")
      SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mfpu=neon")
  endif()
endif()

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}" CACHE STRING "c++ flags")
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}" CACHE STRING "c flags")
      
#-Wl,-L${LIBCPP_LINK_DIR},-lstdc++,-lsupc++
#-L${LIBCPP_LINK_DIR} -lstdc++ -lsupc++
#Also, this is *required* to use the following linker flags that routes around
#a CPU bug in some Cortex-A8 implementations:

SET(CMAKE_SHARED_LINKER_FLAGS "-Wl,--fix-cortex-a8 -L${CMAKE_INSTALL_PREFIX}/lib" CACHE STRING "linker flags" FORCE)
SET(CMAKE_MODULE_LINKER_FLAGS "-Wl,--fix-cortex-a8 -L${CMAKE_INSTALL_PREFIX}/lib" CACHE STRING "linker flags" FORCE)

#set these global flags for cmake client scripts to change behavior
set(ANDROID True)
set(BUILD_ANDROID True)

set(OPENGL_LIBRARIES "-lGLESv1_CM -lGLESv2 -lEGL")
set(OPENGL_gl_LIBRARY "-lGLESv1_CM -lGLESv2 -lEGL")
set(OPENGL_glu_LIBRARY "")




