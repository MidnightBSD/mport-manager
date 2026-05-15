/*-
 * Copyright (C) 2008, 2016, 2026 Lucas Holt. All rights reserved.
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

#include <sys/param.h>

#include <gtk/gtk.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <err.h>
#include <string.h>
#include <locale.h>

#include <libutil.h>

#include <mport.h>

#define NAME "MidnightBSD Package Manager"
#define ICONFILE "/usr/local/share/mport/icon.png"
#define MPORT_LOCAL_PKG_PATH "/var/db/mport/downloads"

GtkWidget *window;
GtkWidget *search; /* textboxes */
GtkWidget *tree;
GtkWidget *installedTree;
GtkWidget *updateTree;
GtkWidget *progressBar = NULL;

mportInstance *mport;

struct available_detail {
	GtkWidget *image; // icon
	GtkWidget *labelVersion;
	GtkWidget *labelName;
	GtkWidget *label; // comment
	GtkWidget *labelLicense;
	GtkWidget *labelType;
	GtkWidget *installButton;
};

struct available_detail detail;

struct stats_detail {
	GtkWidget *labelInstalledPackages;
	GtkWidget *labelDiskSpaceOccupied;
	GtkWidget *labelPackagesAvailable;
};

struct stats_detail stats;

/*
 * Installed Software Tab's selected name
 */
char selectedInstalled[256] = {'\0'};
char selectedInstalledVersion[32] = {'\0'};

enum
{
	TITLE_COLUMN,
	VERSION_COLUMN,
	N_COLUMNS
};

enum
{
	INST_TITLE_COLUMN,
	INST_VERSION_COLUMN,
	INST_FLATSIZE_COLUMN,
	INST_LOCK_STATUS_COLUMN,
	INST_N_COLUMNS
};

enum
{
	UPD_TITLE_COLUMN,
	UPD_VERSION_COLUMN,
	UPD_OS_RELEASE_COLUMN,
	UPD_NEW_VERSION_COLUMN,
	UPD_N_COLUMNS
};

static void reload_installed(void);
static void reload_updates(void);
static void refresh_stats(void);
static void do_search(void);
static void button_clicked(GtkButton *button, GtkWindow *parent);
static void reset_search_button_clicked(GtkButton *button, GtkWindow *parent);
static void update_button_clicked(GtkButton *button, GtkWindow *parent);
static void install_button_clicked(GtkButton *button, GtkWidget *parent);
static void installed_delete_button_clicked(GtkButton *button, GtkWidget *parent);
static void msgbox(GtkWindow *parent, const char * msg);
static void msgbox_modal(GtkWindow *parent, const char *title, const char *button_label, const char *msg);
static bool msgbox_bool(GtkWindow *parent, const char *msg);
static void cut_clicked (GtkButton *, GtkEditable *);
static void copy_clicked (GtkButton *, GtkEditable *);
static void paste_clicked (GtkButton *, GtkEditable *);
static int msgbox_select(GtkWindow *parent, const char *msg, mportIndexEntry **choices, int def);
static void setup_tree(void);
static void create_installed_tree(void);

static GtkWidget *
create_app_icon_image(int pixel_size)
{
	const char *const icon_paths[] = {ICONFILE, "icon.png", NULL};

	for (size_t i = 0; icon_paths[i] != NULL; i++) {
		if (g_file_test(icon_paths[i], G_FILE_TEST_EXISTS)) {
			GtkWidget *image = gtk_image_new_from_file(icon_paths[i]);
			gtk_image_set_pixel_size(GTK_IMAGE(image), pixel_size);
			return image;
		}
	}

	GtkWidget *image = gtk_image_new_from_icon_name("dialog-information-symbolic");
	gtk_image_set_pixel_size(GTK_IMAGE(image), pixel_size);
	return image;
}

static void
set_window_app_icon(GtkWindow *window)
{
	gtk_window_set_icon_name(window, "mport-manager");
}
static void create_update_tree(void);
static void populate_installed_packages(GtkTreeStore *);
static void populate_update_packages(GtkTreeStore *); 
static void populate_remote_index(GtkTreeStore *store);
static void search_remote_index(GtkTreeStore *store, const char *query);
static GtkWidget *create_header_bar(GtkWidget *window, GtkWidget *search);
static void create_detail_box(GtkWidget *parent);
static void create_stats_box(GtkWidget *parent);
static void available_row_click_handler(GtkTreeView *treeView, GtkTreePath *path, GtkTreeViewColumn *column, gpointer data);
static void installed_tree_available_row_click_handler(GtkTreeView *treeView, GtkTreePath *path, GtkTreeViewColumn *column, gpointer data);
static void available_cursor_changed_handler(GtkTreeView *treeView, gpointer data);
static void installed_cursor_changed_handler(GtkTreeView *treeView, gpointer data);
void reset_progress_bar(void);
static void lock_button_clicked(GtkButton *button, GtkWidget *parent);
static void unlock_button_clicked(GtkButton *button, GtkWidget *parent);
static mportPackageMeta** lookup_for_lock(mportInstance *mport, const char *packageName);
static gint run_dialog_sync(GtkDialog *dialog);

// mport stuff
static int delete(const char *);
static int install(mportInstance *, const char *);
static int install_depends(mportInstance *mport, const char *packageName, const char *version, mportAutomatic automatic);
static mportIndexEntry ** lookupIndex(mportInstance *, const char *);
static int lock(mportInstance *mport, const char *packageName);
static int unlock(mportInstance *mport, const char *packageName);

/* Callbacks */
void mport_gtk_msg_cb(const char *msg);
int mport_gtk_confirm_cb(const char *msg, const char *yes, const char *no, int def);
int mport_gtk_select_cb(const char *msg, mportIndexEntry **choices, int def);
void mport_gtk_progress_init_cb(const char *title);
void mport_gtk_progress_step_cb(int current, int total, const char *msg);
void mport_gtk_progress_free_cb(void);

