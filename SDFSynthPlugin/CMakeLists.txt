cmake_minimum_required(VERSION 3.15)
project(SDFSynthPlugin VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# =============================================================================
# JUCE Dependency (offline-friendly: local folder > env var > FetchContent)
# =============================================================================
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/JUCE/CMakeLists.txt")
    message(STATUS "Using local JUCE from ${CMAKE_CURRENT_SOURCE_DIR}/JUCE")
    add_subdirectory(JUCE)
elseif(DEFINED ENV{JUCE_DIR})
    message(STATUS "Using JUCE from JUCE_DIR: $ENV{JUCE_DIR}")
    add_subdirectory($ENV{JUCE_DIR} JUCE)
elseif(DEFINED JUCE_DIR)
    message(STATUS "Using JUCE from JUCE_DIR: ${JUCE_DIR}")
    add_subdirectory(${JUCE_DIR} JUCE)
else()
    message(STATUS "Fetching JUCE via FetchContent (one-time download)...")
    include(FetchContent)
    FetchContent_Declare(
        JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG        8.0.4
        GIT_SHALLOW    TRUE
    )
    FetchContent_MakeAvailable(JUCE)
endif()

# =============================================================================
# Platform-specific plugin formats
# =============================================================================
set(PLUGIN_FORMATS VST3 Standalone)
if(APPLE)
    list(APPEND PLUGIN_FORMATS AU)
endif()

# =============================================================================
# Plugin Target
# =============================================================================
juce_add_plugin(SDFSynthPlugin
    COMPANY_NAME          "UNIX Audio"
    COMPANY_WEBSITE       "https://github.com/your-username/sdf-synth"
    PLUGIN_MANUFACTURER_CODE Unix
    PLUGIN_CODE           SdSy

    IS_SYNTH              TRUE
    NEEDS_MIDI_INPUT      TRUE
    NEEDS_MIDI_OUTPUT     FALSE
    IS_MIDI_EFFECT        FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    COPY_PLUGIN_AFTER_BUILD FALSE

    FORMATS               ${PLUGIN_FORMATS}
    PRODUCT_NAME          "SDF Synth"

    BUNDLE_ID             "com.unixaudio.sdfsynth"
    HARDENED_RUNTIME_ENABLED TRUE
    VST3_CATEGORIES       Instrument Synth
)

# =============================================================================
# Source Files
# =============================================================================
target_sources(SDFSynthPlugin
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
        Source/DSP/SDFScene3D.cpp
        Source/DSP/ContourExtractor.cpp
        Source/DSP/WavetableGenerator.cpp
        Source/DSP/SDFVoice.cpp
        Source/DSP/SDFSynthesiser.cpp
        Source/DSP/MeshImporter.cpp
        Source/GUI/SDFLookAndFeel.cpp
        Source/GUI/ArcKnob.cpp
        Source/GUI/ADSRDisplay.cpp
        Source/GUI/ShapeSelector.cpp
        Source/GUI/WaveformScope.cpp
        Source/GUI/SDFViewport3D.cpp
        Source/GUI/TextureMenu.cpp
        Source/GUI/SkyboxSystem.cpp
        Source/Texture/TextureSystem.cpp
        Source/Texture/ProceduralLibrary.cpp
        Source/Utility/PresetManager.cpp
)

target_include_directories(SDFSynthPlugin
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/Source
        ${CMAKE_CURRENT_SOURCE_DIR}/Source/DSP
        ${CMAKE_CURRENT_SOURCE_DIR}/Source/GUI
        ${CMAKE_CURRENT_SOURCE_DIR}/Source/Texture
        ${CMAKE_CURRENT_SOURCE_DIR}/Source/Utility
)

# =============================================================================
# Compile Definitions
# =============================================================================
target_compile_definitions(SDFSynthPlugin
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_DISPLAY_SPLASH_SCREEN=0
        JUCE_REPORT_APP_USAGE=0
        JUCE_MODAL_LOOPS_PERMITTED=1
        JUCE_OPENGL=1
)

# =============================================================================
# Platform-specific compiler settings
# =============================================================================
if(MSVC)
    target_compile_options(SDFSynthPlugin PRIVATE
        /W4 /permissive- /Zc:__cplusplus /MP
    )
    target_compile_definitions(SDFSynthPlugin PRIVATE
        _CRT_SECURE_NO_WARNINGS NOMINMAX
    )
elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(SDFSynthPlugin PRIVATE
        -Wall -Wextra -Wpedantic -Wno-unused-parameter
    )
endif()

# =============================================================================
# Linux dependencies
# =============================================================================
if(UNIX AND NOT APPLE)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(ALSA REQUIRED alsa)
    pkg_check_modules(FREETYPE2 REQUIRED freetype2)
    find_package(X11 REQUIRED)
    find_package(OpenGL REQUIRED)

    target_link_libraries(SDFSynthPlugin
        PRIVATE
            ${ALSA_LIBRARIES}
            ${FREETYPE2_LIBRARIES}
            ${X11_LIBRARIES}
            OpenGL::GL
            pthread dl
    )
    if(X11_Xrandr_FOUND)
        target_link_libraries(SDFSynthPlugin PRIVATE ${X11_Xrandr_LIB})
    endif()
    if(X11_Xinerama_FOUND)
        target_link_libraries(SDFSynthPlugin PRIVATE ${X11_Xinerama_LIB})
    endif()
    if(X11_Xcursor_FOUND)
        target_link_libraries(SDFSynthPlugin PRIVATE ${X11_Xcursor_LIB})
    endif()
endif()

# =============================================================================
# Link Libraries
# =============================================================================
target_link_libraries(SDFSynthPlugin
    PRIVATE
        juce::juce_audio_utils
        juce::juce_audio_processors
        juce::juce_dsp
        juce::juce_opengl
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

# =============================================================================
# Unit Test Target
# =============================================================================
juce_add_console_app(SDFSynthTests
    PRODUCT_NAME "SDF Synth Tests"
)

target_sources(SDFSynthTests
    PRIVATE
        Tests/TestMain.cpp
        Tests/TestSVFFilter.cpp
        Tests/TestSDFOscillator.cpp
        Tests/TestWavetableGenerator.cpp
        Tests/TestContourExtractor.cpp
        Tests/TestThreadSafeSwap.cpp
        Tests/TestNoiseGenerator.cpp
        Tests/TestOscEffects.cpp
        Tests/TestOscB.cpp
        Tests/TestUnison.cpp
        Tests/TestFilterModes.cpp
        Tests/TestPhase2Integration.cpp
        Tests/TestPresetData.cpp
        Tests/TestVoxelSDF.cpp
        Tests/TestMeshImporter.cpp
        Tests/TestLFO.cpp
        Tests/TestModulationMatrix.cpp
        Tests/TestFXChain.cpp
        Tests/TestClickDetection.cpp
        Source/DSP/SDFScene3D.cpp
        Source/DSP/ContourExtractor.cpp
        Source/DSP/WavetableGenerator.cpp
        Source/DSP/SDFVoice.cpp
        Source/DSP/SDFSynthesiser.cpp
        Source/DSP/MeshImporter.cpp
        Source/Utility/PresetManager.cpp
        Source/Texture/ProceduralLibrary.cpp
)

target_include_directories(SDFSynthTests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/Source
        ${CMAKE_CURRENT_SOURCE_DIR}/Source/DSP
        ${CMAKE_CURRENT_SOURCE_DIR}/Source/GUI
        ${CMAKE_CURRENT_SOURCE_DIR}/Source/Texture
        ${CMAKE_CURRENT_SOURCE_DIR}/Source/Utility
)

target_compile_definitions(SDFSynthTests
    PRIVATE
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_DISPLAY_SPLASH_SCREEN=0
        JUCE_REPORT_APP_USAGE=0
        JUCE_UNIT_TESTS=1
)

if(MSVC)
    target_compile_options(SDFSynthTests PRIVATE /W4 /permissive- /Zc:__cplusplus /MP)
    target_compile_definitions(SDFSynthTests PRIVATE _CRT_SECURE_NO_WARNINGS NOMINMAX)
elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(SDFSynthTests PRIVATE -Wall -Wextra -Wpedantic -Wno-unused-parameter)
endif()

target_link_libraries(SDFSynthTests
    PRIVATE
        juce::juce_core
        juce::juce_audio_basics
        juce::juce_audio_processors
        juce::juce_graphics
        juce::juce_dsp
)

# =============================================================================
# Installation (VST3 to standard system paths)
# =============================================================================
if(APPLE)
    set(VST3_INSTALL_DIR "$ENV{HOME}/Library/Audio/Plug-Ins/VST3")
    set(AU_INSTALL_DIR "$ENV{HOME}/Library/Audio/Plug-Ins/Components")
elseif(UNIX)
    set(VST3_INSTALL_DIR "$ENV{HOME}/.vst3")
elseif(WIN32)
    set(VST3_INSTALL_DIR "$ENV{COMMONPROGRAMFILES}/VST3")
endif()

add_custom_target(install_plugin
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "$<TARGET_FILE_DIR:SDFSynthPlugin_VST3>/../"
        "${VST3_INSTALL_DIR}/SDF Synth.vst3"
    DEPENDS SDFSynthPlugin_VST3
    COMMENT "Installing VST3 to ${VST3_INSTALL_DIR}"
)

# =============================================================================
# Summary
# =============================================================================
message(STATUS "")
message(STATUS "SDF Synth Plugin Configuration:")
message(STATUS "  Version:    ${PROJECT_VERSION}")
message(STATUS "  Formats:    ${PLUGIN_FORMATS}")
message(STATUS "  Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  Compiler:   ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "  Platform:   ${CMAKE_SYSTEM_NAME}")
message(STATUS "")
