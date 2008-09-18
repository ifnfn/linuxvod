object Form1: TForm1
  Left = 307
  Top = 201
  BorderStyle = bsDialog
  Caption = #21253#25151#25511#21046#27979#35797
  ClientHeight = 208
  ClientWidth = 354
  Color = clBtnFace
  Font.Charset = GB2312_CHARSET
  Font.Color = clWindowText
  Font.Height = -12
  Font.Name = #23435#20307
  Font.Style = []
  OldCreateOrder = False
  PixelsPerInch = 96
  TextHeight = 12
  object Label1: TLabel
    Left = 8
    Top = 12
    Width = 126
    Height = 12
    Caption = #35831#36755#20837#24037#20316#31449#30340#32593#21345#21495':'
  end
  object Button2: TButton
    Left = 272
    Top = 176
    Width = 75
    Height = 25
    Caption = #21457#36865#21629#20196
    TabOrder = 0
    OnClick = Button2Click
  end
  object GroupBox1: TGroupBox
    Left = 8
    Top = 32
    Width = 337
    Height = 137
    Caption = #25805#20316':'
    TabOrder = 1
    object RadioButton1: TRadioButton
      Left = 16
      Top = 24
      Width = 73
      Height = 17
      Caption = #20851#38381#21253#25151
      Checked = True
      TabOrder = 0
      TabStop = True
      OnClick = RadioButton3Click
    end
    object RadioButton2: TRadioButton
      Left = 96
      Top = 24
      Width = 73
      Height = 17
      Caption = #24320#21551#21253#25151
      TabOrder = 1
      OnClick = RadioButton3Click
    end
    object RadioButton3: TRadioButton
      Left = 192
      Top = 24
      Width = 73
      Height = 17
      Caption = #21457#36865#20449#24687
      TabOrder = 2
      OnClick = RadioButton3Click
    end
    object Memo1: TMemo
      Left = 16
      Top = 48
      Width = 305
      Height = 73
      Enabled = False
      Lines.Strings = (
        #35831#36755#20837#35201#21457#36865#30340#20449#24687)
      TabOrder = 3
    end
  end
  object Edit1: TEdit
    Left = 144
    Top = 8
    Width = 201
    Height = 20
    TabOrder = 2
    Text = '00:0f:ea:22:59:96'
  end
end
