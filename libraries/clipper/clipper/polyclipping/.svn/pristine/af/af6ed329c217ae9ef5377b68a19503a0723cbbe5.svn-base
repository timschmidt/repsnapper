object MainForm: TMainForm
  Left = 235
  Top = 110
  Width = 731
  Height = 547
  Caption = 'Clipper Delphi Demo'
  Color = clBtnFace
  Font.Charset = ARABIC_CHARSET
  Font.Color = clWindowText
  Font.Height = -12
  Font.Name = 'Arial'
  Font.Style = []
  KeyPreview = True
  OldCreateOrder = False
  Position = poDesktopCenter
  OnCreate = FormCreate
  OnKeyPress = FormKeyPress
  OnMouseWheel = FormMouseWheel
  OnResize = FormResize
  PixelsPerInch = 96
  TextHeight = 15
  object Panel1: TPanel
    Left = 0
    Top = 0
    Width = 183
    Height = 501
    Align = alLeft
    TabOrder = 0
    object lblClipOpacity: TLabel
      Left = 17
      Top = 408
      Width = 100
      Height = 15
      Caption = 'Clip Opacity (255):'
      FocusControl = tbClipOpacity
    end
    object lblSubjOpacity: TLabel
      Left = 17
      Top = 365
      Width = 103
      Height = 15
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
      Left = 109
      Top = 453
      Width = 52
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
      Height = 169
      TabOrder = 4
      object lblSubjCount: TLabel
        Left = 4
        Top = 40
        Width = 129
        Height = 15
        Caption = 'No. Subject edges: (20)'
        Enabled = False
        FocusControl = tbSubj
      end
      object lblClipCount: TLabel
        Left = 4
        Top = 87
        Width = 110
        Height = 15
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
        Caption = '&New Polygons'
        TabOrder = 4
        OnClick = bNextClick
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
      Top = 425
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
      Top = 382
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
    object bSaveSvg: TButton
      Left = 19
      Top = 453
      Width = 82
      Height = 25
      Cancel = True
      Caption = 'Save S&VG ...'
      TabOrder = 8
      OnClick = bSaveSvgClick
    end
  end
  object StatusBar1: TStatusBar
    Left = 0
    Top = 501
    Width = 723
    Height = 19
    Panels = <>
    SimplePanel = True
  end
  object ImgView321: TImgView32
    Left = 183
    Top = 0
    Width = 540
    Height = 501
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
  object SaveDialog1: TSaveDialog
    DefaultExt = 'svg'
    Filter = 'SVG Files (*.svg)|*.svg'
    Left = 239
    Top = 32
  end
end
