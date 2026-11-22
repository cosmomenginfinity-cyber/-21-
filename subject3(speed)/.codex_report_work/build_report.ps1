$ErrorActionPreference = 'Stop'

$templatePath = 'C:\Users\Legion\Downloads\山东大学-龙山微光-缩微光电组.docx'
$workRoot = (Resolve-Path '.codex_report_work').Path
$outputPath = Join-Path $workRoot '山东大学_隆山飞驰_卡丁快跑本科组_技术报告.docx'
$workingCopyPath = Join-Path $workRoot '山东大学技术报告模板_工作副本.docx'
$logoPath = Join-Path $workRoot 'image1.png'

Copy-Item -LiteralPath $templatePath -Destination $workingCopyPath -Force
Unblock-File -LiteralPath $workingCopyPath
if (Test-Path -LiteralPath $outputPath) {
    Remove-Item -LiteralPath $outputPath -Force
}

$word = New-Object -ComObject Word.Application
$word.Visible = $false
$word.DisplayAlerts = 0
$word.ScreenUpdating = $false
$doc = $null

function Set-RunFont {
    param($Range, [string]$FarEast = '宋体', [string]$Latin = 'Times New Roman', [double]$Size = 12, [int]$Bold = 0, [int]$Italic = 0, [int]$Color = 0)
    try { $Range.Font.NameFarEast = $FarEast } catch {}
    $Range.Font.Name = $Latin
    $Range.Font.Size = $Size
    $Range.Font.Bold = $Bold
    $Range.Font.Italic = $Italic
    $Range.Font.Color = $Color
}

function Set-ParagraphLayout {
    param($Range, [int]$Align = 3, [double]$FirstIndent = 24, [double]$SpaceBefore = 0, [double]$SpaceAfter = 0, [double]$LineSpacing = 18, [int]$KeepWithNext = 0)
    $Range.ParagraphFormat.Alignment = $Align
    $Range.ParagraphFormat.FirstLineIndent = $FirstIndent
    $Range.ParagraphFormat.LeftIndent = 0
    $Range.ParagraphFormat.RightIndent = 0
    $Range.ParagraphFormat.SpaceBefore = $SpaceBefore
    $Range.ParagraphFormat.SpaceAfter = $SpaceAfter
    $Range.ParagraphFormat.LineSpacingRule = 4
    $Range.ParagraphFormat.LineSpacing = $LineSpacing
    $Range.ParagraphFormat.KeepWithNext = $KeepWithNext
    $Range.ParagraphFormat.WidowControl = -1
}

function Add-Paragraph {
    param(
        [string]$Text,
        [string]$Font = '宋体',
        [double]$Size = 12,
        [int]$Bold = 0,
        [int]$Italic = 0,
        [int]$Align = 3,
        [double]$FirstIndent = 24,
        [double]$SpaceBefore = 0,
        [double]$SpaceAfter = 0,
        [double]$LineSpacing = 18,
        [int]$Color = 0,
        [int]$KeepWithNext = 0
    )
    $script:sel.EndKey(6) | Out-Null
    $start = $script:sel.Start
    $script:sel.TypeText($Text)
    $script:sel.TypeParagraph()
    $finish = $script:sel.Start
    $range = $script:doc.Range($start, [Math]::Max($start, $finish - 1))
    Set-RunFont $range $Font 'Times New Roman' $Size $Bold $Italic $Color
    Set-ParagraphLayout $range $Align $FirstIndent $SpaceBefore $SpaceAfter $LineSpacing $KeepWithNext
    return $range
}

function Add-BlankLine {
    $script:sel.EndKey(6) | Out-Null
    $script:sel.TypeParagraph()
}

function Add-PageBreak {
    $script:sel.EndKey(6) | Out-Null
    $script:sel.InsertBreak(7)
}

function Add-SectionBreak {
    $script:sel.EndKey(6) | Out-Null
    $script:sel.InsertBreak(2)
}

function Add-Heading {
    param([string]$Text, [int]$Level)
    $size = 15
    $align = 0
    $before = 10
    $after = 6
    if ($Level -eq 1) {
        $size = 18
        $align = 1
        $before = 0
        $after = 14
    } elseif ($Level -eq 2) {
        $size = 15
        $align = 0
        $before = 12
        $after = 6
    } else {
        $size = 12
        $align = 0
        $before = 8
        $after = 4
    }
    $range = Add-Paragraph -Text $Text -Font '黑体' -Size $size -Bold -1 -Align $align -FirstIndent 0 -SpaceBefore $before -SpaceAfter $after -LineSpacing 20 -KeepWithNext -1
    $range.ParagraphFormat.OutlineLevel = $Level
    return $range
}

function Add-Chapter {
    param([string]$Text, [switch]$First)
    if (-not $First) { Add-PageBreak }
    Add-Heading $Text 1 | Out-Null
}

function Add-Caption {
    param([string]$Text)
    Add-Paragraph -Text $Text -Font '宋体' -Size 10.5 -Align 1 -FirstIndent 0 -SpaceBefore 4 -SpaceAfter 8 -LineSpacing 14 | Out-Null
}

function Add-Equation {
    param([string]$Text, [string]$Number)
    $display = $Text
    if ($Number) { $display = $Text + (' ' * 8) + $Number }
    Add-Paragraph -Text $display -Font 'Times New Roman' -Size 11.5 -Italic 0 -Align 1 -FirstIndent 0 -SpaceBefore 5 -SpaceAfter 5 -LineSpacing 16 | Out-Null
}

function Add-FigurePlaceholder {
    param([string]$Label, [string]$Description, [double]$HeightCm = 5.4)
    $script:sel.EndKey(6) | Out-Null
    $table = $script:doc.Tables.Add($script:sel.Range, 1, 1)
    $table.AllowAutoFit = $false
    $table.PreferredWidthType = 2
    $table.PreferredWidth = 100
    $table.Rows.Item(1).HeightRule = 2
    $table.Rows.Item(1).Height = $script:word.CentimetersToPoints($HeightCm)
    $table.Rows.Item(1).AllowBreakAcrossPages = 0
    $cell = $table.Cell(1, 1)
    $cell.VerticalAlignment = 1
    $cell.Shading.BackgroundPatternColor = 16119285
    foreach ($border in @(-1,-2,-3,-4)) {
        $cell.Borders.Item($border).LineStyle = 1
        $cell.Borders.Item($border).Color = 8421504
    }
    $cell.Range.Text = "【图位：$Description】`r请在此处插入清晰图片，并按页面宽度等比例裁切。"
    Set-RunFont $cell.Range '宋体' 'Times New Roman' 11 0 0 8421504
    $cell.Range.ParagraphFormat.Alignment = 1
    $script:sel.SetRange($table.Range.End, $table.Range.End)
    $script:sel.TypeParagraph()
    Add-Caption "$Label  $Description"
}

