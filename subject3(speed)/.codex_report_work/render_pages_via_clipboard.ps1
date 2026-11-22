$ErrorActionPreference = 'Stop'

Add-Type -AssemblyName System.Drawing
Add-Type @'
using System;
using System.Runtime.InteropServices;
public static class ClipEmfPage {
    [DllImport("user32.dll")] public static extern bool OpenClipboard(IntPtr hWndNewOwner);
    [DllImport("user32.dll")] public static extern bool CloseClipboard();
    [DllImport("user32.dll")] public static extern IntPtr GetClipboardData(uint uFormat);
    [DllImport("gdi32.dll", CharSet=CharSet.Unicode)] public static extern IntPtr CopyEnhMetaFile(IntPtr hemfSrc, string lpszFile);
    [DllImport("gdi32.dll")] public static extern bool DeleteEnhMetaFile(IntPtr hemf);
}
'@

function Copy-ClipboardEmfToFile {
    param([Parameter(Mandatory = $true)][string]$Path)

    $opened = $false
    for ($attempt = 0; $attempt -lt 30; $attempt++) {
        if ([ClipEmfPage]::OpenClipboard([IntPtr]::Zero)) {
            $opened = $true
            break
        }
        Start-Sleep -Milliseconds 50
    }
    if (-not $opened) { throw 'OpenClipboard failed' }

    try {
        $sourceHandle = [ClipEmfPage]::GetClipboardData(14)
        if ($sourceHandle -eq [IntPtr]::Zero) { throw 'No CF_ENHMETAFILE on clipboard' }
        $copyHandle = [ClipEmfPage]::CopyEnhMetaFile($sourceHandle, $Path)
        if ($copyHandle -eq [IntPtr]::Zero) { throw 'CopyEnhMetaFile failed' }
        [void][ClipEmfPage]::DeleteEnhMetaFile($copyHandle)
    }
    finally {
        [void][ClipEmfPage]::CloseClipboard()
    }
}

function Convert-EmfToPng {
    param(
        [Parameter(Mandatory = $true)][string]$EmfPath,
        [Parameter(Mandatory = $true)][string]$PngPath
    )

    $metafile = [System.Drawing.Imaging.Metafile]::FromFile($EmfPath)
    try {
        $scale = 0.75
        $width = [Math]::Max(1, [int]($metafile.Width * $scale))
        $height = [Math]::Max(1, [int]($metafile.Height * $scale))
        $bitmap = New-Object System.Drawing.Bitmap $width, $height
        try {
            $graphics = [System.Drawing.Graphics]::FromImage($bitmap)
            try {
                $graphics.Clear([System.Drawing.Color]::White)
                $graphics.CompositingQuality = [System.Drawing.Drawing2D.CompositingQuality]::HighQuality
                $graphics.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
                $graphics.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::HighQuality
                $graphics.DrawImage($metafile, 0, 0, $width, $height)
            }
            finally {
                $graphics.Dispose()
            }
            $bitmap.Save($PngPath, [System.Drawing.Imaging.ImageFormat]::Png)
        }
        finally {
            $bitmap.Dispose()
        }
    }
    finally {
        $metafile.Dispose()
    }
}

$root = Split-Path -Parent $MyInvocation.MyCommand.Path
$docx = Join-Path $root 'final_report_render_only.docx'
$outDir = Join-Path $root 'render_final_pages'
New-Item -ItemType Directory -Path $outDir -Force | Out-Null

$word = $null
$document = $null
try {
    $word = New-Object -ComObject Word.Application
    $word.Visible = $false
    $word.DisplayAlerts = 0
    $word.Options.UpdateLinksAtOpen = $false
    $document = $word.Documents.Open($docx, $false, $true)
    $document.Repaginate()
    $pageCount = $document.ComputeStatistics(2)
    $selection = $word.Selection

    for ($pageNo = 1; $pageNo -le $pageCount; $pageNo++) {
        [void]$selection.GoTo(1, 1, $pageNo)
        $pageRange = $document.Bookmarks.Item('\Page').Range
        try {
            $pageRange.CopyAsPicture()
            Start-Sleep -Milliseconds 200
            $emfPath = Join-Path $outDir ("page-{0}.emf" -f $pageNo)
            $pngPath = Join-Path $outDir ("page-{0}.png" -f $pageNo)
            Copy-ClipboardEmfToFile -Path $emfPath
            Convert-EmfToPng -EmfPath $emfPath -PngPath $pngPath
            Remove-Item -LiteralPath $emfPath -Force
            Write-Output ("PAGE={0}/{1}" -f $pageNo, $pageCount)
        }
        finally {
            [void][Runtime.InteropServices.Marshal]::FinalReleaseComObject($pageRange)
        }
    }

    Write-Output ("RENDERED={0}" -f $pageCount)
}
finally {
    if ($document -ne $null) { $document.Close(0) }
    if ($word -ne $null) { $word.Quit() }
    [GC]::Collect()
    [GC]::WaitForPendingFinalizers()
}
