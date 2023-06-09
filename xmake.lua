add_requires("dpp")
add_requires("sqlite3")

target("game_lobby")
    set_kind("binary")
    set_languages("c++23")
    add_files("source/main.cpp")
    add_files("source/**.cpp")
    add_includedirs("include", "third_party/ndb/include", "third_party/ndb/third_party/boost")
    add_defines("NOMINMAX")
    add_packages("dpp", "sqlite3")

set_installdir("$(scriptdir)")