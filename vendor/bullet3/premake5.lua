project "BulletPhysics"
    kind "StaticLib"
    language "C++"
    targetdir "bin/%{cfg.buildcfg}"
    includedirs { "src" }
    warnings "Off"
    files {
        "src/BulletCollision/**.cpp",
        "src/BulletCollision/**.h",
        "src/BulletDynamics/**.cpp",
        "src/BulletDynamics/**.h",
        "src/LinearMath/**.cpp",
        "src/LinearMath/**.h"
    }

