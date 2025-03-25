/*-
 * Copyright (C) 2008, 2016 Lucas Holt. All rights reserved.
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
#include <gdk/gdk.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <err.h>
#include <string.h>
#include <locale.h>
#include <X11/Xlib.h>

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

/*
 * Installed Software Tab's selected name
 */
char selectedInstalled[256] = {'\0'};

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
static void do_search(void);
static void button_clicked(GtkButton *button, GtkWindow *parent);
static void reset_search_button_clicked(GtkButton *button, GtkWindow *parent);
static void update_button_clicked(GtkButton *button, GtkWindow *parent);
static void install_button_clicked(GtkButton *button, GtkWidget *parent);
static void installed_delete_button_clicked(GtkButton *button, GtkWidget *parent);
static void msgbox(GtkWindow *parent, const char * msg);
static bool msgbox_bool(GtkWindow *parent, const char *msg);
static void cut_clicked (GtkButton *, GtkEntry *);
static void copy_clicked (GtkButton *, GtkEntry *);
static void paste_clicked (GtkButton *, GtkEntry *);
static void setup_tree(void);
static void create_installed_tree(void);
static void create_update_tree(void);
static void populate_installed_packages(GtkTreeStore *);
static void populate_update_packages(GtkTreeStore *); 
static void populate_remote_index(GtkTreeStore *store);
static void search_remote_index(GtkTreeStore *store, const char *query);
static void create_menus(GtkWidget *window, GtkWidget *parent, GtkWidget *search);
static void create_detail_box(GtkWidget *parent);
static void available_row_click_handler(GtkTreeView *treeView, GtkTreePath *path, GtkTreeViewColumn *column, gpointer data);
static void installed_tree_available_row_click_handler(GtkTreeView *treeView, GtkTreePath *path, GtkTreeViewColumn *column, gpointer data);
void reset_progress_bar(void);

// mport stuff
static int delete(const char *);
static int indexCheck(mportInstance *, mportPackageMeta *);
static int install(mportInstance *, const char *);
static mportIndexEntry ** lookupIndex(mportInstance *, const char *);

/* Callbacks */
void mport_gtk_msg_cb(const char *msg);
int mport_gtk_confirm_cb(const char *msg, const char *yes, const char *no, int def);
void mport_gtk_progress_init_cb(const char *title);
void mport_gtk_progress_step_cb(int current, int total, const char *msg);
void mport_gtk_progress_free_cb(void);