static void
activate(GtkApplication *app, gpointer user_data)
{
	GtkWidget *vbox, *authbox, *vauthbox;
	GtkWidget *submit; // search button
	GtkWidget *resetSearchButton;
	GtkWidget *scrolled_win;
	GtkWidget *stackSwitcher;
	GtkWidget *stack;

	// setup env for mport
	if (setenv("POSIXLY_CORRECT", "1", 1) == -1)
		err(EXIT_FAILURE, "setenv() failed");
	setlocale(LC_ALL, "");

	// hint to libmport to act differently depending on whether we're in a GUI or console context
	// used to avoid stopping services until triggers can be implemented.
	if (setenv("MPORT_GUI", "1", 1) == -1)
		err(EXIT_FAILURE, "setenv() failed");

	mport = mport_instance_new();
	if (mport == NULL) {
		errx(EXIT_FAILURE, "mport_instance_new() failed");
	}

	// init window
	window = gtk_application_window_new(app);
	gtk_window_set_title(GTK_WINDOW(window), NAME);
	gtk_widget_set_size_request(window, 800, 600);

	progressBar = gtk_progress_bar_new();
	gtk_progress_bar_set_show_text(GTK_PROGRESS_BAR(progressBar), TRUE);

#if defined(DEBUG)
	g_print ("Initializing mport instance");
#endif

	if (mport_instance_init(mport, NULL, NULL, false, MPORT_VNORMAL) != MPORT_OK) {
		errx(1, "%s", mport_err_string());
	}
	mport->force = false;

	/* Setup callbacks */
	mport_set_msg_cb(mport, &mport_gtk_msg_cb);
	mport_set_confirm_cb(mport, &mport_gtk_confirm_cb);
	mport_set_select_cb(mport, &mport_gtk_select_cb);
	mport_set_progress_init_cb(mport, &mport_gtk_progress_init_cb);
	mport_set_progress_step_cb(mport, &mport_gtk_progress_step_cb);
	mport_set_progress_free_cb(mport, &mport_gtk_progress_free_cb);

	if (mport_index_load(mport) != MPORT_OK) {
		mport_instance_free(mport);
		// In GTK4 we should ideally use GtkAlertDialog, but for now we'll try to use a simplified msgbox
		// or just exit if we can't even show a window yet.
		fprintf(stderr, "Unable to load updates index: %s\n", mport_err_string());
		exit(1);
	}

	// create search button
	submit = gtk_button_new_with_mnemonic("_Search");
	g_signal_connect(G_OBJECT(submit), "clicked",
	                 G_CALLBACK(button_clicked),
	                 (gpointer) window);

	// create reset search button
	resetSearchButton = gtk_button_new_with_mnemonic("_Reset");
	g_signal_connect(G_OBJECT(resetSearchButton), "clicked",
	                 G_CALLBACK(reset_search_button_clicked),
	                 (gpointer) window);

	setup_tree();
	create_installed_tree();
	create_update_tree();

	stackSwitcher = gtk_stack_switcher_new();
	stack = gtk_stack_new();
	search = gtk_entry_new();

	authbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
	gtk_box_append(GTK_BOX(authbox), search);
	gtk_box_append(GTK_BOX(authbox), submit);
	gtk_box_append(GTK_BOX(authbox), resetSearchButton);
	gtk_widget_set_hexpand(search, TRUE);

	vauthbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
	gtk_box_append(GTK_BOX(vauthbox), authbox);

	/* Setup the final box for layout in the window */
	vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
	gtk_box_append(GTK_BOX(vbox), vauthbox);

	scrolled_win = gtk_scrolled_window_new();
	gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scrolled_win), tree);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_win),
	                               GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);

	gtk_box_append(GTK_BOX(vbox), scrolled_win);
	gtk_widget_set_vexpand(scrolled_win, TRUE);
	create_detail_box(vbox);


	// Scroll for install packages
	GtkWidget *scrolled_installed = gtk_scrolled_window_new();
	gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scrolled_installed), installedTree);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_installed),
	                               GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);
	// create remove button on Installed Software
	GtkWidget *removeInstalledAppButton = gtk_button_new_with_mnemonic("_Remove Application");
	g_signal_connect(G_OBJECT(removeInstalledAppButton), "clicked",
	                 G_CALLBACK(installed_delete_button_clicked),
	                 (gpointer) window);
	// Create lock button
    GtkWidget *lockButton = gtk_button_new_with_mnemonic("_Lock");
    g_signal_connect(G_OBJECT(lockButton), "clicked",
                     G_CALLBACK(lock_button_clicked),
                     (gpointer) window);

    // Create unlock button
    GtkWidget *unlockButton = gtk_button_new_with_mnemonic("_Unlock");
    g_signal_connect(G_OBJECT(unlockButton), "clicked",
                     G_CALLBACK(unlock_button_clicked),
                     (gpointer) window);
	// create installed box
	GtkWidget *installedBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
	gtk_box_append(GTK_BOX(installedBox), scrolled_installed);
	gtk_widget_set_vexpand(scrolled_installed, TRUE);
	GtkWidget *buttonBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
	gtk_box_append(GTK_BOX(buttonBox), removeInstalledAppButton);
    gtk_box_append(GTK_BOX(buttonBox), lockButton);
    gtk_box_append(GTK_BOX(buttonBox), unlockButton);
	gtk_widget_set_hexpand(removeInstalledAppButton, TRUE);
	gtk_widget_set_hexpand(lockButton, TRUE);
	gtk_widget_set_hexpand(unlockButton, TRUE);
	gtk_box_append(GTK_BOX(installedBox), buttonBox);


	// Scroll for updates
	GtkWidget *scrolled_updates = gtk_scrolled_window_new();
	gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scrolled_updates), updateTree);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_updates),
	                               GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);
	// create update button
	GtkWidget *updateButton = gtk_button_new_with_mnemonic("_Upgrade Installed Software");
	g_signal_connect(G_OBJECT(updateButton), "clicked",
	                 G_CALLBACK(update_button_clicked),
	                 (gpointer) window);
	// create update box
	GtkWidget *updateBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
	gtk_box_append(GTK_BOX(updateBox), scrolled_updates);
	gtk_widget_set_vexpand(scrolled_updates, TRUE);
	gtk_box_append(GTK_BOX(updateBox), updateButton);
	// create stats box
	GtkWidget *statsBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
	create_stats_box(statsBox);
	
	// add all the stacks
	gtk_stack_add_titled(GTK_STACK(stack), vbox, "page-1", "Available Software");
	gtk_stack_add_titled(GTK_STACK(stack), installedBox, "page-2", "Installed Software");
	gtk_stack_add_titled(GTK_STACK(stack), updateBox, "page-3", "Updates");
	gtk_stack_add_titled(GTK_STACK(stack), statsBox, "page-4", "Stats");

	gtk_stack_set_visible_child_name(GTK_STACK(stack), "page-1");

	gtk_stack_switcher_set_stack(GTK_STACK_SWITCHER(stackSwitcher), GTK_STACK(stack));

	GtkWidget *header_bar = create_header_bar(window, search);
	gtk_window_set_titlebar(GTK_WINDOW(window), header_bar);

	GtkWidget *stackHolder = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
	gtk_box_append(GTK_BOX(stackHolder), stackSwitcher);
	gtk_box_append(GTK_BOX(stackHolder), stack);
	gtk_widget_set_vexpand(stack, TRUE);
	gtk_box_append(GTK_BOX(stackHolder), progressBar);

	gtk_window_set_child(GTK_WINDOW(window), stackHolder);
	gtk_window_present(GTK_WINDOW(window));
}

int
main(int argc, char *argv[])
{
	GtkApplication *app;
	int status;

	app = gtk_application_new("org.midnightbsd.mport-manager", G_APPLICATION_DEFAULT_FLAGS);
	g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
	status = g_application_run(G_APPLICATION(app), argc, argv);
	g_object_unref(app);

	mport_instance_free(mport);
	return status;
}

void
mport_gtk_msg_cb(const char *msg)
{

	msgbox(GTK_WINDOW(window), msg);
}

int
mport_gtk_confirm_cb(const char *msg, const char *yes, const char *no, int def)
{
	bool response;

	response = msgbox_bool(GTK_WINDOW(window), msg);
	if (response)
		return MPORT_OK;
	else
		return -1;
}

int
mport_gtk_select_cb(const char *msg, mportIndexEntry **choices, int def)
{
	return msgbox_select(GTK_WINDOW(window), msg, choices, def);
}

void
mport_gtk_progress_init_cb(const char *title)
{

	if (progressBar == NULL)
		return;
	gtk_progress_bar_set_text(GTK_PROGRESS_BAR(progressBar), title);
}

void
mport_gtk_progress_step_cb(int current, int total, const char *msg)
{
	gdouble percent;

	if (progressBar == NULL)
		return;

	if (total <= 0)
		return;

	if (current > total)
		current = total;

	percent = (gdouble) current / (gdouble) total;

	gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progressBar), percent);
}

void
mport_gtk_progress_free_cb(void)
{
	if (progressBar == NULL)
		return;

	gtk_progress_bar_set_text(GTK_PROGRESS_BAR(progressBar), "Task Completed");
}

void
reset_progress_bar(void)
{
	if (progressBar == NULL)
		return;

	gtk_progress_bar_set_text(GTK_PROGRESS_BAR(progressBar), "");
	gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progressBar), 0.0);
}

static void
lock_button_clicked(GtkButton *button, GtkWidget *parent)
{
	g_print("Lock button clicked. Selected package: %s\n", selectedInstalled);

    if (selectedInstalled[0] != '\0') {
        int result = lock(mport, selectedInstalled);
        if (result == MPORT_OK) {
            g_print("Successfully locked package: %s\n", selectedInstalled);
            msgbox(GTK_WINDOW(window), "Package locked successfully.");
			reload_installed();
        } else {
            g_print("Failed to lock package: %s\n", selectedInstalled);
            msgbox(GTK_WINDOW(window), "Failed to lock package.");
        }
    } else {
        msgbox(GTK_WINDOW(window), "Please select a package to lock.");
    }
}

