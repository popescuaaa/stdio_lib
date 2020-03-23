CC = cl
L = link
CFLAGS = /nologo /W3 /DDLL_IMPORTS

build: so_stdio.lib

so_stdio.lib: so_stdio.dll

so_stdio.dll: so_stdio.obj
	$(L) /nologo /dll /out:$@ /implib:so_stdio.lib $**

so_stdio.obj: so_stdio.c
	$(CC) $(CFLAGS) /c $**

clean:
	del /Q /F *.obj *.lib *.dll  
