@echo off
setlocal
cmake -S . -B build\windows-release -G "Visual Studio 17 2022" -A x64 -DVOICEDUB_BUILD_PLUGIN=ON -DVOICEDUB_BUILD_TESTS=ON
if errorlevel 1 exit /b %errorlevel%
cmake --build build\windows-release --config Release --target VoiceDubCoreTests VoiceDub_VST3
if errorlevel 1 exit /b %errorlevel%
ctest --test-dir build\windows-release -C Release --output-on-failure
if errorlevel 1 exit /b %errorlevel%
echo.
echo Built plugin should be in build\windows-release\VoiceDub_artefacts\Release\VST3\VoiceDub.vst3
