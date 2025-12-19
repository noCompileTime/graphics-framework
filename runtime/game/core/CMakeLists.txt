#=======================================================================================================================
target_link_libraries(${PROJECT_NAME} PRIVATE common_module graphics_module platforms_module resources_module)
       target_sources(${PROJECT_NAME} PRIVATE main.cpp)
#=======================================================================================================================