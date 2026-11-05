param([switch]$Debug)
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
Set-Location -LiteralPath $root
if ([IO.Path]::GetPathRoot($root) -eq 'C:\') { throw 'Use a non-C drive workspace.' }
New-Item -ItemType Directory -Path 'build/tmp' -Force | Out-Null
$env:TEMP = Join-Path $root 'build/tmp'
$env:TMP = $env:TEMP
$env:TMPDIR = $env:TEMP
$gcc = (Get-Command gcc -ErrorAction Stop).Source
$flags = @('-std=c11','-Wall','-Wextra','-Wpedantic','-Werror','-Wconversion','-Wshadow','-g','-I.')
$flags += $(if ($Debug) { '-O0' } else { '-O2' })
$sources = @('ds/cqueue.c','ds/seqstack.c','ds/liststack.c','ds/listqueue.c')
& $gcc @flags @sources app/acquisition.c app/expression.c app/main.c -o build/lab.exe
if ($LASTEXITCODE -ne 0) { throw 'Build failed: lab' }
& $gcc @flags @sources bench/benchmark.c -o build/benchmark.exe
if ($LASTEXITCODE -ne 0) { throw 'Build failed: benchmark' }
Write-Output "Built build/lab.exe and build/benchmark.exe (Debug=$Debug)."
