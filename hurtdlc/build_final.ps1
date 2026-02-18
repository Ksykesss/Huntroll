$ErrorActionPreference = "Stop"

$VSDIR = "C:\Program Files\Microsoft Visual Studio\18\Community"
$env:PATH = "$VSDIR\VC\Tools\MSVC\14.50.35717\bin\Hostx64\x64;$env:PATH"
$env:INCLUDE = "$VSDIR\VC\Tools\MSVC\14.50.35717\include;$VSDIR\VC\Tools\MSVC\14.50.35717\atlmfc\include;C:\Program Files (x86)\Windows Kits\10\Include\10.0.26100.0\ucrt;C:\Program Files (x86)\Windows Kits\10\Include\10.0.26100.0\um;C:\Program Files (x86)\Windows Kits\10\Include\10.0.26100.0\shared;C:\Program Files\Java\jdk-21\include;C:\Program Files\Java\jdk-21\include\win32"
$env:LIB = "$VSDIR\VC\Tools\MSVC\14.50.35717\lib\x64;$VSDIR\VC\Tools\MSVC\14.50.35717\atlmfc\lib\x64;C:\Program Files (x86)\Windows Kits\10\Lib\10.0.26100.0\ucrt\x64;C:\Program Files (x86)\Windows Kits\10\Lib\10.0.26100.0\um\x64;C:\Program Files\Java\jdk-21\lib"

Write-Host "[*] Compiling HurtDLC..." -ForegroundColor Cyan

cl.exe /LD /O2 /MT /EHsc /std:c++17 `
  /I. /Iext `
  /DNDEBUG /D_WINDOWS /D_USRDLL `
  dllmain.cpp hooks.cpp gui.cpp esp.cpp `
  jni_helper.cpp minecraft_classes.cpp `
  aimassist_jni.cpp triggerbot_jni.cpp `
  ext\imgui\imgui.cpp ext\imgui\imgui_draw.cpp ext\imgui\imgui_widgets.cpp `
  ext\imgui\imgui_tables.cpp ext\imgui\imgui_impl_win32.cpp ext\imgui\imgui_impl_opengl3.cpp `
  /link /OUT:hurtdlc.dll opengl32.lib gdi32.lib user32.lib `
  "C:\Program Files\Java\jdk-21\lib\jvm.lib"

if (Test-Path "hurtdlc.dll") {
    Write-Host "[+] DLL compiled: hurtdlc.dll" -ForegroundColor Green
    Remove-Item *.obj, *.exp, *.lib -ErrorAction SilentlyContinue
} else {
    Write-Host "[!] DLL compilation failed!" -ForegroundColor Red
    exit 1
}

Write-Host "[*] Done! HurtDLC ready to inject" -ForegroundColor Green
