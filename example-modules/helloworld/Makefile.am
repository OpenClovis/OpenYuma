netconfmodule_LTLIBRARIES = libhelloworld.la

libhelloworld_la_SOURCES = \
$(top_srcdir)/example-modules/helloworld/helloworld.c

libhelloworld_la_CPPFLAGS = -I $(top_srcdir)/netconf/src/ -I$(top_srcdir)/netconf/src/agt -I$(top_srcdir)/netconf/src/mgr -I$(top_srcdir)/netconf/src/ncx -I$(top_srcdir)/netconf/src/platform -I$(top_srcdir)/netconf/src/ydump -I/usr/include/libxml2 -I/usr/include/libxml2/libxml
libhelloworld_la_LDFLAGS = -module $(top_builddir)/netconf/src/agt/libagt.la $(top_builddir)/netconf/src/ncx/libncx.la -lxml2 -lz  -ldl

yang_DATA = $(top_srcdir)/example-modules/helloworld/helloworld.yang
