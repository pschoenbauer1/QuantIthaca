add_library(utils SHARED
  threads.cpp
)

# Allow consumers to include "threads.h" when they include from this library.
target_include_directories(utils PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
)

if(WIN32)
  # Ensure an import library is generated even without explicit
  # __declspec(dllexport) annotations on individual symbols.
  set_target_properties(utils PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS ON)
endif()

