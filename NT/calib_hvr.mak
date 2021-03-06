# Microsoft Developer Studio Generated NMAKE File, Based on calib_hvr.dsp
!IF "$(CFG)" == ""
CFG=calib_hvr - Win32 Debug
!MESSAGE No configuration specified. Defaulting to calib_hvr - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "calib_hvr - Win32 Release" && "$(CFG)" != "calib_hvr - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "calib_hvr.mak" CFG="calib_hvr - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "calib_hvr - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "calib_hvr - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE 
!ERROR An invalid configuration is specified.
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 

!IF  "$(CFG)" == "calib_hvr - Win32 Release"

OUTDIR=.\Release
INTDIR=.\Release

ALL : ".\bin\calib_hvr.exe"


CLEAN :
	-@erase "$(INTDIR)\calib_hvr.obj"
	-@erase "$(INTDIR)\mscb.obj"
	-@erase "$(INTDIR)\mscbrpc.obj"
	-@erase "$(INTDIR)\strlcpy.obj"
	-@erase "$(INTDIR)\musbstd.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase ".\bin\calib_hvr.exe"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /W3 /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /D "_CRT_SECURE_NO_DEPRECATE" /D "_CRT_NONSTDC_NO_DEPRECATE" /D "HAVE_LIBUSB" /Fp"$(INTDIR)\calib_hvr.pch" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c /I "\mxml" /I "\midas\include" /I "\midas\mscb\drivers\windows\libusb\include"

.c{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.c{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

RSC=rc.exe
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\calib_hvr.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=wsock32.lib /nologo /subsystem:console /incremental:no /pdb:"$(OUTDIR)\calib_hvr.pdb" /machine:I386 /out:"c:\midas\nt\bin\calib_hvr.exe" 
LINK32_OBJS= \
	"$(INTDIR)\calib_hvr.obj" \
	"$(INTDIR)\mscb.obj" \
	"$(INTDIR)\strlcpy.obj" \
	"$(INTDIR)\musbstd.obj" \
	"$(INTDIR)\mscbrpc.obj" \
	"\midas\mscb\drivers\windows\libusb\lib\libusb.lib"       


".\bin\calib_hvr.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "calib_hvr - Win32 Debug"

OUTDIR=.\Debug
INTDIR=.\Debug

ALL : ".\bin\calib_hvr.exe"


CLEAN :
	-@erase "$(INTDIR)\calib_hvr.obj"
	-@erase "$(INTDIR)\mscb.obj"
	-@erase "$(INTDIR)\mscbrpc.obj"
	-@erase "$(INTDIR)\strlcpy.obj"
	-@erase "$(INTDIR)\musbstd.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\vc60.pdb"
	-@erase "$(OUTDIR)\calib_hvr.pdb"
	-@erase ".\bin\calib_hvr.exe"
	-@erase ".\bin\calib_hvr.ilk"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /MLd /W3 /Gm /ZI /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /D "_CRT_SECURE_NO_DEPRECATE" /D "_CRT_NONSTDC_NO_DEPRECATE" /D "HAVE_LIBUSB" /Fp"$(INTDIR)\calib_hvr.pch"  /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c /I "\mxml" /I "\midas\include" /I "\midas\mscb\drivers\windows\libusb\include"

.c{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.c{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

RSC=rc.exe
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\calib_hvr.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=wsock32.lib /nologo /subsystem:console /incremental:yes /pdb:"$(OUTDIR)\calib_hvr.pdb" /debug /machine:I386 /out:"c:\midas\nt\bin\calib_hvr.exe" /pdbtype:sept 
LINK32_OBJS= \
	"$(INTDIR)\calib_hvr.obj" \
	"$(INTDIR)\mscb.obj" \
	"$(INTDIR)\strlcpy.obj" \
	"$(INTDIR)\musbstd.obj" \
	"$(INTDIR)\mscbrpc.obj" \
	"\midas\mscb\drivers\windows\libusb\lib\libusb.lib"       


".\bin\calib_hvr.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ENDIF 


!IF "$(CFG)" == "calib_hvr - Win32 Release" || "$(CFG)" == "calib_hvr - Win32 Debug"
SOURCE=..\mscb\calib_hvr.c

"$(INTDIR)\calib_hvr.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\mscb\mscb.c

"$(INTDIR)\mscb.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\mscb\mscbrpc.c

"$(INTDIR)\mscbrpc.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)

SOURCE=\mxml\strlcpy.c

"$(INTDIR)\strlcpy.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=\midas\drivers\usb\musbstd.c

"$(INTDIR)\musbstd.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

