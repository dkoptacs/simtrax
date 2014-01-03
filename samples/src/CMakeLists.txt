# all of the TRAX projects we want to compile go here

# try scanning all directories here such that they include CMakeLists.txt
# this way we can automatically find all of our projects within without modifying
# this file. Might need to make a recurivse search...
file(GLOB test "${CMAKE_CURRENT_SOURCE_DIR}/*/CMakeLists.txt")
foreach(_folder ${test})
	get_filename_component(folderToAppend ${_folder} PATH)
	add_subdirectory(${folderToAppend})
endforeach()
