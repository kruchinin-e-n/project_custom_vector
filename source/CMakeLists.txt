cmake_minimum_required(
	VERSION 
	3.11
)
project(
	project_advanced_vector 
	CXX
)
set( 
	CMAKE_CXX_STANDARD 
	17
)
set(
	SOURCE_DIR 
	src
)
set(
	FILES_MAIN 
	"${SOURCE_DIR}/main.cpp"
)
set(
	FILES_VECTOR
	"${SOURCE_DIR}/vector.h" 
)
add_executable(
	advanced_vector
	${FILES_MAIN}
	${FILES_VECTOR}

)
source_group(
	"Main"
	FILES ${FILES_MAIN}
)
source_group(
	"Vector"
	FILES ${FILES_VECTOR}
)

if(CMAKE_SYSTEM_NAME MATCHES "^MINGW")
	set(
		SYSTEM_LIBS 
		-lstdc++
	)
else()
	set(
		SYSTEM_LIBS
	)
endif()

target_link_libraries(
	advanced_vector
	${SYSTEM_LIBS}
)