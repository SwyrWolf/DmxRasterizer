param (
	[string]$pMode
)

$programEXE = "DmxRasterizer.exe"
$exePath = Join-Path -Path "./build" -ChildPath $programEXE
$buildstampHPP = "src/include/buildstamp.hpp"
$changelogPath = Join-Path "./docs" "changelog.md"

function New-DistZip {
	param(
		[string]$Version
	)

	$distDir = Join-Path $PSScriptRoot "..\dist"
	if (-not (Test-Path $distDir)) {
		New-Item -ItemType Directory -Path $distDir | Out-Null
	}

	$name = [IO.Path]::GetFileNameWithoutExtension($ExePath)
	$zipPath = Join-Path $distDir "${name}_${Version}.zip"

	if (Test-Path $zipPath) { Remove-Item $zipPath -Force }

	Compress-Archive -Path @($ExePath, $changelogPath) -DestinationPath $zipPath
	Write-Host "Created $zipPath"
}

switch ($pMode) {
	"--release" {
		Write-Host "Generating Release Build!"
		cmake -G Ninja -B ./build -DCMAKE_BUILD_TYPE=Release
		ninja -C build
		return
	}
	"--releaseDist" {
		while ($true) {
			$input = Read-Host "Incriment new release (y/n)"

			switch ($input) {
				"y" { 
					Write-Host "Creating new dist release"

					if (-not (Test-Path -LiteralPath $buildstampHPP)) {
						throw "File not found: $buildstampHPP"
					}
					$text = Get-Content -LiteralPath $buildstampHPP -Raw -ErrorAction Stop
					$time = (Get-Date).ToString("yyyy-MM-dd HH:mm:ss")

					$patternTime = 'APP_BUILD_TIME\s+"[^"]*"'
					$pattern = 'APP_BUILD_VERSION\s+"v(\d+)\.(\d+)\.(\d+)"'
					$m = [regex]::Match($text, $pattern)
					if (-not $m.Success) { throw "APP_BUILD_VERSION string not found in: $buildstampHPP" }

					$major = [int]$m.Groups[1].Value
					$minor = [int]$m.Groups[2].Value
					$patch = [int]$m.Groups[3].Value

					$choices = @("&Major","M&inor","&Patch")
					$pick = $Host.UI.PromptForChoice(
						"Release Version",
						"Increment which part of v$major.$minor.$patch ?",
						$choices,
						2
					)
					switch ($pick) {
						0 { $major++; $minor = 0; $patch = 0 } # Major
						1 { $minor++; $patch = 0 }             # Minor
						2 { $patch++ }                         # Patch
						default { return }                     # cancelled/unknown
					}
					$newVersion = "v$major.$minor.$patch"

					$text = [regex]::Replace($text, $patternTime, "APP_BUILD_TIME `"$time`"", 1)
					$text = [regex]::Replace($text, $pattern, "APP_BUILD_VERSION `"$newVersion`"", 1)

					Set-Content -LiteralPath $buildstampHPP -Value $text -Encoding utf8 -ErrorAction Stop
					Write-Host "Updated APP_BUILD_VERSION -> $newVersion"

					cmake -G Ninja -B ./build -DCMAKE_BUILD_TYPE=Release
					ninja -C build

					New-DistZip $newVersion
					return
				}
				"n" { return $false }
				default { Write-Host "Please enter y or n." }
			}
		}
	}
	"--debug" {
		Write-Host "Generating Debug Build!"
		cmake -G Ninja -B ./build -DCMAKE_BUILD_TYPE=Debug
		ninja -C build
		return
	}
	"--rundb" {
		Write-Host "Running Program"
		lldb $exePath
		return
	}
	"--run" {
		Write-Host "Running Program"
		& $exePath
		return
	}
	"--setup" {
		Write-Host "Generating debug Build with clang Setup!"
		cmake -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ -G Ninja -B ./build -DCMAKE_BUILD_TYPE=Debug
		return
	}
}