int
main(int argc, char *argv[])
{
	GtkWidget *vbox, *authbox, *vauthbox;
	GtkWidget *submit; // search button
	GtkWidget *resetSearchButton;
	GtkWidget *scrolled_win;
	GtkWidget *stackSwitcher;
	GtkWidget *stack;
	GdkPixbuf *icon;

	// setup env for mport
	if (setenv("POSIXLY_CORRECT", "1", 1) == -1)
		err(EXIT_FAILURE, "setenv() failed");
	setlocale(LC_ALL, "");

	mport = mport_instance_new();
	if (mport == NULL) {
		errx(EXIT_FAILURE, "mport_instance_new() failed");
	}

	XInitThreads();
	gtk_init(&argc, &argv);

	// init window
	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(window), NAME);
	gtk_container_set_border_width(GTK_CONTAINER(window), 10);
	gtk_widget_set_size_request(window, 800, 600);

	icon = gdk_pixbuf_new_from_file(ICONFILE, NULL);
	if (icon) {
		// Scale the icon to a more appropriate size (e.g., 32x32 pixels)
		GdkPixbuf *scaled_icon = gdk_pixbuf_scale_simple(icon, 32, 32, GDK_INTERP_BILINEAR);
		if (scaled_icon) {
			GList *icon_list = NULL;
			icon_list = g_list_append(icon_list, scaled_icon);
			gtk_window_set_icon_list(GTK_WINDOW(window), icon_list);
			g_list_free(icon_list);
			g_object_unref(scaled_icon);
		}
		g_object_unref(icon);
	}

	// setup destroy signal
	g_signal_connect(G_OBJECT(window), "destroy",
	                 G_CALLBACK(gtk_main_quit), NULL);

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
	mport->msg_cb = &mport_gtk_msg_cb;
	mport->confirm_cb = &mport_gtk_confirm_cb;
	mport->progress_init_cb = &mport_gtk_progress_init_cb;
	mport->progress_step_cb = &mport_gtk_progress_step_cb;
	mport->progress_free_cb = &mport_gtk_progress_free_cb;

	if (mport_index_load(mport) != MPORT_OK) {
		mport_instance_free(mport);
		GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(window),
		GTK_DIALOG_DESTROY_WITH_PARENT,
		GTK_MESSAGE_ERROR,
		GTK_BUTTONS_CLOSE,
		"Unable to load updates index");
		gtk_dialog_run(GTK_DIALOG(dialog));
		gtk_widget_destroy(dialog);
		gtk_main_quit();
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
	gtk_box_pack_start(GTK_BOX(authbox), search, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(authbox), submit, FALSE, TRUE, 3);
	gtk_box_pack_start(GTK_BOX(authbox), resetSearchButton, FALSE, TRUE, 3);

	vauthbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
	gtk_box_pack_start(GTK_BOX(vauthbox), authbox, TRUE, TRUE, 0);

	/* Setup the final box for layout in the window */
	vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
	gtk_box_pack_start(GTK_BOX(vbox), vauthbox, FALSE, TRUE, 5);

	scrolled_win = gtk_scrolled_window_new(NULL, NULL);
	gtk_container_add(GTK_CONTAINER(scrolled_win), tree);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_win),
	                               GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);

	gtk_box_pack_start(GTK_BOX(vbox), scrolled_win, TRUE, TRUE, 5);
	create_detail_box(vbox);


	// Scroll for install packages
	GtkWidget *scrolled_installed = gtk_scrolled_window_new(NULL, NULL);
	gtk_container_add(GTK_CONTAINER(scrolled_installed), installedTree);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_installed),
	                               GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);
	// create remove button on Installed Software
	GtkWidget *removeInstalledAppButton = gtk_button_new_with_mnemonic("_Remove Application");
	g_signal_connect(G_OBJECT(removeInstalledAppButton), "clicked",
	                 G_CALLBACK(installed_delete_button_clicked),
	                 (gpointer) window);
	// create update box
	GtkWidget *installedBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
	gtk_box_pack_start(GTK_BOX(installedBox), scrolled_installed, TRUE, TRUE, 5);
	gtk_box_pack_start(GTK_BOX(installedBox), removeInstalledAppButton, FALSE, TRUE, 5);


	// Scroll for updates
	GtkWidget *scrolled_updates = gtk_scrolled_window_new(NULL, NULL);
	gtk_container_add(GTK_CONTAINER(scrolled_updates), updateTree);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_updates),
	                               GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);
	// create update button
	GtkWidget *updateButton = gtk_button_new_with_mnemonic("_Upgrade Installed Software");
	//updateButton = 
	g_signal_connect(G_OBJECT(updateButton), "clicked",
	                 G_CALLBACK(update_button_clicked),
	                 (gpointer) window);
	// create update box
	GtkWidget *updateBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
	gtk_box_pack_start(GTK_BOX(updateBox), scrolled_updates, TRUE, TRUE, 5);
	gtk_box_pack_start(GTK_BOX(updateBox), updateButton, FALSE, TRUE, 5);

	// add all the stacks
	gtk_stack_add_titled(GTK_STACK(stack), vbox, "page-1", "Available Software");
	gtk_stack_add_titled(GTK_STACK(stack), installedBox, "page-2", "Installed Software");
	gtk_stack_add_titled(GTK_STACK(stack), updateBox, "page-3", "Updates");

	gtk_stack_set_visible_child(GTK_STACK(stack), vbox);
	gtk_stack_set_visible_child_name(GTK_STACK(stack), "page-1");

	gtk_stack_switcher_set_stack(GTK_STACK_SWITCHER(stackSwitcher), GTK_STACK(stack));

	GtkWidget *stackHolder = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
	create_menus(window, stackHolder, search);
	gtk_box_pack_start(GTK_BOX(stackHolder), stackSwitcher, FALSE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(stackHolder), stack, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(stackHolder), progressBar, FALSE, TRUE, 8);

	gtk_container_add(GTK_CONTAINER(window), stackHolder);
	gtk_widget_show_all(window);

	gtk_main();
	mport_instance_free(mport);
	exit(0);
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

	if (current > total)
		current = total;

	percent = (gdouble) current / (gdouble) total;

	gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progressBar), percent);
	while (gtk_events_pending())
		gtk_main_iteration();
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
            if (mport_index_lookup_pkgname(mport, name, &indexEntries) != MPORT_OK) {
                g_warning("Error looking up package name %s: %s", name, mport_err_string());
            } else if (indexEntries != NULL) {
                for (mportIndexEntry **entry = indexEntries; *entry != NULL; entry++) {
                    if ((*entry)->version != NULL && mport_version_cmp(version, (*entry)->version) == 0) {
                        g_strlcpy(selectedInstalled, name, sizeof(selectedInstalled));
                        break;
                    }
                }
                mport_index_entry_free_vec(indexEntries);
            }
        }

        g_free(name);
        g_free(version);
	}
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
			asprintf(&msg, "Error looking up package name %s: %s\n", name, mport_err_string());
			(mport->msg_cb)(msg);
			free(msg);
			return;
		}

		if (indexEntries != NULL) {
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
			mport_index_entry_free_vec(indexEntries);
		}

