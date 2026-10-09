@echo off
title Start TMOD Client
cd /d "%~dp0"
if not exist "mta\libcef.dll" (
    call setup_bin.bat
)
start "" TMOD.exe %*
