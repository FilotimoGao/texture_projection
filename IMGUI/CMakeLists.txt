file(GLOB_RECURSE IMGUI ./ *.cpp)
include_directories(
	SYSTEM
	../thirdParty/include/IMGUI
	../thirdParty/include
)
add_library(imgui ${IMGUI})