#if defined(DEBUG)
		g_print ("The row containing the name '%s' has been double-clicked.\n", name);
#endif

		g_free(name);
		g_free(version);
	}
}

static void
create_detail_box(GtkWidget *parent)
{
	GtkWidget *headerBar = gtk_header_bar_new();

	GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
	GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
	GtkWidget *iconBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
	GtkWidget *rightBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
	GtkWidget *licenseBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);

	// buttons
	GtkWidget *buttonBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);

	// create install button and wire it up
	detail.installButton = gtk_button_new_with_mnemonic("_Install Application");
	g_signal_connect(G_OBJECT(detail.installButton), "clicked",
	                 G_CALLBACK(install_button_clicked),
	                 (gpointer) parent);
					 gtk_box_pack_start(GTK_BOX(buttonBox), detail.installButton, TRUE, TRUE, 0);

	// setup header bar
	gtk_header_bar_set_title(GTK_HEADER_BAR(headerBar), "Detail");
	gtk_box_pack_start(GTK_BOX(vbox), headerBar, TRUE, TRUE, 0);

	// set up placeholders for detail view
	detail.label = gtk_label_new("");
	detail.labelVersion = gtk_label_new("");
	detail.labelName = gtk_label_new("");
	detail.labelLicense = gtk_label_new("");
	detail.labelType = gtk_label_new("");
	detail.image = gtk_image_new_from_icon_name("dialog-information", GTK_ICON_SIZE_DIALOG);
	gtk_image_set_pixel_size(GTK_IMAGE(detail.image), 48); 

	// setup  left label area
	gtk_box_set_homogeneous(GTK_BOX(iconBox), FALSE);
	gtk_box_pack_start(GTK_BOX(iconBox), detail.image, FALSE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(iconBox), detail.labelName, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(iconBox), detail.labelVersion, FALSE, TRUE, 0);

	// setup license
	gtk_box_set_homogeneous(GTK_BOX(licenseBox), FALSE);
	GtkWidget *licenseLabel = gtk_label_new("License: ");
	gtk_box_pack_start(GTK_BOX(licenseBox), licenseLabel, FALSE, FALSE, 2);
	gtk_box_pack_start(GTK_BOX(licenseBox), detail.labelLicense, FALSE, TRUE, 0);

	// setup type
	gtk_box_set_homogeneous(GTK_BOX(licenseBox), FALSE);
	GtkWidget *typeLabel = gtk_label_new("Type: ");
	gtk_box_pack_start(GTK_BOX(licenseBox), typeLabel, FALSE, FALSE, 2);
	gtk_box_pack_start(GTK_BOX(licenseBox), detail.labelType, FALSE, TRUE, 0);

	// setup right side
	gtk_box_set_homogeneous(GTK_BOX(rightBox), FALSE);
	gtk_label_set_justify(GTK_LABEL(detail.label), GTK_JUSTIFY_LEFT);
	gtk_box_pack_start(GTK_BOX(rightBox), detail.label, FALSE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(rightBox), licenseBox, FALSE, TRUE, 0);

	// set up outer box
	gtk_container_set_border_width(GTK_CONTAINER(hbox), 10);
	gtk_box_pack_start(GTK_BOX(hbox), iconBox, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), rightBox, TRUE, TRUE, 0);

	// put our hbox for detail info into the vbox
	gtk_box_pack_start(GTK_BOX(vbox), hbox, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), buttonBox, TRUE, TRUE, 0);

	// add to the parent container.
	gtk_container_add(GTK_CONTAINER(parent), vbox);
}

