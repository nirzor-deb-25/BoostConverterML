param(
    [string]$ProgrammerCli = "C:\ST\STM32CubeIDE_2.2.0\STM32CubeIDE\plugins\com.st.stm32cube.ide.mcu.externaltools.cubeprogrammer.win32_2.2.500.202603051304\tools\bin\STM32_Programmer_CLI.exe"
)

$ErrorActionPreference = "Stop"

$csvPath = Join-Path $PSScriptRoot "controller_test_results.csv"
$results = @()

if (-not (Test-Path -LiteralPath $ProgrammerCli)) {
    throw "STM32CubeProgrammer CLI was not found at: $ProgrammerCli"
}

function Invoke-Programmer {
    param([string[]]$Arguments)

    $output = @(& $ProgrammerCli @Arguments 2>&1)
    $exitCode = $LASTEXITCODE

    if ($exitCode -ne 0) {
        $output | ForEach-Object { Write-Host $_ }
        throw "STM32CubeProgrammer failed with exit code $exitCode."
    }

    return $output
}

$tests = @(
    [PSCustomObject]@{ Sequence = 5; Measured_mV = 19500; Case = "Below target" },
    [PSCustomObject]@{ Sequence = 6; Measured_mV = 20000; Case = "At target" },
    [PSCustomObject]@{ Sequence = 7; Measured_mV = 20600; Case = "Above target" }
)

foreach ($test in $tests) {
    $fields = @(
        @{ Offset = 4;  Value = 12000 },
        @{ Offset = 8;  Value = $test.Measured_mV },
        @{ Offset = 12; Value = 20000 },
        @{ Offset = 16; Value = 40000 }
    )

    foreach ($field in $fields) {
        $address = "0x{0:X8}" -f (0x20000000 + $field.Offset)
        $value = "0x{0:X8}" -f $field.Value

        $null = Invoke-Programmer @(
            "-c", "port=SWD", "mode=HOTPLUG",
            "-w32", $address, $value
        )
    }

    # Publish the request sequence last, after all input values are written.
    $sequenceValue = "0x{0:X8}" -f $test.Sequence
    $null = Invoke-Programmer @(
        "-c", "port=SWD", "mode=HOTPLUG",
        "-w32", "0x20000000", $sequenceValue
    )

    Start-Sleep -Milliseconds 150

    $readOutput = Invoke-Programmer @(
        "-c", "port=SWD", "mode=HOTPLUG",
        "-r32", "0x20000000", "32"
    )

    $words = @{}

    foreach ($line in $readOutput) {
        $lineText = [string]$line

        if ($lineText -match '^\s*0x([0-9A-Fa-f]{8})\s*:\s*(.*)$') {
            $baseAddress = [Convert]::ToUInt32($Matches[1], 16)
            $wordMatches = [regex]::Matches(
                $Matches[2],
                '\b[0-9A-Fa-f]{8}\b'
            )

            for ($i = 0; $i -lt $wordMatches.Count; $i++) {
                $address = $baseAddress + (4 * $i)
                $words[$address] = [Convert]::ToUInt32(
                    $wordMatches[$i].Value, 16
                )
            }
        }
    }

    if ($words.Count -lt 8) {
        $readOutput | ForEach-Object { Write-Host $_ }
        throw "Could not parse the complete 32-byte mailbox response."
    }

    $responseSequence = $words[[uint32]0x2000001C]

    if ($responseSequence -ne $test.Sequence) {
        throw "Firmware response was not ready: expected sequence $($test.Sequence), received $responseSequence."
    }

    $dutyPermille = $words[[uint32]0x20000014]
    $statusCode = $words[[uint32]0x20000018]

    switch ($statusCode) {
        1 { $statusText = "In range (green)" }
        2 { $statusText = "Below target (orange)" }
        3 { $statusText = "Above target / invalid (red)" }
        default { $statusText = "Unknown status: $statusCode" }
    }

    $results += [PSCustomObject]@{
        Case            = $test.Case
        MeasuredOutputV = [math]::Round($test.Measured_mV / 1000.0, 1)
        DutyCommandPct  = [math]::Round($dutyPermille / 10.0, 1)
        Status          = $statusText
        Request         = $test.Sequence
        Response        = $responseSequence
    }
}

$results | Format-Table -AutoSize
$results | Export-Csv -Path $csvPath -NoTypeInformation -Encoding UTF8
Write-Host "Saved CSV: $csvPath"