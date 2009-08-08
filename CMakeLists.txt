cmake_minimum_required(VERSION 2.4.7 FATAL_ERROR)

project(MinionProject)

set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake-modules)
include(CheckIncludeFileCXX)
include(CheckCXXCompilerFlag)

set(CMAKE_ALLOW_LOOSE_LOOP_CONSTRUCTS true)

if(NOT WIN32)
    include(LocalFindBoost)
endif()

include(constraints)
include(CheckIncludeFiles)

set(BUILD_DEF_FILE "${PROJECT_SOURCE_DIR}/minion/BuildDefines.h")
file(REMOVE ${BUILD_DEF_FILE})

if(${PROJECT_SOURCE_DIR} STREQUAL ${PROJECT_BINARY_DIR})
    message(FATAL_ERROR "You cannot run CMake in the source directory (please take a look at the README).")
endif()

set(GITVERSION "0")
set(GITDATE "unknown")
find_program(GIT git)
if(GIT)
    execute_process(COMMAND ${GIT} log -1 --format=format:%H
            WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
            OUTPUT_VARIABLE GITVERSION OUTPUT_STRIP_TRAILING_WHITESPACE)
    execute_process(COMMAND ${GIT} log -1 --format=format:%ad
            WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
            OUTPUT_VARIABLE GITDATE OUTPUT_STRIP_TRAILING_WHITESPACE)
    if(GITDATE STREQUAL "unknown")
        message(STATUS "Unable to determine git version (no checkout?)")
    endif()
    message(STATUS "Git version ${GITVERSION}, date ${GITDATE}")
else()
    message(STATUS "No git executable found, unable to determine git version and date")
endif()

file(APPEND ${BUILD_DEF_FILE} "#define GIT_VER ${GITVERSION}\n")
file(APPEND ${BUILD_DEF_FILE} "#define GIT_DATE ${GITDATE}\n")


# if we have a bash, generate constraints and help if necessary and add build
# targets to do that
if(UNIX)
    find_program(BASH bash)
    if(BASH)
        add_custom_target(generate-help ${BASH} "minion/help/genhelp.sh" > "minion/help/help.cpp"
                  WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
                  COMMENT "generate-help" VERBATIM)
        if(EXISTS "${PROJECT_SOURCE_DIR}/minion/help/help.cpp")
            message(STATUS "Help file exists, not regenerating")
        else()
            message(STATUS "Generating help")
            execute_process(COMMAND ${BASH} "minion/help/genhelp.sh"
                    WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
                    OUTPUT_FILE "minion/help/help.cpp")
            message(STATUS "Generating help - done")
        endif()
    else()
        message(STATUS "No bash executable found, not generating help")
    endif()
else()
    message(STATUS "You need a UNIX-like system to regenerate help")
endif()

if(CONSTRAINTS)
    select_constraints(${CONSTRAINTS})
elseif(CONSTRAINTS_FILE)
    file(READ ${CONSTRAINTS_FILE} CONSTRAINTS_FILE_CONTENTS)
    string(REGEX MATCHALL "([^a-z_-]|^)[a-z_-]+\\(" FINDS ${CONSTRAINTS_FILE_CONTENTS})
    foreach(FIND ${FINDS})
        string(REGEX REPLACE "[^a-z_-]" "" CONSTRAINT ${FIND})
        set(FOUND False)
        foreach(LIST_CONSTRAINT ${CONSTRAINTS})
            if(${LIST_CONSTRAINT} STREQUAL ${CONSTRAINT})
                set(FOUND True)
            endif()
        endforeach()
        if(NOT FOUND)
            list(APPEND CONSTRAINTS ${CONSTRAINT})
        endif()
    endforeach()
    select_constraints(${CONSTRAINTS})
else()
    select_constraints(${ALL_CONSTRAINTS})
endif()

file(GLOB SRCS "${PROJECT_SOURCE_DIR}/minion/build_constraints/CT*.cpp")

foreach(ct ${SRCS})
    string(REGEX MATCH "CT_[A-Z_0-9]+" ct_name ${ct})
	file(APPEND ${BUILD_DEF_FILE} "#define ${ct_name}_ABC\n")
#    add_definitions(-D${ct_name}_ABC)
endforeach()