static void
create_menus(GtkWidget *window, GtkWidget *parent, GtkWidget *search)
{
	GtkWidget *menuBar = gtk_menu_bar_new();

	// File Menu
	GtkWidget *menuItem1 = gtk_menu_item_new_with_mnemonic("_File");
	GtkWidget *submenu1 = gtk_menu_new();
	GtkWidget *item_quit = gtk_menu_item_new_with_label("Quit");
	g_signal_connect_swapped(item_quit, "activate", G_CALLBACK(gtk_widget_destroy), G_OBJECT(window));
	gtk_menu_shell_append(GTK_MENU_SHELL(submenu1), item_quit);
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(menuItem1), submenu1);
	gtk_menu_shell_append(GTK_MENU_SHELL(menuBar), menuItem1);

	// Edit Menu
	GtkWidget *menuItem2 = gtk_menu_item_new_with_mnemonic("_Edit");
	GtkWidget *submenu2 = gtk_menu_new();
	GtkWidget *cut = gtk_menu_item_new_with_label("Cut");
	GtkWidget *copy = gtk_menu_item_new_with_label("Copy");
	GtkWidget *paste = gtk_menu_item_new_with_label("Paste");

	g_signal_connect(G_OBJECT(cut), "activate",
	                 G_CALLBACK(cut_clicked),
	                 (gpointer) search);
	g_signal_connect(G_OBJECT(copy), "activate",
	                 G_CALLBACK(copy_clicked),
	                 (gpointer) search);
	g_signal_connect(G_OBJECT(paste), "activate",
	                 G_CALLBACK(paste_clicked),
	                 (gpointer) search);

	gtk_menu_shell_append(GTK_MENU_SHELL(submenu2), cut);
	gtk_menu_shell_append(GTK_MENU_SHELL(submenu2), copy);
	gtk_menu_shell_append(GTK_MENU_SHELL(submenu2), paste);
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(menuItem2), submenu2);
	gtk_menu_shell_append(GTK_MENU_SHELL(menuBar), menuItem2);

	// connect menubar to parent
	gtk_container_add(GTK_CONTAINER(parent), menuBar);
}

static void
update_button_clicked(GtkButton *button, GtkWindow *parent)
{
	int resultCode = mport_upgrade(mport);
	if (resultCode != MPORT_OK)
		msgbox(parent, mport_err_string());
}

int
indexCheck(mportInstance *mport, mportPackageMeta *pack)
{
	mportIndexEntry **indexEntries;
	int ret = 0;

	if (mport_index_lookup_pkgname(mport, pack->name, &indexEntries) != MPORT_OK) {
		char *msg;
		asprintf(&msg,"Error looking up package name %s: %s\n", pack->name, mport_err_string());
		(mport->msg_cb)(msg);
		free(msg);
		return (0);
	}

	if (indexEntries != NULL) {
		while (*indexEntries != NULL) {
			if ((*indexEntries)->version != NULL && mport_version_cmp(pack->version, (*indexEntries)->version) < 0) {
				ret = 1;
				break;
			}
			indexEntries++;
		}
		mport_index_entry_free_vec(indexEntries);
	}

	return (ret);
}

mportIndexEntry **
lookupIndex(mportInstance *mport, const char *packageName)
{
	mportIndexEntry **indexEntries;

	if (mport_index_lookup_pkgname(mport, packageName, &indexEntries) != MPORT_OK)
	{
		GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(window),
												   GTK_DIALOG_DESTROY_WITH_PARENT,
												   GTK_MESSAGE_ERROR,
												   GTK_BUTTONS_CLOSE,
												   "Error looking up package name %s: %d %s\n",
												   packageName, mport_err_code(), mport_err_string());
		gtk_dialog_run(GTK_DIALOG(dialog));
		gtk_widget_destroy(dialog);
		gtk_main_quit();
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

	query = gtk_entry_get_text(GTK_ENTRY(search));

	GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(tree));
	if (model != NULL) {
		gtk_tree_store_clear(GTK_TREE_STORE(model));
		search_remote_index(GTK_TREE_STORE(model), query);
	}
}

