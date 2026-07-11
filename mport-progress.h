#ifndef MPORT_PROGRESS_H
#define MPORT_PROGRESS_H

#ifdef TEST_SUITE
#include <stddef.h>
typedef void GtkWidget;
#else
#include <gtk/gtk.h>
#endif

extern GtkWidget *progressBar;

void reset_progress_bar(void);

#endif