static void
unlock_button_clicked(GtkButton *button, GtkWidget *parent)
{
	g_print("Unlock button clicked. Selected package: %s\n", selectedInstalled);

    if (selectedInstalled[0] != '\0') {
        int result = unlock(mport, selectedInstalled);
        if (result == MPORT_OK) {
            g_print("Successfully unlocked package: %s\n", selectedInstalled);
            msgbox(GTK_WINDOW(window), "Package unlocked successfully.");
			reload_installed();
        } else {
            g_print("Failed to unlock package: %s\n", selectedInstalled);
            msgbox(GTK_WINDOW(window), "Failed to unlock package.");
        }
    } else {
        msgbox(GTK_WINDOW(window), "Please select a package to unlock.");
    }
}


static void
installed_tree_available_row_click_handler(GtkTreeView *treeView, GtkTreePath *path, GtkTreeViewColumn *column,
                                           gpointer data)
{
	GtkTreeIter iter;
	GtkTreeModel *model;
	mportIndexEntry **indexEntries = NULL;

	reset_progress_bar();

	model = gtk_tree_view_get_model(treeView);

	if (gtk_tree_model_get_iter(model, &iter, path)) {
		gchar *name = NULL;
		gchar *version = NULL;

		gtk_tree_model_get(model, &iter, INST_TITLE_COLUMN, &name, -1);
		gtk_tree_model_get(model, &iter, INST_VERSION_COLUMN, &version, -1);

		if (name && version) {
			selectedInstalled[0] = '\0';
			selectedInstalledVersion[0] = '\0';
			if (mport_index_lookup_pkgname(mport, name, &indexEntries) != MPORT_OK) {
				g_warning("Error looking up package name %s: %s", name, mport_err_string());
			} else if (indexEntries != NULL) {
				for (mportIndexEntry **entry = indexEntries; entry != NULL && *entry != NULL; entry++) {
					if ((*entry)->version != NULL && mport_version_cmp(version, (*entry)->version) == 0) {
						g_strlcpy(selectedInstalled, name, sizeof(selectedInstalled));
						g_strlcpy(selectedInstalledVersion, version, sizeof(selectedInstalledVersion));
						break;
					}
				}
				mport_index_entry_free_vec(indexEntries);
			}
		} else {
			selectedInstalled[0] = '\0';
			selectedInstalledVersion[0] = '\0';
		}

		g_free(name);
		g_free(version);
	}
}

static void
installed_cursor_changed_handler(GtkTreeView *treeView, gpointer data)
{
	GtkTreePath *path = NULL;
	GtkTreeViewColumn *column = NULL;

	gtk_tree_view_get_cursor(treeView, &path, &column);
	if (path == NULL)
		return;

	installed_tree_available_row_click_handler(treeView, path, column, data);
	gtk_tree_path_free(path);
}


static void
available_row_click_handler(GtkTreeView *treeView, GtkTreePath *path, GtkTreeViewColumn *column, gpointer data)
{
	GtkTreeIter iter;
	GtkTreeModel *model;
	mportIndexEntry **indexEntries;

	reset_progress_bar();

	model = gtk_tree_view_get_model(treeView);

	if (gtk_tree_model_get_iter(model, &iter, path)) {
		gchar *name;
		gchar *version;

		gtk_tree_model_get(model, &iter, TITLE_COLUMN, &name, -1);
		gtk_tree_model_get(model, &iter, VERSION_COLUMN, &version, -1);

		if (mport_index_lookup_pkgname(mport, name, &indexEntries) != MPORT_OK) {
			char *msg;
			if (asprintf(&msg, "Error looking up package name %s: %s\n", name, mport_err_string()) != -1) {
				(mport->msg_cb)(msg);
				free(msg);
			}
			return;
		}

		if (indexEntries != NULL) {
			mportIndexEntry **indexEntriesHead = indexEntries;
			while (*indexEntries != NULL) {
				if ((*indexEntries)->version != NULL && mport_version_cmp(version, (*indexEntries)->version) == 0) {

					gtk_label_set_text(GTK_LABEL(detail.label), (*indexEntries)->comment);
					gtk_label_set_text(GTK_LABEL(detail.labelVersion), (*indexEntries)->version);
					gtk_label_set_text(GTK_LABEL(detail.labelName), (*indexEntries)->pkgname);
					gtk_label_set_text(GTK_LABEL(detail.labelLicense), (*indexEntries)->license);
					gtk_label_set_text(GTK_LABEL(detail.labelType), (*indexEntries)->type == MPORT_TYPE_SYSTEM ? "System" : "Application");

					break;
				}
				indexEntries++;
			}
			mport_index_entry_free_vec(indexEntriesHead);
		}

	#if defined(DEBUG)
			g_print ("The row containing the name '%s' has been double-clicked.\n", name);
	#endif

			g_free(name);
			g_free(version);
		}
	}

static void
available_cursor_changed_handler(GtkTreeView *treeView, gpointer data)
{
	GtkTreePath *path = NULL;
	GtkTreeViewColumn *column = NULL;

	gtk_tree_view_get_cursor(treeView, &path, &column);
	if (path == NULL)
		return;

	available_row_click_handler(treeView, path, column, data);
	gtk_tree_path_free(path);
}

/**
 * @brief Creates and populates a box with package statistics.
 *
 * This function creates a vertical box containing labels for various package statistics,
 * including the number of installed packages, disk space occupied, and available packages.
 * The statistics are organized into sections for local and remote package databases.
 *
 * @param parent The parent widget to which the stats box will be added.
 *
 * @return This function does not return a value.
 */

static void
refresh_stats(void)
{
	char flatsize_str[10];
	char installed_str[16];
	char available_str[16];
	mportStats *s = NULL;
	if (mport_stats(mport, &s) != MPORT_OK)
	{
		const char* err_msg = mport_err_string();
        g_warning("Failed to get mport stats: %s", err_msg);
        msgbox(GTK_WINDOW(window), err_msg);  // Display error to user
      
		return;
	}

	if (s == NULL)
    {
        g_warning("mport_stats returned NULL");
        msgbox(GTK_WINDOW(window), "Failed to retrieve package statistics");
        return;
    }
	
	if (snprintf(installed_str, sizeof(installed_str), "%u", s->pkg_installed) < 0)
    {
        g_warning("Error formatting installed package count");
    }

    if (snprintf(available_str, sizeof(available_str), "%u", s->pkg_available) < 0)
    {
        g_warning("Error formatting available package count");
    }

	if (humanize_number(flatsize_str, sizeof(flatsize_str), s->pkg_installed_size, "B",
	 HN_AUTOSCALE, HN_DECIMAL | HN_IEC_PREFIXES) < 0)
    {
        g_warning("Error formatting installed size");
        snprintf(flatsize_str, sizeof(flatsize_str), "%lld B", (long long)s->pkg_installed_size);
    }
	gtk_label_set_text(GTK_LABEL(stats.labelInstalledPackages), installed_str);
	gtk_label_set_text(GTK_LABEL(stats.labelDiskSpaceOccupied), flatsize_str);
	gtk_label_set_text(GTK_LABEL(stats.labelPackagesAvailable), available_str);

	mport_stats_free(s);
}