static void
reset_search_button_clicked(GtkButton *button, GtkWindow *parent)
{

	gtk_entry_set_text(GTK_ENTRY(search), "");
	do_search();
}

static void
install_button_clicked(GtkButton *button, GtkWidget *parent)
{
	int resultCode = 0;

	const gchar *c = gtk_label_get_text(GTK_LABEL(detail.labelName));
	if (c == NULL) {
		msgbox(GTK_WINDOW(window),"mport package name not defined.");
		return;
	}
	resultCode = install(mport, c);
	g_print("Install returned %d\n", resultCode);

	/* reload search data after install */
	do_search();
}

static void
installed_delete_button_clicked(GtkButton *button, GtkWidget *parent)
{
	int result = 0;

	if (selectedInstalled[0] != '\0') {
		result = delete(selectedInstalled);
		g_print("Delete returned %d", result);

		/* reload search data after delete */
		reload_installed();
		do_search();
	}
}

static void
reload_installed(void) {
	GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(installedTree));
	if (model != NULL) {
		gtk_tree_store_clear(GTK_TREE_STORE(model));
		populate_installed_packages(GTK_TREE_STORE(model));
	}
}

static int
install(mportInstance *mport, const char *packageName)
{
	mportIndexEntry **indexEntry;
	mportIndexEntry **i2;
	int resultCode = MPORT_OK;
	int item;
	int choice;

	indexEntry = lookupIndex(mport, packageName);
	if (indexEntry == NULL || *indexEntry == NULL)
	{
		GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(window),
												   GTK_DIALOG_DESTROY_WITH_PARENT,
												   GTK_MESSAGE_ERROR,
												   GTK_BUTTONS_CLOSE,
												   "Package %s not found in the index.", packageName);
		gtk_dialog_run(GTK_DIALOG(dialog));
		gtk_widget_destroy(dialog);
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
			NULL, NULL);  // No buttons yet

		gtk_dialog_add_button(GTK_DIALOG(dialog), "_OK", GTK_RESPONSE_ACCEPT);
		gtk_dialog_add_button(GTK_DIALOG(dialog), "_Cancel", GTK_RESPONSE_CANCEL);

		content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
		combo_box = gtk_combo_box_text_new();

		i2 = indexEntry;
		item = 0;
		while (*i2 != NULL)
		{
			char *package_info;
			asprintf(&package_info, "%s-%s", (*i2)->pkgname, (*i2)->version);
			gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo_box), package_info);
			free(package_info);
			item++;
			i2++;
		}

		gtk_combo_box_set_active(GTK_COMBO_BOX(combo_box), 0);
		gtk_container_add(GTK_CONTAINER(content_area), combo_box);
		gtk_widget_show_all(dialog);

		result = gtk_dialog_run(GTK_DIALOG(dialog));
		if (result == GTK_RESPONSE_ACCEPT)
		{
			choice = gtk_combo_box_get_active(GTK_COMBO_BOX(combo_box));
		}
		else
		{
			gtk_widget_destroy(dialog);
			mport_index_entry_free_vec(indexEntry);
			return MPORT_ERR_WARN;
		}

		gtk_widget_destroy(dialog);

		// Set indexEntry to the chosen package
		indexEntry += choice;
	}

	// Perform the actual installation
	resultCode = mport_install(mport, (*indexEntry)->pkgname, (*indexEntry)->version, NULL, MPORT_EXPLICT);

	if (resultCode != MPORT_OK)
	{
		GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(window),
												   GTK_DIALOG_DESTROY_WITH_PARENT,
												   GTK_MESSAGE_ERROR,
												   GTK_BUTTONS_CLOSE,
												   "Failed to install package %s: %s",
												   packageName, mport_err_string());
		gtk_dialog_run(GTK_DIALOG(dialog));
		gtk_widget_destroy(dialog);
	}
	else
	{
		GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(window),
												   GTK_DIALOG_DESTROY_WITH_PARENT,
												   GTK_MESSAGE_INFO,
												   GTK_BUTTONS_CLOSE,
												   "Successfully installed package %s", packageName);
		gtk_dialog_run(GTK_DIALOG(dialog));
		gtk_widget_destroy(dialog);
	}

	mport_index_entry_free_vec(indexEntry);

	return (resultCode);
}

