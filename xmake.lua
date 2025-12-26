set_project("benjamin")
set_version("0.0.1")

add_rules("mode.debug", "mode.release")
add_rules("plugin.compile_commands.autoupdate")

set_languages("c++23")

target("core_modules")
    set_kind("static")
    set_policy("build.c++.modules", true)
    add_files("core_modules/defines.ixx", { public = true })

target("gameplay")
    set_kind("shared")
    set_policy("build.c++.modules", true)
    add_deps("core_modules")
    add_files("gameplay/api.cpp")

target("engine")
    set_kind("binary")
    set_policy("build.c++.modules", false)
    add_files("engine/entry.cpp", "engine/window.cpp", "engine/renderer.cpp", "engine/pipeline.cpp", "engine/buffer.cpp", "engine/camera.cpp")
    add_headerfiles("engine/*.hpp")
    add_includedirs("libs")
    add_syslinks("d3d12", "dxgi", "d3dcompiler", "user32")
