cmake_minimum_required (VERSION 3.10)
project(mclib)

set(SOURCES ${SOURCES}
    alphapalette.cpp
    bitflag.cpp
    cident.cpp
    cmponent.cpp
    color.cpp
    debugging.cpp
    err.cpp
    floathelp.cpp
    heap.cpp
    lzcomp.cpp
    lzdecomp.cpp
    mathfunc.cpp
    mouse.cpp
    msl.cpp
    paths.cpp
    pqueue.cpp
    routines.cpp
    scale.cpp
    sortlist.cpp
    tgainfo.cpp
    tgl.cpp
    timing.cpp
    txmmgr.cpp
    userinput.cpp
    vport.cpp
    weaponfx.cpp
    csvfile.cpp
    fastfile.cpp
    ffile.cpp
    file.cpp
    gamelog.cpp
    inifile.cpp
    packet.cpp
    celement.cpp
    celine.cpp
    cellip.cpp
    cepane.cpp
    cevfx.cpp
    vfx_ellipse.cpp
    vfx_map_polygon.cpp
    vfx_transform.cpp
    vfx_translatedraw.cpp
    vfxtile.cpp
    abldbug.cpp
    abldecl.cpp
    ablenv.cpp
    ablerr.cpp
    ablexec.cpp
    ablexpr.cpp
    ablrtn.cpp
    ablscan.cpp
    ablstd.cpp
    ablstmt.cpp
    ablsymt.cpp
    ablxexpr.cpp
    ablxstd.cpp
    ablxstmt.cpp
    camera.cpp
    clouds.cpp
    crater.cpp
    mapdata.cpp
    quad.cpp
    terrain.cpp
    terrtxm.cpp
    terrtxm2.cpp
    txmconv.cpp
    appear.cpp
    apprtype.cpp
    bdactor.cpp
    genactor.cpp
    gvactor.cpp
    mech3d.cpp
    soundsys.cpp
    move.cpp
    echarstring.cpp
    estring.cpp
    ewcharstring.cpp
    tacmap.cpp
    utilities.cpp
    )

include_directories(${COMMON_INCLUDE_DIRS} "./mclib" "../Editor" "../DirectX8/include" "./")
include_directories(${THIRDPARTY_INCLUDE_DIRS})

add_definitions(-DBGR)
add_library(mclib ${SOURCES})

if(BUILD_TESTING)
	add_executable(msl_texture_regression msl_texture_regression.cpp)
	target_link_libraries(msl_texture_regression mclib gosfx mlr stuff gui gameos gameos_main windows ZLIB::ZLIB ${SDL2_LIBRARIES} GLEW::GLEW SDL2_mixer::SDL2_mixer ${ADDITIONAL_LIBS} OpenGL::GL)
	add_test(NAME msl_texture_regression COMMAND msl_texture_regression)
endif()

# cat MCLib.vcproj | grep ".cpp" | perl -pe 's/.+\"(\w+\.cpp)\".+/\1/g' >> CMakeLists.txt

