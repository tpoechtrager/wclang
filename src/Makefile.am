AM_CXXFLAGS=-pedantic -Wall -Wextra -O3

bin_PROGRAMS = wclang
wclang_SOURCES = wclang.cpp wclang_time.cpp wclang_cache.cpp

install-exec-hook:
	ln -rfs $(DESTDIR)$(bindir)/wclang $(DESTDIR)$(bindir)/w32-clang
	ln -rfs $(DESTDIR)$(bindir)/wclang $(DESTDIR)$(bindir)/w32-clang++
	ln -rfs $(DESTDIR)$(bindir)/wclang $(DESTDIR)$(bindir)/w64-clang
	ln -rfs $(DESTDIR)$(bindir)/wclang $(DESTDIR)$(bindir)/w64-clang++
	ln -rfs $(DESTDIR)$(bindir)/wclang $(DESTDIR)$(bindir)/i486-mingw32-clang
	ln -rfs $(DESTDIR)$(bindir)/wclang $(DESTDIR)$(bindir)/i486-mingw32-clang++
	ln -rfs $(DESTDIR)$(bindir)/wclang $(DESTDIR)$(bindir)/i586-mingw32-clang
	ln -rfs $(DESTDIR)$(bindir)/wclang $(DESTDIR)$(bindir)/i586-mingw32-clang++
	ln -rfs $(DESTDIR)$(bindir)/wclang $(DESTDIR)$(bindir)/i586-mingw32msvc-clang
	ln -rfs $(DESTDIR)$(bindir)/wclang $(DESTDIR)$(bindir)/i586-mingw32msvc-clang++
	ln -rfs $(DESTDIR)$(bindir)/wclang $(DESTDIR)$(bindir)/amd64-mingw32msvc-clang
	ln -rfs $(DESTDIR)$(bindir)/wclang $(DESTDIR)$(bindir)/amd64-mingw32msvc-clang++
	ln -rfs $(DESTDIR)$(bindir)/wclang $(DESTDIR)$(bindir)/i686-w64-mingw32-clang
	ln -rfs $(DESTDIR)$(bindir)/wclang $(DESTDIR)$(bindir)/i686-w64-mingw32-clang++
	ln -rfs $(DESTDIR)$(bindir)/wclang $(DESTDIR)$(bindir)/i686-pc-mingw32-clang
	ln -rfs $(DESTDIR)$(bindir)/wclang $(DESTDIR)$(bindir)/i686-pc-mingw32-clang++
	ln -rfs $(DESTDIR)$(bindir)/wclang $(DESTDIR)$(bindir)/x86_64-w64-mingw32-clang
	ln -rfs $(DESTDIR)$(bindir)/wclang $(DESTDIR)$(bindir)/x86_64-w64-mingw32-clang++
	ln -rfs $(DESTDIR)$(bindir)/wclang $(DESTDIR)$(bindir)/x86_64-pc-mingw32-clang
	ln -rfs $(DESTDIR)$(bindir)/wclang $(DESTDIR)$(bindir)/x86_64-pc-mingw32-clang++
