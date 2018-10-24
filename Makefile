.PHONY: all qtgui/c1demo tracer/linux clean

all: qtgui/c1demo tracer/linux

qtgui/c1demo:
	qmake -o qtgui/Makefile qtgui/c1demo.pro
	make -j 8 -C qtgui

tracer/linux:
	make -C tracer

clean:
	make -C qtgui clean
	rm -f qtgui/.qmake.stash
	rm -f qtgui/Makefile
	make -C tracer clean