static int
delete(const char *packageName)
{
    const char *where = "LOWER(pkg)=LOWER(%Q)";
	mportPackageMeta **packs = NULL;
	mportPackageMeta **packs_orig = NULL;

    if (mport_pkgmeta_search_master(mport, &packs, where, packageName) != MPORT_OK) {
        msgbox(GTK_WINDOW(window), mport_err_string());
        return MPORT_ERR_FATAL;
    }

    if (packs == NULL) {
        GtkWidget *dialog;
        gchar *message;

        message = g_strdup_printf("No packages installed matching '%s'", packageName);
        
        dialog = gtk_message_dialog_new(GTK_WINDOW(window),
                                        GTK_DIALOG_DESTROY_WITH_PARENT,
                                        GTK_MESSAGE_ERROR,
                                        GTK_BUTTONS_CLOSE,
                                        "%s", message);
        
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        g_free(message);
        
        return MPORT_ERR_WARN;
    }

	packs = packs_orig;
    while (*packs != NULL) {
		(*packs)->action = MPORT_ACTION_DELETE;
        if (mport_delete_primative(mport, *packs, mport->force) != MPORT_OK) {
            msgbox(GTK_WINDOW(window), mport_err_string());
			mport_pkgmeta_vec_free(packs_orig);
            return MPORT_ERR_FATAL;
        }
        packs++;
    }

	mport_pkgmeta_vec_free(packs_orig);
    return MPORT_OK;
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
										 NULL, NULL); // No buttons yet

	gtk_dialog_add_button(GTK_DIALOG(dialog), "_OK", GTK_RESPONSE_ACCEPT);

	label = gtk_label_new(msg);
	image = gtk_image_new_from_icon_name("dialog-information", GTK_ICON_SIZE_DIALOG);
	gtk_image_set_pixel_size(GTK_IMAGE(image), 48); 

	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
	gtk_container_set_border_width(GTK_CONTAINER(hbox), 10);
	gtk_box_pack_start(GTK_BOX(hbox), image, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), label, TRUE, TRUE, 0);

	gtk_box_pack_start(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), hbox, TRUE, TRUE, 0);
	gtk_widget_show_all(dialog);

	g_signal_connect(G_OBJECT(dialog), "response",
	                 G_CALLBACK(gtk_widget_destroy), NULL);
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
										 NULL, NULL); // No buttons yet                                 
	   
	   gtk_dialog_add_button(GTK_DIALOG(dialog), "_Yes", GTK_RESPONSE_ACCEPT);
	   gtk_dialog_add_button(GTK_DIALOG(dialog), "_No", GTK_RESPONSE_REJECT);

	label = gtk_label_new(msg);
	image = gtk_image_new_from_icon_name("dialog-information", GTK_ICON_SIZE_DIALOG);
	gtk_image_set_pixel_size(GTK_IMAGE(image), 48); 

	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
	gtk_container_set_border_width(GTK_CONTAINER(hbox), 10);
	gtk_box_pack_start(GTK_BOX(hbox), image, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), label, TRUE, TRUE, 0);

	gtk_box_pack_start(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), hbox, TRUE, TRUE, 0);

	gint result = gtk_dialog_run(GTK_DIALOG(dialog));
	switch (result) {
		case GTK_RESPONSE_ACCEPT:
			gtk_widget_destroy(dialog);
			return true;
		default:
			gtk_widget_destroy(dialog);
			return false;
	}
}


/* Copy the selected text to the clipboard and remove it from the buffer. */
static void
cut_clicked(GtkButton *cut, GtkEntry *textview)
{
	gtk_editable_cut_clipboard(GTK_EDITABLE(textview));
}

/* Copy the selected text to the clipboard. */
static void
copy_clicked(GtkButton *copy, GtkEntry *textview)
{
	gtk_editable_copy_clipboard(GTK_EDITABLE(textview));
}

