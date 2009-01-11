cmake_minimum_required(VERSION 2.4.7)

set(CMAKE_MODULE_PATH cmake-modules)
include(CheckIncludeFileCXX)
include(CheckCXXCompilerFlag)
include(FindBoost)
include(FindGMP)
include(constraints)

set(CMAKE_ALLOW_LOOSE_LOOP_CONSTRUCTS true)

project(minion)
set(EXECUTABLE_OUTPUT_PATH "${PROJECT_SOURCE_DIR}/bin")
set(LIBRARY_OUTPUT_PATH "${PROJECT_SOURCE_DIR}/bin/libraries")
# determine svn version and date
find_program(SVN svn)
if(SVN)
    execute_process(COMMAND ${SVN} info
            WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
            RESULT_VARIABLE SVNINFO_STATUS
            OUTPUT_VARIABLE SVNINFO OUTPUT_STRIP_TRAILING_WHITESPACE)
    if(SVNINFO_STATUS EQUAL 0)
        string(REGEX MATCH "Last Changed Rev: [0-9]+" SVNVERSION "${SVNINFO}")
        string(REPLACE "Last Changed Rev: " "" SVNVERSION ${SVNVERSION})
        string(REGEX MATCH "Last Changed Date: .+" SVNDATE "${SVNINFO}")
        string(REPLACE "Last Changed Date: " "" SVNDATE ${SVNDATE})
        message(STATUS "SVN version ${SVNVERSION}, date ${SVNDATE}")
    else()
        # try git
        find_program(GIT git-svn)
        if(GIT)
            execute_process(COMMAND ${GIT} info
                    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
                    RESULT_VARIABLE SVNINFO_STATUS
                    OUTPUT_VARIABLE SVNINFO OUTPUT_STRIP_TRAILING_WHITESPACE)
            if(SVNINFO_STATUS EQUAL 0)
                string(REGEX MATCH "Last Changed Rev: [0-9]+" SVNVERSION "${SVNINFO}")
                string(REPLACE "Last Changed Rev: " "" SVNVERSION ${SVNVERSION})
                string(REGEX MATCH "Last Changed Date: .+" SVNDATE "${SVNINFO}")
                string(REPLACE "Last Changed Date: " "" SVNDATE ${SVNDATE})
                message(STATUS "SVN version ${SVNVERSION}, date ${SVNDATE}")
            endif()
        else()
            message(STATUS "Unable to determine SVN version (no checkout?)")
            set(SVNVERSION "0")
            set(SVNDATE "unknown")
        endif()
    endif()
else()
    message(STATUS "No SVN executable found, unable to determine SVN version and date")
endif()
add_definitions(-DSVN_VER="${SVNVERSION}" -DSVN_DATE="\\\"${SVNDATE}\\\"")

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

file(GLOB SRCS "minion/build_constraints/CT*.cpp")
foreach(ct ${SRCS})
    string(REGEX MATCH "CT_[A-Z_0-9]+" ct_name ${ct})
    add_definitions(-D${ct_name}_ABC)
endforeach()
list(APPEND SRCS minion/BuildConstraint2.cpp
                 minion/BuildCSP.cpp
                 minion/commandline_parse.cpp
                 minion/constraint_setup.cpp
                 minion/debug_functions.cpp
                 minion/get_info.cpp
                 minion/inputfile_parse.cpp
#                minion/minion.cpp
                 minion/globals.cpp
                 minion/preprocess.cpp)
list(APPEND SRCS minion/build_constraints/BuildStaticStart.cpp)
list(APPEND SRCS minion/system/trigger_timer.cpp)
list(APPEND SRCS minion/help/help.cpp)

set(BINARY ${PROJECT_NAME})

find_program(UNAME uname)
if(UNAME)
    execute_process(COMMAND ${UNAME}
            OUTPUT_VARIABLE UNAME_OUT OUTPUT_STRIP_TRAILING_WHITESPACE)
    if(UNAME_OUT STREQUAL "Darwin")
        set(APPLEMAC 1)
    endif()
endif()

find_program(CCACHE ccache)
if(CCACHE)
    message(STATUS "Using ccache")
    set(CMAKE_CXX_COMPILER_ARG1 ${CMAKE_CXX_COMPILER})
    set(CMAKE_CXX_COMPILER ${CCACHE})
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

find_package(Boost 1.33.0 COMPONENTS iostreams thread)
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
    message(FATAL_ERROR "Boost not found -- do you have both iostreams and thread components?")
endif()

#find_package(GMP)
#if(GMP_FOUND)
#    set(CFLAGS "${CFLAGS} -I${GMP_INCLUDE_DIR}")
#    message(STATUS "GMP found")
#    add_definitions(-DUSE_GMP)
#else()
#    message(STATUS "GMP not found, no bounds checking")
#endif()

#check_cxx_compiler_flag("-std=c++0x" CheckForCXX)
#if(CheckForCXX)
#   message(STATUS "Compiler supports C++0x!")
#   add_definitions("-std=c++0x")
#else()
#   message(STATUS "Compiler does not support C++0x")
#endif()

