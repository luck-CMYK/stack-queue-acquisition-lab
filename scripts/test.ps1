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
$total = 0
foreach ($module in @('cqueue','seqstack','liststack','listqueue')) {
    $exe = "build/test_$module.exe"
    & $gcc @flags -include test/allocation_hooks.h -c "ds/$module.c" -o "build/audit_$module.o"
    if ($LASTEXITCODE -ne 0) { throw "Compile failed: $module" }
    & $gcc @flags "build/audit_$module.o" "test/test_$module.c" test/allocation_audit.c -o $exe
    if ($LASTEXITCODE -ne 0) { throw "Compile failed: $module" }
    & ".\$exe" 2>&1 | Tee-Object -FilePath "docs/evidence/green_$module.txt"
    if ($LASTEXITCODE -ne 0) { throw "Tests failed: $module" }
    ++$total
}
Write-Output "All $total module test executables passed (Release=$Release)."
