/*-
 * Copyright (C) 2026 Lucas Holt. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/types.h>
#include <unistd.h>
#include <atf-c.h>
#include <gtk/gtk.h>
#include <math.h>

#include "mport-manager.h"

static void
init_gtk(void)
{
	if (!gtk_is_initialized()) {
		gtk_init();
	}
}

/* 1. mport_gtk_progress_init_cb */

ATF_TC(progress_init_null_bar);
ATF_TC_HEAD(progress_init_null_bar, tc)
{
	atf_tc_set_md_var(tc, "descr", "Test mport_gtk_progress_init_cb with NULL progressBar handles safely");
}
ATF_TC_BODY(progress_init_null_bar, tc)
{
	progressBar = NULL;
	mport_gtk_progress_init_cb("Test Init NULL");
	ATF_REQUIRE(progressBar == NULL);
}

ATF_TC(progress_init_valid);
ATF_TC_HEAD(progress_init_valid, tc)
{
	atf_tc_set_md_var(tc, "descr", "Test mport_gtk_progress_init_cb sets progress bar title text");
}
ATF_TC_BODY(progress_init_valid, tc)
{
	init_gtk();
	progressBar = gtk_progress_bar_new();
	ATF_REQUIRE(progressBar != NULL);

	mport_gtk_progress_init_cb("Installing Package");
	const char *text = gtk_progress_bar_get_text(GTK_PROGRESS_BAR(progressBar));
	ATF_REQUIRE_STREQ(text, "Installing Package");

	g_object_ref_sink(progressBar);
	g_object_unref(progressBar);
	progressBar = NULL;
}

/* 2. mport_gtk_progress_step_cb */

ATF_TC(progress_step_null_bar);
ATF_TC_HEAD(progress_step_null_bar, tc)
{
	atf_tc_set_md_var(tc, "descr", "Test mport_gtk_progress_step_cb with NULL progressBar handles safely");
}
ATF_TC_BODY(progress_step_null_bar, tc)
{
	progressBar = NULL;
	mport_gtk_progress_step_cb(50, 100, "Step NULL");
	ATF_REQUIRE(progressBar == NULL);
}

ATF_TC(progress_step_invalid_total);
ATF_TC_HEAD(progress_step_invalid_total, tc)
{
	atf_tc_set_md_var(tc, "descr", "Test mport_gtk_progress_step_cb with total <= 0 leaves fraction unchanged");
}
ATF_TC_BODY(progress_step_invalid_total, tc)
{
	init_gtk();
	progressBar = gtk_progress_bar_new();
	gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progressBar), 0.0);

	/* total == 0 */
	mport_gtk_progress_step_cb(5, 0, "Zero total");
	ATF_REQUIRE_EQ(gtk_progress_bar_get_fraction(GTK_PROGRESS_BAR(progressBar)), 0.0);

	/* total < 0 */
	mport_gtk_progress_step_cb(5, -10, "Negative total");
	ATF_REQUIRE_EQ(gtk_progress_bar_get_fraction(GTK_PROGRESS_BAR(progressBar)), 0.0);

	g_object_ref_sink(progressBar);
	g_object_unref(progressBar);
	progressBar = NULL;
}

ATF_TC(progress_step_normal);
ATF_TC_HEAD(progress_step_normal, tc)
{
	atf_tc_set_md_var(tc, "descr", "Test mport_gtk_progress_step_cb calculates fraction correctly");
}
ATF_TC_BODY(progress_step_normal, tc)
{
	init_gtk();
	progressBar = gtk_progress_bar_new();

	/* 50 / 100 -> 0.5 */
	mport_gtk_progress_step_cb(50, 100, "50%");
	double fraction = gtk_progress_bar_get_fraction(GTK_PROGRESS_BAR(progressBar));
	ATF_REQUIRE(fabs(fraction - 0.5) < 0.001);

	/* 1 / 4 -> 0.25 */
	mport_gtk_progress_step_cb(1, 4, "25%");
	fraction = gtk_progress_bar_get_fraction(GTK_PROGRESS_BAR(progressBar));
	ATF_REQUIRE(fabs(fraction - 0.25) < 0.001);

	g_object_ref_sink(progressBar);
	g_object_unref(progressBar);
	progressBar = NULL;
}

ATF_TC(progress_step_clamp_current);
ATF_TC_HEAD(progress_step_clamp_current, tc)
{
	atf_tc_set_md_var(tc, "descr", "Test mport_gtk_progress_step_cb clamps current when current > total");
}
ATF_TC_BODY(progress_step_clamp_current, tc)
{
	init_gtk();
	progressBar = gtk_progress_bar_new();

	/* 150 / 100 -> clamped to 100 / 100 -> 1.0 */
	mport_gtk_progress_step_cb(150, 100, "Overflow");
	double fraction = gtk_progress_bar_get_fraction(GTK_PROGRESS_BAR(progressBar));
	ATF_REQUIRE(fabs(fraction - 1.0) < 0.001);

	g_object_ref_sink(progressBar);
	g_object_unref(progressBar);
	progressBar = NULL;
}