check_cxx_compiler_flag("-W -Wall -Wno-sign-compare -Wno-missing-braces -Wno-unused-parameter" CheckWarnings)
if(CheckWarnings)
    add_definitions("-W -Wall -Wno-sign-compare -Wno-missing-braces -Wno-unused-parameter")
endif()

# slightly weird check to make sure that the boost TR1 libs are there

if(Boost_FOUND)
    set(CMAKE_REQUIRED_INCLUDES ${CMAKE_REQUIRED_INCLUDES} ${Boost_INCLUDE_DIRS})
    check_include_file_cxx("boost/tr1/array.hpp" BOOST-TR1)
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

option(SLOW_DEBUG "" OFF)
if(SLOW_DEBUG)
    message(STATUS "Enabling slow DEBUG")
    set(BINARY ${BINARY}-debugextra)
    add_definitions(-DSLOW_DEBUG)
    set(DEBUG "TRUE")
endif()

option(DEBUG "" OFF)
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

option(PROFILE "" OFF)
if(PROFILE)
    message(STATUS "Enabling profiling")
    set(BINARY "${BINARY}-profile")
    set(CFLAGS "${CFLAGS} -O -g -fno-inline -fno-inline-functions")
    set(UNOPTIMISED ON) # Profiing can't cope with -fomit-frame-pointer
endif()

option(UNOPTIMISED "" OFF)
if(UNOPTIMISED)
    message(STATUS "Building unoptimised binary")
    set(BINARY "${BINARY}-unoptimised")
    set(CFLAGS "${CFLAGS} -g")
else()
    # apparently fomit-frame-pointer is only automatically enabled for O* levels
    # on machines where this doesn't affect debugging
    set(CFLAGS "${CFLAGS} -O3 -fomit-frame-pointer")
    check_cxx_compiler_flag("-mdynamic-no-pic" MAC_MDYNAMIC)
    if(MAC_MDYNAMIC)
        set(CFLAGS "${CFLAGS} -mdynamic-no-pic")
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

option(BACK_VEC "" OFF)
option(CACHE_MALLOC "" OFF)
if(BACK_VEC)
    message(STATUS "Enabling backtrack memory in vector")
    set(BINARY "${BINARY}-backtrack-vec")
    add_definitions(-DBACKTRACK_VEC)
    if(CACHE_MALLOC)
        message(STATUS "Enabling Caching Malloc")
        add_definitions(-DMALLOC_CACHE)
    endif()
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

add_library(${BINARY}-lib STATIC EXCLUDE_FROM_ALL ${SRCS})

add_executable(${BINARY} ${SRCS} minion/minion.cpp)
#target_link_libraries(${BINARY} ${BINARY}-lib)

# when we're building minion-something, add a "minion" target that does the same
if(NOT ${BINARY} STREQUAL ${PROJECT_NAME})
    add_custom_target(${PROJECT_NAME} DEPENDS ${BINARY})
    add_custom_target(${PROJECT_NAME}-lib DEPENDS ${BINARY}-lib)
endif()

if(Boost_FOUND)
    target_link_libraries(${BINARY} "-L${Boost_LIBRARY_DIRS}" ${Boost_LIBRARIES} ${ZLIB_LIBRARIES} ${BZIP2_LIBRARIES})
endif()

if(GMP_FOUND)
    target_link_libraries(${BINARY} ${GMPXX_LIBRARY} ${GMP_LIBRARY})
endif()

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
        WORKING_DIRECTORY ${EXECUTABLE_OUTPUT_PATH}
        COMMENT "minion-helper" VERBATIM)
    add_custom_target(minion-sat ${CLISP} ${CLISP_FLAGS}
        "(clisp-make-executable \"minion-sat\" (function clisp-toplevel-sat))"
        -i "${PROJECT_SOURCE_DIR}/generators/MinionHelper.lsp"
        -i "${PROJECT_SOURCE_DIR}/generators/SAT/MinionDimacsSAT.lsp"
        WORKING_DIRECTORY ${EXECUTABLE_OUTPUT_PATH}
        COMMENT "minion-sat" VERBATIM)
    add_custom_target(minion-quasigroup ${CLISP} ${CLISP_FLAGS}
        "(clisp-make-executable \"minion-quasigroup\" (function clisp-toplevel-quasigroup))"
        -i "${PROJECT_SOURCE_DIR}/generators/MinionHelper.lsp"
        -i "${PROJECT_SOURCE_DIR}/generators/Quasigroup/MinionQuasigroup.lsp"
        WORKING_DIRECTORY ${EXECUTABLE_OUTPUT_PATH}
        COMMENT "minion-quasigroup" VERBATIM)
    add_custom_target(lisp-generate WORKING_DIRECTORY ${EXECUTABLE_OUTPUT_PATH})
    # XXX: adding the dependencies directly to the target causes it to fail
    # if the current dir != EXECUTABLE_OUTPUT_PATH
    add_dependencies(lisp-generate minion-helper minion-sat minion-quasigroup)
    set_directory_properties(PROPERTY ADDITIONAL_MAKE_CLEAN_FILES
        "${EXECUTABLE_OUTPUT_PATH}/minion-helper;${EXECUTABLE_OUTPUT_PATH}/minion-sat;${EXECUTABLE_OUTPUT_PATH}/minion-quasigroup")
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
