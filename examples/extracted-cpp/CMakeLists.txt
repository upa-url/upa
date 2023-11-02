# Examples to build
file(GLOB example_sources *.cpp)

foreach(source ${example_sources})
  get_filename_component(exe_name ${source} NAME_WE)
  add_executable(${exe_name} ${source})
  target_link_libraries(${exe_name} PRIVATE upa::url)
endforeach()
