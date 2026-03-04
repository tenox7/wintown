CC = cl
RC = rc
CFLAGS = /nologo /O2 /Ot /Oi /Z7 /I. /D_CRT_SECURE_NO_WARNINGS
#CFLAGS = -I. -D_AXP64_=1 -D_ALPHA64_=1 -DALPHA=1 -DWIN64 -D_WIN64 -DWIN32 -D_WIN32  -Wp64 -W4 -Ap64
LIBS = gdi32.lib user32.lib kernel32.lib COMDLG32.lib

LOCAL_OBJS = anim.obj animtab.obj assets.obj budget.obj charts.obj disastr.obj eval.obj gdifix.obj main.obj mapgen.obj newgame.obj notify.obj scenario.obj sim.obj sprite.obj tiles.obj tools.obj
SIM_OBJS = s_disast.obj s_eval.obj s_gen.obj s_power.obj s_scan.obj s_sim.obj s_traf.obj s_zone.obj
OBJS = $(LOCAL_OBJS) $(SIM_OBJS)

all: wintown.exe

.c.obj:
	$(CC) $(CFLAGS) /c $<

sim.obj: sim.c
	$(CC) $(CFLAGS) /c sim.c

{micropolis\src\sim\}.c.obj:
	$(CC) $(CFLAGS) /c /Fo$@ $<

s_sim.obj: micropolis\src\sim\s_sim.c
	$(CC) $(CFLAGS) /FIw_sim.h /c /Fos_sim.obj micropolis\src\sim\s_sim.c

wintown.res: wintown.rc
	$(RC) wintown.rc

wintown.exe: $(OBJS) wintown.res
	link /NOLOGO /DEBUG /OUT:wintown.exe $(OBJS) wintown.res $(LIBS)
        del /q /f *.obj *.res

clean:
	del /q /f *.obj *.res *.pdb *.ilk wintown.exe
