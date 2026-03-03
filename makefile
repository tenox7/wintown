CC = cl
RC = rc
CFLAGS = /nologo /O2 /Ot /Oi /Z7 /W3 /I. /D_CRT_SECURE_NO_WARNINGS
#CFLAGS = -D_AXP64_=1 -D_ALPHA64_=1 -DALPHA=1 -DWIN64 -D_WIN64 -DWIN32 -D_WIN32  -Wp64 -W4 -Ap64
LIBS = gdi32.lib user32.lib kernel32.lib COMDLG32.lib

OBJS = anim.obj animtab.obj assets.obj budget.obj charts.obj disastr.obj eval.obj gdifix.obj main.obj mapgen.obj newgame.obj notify.obj s_disast.obj s_eval.obj s_gen.obj s_power.obj s_scan.obj s_simscan.obj s_spzone.obj s_traf.obj s_zone.obj scenario.obj sim.obj sprite.obj tiles.obj tools.obj

all: wintown.exe

.c.obj:
	$(CC) $(CFLAGS) /c $<

s_gen.obj: sim\s_gen.c
	$(CC) $(CFLAGS) /c /Fos_gen.obj sim\s_gen.c

s_simscan.obj: sim\s_simscan.c
	$(CC) $(CFLAGS) /c /Fos_simscan.obj sim\s_simscan.c

s_disast.obj: sim\s_disast.c
	$(CC) $(CFLAGS) /c /Fos_disast.obj sim\s_disast.c

s_eval.obj: sim\s_eval.c
	$(CC) $(CFLAGS) /c /Fos_eval.obj sim\s_eval.c

s_power.obj: sim\s_power.c
	$(CC) $(CFLAGS) /c /Fos_power.obj sim\s_power.c

s_scan.obj: sim\s_scan.c
	$(CC) $(CFLAGS) /c /Fos_scan.obj sim\s_scan.c

s_spzone.obj: sim\s_spzone.c
	$(CC) $(CFLAGS) /c /Fos_spzone.obj sim\s_spzone.c

s_traf.obj: sim\s_traf.c
	$(CC) $(CFLAGS) /c /Fos_traf.obj sim\s_traf.c

s_zone.obj: sim\s_zone.c
	$(CC) $(CFLAGS) /c /Fos_zone.obj sim\s_zone.c

wintown.res: wintown.rc
	$(RC) wintown.rc

wintown.exe: $(OBJS) wintown.res
	link /NOLOGO /DEBUG /OUT:wintown.exe $(OBJS) wintown.res $(LIBS)
        del /q /f *.obj *.res

clean:
	del /q /f *.obj *.res *.pdb *.ilk wintown.exe
