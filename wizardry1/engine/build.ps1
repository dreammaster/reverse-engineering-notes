# Configure + build the standalone engine with the CMake/Ninja/MSVC that ships
# with Visual Studio 2022 Community.  Run from anywhere.
$ErrorActionPreference = "Stop"
$vs = "C:\Program Files\Microsoft Visual Studio\2022\Community"
$cmake = "$vs\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
$ninja = "$vs\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja"
$vcvars = "$vs\VC\Auxiliary\Build\vcvars64.bat"
$eng = $PSScriptRoot
$env:PATH = "$ninja;$env:PATH"
cmd /c "call `"$vcvars`" >nul 2>&1 && `"$cmake`" -S `"$eng`" -B `"$eng\build`" -G Ninja -DCMAKE_BUILD_TYPE=Release && `"$cmake`" --build `"$eng\build`""
if ($LASTEXITCODE -ne 0) { throw "build failed" }
Write-Host "`n-> $eng\build\wiz1.exe"
