@ECHO OFF
del /s *.stat;*.~*;*.dcu;*.ddp;*.bak;*.*~;*.exe
del /s singerData.db; SongData.db;server.rcs
del / s .#*.*
ECHO Complete!

ECHO OK!
PAUSE
