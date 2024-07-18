param(
    [ValidateSet("14.1", "14.3")]
    [string[]] $InstallToolset = @()
)

function Write-ScriptLog {
    param(
        [Parameter(Mandatory)]
        [ValidateSet("Visual Studio", "Toolset", "Component", "Execute")]
        [string] $Category,

        [Parameter(Mandatory)]
        [string[]] $Message,

        [ValidateSet("Error", "Notice")]
        [string] $Level = "Notice"
    )

    if ($Level -eq "Notice") {
        Write-Host "[$Category] $Message"
    } else {
        Write-Host "[$Category] [$Level] $Message"
    }
}

function Get-LatestVisualStudio {
    $Vswhere = Get-Command vswhere
    Write-ScriptLog "Visual Studio" "Path (vswhere): $($Vswhere.Path)"

    $VisualStudio = New-Object -TypeName PSObject -Property @{
        "InstallationPath" = & $Vswhere.Path -products * -latest -property installationPath
        "InstallationVersion" = & $Vswhere.Path -products * -latest -property installationVersion
    }

    if (-not $VisualStudio.InstallationPath) {
        Write-ScriptLog "Visual Studio" "Failed to find suitable Visual Studio installation" -Level Error
        exit 1
    }

    $Version = $VisualStudio.InstallationVersion.Split(".", 3)

    if ($Version.Count -lt 2) {
        Write-ScriptLog "Visual Studio" "Failed to parse Visual Studio installation version '$($VisualStudio.InstallationVersion)'" -Level Error
        exit 1
    }

    $Major = [int]($Version[0])
    $Minor = [int]($Version[1])

    Write-ScriptLog "Visual Studio" "Found Visual Studio $Major.$Minor"

    if ($Major -eq 16) {
        $VisualStudio | Add-Member -MemberType NoteProperty -Name Year -Value "2019"
        $VisualStudio | Add-Member -MemberType NoteProperty -Name V141Component -Value "Microsoft.VisualStudio.Component.VC.v141.x86.x64"
        $VisualStudio | Add-Member -MemberType NoteProperty -Name V142Component -Value "Microsoft.VisualStudio.Component.VC.14.2$($Version[1]).$($Version[0]).$($Version[1]).x86.x64"
        $VisualStudio | Add-Member -MemberType NoteProperty -Name V143Component -Value $null
    } elseif ($Major -eq 17) {
        $VisualStudio | Add-Member -MemberType NoteProperty -Name Year -Value "2022"
        $VisualStudio | Add-Member -MemberType NoteProperty -Name V141Component -Value "Microsoft.VisualStudio.Component.VC.v141.x86.x64"
        $VisualStudio | Add-Member -MemberType NoteProperty -Name V142Component -Value "Microsoft.VisualStudio.Component.VC.14.29.16.11.x86.x64"
        if ($Minor -lt 10) {
            $VisualStudio | Add-Member -MemberType NoteProperty -Name V143Component -Value "Microsoft.VisualStudio.Component.VC.14.3$($Version[1][-1]).$($Version[0]).$($Version[1]).x86.x64"
        } elseif ($Minor -lt 20) {
            $VisualStudio | Add-Member -MemberType NoteProperty -Name V143Component -Value "Microsoft.VisualStudio.Component.VC.14.4$($Version[1][-1]).$($Version[0]).$($Version[1]).x86.x64"
        } else {
            Write-ScriptLog "Visual Studio" "Failed to match minor version '$($Version[1])'" -Level Error
            exit 5
        }
    } else {
        Write-ScriptLog "Visual Studio" "Unknown Visual Studio major version '$($Version[0])'" -Level Error
        exit 1
    }

    return $VisualStudio
}

$VisualStudio = Get-LatestVisualStudio
Write-ScriptLog "Visual Studio" "Installation Path: $($VisualStudio.InstallationPath)"
Write-ScriptLog "Visual Studio" "Installation Version: $($VisualStudio.InstallationVersion)"

$InstallToolset = $InstallToolset | Sort-Object | Get-Unique

Write-ScriptLog "Toolset" "Install: $($InstallToolset -join ", ")"

$ComponentsToInstall = @()

foreach ($Toolset in $InstallToolset) {
    if ($Toolset -eq "14.1") {
        if (-not $VisualStudio.V141Component) {
            Write-ScriptLog "Component" "No component available for $Toolset in Visual Studio $($VisualStudio.Year)" -Level Error
            exit 2
        }

        $ComponentsToInstall += $VisualStudio.V141Component
    } elseif ($Toolset -eq "14.2") {
        if (-not $VisualStudio.V142Component) {
            Write-ScriptLog "Component" "No component available for $Toolset in Visual Studio $($VisualStudio.Year)" -Level Error
            exit 2
        }

        $ComponentsToInstall += $VisualStudio.V142Component
    } elseif ($Toolset -eq "14.3") {
        if (-not $VisualStudio.V143Component) {
            Write-ScriptLog "Component" "No component available for $Toolset in Visual Studio $($VisualStudio.Year)" -Level Error
            exit 2
        }

        $ComponentsToInstall += $VisualStudio.V143Component
    } else {
        Write-ScriptLog "Component" "Unknown toolset $Toolset" -Level Error
        exit 3
    }
}

if ($ComponentsToInstall.Count -eq 0) {
    Write-ScriptLog "Component" "No components selected for installation"
    exit 0
}

Write-ScriptLog "Component" "Selected components for installation: $($ComponentsToInstall -join ", ")"

try {
    Push-Location "C:\Program Files (x86)\Microsoft Visual Studio\Installer\"

    $CmdArguments = @()

    $CmdArguments += "/c"
    $CmdArguments += "vs_installer.exe"

    $CmdArguments += "modify"
    $CmdArguments += "--installPath"
    $CmdArguments += "`"$($VisualStudio.InstallationPath)`""

    foreach ($Component in $ComponentsToInstall) {
        $CmdArguments += "--add"
        $CmdArguments += $Component
    }

    $CmdArguments += "--quiet"
    $CmdArguments += "--norestart"
    $CmdArguments += "--nocache"

    $CmdArgumentsString = $CmdArguments -join " "

    # command should run twice

    Write-ScriptLog "Execute" "cmd.exe $CmdArgumentsString"
    $process = Start-Process -FilePath cmd.exe -ArgumentList $CmdArguments -Wait -PassThru -WindowStyle Hidden

    Write-ScriptLog "Execute" "cmd.exe $CmdArgumentsString"
    $process = Start-Process -FilePath cmd.exe -ArgumentList $CmdArguments -Wait -PassThru -WindowStyle Hidden
} finally {
    Pop-Location
}
