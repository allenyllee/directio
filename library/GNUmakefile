#

CFLAGS=-Os
LDFLAGS=-s -shared -Wl,--add-stdcall-alias

X32=i686-pc-mingw32
CC32=$(X32)-gcc
RC32=$(X32)-windres

X64=x86_64-pc-mingw32
CC64=$(X64)-gcc
RC64=$(X64)-windres

all: DirectIOLib32.dll DirectIOLibx64.dll

DirectIOLib32.o: directiolib.c directiolib.h
	$(CC32) -c -o $@ $(CFLAGS) $<

DirectIOLib32.dll: DirectIOLib32.o directiolib_rc32.o
	$(CC32) $^ $(LDFLAGS) -o $@ -Wl,--out-implib,$@.a

directiolib_rc32.o: directiolib.rc
	$(RC32) -o $@ $<

DirectIOLibx64.o: directiolib.c directiolib.h
	$(CC64) -c -o $@ $(CFLAGS) $<

DirectIOLibx64.dll: DirectIOLibx64.o directiolib_rcx64.o
	$(CC64) $^ $(LDFLAGS) -o $@ -Wl,--out-implib,$@.a

directiolib_rcx64.o: directiolib.rc
	$(RC64) -o $@ $<

clean:
	rm *.o *.dll *.a