static void
create_stats_box(GtkWidget *parent)
{
	// create labels
	stats.labelDiskSpaceOccupied = gtk_label_new("");
	stats.labelInstalledPackages = gtk_label_new("");
	stats.labelPackagesAvailable = gtk_label_new("");

	GtkWidget *grid = gtk_grid_new();
	gtk_grid_set_column_spacing(GTK_GRID(grid), 12);
	gtk_grid_set_row_spacing(GTK_GRID(grid), 8);

	GtkWidget *localPackageDatabase = gtk_label_new("Local package database");
	gtk_widget_set_halign(localPackageDatabase, GTK_ALIGN_START);
	gtk_label_set_markup(GTK_LABEL(localPackageDatabase), "<b><span size=\"larger\">Local package database</span></b>");
	gtk_grid_attach(GTK_GRID(grid), localPackageDatabase, 0, 0, 2, 1);

	GtkWidget *installedPackagesLabel = gtk_label_new("Installed packages:");
	gtk_widget_set_halign(installedPackagesLabel, GTK_ALIGN_END);
	gtk_grid_attach(GTK_GRID(grid), installedPackagesLabel, 0, 1, 1, 1);
	gtk_widget_set_halign(stats.labelInstalledPackages, GTK_ALIGN_START);
	gtk_grid_attach(GTK_GRID(grid), stats.labelInstalledPackages, 1, 1, 1, 1);

	GtkWidget *diskSpaceLabel = gtk_label_new("Disk space used:");
	gtk_widget_set_halign(diskSpaceLabel, GTK_ALIGN_END);
	gtk_grid_attach(GTK_GRID(grid), diskSpaceLabel, 0, 2, 1, 1);
	gtk_widget_set_halign(stats.labelDiskSpaceOccupied, GTK_ALIGN_START);
	gtk_grid_attach(GTK_GRID(grid), stats.labelDiskSpaceOccupied, 1, 2, 1, 1);

	GtkWidget *remotePackageDatabase = gtk_label_new("Remote package database");
	gtk_widget_set_halign(remotePackageDatabase, GTK_ALIGN_START);
	gtk_label_set_markup(GTK_LABEL(remotePackageDatabase), "<b><span size=\"larger\">Remote package database</span></b>");
	gtk_grid_attach(GTK_GRID(grid), remotePackageDatabase, 0, 3, 2, 1);

	GtkWidget *paLabel = gtk_label_new("Packages available:");
	gtk_widget_set_halign(paLabel, GTK_ALIGN_END);
	gtk_grid_attach(GTK_GRID(grid), paLabel, 0, 4, 1, 1);
	gtk_widget_set_halign(stats.labelPackagesAvailable, GTK_ALIGN_START);
	gtk_grid_attach(GTK_GRID(grid), stats.labelPackagesAvailable, 1, 4, 1, 1);

	// Ensure all labels in the first column have the same width
	gtk_widget_set_size_request(installedPackagesLabel, 150, -1);
	gtk_widget_set_size_request(diskSpaceLabel, 150, -1);
	gtk_widget_set_size_request(paLabel, 150, -1);

	// Make the grid expand to fill available space
	gtk_widget_set_hexpand(grid, TRUE);
	gtk_widget_set_vexpand(grid, TRUE);

	// Add some padding around the grid
	GtkWidget *padding_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	gtk_widget_set_margin_start(padding_box, 10);
	gtk_widget_set_margin_end(padding_box, 10);
	gtk_widget_set_margin_top(padding_box, 10);
	gtk_widget_set_margin_bottom(padding_box, 10);

	gtk_box_append(GTK_BOX(padding_box), grid);
	gtk_box_append(GTK_BOX(parent), padding_box);

	refresh_stats();
}

static void
create_detail_box(GtkWidget *parent)
{
	GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
	GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
	GtkWidget *iconBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
	GtkWidget *rightBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
	GtkWidget *licenseBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
	GtkWidget *typeBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);

	// buttons
	GtkWidget *buttonBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);

	// create install button and wire it up
	detail.installButton = gtk_button_new_with_mnemonic("_Install Application");
	g_signal_connect(G_OBJECT(detail.installButton), "clicked",
	                 G_CALLBACK(install_button_clicked),
	                 (gpointer) parent);
	gtk_box_append(GTK_BOX(buttonBox), detail.installButton);
	gtk_widget_set_hexpand(detail.installButton, TRUE);

	// set up placeholders for detail view
	detail.label = gtk_label_new("");
	detail.labelVersion = gtk_label_new("");
	detail.labelName = gtk_label_new("");
	detail.labelLicense = gtk_label_new("");
	detail.labelType = gtk_label_new("");
	detail.image = gtk_image_new_from_icon_name("dialog-information");
	gtk_image_set_pixel_size(GTK_IMAGE(detail.image), 48); 

	// setup  left label area
	gtk_box_set_homogeneous(GTK_BOX(iconBox), FALSE);
	gtk_box_append(GTK_BOX(iconBox), detail.image);
	gtk_box_append(GTK_BOX(iconBox), detail.labelName);
	gtk_box_append(GTK_BOX(iconBox), detail.labelVersion);

	
	// setup right side
	gtk_box_set_homogeneous(GTK_BOX(rightBox), FALSE);
	gtk_label_set_justify(GTK_LABEL(detail.label), GTK_JUSTIFY_LEFT);
	gtk_label_set_xalign(GTK_LABEL(detail.label), 0.0);  // Left align the label
	gtk_box_append(GTK_BOX(rightBox), detail.label);

	// setup license
	gtk_box_set_homogeneous(GTK_BOX(licenseBox), FALSE);
	GtkWidget *licenseLabel = gtk_label_new("License: ");
	gtk_label_set_xalign(GTK_LABEL(licenseLabel), 0.0);  // Left align the label
	gtk_box_append(GTK_BOX(licenseBox), licenseLabel);
	gtk_label_set_xalign(GTK_LABEL(detail.labelLicense), 0.0);  // Left align the label
	gtk_box_append(GTK_BOX(licenseBox), detail.labelLicense);
	gtk_box_append(GTK_BOX(rightBox), licenseBox);

	// setup type
	gtk_box_set_homogeneous(GTK_BOX(typeBox), FALSE);
	GtkWidget *typeLabel = gtk_label_new("Type: ");
	gtk_label_set_xalign(GTK_LABEL(typeLabel), 0.0);  // Left align the label
	gtk_box_append(GTK_BOX(typeBox), typeLabel);
	gtk_label_set_xalign(GTK_LABEL(detail.labelType), 0.0);  // Left align the label
	gtk_box_append(GTK_BOX(typeBox), detail.labelType);
	gtk_box_append(GTK_BOX(rightBox), typeBox);


	// set up outer box
	gtk_widget_set_margin_start(hbox, 10);
	gtk_widget_set_margin_end(hbox, 10);
	gtk_widget_set_margin_top(hbox, 10);
	gtk_widget_set_margin_bottom(hbox, 10);

	gtk_box_append(GTK_BOX(hbox), iconBox);
	gtk_box_append(GTK_BOX(hbox), rightBox);
	gtk_widget_set_hexpand(iconBox, TRUE);
	gtk_widget_set_hexpand(rightBox, TRUE);

	// put our hbox for detail info into the vbox
	gtk_box_append(GTK_BOX(vbox), hbox);
	gtk_box_append(GTK_BOX(vbox), buttonBox);

	// add to the parent container.
	gtk_box_append(GTK_BOX(parent), vbox);
}

static GtkWidget *
create_header_bar(GtkWidget *window, GtkWidget *search)
{
	GtkWidget *header_bar = gtk_header_bar_new();
	GtkWidget *icon = gtk_image_new_from_file(ICONFILE);
	gtk_image_set_pixel_size(GTK_IMAGE(icon), 24);
	gtk_header_bar_pack_start(GTK_HEADER_BAR(header_bar), icon);

	GtkWidget *quit_btn = gtk_button_new_with_label("Quit");
	g_signal_connect_swapped(quit_btn, "clicked", G_CALLBACK(gtk_window_destroy), G_OBJECT(window));
	gtk_header_bar_pack_start(GTK_HEADER_BAR(header_bar), quit_btn);

	GtkWidget *edit_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_widget_add_css_class(edit_box, "linked");

	GtkWidget *cut_btn = gtk_button_new_with_label("Cut");
	g_signal_connect_swapped(cut_btn, "clicked", G_CALLBACK(cut_clicked), G_OBJECT(search));
	gtk_box_append(GTK_BOX(edit_box), cut_btn);

	GtkWidget *copy_btn = gtk_button_new_with_label("Copy");
	g_signal_connect_swapped(copy_btn, "clicked", G_CALLBACK(copy_clicked), G_OBJECT(search));
	gtk_box_append(GTK_BOX(edit_box), copy_btn);

	GtkWidget *paste_btn = gtk_button_new_with_label("Paste");
	g_signal_connect_swapped(paste_btn, "clicked", G_CALLBACK(paste_clicked), G_OBJECT(search));
	gtk_box_append(GTK_BOX(edit_box), paste_btn);

	gtk_header_bar_pack_end(GTK_HEADER_BAR(header_bar), edit_box);

	return header_bar;
}

