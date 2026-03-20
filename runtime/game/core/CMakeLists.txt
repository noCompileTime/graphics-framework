#=======================================================================================================================
target_link_libraries(${PROJECT_NAME} PRIVATE platforms_module graphics_module resources_module)
       target_sources(${PROJECT_NAME} PRIVATE object.cpp main.cpp)
#=======================================================================================================================