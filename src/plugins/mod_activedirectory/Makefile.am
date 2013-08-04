pkglib_LTLIBRARIES = mod_activedirectory.la
mod_activedirectory_la_SOURCES = webcontrol_activedirectory.c
mod_activedirectory_la_CFLAGS = -I$(top_builddir)/src
mod_activedirectory_la_LDFLAGS = -module -export-dynamic -avoid-version -no-undefined
mod_activedirectory_la_LIBADD = -lldap_r
#mod_activedirectory_la_LIBADD = $(MYSQL_LIBS) $(common_libadd)
#mod_activedirectory_la_CPPFLAGS = $(MYSQL_INCLUDE)

