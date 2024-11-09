file(GLOB_RECURSE Application ./ *.cpp)
include_directories(
	SYSTEM
	../thirdParty/include/IMGUI
	../thirdParty/include
)
add_library(app ${Application})