Add-Type -AssemblyName System.Drawing

$root = Resolve-Path (Join-Path $PSScriptRoot "..")
$outDir = Join-Path $root "docs\screenshots"
New-Item -ItemType Directory -Force -Path $outDir | Out-Null
$outFile = Join-Path $outDir "dashboard.png"

$width = 1440
$height = 1000
$bitmap = New-Object System.Drawing.Bitmap $width, $height
$graphics = [System.Drawing.Graphics]::FromImage($bitmap)
$graphics.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::AntiAlias
$graphics.TextRenderingHint = [System.Drawing.Text.TextRenderingHint]::ClearTypeGridFit

function Brush($hex) {
    return New-Object System.Drawing.SolidBrush ([System.Drawing.ColorTranslator]::FromHtml($hex))
}

function Pen($hex, $size = 1) {
    return New-Object System.Drawing.Pen ([System.Drawing.ColorTranslator]::FromHtml($hex)), $size
}

function Rect($x, $y, $w, $h, $fill, $stroke = "#243244") {
    $graphics.FillRectangle((Brush $fill), $x, $y, $w, $h)
    $graphics.DrawRectangle((New-Object System.Drawing.Pen ([System.Drawing.ColorTranslator]::FromHtml($stroke)), 1), $x, $y, $w, $h)
}

function Text($value, $x, $y, $size, $color = "#f8fafc", $style = "Regular") {
    $fontStyle = [System.Drawing.FontStyle]::$style
    $font = New-Object System.Drawing.Font "Segoe UI", $size, $fontStyle
    $graphics.DrawString($value, $font, (Brush $color), $x, $y)
}

$bg = New-Object System.Drawing.Drawing2D.LinearGradientBrush(
    (New-Object System.Drawing.Rectangle 0, 0, $width, $height),
    ([System.Drawing.ColorTranslator]::FromHtml("#08111f")),
    ([System.Drawing.ColorTranslator]::FromHtml("#172554")),
    35
)
$graphics.FillRectangle($bg, 0, 0, $width, $height)

Rect 0 0 285 $height "#050b16" "#1f2937"
Text "DSA Weather" 30 28 22 "#f8fafc" "Bold"
Text "Analytics Dashboard" 30 58 22 "#f8fafc" "Bold"
Text "C++17 backend with hash maps," 30 102 11 "#94a3b8"
Text "graphs, heaps, stacks, sorting," 30 122 11 "#94a3b8"
Text "and Trie autocomplete." 30 142 11 "#94a3b8"

Text "CITY LOOKUP" 30 195 9 "#94a3b8" "Bold"
Rect 30 220 225 42 "#111827"
Text "Topi" 46 230 12 "#f8fafc"
Text "TRIE AUTOCOMPLETE" 30 286 9 "#94a3b8" "Bold"
Text "GET /api/suggest" 150 286 7 "#38bdf8"
Rect 30 310 225 42 "#111827"
Text "Type a city name" 46 320 11 "#64748b"
Rect 30 365 225 42 "#38bdf8" "#38bdf8"
Text "Load City" 98 374 12 "#082f49" "Bold"

Text "GRAPH NEIGHBORS" 30 446 9 "#94a3b8" "Bold"
Rect 30 472 90 34 "#111827"
Text "Islamabad" 42 480 10 "#f8fafc"
Rect 130 472 90 34 "#111827"
Text "Peshawar" 144 480 10 "#f8fafc"

Text "LIVE API PANELS" 30 548 9 "#94a3b8" "Bold"
Text "GET /api/alerts" 30 578 10 "#38bdf8"
Text "GET /api/route" 30 602 10 "#38bdf8"
Text "GET /api/weather" 30 626 10 "#38bdf8"

Text "Portfolio-ready C++ DSA project" 330 30 13 "#94a3b8" "Bold"
Rect 1190 24 190 34 "#111827"
Text "Static preview mode" 1210 31 10 "#94a3b8"

Rect 330 82 610 265 "#101b2e"
Text "Topi" 360 112 46 "#f8fafc" "Bold"
Text "Windy near 34.07, 72.63" 365 174 14 "#94a3b8"
Text "21 deg" 690 110 46 "#f8fafc" "Bold"
Rect 360 260 122 58 "#0b1220"
Text "WIND" 378 271 8 "#94a3b8" "Bold"
Text "15 km/h" 378 290 14 "#f8fafc" "Bold"
Rect 500 260 122 58 "#0b1220"
Text "HUMIDITY" 518 271 8 "#94a3b8" "Bold"
Text "45%" 518 290 14 "#f8fafc" "Bold"
Rect 640 260 122 58 "#0b1220"
Text "AQI" 658 271 8 "#94a3b8" "Bold"
Text "45" 658 290 14 "#f8fafc" "Bold"
Rect 780 260 122 58 "#0b1220"
Text "RAIN" 798 271 8 "#94a3b8" "Bold"
Text "0 mm" 798 290 14 "#f8fafc" "Bold"

