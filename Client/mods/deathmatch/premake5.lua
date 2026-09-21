project "Client Deathmatch"
	language "C++"
	kind "SharedLib"
	targetname "client"
	targetdir(buildpath("mods/deathmatch"))
	clangtidy "On"

	pchheader "StdInc.h"
	pchsource "StdInc.cpp"

	defines { "LUNASVG_BUILD", "LUA_USE_APICHECK", "SDK_WITH_BCRYPT" }
	links {
		"Lua_Client", "pcre2", "json-c", "ws2_32", "portaudio", "zlib", "cryptopp", "libspeex", "blowfish_bcrypt", "lunasvg", "BulletPhysics", "assimp",
		"../../../vendor/bass/lib/bass",
		"../../../vendor/bass/lib/bass_fx",
		"../../../vendor/bass/lib/bassmix",
		"../../../vendor/bass/lib/tags"
	}

	libdirs {
		"../../../vendor/assimp/lib",
		"../../../vendor/vtflib/lib"
	}
	vpaths {
		["Headers/*"] = {"**.h", "../../../Shared/mods/deathmatch/**.h", "../../**.h"},
		["Sources/*"] = {"**.cpp", "../../../Shared/mods/deathmatch/**.cpp", "../../../Shared/**.cpp", "../../../vendor/**.cpp"},
		["*"] = "premake5.lua"
	}

	filter "system:windows"
		includedirs { "../../../vendor/sparsehash/src/windows" }
		linkoptions { "/SAFESEH:NO" }

	filter {}
		includedirs {
			"../../../Shared/sdk",
			".",
			"./logic",
			"../../sdk/",
			"../../",
			"../../../vendor/pthreads/include",
			"../../../vendor/bochs",
			"../../../vendor/bass",
			"../../../vendor/libspeex",
			"../../../vendor/zlib",
			"../../../vendor/pcre2",
			"../../../vendor/json-c",
			"../../../vendor/lua/src",
			"../../../Shared/mods/deathmatch/logic",
			"../../../Shared/animation",
			"../../../Shared",
			"../../../vendor/sparsehash/src/",
			"../../../vendor/lunasvg/include",
			"../../../vendor/bullet3/src",
			"../../../vendor/assimp/include",
			"../../../vendor/vtflib/include"
	}

	files {
		"premake5.lua",
		"**.h",
		"**.cpp",
		"../../../Shared/mods/deathmatch/logic/**.cpp",
		"../../../Shared/mods/deathmatch/logic/**.h",
		"../../../Shared/animation/CEasingCurve.cpp",
		"../../../Shared/animation/CPositionRotationAnimation.cpp",
		-- Todo: Replace these two by using the CryptoPP functions instead
		"../../../vendor/bochs/bochs_internal/bochs_crc32.cpp"
	}

	filter "system:windows"
		buildoptions { "-Zm180" }

	filter "architecture:not x86"
		flags { "ExcludeFromBuild" }

	filter "system:not windows"
		flags { "ExcludeFromBuild" }


