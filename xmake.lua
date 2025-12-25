set_project("benjamin")
set_version("0.0.1")

add_rules("mode.debug", "mode.release")
set_languages("c++23")
set_policy("build.c++.modules", true)

target("core_modules")
    set_kind("static")
    set_policy("build.c++.modules", true)
    add_files("core_modules/defines.ixx", { public = true })

target("gameplay")
    set_kind("shared")
    add_deps("core_modules")
    add_files("gameplay/api.cpp")

target("engine")
    set_kind("binary")
    add_deps("core_modules")
    add_files("engine/entry.cpp")
