@echo off
rem Builds the DelveDeeperTest editor target with Unreal Engine 5.8. Used as the agent's --build-cmd on Windows.
rem Set UE_ROOT first if Unreal is not installed in the default place.
setlocal
if "%UE_ROOT%"=="" set "UE_ROOT=C:\Program Files\Epic Games\UE_5.8"
set "PROJECT=%~dp0..\..\3_unreal_project\DelveDeeperTest\DelveDeeperTest.uproject"
call "%UE_ROOT%\Engine\Build\BatchFiles\Build.bat" DelveDeeperTestEditor Win64 Development -Project="%PROJECT%" -WaitMutex
exit /b %ERRORLEVEL%
