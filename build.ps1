param (
	[string]$pMode
)
Write-Host "Build script!"

$programEXE = "DmxRasterizer.exe"
$exePath = Join-Path -Path "./build" -ChildPath $programEXE

switch ($pMode) {
	"--release" {
		Write-Host "Generating Release Build!"
		cmake -G Ninja -B ./build -DCMAKE_BUILD_TYPE=Release
		ninja -C build
		return
	}
	"--debug" {
		Write-Host "Generating Debug Build!"
		cmake -G Ninja -B ./build -DCMAKE_BUILD_TYPE=Debug
		ninja -C build
		return
	}
	"--run" {
		Write-Host "Running Program"
		lldb $exePath
		return
	}
	default {
		if ($pMode -match '1') {
			Write-Host "Generating debug Build with clang Setup!"
			cmake -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ -G Ninja -B ./build -DCMAKE_BUILD_TYPE=Debug
		}
		if ($pMode -match 'g') {
			Write-Host "Generating debug Build!"
			cmake -G Ninja -B ./build
		}
		if ($pMode -match 'b') {
			Write-Host "Building!"
			ninja -C build
		}
		if ($pMode -match 'r') {
			Write-Host "Launching!"
			& $exePath
		}
		if ($pMode -notmatch '[gbr1]') {
			ninja -C build
			& $exePath
		}
	}
}

if ($pMode2 -match 'r') {
	Write-Host "Launching!"
	& $exePath
}