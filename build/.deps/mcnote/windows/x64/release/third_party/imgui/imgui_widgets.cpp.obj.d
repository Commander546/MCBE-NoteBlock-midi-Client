{
    values = {
        [[C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.44.35207\bin\HostX64\x64\cl.exe]],
        {
            "-nologo",
            "-MD",
            "-O2",
            "-std:c++23preview",
            "-Isrc",
            [[-Isrc\core]],
            [[-Isrc\platform]],
            [[-Isrc\game]],
            [[-Isrc\app]],
            [[-Isrc\ui]],
            [[-Isrc\dev]],
            [[-Ithird_party\imgui]],
            [[-Ithird_party\imgui\backends]],
            "-DUNICODE",
            "-D_UNICODE",
            "-DNOMINMAX",
            "-DZYDIS_STATIC_BUILD",
            "/EHsc",
            "-external:W0",
            [[-external:IC:\Users\yoshihiro\AppData\Local\.xmake\packages\s\safetyhook\v0.7.0\13306f1ea52a4a9b962d07a21c9dcef7\include]],
            "-external:W0",
            [[-external:IC:\Users\yoshihiro\AppData\Local\.xmake\packages\z\zydis\v4.1.0\391656aa809f4fddbc96778334d19c48\include]],
            "-external:W0",
            [[-external:IC:\Users\yoshihiro\AppData\Local\.xmake\packages\z\zycore-c\v1.5.0\28a4d85ea4b8457496f28b75007f6b23\include]],
            "/utf-8",
            "-DNDEBUG"
        }
    },
    depfiles_format = "cl_json",
    files = {
        [[third_party\imgui\imgui_widgets.cpp]]
    },
    depfiles = "{\
    \"Version\": \"1.2\",\
    \"Data\": {\
        \"Source\": \"c:\\\\users\\\\yoshihiro\\\\desktop\\\\mc-note-client\\\\third_party\\\\imgui\\\\imgui_widgets.cpp\",\
        \"ProvidedModule\": \"\",\
        \"Includes\": [\
            \"c:\\\\users\\\\yoshihiro\\\\desktop\\\\mc-note-client\\\\third_party\\\\imgui\\\\imgui.h\",\
            \"c:\\\\users\\\\yoshihiro\\\\desktop\\\\mc-note-client\\\\third_party\\\\imgui\\\\imconfig.h\",\
            \"c:\\\\program files (x86)\\\\windows kits\\\\10\\\\include\\\\10.0.26100.0\\\\ucrt\\\\float.h\",\
            \"c:\\\\program files (x86)\\\\windows kits\\\\10\\\\include\\\\10.0.26100.0\\\\ucrt\\\\corecrt.h\",\
            \"c:\\\\program files\\\\microsoft visual studio\\\\2022\\\\community\\\\vc\\\\tools\\\\msvc\\\\14.44.35207\\\\include\\\\vcruntime.h\",\
            \"c:\\\\program files\\\\microsoft visual studio\\\\2022\\\\community\\\\vc\\\\tools\\\\msvc\\\\14.44.35207\\\\include\\\\sal.h\",\
            \"c:\\\\program files\\\\microsoft visual studio\\\\2022\\\\community\\\\vc\\\\tools\\\\msvc\\\\14.44.35207\\\\include\\\\concurrencysal.h\",\
            \"c:\\\\program files\\\\microsoft visual studio\\\\2022\\\\community\\\\vc\\\\tools\\\\msvc\\\\14.44.35207\\\\include\\\\vadefs.h\",\
            \"c:\\\\program files\\\\microsoft visual studio\\\\2022\\\\community\\\\vc\\\\tools\\\\msvc\\\\14.44.35207\\\\include\\\\stdarg.h\",\
            \"c:\\\\program files (x86)\\\\windows kits\\\\10\\\\include\\\\10.0.26100.0\\\\ucrt\\\\stddef.h\",\
            \"c:\\\\program files (x86)\\\\windows kits\\\\10\\\\include\\\\10.0.26100.0\\\\ucrt\\\\string.h\",\
            \"c:\\\\program files (x86)\\\\windows kits\\\\10\\\\include\\\\10.0.26100.0\\\\ucrt\\\\corecrt_memory.h\",\
            \"c:\\\\program files (x86)\\\\windows kits\\\\10\\\\include\\\\10.0.26100.0\\\\ucrt\\\\corecrt_memcpy_s.h\",\
            \"c:\\\\program files (x86)\\\\windows kits\\\\10\\\\include\\\\10.0.26100.0\\\\ucrt\\\\errno.h\",\
            \"c:\\\\program files\\\\microsoft visual studio\\\\2022\\\\community\\\\vc\\\\tools\\\\msvc\\\\14.44.35207\\\\include\\\\vcruntime_string.h\",\
            \"c:\\\\program files (x86)\\\\windows kits\\\\10\\\\include\\\\10.0.26100.0\\\\ucrt\\\\corecrt_wstring.h\",\
            \"c:\\\\program files (x86)\\\\windows kits\\\\10\\\\include\\\\10.0.26100.0\\\\ucrt\\\\assert.h\",\
            \"c:\\\\users\\\\yoshihiro\\\\desktop\\\\mc-note-client\\\\third_party\\\\imgui\\\\imgui_internal.h\",\
            \"c:\\\\program files (x86)\\\\windows kits\\\\10\\\\include\\\\10.0.26100.0\\\\ucrt\\\\stdio.h\",\
            \"c:\\\\program files (x86)\\\\windows kits\\\\10\\\\include\\\\10.0.26100.0\\\\ucrt\\\\corecrt_wstdio.h\",\
            \"c:\\\\program files (x86)\\\\windows kits\\\\10\\\\include\\\\10.0.26100.0\\\\ucrt\\\\corecrt_stdio_config.h\",\
            \"c:\\\\program files (x86)\\\\windows kits\\\\10\\\\include\\\\10.0.26100.0\\\\ucrt\\\\stdlib.h\",\
            \"c:\\\\program files (x86)\\\\windows kits\\\\10\\\\include\\\\10.0.26100.0\\\\ucrt\\\\corecrt_malloc.h\",\
            \"c:\\\\program files (x86)\\\\windows kits\\\\10\\\\include\\\\10.0.26100.0\\\\ucrt\\\\corecrt_search.h\",\
            \"c:\\\\program files (x86)\\\\windows kits\\\\10\\\\include\\\\10.0.26100.0\\\\ucrt\\\\corecrt_wstdlib.h\",\
            \"c:\\\\program files\\\\microsoft visual studio\\\\2022\\\\community\\\\vc\\\\tools\\\\msvc\\\\14.44.35207\\\\include\\\\limits.h\",\
            \"c:\\\\program files (x86)\\\\windows kits\\\\10\\\\include\\\\10.0.26100.0\\\\ucrt\\\\math.h\",\
            \"c:\\\\program files (x86)\\\\windows kits\\\\10\\\\include\\\\10.0.26100.0\\\\ucrt\\\\corecrt_math.h\",\
            \"c:\\\\program files\\\\microsoft visual studio\\\\2022\\\\community\\\\vc\\\\tools\\\\msvc\\\\14.44.35207\\\\include\\\\immintrin.h\",\
            \"c:\\\\program files\\\\microsoft visual studio\\\\2022\\\\community\\\\vc\\\\tools\\\\msvc\\\\14.44.35207\\\\include\\\\wmmintrin.h\",\
            \"c:\\\\program files\\\\microsoft visual studio\\\\2022\\\\community\\\\vc\\\\tools\\\\msvc\\\\14.44.35207\\\\include\\\\nmmintrin.h\",\
            \"c:\\\\program files\\\\microsoft visual studio\\\\2022\\\\community\\\\vc\\\\tools\\\\msvc\\\\14.44.35207\\\\include\\\\smmintrin.h\",\
            \"c:\\\\program files\\\\microsoft visual studio\\\\2022\\\\community\\\\vc\\\\tools\\\\msvc\\\\14.44.35207\\\\include\\\\tmmintrin.h\",\
            \"c:\\\\program files\\\\microsoft visual studio\\\\2022\\\\community\\\\vc\\\\tools\\\\msvc\\\\14.44.35207\\\\include\\\\pmmintrin.h\",\
            \"c:\\\\program files\\\\microsoft visual studio\\\\2022\\\\community\\\\vc\\\\tools\\\\msvc\\\\14.44.35207\\\\include\\\\emmintrin.h\",\
            \"c:\\\\program files\\\\microsoft visual studio\\\\2022\\\\community\\\\vc\\\\tools\\\\msvc\\\\14.44.35207\\\\include\\\\xmmintrin.h\",\
            \"c:\\\\program files\\\\microsoft visual studio\\\\2022\\\\community\\\\vc\\\\tools\\\\msvc\\\\14.44.35207\\\\include\\\\mmintrin.h\",\
            \"c:\\\\program files (x86)\\\\windows kits\\\\10\\\\include\\\\10.0.26100.0\\\\ucrt\\\\malloc.h\",\
            \"c:\\\\program files\\\\microsoft visual studio\\\\2022\\\\community\\\\vc\\\\tools\\\\msvc\\\\14.44.35207\\\\include\\\\zmmintrin.h\",\
            \"c:\\\\program files\\\\microsoft visual studio\\\\2022\\\\community\\\\vc\\\\tools\\\\msvc\\\\14.44.35207\\\\include\\\\stdint.h\",\
            \"c:\\\\users\\\\yoshihiro\\\\desktop\\\\mc-note-client\\\\third_party\\\\imgui\\\\imstb_textedit.h\",\
            \"c:\\\\users\\\\yoshihiro\\\\desktop\\\\mc-note-client\\\\third_party\\\\imgui\\\\imstb_textedit.h\"\
        ],\
        \"ImportedModules\": [],\
        \"ImportedHeaderUnits\": []\
    }\
}"
}