list(APPEND SRCS ${PROJECT_SOURCE_DIR}/minion/BuildVariables.cpp
                 ${PROJECT_SOURCE_DIR}/minion/BuildCSP.cpp
                 ${PROJECT_SOURCE_DIR}/minion/commandline_parse.cpp
                 ${PROJECT_SOURCE_DIR}/minion/lock.cpp
                 ${PROJECT_SOURCE_DIR}/minion/debug_functions.cpp
                 ${PROJECT_SOURCE_DIR}/minion/get_info.cpp
#                ${PROJECT_SOURCE_DIR}/minion/minion.cpp
                 ${PROJECT_SOURCE_DIR}/minion/globals.cpp
                 ${PROJECT_SOURCE_DIR}/minion/preprocess.cpp
                 ${PROJECT_SOURCE_DIR}/minion/build_constraints/BuildStaticStart.cpp
                 ${PROJECT_SOURCE_DIR}/minion/system/trigger_timer.cpp
                 ${PROJECT_SOURCE_DIR}/minion/help/help.cpp)

set(BINARY minion)

find_program(UNAME uname)
if(UNAME)
    execute_process(COMMAND ${UNAME}
            OUTPUT_VARIABLE UNAME_OUT OUTPUT_STRIP_TRAILING_WHITESPACE)
    if(UNAME_OUT STREQUAL "Darwin")
        set(APPLEMAC 1)
    elseif(UNAME_OUT MATCHES "CYGWIN.*")
        set(WINCYGWIN 1)
    endif()
endif()

if(WINCYGWIN)
    set(CFLAGS "${CFLAGS} -enable-auto-import")
endif()

set(BOOST_COMPONENTS iostreams)

option(NAUTY "" OFF)
if(NAUTY)
  message(STATUS "Please be naughty")
  add_definitions(-DUSE_NAUTY)
  list(APPEND SRCS ${PROJECT_SOURCE_DIR}/nauty24b7/nautycxx.cpp
                   ${PROJECT_SOURCE_DIR}/nauty24b7/nauty.c
                   ${PROJECT_SOURCE_DIR}/nauty24b7/nautil.c
                   ${PROJECT_SOURCE_DIR}/nauty24b7/nausparse.c)
endif()

option(STATIC "" OFF)
if(STATIC)
    message(STATUS "Building static binary")
    set(BINARY "${BINARY}-static")
    if(APPLEMAC)
        set(Boost_USE_STATIC_LIBS true)
        set(CMAKE_OSX_SYSROOT "/Developer/SDKs/MacOSX10.4u.sdk")
        add_definitions(-mmacosx-version-min=10.4)
        set(CMAKE_EXE_LINKER_FLAGS ${CMAKE_EXE_LINKER_FLAGS} -mmacosx-version-min=10.4 )
    else()
        # the line below builds minion with just the boost stuff statically
        # linked, however, as the location of the other libraries varies between
        # linux distros, we just stuff everything into the binary
        #set(Boost_USE_STATIC_LIBS true)
        set(CMAKE_EXE_LINKER_FLAGS "-static")
    endif()
endif()

find_package(Boost 1.33.0 COMPONENTS ${BOOST_COMPONENTS})
if(Boost_FOUND)
    set(CFLAGS "${CFLAGS} -I${Boost_INCLUDE_DIRS}")
    find_package(ZLIB)
    if(ZLIB_FOUND)
        find_package(BZip2)
        if(BZIP2_FOUND OR NOT BZIP2_LIBRARIES)
            message(STATUS "Boost and all required libraries found, building with boost")
            aux_source_directory("${PROJECT_SOURCE_DIR}/minion/boost_files" SRCS)
            add_definitions(-DUSE_BOOST_STREAMS)
        else()
            message(STATUS "Bzip2 not found, building without compressed file support")
        endif()
    else()
        message(STATUS "Zlib not found, building without compressed file support")
    endif()
else()
    message(FATAL_ERROR "Boost not found -- do you have the iostreams component?")
endif()

find_program(CCONTROL ccontrol)
if(CCONTROL)
    message(STATUS "Using ccontrol")
    set(CMAKE_CXX_COMPILER_ARG1 ${CMAKE_CXX_COMPILER})
    set(CMAKE_CXX_COMPILER ${CCONTROL})
else()
    find_program(CCACHE ccache)

    option(DISTCC "" OFF)
    if(DISTCC)
        find_program(DISTCC-P distcc)
        if(DISTCC-P)
            message(STATUS "Using distcc")
            if(CCACHE)
                set(ENV{CCACHE_PREFIX} "distcc")
            else()
                set(CMAKE_CXX_COMPILER ${DISTCC-P})
            endif()
        endif()
    endif()

    if(CCACHE)
        message(STATUS "Using ccache")
        set(CMAKE_CXX_COMPILER_ARG1 ${CMAKE_CXX_COMPILER})
        set(CMAKE_CXX_COMPILER ${CCACHE})
    endif()
