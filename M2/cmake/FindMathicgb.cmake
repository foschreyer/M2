# Try to find the mathicgb librairies
# See https://github.com/Macaulay2/mathicgb
#
# This file sets up mathicgb for CMake. Once done this will define
#  MATHICGB_FOUND           - system has MATHICGB lib
#  MATHICGB_INCLUDE_DIR     - the MATHICGB include directory
#  MATHICGB_LIBRARIES       - Libraries needed to use MATHICGB
#
# Copyright (c) 2020, Mahrud Sayrafi, <mahrud@umn.edu>
#
# Redistribution and use is allowed according to the terms of the BSD license.

find_path(MATHICGB_INCLUDE_DIR NAMES mathicgb.h )
find_library(MATHICGB_LIBRARIES NAMES mathicgb libmathicgb)

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(MATHICGB DEFAULT_MSG MATHICGB_INCLUDE_DIR MATHICGB_LIBRARIES)

mark_as_advanced(MATHICGB_INCLUDE_DIR MATHICGB_LIBRARIES)