function Add-GridTable {
    param([object[]]$Headers, [object[]]$Rows, [string]$Caption = '')
    $script:sel.EndKey(6) | Out-Null
    $rowCount = $Rows.Count + 1
    $colCount = $Headers.Count
    $table = $script:doc.Tables.Add($script:sel.Range, $rowCount, $colCount)
    $table.Borders.Enable = 1
    $table.AllowAutoFit = $true
    $table.AutoFitBehavior(2)
    $table.Rows.Item(1).HeadingFormat = -1
    for ($c = 1; $c -le $colCount; $c++) {
        $table.Cell(1, $c).Range.Text = [string]$Headers[$c - 1]
        $table.Cell(1, $c).Shading.BackgroundPatternColor = 14277081
    }
    for ($r = 0; $r -lt $Rows.Count; $r++) {
        $row = @($Rows[$r])
        for ($c = 0; $c -lt $colCount; $c++) {
            $table.Cell($r + 2, $c + 1).Range.Text = [string]$row[$c]
        }
    }
    for ($r = 1; $r -le $rowCount; $r++) {
        $table.Rows.Item($r).AllowBreakAcrossPages = 0
        for ($c = 1; $c -le $colCount; $c++) {
            $cr = $table.Cell($r, $c).Range
            Set-RunFont $cr '宋体' 'Times New Roman' 10.5 $(if($r -eq 1){-1}else{0}) 0 0
            $cr.ParagraphFormat.Alignment = $(if($r -eq 1){1}else{0})
            $cr.ParagraphFormat.SpaceAfter = 0
            $cr.ParagraphFormat.LineSpacingRule = 0
        }
    }
    $script:sel.SetRange($table.Range.End, $table.Range.End)
    $script:sel.TypeParagraph()
    if ($Caption) { Add-Caption $Caption }
}

function Add-Callout {
    param([string]$Title, [string]$Text)
    $script:sel.EndKey(6) | Out-Null
    $table = $script:doc.Tables.Add($script:sel.Range, 2, 1)
    $table.Borders.Enable = 1
    $table.PreferredWidthType = 2
    $table.PreferredWidth = 100
    $table.Cell(1,1).Range.Text = $Title
    $table.Cell(1,1).Shading.BackgroundPatternColor = 14277081
    $table.Cell(2,1).Range.Text = $Text
    Set-RunFont $table.Cell(1,1).Range '黑体' 'Times New Roman' 11 -1 0 0
    Set-RunFont $table.Cell(2,1).Range '宋体' 'Times New Roman' 10.5 0 0 0
    $table.Cell(1,1).Range.ParagraphFormat.Alignment = 0
    $table.Cell(2,1).Range.ParagraphFormat.Alignment = 3
    $table.Cell(2,1).Range.ParagraphFormat.FirstLineIndent = 21
    $script:sel.SetRange($table.Range.End, $table.Range.End)
    $script:sel.TypeParagraph()
}

