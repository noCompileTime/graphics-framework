#=======================================================================================================================
target_compile_definitions(${PROJECT_NAME} PRIVATE PROJECT_NAME="${PROJECT_NAME}")
#=======================================================================================================================
     target_link_libraries(${PROJECT_NAME} PRIVATE common_module platforms_module graphics_module resources_module tools_module)
            target_sources(${PROJECT_NAME} PRIVATE main.cpp object.cpp)
#=======================================================================================================================