ATF_TC(progress_step_boundaries);
ATF_TC_HEAD(progress_step_boundaries, tc)
{
	atf_tc_set_md_var(tc, "descr", "Test mport_gtk_progress_step_cb at 0% and 100% boundary conditions");
}
ATF_TC_BODY(progress_step_boundaries, tc)
{
	init_gtk();
	progressBar = gtk_progress_bar_new();

	/* 0 / 100 -> 0.0 */
	mport_gtk_progress_step_cb(0, 100, "0%");
	double fraction = gtk_progress_bar_get_fraction(GTK_PROGRESS_BAR(progressBar));
	ATF_REQUIRE(fabs(fraction - 0.0) < 0.001);

	/* 100 / 100 -> 1.0 */
	mport_gtk_progress_step_cb(100, 100, "100%");
	fraction = gtk_progress_bar_get_fraction(GTK_PROGRESS_BAR(progressBar));
	ATF_REQUIRE(fabs(fraction - 1.0) < 0.001);

	g_object_ref_sink(progressBar);
	g_object_unref(progressBar);
	progressBar = NULL;
}

/* 3. mport_gtk_progress_free_cb */

ATF_TC(progress_free_null_bar);
ATF_TC_HEAD(progress_free_null_bar, tc)
{
	atf_tc_set_md_var(tc, "descr", "Test mport_gtk_progress_free_cb with NULL progressBar handles safely");
}
ATF_TC_BODY(progress_free_null_bar, tc)
{
	progressBar = NULL;
	mport_gtk_progress_free_cb();
	ATF_REQUIRE(progressBar == NULL);
}

ATF_TC(progress_free_valid);
ATF_TC_HEAD(progress_free_valid, tc)
{
	atf_tc_set_md_var(tc, "descr", "Test mport_gtk_progress_free_cb sets 'Task Completed' text");
}
ATF_TC_BODY(progress_free_valid, tc)
{
	init_gtk();
	progressBar = gtk_progress_bar_new();

	mport_gtk_progress_free_cb();
	const char *text = gtk_progress_bar_get_text(GTK_PROGRESS_BAR(progressBar));
	ATF_REQUIRE_STREQ(text, "Task Completed");

	g_object_ref_sink(progressBar);
	g_object_unref(progressBar);
	progressBar = NULL;
}

/* 4. reset_progress_bar */

ATF_TC(reset_progress_null_bar);
ATF_TC_HEAD(reset_progress_null_bar, tc)
{
	atf_tc_set_md_var(tc, "descr", "Test reset_progress_bar with NULL progressBar handles safely");
}
ATF_TC_BODY(reset_progress_null_bar, tc)
{
	progressBar = NULL;
	reset_progress_bar();
	ATF_REQUIRE(progressBar == NULL);
}

ATF_TC(reset_progress_valid);
ATF_TC_HEAD(reset_progress_valid, tc)
{
	atf_tc_set_md_var(tc, "descr", "Test reset_progress_bar clears text and resets fraction to 0.0");
}
ATF_TC_BODY(reset_progress_valid, tc)
{
	init_gtk();
	progressBar = gtk_progress_bar_new();

	/* Set initial non-zero state */
	gtk_progress_bar_set_text(GTK_PROGRESS_BAR(progressBar), "In Progress");
	gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progressBar), 0.75);

	reset_progress_bar();

	const char *text = gtk_progress_bar_get_text(GTK_PROGRESS_BAR(progressBar));
	ATF_REQUIRE(text == NULL || strcmp(text, "") == 0);
	ATF_REQUIRE_EQ(gtk_progress_bar_get_fraction(GTK_PROGRESS_BAR(progressBar)), 0.0);

	g_object_ref_sink(progressBar);
	g_object_unref(progressBar);
	progressBar = NULL;
}

ATF_TP_ADD_TCS(tp)
{
	ATF_TP_ADD_TC(tp, progress_init_null_bar);
	ATF_TP_ADD_TC(tp, progress_init_valid);
	ATF_TP_ADD_TC(tp, progress_step_null_bar);
	ATF_TP_ADD_TC(tp, progress_step_invalid_total);
	ATF_TP_ADD_TC(tp, progress_step_normal);
	ATF_TP_ADD_TC(tp, progress_step_clamp_current);
	ATF_TP_ADD_TC(tp, progress_step_boundaries);
	ATF_TP_ADD_TC(tp, progress_free_null_bar);
	ATF_TP_ADD_TC(tp, progress_free_valid);
	ATF_TP_ADD_TC(tp, reset_progress_null_bar);
	ATF_TP_ADD_TC(tp, reset_progress_valid);

	return atf_no_error();
}
