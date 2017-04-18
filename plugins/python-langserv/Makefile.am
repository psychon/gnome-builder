if ENABLE_PYTHON_LANGSERV_PLUGIN

EXTRA_DIST = $(plugin_DATA)

plugindir = $(libdir)/gnome-builder/plugins
dist_plugin_DATA =           \
	python-langserv.plugin \
	python_langserv.py

endif

include $(top_srcdir)/plugins/Makefile.plugin

-include $(top_srcdir)/git.mk