endif()

#check_cxx_compiler_flag("-std=c++0x" CheckForCXX)
#if(CheckForCXX)
#   message(STATUS "Compiler supports C++0x!")
#   add_definitions("-std=c++0x")
#else()
#   message(STATUS "Compiler does not support C++0x")
#endif()

set(WFLAGS "-W -Wall -Wno-sign-compare -Wno-missing-braces -Wno-unused-parameter -fwrapv")
check_cxx_compiler_flag(${WFLAGS} CheckWarnings)
if(CheckWarnings)
    add_definitions(${WFLAGS})
endif()

# slightly weird check to make sure that the boost TR1 libs are there

if(Boost_FOUND)
    set(CMAKE_REQUIRED_INCLUDES ${CMAKE_REQUIRED_INCLUDES} ${Boost_INCLUDE_DIRS})
    check_include_file_cxx("boost/unordered_set.hpp" BOOST-TR1)
endif()

if(BOOST-TR1)
    message(STATUS "Using Boost TR1 libraries")
    add_definitions(-DUSE_BOOST)
else()
    check_include_file_cxx("tr1/unordered_map" TR1-MAP)
    check_include_file_cxx("tr1/unordered_set" TR1-SET)
    if(TR1-MAP AND TR1-SET)
        message(STATUS "Using TR1 hashed containers")
        add_definitions(-DUSE_TR1_HASH_MAP_AND_SET)
    else()
        message(STATUS "Using tree containers")
    endif()
endif()

option(DEBUG "" OFF)
option(SLOW_DEBUG "" OFF)
if(SLOW_DEBUG)
    message(STATUS "Enabling slow DEBUG")
    set(BINARY ${BINARY}-debugextra)
    add_definitions(-DSLOW_DEBUG)
    set(DEBUG TRUE)
endif()

option(PRINT "" OFF)
if(DEBUG)
    message(STATUS "Enabling DEBUG")
    set(BINARY "${BINARY}-debug")
    if(PRINT)
        message(STATUS "Enabling PRINT")
        add_definitions(-D_GLIBCXX_DEBUG -DMINION_DEBUG_PRINT -DMORE_SEARCH_INFO)
    else()
        add_definitions(-D_GLIBCXX_DEBUG -DMINION_DEBUG -DMORE_SEARCH_INFO)
    endif()
endif()


option(THREADSAFE "" OFF)
if(THREADSAFE)
    message(STATUS "Compiling thread-safe minion")
    set(BINARY "${BINARY}-threads")
    add_definitions(-DTHREADSAFE)
endif()

if(WIN32)
	set(CFLAGS "${CFLAGS} /EHsc")
endif()

option(PROFILE "" OFF)
option(GPROF "" OFF)
if(PROFILE)
    message(STATUS "Enabling profiling")
    set(BINARY "${BINARY}-profile")
    set(CFLAGS "${CFLAGS} -O -g -fno-inline -fno-inline-functions")
    if(GPROF)
        message(STATUS "Enabling gprof")
        set(CFLAGS "${CFLAGS} -pg")
    endif()
    set(UNOPTIMISED ON) # Profiing can't cope with -fomit-frame-pointer
endif()

option(UNOPTIMISED "" OFF)
option(COVERAGE "" OFF)
if(COVERAGE)
  set(CFLAGS "${CFLAGS} -ftest-coverage -fprofile-arcs")
  set(BINARY "${BINARY}-coverage")
  set(UNOPTIMISED TRUE)
endif()

if(UNOPTIMISED)
    message(STATUS "Building unoptimised binary")
    set(BINARY "${BINARY}-unoptimised")
    set(CFLAGS "${CFLAGS} -g")
else()
    # apparently fomit-frame-pointer is only automatically enabled for O* levels
    # on machines where this doesn't affect debugging
	if(WIN32)
		set(CFLAGS "${CFLAGS} /O2")
	else()
		set(CFLAGS "${CFLAGS} -O3 -fomit-frame-pointer")
		check_cxx_compiler_flag("-mdynamic-no-pic" MAC_MDYNAMIC)
		if(MAC_MDYNAMIC)
			set(CFLAGS "${CFLAGS} -mdynamic-no-pic")
		endif()
	endif()
endif()



option(SMALL "" OFF)
if(SMALL)
    message(STATUS "Building small binary")
    set(BINARY "${BINARY}-small")
    set(CFLAGS "${CFLAGS} -Os")
endif()

