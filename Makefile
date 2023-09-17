WAYLAND_SCANNER=$(shell pkg-config --variable=wayland_scanner wayland-scanner)

protocols/wlr-output-management-unstable-v1-protocol.h:
	$(WAYLAND_SCANNER) client-header \
		protocols/wlr-output-management-unstable-v1.xml $@

protocols/wlr-output-management-unstable-v1-protocol.c:
	$(WAYLAND_SCANNER) private-code \
		protocols/wlr-output-management-unstable-v1.xml $@

protocols/wlr-output-management-unstable-v1-protocol.o: protocols/wlr-output-management-unstable-v1-protocol.h

protocols: protocols/wlr-output-management-unstable-v1-protocol.o

clear:
	rm -rf build
	rm -f *.o *-protocol.h *-protocol.c

release:
	mkdir -p build && cmake --no-warn-unused-cli -DCMAKE_BUILD_TYPE:STRING=Release -B./build -G "Unix Makefiles"
	cmake --build ./build --config Release --target all -j 10

debug:
	mkdir -p build && cmake --no-warn-unused-cli -DCMAKE_BUILD_TYPE:STRING=Debug -B./build -G "Unix Makefiles"
	cmake --build ./build --config Debug --target all -j 10

all:
	make clear
	make protocols
	make release

install:
	make all
	cp -f ./build/wlr-autorotate /usr/bin