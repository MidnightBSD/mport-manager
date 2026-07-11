#include <sys/types.h>
#include <unistd.h>
#include <atf-c.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Mock GTK types & function declarations (or include them)
typedef void GtkProgressBar;
typedef char gchar;
typedef double gdouble;

#define GTK_PROGRESS_BAR(obj) ((GtkProgressBar *)(obj))

// Include our header under test
#include "../mport-progress.h"

// Mock state for verification
static char last_text[256] = {0};
static double last_fraction = -1.0;
static int set_text_called = 0;
static int set_fraction_called = 0;

void gtk_progress_bar_set_text(GtkProgressBar *pbar, const gchar *text) {
    set_text_called++;
    if (text) {
        strncpy(last_text, text, sizeof(last_text) - 1);
        last_text[sizeof(last_text) - 1] = '\0';
    } else {
        last_text[0] = '\0';
    }
}

void gtk_progress_bar_set_fraction(GtkProgressBar *pbar, gdouble fraction) {
    set_fraction_called++;
    last_fraction = fraction;
}

// Reset mocks between test runs
static void reset_mocks(void) {
    memset(last_text, 0, sizeof(last_text));
    last_fraction = -1.0;
    set_text_called = 0;
    set_fraction_called = 0;
}

ATF_TC_WITHOUT_HEAD(test_reset_progress_bar_null);
ATF_TC_BODY(test_reset_progress_bar_null, tc)
{
    reset_mocks();
    progressBar = NULL;

    reset_progress_bar();

    ATF_REQUIRE_EQ_MSG(set_text_called, 0, "gtk_progress_bar_set_text called when progressBar was NULL");
    ATF_REQUIRE_EQ_MSG(set_fraction_called, 0, "gtk_progress_bar_set_fraction called when progressBar was NULL");
}

ATF_TC_WITHOUT_HEAD(test_reset_progress_bar_not_null);
ATF_TC_BODY(test_reset_progress_bar_not_null, tc)
{
    reset_mocks();

    int dummy_widget = 42;
    progressBar = (GtkWidget *)&dummy_widget;

    reset_progress_bar();

    ATF_REQUIRE_EQ_MSG(set_text_called, 1, "gtk_progress_bar_set_text should be called exactly once");
    ATF_REQUIRE_EQ_MSG(set_fraction_called, 1, "gtk_progress_bar_set_fraction should be called exactly once");
    ATF_REQUIRE_STREQ_MSG(last_text, "", "progress bar text should be empty");
    ATF_REQUIRE_EQ_MSG(last_fraction, 0.0, "progress bar fraction should be 0.0");
}

ATF_TP_ADD_TCS(tp)
{
    ATF_TP_ADD_TC(tp, test_reset_progress_bar_null);
    ATF_TP_ADD_TC(tp, test_reset_progress_bar_not_null);
    return atf_no_error();
}
