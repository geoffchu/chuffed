cmake_minimum_required (VERSION 2.8.7)

if (USE_STDLIBCPP)
  set(LIBCPP libstdc++)
else()
  set(LIBCPP libc++)
endif()

if(APPLE)
   SET(CMAKE_OSX_DEPLOYMENT_TARGET "10.7")
   set(CMAKE_XCODE_ATTRIBUTE_CLANG_CXX_LANGUAGE_STANDARD "c++0x")
   set(CMAKE_XCODE_ATTRIBUTE_CLANG_CXX_LIBRARY "${LIBCPP}")
endif(APPLE)

project (chuffed CXX)

# The version number.
set (chuffed_VERSION_MAJOR 0)
set (chuffed_VERSION_MINOR 9)
set (chuffed_VERSION_PATCH 0)

include_directories("chuffed")
include_directories(".")

SET_SOURCE_FILES_PROPERTIES(
chuffed/vars/int-var.c
chuffed/vars/int-var-el.c
chuffed/vars/modelling.c
chuffed/vars/int-var-sl.c
chuffed/vars/bool-view.c
chuffed/vars/int-var-ll.c
chuffed/parallel/master.c
chuffed/parallel/slave.c
chuffed/ldsb/ldsb.c
chuffed/globals/mddglobals.c
chuffed/globals/sym-break.c
chuffed/globals/linear-bool.c
chuffed/globals/well-founded.c
chuffed/globals/circuit.c
chuffed/globals/minimum.c
chuffed/globals/alldiff.c
chuffed/globals/template.c
chuffed/globals/directives.c
chuffed/globals/cumulative.c
chuffed/globals/disjunctive.c
chuffed/globals/regular.c
chuffed/globals/lex.c
chuffed/globals/table.c
chuffed/mdd/MDD.c
chuffed/mdd/mdd_prop.c
chuffed/mip/mip.c
chuffed/mip/recalc.c
chuffed/mip/simplex.c
chuffed/primitives/element.c
chuffed/primitives/bool.c
chuffed/primitives/linear.c
chuffed/primitives/arithmetic.c
chuffed/primitives/binary.c
chuffed/flatzinc/parser.tab.c
chuffed/flatzinc/registry.c
chuffed/flatzinc/flatzinc.c
chuffed/flatzinc/lexer.yy.c
chuffed/branching/branching.c
chuffed/core/init.c
chuffed/core/stats.c
chuffed/core/engine.c
chuffed/core/options.c
chuffed/core/sat.c
chuffed/core/conflict.c
chuffed/flatzinc/fzn_chuffed.c
PROPERTIES LANGUAGE CXX )

add_library(chuffed
chuffed/vars/int-var.c
chuffed/vars/int-var-el.c
chuffed/vars/modelling.c
chuffed/vars/int-var-sl.c
chuffed/vars/bool-view.c
chuffed/vars/int-var-ll.c
chuffed/parallel/master.c
chuffed/parallel/slave.c
chuffed/ldsb/ldsb.c
chuffed/globals/mddglobals.c
chuffed/globals/sym-break.c
chuffed/globals/linear-bool.c
chuffed/globals/well-founded.c
chuffed/globals/circuit.c
chuffed/globals/minimum.c
chuffed/globals/alldiff.c
chuffed/globals/template.c
chuffed/globals/directives.c
chuffed/globals/cumulative.c
chuffed/globals/disjunctive.c
chuffed/globals/regular.c
chuffed/globals/lex.c
chuffed/globals/table.c
chuffed/mdd/MDD.c
chuffed/mdd/mdd_prop.c
chuffed/mip/mip.c
chuffed/mip/recalc.c
chuffed/mip/simplex.c
chuffed/primitives/element.c
chuffed/primitives/bool.c
chuffed/primitives/linear.c
chuffed/primitives/arithmetic.c
chuffed/primitives/binary.c
chuffed/branching/branching.c
chuffed/core/init.c
chuffed/core/stats.c
chuffed/core/engine.c
chuffed/core/options.c
chuffed/core/sat.c
chuffed/core/conflict.c
chuffed/vars/int-var.h
chuffed/vars/vars.h
chuffed/vars/int-var-sl.h
chuffed/vars/int-var-ll.h
chuffed/vars/int-view.h
chuffed/vars/int-var-el.h
chuffed/vars/modelling.h
chuffed/vars/bool-view.h
chuffed/parallel/parallel.h
chuffed/support/vec.h
chuffed/support/misc.h
chuffed/support/heap.h
chuffed/support/sparse_set.h
chuffed/ldsb/ldsb.h
chuffed/globals/globals.h
chuffed/globals/mddglobals.h
chuffed/mdd/opts.h
chuffed/mdd/MDD.h
chuffed/mdd/mdd_prop.h
chuffed/mip/mip.h
chuffed/mip/simplex.h
chuffed/primitives/primitives.h
chuffed/branching/branching.h
chuffed/core/sat.h
chuffed/core/engine.h
chuffed/core/sat-types.h
chuffed/core/options.h
chuffed/core/propagator.h
)

add_library(chuffed_fzn
chuffed/flatzinc/parser.tab.c
chuffed/flatzinc/registry.c
chuffed/flatzinc/flatzinc.c
chuffed/flatzinc/lexer.yy.c
chuffed/flatzinc/parser.tab.h
chuffed/flatzinc/flatzinc.h
chuffed/flatzinc/ast.h
)

add_executable(fzn_chuffed chuffed/flatzinc/fzn_chuffed.c)
target_link_libraries(fzn_chuffed chuffed_fzn chuffed)

INSTALL(TARGETS fzn_chuffed chuffed
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
)

MACRO(INSTALL_HEADERS_WITH_DIRECTORY HEADER_LIST)

    FOREACH(HEADER ${${HEADER_LIST}})
        STRING(REGEX MATCH "(.\\\*)\\\[/\\\]" DIR ${HEADER})
        INSTALL(FILES ${HEADER} DESTINATION include/${DIR})
    ENDFOREACH(HEADER)

ENDMACRO(INSTALL_HEADERS_WITH_DIRECTORY)

INSTALL_HEADERS_WITH_DIRECTORY(
chuffed/vars/int-var.h
chuffed/vars/vars.h
chuffed/vars/int-var-sl.h
chuffed/vars/int-var-ll.h
chuffed/vars/int-view.h
chuffed/vars/int-var-el.h
chuffed/vars/modelling.h
chuffed/vars/bool-view.h
chuffed/parallel/parallel.h
chuffed/support/vec.h
chuffed/support/misc.h
chuffed/support/heap.h
chuffed/support/sparse_set.h
chuffed/ldsb/ldsb.h
chuffed/globals/globals.h
chuffed/globals/mddglobals.h
chuffed/mdd/opts.h
chuffed/mdd/MDD.h
chuffed/mdd/mdd_prop.h
chuffed/mip/mip.h
chuffed/mip/simplex.h
chuffed/primitives/primitives.h
chuffed/branching/branching.h
chuffed/core/sat.h
chuffed/core/engine.h
chuffed/core/sat-types.h
chuffed/core/options.h
chuffed/core/propagator.h
)

INSTALL(DIRECTORY chuffed DESTINATION include
        PATTERN *.c EXCLUDE)