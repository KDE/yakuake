METASOURCES = AUTO

KDE_ICON = AUTO

bin_PROGRAMS = yakuake

yakuake_SOURCES = dcop_interface.skel first_run_dialog.cpp \
	first_run_dialog_ui.ui general_settings.cpp general_settings_ui.ui \
	image_button.cpp main.cpp main_window.cpp session.cpp settings.kcfgc \
	skin_list_item.cpp skin_settings.cpp skin_settings_ui.ui \
	tabbed_widget.cpp tab_bar.cpp terminal.cpp terminal_focus_watcher.cpp \
	terminal_splitter.cpp title_bar.cpp

yakuake_LDFLAGS = $(KDE_RPATH) $(all_libraries)
yakuake_LDADD = $(LIB_KDEUI) $(LIB_KIO)

INCLUDES = -I$(top_srcdir)/. -I$(top_srcdir)/src $(all_includes)

kde_kcfg_DATA = yakuake.kcfg

xdg_apps_DATA = yakuake.desktop

shellrcdir = $(kde_datadir)/yakuake

noinst_HEADERS = dcop_interface.h first_run_dialog.h general_settings.h \
	image_button.h main_window.h session.h skin_list_item.h \
	skin_settings.h tabbed_widget.h tab_bar.h terminal.h \
	terminal_focus_watcher.h terminal_splitter.h title_bar.h
