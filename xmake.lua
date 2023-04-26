add_requires("dpp")

target("main")
    set_kind("binary")
    set_languages("c++23")
    add_files("source/main.cpp")
    add_files("source/**.cpp")
    add_includedirs("include")
    add_packages("dpp")