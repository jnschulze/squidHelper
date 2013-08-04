bin_PROGRAMS = webcontrol_helper
webcontrol_helper_SOURCES = main.c webcontrol_buffer.c webcontrol_plugin.c webcontrol_request.c webcontrol_netutil.c
webcontrol_helper_CFLAGS = -I$(top_builddir)/src
webcontrol_helper_LDFLAGS = -export-dynamic

#webcontrol_helper_LDADD = ${noinst_LIBRARIES}
webcontrol_helper_LDADD = ${top_builddir}/src/thirdparty/c-pthread-queue/libqueue.a
