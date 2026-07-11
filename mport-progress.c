#include "mport-progress.h"

#ifdef TEST_SUITE
typedef void GtkProgressBar;
typedef char gchar;
typedef double gdouble;
#define GTK_PROGRESS_BAR(obj) ((GtkProgressBar *)(obj))
void gtk_progress_bar_set_text(GtkProgressBar *pbar, const gchar *text);
void gtk_progress_bar_set_fraction(GtkProgressBar *pbar, gdouble fraction);
#endif

GtkWidget *progressBar = NULL;

void
reset_progress_bar(void)
{
	if (progressBar == NULL)
		return;

	gtk_progress_bar_set_text(GTK_PROGRESS_BAR(progressBar), "");
	gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progressBar), 0.0);
}
