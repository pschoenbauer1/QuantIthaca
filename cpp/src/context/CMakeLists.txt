add_library(context SHARED
  graph.cpp
)

# Allow consumers to include "threads.h" when they include from this library.
target_include_directories(context PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(context PUBLIC utils)

if(WIN32)
  # Ensure an import library is generated even without explicit
  # __declspec(dllexport) annotations on individual symbols.
  set_target_properties(context PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS ON)
endif()

