cmake_minimum_required(VERSION 3.5)

project(fcgi-client)

############################################################
# Create a library
############################################################

#Generate the shared library from the library sources
add_library(${PROJECT_NAME} SHARED
    src/DefaultLogger.cpp
)

target_include_directories(${PROJECT_NAME}
    PUBLIC 
        ${PROJECT_SOURCE_DIR}/inc
)

############################################################
# Install
############################################################

# Library
install (TARGETS ${PROJECT_NAME}
    LIBRARY DESTINATION lib)

# Header files
install(DIRECTORY ${PROJECT_SOURCE_DIR}/inc/ 
    DESTINATION inc)
