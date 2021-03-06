#
# CMakeLists.txt for ./tests folder of MUGIQ
#
# C. Kallidonis, William & Mary
# Dec. 2019

if (MUGIQ_BUILD_ALL_TESTS)
  # Create another library that contains dependency files for tests
  set(MUGIQ_TEST_DEPS test_params_mugiq.cpp)
  add_library(mugiq_deps STATIC ${MUGIQ_TEST_DEPS})
  
  # Link with libmugiq.a
  set(EXE_LIBS mugiq mugiq_deps ${LIB_QUDATEST})
  
  # Macro to exclude test for building, if NOT building all tests
  macro(MUGIQ_CHECKTEST TEST_TARGET MUGIQ_BUILD_ALL_TESTS_BOOL)
    if(NOT ${MUGIQ_BUILD_ALL_TESTS_BOOL})
      set_property(TARGET ${TEST_TARGET} PROPERTY EXCLUDE_FROM_ALL 1)
    endif()
  endmacro()
  
  
  # Test targets
  
  add_executable(eigensolve eigensolve.cpp)
  target_link_libraries(eigensolve ${EXE_LIBS})
  mugiq_checktest(eigensolve MUGIQ_BUILD_ALL_TESTS)
  
  add_executable(loop loop.cpp)
  target_link_libraries(loop ${EXE_LIBS})
  mugiq_checktest(loop MUGIQ_BUILD_ALL_TESTS)
endif()
