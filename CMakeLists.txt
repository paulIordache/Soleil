cmake_minimum_required(VERSION 3.20)
project(vulkan_engine)

set(CMAKE_CXX_STANDARD 20)

set(VULKAN_SDK "C:/VulkanSDK/1.4.321.1")
set(VULKAN_SDK_LIB_DIR "${VULKAN_SDK}/Lib")

set(IMGUI_DIR "${CMAKE_SOURCE_DIR}/3rd_party/ImGui/GLFW")
set(IMGUI_GIZMO_DIR "${CMAKE_SOURCE_DIR}/3rd_party/ImGui/imGuIZMO.quat")

set(IMGUI_SOURCES
        ${IMGUI_DIR}/imgui.cpp
        ${IMGUI_DIR}/imgui_draw.cpp
        ${IMGUI_DIR}/imgui_widgets.cpp
        ${IMGUI_DIR}/imgui_tables.cpp
        ${IMGUI_DIR}/imgui_impl_glfw.cpp
        ${IMGUI_DIR}/imgui_impl_vulkan.cpp
        ${IMGUI_GIZMO_DIR}/imGuIZMOquat.cpp
        core/src/svgf_pipeline.cpp
        core/include/svgf_pipeline.h
        core/include/vulkan_pipeline_base.h
)

include_directories(
        ${VULKAN_SDK}/Include
        ${PROJECT_SOURCE_DIR}/core/include
        ${PROJECT_SOURCE_DIR}/common/include
        3rd_party/glfw-3.4/include
        3rd_party/include
        3rd_party/glm-1.0.2
#        3rd_party/meshoptimizer/src
        3rd_party/gli
        3rd_party/ImGui/GLFW
        3rd_party/ImGui/imGuIZMO.quat
        3rd_party/ImGui/
)

link_directories(
        ${VULKAN_SDK_LIB_DIR}
        3rd_party/glfw-3.4/lib-vc2022
)

file(GLOB CORE_SOURCES "core/src/*.cpp")
file(GLOB COMMON_SOURCES "common/src/*.cpp")
file(COPY ${CMAKE_SOURCE_DIR}/shaders DESTINATION ${CMAKE_BINARY_DIR})

add_subdirectory(3rd_party/assimp-6.0.2)
add_subdirectory(3rd_party/meshoptimizer)

add_executable(vulkan_engine
        main.cpp
        ${CORE_SOURCES}
        ${COMMON_SOURCES}
        ${IMGUI_SOURCES}
)

find_package(Vulkan REQUIRED)
find_package(GLEW REQUIRED)

set(GLSLANG_LIBS_DEBUG
        "${VULKAN_SDK_LIB_DIR}/glslangd.lib"
        "${VULKAN_SDK_LIB_DIR}/OSDependentd.lib"
        "${VULKAN_SDK_LIB_DIR}/GenericCodeGend.lib"
        "${VULKAN_SDK_LIB_DIR}/MachineIndependentd.lib"
        "${VULKAN_SDK_LIB_DIR}/glslang-default-resource-limitsd.lib"
        "${VULKAN_SDK_LIB_DIR}/SPIRVd.lib"
        "${VULKAN_SDK_LIB_DIR}/SPIRV-Toolsd.lib"
        "${VULKAN_SDK_LIB_DIR}/SPIRV-Tools-diffd.lib"
        "${VULKAN_SDK_LIB_DIR}/SPIRV-Tools-optd.lib"
)

set(GLSLANG_LIBS_RELEASE
        "${VULKAN_SDK_LIB_DIR}/glslang.lib"
        "${VULKAN_SDK_LIB_DIR}/OSDependent.lib"
        "${VULKAN_SDK_LIB_DIR}/GenericCodeGen.lib"
        "${VULKAN_SDK_LIB_DIR}/MachineIndependent.lib"
        "${VULKAN_SDK_LIB_DIR}/glslang-default-resource-limits.lib"
        "${VULKAN_SDK_LIB_DIR}/SPIRV.lib"
        "${VULKAN_SDK_LIB_DIR}/SPIRV-Tools.lib"
        "${VULKAN_SDK_LIB_DIR}/SPIRV-Tools-diff.lib"
        "${VULKAN_SDK_LIB_DIR}/SPIRV-Tools-opt.lib"
)

target_link_libraries(vulkan_engine
        PRIVATE
        glfw3
        vulkan-1
        $<$<CONFIG:Debug>:${GLSLANG_LIBS_DEBUG}>
        #   $<$<CONFIG:Release>:${GLSLANG_LIBS_RELEASE}>
        #   $<$<CONFIG:RelWithDebInfo>:${GLSLANG_LIBS_RELEASE}>
        #   $<$<CONFIG:MinSizeRel>:${GLSLANG_LIBS_RELEASE}>
        assimp
        GLEW::GLEW
        meshoptimizer
)

