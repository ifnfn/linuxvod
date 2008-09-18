unit dll;

interface

uses SysUtils, WinSock;

var
  Count: integer;
  MACList: array of string;
  IPList: array of string;

  FSocket: TSocket;
  FUdpRemoteAddr: TSockAddrIn;

procedure SendMsg(MAC: PChar; buffer: PChar; Len: integer); stdcall;
function GetIP(MAC: string): string;

implementation

procedure InitUDP;
var
  N: integer;
begin
  Count:= 0;
  Setlength(MACList, 0);
  Setlength(IPList, 0);
  FSocket:= socket(AF_INET, SOCK_DGRAM, 0);
  if FSocket <> INVALID_SOCKET then;
  begin
    N := 1;
    setsockopt(FSocket, SOL_SOCKET, SO_BROADCAST, PChar(@n), sizeof(n));
  end;
end;

function SendBuf(Host: string; Port: integer; var ABuffer; const AByteCount: integer): boolean;
begin
  FillChar(FUdpRemoteAddr, Sizeof(TSockAddrIn), 0);
  FUdpRemoteAddr.sin_family:= AF_INET;
  FUdpRemoteAddr.sin_addr.S_addr:= inet_addr(PChar(Host));
  FUdpRemoteAddr.sin_port:=htons(Port);
  Result:= sendto(FSocket, ABuffer, AByteCount, 0, FUdpRemoteAddr, Sizeof(FUdpRemoteAddr)) < 0;
end;

function ReceiveString(Timeout: integer): string;
var
  I: integer;
  Len: integer;
begin
  setsockopt(FSocket, SOL_SOCKET, SO_RCVTIMEO, @TimeOut, TimeOut);
  SetLength(Result, 1024);
  Len:= sizeof(FUdpRemoteAddr);
  I:= Winsock.recvfrom(FSocket, Result[1], 1024, 0, FUdpRemoteAddr, len);
  SetLength(Result, i);
end;

function GetIP(MAC: string): string;
var
  I: integer;
  S: string;
begin
  Result:= '';
  for i:= 0 to Count - 1 do
  begin
    if SameText(MAC, MACList[I]) then
    begin
      Result:= IPList[I];
      break;
    end;
  end;
  if Result = '' then
  begin
    S:= 'MAC='+MAC;
    SendBuf('255.255.255.255', 6789, S[1], Length(S));
    Result:= ReceiveString(1000);
    if Result <> '' then
    begin
      Count:= Count + 1;
      Setlength(MACList, Count);
      Setlength(IPList, Count);
      MACList[Count-1]:= MAC;
      IPList[Count-1]:= Result;
    end;
  end;
end;

procedure SendMsg(MAC: PChar; buffer: PChar; Len: integer);
var
  IP: string;
begin
  IP := GetIP(MAC);
  if IP <> '' then
    SendBuf(IP, 6789, buffer[0], len);
end;

var
  WSAData: TWSAData;

initialization
  WSAStartup($0101, WSAData);
  InitUDP;

finalization
  closesocket(FSocket);
  WSACleanup;

end.