option(WDEG "" OFF)
if(WDEG)
    message(STATUS "Enabling wdeg heuristics")
    set(BINARY "${BINARY}-wdeg")
    add_definitions(-DWDEG)
endif()

option(WTRIG "" OFF)
if(WTRIG)
    message(STATUS "Enabling weighted triggers")
    set(BINARY "${BINARY}-wtrig")
    add_definitions(-DWEIGHTED_TRIGGERS)
endif()

option(INFO "" OFF)
if(INFO)
    message(STATUS "Enabling INFO")
    set(BINARY "${BINARY}-info")
    add_definitions(-DMORE_SEARCH_INFO)
endif()

option(QUICK "" OFF)
if(QUICK)
    message(STATUS "Enabling quick compilation")
    set(BINARY "${BINARY}-quick")
    add_definitions(-DQUICK_COMPILE)
endif()

option(REENTER "" OFF)
if(REENTER)
    message(STATUS "Enabling REENTER")
    set(BINARY "${BINARY}-reenter")
    add_definitions(-DREENTER)
endif()

if(DEFINED ENV{CPU})
    message(STATUS "Using CPU-specific flags $ENV{CPU}")
    set(CFLAGS "${CFLAGS} $ENV{CPU}")
else()
    message(STATUS "No CPU-specific compiler flags configured")
endif()
set(CMAKE_CXX_FLAGS ${CFLAGS})
set(CMAKE_C_FLAGS ${CFLAGS})

if(NAME)
    set(BINARY ${NAME})
endif()
message(STATUS "Executable name set to \"${BINARY}\"")

add_library(inputfile_parse ${PROJECT_SOURCE_DIR}/minion/inputfile_parse/inputfile_parse.cpp)

add_library(${BINARY}-lib STATIC EXCLUDE_FROM_ALL ${SRCS})

add_executable(${BINARY} ${SRCS} ${PROJECT_SOURCE_DIR}/minion/minion.cpp)
target_link_libraries(${BINARY} inputfile_parse)
#target_link_libraries(${BINARY} ${BINARY}-lib)

# when we're building minion-something, add a "minion" target that does the sa~
if(NOT ${BINARY} STREQUAL minion)
    add_custom_target(minion DEPENDS ${BINARY})
    add_custom_target(minion-lib DEPENDS ${BINARY}-lib)
endif()

if(Boost_FOUND)
    target_link_libraries(${BINARY} "-L${Boost_LIBRARY_DIRS}" ${Boost_LIBRARIES} ${ZLIB_LIBRARIES} ${BZIP2_LIBRARIES})
endif()

#if(GMP_FOUND)
#    target_link_libraries(${BINARY} ${GMPXX_LIBRARY} ${GMP_LIBRARY})
#endif()

# generators
add_executable(bibd "${PROJECT_SOURCE_DIR}/generators/Bibd/MinionBIBDInstanceGenerator.cpp")
add_executable(golomb "${PROJECT_SOURCE_DIR}/generators/Golomb/GolombMinionGenerator.cpp")
add_executable(graceful "${PROJECT_SOURCE_DIR}/generators/Graceful/GracefulMinionGenerator.cpp")
add_executable(indicator "${PROJECT_SOURCE_DIR}/generators/indicator/indicator.cpp")
add_executable(langford "${PROJECT_SOURCE_DIR}/generators/Langford/langford.cpp")
add_executable(nqueens "${PROJECT_SOURCE_DIR}/generators/nqueens-JFP/nqueensgen.cpp")
add_executable(primequeens "${PROJECT_SOURCE_DIR}/generators/PrimeQueens/MinionPrimeQueenInstanceGenerator.cpp")
add_executable(solitaire "${PROJECT_SOURCE_DIR}/generators/Solitaire/solitaire-solver.cpp")
add_executable(sports "${PROJECT_SOURCE_DIR}/generators/SportsSchedule/MinionSportsInstanceGenerator.cpp")
add_executable(steelmill "${PROJECT_SOURCE_DIR}/generators/Steelmill/steelmill-solver.cpp")

add_custom_target(generate DEPENDS bibd golomb graceful indicator langford
                   nqueens primequeens solitaire steelmill sports)

