param(
  [Parameter(Mandatory=$true)][string]$Archive,
  [string]$CardRoot = 'E:\'
)
$ErrorActionPreference = 'Stop'
$archivePath = (Resolve-Path -LiteralPath $Archive).Path
$root = (Resolve-Path -LiteralPath $CardRoot).Path
$volume = Get-Volume -DriveLetter $root.Substring(0,1)
if ($volume.FileSystemLabel -ne 'MIYOO MINI' -or $volume.FileSystem -ne 'FAT32') {
  throw 'Target must be the MIYOO MINI FAT32 card'
}
$expectedHash = ((Get-Content -LiteralPath ($archivePath + '.sha256') -Raw).Trim() -split '\s+')[0]
if ((Get-FileHash -LiteralPath $archivePath -Algorithm SHA256).Hash -ne $expectedHash) {
  throw 'Archive checksum mismatch'
}
$stamp = Get-Date -Format 'yyyyMMdd-HHmmss'
$stage = Join-Path $env:TEMP ('gwent-install-' + $stamp)
Expand-Archive -LiteralPath $archivePath -DestinationPath $stage
$sourceGame = Join-Path $stage 'Roms\PORTS\Games\GwentClassic'
if (!(Test-Path -LiteralPath (Join-Path $sourceGame 'gwent'))) { throw 'Missing game binary' }
$targetGame = Join-Path $root 'Roms\PORTS\Games\GwentClassic'
$backup = Join-Path (Split-Path $archivePath) ('sd-backup-' + $stamp)
if (Test-Path -LiteralPath $targetGame) {
  New-Item -ItemType Directory -Path $backup | Out-Null
  Copy-Item -LiteralPath $targetGame -Destination $backup -Recurse
}
Copy-Item -Path (Join-Path $stage 'Roms') -Destination $root -Recurse -Force
$files = @(Get-ChildItem -LiteralPath (Join-Path $stage 'Roms') -Recurse -File)
foreach ($file in $files) {
  $relative = $file.FullName.Substring($stage.Length + 1)
  $target = Join-Path $root $relative
  if (!(Test-Path -LiteralPath $target) -or
      (Get-FileHash -LiteralPath $file.FullName -Algorithm SHA256).Hash -ne
      (Get-FileHash -LiteralPath $target -Algorithm SHA256).Hash) {
    throw "SD verification failed: $relative"
  }
}
"Installed and SHA-256 verified $($files.Count) files; backup: $backup"
