-- A solution contains projects, and defines the available configurations
solution "vm"
    configurations { "Debug", "Release" }
    buildoptions { "`sdl-config --cflags`" }
    linkoptions { "`sdl-config --static-libs`", "-llua", "-ldl" }
    
    -- A project defines one build target
    project "vm"
        kind "ConsoleApp"
        language "C++"
        files { "*.cc" }
        
        configuration "Debug"
            defines { "DEBUG" }
            flags { "Symbols" }
            files { "main.cc" }
        
        configuration "SDL"
        
        configuration "Release"
            defines { "NDEBUG" }
            flags { "Optimize" }
    
