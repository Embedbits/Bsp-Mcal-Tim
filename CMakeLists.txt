# Template version: 1.0.0
# Sets the version of CMakeLists.txt file.
# Never, under any circumstances, change this line. It is used by
# the module generation script to identify the template version.

#======================== Library configuration ===============================#
# Publicly included directories to provide headers include
set( Tim_PublicIncludeDirs
    
)


# Privately included directories to provide headers include
set( Tim_PrivateIncludeDirs
    ${CMAKE_CURRENT_SOURCE_DIR}
)


# Source files list for current library generation
set( Tim_SourceFiles
    Tim.c
)


# List of public header files provided by the current library
set( Tim_PublicHeaders 
    Tim_Types.h
    Tim_Port.h 
)


# List of dependent libraries that are publicly used by the current library
set( Tim_PublicDependentLibs
    Ral_Lib 
)


# List of dependent libraries that are privately used by the current library
set( Tim_PrivateDependentLibs
    Rcc_Lib
    Nvic_Lib
    Gpio_Lib
)


#========================== Library generation ================================#
# Create library
add_library(Tim_Lib STATIC ${Tim_SourceFiles})


# Include necessary libraries to the target
target_link_libraries( Tim_Lib
    PUBLIC 
        ${Tim_PublicDependentLibs} 
    PRIVATE 
        ${Tim_PrivateDependentLibs}
)


# Set public headers directory path in build folder
set(PUBLIC_HEADERS_DIR ${CMAKE_CURRENT_BINARY_DIR}/PublicHeaders)

# Generates the public headers directory in build folder
file(MAKE_DIRECTORY ${PUBLIC_HEADERS_DIR})

# Process all public headers
foreach(PublicHeader IN LISTS Tim_PublicHeaders)
    
    # Copy (or configure) only the public header there
    configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/${PublicHeader} 
        ${PUBLIC_HEADERS_DIR}/${PublicHeader}
        COPYONLY
    )
    
endforeach()


# Set include directories for the created target
target_include_directories( Tim_Lib
    PUBLIC
        $<BUILD_INTERFACE:${PUBLIC_HEADERS_DIR}>
        $<INSTALL_INTERFACE:include>
        Tim_PublicIncludeDirs
    PRIVATE
        ${Tim_IncludeDirs}
        Tim_PrivateIncludeDirs
)


# Print status message
message(STATUS "Tim_Lib library created")

#========================= Documentation configuration ========================#
if(DOXYGEN_AVAILABLE STREQUAL "ON")
    Doxygen_AddPath("${CMAKE_CURRENT_SOURCE_DIR}/")
endif()
