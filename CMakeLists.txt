cmake_minimum_required(VERSION 2.4)

include(CheckIncludeFileCXX)
set(CMAKE_ALLOW_LOOSE_LOOP_CONSTRUCTS true)

project(minion)
set(EXECUTABLE_OUTPUT_PATH "${PROJECT_SOURCE_DIR}/bin")

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
		endif()
	endif()
else()
	message(STATUS "No SVN executable found, unable to determine SVN version and date")
endif()
add_definitions(-DSVN_VER="${SVNVERSION}" -DSVN_DATE="${SVNDATE}")

# if we have a bash, generate constraints and help if necessary and add build
# targets to do that
if(UNIX)
	find_program(BASH bash)
	if(BASH)
		add_custom_target(generate-constraints ${BASH} "rebuild_files.sh"
				  WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}/minion/build_constraints"
				  COMMENT "generate-constraints" VERBATIM)
		file(GLOB CONSTRAINTS "${PROJECT_SOURCE_DIR}/minion/build_constraints/CT_*.cpp")
		if(CONSTRAINTS)
			message(STATUS "Constraints exist, not regenerating")
		else()
			message(STATUS "Generating constraints")
			execute_process(COMMAND ${BASH} "rebuild_files.sh"
					WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}/minion/build_constraints")
			message(STATUS "Generating constraints - done")
		endif()

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
		message(STATUS "No bash executable found, not generating contraints and help")
	endif()
else()
	message(STATUS "You need a UNIX-like system to regenerate constraints and help")
endif()

aux_source_directory(minion SRCS)
aux_source_directory(minion/build_constraints SRCS)
aux_source_directory(minion/system SRCS)

set(BINARY ${PROJECT_NAME})
set(CFLAGS "-Wextra -Wno-sign-compare")

find_package(Boost 1.35 COMPONENTS iostreams)
if(Boost_FOUND)
	set(CFLAGS "${CFLAGS} -I${Boost_INCLUDE_DIR}")
	find_package(ZLIB)
	if(ZLIB_FOUND)
		find_package(BZip2)
		if(BZIP2_FOUND)
			message(STATUS "Boost and all required libraries found, building with boost")
			aux_source_directory("${PROJECT_SOURCE_DIR}/minion/boost_files" SRCS)
			add_definitions(-DUSE_BOOST)
		else()
			message(STATUS "Bzip2 not found. Building without boost")
		endif()
	else()
		message(STATUS "Zlib not found, building without boost")
	endif()
else()
	message(STATUS "Boost not found, building without boost")
endif()

check_include_file_cxx("tr1/unordered_map" TR1-MAP)
check_include_file_cxx("tr1/unordered_set" TR1-SET)
if(TR1-MAP AND TR1-SET)
	message(STATUS "Using hashed containers")
	add_definitions(-DUSE_TR1_HASH_MAP_AND_SET)
else()
	message(STATUS "Using tree containers")
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

option(UNOPTIMISED "" OFF)
if(UNOPTIMISED)
	message(STATUS "Building unoptimised binary")
	set(CFLAGS "${CFLAGS} -g")
else()
	set(CFLAGS "${CFLAGS} -O3")
endif()

option(PROFILE "" OFF)
if(PROFILE)
	message(STATUS "Enabling profiling")
	set(BINARY "${BINARY}-profile")
	set(CFLAGS "${CFLAGS} -g -fno-inline -fno-inline-functions")
endif()

option(SMALL "" OFF)
if(SMALL)
	message(STATUS "Building small binary")
	set(BINARY "${BINARY}-small")
	set(CFLAGS "${CFLAGS} -Os")
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

option(STATIC "" OFF)
if(STATIC)
	message(STATUS "Building static binary")
	set(BINARY "${BINARY}-static")
	set(CMAKE_EXE_LINKER_FLAGS "-static")
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

add_executable(${BINARY} ${SRCS})
# when we're building minion-something, add a "minion" target that does the same
if(NOT ${BINARY} STREQUAL ${PROJECT_NAME})
	add_custom_target(${PROJECT_NAME} DEPENDS ${BINARY})
endif()

if(Boost_FOUND)
	target_link_libraries(${BINARY} ${Boost_LIBRARIES} ${ZLIB_LIBRARIES} ${BZIP2_LIBRARIES})
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
