param (
	[string]$pMode
)

$programEXE = "dmxrasterizer.exe"
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