# lisp stuff
find_program(CLISP clisp)
if(CLISP)
    message(STATUS "Clisp found, generating LISP targets")
    set(CLISP_FLAGS "-q -q -C -x")
    add_custom_target(minion-helper ${CLISP} ${CLISP_FLAGS}
        "(clisp-make-executable \"minion-helper\")"
        -i "${PROJECT_SOURCE_DIR}/generators/MinionHelper.lsp"
        WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
        COMMENT "minion-helper" VERBATIM)
    add_custom_target(minion-sat ${CLISP} ${CLISP_FLAGS}
        "(clisp-make-executable \"minion-sat\" (function clisp-toplevel-sat))"
        -i "${PROJECT_SOURCE_DIR}/generators/MinionHelper.lsp"
        -i "${PROJECT_SOURCE_DIR}/generators/SAT/MinionDimacsSAT.lsp"
        WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
        COMMENT "minion-sat" VERBATIM)
    add_custom_target(minion-quasigroup ${CLISP} ${CLISP_FLAGS}
        "(clisp-make-executable \"minion-quasigroup\" (function clisp-toplevel-quasigroup))"
        -i "${PROJECT_SOURCE_DIR}/generators/MinionHelper.lsp"
        -i "${PROJECT_SOURCE_DIR}/generators/Quasigroup/MinionQuasigroup.lsp"
        WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
        COMMENT "minion-quasigroup" VERBATIM)
    add_custom_target(lisp-generate WORKING_DIRECTORY ${PROJECT_BINARY_DIR})
    add_dependencies(lisp-generate minion-helper minion-sat minion-quasigroup)
    set_directory_properties(PROPERTY ADDITIONAL_MAKE_CLEAN_FILES
        "${PROJECT_BINARY_DIR}/minion-helper;${PROJECT_BINARY_DIR}/minion-sat;${PROJECT_BINARY_DIR}/minion-quasigroup")
else()
    message(STATUS "No clisp executable found, not generating LISP targets")
endif()

# help
# no need to check for bash again, if it's a UNIX system the check will have
# happened already
if(UNIX AND BASH)
    message(STATUS "Generating HTML documentation target")
    add_custom_target(htmlhelp ${BASH} "docs/genhelp/genhelp.sh" "minion"
              WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
              COMMENT "htmlhelp" VERBATIM)
    find_program(PDFLATEX pdflatex)
    if(PDFLATEX)
        message(STATUS "Generating PDF documentation target")
        add_custom_target(pdfhelp
                  COMMAND ${BASH} "docs/genhelp/genlatexhelp.sh" "minion"
                  COMMAND ${PDFLATEX} "-output-directory=docs/latexhelp" "docs/latexhelp/doc.latex"
                  COMMAND ${PDFLATEX} "-output-directory=docs/latexhelp" "docs/latexhelp/doc.latex"
                  WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
                  COMMENT "pdfhelp" VERBATIM)
    else()
        message(STATUS "No pdflatex executable found, not generating PDF documentation target")
    endif()
else()
    message(STATUS "You need a UNIX-like system to generate the documentation")
endif()

find_program(DOXYGEN doxygen)
if(DOXYGEN)
    message(STATUS "Generating doxygen documentation target")
    add_custom_target(api-doc ${DOXYGEN}
              WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}/minion"
              COMMENT "api-doc" VERBATIM)
else()
    message(STATUS "Doxygen not found, not generating doxygen documentation target")
endif()

add_custom_target(test-instances-quick DEPENDS ${BINARY}
                  WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}/test_instances"
                  COMMAND "./do_basic_tests.sh" "${PROJECT_BINARY_DIR}/${BINARY}" VERBATIM)
                  
add_custom_target(test-instances-slow DEPENDS ${BINARY}
                  WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}/test_instances"
                  COMMAND "./run_tests.sh" "${PROJECT_BINARY_DIR}/${BINARY}" VERBATIM)
                
add_custom_target(test-random-quick DEPENDS ${BINARY}
                 WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}            
                 COMMAND "./mini-scripts/testallconstraints.py"
                    "--numtests=25" "--procs=2"
                    "--minion=${PROJECT_BINARY_DIR}/${BINARY}" VERBATIM)
                
add_custom_target(test-random-slow DEPENDS ${BINARY}
                  WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
                  COMMAND "./mini-scripts/testallconstraints.py"
                    "--numtests=160" "--procs=2"
                    "--minion=${PROJECT_BINARY_DIR}/${BINARY}" VERBATIM)

add_custom_target(test-resume DEPENDS ${BINARY}
                  WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
		  COMMAND "./mini-scripts/test-resume.sh"
		     "${PROJECT_BINARY_DIR}/${BINARY}" VERBATIM)

add_custom_target(test-quick)
add_dependencies(test-quick test-instances-quick)
add_custom_target(test)
add_dependencies(test test-instances-slow test-random-quick test-resume)
add_custom_target(test-slow)
add_dependencies(test-slow test-instances-slow test-random-slow test-resume)
