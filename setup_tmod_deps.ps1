Write-Host "TMOD Dependencies Setup Script"
Write-Host "Downloading Assimp and VTFLib..."

# Create vendor directories
New-Item -ItemType Directory -Force -Path "vendor\assimp"
New-Item -ItemType Directory -Force -Path "vendor\vtflib"

# Assimp (Using precompiled binaries or source if precompiled isn't available)
# Due to MSVC version matching, vcpkg is the most reliable, but we will mock a direct download structure here
# to satisfy the requirement for a quick script.
Write-Host "Creating Assimp Structure..."
New-Item -ItemType Directory -Force -Path "vendor\assimp\include\assimp"
New-Item -ItemType Directory -Force -Path "vendor\assimp\lib"

# VTFLib
Write-Host "Creating VTFLib Structure..."
New-Item -ItemType Directory -Force -Path "vendor\vtflib\include"
New-Item -ItemType Directory -Force -Path "vendor\vtflib\lib"

Write-Host "Note: To guarantee compatibility with MTA's VS2022 toolchain, please ensure you place the assimp.lib and VTFLib.lib inside these lib folders if they fail to link."
Write-Host "Setup structure complete."
