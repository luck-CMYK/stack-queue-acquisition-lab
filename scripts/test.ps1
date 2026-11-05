param([switch]$Release)
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
Set-Location -LiteralPath $root
if ([IO.Path]::GetPathRoot($root) -eq 'C:\') { throw 'Use a non-C drive workspace.' }
New-Item -ItemType Directory -Path 'build/tmp','docs/evidence' -Force | Out-Null
$env:TEMP = Join-Path $root 'build/tmp'
$env:TMP = $env:TEMP
$env:TMPDIR = $env:TEMP
$gcc = (Get-Command gcc -ErrorAction Stop).Source
$flags = @('-std=c11','-Wall','-Wextra','-Wpedantic','-Werror','-Wconversion','-Wshadow','-g','-I.')
$flags += $(if ($Release) { '-O2' } else { '-O0' })
$suffix = $(if ($Release) { '_release' } else { '' })
$total = 0
foreach ($module in @('cqueue','seqstack','liststack','listqueue')) {
    $exe = "build/test_$module.exe"
    & $gcc @flags -include test/allocation_hooks.h -c "ds/$module.c" -o "build/audit_$module.o"
    if ($LASTEXITCODE -ne 0) { throw "Compile failed: $module" }
    & $gcc @flags "build/audit_$module.o" "test/test_$module.c" test/allocation_audit.c -o $exe
    if ($LASTEXITCODE -ne 0) { throw "Compile failed: $module" }
    & ".\$exe" 2>&1 | Tee-Object -FilePath "docs/evidence/green_$module$suffix.txt"
    if ($LASTEXITCODE -ne 0) { throw "Tests failed: $module" }
    ++$total
}
& $gcc @flags -include test/allocation_hooks.h -c app/expression.c -o build/audit_expression.o
if ($LASTEXITCODE -ne 0) { throw 'Compile failed: expression' }
& $gcc @flags build/audit_expression.o build/audit_seqstack.o test/test_expression.c test/allocation_audit.c -o build/test_expression.exe
if ($LASTEXITCODE -ne 0) { throw 'Link failed: expression' }
& .\build\test_expression.exe 2>&1 | Tee-Object -FilePath "docs/evidence/green_expression$suffix.txt"
if ($LASTEXITCODE -ne 0) { throw 'Tests failed: expression' }
++ $total
& $gcc @flags -include test/allocation_hooks.h -c app/acquisition.c -o build/audit_acquisition.o
if ($LASTEXITCODE -ne 0) { throw 'Compile failed: acquisition' }
& $gcc @flags build/audit_acquisition.o build/audit_cqueue.o test/test_acquisition.c test/allocation_audit.c -o build/test_acquisition.exe
if ($LASTEXITCODE -ne 0) { throw 'Link failed: acquisition' }
& .\build\test_acquisition.exe 2>&1 | Tee-Object -FilePath "docs/evidence/green_acquisition$suffix.txt"
if ($LASTEXITCODE -ne 0) { throw 'Tests failed: acquisition' }
++ $total
Write-Output "All $total module test executables passed (Release=$Release)."