static void
update_button_clicked(GtkButton *button, GtkWindow *parent)
{
	int resultCode = mport_upgrade(mport);
	if (resultCode != MPORT_OK)
		msgbox(parent, mport_err_string());

	refresh_stats();
}


mportIndexEntry **
lookupIndex(mportInstance *mport, const char *packageName)
{
	mportIndexEntry **indexEntries;

	if (mport_index_lookup_pkgname(mport, packageName, &indexEntries) != MPORT_OK)
	{
		g_autofree gchar *message = g_strdup_printf(
			"Error looking up package name %s: %d %s\n",
			packageName, mport_err_code(), mport_err_string());
		msgbox_modal(GTK_WINDOW(window), "Error", "_Close", message);
		g_application_quit(G_APPLICATION(g_application_get_default()));
		return (NULL);
	}

	return (indexEntries);
}

static void
button_clicked(GtkButton *button, GtkWindow *parent)
{

	do_search();
}

static void
do_search(void)
{
	const gchar *query;

	query = gtk_editable_get_text(GTK_EDITABLE(search));

	GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(tree));
	if (model != NULL) {
		gtk_tree_store_clear(GTK_TREE_STORE(model));
		search_remote_index(GTK_TREE_STORE(model), query);
	}
}

static void
reset_search_button_clicked(GtkButton *button, GtkWindow *parent)
{

	gtk_editable_set_text(GTK_EDITABLE(search), "");
	do_search();
}

static void
install_button_clicked(GtkButton *button, GtkWidget *parent)
{
	int resultCode = 0;

	const gchar *c = gtk_label_get_text(GTK_LABEL(detail.labelName));
	if (c == NULL)
	{
		msgbox(GTK_WINDOW(window), "mport package name not defined.");
		return;
	}
	resultCode = install(mport, c);
	g_print("Install returned %d\n", resultCode);

	/* reload search data after install */
	reload_installed();
	reload_updates();
	refresh_stats();
	do_search();
}

static void
installed_delete_button_clicked(GtkButton *button, GtkWidget *parent)
{
	int result = 0;

	if (selectedInstalled[0] != '\0') {
		result = delete(selectedInstalled);
		g_print("Delete %s returned %d", selectedInstalled, result);

		/* reload search data after delete */
		reload_installed();
		reload_updates();
		refresh_stats();
		do_search();
	}
}

static void
reload_updates(void) {
	GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(updateTree));
	if (model != NULL) {
		gtk_tree_store_clear(GTK_TREE_STORE(model));
		populate_update_packages(GTK_TREE_STORE(model));
	}
}

static void
reload_installed(void) {
	g_print("Starting reload_installed\n");
	GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(installedTree));
	
	if (model == NULL) {
        g_critical("Failed to get tree model for installed packages");
        return;
    }

	GtkTreeStore *store = GTK_TREE_STORE(model);
    if (store == NULL) {
        g_critical("Tree model is not a GtkTreeStore");
        return;
    }

    gtk_tree_store_clear(store);
    g_print("Cleared installed packages tree\n");

    populate_installed_packages(store);
    g_print("Repopulated installed packages tree\n");

    g_print("Finished reload_installed\n");
}

static int
install(mportInstance *mport, const char *packageName)
{
	mportIndexEntry **indexEntry;
	mportIndexEntry **indexEntryHead;
	mportIndexEntry **i2;
	int resultCode = MPORT_OK;
	int item;

	indexEntry = lookupIndex(mport, packageName);
	indexEntryHead = indexEntry;
	if (indexEntry == NULL || *indexEntry == NULL)
	{
		g_warning("Package %s not found in the index.", packageName);
		g_autofree gchar *message = g_strdup_printf(
			"Package %s not found in the index.",
			packageName);
		msgbox_modal(GTK_WINDOW(window), "Error", "_Close", message);
		return MPORT_ERR_WARN;
	}

	if (indexEntry[1] != NULL)
	{
		GtkWidget *dialog;
		GtkWidget *content_area;
		GtkWidget *combo_box;
		gint result;
		int choice = 0;

		dialog = gtk_dialog_new_with_buttons("Select Package",
		                                     GTK_WINDOW(window),
		                                     GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
		                                     "_OK", GTK_RESPONSE_ACCEPT,
		                                     "_Cancel", GTK_RESPONSE_CANCEL,
		                                     NULL);
		set_window_app_icon(GTK_WINDOW(dialog));

		content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
		combo_box = gtk_combo_box_text_new();

		i2 = indexEntry;
		item = 0;
		while (*i2 != NULL)
		{
			char *package_info;
			if (asprintf(&package_info, "%s-%s", (*i2)->pkgname, (*i2)->version) != -1) {
				gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo_box), package_info);
				free(package_info);
			}
			item++;
			i2++;
		}

		gtk_combo_box_set_active(GTK_COMBO_BOX(combo_box), 0);
		gtk_box_append(GTK_BOX(content_area), combo_box);

		result = run_dialog_sync(GTK_DIALOG(dialog));
		if (result == GTK_RESPONSE_ACCEPT)
		{
			choice = gtk_combo_box_get_active(GTK_COMBO_BOX(combo_box));
		}
		else
		{
			gtk_window_destroy(GTK_WINDOW(dialog));
			mport_index_entry_free_vec(indexEntryHead);
			return MPORT_ERR_WARN;
		}

		gtk_window_destroy(GTK_WINDOW(dialog));

		// Set indexEntry to the chosen package
		indexEntry += choice;
	}

	// Perform the actual installation
	g_print("Starting installation of package: %s version: %s\n", (*indexEntry)->pkgname, (*indexEntry)->version);
 
	g_print("Calling install_depends...\n");
	resultCode = install_depends(mport, (*indexEntry)->pkgname, (*indexEntry)->version, MPORT_EXPLICIT);
	g_print("install_depends returned with code: %d\n", resultCode);

	if (resultCode != MPORT_OK)
	{
		g_warning("Failed to install package %s: %s", packageName, mport_err_string());
     
		g_autofree gchar *message = g_strdup_printf(
			"Failed to install package %s: %s",
			packageName, mport_err_string());
		msgbox_modal(GTK_WINDOW(window), "Error", "_Close", message);
	}
	else
	{
		g_print("Successfully installed package %s\n", packageName);
		g_autofree gchar *message = g_strdup_printf(
			"Successfully installed package %s",
			packageName);
		msgbox_modal(GTK_WINDOW(window), "Information", "_OK", message);
	}

	g_print("Freeing index entries...\n");
	mport_index_entry_free_vec(indexEntryHead);

	g_print("Finished installation process for package: %s\n", packageName);

	return (resultCode);
}

static int
install_depends(mportInstance *mport, const char *packageName, const char *version, mportAutomatic automatic) 
{
        mportPackageMeta **packs;
        mportDependsEntry **depends;

        if (packageName == NULL || version == NULL)
                return (MPORT_ERR_FATAL);

        if (mport_index_depends_list(mport, packageName, version, &depends) != MPORT_OK) {
                msgbox(GTK_WINDOW(window), mport_err_string());
                return mport_err_code();
        }

        if (mport_pkgmeta_search_master(mport, &packs, "pkg=%Q", packageName) != MPORT_OK) {
                msgbox(GTK_WINDOW(window), mport_err_string());
                mport_index_depends_free_vec(depends);
                return mport_err_code();
        }

        if (packs == NULL && depends == NULL) {
			g_print("No existing package or dependencies, installing directly...\n");
     
                /* Package is not installed and there are no dependencies */
                if (mport_install(mport, packageName, version, NULL, automatic) != MPORT_OK) {
                        msgbox(GTK_WINDOW(window), mport_err_string());
                        return mport_err_code();
                }
        } else if (packs == NULL) {
			g_print("Installing dependencies...\n");
                /* Package is not installed */
                mportDependsEntry **dependsHead = depends;
                while (*depends != NULL) {
					g_print("Installing dependency: %s version: %s\n", (*depends)->d_pkgname, (*depends)->d_version);

                        install_depends(mport, (*depends)->d_pkgname, (*depends)->d_version, MPORT_AUTOMATIC);
                        depends++;
                }
				g_print("Installing main package...\n");
                if (mport_install(mport, packageName, version, NULL, MPORT_EXPLICIT) != MPORT_OK) {
					g_warning("Main package installation failed: %s", mport_err_string());
					mport_index_depends_free_vec(dependsHead);
                        msgbox(GTK_WINDOW(window), mport_err_string());
                        return mport_err_code();
                }
                mport_index_depends_free_vec(dependsHead);
        } else {
                /* already installed */
				g_print("Package already installed\n");
                mport_pkgmeta_vec_free(packs);
                mport_index_depends_free_vec(depends);
        }

		g_print("Exiting install_depends for package: %s\n", packageName);
        return (MPORT_OK);
}

