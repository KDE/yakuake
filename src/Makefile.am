METASOURCES = AUTO

KDE_ICON = AUTO

# this is the program that gets installed.  it's name is used for all
# of the other Makefile.am variables
bin_PROGRAMS = yakuake

# the application source, library search path, and link libraries
yakuake_SOURCES = main.cpp main_window.cpp title_bar.cpp image_button.cpp \
	tabs_bar.cpp tabbed_widget.cpp dcop_interface.skel shell_session.cpp
yakuake_LDFLAGS = $(KDE_RPATH) $(all_libraries)
yakuake_LDADD = $(LIB_KDEUI)

# set the include path for X, qt and KDE
INCLUDES = -I$(top_srcdir)/. -I$(top_srcdir)/src $(all_includes)

# this is where the desktop file will go
shelldesktopdir = $(kde_appsdir)/Utilities
shelldesktop_DATA = yakuake.desktop

# this is where the shell's XML-GUI resource file goes
shellrcdir = $(kde_datadir)/yakuake

messages: rc.cpp
	$(EXTRACTRC) `find . -name \*.ui -o -name \*.rc` > rc.cpp
	$(XGETTEXT) *.cpp -o $(podir)/yakuake.pot

noinst_HEADERS = title_bar.h image_button.h tabs_bar.h tabbed_widget.h \
	dcop_interface.h shell_session.h
