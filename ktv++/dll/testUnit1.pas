unit testUnit1;

interface

uses
  Forms, Controls, Classes, StdCtrls;

type
  TForm1 = class(TForm)
    Button2: TButton;
    Label1: TLabel;
    GroupBox1: TGroupBox;
    RadioButton1: TRadioButton;
    RadioButton2: TRadioButton;
    RadioButton3: TRadioButton;
    Edit1: TEdit;
    Memo1: TMemo;
    procedure Button2Click(Sender: TObject);
    procedure RadioButton3Click(Sender: TObject);
  private
    { Private declarations }
  public
    { Public declarations }
  end;

var
  Form1: TForm1;

procedure SendMsg(MAC: PChar; buffer: PChar; Len: integer); stdcall; external 'dllapi.dll'
  
implementation

{$R *.dfm}
{$R WindowsXP.res}

procedure TForm1.Button2Click(Sender: TObject);
var
  S: string;
begin
  if RadioButton1.Checked then
    S:= 'lock'
  else if RadioButton2.Checked then
    S:= 'unlock'
  else
    S:= Memo1.Lines.Text;                           
  SendMsg(PChar(Edit1.Text), PChar(S), Length(S));
end;

procedure TForm1.RadioButton3Click(Sender: TObject);
begin
  Memo1.Enabled:= RadioButton3.Checked;  
end;

end.
