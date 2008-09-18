unit avio;

interface

uses Windows, Classes, RTLConsts, SysUtils;

const
   LAMEDLL = 'avio.dll';

type
  TURLContext = packed record
    prot: Pointer;
    flags: integer;
    is_streamed: integer;
    max_packet_size: integer;
    priv_data: Pointer;
    filename: PChar;
  end;
  PURLContext = ^TURLContext;

  TUrlStream = class(TStream)
  private
    FURLContext: PURLContext;
    FUrl: string;
    function URLBlank(X: string): string;
  protected
    procedure SetSize(NewSize: Longint); override;
  public
    constructor Create(const UrlStr: string; Mode: Word);
    destructor Destroy; override;
    function Read(var Buffer; Count: Longint): Longint; override;
    function Seek(const Offset: Int64; Origin: TSeekOrigin): Int64; override;
    function Write(const Buffer; Count: Longint): Longint; override;
    function GetString: string;
    function GetStrings(List: TStrings): integer;
    property Url: string read FUrl;
  end;

function url_open   (var h: PURLContext; const filename: PChar; flags: integer): integer; cdecl; external LAMEDLL;
function url_read   (var h: TURLContext; var buf; size: integer): integer; cdecl; external LAMEDLL;
function url_write  (var h: TURLContext; const buf; size: integer): integer; cdecl; external LAMEDLL;
function url_seek   (var h: TURLContext; pos: int64; whence: integer): int64; cdecl; external LAMEDLL;
function url_close  (var h: TURLContext): integer; cdecl; external LAMEDLL;
function url_exist  (filename: PChar): integer; cdecl; external LAMEDLL;
function url_size   (var h: TURLContext): int64; cdecl; external LAMEDLL;
function url_readbuf(url: PChar; var size: int64): Pointer; cdecl; external LAMEDLL;
procedure av_register_all(); cdecl; external LAMEDLL;
function ReadUrl(Url: string): string;

implementation

{ TUrlStream }

function ReadUrl(Url: string): string;
var
  UrlStream: TUrlStream;
begin
  try
    Result:= '';
    UrlStream:= TUrlStream.Create(Url, 0);
    if UrlStream = nil then Exit;
    Result:= UrlStream.GetString;
    UrlStream.Free;
  except
  end;
end;

constructor TUrlStream.Create(const UrlStr: string; Mode: Word);
begin
  inherited Create;
  FUrl := URLBlank(UrlStr);
  FURLContext:= nil;
  av_register_all;

	url_open(FURLContext, PChar(FUrl), Mode);
  if FURLContext = nil then
    raise EFCreateError.CreateResFmt(@SFCreateErrorEx, [Url, SysErrorMessage(GetLastError)]);
end;

destructor TUrlStream.Destroy;
begin
  if FURLContext <> nil then
    url_close(FURLContext^);
  inherited;
end;

function TUrlStream.GetString: string;
var
  Len, L: integer;
  Buf: array[0..1024] of byte;
begin
  Result:= '';
  while true do
  begin
    Len:= url_read(FURLContext^, Buf, 1024);
    if Len <= 0 then break;
    L:= Length(Result);
    SetLength(Result, L + Len);
    Move(Buf, PChar(@Result[L + 1])^, Len);
  end;
end;

function TUrlStream.GetStrings(List: TStrings): integer;
var
  Len: integer;
  Buf: array[0..1024] of byte;
  Str: string;
begin
  List.Text:= '';
  while true do
  begin
    Len:= url_read(FURLContext^, Buf, 1024);
    if Len <= 0 then break;
    SetLength(Str, Len);
    Move(Buf, Str[1], Len);
    List.Text:= List.Text + Str;
  end;
  Result:= List.Count;
end;

function TUrlStream.Read(var Buffer; Count: Integer): Longint;
var
  P: Pointer;
  Len: integer;
begin
  P:= @Buffer;
  Result:= 0;
  while Result < Count do
  begin
    Len:= url_read(FURLContext^, Pointer(Longint(P) + Result)^, Count - Result);
    if Len = 0 then break;
    Result := Result + Len;
  end;
  if Result <= -1 then Result := 0;
end;

function TUrlStream.Seek(const Offset: Int64; Origin: TSeekOrigin): Int64;
begin
  Result:= url_seek(FURLContext^, Offset, Ord(Origin));
end;

procedure TUrlStream.SetSize(NewSize: Integer);
begin
  Seek(NewSize, soBeginning);
end;

function TUrlStream.URLBlank(X: string): string;
var
  I: integer;
begin
  Result:= '';
  for I:= 1 to Length(X) do
    if X[i]= ' ' then
      Result:= Result + '%20'
    else
      Result:= Result + X[I];
end;

function TUrlStream.Write(const Buffer; Count: Integer): Longint;
begin
  Result := url_write(FURLContext^, Buffer, Count);
  if Result = -1 then Result := 0;
end;

end.
