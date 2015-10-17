
AUTOMAKE_OPTIONS = subdir-objects

LIBS = @libcrypto@ @libnetwork@ @libsound@ @libthread@ @libwindow@

bin_PROGRAMS = tribunal

tribunal_SOURCES = src/main.cpp \
                   src/engine/application/window.cpp
tribunal_CPPFLAGS = -g -rdynamic -Wall -std=c++11 -I$(top_builddir) -I$(top_srcdir)/src

noinst_PROGRAMS = tribunal-check

tribunal_check_SOURCES = tst/main.cpp
tribunal_check_CPPFLAGS = -g -rdynamic -Wall -std=c++11 -I$(top_builddir) -I$(top_srcdir)/src