static int
delete(const char *packageName)
{
    const char *where = "LOWER(pkg)=LOWER(%Q)";
	mportPackageMeta **packs = NULL;
	int result = MPORT_OK;

	if (packageName == NULL) {
		g_print("package name not defined\n");
		return MPORT_ERR_FATAL;
	}

    if (mport_pkgmeta_search_master(mport, &packs, where, packageName) != MPORT_OK) {
		g_print("package lookup failure %s: %s\n", packageName, mport_err_string());
        msgbox(GTK_WINDOW(window), mport_err_string());
        return MPORT_ERR_FATAL;
    }

	if (packs == NULL || *packs == NULL) {
		g_autofree gchar *message = g_strdup_printf(
			"No packages installed matching '%s'",
			packageName);
		g_print("%s\n", message);
		msgbox_modal(GTK_WINDOW(window), "Error", "_Close", message);
		
		return MPORT_ERR_WARN;
	}

	for (mportPackageMeta **pack = packs; *pack != NULL; pack++) {
		(*pack)->action = MPORT_ACTION_DELETE;
		if (mport_delete_primative(mport, *pack, 0) != MPORT_OK) {
			g_print("could not delete package %s: %s\n", packageName, mport_err_string());
            msgbox(GTK_WINDOW(window), mport_err_string());
			result = MPORT_ERR_FATAL;
			break;
        }
    }

    mport_pkgmeta_vec_free(packs);
    return result;
}


/**
 * Create a non-modal dialog with one OK button.
 */
static void
msgbox(GtkWindow *parent, const char *msg)
{
	GtkWidget *dialog, *label, *image, *hbox;

	dialog = gtk_dialog_new_with_buttons("Information",
										 parent, GTK_DIALOG_DESTROY_WITH_PARENT,
										 "_OK", GTK_RESPONSE_ACCEPT,
										 NULL);
	set_window_app_icon(GTK_WINDOW(dialog));

	label = gtk_label_new(msg);
	image = create_app_icon_image(48);

	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
	gtk_widget_set_margin_start(hbox, 10);
	gtk_widget_set_margin_end(hbox, 10);
	gtk_widget_set_margin_top(hbox, 10);
	gtk_widget_set_margin_bottom(hbox, 10);
	gtk_box_append(GTK_BOX(hbox), image);
	gtk_box_append(GTK_BOX(hbox), label);

	gtk_box_append(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), hbox);

	g_signal_connect(G_OBJECT(dialog), "response",
	                 G_CALLBACK(gtk_window_destroy), NULL);
	gtk_window_present(GTK_WINDOW(dialog));
}

static void
msgbox_modal(GtkWindow *parent, const char *title, const char *button_label, const char *msg)
{
	GtkWidget *dialog, *label, *image, *hbox;

	dialog = gtk_dialog_new_with_buttons(title,
	                                     parent,
	                                     GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
	                                     button_label, GTK_RESPONSE_ACCEPT,
	                                     NULL);
	set_window_app_icon(GTK_WINDOW(dialog));

	label = gtk_label_new(msg);
	image = create_app_icon_image(48);
	gtk_label_set_wrap(GTK_LABEL(label), TRUE);
	gtk_label_set_xalign(GTK_LABEL(label), 0.0f);

	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
	gtk_widget_set_margin_start(hbox, 10);
	gtk_widget_set_margin_end(hbox, 10);
	gtk_widget_set_margin_top(hbox, 10);
	gtk_widget_set_margin_bottom(hbox, 10);
	gtk_box_append(GTK_BOX(hbox), image);
	gtk_box_append(GTK_BOX(hbox), label);

	gtk_box_append(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), hbox);

	run_dialog_sync(GTK_DIALOG(dialog));
	gtk_window_destroy(GTK_WINDOW(dialog));
}

static void
on_dialog_response(GtkDialog *dialog, gint response_id, gpointer user_data)
{
    gint *response_ptr = user_data;
    *response_ptr = response_id;
    g_main_loop_quit((GMainLoop *)g_object_get_data(G_OBJECT(dialog), "loop"));
}

static gint
run_dialog_sync(GtkDialog *dialog)
{
    GMainLoop *loop = g_main_loop_new(NULL, FALSE);
    gint response = GTK_RESPONSE_NONE;

    g_object_set_data(G_OBJECT(dialog), "loop", loop);
    g_signal_connect(dialog, "response", G_CALLBACK(on_dialog_response), &response);

    gtk_window_present(GTK_WINDOW(dialog));
    g_main_loop_run(loop);
    g_main_loop_unref(loop);

    return response;
}

/**
 * Create a modal dialog with Yes/No question
 */
static bool
msgbox_bool(GtkWindow *parent, const char *msg)
{
	GtkWidget *dialog, *label, *image, *hbox;

	dialog = gtk_dialog_new_with_buttons("Question", parent,
	                                     GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,                     
										 "_Yes", GTK_RESPONSE_ACCEPT,
										 "_No", GTK_RESPONSE_REJECT,
										 NULL);
	set_window_app_icon(GTK_WINDOW(dialog));

	label = gtk_label_new(msg);
	image = create_app_icon_image(48);

	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
	gtk_widget_set_margin_start(hbox, 10);
	gtk_widget_set_margin_end(hbox, 10);
	gtk_widget_set_margin_top(hbox, 10);
	gtk_widget_set_margin_bottom(hbox, 10);
	gtk_box_append(GTK_BOX(hbox), image);
	gtk_box_append(GTK_BOX(hbox), label);

	gtk_box_append(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), hbox);

	gint result = run_dialog_sync(GTK_DIALOG(dialog));
	gtk_window_destroy(GTK_WINDOW(dialog));
	return (result == GTK_RESPONSE_ACCEPT);
}

static int
msgbox_select(GtkWindow *parent, const char *msg, mportIndexEntry **choices, int def)
{
	GtkWidget *dialog, *label, *image, *hbox, *content, *combo;
	int count = 0;

	if (choices == NULL)
		return -1;

	while (choices[count] != NULL)
		count++;

	if (count == 0)
		return -1;

	dialog = gtk_dialog_new_with_buttons("Select Package", parent,
	    GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT, NULL, NULL);
	gtk_dialog_add_button(GTK_DIALOG(dialog), "_Cancel", GTK_RESPONSE_CANCEL);
	gtk_dialog_add_button(GTK_DIALOG(dialog), "_OK", GTK_RESPONSE_ACCEPT);
	set_window_app_icon(GTK_WINDOW(dialog));

	content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
	image = create_app_icon_image(48);
	label = gtk_label_new(msg);
	gtk_label_set_xalign(GTK_LABEL(label), 0.0f);
	gtk_label_set_wrap(GTK_LABEL(label), TRUE);

	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
	gtk_widget_set_margin_start(hbox, 10);
	gtk_widget_set_margin_end(hbox, 10);
	gtk_widget_set_margin_top(hbox, 10);
	gtk_widget_set_margin_bottom(hbox, 10);
	gtk_box_append(GTK_BOX(hbox), image);
	gtk_box_append(GTK_BOX(hbox), label);
	gtk_box_append(GTK_BOX(content), hbox);

	combo = gtk_combo_box_text_new();
	for (int i = 0; i < count; i++) {
		char *item = NULL;
		if (asprintf(&item, "%s-%s - %s",
		    choices[i]->pkgname == NULL ? "" : choices[i]->pkgname,
		    choices[i]->version == NULL ? "" : choices[i]->version,
		    choices[i]->comment == NULL ? "" : choices[i]->comment) == -1) {
			item = NULL;
		}
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo), item == NULL ? "" : item);
		free(item);
	}

	if (def >= 0 && def < count)
		gtk_combo_box_set_active(GTK_COMBO_BOX(combo), def);
	else
		gtk_combo_box_set_active(GTK_COMBO_BOX(combo), 0);

	gtk_widget_set_margin_start(combo, 10);
	gtk_widget_set_margin_end(combo, 10);
	gtk_widget_set_margin_bottom(combo, 10);
	gtk_box_append(GTK_BOX(content), combo);

	gint result = run_dialog_sync(GTK_DIALOG(dialog));
	if (result == GTK_RESPONSE_ACCEPT) {
		int selected = gtk_combo_box_get_active(GTK_COMBO_BOX(combo));
		gtk_window_destroy(GTK_WINDOW(dialog));
		return selected;
	}

	gtk_window_destroy(GTK_WINDOW(dialog));
	return -1;
}


