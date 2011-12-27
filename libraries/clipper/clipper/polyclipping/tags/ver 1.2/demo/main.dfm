object MainForm: TMainForm
  Left = 151
  Top = 140
  Width = 859
  Height = 604
  Caption = 'Clipper Demo'
  Color = clBtnFace
  Font.Charset = ARABIC_CHARSET
  Font.Color = clWindowText
  Font.Height = -13
  Font.Name = 'Arial'
  Font.Style = []
  OldCreateOrder = False
  Position = poDesktopCenter
  OnCreate = FormCreate
  OnDestroy = FormDestroy
  OnResize = FormResize
  PixelsPerInch = 96
  TextHeight = 16
  object Panel1: TPanel
    Left = 0
    Top = 0
    Width = 185
    Height = 547
    Align = alLeft
    TabOrder = 0
    object lblClipOpacity: TLabel
      Left = 21
      Top = 459
      Width = 108
      Height = 16
      Caption = 'Clip Opacity (255):'
      FocusControl = tbClipOpacity
    end
    object lblSubjOpacity: TLabel
      Left = 21
      Top = 416
      Width = 112
      Height = 16
      Caption = 'Subj &Opacity (255):'
      FocusControl = tbSubjOpacity
    end
    object GroupBox1: TGroupBox
      Left = 13
      Top = 8
      Width = 156
      Height = 122
      Caption = 'Clipping  Oper&ation'
      TabOrder = 0
      object rbIntersection: TRadioButton
        Left = 14
        Top = 42
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
        Top = 60
        Width = 113
        Height = 17
        Caption = 'Union'
        TabOrder = 2
        OnClick = rbIntersectionClick
      end
      object rbDifference: TRadioButton
        Left = 14
        Top = 78
        Width = 113
        Height = 17
        Caption = 'Difference'
        TabOrder = 3
        OnClick = rbIntersectionClick
      end
      object rbXOR: TRadioButton
        Left = 14
        Top = 96
        Width = 113
        Height = 17
        Caption = 'XOR'
        TabOrder = 4
        OnClick = rbIntersectionClick
      end
      object rbNone: TRadioButton
        Left = 14
        Top = 24
        Width = 113
        Height = 17
        Caption = 'None'
        TabOrder = 0
        OnClick = rbIntersectionClick
      end
    end
    object rbStatic: TRadioButton
      Left = 16
      Top = 139
      Width = 113
      Height = 17
      Caption = '&Static Polygons'
      Checked = True
      TabOrder = 1
      TabStop = True
      OnClick = rbStaticClick
    end
    object bExit: TButton
      Left = 21
      Top = 509
      Width = 135
      Height = 25
      Caption = 'E&xit'
      TabOrder = 6
      OnClick = bExitClick
    end
    object GroupBox2: TGroupBox
      Left = 11
      Top = 183
      Width = 158
      Height = 219
      TabOrder = 3
      object lblSubjCount: TLabel
        Left = 4
        Top = 15
        Width = 146
        Height = 16
        Caption = 'Random S&ubj Count (10):'
        FocusControl = tbSubj
      end
      object lblClipCount: TLabel
        Left = 4
        Top = 62
        Width = 142
        Height = 16
        Caption = 'Random Cl&ip Count (10):'
        FocusControl = tbClip
      end
      object tbSubj: TTrackBar
        Left = 5
        Top = 33
        Width = 145
        Height = 28
        Max = 60
        Min = 3
        Position = 10
        TabOrder = 0
        ThumbLength = 16
        TickStyle = tsNone
        OnChange = tbSubjChange
      end
      object tbClip: TTrackBar
        Left = 5
        Top = 81
        Width = 145
        Height = 28
        Max = 60
        Min = 3
        Position = 10
        TabOrder = 1
        ThumbLength = 16
        TickStyle = tsNone
        OnChange = tbSubjChange
      end
      object bNext: TButton
        Left = 10
        Top = 148
        Width = 135
        Height = 25
        Caption = '&Next Random'
        Enabled = False
        TabOrder = 3
        OnClick = bNextClick
      end
      object bStart: TButton
        Left = 10
        Top = 118
        Width = 135
        Height = 25
        Caption = 'S&tart Loop'
        Enabled = False
        TabOrder = 2
        OnClick = bStartClick
      end
      object bStop: TButton
        Left = 10
        Top = 181
        Width = 135
        Height = 25
        Cancel = True
        Caption = 'Stop Loop'
        Enabled = False
        TabOrder = 4
        OnClick = bStopClick
      end
    end
    object rbRandom: TRadioButton
      Left = 16
      Top = 159
      Width = 131
      Height = 17
      Caption = '&Random Polygons'
      TabOrder = 2
      OnClick = rbStaticClick
    end
    object tbClipOpacity: TTrackBar
      Left = 16
      Top = 476
      Width = 145
      Height = 28
      Max = 255
      Position = 255
      TabOrder = 5
      ThumbLength = 16
      TickStyle = tsNone
      OnChange = tbClipOpacityChange
    end
    object tbSubjOpacity: TTrackBar
      Left = 16
      Top = 433
      Width = 145
      Height = 28
      Max = 255
      Position = 255
      TabOrder = 4
      ThumbLength = 16
      TickStyle = tsNone
      OnChange = tbSubjOpacityChange
    end
  end
  object StatusBar1: TStatusBar
    Left = 0
    Top = 547
    Width = 843
    Height = 19
    Panels = <>
    SimplePanel = True
  end
  object ImgView321: TImgView32
    Left = 185
    Top = 0
    Width = 658
    Height = 547
    Align = alClient
    Bitmap.ResamplerClassName = 'TNearestResampler'
    BitmapAlign = baCustom
    Scale = 1.000000000000000000
    ScaleMode = smScale
    ScrollBars.ShowHandleGrip = True
    ScrollBars.Style = rbsDefault
    OverSize = 0
    TabOrder = 2
    OnResize = ImgView321Resize
  end
  object Timer1: TTimer
    Enabled = False
    OnTimer = Timer1Timer
    Left = 209
    Top = 32
  end
end