/* Insert the text from the clipboard into the text buffer. */
static void
paste_clicked(GtkButton *paste, GtkEntry *textview)
{
	gtk_editable_paste_clipboard(GTK_EDITABLE(textview));
}

static void
setup_tree(void) {
	GtkTreeStore *store;

	GtkTreeViewColumn *column;
	GtkCellRenderer *renderer;

	store = gtk_tree_store_new(N_COLUMNS,
	                           G_TYPE_STRING,
	                           G_TYPE_STRING,
	                           G_TYPE_BOOLEAN);

	populate_remote_index(store);

	tree = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
	g_object_unref(G_OBJECT(store));
	gtk_tree_view_set_activate_on_single_click(GTK_TREE_VIEW(tree), true);

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

	g_signal_connect(G_OBJECT(tree), "row-activated",
	                 G_CALLBACK(available_row_click_handler),
	                 (gpointer)NULL);
}

static void
create_installed_tree(void)
{
	GtkTreeStore *store;

	GtkTreeViewColumn *column;
	GtkCellRenderer *renderer;

	store = gtk_tree_store_new(INST_N_COLUMNS,
	                           G_TYPE_STRING,
	                           G_TYPE_STRING);

	populate_installed_packages(store);

	installedTree = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
	g_object_unref(G_OBJECT(store));
	gtk_tree_view_set_activate_on_single_click(GTK_TREE_VIEW(installedTree), true);

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

	g_signal_connect(G_OBJECT(installedTree), "row-activated",
	                 G_CALLBACK(installed_tree_available_row_click_handler),
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
	column = gtk_tree_view_column_new_with_attributes("Installed Version", renderer,
	                                                  "text", UPD_VERSION_COLUMN,
	                                                  NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(updateTree), column);

	/* OS RELEASE */
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

	if (query == NULL || query[0] == '\0') {
		populate_remote_index(store);
		return;
	}

	if (mport_index_search(mport, &packs, "pkg glob %Q or comment glob %Q", query, query) != MPORT_OK) {
		warnx("%s", mport_err_string());
		mport_instance_free(mport);
		exit(1);
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
}

static void
populate_remote_index(GtkTreeStore *store)
{
	mportIndexEntry **indexEntries;
	mportPackageMeta **packs;

	if (mport_index_list(mport, &indexEntries) != MPORT_OK) {
		msgbox(GTK_WINDOW(window), mport_err_string());
		mport_instance_free(mport);
		exit(1);
	}

	if (mport_pkgmeta_list(mport, &packs) != MPORT_OK) {
		msgbox(GTK_WINDOW(window), mport_err_string());
		mport_instance_free(mport);
		exit(1);
	}

	while (*indexEntries != NULL) {
		GtkTreeIter iter;
		gboolean installed = FALSE;

#ifdef DEBUG
		g_print("Working on %s\n", (*indexEntries)->pkgname);
#endif

		mportPackageMeta **p2 = packs;
		while (*p2 != NULL) {
			if ((strcmp((*indexEntries)->pkgname, (*p2)->name) == 0) &&
			    (strcmp((*indexEntries)->version, (*p2)->version) == 0)) {
				installed = TRUE;
				break;
			}

			p2++;
		}

		if (!installed) {
			gtk_tree_store_append(store, &iter, NULL);

			gtk_tree_store_set(store, &iter,
		                   TITLE_COLUMN, (*indexEntries)->pkgname,
		                   VERSION_COLUMN, (*indexEntries)->version,
		                   -1);
		}

		indexEntries++;
	}
}

static void
populate_installed_packages(GtkTreeStore *store)
{
	mportPackageMeta **packs;

	if (mport_pkgmeta_list(mport, &packs) != MPORT_OK) {
		msgbox(GTK_WINDOW(window), mport_err_string());
		mport_instance_free(mport);
		exit(1);
	}

	while (*packs != NULL) {
		GtkTreeIter iter;
		gtk_tree_store_append(store, &iter, NULL);
		gtk_tree_store_set(store, &iter,
		                   INST_TITLE_COLUMN, (*packs)->name,
		                   INST_VERSION_COLUMN, (*packs)->version,
		                   -1);

		packs++;
	}
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

	g_debug("Finished populate_update_packages");

	free(os_release);
    mport_pkgmeta_vec_free(packs);
}
