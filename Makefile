# Leyline Audio (C++) - Task Makefile
# Equivalent to Makefile.toml from the Rust project.
# Usage:  cargo-make equivalent -> just use PowerShell directly.
# Kept as a thin wrapper that maps task names to script invocations.

.PHONY: build clean install uninstall test test-endpoints

build:
	powershell -ExecutionPolicy Bypass -File scripts\Install.ps1

clean:
	powershell -ExecutionPolicy Bypass -File scripts\Install.ps1 -clean

install:
	powershell -ExecutionPolicy Bypass -File scripts\Install.ps1

uninstall:
	powershell -ExecutionPolicy Bypass -File scripts\Uninstall.ps1

test:
	dotnet test test\EndpointTester\EndpointTester.csproj

test-endpoints:
	cd test\EndpointTester && dotnet run
