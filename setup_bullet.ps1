# TMOD Bullet Physics Kurulum Scripti
# ====================================
# Bu script, bullet3.zip dosyasini vendor/bullet3 klasorune acar.
#
# ADIM 1: Asagidaki linke tarayicinizdan gidin ve ZIP'i indirin:
#   https://github.com/bulletphysics/bullet3/archive/refs/tags/3.25.zip
#
# ADIM 2: Indirdiginiz ZIP dosyasini su konuma kopyalayin:
#   c:\Users\dogu\Desktop\mtasa-blue\bullet3.zip
#
# ADIM 3: Bu scripti calistirin:
#   PowerShell -ExecutionPolicy Bypass -File setup_bullet.ps1

Write-Host "TMOD Bullet3 Kurulum Scripti" -ForegroundColor Cyan
Write-Host "============================" -ForegroundColor Cyan

$zipPath = "c:\Users\dogu\Desktop\mtasa-blue\bullet3.zip"
$vendorPath = "c:\Users\dogu\Desktop\mtasa-blue\vendor"

if (!(Test-Path $zipPath)) {
    Write-Host "HATA: bullet3.zip bulunamadi!" -ForegroundColor Red
    Write-Host "Lutfen https://github.com/bulletphysics/bullet3/archive/refs/tags/3.25.zip adresinden indirip" -ForegroundColor Yellow
    Write-Host "c:\Users\dogu\Desktop\mtasa-blue\bullet3.zip konumuna koyun." -ForegroundColor Yellow
    exit 1
}

Write-Host "ZIP aciliyor..." -ForegroundColor Green
Remove-Item "$vendorPath\bullet3" -Recurse -Force -ErrorAction SilentlyContinue
Expand-Archive -Path $zipPath -DestinationPath $vendorPath -Force

# Rename bullet3-3.25 -> bullet3
$extracted = Get-ChildItem $vendorPath -Directory -Filter "bullet3-*" | Select-Object -First 1
if ($extracted) {
    Rename-Item $extracted.FullName "bullet3"
    Write-Host "bullet3 klasoru olusturuldu: $vendorPath\bullet3" -ForegroundColor Green
} else {
    Write-Host "HATA: ZIP icinde bullet3 klasoru bulunamadi!" -ForegroundColor Red
    exit 1
}

# Premake dosyasini olustur
$premake = @'
project "BulletPhysics"
    kind "StaticLib"
    language "C++"
    targetdir "bin/%{cfg.buildcfg}"
    includedirs { "src" }
    files {
        "src/BulletCollision/**.cpp",
        "src/BulletCollision/**.h",
        "src/BulletDynamics/**.cpp",
        "src/BulletDynamics/**.h",
        "src/LinearMath/**.cpp",
        "src/LinearMath/**.h"
    }
'@
Set-Content "$vendorPath\bullet3\premake5.lua" $premake

Write-Host ""
Write-Host "TAMAM! Bullet3 basariyla kuruldu." -ForegroundColor Green
Write-Host "Simdi premake5 calistirin: utils\premake5.exe vs2022" -ForegroundColor Cyan

# Temizlik
Remove-Item $zipPath -ErrorAction SilentlyContinue
