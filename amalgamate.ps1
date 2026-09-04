$ErrorActionPreference = 'Stop'
$base = Join-Path $PSScriptRoot 'upstream-rapfi2018\AIRapFi'
$parts = @(
    (Join-Path $PSScriptRoot 'rapfi_prelude.inc'),
    (Join-Path $base 'Define.h'),
    (Join-Path $base 'Board.h'),
    (Join-Path $base 'Evaluator.h'),
    (Join-Path $base 'Search.h'),
    (Join-Path $base 'HashTable.h'),
    (Join-Path $base 'MoveDatabase.cpp'),
    (Join-Path $base 'Config.cpp'),
    (Join-Path $base 'Board.cpp'),
    (Join-Path $base 'Evaluator.cpp'),
    (Join-Path $base 'HashTable.cpp'),
    (Join-Path $base 'Search.cpp'),
    (Join-Path $PSScriptRoot 'rapfi_main.inc')
)
$chunks = foreach ($path in $parts) {
    $text = Get-Content -LiteralPath $path -Raw
    $text = $text -replace '(?m)^\s*#pragma once\s*\r?\n', ''
    $text = $text -replace '(?m)^\s*#include\s+"[^"]+"\s*\r?\n', ''
    $text = $text -replace '(?mi)^\s*#include\s+<windows\.h>\s*\r?\n', ''
    "`n/* ===== $([IO.Path]::GetFileName($path)) ===== */`n$text"
}
$out = Join-Path $PSScriptRoot 'src_rapfi_raw.cpp'
[IO.File]::WriteAllText($out, ($chunks -join "`n"), [Text.UTF8Encoding]::new($false))
Write-Output $out