try {
    # Use a clean Word document container while reproducing the distilled
    # template's exact A4 measurements, visual hierarchy and SDU identity.
    # The untouched template working copy remains beside the output.
    $doc = $word.Documents.Add()
    $script:doc = $doc
    $script:word = $word
    try { $word.Options.Pagination = $false } catch {}

    $section = $doc.Sections.Item(1)
    $section.PageSetup.PaperSize = 7
    $section.PageSetup.Orientation = 0
    $section.PageSetup.TopMargin = 113.4
    $section.PageSetup.BottomMargin = 90.7
    $section.PageSetup.LeftMargin = 90.7
    $section.PageSetup.RightMargin = 90.7
    $section.PageSetup.HeaderDistance = 42.5
    $section.PageSetup.FooterDistance = 42.5
    $section.PageSetup.DifferentFirstPageHeaderFooter = -1
    foreach ($i in 1..3) {
        try { $section.Headers.Item($i).Range.Text = '' } catch {}
        try { $section.Footers.Item($i).Range.Text = '' } catch {}
    }

    $script:sel = $word.Selection
    $sel.SetRange(0,0)

    # -------------------------- 封面 --------------------------
    Add-BlankLine
    Add-Paragraph -Text '第二十一届全国大学生' -Font '华文中宋' -Size 22 -Bold -1 -Align 1 -FirstIndent 0 -SpaceAfter 0 -LineSpacing 25 | Out-Null
    Add-Paragraph -Text '智能汽车竞赛' -Font '华文中宋' -Size 22 -Bold -1 -Align 1 -FirstIndent 0 -SpaceAfter 22 -LineSpacing 25 | Out-Null
    Add-Paragraph -Text '技  术  报  告' -Font '黑体' -Size 34 -Bold -1 -Align 1 -FirstIndent 0 -SpaceBefore 10 -SpaceAfter 10 -LineSpacing 38 | Out-Null

    $sel.EndKey(6) | Out-Null
    $logo = $sel.InlineShapes.AddPicture($logoPath, $false, $true)
    $logo.LockAspectRatio = -1
    $logo.Width = $word.CentimetersToPoints(4.2)
    $logo.Range.ParagraphFormat.Alignment = 1
    $sel.SetRange($logo.Range.End, $logo.Range.End)
    $sel.TypeParagraph()
    Add-BlankLine

    $coverHeaders = @('项  目', '内  容')
    $coverRows = @(
        ,@('学    校', '山东大学'),
        ,@('队伍名称', '隆山飞驰'),
        ,@('比赛组别', '卡丁快跑（本科）'),
        ,@('参赛队员', '【姓名1】  【姓名2】  【姓名3】'),
        ,@('带队教师', '【指导教师姓名】')
    )
    Add-GridTable $coverHeaders $coverRows
    $coverTable = $doc.Tables.Item($doc.Tables.Count)
    $coverTable.Borders.Enable = 0
    for ($r = 1; $r -le $coverTable.Rows.Count; $r++) {
        $coverTable.Cell($r,1).Range.ParagraphFormat.Alignment = 2
        $coverTable.Cell($r,2).Range.ParagraphFormat.Alignment = 0
        Set-RunFont $coverTable.Cell($r,1).Range '宋体' 'Times New Roman' 14 0 0 0
        Set-RunFont $coverTable.Cell($r,2).Range '宋体' 'Times New Roman' 14 0 0 0
    }
    Add-BlankLine
    Add-Paragraph -Text '二〇二六年八月【日】' -Font '宋体' -Size 14 -Align 1 -FirstIndent 0 -SpaceBefore 8 -LineSpacing 20 | Out-Null

    # -------------------------- 授权页 --------------------------
    Add-PageBreak
    Add-Paragraph -Text '关于技术报告和研究成果的使用授权说明' -Font '黑体' -Size 18 -Bold -1 -Align 1 -FirstIndent 0 -SpaceBefore 12 -SpaceAfter 18 -LineSpacing 24 | Out-Null
    Add-Paragraph -Text '本人完全了解第二十一届全国大学生智能汽车竞赛有关保留、使用技术报告和研究成果的规定，即参赛作品的知识产权归参赛者本人所有，竞赛组委会和山东大学可保存、汇编并在竞赛交流范围内使用本报告。' | Out-Null
    Add-Paragraph -Text '本报告所述方案由山东大学“隆山飞驰”队围绕卡丁快跑（本科）组任务独立完成。除文中明确引用的资料和逐飞 TC387 开源库外，报告不包含他人已经发表或撰写的研究成果。' | Out-Null
    Add-Paragraph -Text '参赛队员签名：____________________    日期：______年____月____日' -Align 0 -FirstIndent 0 -SpaceBefore 24 -SpaceAfter 14 | Out-Null
    Add-Paragraph -Text '带队教师签名：____________________    日期：______年____月____日' -Align 0 -FirstIndent 0 -SpaceBefore 8 -SpaceAfter 14 | Out-Null

    # -------------------------- 摘要 --------------------------
    Add-PageBreak
    Add-Paragraph -Text '摘  要' -Font '黑体' -Size 18 -Bold -1 -Align 1 -FirstIndent 0 -SpaceAfter 16 -LineSpacing 24 | Out-Null
    Add-Paragraph -Text '本文介绍山东大学隆山飞驰队面向第二十一届全国大学生智能汽车竞赛卡丁快跑（本科）组设计的可载人智能车系统。整车以 Infineon AURIX TC387 多核微控制器为控制核心，在有限算力下完成视觉引导员检测、车辆纵横向闭环、去程路径在线记录、二维惯导倒车返航、语音任务和灯光显示。系统采用单路 SCC8660 彩色摄像头采集 160×120 RGB565 图像，CPU2 独立完成色彩分割、形态学处理、连通域提取和目标连续匹配，CPU0 在 100 Hz 控制中断中完成跟随控制和执行器闭环。' | Out-Null
    Add-Paragraph -Text '针对人员携带亮绿色横向 A4 色块时的距离波动与高速转向问题，系统以目标外接框高度代替像素面积测距，并使用基于视觉采样周期的距离—相对速度 α-β 状态滤波。纵向控制器根据距离误差与相对速度生成加速度命令，在 100 Hz 周期内积分为唯一目标速度，避免视觉加速度环与底盘速度斜坡重复限速。横向控制采用车辆坐标系下的直接目标 Pure Pursuit，前视距离随实测车速在 1.00～1.80 m 内变化；小横向误差区降低中心更新系数和转向增益，大误差区平滑恢复完整响应。目标短时丢失时先保持、再线性减速，最终失效停车，从而兼顾连贯性和可控性。' | Out-Null
    Add-Paragraph -Text '返航阶段以编码器里程和 IMU660RA 航向角积分得到平面坐标，按 0.20 m 间隔记录去程轨迹；返航时将记录点逆序读入并对齐到当前坐标系，使用二维 XY 投影、动态前视 Pure Pursuit 和曲率约束速度规划完成倒车复现。工程同时将无线示波器、IPS200 菜单、语音网络状态和 TLD7002 点阵灯板纳入多核协同，为现场调参与比赛流程提供支持。' | Out-Null
    Add-Paragraph -Text '关键词：AURIX TC387；机器视觉；色块跟随；Pure Pursuit；距离速度闭环；二维惯导返航；多核协同' -FirstIndent 0 -SpaceBefore 10 | Out-Null

    # -------------------------- 目录 --------------------------
    Add-PageBreak
    Add-Paragraph -Text '目  录' -Font '黑体' -Size 18 -Bold -1 -Align 1 -FirstIndent 0 -SpaceAfter 14 -LineSpacing 24 | Out-Null
    foreach ($tocLine in @(
        '第一章  系统任务与总体方案',
        '第二章  车模机械结构与硬件总体设计',
        '第三章  多核软件架构与任务调度',
        '第四章  视觉目标检测与跟随控制',
        '第五章  转向与后轮运动控制',
        '第六章  路径记录与二维惯导返航',
        '第七章  语音任务、灯板与人机交互',
        '第八章  调试方法、参数整定与试验验证',
        '第九章  总结与展望',
        '致谢',
        '参考文献',
        '附录  图纸与实物资料占位'
    )) {
        Add-Paragraph -Text $tocLine -Size 11 -Align 0 -FirstIndent 0 -SpaceAfter 5 -LineSpacing 16 | Out-Null
    }

    # -------------------------- 正文节 --------------------------
    Add-SectionBreak
    $bodySection = $doc.Sections.Item($doc.Sections.Count)
    $bodySection.PageSetup.PaperSize = 7
    $bodySection.PageSetup.Orientation = 0
    $bodySection.PageSetup.TopMargin = 85.0
    $bodySection.PageSetup.BottomMargin = 72.0
    $bodySection.PageSetup.LeftMargin = 90.7
    $bodySection.PageSetup.RightMargin = 90.7
    $bodySection.PageSetup.HeaderDistance = 42.5
    $bodySection.PageSetup.FooterDistance = 42.5
    $bodySection.Headers.Item(1).LinkToPrevious = $false
    $bodySection.Footers.Item(1).LinkToPrevious = $false
    $bodySection.Headers.Item(1).Range.Text = '第二十一届全国大学生智能汽车竞赛技术报告'
    Set-RunFont $bodySection.Headers.Item(1).Range '宋体' 'Times New Roman' 9 0 0 8421504
    $bodySection.Headers.Item(1).Range.ParagraphFormat.Alignment = 1
    $bodySection.Footers.Item(1).PageNumbers.RestartNumberingAtSection = $true
    $bodySection.Footers.Item(1).PageNumbers.StartingNumber = 1
    $bodySection.Footers.Item(1).PageNumbers.Add(2, $true) | Out-Null

    # ========================== 第一章 ==========================
    Add-Chapter '第一章  系统任务与总体方案' -First
    Add-Heading '1.1 任务分析' 2 | Out-Null
    Add-Paragraph -Text '卡丁快跑（本科）组要求车辆在引导员带领下完成去程，并依据现场记录的信息自主返回，随后完成停车区语音与灯光任务。该任务同时考察动态目标识别、可载人底盘的稳定操纵、有限场地内的转弯能力、路径记录与复现，以及现场操作流程的可靠性。与小型车模相比，本车轴距、轮距和惯量更大，执行器延迟、地面附着和支架振动对控制效果的影响更明显。' | Out-Null
    Add-Paragraph -Text '设计目标不是单纯提高瞬时速度，而是在目标距离、视野、转弯半径与返航可重复性之间取得平衡。去程应使车辆与引导员尽量维持设定距离，转向不过度切弦；返航应复现去程二维轨迹并根据曲率提前限速；语音任务应在操作界面中给出明确倒计时和执行反馈。' | Out-Null

    Add-Heading '1.2 总体技术路线' 2 | Out-Null
    Add-Paragraph -Text '系统采用“视觉去程—在线记录—二维返航—语音停车”的流程。去程由彩色相机检测引导员身上的高饱和度色块，视觉结果转换为后轴坐标系中的目标点，纵向加速度控制和横向 Pure Pursuit 同时工作；CPU0 同步记录编码器里程、IMU 航向角与积分坐标。去程结束后优先从 RAM 逆序读取轨迹，必要时也可从 Flash 恢复，返航控制器沿记录轨迹倒车。到达起点后系统回到菜单并可直接进入停车语音任务。' | Out-Null
    Add-GridTable @('层级','主要输入','核心处理','主要输出') @(
        ,@('感知层','SCC8660 RGB565 图像','HSV 分割、形态学、连通域、连续匹配','中心、宽高、面积、时间戳'),
        ,@('估计层','目标框高度、中心与帧间隔','高度测距、α-β 距离/相对速度滤波、坐标变换','距离、相对速度、目标 X/Y'),
        ,@('决策层','目标 X/Y、实测速度、历史状态','加速度积分、动态前视 PP、丢帧迟滞','目标车速、真实前轮角'),
        ,@('执行层','目标车速、目标前轮角','前轮角度 PD、后轮增量 PI、几何差速','转向与双后轮 PWM'),
        ,@('返航层','记录轨迹、IMU 航向、编码器里程','XY 投影、倒车 PP、曲率速度规划','返航转角与速度')
    ) '表1-1  系统分层与信息流'

    Add-FigurePlaceholder '图1-1' '整车系统总体框图（感知—控制—执行—返航—语音）' 5.6

    Add-Heading '1.3 方案特点' 2 | Out-Null
    Add-Paragraph -Text '本方案的主要特点包括：利用 TC387 四核并行分担实时任务；使用目标框高度完成距离估计，降低色块姿态导致的面积波动；通过连续目标匹配减少环境绿色物体引发的目标切换；将视觉测量周期与 100 Hz 控制周期分离；以单一加速度积分生成跟随速度，避免二次斜坡造成相位迟滞；以二维 XY 轨迹和曲率速度规划提高返航弯道复现能力。' | Out-Null

    # ========================== 第二章 ==========================
    Add-Chapter '第二章  车模机械结构与硬件总体设计'
    Add-Heading '2.1 底盘与驱动形式' 2 | Out-Null
    Add-Paragraph -Text '车辆采用前轮独立转向、双后轮驱动的可载人卡丁底盘。当前工程中前后轴距取 0.58 m，左右后轮轮距取 0.50 m，后轮直径取 0.24 m。前轮由 24 V、150 W 有刷直流电机经机械机构驱动，实际由 36 V 支路供电；绝对编码器测量转向轴位置，软件以 0.59 的机构比例换算真实前轮角。修改后的限位结构允许真实轮角达到 ±32°。' | Out-Null
    Add-Paragraph -Text '左右后轮分别配备增量编码器和 H 桥驱动。双轮速度闭环使车辆在转弯时仍能按几何关系分配内外轮速度，减小单轮饱和和机械拖曳。车辆外形、质量、质心和转向机构照片尚未由用户提供，报告在附录中保留了整车三视图与关键机械结构图位。' | Out-Null
    Add-FigurePlaceholder '图2-1' '整车正视、侧视与俯视照片' 6.0

    Add-Heading '2.2 摄像头与安装结构' 2 | Out-Null
    Add-Paragraph -Text '视觉传感器为一路 SCC8660 彩色摄像头，当前工程并未并行使用第二路相机。相机安装在车头前部，图像分辨率为 160×120，数据格式为 RGB565，经 P00.0～P00.7 并行数据线、P02.1 像素时钟、P02.0 场同步和 DMA5 传输到 TC387。配置帧率为 60，像素时钟分频为 1/2，实际可用处理帧率还取决于 DMA 和 CPU2 全帧处理时间。' | Out-Null
    Add-Paragraph -Text '相机支架应同时满足高位视场与抗振要求。实际调试采用加粗双立柱、平台连接和前后左右拉线约束以提高刚度；最终结构应避免长悬臂的一阶弯曲模态进入车辆转向频带。相机俯仰角、光轴高度和后轴外参需要在支架定型后重新测量，并与视觉头文件中的外参一致。' | Out-Null
    Add-FigurePlaceholder '图2-2' '摄像头安装位置、加固支架及视场示意' 5.8

    Add-Heading '2.3 供电与功率驱动' 2 | Out-Null
    Add-Paragraph -Text '整车使用两块额定 24 V、15000 mAh 锂电池串联形成 48 V 母线。分电板将 48 V 直接提供给后轮电调，同时采用两路 LM5116 降压：一路输出 24 V 为主板及辅助负载供电，另一路输出 36 V 为前轮转向电调供电。前、后驱动均为 H 桥拓扑，门极驱动采用 2ED2184，MOSFET 按电压、电流和热裕量选型。' | Out-Null
    Add-Paragraph -Text '功率系统应在 PCB 中配置输入保险、反接/浪涌抑制、母线电容、门极电阻、栅极下拉、电流回路最小化和逻辑地/功率地单点连接。相机、IMU 与无线模块的电源需与电机大电流回路分区布置，并在连接器附近配置去耦，以降低高速换相对图像和串口通信的影响。' | Out-Null
    Add-FigurePlaceholder '图2-3' '48 V 分电及 LM5116 24 V/36 V 电源原理图' 6.2
    Add-FigurePlaceholder '图2-4' '2ED2184 前轮 H 桥驱动原理图及 PCB' 6.2

    Add-Heading '2.4 控制板与外设接口' 2 | Out-Null
    Add-GridTable @('模块','接口/资源','用途') @(
        ,@('AURIX TC387','四核、DMA、CCU6、GTM、ASCLIN','视觉、控制、返航、通信与灯板协同'),
        ,@('SCC8660','UART1 配置；P00 并口；DMA5','彩色目标图像采集'),
        ,@('IMU660RA','SPI0','角速度、姿态和航向积分'),
        ,@('绝对编码器','软件 SPI','转向轴绝对角度闭环'),
        ,@('双后轮编码器','TIM 编码器接口','左右轮速度与总里程'),
        ,@('IPS200','SPI2','菜单、视觉与返航状态显示'),
        ,@('无线串口','UART2','逐飞助手 7 通道示波器'),
        ,@('SBUS 接收机','UART10','遥控模式与人工接管'),
        ,@('TLD7002 点阵','UART1，CPU3 运行期复用','语音任务灯光/字符显示')
    ) '表2-1  主要硬件与接口分配'
    Add-FigurePlaceholder '图2-5' '主控板原理图' 6.4
    Add-FigurePlaceholder '图2-6' '主控板 PCB 正反面' 6.0

    # ========================== 第三章 ==========================
    Add-Chapter '第三章  多核软件架构与任务调度'
    Add-Heading '3.1 四核任务划分' 2 | Out-Null
    Add-Paragraph -Text '工程采用 TC387 四核分工以隔离高带宽视觉处理和强实时底盘控制。CPU0 完成时钟、底盘、编码器、绝对转角、IMU、IPS200、SBUS 和相机初始化，并在 10 ms CCU60_CH0 中断中执行全部车辆闭环。CPU1 初始化无线串口与逐飞助手，每 10 ms 发送 7 通道诊断数据。CPU2 不初始化外设，只在主循环中轮询新帧并执行色彩视觉处理。CPU3 在所有核心初始化完成后接管 UART1 的 TLD7002 引脚，运行点阵扫描和语音灯光服务。' | Out-Null
    Add-GridTable @('核心','主任务','典型周期/触发') @(
        ,@('CPU0','初始化、菜单、相机中断、100 Hz 底盘闭环、路径记录/返航','控制 10 ms；菜单轮询'),
        ,@('CPU1','无线示波器与返航诊断数据发送','10 ms'),
        ,@('CPU2','RGB565→HSV、形态学、连通域和目标发布','新图像到达即处理'),
        ,@('CPU3','TLD7002 初始化、点阵显示与语音灯光执行','主循环 1 ms；点阵同步中断')
    ) '表3-1  TC387 四核任务分工'
    Add-FigurePlaceholder '图3-1' '四核软件数据流和外设归属图' 6.0

    Add-Heading '3.2 中断与时间基准' 2 | Out-Null
    Add-Paragraph -Text '底盘主控制频率为 100 Hz。每个控制拍先更新编码器和 IMU，再依据当前模式执行遥控、视觉跟随、返航或停车逻辑。相机 VSYNC 与 DMA5 完成中断均路由至 CPU0；无线 UART2 中断路由至 CPU1；TLD7002 的 UART1 和点阵同步服务路由至 CPU3。语音采样在任务启动时使用独立的 125 μs 定时节拍。' | Out-Null
    Add-Paragraph -Text '所有核心通过 cpu_wait_event_ready() 建立启动屏障。CPU0 完成长时间 IMU 静止标定和外设初始化后，各核心才进入运行循环，避免 CPU2 读取未就绪的相机缓冲或 CPU3 提前切换 UART1 引脚。系统毫秒计数由 10 ms 控制中断递增，因此控制相关时间戳分辨率为 10 ms。' | Out-Null

    Add-Heading '3.3 跨核数据一致性' 2 | Out-Null
    Add-Paragraph -Text 'CPU2 通过全局地址访问 CPU0 DSPR 中的相机完成标志和原始图像，将一帧 38400 B 数据复制到 CPU2 私有缓冲后处理。处理期间新到达的完成标志被主动丢弃，系统以“最新帧优先”控制延迟，而不建立图像队列。视觉结果发布采用奇偶 guard 与 __dsync() 的轻量顺序锁：CPU2 先将 guard 置为奇数、写入完整结果，再置为偶数；CPU0 仅在两次 guard 一致且为偶数时接受快照。' | Out-Null
    Add-Callout '设计取舍' '单帧工作缓冲和丢旧帧策略减少了 DSRAM 占用与排队延迟，适合 160×120 色块视觉；代价是高负载时有效帧率下降，因此报告将处理时间、视觉年龄和丢帧状态作为必须观测的调试量。'

    Add-Heading '3.4 模式状态机与菜单' 2 | Out-Null
    Add-Paragraph -Text '上电默认进入 MODE_STOP。主菜单依次提供 Action Task、Parking Task、Remote Control、Static Vision、跟随速度、返航速度和 Servo Center。Action Task 对应车跟人去程与随后返航；Parking Task 对应 10 s 语音停车任务；Remote Control 用于遥控和影子视觉诊断；Static Vision 用于静态阈值与框选检查。KEY1/KEY2 选择，KEY3 进入或确认，KEY4 退出/停车。' | Out-Null

    # ========================== 第四章 ==========================
    Add-Chapter '第四章  视觉目标检测与跟随控制'
    Add-Heading '4.1 图像采集与颜色空间转换' 2 | Out-Null
    Add-Paragraph -Text '相机输出 RGB565。由于 SCC8660 数据高字节先到而 TC387 以小端方式保存 uint16，程序首先执行字节交换，再展开为 8 bit R、G、B。随后使用整数运算将像素转换到 OpenCV 约定的 HSV 范围，其中 H 为 0～179，S、V 为 0～255。当前亮绿色色块阈值为 H=36～80、S=80～255、V=50～255。阈值集中定义在 color_vision.h，便于在阳光、阴影和逆光条件下统一标定。' | Out-Null
    Add-Equation 'M(x,y)=1,  当 H∈[36,80]、S∈[80,255]、V∈[50,255]' '(4-1)'
    Add-FigurePlaceholder '图4-1' 'SCC8660 原图、HSV 掩膜与目标框选示例' 6.0

    Add-Heading '4.2 形态学与连通域' 2 | Out-Null
    Add-Paragraph -Text '二值掩膜执行 3×3 开运算后再执行闭运算：开运算去除零散亮点，闭运算填补布料折痕和局部阴影产生的小孔。程序以 8 邻域洪泛搜索连通域，统计每个候选的面积、边界、像素和以及外接框宽高。面积小于 20 pixel 的候选被视为噪声。该实现全部使用 CPU2 的静态数组与预分配队列，不在运行时申请动态内存。' | Out-Null

    Add-Heading '4.3 目标连续匹配' 2 | Out-Null
    Add-Paragraph -Text '首次识别时选择面积最大的有效绿色连通域；建立目标后，不再逐帧只认全图最大色块，而是对每个候选计算中心 X、中心 Y 和高度相对于上一目标的平方差和。得分最小的候选优先，得分相同时取面积更大的候选。这样可以在赛道附近出现草坪、树木或小型绿色物体时，依靠空间和尺度连续性维持原目标。' | Out-Null
    Add-Equation 'J_i=(u_i-u_{k-1})²+(v_i-v_{k-1})²+(h_i-h_{k-1})²' '(4-2)'
    Add-Paragraph -Text '连续匹配不会替代色彩阈值本身。若候选均因光照或遮挡消失，跟随控制进入丢帧迟滞；重新出现后，当前 CPU2 仍会按上一轨迹状态选择最连续的候选。因此测试时应记录目标中心、框高、视觉年龄和目标有效位，确认不存在长期误锁。' | Out-Null

    Add-Heading '4.4 高度测距与车辆坐标转换' 2 | Out-Null
    Add-Paragraph -Text '色块采用横向 A4 规格，物理竖直尺寸为 210 mm。工程以相机光轴附近、距离 0.50 m 时框高 35 pixel 的实测值建立反比例模型，避免目标转身或布料角度变化对面积测距造成更大影响。' | Out-Null
    Add-Equation 'Z = Z_c h_c / h = 0.50×35/h' '(4-3)'
    Add-Paragraph -Text '横向位置由针孔模型估计。设图像中心为 c_x=79.5 pixel，水平等效焦距 f_x=40.91 pixel，则相机坐标系横向量为 Y_c=−(u−c_x)Z/f_x。程序再根据相机相对后轴中心的平移与偏航外参，将目标变换到车辆坐标系。当前相机前向外参 X_c=0.90 m，横向偏置与偏航角均为 0。外参必须在最终支架定型后复测。' | Out-Null
    Add-Equation 'Y_c=−(u−c_x)Z/f_x' '(4-4)'
    Add-Equation '[X  Y]ᵀ=[X_c  Y_c0]ᵀ+R(ψ_c)[Z  Y_c]ᵀ' '(4-5)'

    Add-Heading '4.5 距离—相对速度状态滤波与加速度环' 2 | Out-Null
    Add-Paragraph -Text '视觉测量只在新有效帧到达时更新，滤波时间步长取相邻视觉时间戳之差，而不是固定 10 ms。程序使用 α-β 预测—校正结构估计距离和距离变化率。若相邻测量间隔为 0 或超过 120 ms，状态重新初始化，防止使用失真的时间差放大相对速度。' | Out-Null
    Add-Equation 'Ẑ⁻_k=Ẑ_{k-1}+v̂_{r,k-1}Δt，r_k=Z_k−Ẑ⁻_k' '(4-6)'
    Add-Equation 'Ẑ_k=Ẑ⁻_k+0.35r_k，v̂_{r,k}=v̂_{r,k-1}+0.06r_k/Δt' '(4-7)'
    Add-Paragraph -Text '期望车距为 1.00 m，距离死区为 ±0.12 m。距离大于设定值时误差为正并加速，距离过近时误差为负并减速；相对速度项提供阻尼。加速度命令限幅为 ±2.00 m/s²，并在 100 Hz 控制中断中积分为目标速度，默认跟随上限为 2.50 m/s，菜单可在 0～5.0 m/s 范围内调整。' | Out-Null
    Add-Equation 'a_k=1.20(Ẑ_k−1.00)+1.50v̂_{r,k}' '(4-8)'
    Add-Equation 'v*_k=sat[v*_{k−1}+a_k/100，0，v_max]' '(4-9)'
    Add-Paragraph -Text '视觉模式下 encoder_motor_ctl.c 直接令底盘 target_ramp 等于视觉积分速度，不再调用通用 Speed_Ramp_Update()。因此整条链路只有一个加速度限制器，避免两级斜坡叠加造成目标速度长期滞后。' | Out-Null

    Add-Heading '4.6 动态前视 Pure Pursuit' 2 | Out-Null
    Add-Paragraph -Text '目标点由车辆后轴中心指向当前视觉目标。首先计算目标方位 α=atan2(Y,X)，再根据左右后轮实测平均速度计算前视距离。当前折中参数为基础 0.70 m、速度系数 0.40 s，并在 1.00～1.80 m 之间限幅。低速时保留小半径转弯能力，高速时增加前视以降低直道画龙。' | Out-Null
    Add-Equation 'L_d=clamp(0.70+0.40|v|，1.00，1.80)' '(4-10)'
    Add-Equation 'δ=atan2(2L sinα，L_d)，L=0.58 m' '(4-11)'
    Add-Paragraph -Text '为兼顾直线稳定和切弯响应，程序按原始横向像素误差连续调度中心滤波与转向增益：误差不大于 4 pixel 时，中心滤波系数为 0.25、转向增益为 0.50；误差不小于 18 pixel 时分别恢复为 0.50 和 1.00；中间区间线性插值。原始误差直接参与调度，使真实转弯能立即恢复响应，而小幅视觉抖动不会反复驱动车头。' | Out-Null

    Add-Heading '4.7 丢帧迟滞与命令有效性' 2 | Out-Null
    Add-Paragraph -Text '最新有效目标出现后的 100 ms 内保持已有加速度与转角，70 ms 后停止继续积分加速度；100～300 ms 区间保持最后转角并将速度按剩余时间线性衰减；达到 300 ms 后命令失效，控制中断清零转向与后轮输出并复位速度环。该策略避免单帧阴影、遮挡或色块边缘破碎造成瞬时急停，同时限制长时间盲走。' | Out-Null
    Add-FigurePlaceholder '图4-2' '目标有效、短时丢失、线性减速和失效停车时序图' 5.4

    # ========================== 第五章 ==========================
    Add-Chapter '第五章  转向与后轮运动控制'
    Add-Heading '5.1 前轮角度闭环' 2 | Out-Null
    Add-Paragraph -Text '转向执行器不是普通舵机，而是“绝对编码器＋有刷直流电机＋双 PWM H 桥”的闭环系统。绝对编码器中位为 1744，程序处理 0/4096 环绕后换算转向传感器角度，再乘 0.59 得到真实前轮角。视觉和返航控制器均输出真实轮角，进入内环前再除以 0.59 变回传感器目标角。' | Out-Null
    Add-Paragraph -Text '内环采用位置 PD。基础比例和微分系数分别为 200 和 100，并按当前传感器角绝对值乘以 1+0.067|θ_s|，补偿大角度机构负载。误差小于 1°时输出置零以减少回中抖动，PWM 最终由电机层限幅。真实前轮角统一限制为 ±32°。' | Out-Null
    Add-Equation 'u_s=[200(1+0.067|θ_s|)]e+[100(1+0.067|θ_s|)]Δe' '(5-1)'

    Add-Heading '5.2 后轮速度闭环' 2 | Out-Null
    Add-Paragraph -Text '后轮编码器每 10 ms 清零计数并计算速度，速度分辨率由 0.24π/2048 m 每脉冲确定。左右轮速度使用一阶滤波，当前 FILTER_ALPHA=0.6；未经滤波的左右轮平均速度用于平面坐标积分，以减少速度滤波造成的位移相位延迟。' | Out-Null
    Add-Paragraph -Text '左右后轮均使用增量式 PI，初始化参数 Kp=1000、Ki=200、Kd=0，输出范围为 ±9500。每拍输出增量限制为总量程的 20%，抑制单次误差突变。视觉模式直接使用视觉加速度积分得到的目标速度；遥控和返航模式仍使用通用速度斜坡和换向管理。' | Out-Null

    Add-Heading '5.3 几何差速' 2 | Out-Null
    Add-Paragraph -Text '当前轮真实角度超过 1°且目标速度非零时，程序根据轴距和轮距计算差速因子。设车辆中心目标速度为 v，前轮角为 δ，差速修正系数为 K_diff，则左右轮目标速度为：' | Out-Null
    Add-Equation 'q=K_diff·W tanδ/(2L)' '(5-2)'
    Add-Equation 'v_L=v(1−q)，v_R=v(1+q)' '(5-3)'
    Add-Paragraph -Text '该分配使内侧轮速度降低、外侧轮速度提高，减少转弯拖滑。K_diff 保留为现场可调参数，用于补偿真实轮胎侧偏、载荷与机构误差。' | Out-Null

    Add-Heading '5.4 模式切换与停车' 2 | Out-Null
    Add-Paragraph -Text 'MODE_STOP 在每个 10 ms 控制拍将前轮和后轮命令清零、复位双轮 PI，并清除速度斜坡。视觉命令失效时执行同样的停车处理。遥控模式保留 SBUS 超时检测；返航模式由 back_ins 的运行结果决定退出。控制逻辑通过统一模式枚举保证不同任务不会同时写执行器。' | Out-Null

    # ========================== 第六章 ==========================
    Add-Chapter '第六章  路径记录与二维惯导返航'
    Add-Heading '6.1 去程轨迹在线记录' 2 | Out-Null
    Add-Paragraph -Text 'Action Task 启动时复位路径记录器并记录初始点。车辆跟随引导员行驶期间，记录器根据后轮绝对里程每 0.20 m 保存一个点，内容包括平面 X/Y、航向角、速度、时间戳和里程参考；结束时强制写入最终参考点。记录首先进入 RAM 环形/线性缓冲，可选择写入 Flash 以支持重新上电后的返航。' | Out-Null
    Add-Paragraph -Text '平面坐标由左右轮未经滤波的平均速度与 IMU 航向角积分：' | Out-Null
    Add-Equation 'x_k=x_{k−1}+v_k cosψ_k·0.01' '(6-1)'
    Add-Equation 'y_k=y_{k−1}+v_k sinψ_k·0.01' '(6-2)'
    Add-Paragraph -Text 'IMU660RA 上电进行 2000 次静止零偏采样。运行时仅在 MODE_STOP 且左右轮速度均小于 0.01 m/s 时缓慢更新 Z 轴零偏，避免把真实转弯角速度吸收到零偏中。Yaw 由 Z 轴角速度积分，Roll/Pitch 使用 Mahony 四元数更新。' | Out-Null

    Add-Heading '6.2 坐标对齐与逆序轨迹' 2 | Out-Null
    Add-Paragraph -Text '返航可从 RAM 直接逆序读取，也可从 Flash 批量逆序读取。加载后将去程终点变换为返航起点，根据当前 IMU 位姿对整条轨迹作统一旋转和平移，使记录坐标与本次返航坐标系对齐。这样既允许去程后立即返航，也允许重新上电后从零坐标启动。' | Out-Null
    Add-FigurePlaceholder '图6-1' '去程记录点逆序、坐标对齐与返航轨迹示意' 5.8

    Add-Heading '6.3 二维 XY 投影与倒车 Pure Pursuit' 2 | Out-Null
    Add-Paragraph -Text '返航进度不是只按累计里程选择索引，而是将当前 XY 投影到上一索引附近的路径线段，得到连续且单调的路径进度。最近点搜索限定在后 4 点、前 40 点窗口内，降低交叉路径跳到错误分支的概率。目标进度为当前进度加动态前视距离，并通过相邻记录点线性插值得到目标 X/Y 和参考航向。' | Out-Null
    Add-Equation 'L_d^b=clamp(0.30+0.40|v|，0.25，1.60)' '(6-3)'
    Add-Paragraph -Text '倒车时车辆实际运动方向为车身 yaw+180°，因此目标方位误差 α 相对于倒车运动方向计算。前轮转向车辆倒车时曲率符号与前进相反，程序对标准 PP 转角取负：' | Out-Null
    Add-Equation 'δ_b=−atan2(2L sinα，d_t)' '(6-4)'
    Add-Paragraph -Text '若当前点到路径的最近距离超过 2.00 m，或编码器里程明显超过控制路径长度，返航判定丢失并停车。末端停车主要依据编码器剩余里程，避免末端 XY 漂移导致反复修正。' | Out-Null

    Add-Heading '6.4 曲率速度规划' 2 | Out-Null
    Add-Paragraph -Text '返航在路径加载阶段预计算局部曲率与速度上限。曲率估计使用 0.40 m 半窗口并在 0.20 m 邻域滤波；根据安全横向加速度 a_y,max=5.92 m/s² 计算弯道速度，同时设置最小路径速度 0.50 m/s、紧弯半径 1.30 m 和紧弯加速度 1.20 m/s²。速度表再执行前向加速传播和反向减速传播，使车辆在进入弯道前完成降速、出弯后平滑恢复。' | Out-Null
    Add-Equation 'v_κ≤√(a_y,max/|κ|)' '(6-5)'
    Add-Paragraph -Text '返航默认巡航上限为 3.0 m/s，菜单允许在 0～7.0 m/s 范围内设置。启动阶段采用 1.50 m/s² 软加速，目标减速度默认 2.50 m/s²；这些参数应结合地面附着和车辆载荷验证。' | Out-Null
    Add-FigurePlaceholder '图6-2' '返航路径曲率、速度上限和实际速度曲线' 5.6

    # ========================== 第七章 ==========================
    Add-Chapter '第七章  语音任务、灯板与人机交互'
    Add-Heading '7.1 语音任务流程' 2 | Out-Null
    Add-Paragraph -Text 'Parking Task 进入后先停车并初始化音频网络。服务器连接成功后，IPS200 显示 10 s 倒计时；时间到后请求结束录音并进入识别。识别结果映射为车辆动作、蜂鸣和灯光命令，执行完成后回到待监听状态。网络初始化、时间同步、鉴权、WebSocket、音频发送和结果超时均有独立错误码在屏幕显示。KEY4 可随时退出任务。' | Out-Null
    Add-FigurePlaceholder '图7-1' '语音任务 10 s 倒计时及命令执行界面' 5.2

    Add-Heading '7.2 TLD7002 点阵灯板' 2 | Out-Null
    Add-Paragraph -Text 'TLD7002 使用 UART1、2 Mbit/s，沿用 MCU TX=P33.13、RX=P20.9 与 GPIN0=P33.12。由于 SCC8660 上电配置阶段也使用 UART1，CPU3 必须等待四核启动屏障结束后再切换 UART1 到灯板引脚。CPU3 初始化点阵并设置亮度，随后以 1 ms 周期处理 CPU0 发布的异步灯光命令，点阵同步扫描中断负责刷新显示。' | Out-Null
    Add-FigurePlaceholder '图7-2' 'TLD7002 点阵灯板原理图、PCB 与实物照片' 6.0

    Add-Heading '7.3 比赛操作流程' 2 | Out-Null
    Add-GridTable @('阶段','操作','车辆状态与界面') @(
        ,@('准备','上电，等待 IMU 标定完成','MODE_STOP；主菜单显示'),
        ,@('去程','选择 Action Task，KEY3 启动','视觉跟随并同步记录轨迹'),
        ,@('去程结束','队友配合 KEY3','停止记录并强制写入终点参考'),
        ,@('返航','确认 RAM 返航','二维 XY 倒车 PP＋曲率速度规划'),
        ,@('返航结束','自动停车并回菜单','菜单箭头切换到 Parking Task'),
        ,@('语音停车','KEY3 进入','10 s 倒计时、识别、动作和灯板显示')
    ) '表7-1  推荐比赛操作流程'

    # ========================== 第八章 ==========================
    Add-Chapter '第八章  调试方法、参数整定与试验验证'
    Add-Heading '8.1 静态视觉标定' 2 | Out-Null
    Add-Paragraph -Text '首先在 Static Vision 页面检查字节顺序、RGB 通道和 HSV 阈值。分别在阳光、阴影、逆光和阴天条件下采集色块中心及边缘 HSV，阈值应覆盖色块而不过多覆盖草坪、树木和地面反光。随后将色块固定在实际穿戴高度和姿态，在 0.5、1.0、1.5、2.0 m 处记录框高，重新拟合 h·Z 常数；最后测量相机光心相对后轴的 X/Y 和偏航外参。' | Out-Null
    Add-GridTable @('测试项目','观测量','异常现象','对应调整') @(
        ,@('静止中心','中心误差、目标角、实测角','目标角非零或车头单侧偏','相机中心 c_x、机械中位 DIR_MID、安装偏航'),
        ,@('静止距离','框高、估计距离','距离随站位系统偏差','高度标定常数与支架俯仰'),
        ,@('左右移动','原始中心、滤波中心','滤波中心明显滞后','中心 α 调度区间'),
        ,@('短时遮挡','视觉年龄、目标有效位','单帧即停或盲走过久','丢帧保持/超时时间')
    ) '表8-1  静态视觉标定与故障对应'

    Add-Heading '8.2 无线示波器诊断' 2 | Out-Null
    Add-Paragraph -Text 'CPU1 在视觉模式以及遥控影子跟随模式下发送 7 通道：CH0 原始像素中心误差，CH1 车辆坐标横向目标 Y，CH2 目标真实前轮角，CH3 实测真实前轮角，CH4 实测平均车速，CH5 IMU Z 轴角速度，CH6 视觉年龄。非视觉模式下通道切换为返航 yaw、目标 yaw、PP 轮角、实测轮角、实测速度、速度斜坡和末端修正量。' | Out-Null
    Add-GridTable @('曲线关系','判断','优先排查') @(
        ,@('CH0 高频抖动，CH2 同步抖动，CH3 跟随','视觉/支架噪声进入转向','相机支架、曝光、中心滤波、小误差增益'),
        ,@('CH2 平滑，CH3 明显滞后或超调','转向执行器带宽不足','角度 PD、供电、机构间隙与摩擦'),
        ,@('CH0/CH2 平滑，CH5 出现周期横摆','底盘/轮胎自身振荡','差速系数、轮胎附着、质心与速度'),
        ,@('CH6 周期突增并伴随速度停顿','视觉处理掉帧或传输异常','CPU2 处理时间、FFC、DMA 与阈值破碎'),
        ,@('CH4 追不上目标且无大打角','后轮速度闭环或供电受限','PI、编码器、母线压降、温升')
    ) '表8-2  直道画龙与结构/算法问题的曲线判据'
    Add-FigurePlaceholder '图8-1' '无线示波器 7 通道典型曲线与标注' 6.0

    Add-Heading '8.3 分阶段提速与转弯测试' 2 | Out-Null
    Add-Paragraph -Text '测试按 1.0、1.5、2.0、2.5 m/s 逐级提高跟随上限，每级至少完成静止偏置、10 m 直线、小幅 S 弯、单锥桶圆弧和完整赛道五项。只有在目标不丢失、转向实测能跟随目标、车距无持续振荡、轮胎不连续侧滑、返航路径可重复的情况下才进入下一级。不要同时修改 HSV、前视距离、转向增益和速度环参数，每轮仅改变一组变量。' | Out-Null
    Add-Paragraph -Text '小误差区仍画龙时，先降低小误差转向增益或降低中心更新系数；进弯初期反应迟钝而误差大后突然打满时，应缩短 4～18 pixel 的过渡区或提高中段增益；低速即出现直角切弯时，应提高低速前视下限或降低中段增益；高速切弦时则需结合引导员跑法、目标车距和前视上限判断，不能只把轮角写死。' | Out-Null
    Add-FigurePlaceholder '图8-2' '不同速度上限下的跟随轨迹与最小转弯半径对比' 6.0

    Add-Heading '8.4 热态与重复性验证' 2 | Out-Null
    Add-Paragraph -Text '针对长时间暴晒、重复发车和烧录失败现象，应分别记录冷机、30 min、60 min、90 min 条件下的主板温度、IMU 零偏、48/36/24 V 母线电压、转向目标/实测角和 Flash 下载稳定性。若同一代码冷机正常而热机逐步恶化，需先排除供电降额、连接器接触、调试器 USB 过流、晶振/主控温度和 IMU 零偏漂移，再进行算法调参。' | Out-Null
    Add-FigurePlaceholder '图8-3' '冷机与热态重复试验数据、温度和供电曲线' 5.6

    # ========================== 第九章 ==========================
    Add-Chapter '第九章  总结与展望'
    Add-Paragraph -Text '本文完成了卡丁快跑（本科）组可载人智能车的视觉去程、在线路径记录、二维惯导返航、语音停车和灯板显示一体化设计。TC387 四核架构将底盘强实时控制、无线诊断、视觉处理和灯板服务分离；视觉链路采用 HSV 色块、形态学、连续目标匹配、高度测距、距离—相对速度滤波、加速度积分和动态前视 PP；底盘采用真实前轮角闭环与双后轮增量 PI 差速；返航采用逆序 XY 轨迹、倒车 PP 和曲率速度规划。' | Out-Null
    Add-Paragraph -Text '当前工程已具备完整比赛流程和现场调参接口。下一步工作的重点是：在最终相机支架上重新标定内外参和高度—距离曲线；补全功率板、主控板和接线图；通过无线示波器分离视觉噪声、支架振动和转向执行器迟滞；在真实赛道完成热态重复性、湿滑地面和不同光照下的分级提速验证。' | Out-Null

    # -------------------------- 致谢与参考文献 --------------------------
    Add-PageBreak
    Add-Paragraph -Text '致  谢' -Font '黑体' -Size 18 -Bold -1 -Align 1 -FirstIndent 0 -SpaceAfter 16 -LineSpacing 24 | Out-Null
    Add-Paragraph -Text '感谢山东大学指导教师在整车结构、电气安全和现场测试方面给予的指导，感谢隆山飞驰队全体成员在机械加工、硬件设计、嵌入式软件、视觉算法和赛道调试中的协同工作。感谢全国大学生智能汽车竞赛组委会提供交流与实践平台。' | Out-Null

    Add-PageBreak
    Add-Paragraph -Text '参考文献' -Font '黑体' -Size 18 -Bold -1 -Align 1 -FirstIndent 0 -SpaceAfter 14 -LineSpacing 24 | Out-Null
    $refs = @(
        '[1] Infineon Technologies AG. AURIX TC3xx User Manual.',
        '[2] 成都逐飞科技有限公司. TC387 开源库与外设驱动使用说明.',
        '[3] 成都逐飞科技有限公司. SCC8660 彩色摄像头使用说明.',
        '[4] 成都逐飞科技有限公司. IMU660RA 六轴惯性传感器使用说明.',
        '[5] R. C. Coulter. Implementation of the Pure Pursuit Path Tracking Algorithm. Carnegie Mellon University, 1992.',
        '[6] R. Mahony, T. Hamel, J.-M. Pflimlin. Nonlinear Complementary Filters on the Special Orthogonal Group. IEEE TAC, 2008.',
        '[7] K. J. Åström, R. M. Murray. Feedback Systems: An Introduction for Scientists and Engineers. Princeton University Press, 2008.',
        '[8] R. C. Gonzalez, R. E. Woods. Digital Image Processing. Pearson.',
        '[9] TLD7002-16ES LED Driver Data Sheet. Infineon Technologies AG.'
    )
    foreach ($ref in $refs) { Add-Paragraph -Text $ref -Size 10.5 -Align 0 -FirstIndent 0 -SpaceAfter 6 -LineSpacing 15 | Out-Null }

    # -------------------------- 附录：图纸与实物资料 --------------------------
    Add-PageBreak
    Add-Paragraph -Text '附  录  图纸与实物资料占位' -Font '黑体' -Size 18 -Bold -1 -Align 1 -FirstIndent 0 -SpaceAfter 14 -LineSpacing 24 | Out-Null
    Add-Paragraph -Text '本附录仅保留正式报告所需的整车、机械、电路、PCB 与测试资料图位。插图前请删除图位中的说明文字，并保持图号和图题。' -FirstIndent 0 | Out-Null
    Add-FigurePlaceholder '附图A-1' '整车正视照片' 7.0
    Add-FigurePlaceholder '附图A-2' '整车侧视照片' 7.0
    Add-FigurePlaceholder '附图A-3' '整车俯视照片' 7.0
    Add-FigurePlaceholder '附图A-4' '摄像头支架与色块穿戴效果' 6.0
    Add-FigurePlaceholder '附图A-5' '主控板原理图' 7.5
    Add-FigurePlaceholder '附图A-6' '主控板 PCB 正面与背面' 7.5
    Add-FigurePlaceholder '附图A-7' '48 V 分电与两路 LM5116 降压原理图' 7.5
    Add-FigurePlaceholder '附图A-8' '前轮 2ED2184 H 桥驱动板原理图与 PCB' 7.5
    Add-FigurePlaceholder '附图A-9' '后轮电调/H 桥原理图与 PCB' 7.5
    Add-FigurePlaceholder '附图A-10' 'SCC8660、IMU660RA、无线串口与 TLD7002 接线图' 7.5
    Add-FigurePlaceholder '附图A-11' '完整赛道测试照片与轨迹叠加图' 7.0

    # Paginate only after all content is present.
    try { $word.Options.Pagination = $true } catch {}
    $doc.Repaginate()
    $doc.SaveAs2($outputPath, 16)
    $doc.Close($true)
    $doc = $null
    $word.Quit()
    $word = $null
    Write-Output $outputPath
}
finally {
    if ($null -ne $doc) { try { $doc.Close($false) } catch {} }
    if ($null -ne $word) { try { $word.Quit() } catch {} }
    [GC]::Collect()
    [GC]::WaitForPendingFinalizers()
}