Rect 960 82 420 265 "#101b2e"
Text "VECTOR TIME SERIES" 990 112 10 "#94a3b8" "Bold"
$points = @(
    @(990,285), @(1028,265), @(1066,240), @(1104,220), @(1142,202), @(1180,185),
    @(1218,176), @(1256,188), @(1294,210), @(1332,232), @(1360,252)
)
$chartPen = New-Object System.Drawing.Pen ([System.Drawing.ColorTranslator]::FromHtml("#38bdf8")), 4
for ($i = 0; $i -lt $points.Length - 1; $i++) {
    $graphics.DrawLine($chartPen, $points[$i][0], $points[$i][1], $points[$i + 1][0], $points[$i + 1][1])
}
Text "Min 17 deg" 990 302 10 "#94a3b8"
Text "Max 25 deg" 1290 302 10 "#94a3b8"

Rect 330 372 500 220 "#101b2e"
Text "PRIORITY QUEUE ALERTS" 360 402 10 "#94a3b8" "Bold"
Text "GET /api/alerts" 690 402 7 "#38bdf8"
Text "Multan" 360 444 13 "#f8fafc" "Bold"
Text "Heat advisory: high temperature trend" 360 466 11 "#94a3b8"
Text "Risk 9/10" 730 448 11 "#ef4444" "Bold"
Text "Lahore" 360 510 13 "#f8fafc" "Bold"
Text "Air quality warning: reduce outdoor exposure" 360 532 11 "#94a3b8"
Text "Risk 8/10" 730 514 11 "#ef4444" "Bold"

Rect 850 372 530 220 "#101b2e"
Text "PARTIAL SORT RANKINGS" 880 402 10 "#94a3b8" "Bold"
Text "GET /api/hottest" 1200 402 7 "#38bdf8"
$cities = @("1. Multan        38 deg", "2. Hyderabad     37 deg", "3. Karachi       35 deg", "4. Faisalabad    34 deg")
for ($i = 0; $i -lt $cities.Length; $i++) {
    Rect 880 (435 + ($i * 36)) 455 28 "#0b1220"
    Text $cities[$i] 895 (438 + ($i * 36)) 11 "#f8fafc"
}

Rect 330 618 500 245 "#101b2e"
Text "GRAPH ROUTE RESULT" 360 648 10 "#94a3b8" "Bold"
Text "GET /api/route" 690 648 7 "#38bdf8"
Rect 360 680 100 32 "#0b1220"
Text "Topi" 378 687 10 "#f8fafc"
Rect 470 680 100 32 "#0b1220"
Text "Karachi" 488 687 10 "#f8fafc"
Rect 580 680 140 32 "#0b1220"
Text "Dijkstra" 598 687 10 "#f8fafc"
Rect 730 680 70 32 "#1f2937"
Text "Find" 750 687 10 "#f8fafc" "Bold"
Rect 360 724 440 64 "#0b1220"
Text "Topi -> Islamabad -> Lahore -> Multan -> Hyderabad -> Karachi" 378 738 10 "#f8fafc" "Bold"
Text "Dijkstra lowest weather risk | 5 hops | risk 18 | 920 km" 378 762 10 "#94a3b8"
Text "Topi" 610 814 10 "#f8fafc"
Text "Islamabad" 682 792 10 "#f8fafc"
Text "Lahore" 714 834 10 "#f8fafc"
Text "Multan" 585 852 10 "#f8fafc"
Text "Karachi" 448 852 10 "#f8fafc"

Rect 850 618 530 245 "#101b2e"
Text "GENERATED 10-DAY FORECAST" 880 648 10 "#94a3b8" "Bold"
for ($i = 0; $i -lt 5; $i++) {
    Rect (880 + ($i * 96)) 690 82 115 "#0b1220"
    Text @("Mon","Tue","Wed","Thu","Fri")[$i] (895 + ($i * 96)) 706 13 "#f8fafc" "Bold"
    Text "Windy" (895 + ($i * 96)) 736 10 "#94a3b8"
    Text ((24 + $i).ToString() + " / " + (17 + $i).ToString() + " deg") (895 + ($i * 96)) 760 9 "#94a3b8"
}

$bitmap.Save($outFile, [System.Drawing.Imaging.ImageFormat]::Png)
$graphics.Dispose()
$bitmap.Dispose()
Write-Host "Generated $outFile"