/* Copy the selected text to the clipboard and remove it from the buffer. */
static void
cut_clicked(GtkButton *cut, GtkEditable *textview)
{
	gtk_widget_activate_action(GTK_WIDGET(textview), "clipboard.cut", NULL);
}

/* Copy the selected text to the clipboard. */
static void
copy_clicked(GtkButton *copy, GtkEditable *textview)
{
	gtk_widget_activate_action(GTK_WIDGET(textview), "clipboard.copy", NULL);
}

/* Insert the text from the clipboard into the text buffer. */
static void
paste_clicked(GtkButton *paste, GtkEditable *textview)
{
	gtk_widget_activate_action(GTK_WIDGET(textview), "clipboard.paste", NULL);
}

static void
setup_tree(void) {
	GtkTreeStore *store;

	GtkTreeViewColumn *column;
	GtkCellRenderer *renderer;

	store = gtk_tree_store_new(N_COLUMNS,
	                           G_TYPE_STRING,
	                           G_TYPE_STRING);

	populate_remote_index(store);

	tree = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
	g_object_unref(G_OBJECT(store));
	gtk_tree_view_set_activate_on_single_click(GTK_TREE_VIEW(tree), false);

	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes("Title",
	                                                  renderer,
	                                                  "text", TITLE_COLUMN,
	                                                  NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column);

	renderer = gtk_cell_renderer_text_new();

	column = gtk_tree_view_column_new_with_attributes("Version", renderer,
	                                                  "text", VERSION_COLUMN,
	                                                  NULL);

	gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column);

	g_signal_connect(G_OBJECT(tree), "cursor-changed",
	                 G_CALLBACK(available_cursor_changed_handler),
	                 (gpointer)NULL);
}

static void
create_installed_tree(void)
{
	GtkTreeStore *store;
	GtkTreeViewColumn *column;
	GtkCellRenderer *renderer;

    store = gtk_tree_store_new(INST_N_COLUMNS,
		G_TYPE_STRING,  // INST_TITLE_COLUMN
		G_TYPE_STRING,  // INST_VERSION_COLUMN
		G_TYPE_STRING,  // INST_FLATSIZE_COLUMN
		G_TYPE_STRING); // INST_LOCK_STATUS_COLUMN

	populate_installed_packages(store);

	installedTree = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
	g_object_unref(G_OBJECT(store));
	gtk_tree_view_set_activate_on_single_click(GTK_TREE_VIEW(installedTree), false);

	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes("Title",
	                                                  renderer,
	                                                  "text", INST_TITLE_COLUMN,
	                                                  NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(installedTree), column);

	renderer = gtk_cell_renderer_text_new();

	column = gtk_tree_view_column_new_with_attributes("Version", renderer,
	                                                  "text", INST_VERSION_COLUMN,
	                                                  NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(installedTree), column);

	renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes("Size", renderer,
                                                      "text", INST_FLATSIZE_COLUMN,
                                                      NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(installedTree), column);

	renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes("Lock Status", renderer,
                                                      "text", INST_LOCK_STATUS_COLUMN,
                                                      NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(installedTree), column);

	g_signal_connect(G_OBJECT(installedTree), "cursor-changed",
	                 G_CALLBACK(installed_cursor_changed_handler),
	                 (gpointer)NULL);
}

/*
 * Create the tree for the updates tab
 */
static void
create_update_tree(void)
{
	GtkTreeStore *store;

	GtkTreeViewColumn *column;
	GtkCellRenderer *renderer;

	store = gtk_tree_store_new(UPD_N_COLUMNS,
	                           G_TYPE_STRING,
	                           G_TYPE_STRING,
	                           G_TYPE_STRING,
	                           G_TYPE_STRING);

	populate_update_packages(store);

	updateTree = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
	/* The view now holds a reference.  We can get rid of our own reference */
	g_object_unref(G_OBJECT(store));

	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes("Title",
	                                                  renderer,
	                                                  "text", UPD_TITLE_COLUMN,
	                                                  NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(updateTree), column);

	/* VERSION */
	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes("Installed Version", renderer,
	                                                  "text", UPD_VERSION_COLUMN,
	                                                  NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(updateTree), column);

	/* OS RELEASE */
	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes("OS", renderer,
	                                                  "text", UPD_OS_RELEASE_COLUMN,
	                                                  NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(updateTree), column);

	/* NEW VERSION */
	renderer = gtk_cell_renderer_text_new();
	// g_object_set (G_OBJECT (renderer), "foreground", "red", NULL);
	column = gtk_tree_view_column_new_with_attributes("New Version", renderer,
	                                                  "text", UPD_NEW_VERSION_COLUMN,
	                                                  NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(updateTree), column);
}


static void
search_remote_index(GtkTreeStore *store, const char *query)
{
	mportIndexEntry **packs;
	mportIndexEntry **packsHead;

	if (query == NULL || query[0] == '\0') {
		populate_remote_index(store);
		return;
	}

	if (mport_index_search_term(mport, &packs, (char *)query) != MPORT_OK) {
		warnx("%s", mport_err_string());
		mport_instance_free(mport);
		exit(1);
	}
	packsHead = packs;

	if (tree != NULL) {
		g_object_ref(store);
		gtk_tree_view_set_model(GTK_TREE_VIEW(tree), NULL);
	}

	while (*packs != NULL) {
		GtkTreeIter iter;
		gtk_tree_store_append(store, &iter, NULL);
		gtk_tree_store_set(store, &iter,
		                   TITLE_COLUMN, (*packs)->pkgname,
		                   VERSION_COLUMN, (*packs)->version,
		                   -1);
		packs++;
	}

	if (tree != NULL) {
		gtk_tree_view_set_model(GTK_TREE_VIEW(tree), GTK_TREE_MODEL(store));
		g_object_unref(store);
	}

	mport_index_entry_free_vec(packsHead);
}

