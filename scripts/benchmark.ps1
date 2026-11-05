$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
Set-Location -LiteralPath $root
& "$PSScriptRoot/build.ps1"
if (-not $?) { throw 'Release build failed.' }
$raw = & .\build\benchmark.exe
if ($LASTEXITCODE -ne 0) { throw 'Benchmark failed; report not replaced.' }
$rows = $raw | ConvertFrom-Csv
if ($rows.Count -ne 60) { throw 'Expected 10 warmup + 50 measured rows.' }
foreach ($row in $rows) {
    if ($row.checksum -ne '4999950000' -or $row.operations -ne '200000') { throw 'Invalid benchmark checksum/operation count.' }
}
$raw | Set-Content -LiteralPath 'docs/bench_raw.csv' -Encoding utf8
$summary = @($rows | Where-Object phase -eq 'measured' | Group-Object scenario,structure | ForEach-Object {
    $samples = $_.Group
    if ($samples.Count -ne 5) { throw 'Expected 5 measured samples per group.' }
    $times = @($samples | ForEach-Object { [double]::Parse($_.ms, [Globalization.CultureInfo]::InvariantCulture) })
    $stat = $times | Measure-Object -Average -Minimum -Maximum
    [pscustomobject]@{
        scenario = $samples[0].scenario
        structure = $samples[0].structure
        rounds = 5
        mean_ms = $stat.Average.ToString('F6', [Globalization.CultureInfo]::InvariantCulture)
        min_ms = $stat.Minimum.ToString('F6', [Globalization.CultureInfo]::InvariantCulture)
        max_ms = $stat.Maximum.ToString('F6', [Globalization.CultureInfo]::InvariantCulture)
    }
})
$summary | Export-Csv -LiteralPath 'docs/bench_summary.csv' -NoTypeInformation -Encoding utf8
$cpu = Get-CimInstance Win32_Processor | Select-Object -First 1
$computer = Get-CimInstance Win32_ComputerSystem
$os = Get-CimInstance Win32_OperatingSystem
$environment = [ordered]@{
    recorded_at = [DateTimeOffset]::Now.ToString('o')
    compiler = (& gcc --version | Select-Object -First 1)
    compiler_path = (Get-Command gcc).Source
    target = (& gcc -dumpmachine)
    flags = '-std=c11 -O2 -g -Wall -Wextra -Wpedantic -Werror -Wconversion -Wshadow -I.; no LTO; no allocation audit'
    cpu = $cpu.Name
    cores = $cpu.NumberOfCores
    logical_processors = $cpu.NumberOfLogicalProcessors
    machine = "$($computer.Manufacturer) $($computer.Model)"
    memory_gib = [math]::Round($computer.TotalPhysicalMemory / 1GB, 2)
    os = "$($os.Caption) $($os.Version) build $($os.BuildNumber)"
    timer = 'QueryPerformanceCounter (elapsed wall time)'
    pairs = 100000
    api_calls = 200000
    effective_capacity = 64
    warmup_per_group = 1
    measured_rounds_per_group = 5
}
$environment | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath 'docs/bench_environment.json' -Encoding utf8
$lines = @(
    '# 栈与队列性能对比与场景分析',
    '',
    '本表由 `scripts/benchmark.ps1` 根据本机真实运行结果生成。原始记录见 [bench_raw.csv](bench_raw.csv)，汇总见 [bench_summary.csv](bench_summary.csv)，环境见 [bench_environment.json](bench_environment.json)。',
    '',
    '## 实验设置',
    '',
    "- 处理器：$($environment.cpu)；$($environment.cores)核/$($environment.logical_processors)线程；内存$($environment.memory_gib) GiB。",
    "- 机器：$($environment.machine)；系统：$($environment.os)。",
    "- 编译器：$($environment.compiler)；目标：$($environment.target)。",
    '- 使用 C11、-O2，无LTO。计时采用Windows高精度QPC，单位ms；不是CPU占用时间。',
    '- 每次插入100,000个整数，再逐个移除；总计200,000次被测API调用。所有结构重放相同预生成操作序列，插入/移除各占50%。',
    '- alternating：插入1个后立即移除；batch64：插入64个后移除64个，末批不足64时按实际数量。',
    '- 数组结构统一有效容量64：牺牲单元法分配65格，计数器法和顺序栈分配64格；链式结构沿用相同最大64的占用序列。',
    '- 每结构每场景先单独预热1轮，再正式测量5轮；预热数据保留但不计均值。正式轮次全部保留，不按结果挑样本；每轮轮换结构执行顺序。',
    '- 初始化、销毁、序列生成、输出和末尾校验在计时区间外；链式结构每次操作所需malloc/free在计时区间内。所有结构使用同样的适配器调用。',
    '- 累加结果使用uint64_t，期望校验和4,999,950,000；结果输出并写volatile，防止无观察结果的循环被优化消除。计时未启用测试分配审计。',
    '',
    '## 实测数据',
    '',
    '| 场景 | 结构 | 正式轮数 | 平均ms | 最小ms | 最大ms |',
    '|---|---|---:|---:|---:|---:|'
)
foreach ($item in $summary) {
    $lines += "| $($item.scenario) | $($item.structure) | $($item.rounds) | $($item.mean_ms) | $($item.min_ms) | $($item.max_ms) |"
}
$lines += @(
    '',
    '## 分析与适用边界',
    '',
    '顺序结构避免了逐结点申请和释放，连续内存也有利于缓存；链式结构的结果同时包含真实分配器开销。两种循环队列还包含取模、判空判满的成本。因此，不能仅凭“数组”或“链表”预测这里所有结构的具体快慢，应以表内结果为准。',
    '',
    '这些是本机、当前编译选项、两种操作分布下的短时测量；小于数毫秒的结果容易受调度和频率变化影响，表中保留范围。实验不测无限增长、扩容、并发或GUI刷新，不声称链式结构总是更慢。队列FIFO与栈LIFO语义不同，这里对比API成本，不表示它们可以互相替换。',
    '',
    '## 场景选型',
    '',
    '| 场景 | 选择 | 依据与溢出策略 |',
    '|---|---|---|',
    '| 容量可预估的传感器采样缓存 | 循环队列 | FIFO且分配固定。普通入队在满时返回-1；实时曲线只关心近期数据时，可显式调用q_enqueue_overwrite丢弃最旧值。需要完整留档时应拒绝新值并记录丢样/施加背压，不能静默覆盖。 |',
    '| 撤销次数不可预测 | 链栈 | LIFO、按需增长；代价是结点、对齐和分配器开销，仍可能内存不足，push必须检查返回值。若撤销上限固定且性能重要，顺序栈更合适。 |',
    '| 容量波动较大的FIFO任务 | 链式队列 | 无固定数组上限，队尾插入、队头删除均O(1)；必须处理malloc失败并销毁残留结点。 |',
    '',
    '同一物理cap下，牺牲单元法只能存cap-1个，计数器法能存cap个。验证等价行为时必须先统一有效容量；课件中“两个方案都在cap-1时满”的注释仅适用于牺牲单元法。',
    '',
    '再现方法：在工程根目录执行 `powershell -NoProfile -ExecutionPolicy Bypass -File scripts/benchmark.ps1`。此命令会重新生成本页和CSV，具体耗时允许随环境变化。'
)
$lines | Set-Content -LiteralPath 'docs/bench.md' -Encoding utf8
$summary | Format-Table -AutoSize
