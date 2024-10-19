pushd %~dp0

protoc.exe -I=./ --cpp_out=./ ./Enum.proto
protoc.exe -I=./ --cpp_out=./ ./Struct.proto
protoc.exe -I=./ --cpp_out=./ ./Protocol.proto

GenPackets.exe --path=./Protocol.proto --output=ClientPacketHandler --recv=S_ --send=C_
GenPackets.exe --path=./Protocol.proto --output=ServerPacketHandler --recv=C_ --send=S_

IF ERRORLEVEL 1 PAUSE

XCOPY /Y Enum.pb.h "../../../Server"
XCOPY /Y Enum.pb.cc "../../../Server"
XCOPY /Y Struct.pb.h "../../../Server"
XCOPY /Y Struct.pb.cc "../../../Server"
XCOPY /Y Protocol.pb.h "../../../Server"
XCOPY /Y Protocol.pb.cc "../../../Server"
XCOPY /Y ServerPacketHandler.h "../../../Server"

XCOPY /Y Enum.pb.h "../../../../P1_Client/Source/P1_Client/Network"
XCOPY /Y Enum.pb.cc "../../../../P1_Client/Source/P1_Client/Network"
XCOPY /Y Struct.pb.h "../../../../P1_Client/Source/P1_Client/Network"
XCOPY /Y Struct.pb.cc "../../../../P1_Client/Source/P1_Client/Network"
XCOPY /Y Protocol.pb.h "../../../../P1_Client/Source/P1_Client/Network"
XCOPY /Y Protocol.pb.cc "../../../../P1_Client/Source/P1_Client/Network"
XCOPY /Y ClientPacketHandler.h "../../../../P1_Client/Source/P1_Client"

DEL /Q /F *.pb.h
DEL /Q /F *.pb.cc
DEL /Q /F *.h

PAUSE