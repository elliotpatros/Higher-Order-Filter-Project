current:
	echo make pd_linux, pd_nt, pd_irix5, or pd_irix6

clean: ; rm -f *.pd_linux *.o

# ----------------------- Windows-----------------------
# note; you will certainly have to edit the definition of VC to agree with
# whatever you've got installed on your machine:

VC="C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC"

pd_nt: allpass~.dll bandpass~.dll fir~.dll highpass~.dll highshelf~.dll lowpass~.dll lowshelf~.dll notch~.dll peak~.dll

.SUFFIXES: .obj .dll

PDNTCFLAGS = /W3 /DNT /DPD /nologo

PDNTINCLUDE = /I. /I\tcl\include /I..\..\src /I$(VC)\include

PDNTLDIR = "C:\Program Files (x86)\Windows Kits\8.1\Lib\winv6.3\um\x86"
PDNTLIB = $(VC)\lib\msvcrt.lib \
	$(VC)\lib\oldnames.lib \
	$(PDNTLDIR)\kernel32.lib \
	..\..\bin\pd.lib 

.c.dll:
	cl $(PDNTCFLAGS) $(PDNTINCLUDE) /c $*.c
	link /dll /export:$*_setup $*.obj $(PDNTLIB)

# override explicitly for tilde objects like this:
allpass~.dll: allpass~.c 
	cl $(PDNTCFLAGS) $(PDNTINCLUDE) /c $*.c
	link /dll /export:allpass_tilde_setup $*.obj $(PDNTLIB)
	
bandpass~.dll: bandpass~.c 
	cl $(PDNTCFLAGS) $(PDNTINCLUDE) /c $*.c
	link /dll /export:bandpass_tilde_setup $*.obj $(PDNTLIB)
	
fir~.dll: fir~.c 
	cl $(PDNTCFLAGS) $(PDNTINCLUDE) /c $*.c
	link /dll /export:fir_tilde_setup $*.obj $(PDNTLIB)
	
highpass~.dll: highpass~.c 
	cl $(PDNTCFLAGS) $(PDNTINCLUDE) /c $*.c
	link /dll /export:highpass_tilde_setup $*.obj $(PDNTLIB)
	
highshelf~.dll: highshelf~.c 
	cl $(PDNTCFLAGS) $(PDNTINCLUDE) /c $*.c
	link /dll /export:highshelf_tilde_setup $*.obj $(PDNTLIB)
	
lowpass~.dll: lowpass~.c 
	cl $(PDNTCFLAGS) $(PDNTINCLUDE) /c $*.c
	link /dll /export:lowpass_tilde_setup $*.obj $(PDNTLIB)

lowshelf~.dll: lowshelf~.c 
	cl $(PDNTCFLAGS) $(PDNTINCLUDE) /c $*.c
	link /dll /export:lowshelf_tilde_setup $*.obj $(PDNTLIB)
	
notch~.dll: notch~.c
	cl $(PDNTCFLAGS) $(PDNTINCLUDE) /c $*.c
	link /dll /export:notch_tilde_setup $*.obj $(PDNTLIB)

peak~.dll: peak~.c
	cl $(PDNTCFLAGS) $(PDNTINCLUDE) /c $*.c
	link /dll /export:peak_tilde_setup $*.obj $(PDNTLIB)

# ----------------------- LINUX i386 -----------------------

pd_linux: obj1.l_ia64 obj2.l_ia64 obj3.l_ia64 obj4.l_ia64 \
    obj5.l_ia64 dspobj~.l_ia64

pd_linux32: obj1.l_i386 obj2.l_i386 obj3.l_i386 obj4.l_i386 \
    obj5.l_i386 dspobj~.l_i386

.SUFFIXES: .l_i386 .l_ia64

LINUXCFLAGS = -DPD -O2 -funroll-loops -fomit-frame-pointer \
    -Wall -W -Wshadow -Wstrict-prototypes -Werror \
    -Wno-unused -Wno-parentheses -Wno-switch

LINUXINCLUDE =  -I../../src

.c.l_i386:
	cc $(LINUXCFLAGS) $(LINUXINCLUDE) -o $*.o -c $*.c
	ld -shared -o $*.l_i386 $*.o -lc -lm
	strip --strip-unneeded $*.l_i386
	rm $*.o

.c.l_ia64:
	cc $(LINUXCFLAGS) $(LINUXINCLUDE) -fPIC -o $*.o -c $*.c
	ld -shared -o $*.l_ia64 $*.o -lc -lm
	strip --strip-unneeded $*.l_ia64
	rm $*.o

# ----------------------- Mac OSX -----------------------

pd_darwin: allpass~.pd_darwin bandpass~.pd_darwin fir~.pd_darwin \
	highpass~.pd_darwin highshelf~.pd_darwin \
	lowpass~.pd_darwin lowshelf~.pd_darwin notch~.pd_darwin \
	peak~.pd_darwin

.SUFFIXES: .pd_darwin

DARWINCFLAGS = -DPD -O2 -Wall -W -Wshadow -Wstrict-prototypes \
    -Wno-unused -Wno-parentheses -Wno-switch -arch i386 -arch x86_64

.c.pd_darwin:
	cc $(DARWINCFLAGS) $(LINUXINCLUDE) -o $*.o -c $*.c
	cc -bundle -undefined suppress -arch i386 -arch x86_64 \
            -flat_namespace -o $*.pd_darwin $*.o 
	rm -f $*.o

