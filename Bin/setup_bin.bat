@echo off
title Setting up TMOD Binaries
echo ========================================================
echo Setting up TMOD runtime files...
echo ========================================================

if not exist "%~dp0mta\libcef.dll" (
    if exist "%~dp0mta\libcef.dll.part1" (
        echo Reassembling libcef.dll from parts (GitHub 100MB limit bypass)...
        copy /b "%~dp0mta\libcef.dll.part1" + "%~dp0mta\libcef.dll.part2" + "%~dp0mta\libcef.dll.part3" "%~dp0mta\libcef.dll" > nul
        if exist "%~dp0mta\libcef.dll" (
            echo libcef.dll successfully assembled!
        ) else (
            echo ERROR: Failed to reassemble libcef.dll.
        )
    ) else (
        echo WARNING: libcef.dll parts not found.
    )
) else (
    echo libcef.dll is already present.
)

echo.
echo TMOD runtime setup complete.
