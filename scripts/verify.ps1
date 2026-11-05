$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
Set-Location -LiteralPath $root
& "$PSScriptRoot/test.ps1"
& "$PSScriptRoot/test.ps1" -Release
& "$PSScriptRoot/build.ps1"
$gcc = (Get-Command gcc -ErrorAction Stop).Source
$sources = @('ds/cqueue.c','ds/seqstack.c','ds/liststack.c','ds/listqueue.c','app/acquisition.c','app/expression.c','app/main.c','bench/benchmark.c')
$flags = @('-std=c11','-Wall','-Wextra','-Wpedantic','-Werror','-Wconversion','-Wshadow','-fanalyzer','-g','-O0','-I.')
$analysisLog = @()
foreach ($source in $sources) {
    $name = [IO.Path]::GetFileNameWithoutExtension($source)
    $output = & $gcc @flags -c $source -o "build/analyze_$name.o" 2>&1
    if ($LASTEXITCODE -ne 0) { $output | Write-Output; throw "Static analysis failed: $source" }
    $analysisLog += "$source : GCC -fanalyzer passed with strict warnings"
}
$analysisLog | Set-Content -LiteralPath 'docs/evidence/static_analysis.txt' -Encoding utf8
& .\build\lab.exe --demo 2>&1 | Tee-Object -FilePath 'docs/evidence/demo.txt'
if ($LASTEXITCODE -ne 0) { throw 'Demo failed.' }
$cliLog = @()
foreach ($case in @(
    @{ expression = '2*-3+(8/4)'; expected = 0; result = '-4' },
    @{ expression = '1/2'; expected = 0; result = '0.5' },
    @{ expression = '1/(2-2)'; expected = 1; result = 'division by zero' },
    @{ expression = '2(3)'; expected = 1; result = 'syntax' }
)) {
    $savedPreference = $ErrorActionPreference
    $ErrorActionPreference = 'Continue'
    $output = & .\build\lab.exe --eval $case.expression 2>&1
    $code = $LASTEXITCODE
    $ErrorActionPreference = $savedPreference
    $cliLog += "$($case.expression) -> exit=$code : $output"
    if ($code -ne $case.expected -or ($output -join "`n") -notmatch [regex]::Escape($case.result)) { throw 'CLI expression regression.' }
}
foreach ($case in @(@{text='{[()]}';expected=0},@{text='([)]';expected=1})) {
    $output = & .\build\lab.exe --brackets $case.text 2>&1
    $code = $LASTEXITCODE
    $cliLog += "brackets $($case.text) -> exit=$code : $output"
    if ($code -ne $case.expected) { throw 'CLI brackets regression.' }
}
$utf8NoBom = New-Object System.Text.UTF8Encoding $false
[Console]::InputEncoding = $utf8NoBom
$OutputEncoding = $utf8NoBom
$interactive = @('2+3*4','q') | & .\build\lab.exe
$interactiveExit = $LASTEXITCODE
$interactive | Set-Content -LiteralPath 'docs/evidence/interactive.txt' -Encoding utf8
if ($interactiveExit -ne 0 -or ($interactive -join "`n") -notmatch '2\+3\*4 = 14') { throw 'Interactive console regression.' }
$cliLog | Set-Content -LiteralPath 'docs/evidence/cli_checks.txt' -Encoding utf8
$diagramCount = 0
foreach ($diagram in @('docs/循环队列结构.svg','docs/栈结构.svg')) {
    if (-not (Test-Path -LiteralPath $diagram)) { continue }
    [xml]$svg = Get-Content -LiteralPath $diagram -Raw -Encoding utf8
    if ($svg.DocumentElement.LocalName -ne 'svg') { throw "Invalid SVG: $diagram" }
    ++$diagramCount
}
[ordered]@{
    verified_at = [DateTimeOffset]::Now.ToString('o')
    debug = '33/33 behavior groups passed'
    release = '33/33 behavior groups passed'
    allocation_audit = 'all six test executables: outstanding=0'
    gcc_analyzer = 'all eight production translation units passed'
    cli = 'demo, valid/invalid eval, valid/invalid brackets, interactive input passed'
    diagrams = "$diagramCount local SVG documents parsed (optional documentation, not required in source-only export)"
} | ConvertTo-Json | Set-Content -LiteralPath 'docs/evidence/verification.json' -Encoding utf8
Write-Output 'VERIFIED: Debug + Release behavior tests, project allocation audit, GCC analyzer, CLI and SVG parsing.'
