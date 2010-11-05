-- A solution contains projects, and defines the available configurations
solution "vm"
    configurations { "Debug", "Release", "SDL" }
    linkoptions { "-llua", "-ldl" }
    
    -- A project defines one build target
    project "vm"
        kind "ConsoleApp"
        language "C++"
        files { "*.cc" }
        
        configuration "Debug"
            defines { "DEBUG" }
            flags { "Symbols" }
        
        configuration "SDL"
            defines { "USE_SDL" }
            buildoptions { "`sdl-config --cflags`" }
            linkoptions { "`sdl-config --static-libs`" }
            defines { "DEBUG" }
            flags { "Optimize" }
            files { "sdl/*.cc" }
        
        configuration "Release"
            defines { "NDEBUG" }
            flags { "Optimize" }
        
    project "test"
            kind "ConsoleApp"
            language "C++"
            files { "*.cc", "tests/*.cc" }
            excludes { "main.cc" }
            defines { "DEBUG" }
            flags { "Symbols" }
