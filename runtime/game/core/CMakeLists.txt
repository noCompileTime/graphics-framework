#=======================================================================================================================
target_link_libraries(${PROJECT_NAME} PRIVATE common-module graphics-module platforms-module resources-module)
       target_sources(${PROJECT_NAME} PRIVATE main.cpp)
#=======================================================================================================================