static void
populate_remote_index(GtkTreeStore *store)
{
	mportIndexEntry **indexEntries;
	mportIndexEntry **indexEntriesHead;
	mportPackageMeta **packs;
	mportPackageMeta **packsHead;

	if (mport_index_list(mport, &indexEntries) != MPORT_OK) {
		msgbox(GTK_WINDOW(window), mport_err_string());
		mport_instance_free(mport);
		exit(1);
	}
	indexEntriesHead = indexEntries;

	if (mport_pkgmeta_list(mport, &packs) != MPORT_OK) {
		msgbox(GTK_WINDOW(window), mport_err_string());
		mport_index_entry_free_vec(indexEntriesHead);
		mport_instance_free(mport);
		exit(1);
	}
	packsHead = packs;

	/* Build a hash set of "name\tversion" for installed packages so the
	 * outer loop can do O(1) lookups instead of an O(m) linear scan. */
	GHashTable *installed_set = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);
	if (packs != NULL) {
		while (*packs != NULL) {
			if ((*packs)->name != NULL && (*packs)->version != NULL) {
				gchar *key = g_strdup_printf("%s\t%s", (*packs)->name, (*packs)->version);
				g_hash_table_insert(installed_set, key, GINT_TO_POINTER(1));
			}
			packs++;
		}
	}

	/* Detach model from view during bulk insert to suppress per-row redraws. */
	if (tree != NULL) {
		g_object_ref(store);
		gtk_tree_view_set_model(GTK_TREE_VIEW(tree), NULL);
	}

	while (*indexEntries != NULL) {
		GtkTreeIter iter;
		const char *pkgname = (*indexEntries)->pkgname;
		const char *version = (*indexEntries)->version;
		gboolean installed = FALSE;

		if (pkgname != NULL && version != NULL) {
			gchar *key = g_strdup_printf("%s\t%s", pkgname, version);
			installed = g_hash_table_lookup(installed_set, key) != NULL;
			g_free(key);
		}

#ifdef DEBUG
		g_print("Working on %s\n", (*indexEntries)->pkgname);
#endif

		if (!installed && pkgname != NULL && version != NULL) {
			gtk_tree_store_append(store, &iter, NULL);
			gtk_tree_store_set(store, &iter,
			                   TITLE_COLUMN, pkgname,
			                   VERSION_COLUMN, version,
			                   -1);
		}

		indexEntries++;
	}

	if (tree != NULL) {
		gtk_tree_view_set_model(GTK_TREE_VIEW(tree), GTK_TREE_MODEL(store));
		g_object_unref(store);
	}

	g_hash_table_destroy(installed_set);
	mport_index_entry_free_vec(indexEntriesHead);
	mport_pkgmeta_vec_free(packsHead);
}

static void
populate_installed_packages(GtkTreeStore *store)
{
	mportPackageMeta **packs;
	char flatsize_str[10];

    g_print("Starting populate_installed_packages\n");

	if (store == NULL) {
		g_critical("Store is NULL in populate_installed_packages");
		return;
	}

	if (mport_pkgmeta_list(mport, &packs) != MPORT_OK) {
		g_warning("Failed to get package list: %s", mport_err_string());
		msgbox(GTK_WINDOW(window), mport_err_string());
		return;
	}

	if (packs == NULL) {
        g_warning("No packages found");
        return;
    }

	if (installedTree != NULL) {
		g_object_ref(store);
		gtk_tree_view_set_model(GTK_TREE_VIEW(installedTree), NULL);
	}

	for (mportPackageMeta **pack = packs; pack && *pack; pack++) {
		GtkTreeIter iter;

        g_print("Processing package: %s\n", (*pack)->name ? (*pack)->name : "NULL");

		if ((*pack)->flatsize < 0) {
			g_warning("Invalid flatsize for package: %s", (*pack)->name);
            snprintf(flatsize_str, sizeof(flatsize_str), "Unknown");
        } else if (humanize_number(flatsize_str, sizeof(flatsize_str), (*pack)->flatsize, "B",
			HN_AUTOSCALE, HN_DECIMAL | HN_IEC_PREFIXES) < 0) {
			snprintf(flatsize_str, sizeof(flatsize_str), "%lld B", (long long)(*pack)->flatsize);
		}

		gtk_tree_store_append(store, &iter, NULL);

		const char *lock_status = (mport_lock_islocked(*pack) == MPORT_LOCKED) ? "Locked" : "Unlocked";

		g_print("Adding installed package: %s\n", (*pack)->name ? (*pack)->name : "NULL");


		gtk_tree_store_set(store, &iter,
		                   INST_TITLE_COLUMN, (*pack)->name ? (*pack)->name : "",
		                   INST_VERSION_COLUMN, (*pack)->version ? (*pack)->version : "",
						   INST_FLATSIZE_COLUMN, flatsize_str,
						   INST_LOCK_STATUS_COLUMN, lock_status,
		                   -1);
	}

	if (installedTree != NULL) {
		gtk_tree_view_set_model(GTK_TREE_VIEW(installedTree), GTK_TREE_MODEL(store));
		g_object_unref(store);
	}

	g_print("Finished populate_installed_packages\n");

	mport_pkgmeta_vec_free(packs);
}

static void
populate_update_packages(GtkTreeStore *store)
{
	mportPackageMeta **packs = NULL;

	g_debug("Starting populate_update_packages");

	if (mport_pkgmeta_list(mport, &packs) != MPORT_OK) {
		g_warning("Failed to get package list: %s", mport_err_string());
		msgbox(GTK_WINDOW(window), mport_err_string());
		return;
	}

	char *os_release = mport_get_osrelease(mport);
	if (os_release == NULL) {
		g_warning("Failed to get OS release");
        msgbox(GTK_WINDOW(window), "Failed to get OS release");
		mport_pkgmeta_vec_free(packs);
        return;
    }

	g_debug("OS Release: %s", os_release);

	if (updateTree != NULL) {
		g_object_ref(store);
		gtk_tree_view_set_model(GTK_TREE_VIEW(updateTree), NULL);
	}

	for (mportPackageMeta **pack = packs; pack && *pack; pack++) {
		mportIndexEntry **indexEntries = NULL;

        g_debug("Processing package: %s", (*pack)->name);

		if (mport_index_lookup_pkgname(mport, (*pack)->name, &indexEntries) != MPORT_OK)
		{
            g_warning("Failed to lookup package: %s", (*pack)->name);
            continue;
        }

        if (indexEntries == NULL || *indexEntries == NULL)
		{
            g_debug("No index entries for package: %s", (*pack)->name);
            continue;
        }

		for (mportIndexEntry **entry = indexEntries; entry && *entry; entry++) {
			if ((*entry)->version == NULL)
			{
                g_warning("Null version for package: %s", (*pack)->name);
                continue;
            }

			int version_cmp = mport_version_cmp((*pack)->version, (*entry)->version);
			int os_cmp = ((*pack)->os_release != NULL) ? mport_version_cmp((*pack)->os_release, os_release) : 0;

			g_debug("Package: %s, Installed: %s, Available: %s, OS: %s",
					(*pack)->name, (*pack)->version, (*entry)->version, (*pack)->os_release);

			if (version_cmp < 0 || os_cmp < 0)
			{

				GtkTreeIter iter;
				gtk_tree_store_append(store, &iter, NULL);
				gtk_tree_store_set(store, &iter,
								   UPD_TITLE_COLUMN, (*pack)->name,
								   UPD_VERSION_COLUMN, (*pack)->version,
								   UPD_OS_RELEASE_COLUMN, (*pack)->os_release ? (*pack)->os_release : "",
								   UPD_NEW_VERSION_COLUMN, (*entry)->version,
								   -1);

				g_debug("Added update for package: %s", (*pack)->name);
			}
		}
		mport_index_entry_free_vec(indexEntries);
	}

	if (updateTree != NULL) {
		gtk_tree_view_set_model(GTK_TREE_VIEW(updateTree), GTK_TREE_MODEL(store));
		g_object_unref(store);
	}

	g_debug("Finished populate_update_packages");

	free(os_release);
    mport_pkgmeta_vec_free(packs);
}

static mportPackageMeta** 
lookup_for_lock(mportInstance *mport, const char *packageName)
{
	mportPackageMeta **packs = NULL;

	if (packageName == NULL) {
		warnx("%s", "Specify package name");
		return (NULL);
	}

	if (mport_pkgmeta_search_master(mport, &packs, "pkg=%Q", packageName) != MPORT_OK) {
		warnx("%s", mport_err_string());
		return (NULL);
	}

	if (packs == NULL) {
		warnx("Package name not found, %s", packageName);
	}

	return (packs);
}

static int
lock(mportInstance *mport, const char *packageName)
{
	mportPackageMeta **packs = lookup_for_lock(mport, packageName);

	if (packs != NULL) {
		mport_lock_lock(mport, (*packs));
		mport_pkgmeta_vec_free(packs);
		return (MPORT_OK);
	}

	return (MPORT_ERR_FATAL);
}

static int
unlock(mportInstance *mport, const char *packageName)
{
	mportPackageMeta **packs = lookup_for_lock(mport, packageName);

	if (packs != NULL) {
		mport_lock_unlock(mport, (*packs));
		mport_pkgmeta_vec_free(packs);
		return (MPORT_OK);
	}

	return (MPORT_ERR_FATAL);
}
