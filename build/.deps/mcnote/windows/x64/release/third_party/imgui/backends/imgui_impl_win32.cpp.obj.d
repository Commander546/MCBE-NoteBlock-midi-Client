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
        [[third_party\imgui\backends\imgui_impl_win32.cpp]]
    },
    depfiles = "{\
    \"Version\": \"1.2\",\
    \"Data\": {\
        \"Source\": \"c:\\\\users\\\\yoshihiro\\\\desktop\\\\mc-note-client\\\\third_party\\\\imgui\\\\backends\\\\imgui_impl_win32.cpp\",\
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
            \"c:\\\\users\\\\yoshihiro\\\\desktop\\\\mc-note-client\\\\third_party\\\\imgui\\\\backends\\\\imgui_impl_win32.h\",\
            \"c:\\\\program files (x86)\\\\windows kits\\\\10\\\\include\\\\10.0.26100.0\\\\um\\\\windows.h\",\
            \"c:\\\\program files (x86)\\\\windows kits\\\\10\\\\include\\\\10.0.26100.0\\\\shared\\\\winapifamily.h\",\
            \"c:\\\\program files (x86)\\\\windows kits\\\\10\\\\include\\\\10.0.26100.0\\\\shared\\\\winpackagefamily.h\",\
            \"c:\\\\program files (x86)\\\\windows kits\\\\10\\\\include\\\\10.0.26100.0\\\\shared\\\\sdkddkver.h\",\
            \"c:\\\\program files\\\\microsoft visual studio\\\\2022\\\\community\\\\vc\\\\tools\\\\msvc\\\\14.44.35207\\\\include\\\\excpt.h\",\
            \"c:\\\\program files (x86)\\\\windows kits\\\\10\\\\include\\\\10.0.26100.0\\\\shared\\\\windef.h\",\
            \"c:\\\\program files (x86)\\\\windows kits\\\\10\\\\include\\\\10.0.26100.0\\\\shared\\\\minwindef.h\",\
            \"c:\\\\program files (x86)\\\\windows kits\\\\10\\\\include\\\\10.0.26100.0\\\\shared\\\\specstrings.h\",\
            \"c:\\\\program files (x86)\\\\windows kits\\\\10\\\\include\\\\10.0.26100.0\\\\shared\\\\specstrings_strict.h\",\
            \"c:\\\\program files (x86)\\\\windows kits\\\\10\\\\include\\\\10.0.26100.0\\\\shared\\\\specstrings_undef.h\",\
            \"c:\\\\program files (x86)\\\\windows kits\\\\10\\\\include\\\\10.0.26100.0\\\\shared\\\\driverspecs.h\",\
            \"c:\\\\program files (x86)\\\\windows kits\\\\10\\\\include\\\\10.0.26100.0\\\\shared\\\\sdv_driverspecs.h\",\
            \"c:\\\\program files (x86)\\\\windows kits\\\\10\\\\include\\\\10.0.26100.0\\\\um\\\\winnt.h\",\
            \"c:\\\\program files (x86)\\\\windows kits\\\\10\\\\include\\\\10.0.26100.0\\\\ucrt\\\\ctype.h\",\
            \"c:\\\\program files (x86)\\\\windows kits\\\\10\\\\include\\\\10.0.26100.0\\\\ucrt\\\\corecrt_wctype.h\",\
            \"c:\\\\program files (x86)\\\\windows kits\\\\10\\\\include\\\\10.0.26100.0\\\\shared\\\\kernelspecs.h\",\
            \"c:\\\\program files (x86)\\\\windows kits\\\\10\\\\include\\\\10.0.26100.0\\\\shared\\\\basetsd.h\",\
            \"c:\\\\program files (x86)\\\\windows kits\\\\10\\\\include\\\\10.0.26100.0\\\\shared\\\\guiddef.h\",\
            \"c:\\\\program files (x86)\\\\windows kits\\\\10\\\\include\\\\10.0.26100.0\\\\shared\\\\pshpack4.h\",\
            \"c:\\\\program files (x86)\\\\windows kits\\\\10\\\\include\\\\10.0.26100.0\\\\shared\\\\poppack.h\",\
            \"c:\\\\program files (x86)\\\\windows kits\\\\10\\\\include\\\\10.0.26100.0\\\\shared\\\\pshpack4.h\",\
            \"c:\\\\program files (x86)\\\\windows kits\\\\10\\\\include\\\\10.0.26100.0\\\\shared\\\\poppack.h\",\
            \"c:\\\\program files (x86)\\\\windows kits\\\\10\\\\include\\\\10.0.26100.0\\\\shared\\\\pshpack4.h\",\
            \"c:\\\\program files (x86)\\\\windows kits\\\\10\\\\include\\\\10.0.26100.0\\\\shared\\\\pshpack2.h\",\
            \"c:\\\\program files (x86)\\\\windows kits\\\\10\\\\include\\\\10.0.26100.0\\\\shared\\\\poppack.h\",\
            \"c:\\\\program files (x86)\\\\windows kits\\\\10\\\\include\\\\10.0.26100.0\\\\shared\\\\pshpack2.h\",\
            \"c:\\\\program files (x86)\\\\windows kits\\\\10\\\\include\\\\10.0.26100.0\\\\shared\\\\pshpack2.h\",\
            \"c:\\\\program files (x86)\\\\windows kits\\\\10\\\\include\\\\10.0.26100.0\\\\shared\\\\poppack.h\",\
            \"c:\\\\program files (x86)\\\\windows kits\\\\10\\\\include\\\\10.0.26100.0\\\\shared\\\\poppack.h\",\
            \"c:\\\\program files (x86)\\\\windows kits\\\\10\\\\include\\\\10.0.26100.0\\\\shared\\\\pshpack8.h\",\
            \"c:\\\\program files (x86)\\\\windows kits\\\\10\\\\include\\\\10.0.26100.0\\\\shared\\\\poppack.h\",\
            \"c:\\\\program files (x86)\\\\windows kits\\\\10\\\\include\\\\10.0.26100.0\\\\shared\\\\pshpack1.h\",\
            \"c:\\\\program files (x86)\\\\windows kits\\\\10\\\\include\\\\10.0.26100.0\\\\shared\\\\poppack.h\",\
            \"c:\\\\program files (x86)\\\\windows kits\\\\10\\\\include\\\\10.0.26100.0\\\\shared\\\\pshpack1.h\",\
            \"c:\\\\program files (x86)\\\\windows kits\\\\10\\\\include\\\\10.0.26100.0\\\\shared\\\\poppack.h\",\
            \"c:\\\\program files (x86)\\\\windows kits\\\\10\\\\include\\\\10.0.26100.0\\\\shared\\\\poppack.h\",\
            \"c:\\\\program files (x86)\\\\windows kits\\\\10\\\\include\\\\10.0.26100.0\\\\shared\\\\apiset.h\",\
            \"c:\\\\program files (x86)\\\\windows kits\\\\10\\\\include\\\\10.0.26100.0\\\\shared\\\\ktmtypes.h\",\
            \"c:\\\\program files (x86)\\\\windows kits\\\\10\\\\include\\\\10.0.26100.0\\\\um\\\\winbase.h\",\
            \"c:\\\\program files (x86)\\\\windows kits\\\\10\\\\include\\\\10.0.26100.0\\\\shared\\\\apisetcconv.h\",\
            \"c:\\\\program files (x86)\\\\windows kits\\\\10\\\\include\\\\10.0.26100.0\\\\um\\\\minwinbase.h\",\
            \"c:\\\\program files (x86)\\\\windows kits\\\\10\\\\include\\\\10.0.26100.0\\\\um\\\\apiquery2.h\",\
            \"c:\\\\program files (x86)\\\\windows kits\\\\10\\\\include\\\\10.0.26100.0\\\\um\\\\processenv.h\",\
            \"c:\\\\program files (x86)\\\\windows kits\\\\10\\\\include\\\\10.0.26100.0\\\\um\\\\fileapifromapp.h\",\
            \"c:\\\\program files (x86)\\\\windows kits\\\\10\\\\include\\\\10.0.26100.0\\\\um\\\\fileapi.h\",\
            \"c:\\\\program files (x86)\\\\windows kits\\\\10\\\\include\\\\10.0.26100.0\\\\um\\\\debugapi.h\",\
            \"c:\\\\program files (x86)\\\\windows kits\\\\10\\\\include\\\\10.0.26100.0\\\\um\\\\utilapiset.h\",\
            \"c:\\\\program files (x86)\\\\windows kits\\\\10\\\\include\\\\10.0.26100.0\\\\um\\\\handleapi.h\",\
            \"c:\\\\program files (x86)\\\\windows kits\\\\10\\\\include\\\\10.0.26100.0\\\\um\\\\errhandlingapi.h\",\
            \"c:\\\\program files (x86)\\\\windows kits\\\\10\\\\include\\\\10.0.26100.0\\\\um\\\\fibersapi.h\",\
            \"c:\\\\program files (x86)\\\\windows kits\\\\10\\\\include\\\\10.0.26100.0\\\\um\\\\namedpipeapi.h\",\
            \"c:\\\\program files (x86)\\\\windows kits\\\\10\\\\include\\\\10.0.26100.0\\\\um\\\\profileapi.h\",\
            \"c:\\\\program files (x86)\\\\windows kits\\\\10\\\\include\\\\10.0.26100.0\\\\um\\\\heapapi.h\",\
            \"c:\\\\program files (x86)\\\\windows kits\\\\10\\\\include\\\\10.0.26100.0\\\\um\\\\ioapiset.h\",\
            \"c:\\\\program files (x86)\\\\windows kits\\\\10\\\\include\\\\10.0.26100.0\\\\um\\\\synchapi.h\",\
            \"c:\\\\program files (x86)\\\\windows kits\\\\10\\\\include\\\\10.0.26100.0\\\\um\\\\interlockedapi.h\",\
            \"c:\\\\program files (x86)\\\\windows kits\\\\10\\\\include\\\\10.0.26100.0\\\\um\\\\processthreadsapi.h\",\
            \"c:\\\\program files (x86)\\\\windows kits\\\\10\\\\include\\\\10.0.26100.0\\\\um\\\\sysinfoapi.h\",\
            \"c:\\\\program files (x86)\\\\windows kits\\\\10\\\\include\\\\10.0.26100.0\\\\um\\\\memoryapi.h\",\
            \"c:\\\\program files (x86)\\\\windows kits\\\\10\\\\include\\\\10.0.26100.0\\\\um\\\\enclaveapi.h\",\
            \"c:\\\\program files (x86)\\\\windows kits\\\\10\\\\include\\\\10.0.26100.0\\\\um\\\\threadpoollegacyapiset.h\",\
            \"c:\\\\program files (x86)\\\\windows kits\\\\10\\\\include\\\\10.0.26100.0\\\\um\\\\threadpoolapiset.h\",\
            \"c:\\\\program files (x86)\\\\windows kits\\\\10\\\\include\\\\10.0.26100.0\\\\um\\\\jobapi.h\",\
            \"c:\\\\program files (x86)\\\\windows kits\\\\10\\\\include\\\\10.0.26100.0\\\\um\\\\jobapi2.h\",\
            \"c:\\\\program files (x86)\\\\windows kits\\\\10\\\\include\\\\10.0.26100.0\\\\um\\\\wow64apiset.h\",\
            \"c:\\\\program files (x86)\\\\windows kits\\\\10\\\\include\\\\10.0.26100.0\\\\um\\\\libloaderapi.h\",\
            \"c:\\\\program files (x86)\\\\windows kits\\\\10\\\\include\\\\10.0.26100.0\\\\um\\\\securitybaseapi.h\",\
            \"c:\\\\program files (x86)\\\\windows kits\\\\10\\\\include\\\\10.0.26100.0\\\\um\\\\namespaceapi.h\",\
            \"c:\\\\program files (x86)\\\\windows kits\\\\10\\\\include\\\\10.0.26100.0\\\\um\\\\systemtopologyapi.h\",\
            \"c:\\\\program files (x86)\\\\windows kits\\\\10\\\\include\\\\10.0.26100.0\\\\um\\\\processtopologyapi.h\",\
            \"c:\\\\program files (x86)\\\\windows kits\\\\10\\\\include\\\\10.0.26100.0\\\\um\\\\securityappcontainer.h\",\
            \"c:\\\\program files (x86)\\\\windows kits\\\\10\\\\include\\\\10.0.26100.0\\\\um\\\\realtimeapiset.h\",\
            \"c:\\\\program files (x86)\\\\windows kits\\\\10\\\\include\\\\10.0.26100.0\\\\shared\\\\winerror.h\",\
            \"c:\\\\program files (x86)\\\\windows kits\\\\10\\\\include\\\\10.0.26100.0\\\\um\\\\timezoneapi.h\",\
            \"c:\\\\program files (x86)\\\\windows kits\\\\10\\\\include\\\\10.0.26100.0\\\\um\\\\wingdi.h\",\
            \"c:\\\\program files (x86)\\\\windows kits\\\\10\\\\include\\\\10.0.26100.0\\\\shared\\\\pshpack1.h\",\
            \"c:\\\\program files (x86)\\\\windows kits\\\\10\\\\include\\\\10.0.26100.0\\\\shared\\\\poppack.h\",\
            \"c:\\\\program files (x86)\\\\windows kits\\\\10\\\\include\\\\10.0.26100.0\\\\shared\\\\pshpack2.h\",\
            \"c:\\\\program files (x86)\\\\windows kits\\\\10\\\\include\\\\10.0.26100.0\\\\shared\\\\poppack.h\",\
            \"c:\\\\program files (x86)\\\\windows kits\\\\10\\\\include\\\\10.0.26100.0\\\\shared\\\\pshpack2.h\",\
            \"c:\\\\program files (x86)\\\\windows kits\\\\10\\\\include\\\\10.0.26100.0\\\\shared\\\\poppack.h\",\
            \"c:\\\\program files (x86)\\\\windows kits\\\\10\\\\include\\\\10.0.26100.0\\\\shared\\\\pshpack4.h\",\
            \"c:\\\\program files (x86)\\\\windows kits\\\\10\\\\include\\\\10.0.26100.0\\\\shared\\\\poppack.h\",\
            \"c:\\\\program files (x86)\\\\windows kits\\\\10\\\\include\\\\10.0.26100.0\\\\shared\\\\pshpack4.h\",\
            \"c:\\\\program files (x86)\\\\windows kits\\\\10\\\\include\\\\10.0.26100.0\\\\shared\\\\poppack.h\",\
            \"c:\\\\program files (x86)\\\\windows kits\\\\10\\\\include\\\\10.0.26100.0\\\\um\\\\winuser.h\",\
            \"c:\\\\program files (x86)\\\\windows kits\\\\10\\\\include\\\\10.0.26100.0\\\\shared\\\\pshpack2.h\",\
            \"c:\\\\program files (x86)\\\\windows kits\\\\10\\\\include\\\\10.0.26100.0\\\\shared\\\\poppack.h\",\
            \"c:\\\\program files (x86)\\\\windows kits\\\\10\\\\include\\\\10.0.26100.0\\\\shared\\\\tvout.h\",\
            \"c:\\\\program files (x86)\\\\windows kits\\\\10\\\\include\\\\10.0.26100.0\\\\um\\\\winnls.h\",\
            \"c:\\\\program files (x86)\\\\windows kits\\\\10\\\\include\\\\10.0.26100.0\\\\um\\\\datetimeapi.h\",\
            \"c:\\\\program files (x86)\\\\windows kits\\\\10\\\\include\\\\10.0.26100.0\\\\um\\\\stringapiset.h\",\
            \"c:\\\\program files (x86)\\\\windows kits\\\\10\\\\include\\\\10.0.26100.0\\\\um\\\\winnls.h\",\
            \"c:\\\\program files (x86)\\\\windows kits\\\\10\\\\include\\\\10.0.26100.0\\\\um\\\\wincon.h\",\
            \"c:\\\\program files (x86)\\\\windows kits\\\\10\\\\include\\\\10.0.26100.0\\\\um\\\\wincontypes.h\",\
            \"c:\\\\program files (x86)\\\\windows kits\\\\10\\\\include\\\\10.0.26100.0\\\\um\\\\consoleapi.h\",\
            \"c:\\\\program files (x86)\\\\windows kits\\\\10\\\\include\\\\10.0.26100.0\\\\um\\\\consoleapi2.h\",\
            \"c:\\\\program files (x86)\\\\windows kits\\\\10\\\\include\\\\10.0.26100.0\\\\um\\\\consoleapi3.h\",\
            \"c:\\\\program files (x86)\\\\windows kits\\\\10\\\\include\\\\10.0.26100.0\\\\um\\\\winver.h\",\
            \"c:\\\\program files (x86)\\\\windows kits\\\\10\\\\include\\\\10.0.26100.0\\\\um\\\\verrsrc.h\",\
            \"c:\\\\program files (x86)\\\\windows kits\\\\10\\\\include\\\\10.0.26100.0\\\\um\\\\winreg.h\",\
            \"c:\\\\program files (x86)\\\\windows kits\\\\10\\\\include\\\\10.0.26100.0\\\\um\\\\reason.h\",\
            \"c:\\\\program files (x86)\\\\windows kits\\\\10\\\\include\\\\10.0.26100.0\\\\um\\\\winnetwk.h\",\
            \"c:\\\\program files (x86)\\\\windows kits\\\\10\\\\include\\\\10.0.26100.0\\\\shared\\\\wnnc.h\",\
            \"c:\\\\program files (x86)\\\\windows kits\\\\10\\\\include\\\\10.0.26100.0\\\\shared\\\\stralign.h\",\
            \"c:\\\\program files (x86)\\\\windows kits\\\\10\\\\include\\\\10.0.26100.0\\\\um\\\\winsvc.h\",\
            \"c:\\\\program files (x86)\\\\windows kits\\\\10\\\\include\\\\10.0.26100.0\\\\um\\\\mcx.h\",\
            \"c:\\\\program files (x86)\\\\windows kits\\\\10\\\\include\\\\10.0.26100.0\\\\um\\\\imm.h\",\
            \"c:\\\\program files (x86)\\\\windows kits\\\\10\\\\include\\\\10.0.26100.0\\\\um\\\\ime_cmodes.h\",\
            \"c:\\\\program files (x86)\\\\windows kits\\\\10\\\\include\\\\10.0.26100.0\\\\shared\\\\windowsx.h\",\
            \"c:\\\\program files (x86)\\\\windows kits\\\\10\\\\include\\\\10.0.26100.0\\\\ucrt\\\\tchar.h\",\
            \"c:\\\\program files (x86)\\\\windows kits\\\\10\\\\include\\\\10.0.26100.0\\\\ucrt\\\\wchar.h\",\
            \"c:\\\\program files (x86)\\\\windows kits\\\\10\\\\include\\\\10.0.26100.0\\\\ucrt\\\\corecrt_wconio.h\",\
            \"c:\\\\program files (x86)\\\\windows kits\\\\10\\\\include\\\\10.0.26100.0\\\\ucrt\\\\corecrt_stdio_config.h\",\
            \"c:\\\\program files (x86)\\\\windows kits\\\\10\\\\include\\\\10.0.26100.0\\\\ucrt\\\\corecrt_wdirect.h\",\
            \"c:\\\\program files (x86)\\\\windows kits\\\\10\\\\include\\\\10.0.26100.0\\\\ucrt\\\\corecrt_wio.h\",\
            \"c:\\\\program files (x86)\\\\windows kits\\\\10\\\\include\\\\10.0.26100.0\\\\ucrt\\\\corecrt_share.h\",\
            \"c:\\\\program files (x86)\\\\windows kits\\\\10\\\\include\\\\10.0.26100.0\\\\ucrt\\\\corecrt_wprocess.h\",\
            \"c:\\\\program files (x86)\\\\windows kits\\\\10\\\\include\\\\10.0.26100.0\\\\ucrt\\\\corecrt_wstdio.h\",\
            \"c:\\\\program files (x86)\\\\windows kits\\\\10\\\\include\\\\10.0.26100.0\\\\ucrt\\\\corecrt_wstdlib.h\",\
            \"c:\\\\program files (x86)\\\\windows kits\\\\10\\\\include\\\\10.0.26100.0\\\\ucrt\\\\corecrt_wtime.h\",\
            \"c:\\\\program files (x86)\\\\windows kits\\\\10\\\\include\\\\10.0.26100.0\\\\ucrt\\\\sys\\\\stat.h\",\
            \"c:\\\\program files (x86)\\\\windows kits\\\\10\\\\include\\\\10.0.26100.0\\\\ucrt\\\\sys\\\\types.h\",\
            \"c:\\\\program files\\\\microsoft visual studio\\\\2022\\\\community\\\\vc\\\\tools\\\\msvc\\\\14.44.35207\\\\include\\\\intrin.h\",\
            \"c:\\\\program files\\\\microsoft visual studio\\\\2022\\\\community\\\\vc\\\\tools\\\\msvc\\\\14.44.35207\\\\include\\\\intrin0.inl.h\",\
            \"c:\\\\program files\\\\microsoft visual studio\\\\2022\\\\community\\\\vc\\\\tools\\\\msvc\\\\14.44.35207\\\\include\\\\setjmp.h\",\
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
            \"c:\\\\program files (x86)\\\\windows kits\\\\10\\\\include\\\\10.0.26100.0\\\\ucrt\\\\corecrt_malloc.h\",\
            \"c:\\\\program files\\\\microsoft visual studio\\\\2022\\\\community\\\\vc\\\\tools\\\\msvc\\\\14.44.35207\\\\include\\\\zmmintrin.h\",\
            \"c:\\\\program files\\\\microsoft visual studio\\\\2022\\\\community\\\\vc\\\\tools\\\\msvc\\\\14.44.35207\\\\include\\\\ammintrin.h\",\
            \"c:\\\\program files (x86)\\\\windows kits\\\\10\\\\include\\\\10.0.26100.0\\\\um\\\\dwmapi.h\",\
            \"c:\\\\program files (x86)\\\\windows kits\\\\10\\\\include\\\\10.0.26100.0\\\\shared\\\\wtypes.h\",\
            \"c:\\\\program files (x86)\\\\windows kits\\\\10\\\\include\\\\10.0.26100.0\\\\shared\\\\rpc.h\",\
            \"c:\\\\program files (x86)\\\\windows kits\\\\10\\\\include\\\\10.0.26100.0\\\\shared\\\\pshpack8.h\",\
            \"c:\\\\program files (x86)\\\\windows kits\\\\10\\\\include\\\\10.0.26100.0\\\\shared\\\\rpcdce.h\",\
            \"c:\\\\program files (x86)\\\\windows kits\\\\10\\\\include\\\\10.0.26100.0\\\\shared\\\\rpcdcep.h\",\
            \"c:\\\\program files (x86)\\\\windows kits\\\\10\\\\include\\\\10.0.26100.0\\\\um\\\\rpcnsi.h\",\
            \"c:\\\\program files (x86)\\\\windows kits\\\\10\\\\include\\\\10.0.26100.0\\\\shared\\\\rpcnterr.h\",\
            \"c:\\\\program files (x86)\\\\windows kits\\\\10\\\\include\\\\10.0.26100.0\\\\shared\\\\rpcasync.h\",\
            \"c:\\\\program files (x86)\\\\windows kits\\\\10\\\\include\\\\10.0.26100.0\\\\shared\\\\pshpack8.h\",\
            \"c:\\\\program files (x86)\\\\windows kits\\\\10\\\\include\\\\10.0.26100.0\\\\shared\\\\poppack.h\",\
            \"c:\\\\program files (x86)\\\\windows kits\\\\10\\\\include\\\\10.0.26100.0\\\\shared\\\\poppack.h\",\
            \"c:\\\\program files (x86)\\\\windows kits\\\\10\\\\include\\\\10.0.26100.0\\\\shared\\\\rpcndr.h\",\
            \"c:\\\\program files (x86)\\\\windows kits\\\\10\\\\include\\\\10.0.26100.0\\\\shared\\\\pshpack8.h\",\
            \"c:\\\\program files (x86)\\\\windows kits\\\\10\\\\include\\\\10.0.26100.0\\\\um\\\\rpcnsip.h\",\
            \"c:\\\\program files (x86)\\\\windows kits\\\\10\\\\include\\\\10.0.26100.0\\\\shared\\\\rpcsal.h\",\
            \"c:\\\\program files (x86)\\\\windows kits\\\\10\\\\include\\\\10.0.26100.0\\\\shared\\\\poppack.h\",\
            \"c:\\\\program files (x86)\\\\windows kits\\\\10\\\\include\\\\10.0.26100.0\\\\shared\\\\wtypesbase.h\",\
            \"c:\\\\program files (x86)\\\\windows kits\\\\10\\\\include\\\\10.0.26100.0\\\\shared\\\\guiddef.h\",\
            \"c:\\\\program files (x86)\\\\windows kits\\\\10\\\\include\\\\10.0.26100.0\\\\um\\\\uxtheme.h\",\
            \"c:\\\\program files (x86)\\\\windows kits\\\\10\\\\include\\\\10.0.26100.0\\\\um\\\\commctrl.h\",\
            \"c:\\\\program files (x86)\\\\windows kits\\\\10\\\\include\\\\10.0.26100.0\\\\um\\\\prsht.h\",\
            \"c:\\\\program files (x86)\\\\windows kits\\\\10\\\\include\\\\10.0.26100.0\\\\shared\\\\pshpack8.h\",\
            \"c:\\\\program files (x86)\\\\windows kits\\\\10\\\\include\\\\10.0.26100.0\\\\shared\\\\poppack.h\",\
            \"c:\\\\program files (x86)\\\\windows kits\\\\10\\\\include\\\\10.0.26100.0\\\\shared\\\\pshpack1.h\",\
            \"c:\\\\program files (x86)\\\\windows kits\\\\10\\\\include\\\\10.0.26100.0\\\\shared\\\\poppack.h\",\
            \"c:\\\\program files (x86)\\\\windows kits\\\\10\\\\include\\\\10.0.26100.0\\\\shared\\\\pshpack1.h\",\
            \"c:\\\\program files (x86)\\\\windows kits\\\\10\\\\include\\\\10.0.26100.0\\\\shared\\\\poppack.h\",\
            \"c:\\\\program files (x86)\\\\windows kits\\\\10\\\\include\\\\10.0.26100.0\\\\shared\\\\pshpack1.h\",\
            \"c:\\\\program files (x86)\\\\windows kits\\\\10\\\\include\\\\10.0.26100.0\\\\shared\\\\poppack.h\",\
            \"c:\\\\program files (x86)\\\\windows kits\\\\10\\\\include\\\\10.0.26100.0\\\\shared\\\\pshpack1.h\",\
            \"c:\\\\program files (x86)\\\\windows kits\\\\10\\\\include\\\\10.0.26100.0\\\\shared\\\\poppack.h\",\
            \"c:\\\\program files (x86)\\\\windows kits\\\\10\\\\include\\\\10.0.26100.0\\\\shared\\\\pshpack1.h\",\
            \"c:\\\\program files (x86)\\\\windows kits\\\\10\\\\include\\\\10.0.26100.0\\\\shared\\\\poppack.h\",\
            \"c:\\\\program files (x86)\\\\windows kits\\\\10\\\\include\\\\10.0.26100.0\\\\um\\\\dpa_dsa.h\",\
            \"c:\\\\program files (x86)\\\\windows kits\\\\10\\\\include\\\\10.0.26100.0\\\\shared\\\\pshpack8.h\",\
            \"c:\\\\program files (x86)\\\\windows kits\\\\10\\\\include\\\\10.0.26100.0\\\\shared\\\\poppack.h\",\
            \"c:\\\\program files (x86)\\\\windows kits\\\\10\\\\include\\\\10.0.26100.0\\\\shared\\\\pshpack8.h\",\
            \"c:\\\\program files (x86)\\\\windows kits\\\\10\\\\include\\\\10.0.26100.0\\\\shared\\\\poppack.h\",\
            \"c:\\\\program files (x86)\\\\windows kits\\\\10\\\\include\\\\10.0.26100.0\\\\shared\\\\pshpack1.h\",\
            \"c:\\\\program files (x86)\\\\windows kits\\\\10\\\\include\\\\10.0.26100.0\\\\shared\\\\poppack.h\",\
            \"c:\\\\program files (x86)\\\\windows kits\\\\10\\\\include\\\\10.0.26100.0\\\\um\\\\xinput.h\"\
        ],\
        \"ImportedModules\": [],\
        \"ImportedHeaderUnits\": []\
    }\
}"
}