object MainForm: TMainForm
  Left = 148
  Top = 122
  Width = 859
  Height = 604
  Caption = 'Clipper Demo'
  Color = clBtnFace
  Font.Charset = ARABIC_CHARSET
  Font.Color = clWindowText
  Font.Height = -13
  Font.Name = 'Arial'
  Font.Style = []
  KeyPreview = True
  OldCreateOrder = False
  Position = poDesktopCenter
  OnCreate = FormCreate
  OnDestroy = FormDestroy
  OnKeyPress = FormKeyPress
  OnMouseWheel = FormMouseWheel
  OnResize = FormResize
  PixelsPerInch = 96
  TextHeight = 16
  object Panel1: TPanel
    Left = 0
    Top = 0
    Width = 183
    Height = 558
    Align = alLeft
    TabOrder = 0
    object lblClipOpacity: TLabel
      Left = 17
      Top = 468
      Width = 108
      Height = 16
      Caption = 'Clip Opacity (255):'
      FocusControl = tbClipOpacity
    end
    object lblSubjOpacity: TLabel
      Left = 17
      Top = 425
      Width = 112
      Height = 16
      Caption = 'Subj &Opacity (255):'
      FocusControl = tbSubjOpacity
    end
    object GroupBox1: TGroupBox
      Left = 13
      Top = 8
      Width = 159
      Height = 115
      Caption = 'Clipping  Oper&ation'
      TabOrder = 0
      object rbIntersection: TRadioButton
        Left = 14
        Top = 38
        Width = 113
        Height = 17
        Caption = 'Intersection'
        Checked = True
        TabOrder = 1
        TabStop = True
        OnClick = rbIntersectionClick
      end
      object rbUnion: TRadioButton
        Left = 14
        Top = 56
        Width = 113
        Height = 17
        Caption = 'Union'
        TabOrder = 2
        OnClick = rbIntersectionClick
      end
      object rbDifference: TRadioButton
        Left = 14
        Top = 74
        Width = 113
        Height = 17
        Caption = 'Difference'
        TabOrder = 3
        OnClick = rbIntersectionClick
      end
      object rbXOR: TRadioButton
        Left = 14
        Top = 92
        Width = 113
        Height = 17
        Caption = 'XOR'
        TabOrder = 4
        OnClick = rbIntersectionClick
      end
      object rbNone: TRadioButton
        Left = 14
        Top = 20
        Width = 113
        Height = 17
        Caption = 'None'
        TabOrder = 0
        OnClick = rbIntersectionClick
      end
    end
    object rbStatic: TRadioButton
      Left = 16
      Top = 129
      Width = 115
      Height = 17
      Caption = '&Static Polygons'
      Checked = True
      TabOrder = 1
      TabStop = True
      OnClick = rbStaticClick
    end
    object bExit: TButton
      Left = 21
      Top = 513
      Width = 136
      Height = 25
      Cancel = True
      Caption = 'E&xit'
      TabOrder = 7
      OnClick = bExitClick
    end
    object gbRandom: TGroupBox
      Left = 11
      Top = 184
      Width = 159
      Height = 233
      TabOrder = 4
      object lblSubjCount: TLabel
        Left = 4
        Top = 40
        Width = 137
        Height = 16
        Caption = 'No. Subject edges: (20)'
        Enabled = False
        FocusControl = tbSubj
      end
      object lblClipCount: TLabel
        Left = 4
        Top = 87
        Width = 115
        Height = 16
        Caption = 'No. Clip edges (20):'
        Enabled = False
        FocusControl = tbClip
      end
      object tbSubj: TTrackBar
        Left = 5
        Top = 58
        Width = 145
        Height = 28
        Enabled = False
        Max = 100
        Min = 3
        Position = 20
        TabOrder = 2
        ThumbLength = 16
        TickStyle = tsNone
        OnChange = tbSubjChange
      end
      object tbClip: TTrackBar
        Left = 5
        Top = 106
        Width = 145
        Height = 28
        Enabled = False
        Max = 100
        Min = 3
        Position = 20
        TabOrder = 3
        ThumbLength = 16
        TickStyle = tsNone
        OnChange = tbSubjChange
      end
      object bNext: TButton
        Left = 10
        Top = 132
        Width = 134
        Height = 25
        Caption = '&Next Random'
        Enabled = False
        TabOrder = 4
        OnClick = bNextClick
      end
      object bStart: TButton
        Left = 10
        Top = 162
        Width = 134
        Height = 25
        Caption = 'S&tart Loop'
        Enabled = False
        TabOrder = 5
        OnClick = bStartClick
      end
      object bStop: TButton
        Left = 10
        Top = 195
        Width = 134
        Height = 25
        Cancel = True
        Caption = 'Stop Loop'
        Enabled = False
        TabOrder = 6
        OnClick = bStopClick
      end
      object rbEvenOdd: TRadioButton
        Left = 5
        Top = 14
        Width = 73
        Height = 17
        Caption = 'E&venOdd'
        Checked = True
        Enabled = False
        TabOrder = 0
        TabStop = True
        OnClick = rbEvenOddClick
      end
      object rbNonZero: TRadioButton
        Left = 82
        Top = 14
        Width = 69
        Height = 17
        Caption = 'Non&Zero'
        Enabled = False
        TabOrder = 1
        OnClick = rbEvenOddClick
      end
    end
    object rbRandom1: TRadioButton
      Left = 16
      Top = 146
      Width = 146
      Height = 17
      Caption = 'Random Polygons &1'
      TabOrder = 2
      OnClick = rbStaticClick
    end
    object tbClipOpacity: TTrackBar
      Left = 12
      Top = 485
      Width = 158
      Height = 28
      Max = 255
      Position = 255
      TabOrder = 6
      ThumbLength = 16
      TickStyle = tsNone
      OnChange = tbClipOpacityChange
    end
    object tbSubjOpacity: TTrackBar
      Left = 12
      Top = 442
      Width = 158
      Height = 28
      Max = 255
      Position = 255
      TabOrder = 5
      ThumbLength = 16
      TickStyle = tsNone
      OnChange = tbSubjOpacityChange
    end
    object rbRandom2: TRadioButton
      Left = 16
      Top = 164
      Width = 146
      Height = 17
      Caption = 'Random Polygons &2'
      TabOrder = 3
      OnClick = rbStaticClick
    end
  end
  object StatusBar1: TStatusBar
    Left = 0
    Top = 558
    Width = 851
    Height = 19
    Panels = <>
    SimplePanel = True
  end
  object ImgView321: TImgView32
    Left = 183
    Top = 0
    Width = 668
    Height = 558
    Align = alClient
    Bitmap.ResamplerClassName = 'TNearestResampler'
    BitmapAlign = baCustom
    Scale = 1.000000000000000000
    ScaleMode = smScale
    ScrollBars.ShowHandleGrip = True
    ScrollBars.Style = rbsDefault
    OverSize = 0
    TabOrder = 2
    OnDblClick = bNextClick
    OnResize = ImgView321Resize
  end
  object Timer1: TTimer
    Enabled = False
    OnTimer = Timer1Timer
    Left = 209
    Top = 32
  end
end
