AM_CXXFLAGS=-pedantic -Wall -Wextra

bin_PROGRAMS = wclang
wclang_SOURCES = wclang.cpp wclang_time.cpp wclang_cache.cpp

install-exec-hook:
	ln -s $(bindir)/wclang $(bindir)/w32-clang
	ln -s $(bindir)/wclang $(bindir)/w32-clang++
	ln -s $(bindir)/wclang $(bindir)/w64-clang
	ln -s $(bindir)/wclang $(bindir)/w64-clang++
