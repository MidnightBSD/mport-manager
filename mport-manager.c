/*-
Copyright (C) 2008, 2016 Lucas Holt. All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:
1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY AUTHOR AND CONTRIBUTORS ``AS IS'' AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED.  IN NO EVENT SHALL AUTHOR OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
SUCH DAMAGE.
*/

#include <gtk/gtk.h>
#include <gdk/gdk.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <err.h>
#include <string.h>

#include <dispatch/dispatch.h>
#include <mport.h>

#define NAME "MidnightBSD Package Manager"
#define ICONFILE "/usr/local/share/mport/icon.png"
#define MPORT_LOCAL_PKG_PATH "/var/db/mport/downloads"

GtkWidget *search; /* textboxes */
GtkWidget *tree;
GtkWidget *installedTree;
GtkWidget *updateTree;

mportInstance *mport;

dispatch_group_t grp;
dispatch_queue_t q;

static void button_clicked (GtkButton *button, GtkWindow *parent);
static void update_button_clicked (GtkButton *button, GtkWindow *parent);
static void msgbox(GtkWindow *parent, char * msg);
static void cut_clicked (GtkButton *, GtkEntry *);
static void copy_clicked (GtkButton *, GtkEntry *);
static void paste_clicked (GtkButton *, GtkEntry *);
static void setup_tree(void);
static void create_installed_tree(void);
static void create_update_tree(void);
static void populate_installed_packages(GtkTreeStore *);
static void populate_update_packages(GtkTreeStore *); 
static void populate_remote_index(GtkTreeStore *store);
static void search_remote_index(GtkTreeStore *store, gchar *query);
static void gtk_box_pack_start_defaults(GtkBox *, GtkWidget *);
static void loadIndex(mportInstance *);
static mportIndexEntry ** lookupIndex(mportInstance *, const char *);
static int update(mportInstance *, const char *);
static int upgrade(mportInstance *);
static int updateDown(mportInstance *, mportPackageMeta *);
static int indexCheck(mportInstance *, mportPackageMeta *);
static void create_menus(GtkWidget *window, GtkWidget *parent, GtkWidget *search);

int 
main( int argc, char *argv[] )
{
	GtkWidget *window, *vbox, *authbox, *vauthbox;
	GtkWidget *submit;
	GtkWidget *scrolled_win;
	GtkWidget *stackSwitcher;
	GtkWidget *stack;
	GdkPixbuf *icon;

	dispatch_queue_t mainq = dispatch_get_main_queue();
	grp = dispatch_group_create();
	q = dispatch_queue_create("print", NULL);
	mport = mport_instance_new();

	if (mport_instance_init(mport, NULL) != MPORT_OK) {
		warnx("%s", mport_err_string());
		exit(1);
	}

	gtk_init( &argc, &argv );

	// init window
	window = gtk_window_new( GTK_WINDOW_TOPLEVEL );
	gtk_window_set_title( GTK_WINDOW (window), NAME );
	gtk_container_set_border_width( GTK_CONTAINER (window), 10 );
	gtk_widget_set_size_request( window, 800, 600 );

	icon = gdk_pixbuf_new_from_file(ICONFILE, NULL);
	if (icon) {
		gtk_window_set_icon(GTK_WINDOW(window), icon);
	}

	// setup destroy signal
	g_signal_connect (G_OBJECT (window), "destroy",
                  G_CALLBACK (gtk_main_quit), NULL);

	// create search button
	submit = gtk_button_new_with_mnemonic("_Search");
	g_signal_connect (G_OBJECT (submit), "clicked",
                    G_CALLBACK (button_clicked),
                    (gpointer) window);

	setup_tree();
	create_installed_tree();
	create_update_tree();

	stackSwitcher = gtk_stack_switcher_new();
	stack = gtk_stack_new();

	search = gtk_entry_new();

	authbox = gtk_box_new( GTK_ORIENTATION_HORIZONTAL, 5 );
	gtk_box_pack_start_defaults( GTK_BOX (authbox), search );
	gtk_box_pack_start( GTK_BOX (authbox), submit, FALSE, TRUE, 5 );

	vauthbox = gtk_box_new( GTK_ORIENTATION_VERTICAL, 5 );
	gtk_box_pack_start_defaults( GTK_BOX (vauthbox), authbox );

	/* Setup the final box for layout in the window */
	vbox = gtk_box_new( GTK_ORIENTATION_VERTICAL, 5 );
	gtk_box_pack_start( GTK_BOX (vbox), vauthbox, FALSE, TRUE, 5 );

 	scrolled_win = gtk_scrolled_window_new (NULL, NULL);
	gtk_container_add (GTK_CONTAINER (scrolled_win), tree);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_win),
                                  GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);


	gtk_box_pack_start( GTK_BOX (vbox), scrolled_win, TRUE, TRUE, 5 );

	// Scroll for install packages
	GtkWidget *scrolled_installed = gtk_scrolled_window_new (NULL, NULL);
	gtk_container_add (GTK_CONTAINER (scrolled_installed), installedTree);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_installed),
                                  GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);

	// Scroll for install packages
	GtkWidget *scrolled_updates = gtk_scrolled_window_new (NULL, NULL);
	gtk_container_add (GTK_CONTAINER (scrolled_updates), updateTree);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_updates),
                                  GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);
	// create update button
	GtkWidget *updateButton = gtk_button_new_with_mnemonic("_Upgrade Installed Software");
	//updateButton = 
	g_signal_connect (G_OBJECT (updateButton), "clicked",
                    G_CALLBACK (update_button_clicked),
                    (gpointer) window);
	// create update box
	GtkWidget *updateBox = gtk_box_new( GTK_ORIENTATION_VERTICAL, 5 );
	gtk_box_pack_start( GTK_BOX (updateBox), scrolled_updates, TRUE, TRUE, 5 );
	gtk_box_pack_start( GTK_BOX (updateBox), updateButton, FALSE, TRUE, 5 );

	// add all the stacks
	gtk_stack_add_titled(GTK_STACK(stack), vbox, "page-1", "Available Software");
	gtk_stack_add_titled(GTK_STACK(stack), scrolled_installed, "page-2", "Installed Software");
	gtk_stack_add_titled(GTK_STACK(stack), updateBox, "page-3", "Updates");

	gtk_stack_set_visible_child(GTK_STACK(stack), vbox);
	gtk_stack_set_visible_child_name(GTK_STACK(stack), "page-1");

	gtk_stack_switcher_set_stack(GTK_STACK_SWITCHER(stackSwitcher), GTK_STACK(stack));

	GtkWidget *stackHolder = gtk_box_new( GTK_ORIENTATION_VERTICAL, 5 );
	create_menus(window, stackHolder, search);
	gtk_box_pack_start( GTK_BOX (stackHolder), stackSwitcher, FALSE, TRUE, 0 );
	gtk_box_pack_start( GTK_BOX (stackHolder), stack, TRUE, TRUE, 0 );

	gtk_container_add( GTK_CONTAINER (window), stackHolder);
	gtk_widget_show_all( window );

	dispatch_group_wait(grp, DISPATCH_TIME_FOREVER);
	dispatch_async(mainq, ^{	
		gtk_main();
		mport_instance_free(mport); 
		exit(0);
	});

	dispatch_main();
}

static void
create_menus(GtkWidget *window, GtkWidget *parent, GtkWidget *search) {
	GtkWidget *menuBar = gtk_menu_bar_new();

	// File Menu
	GtkWidget *menuItem1 = gtk_menu_item_new_with_mnemonic ("_File");
	GtkWidget *submenu1 = gtk_menu_new();
	GtkWidget *item_quit = gtk_menu_item_new_with_label("Quit");
	g_signal_connect_swapped(item_quit, "activate", G_CALLBACK (gtk_widget_destroy), G_OBJECT(window));
	gtk_menu_shell_append (GTK_MENU_SHELL (submenu1), item_quit);
	gtk_menu_item_set_submenu (GTK_MENU_ITEM (menuItem1), submenu1);
	gtk_menu_shell_append (GTK_MENU_SHELL (menuBar), menuItem1);

	// Edit Menu
	GtkWidget *menuItem2 = gtk_menu_item_new_with_mnemonic ("_Edit");
	GtkWidget *submenu2 = gtk_menu_new();
	GtkWidget *cut = gtk_menu_item_new_with_label("Cut");
	GtkWidget *copy = gtk_menu_item_new_with_label("Copy");
	GtkWidget *paste = gtk_menu_item_new_with_label("Paste");

	g_signal_connect (G_OBJECT (cut), "activate",
                    G_CALLBACK (cut_clicked),
                    (gpointer) search);
	g_signal_connect (G_OBJECT (copy), "activate",
                    G_CALLBACK (copy_clicked),
                    (gpointer) search);
	g_signal_connect (G_OBJECT (paste), "activate",
                    G_CALLBACK (paste_clicked),
                    (gpointer) search);

	gtk_menu_shell_append (GTK_MENU_SHELL (submenu2), cut);
	gtk_menu_shell_append (GTK_MENU_SHELL (submenu2), copy);
	gtk_menu_shell_append (GTK_MENU_SHELL (submenu2), paste);
	gtk_menu_item_set_submenu (GTK_MENU_ITEM (menuItem2), submenu2);
	gtk_menu_shell_append (GTK_MENU_SHELL (menuBar), menuItem2);

	

	// connect menubar to parent
	gtk_container_add (GTK_CONTAINER (parent), menuBar);
}

/*
 * Compatibility method for gtk2 to gtk3 conversion
 */
static void 
gtk_box_pack_start_defaults(GtkBox *box, GtkWidget *widget)  {
	gtk_box_pack_start(box, widget, TRUE, TRUE, 0);
}

static void
update_button_clicked(GtkButton *button, GtkWindow *parent) 
{
	dispatch_group_async(grp, q, ^{
		loadIndex(mport);
		int resultCode = upgrade(mport);
		// TODO: handle errors
	});
}

void
loadIndex(mportInstance *mport) {
	int result = mport_index_load(mport);
	if (result == MPORT_ERR_WARN)
		warnx("%s", mport_err_string());
	else if (result != MPORT_OK)
		errx(4, "Unable to load index %s", mport_err_string());
}

int
indexCheck(mportInstance *mport, mportPackageMeta *pack) {
	mportIndexEntry **indexEntries;
	int ret = 0;

	if (mport_index_lookup_pkgname(mport, pack->name, &indexEntries) != MPORT_OK) {
		fprintf(stderr, "Error Looking up package name %s: %d %s\n", pack->name,  mport_err_code(), mport_err_string());
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


int
upgrade(mportInstance *mport) {
	mportPackageMeta **packs;
	int total = 0;
	int updated = 0;

	if (mport_pkgmeta_list(mport, &packs) != MPORT_OK) {
		warnx("%s", mport_err_string());
		return (1);
	}

	if (packs == NULL) {
		warnx("No packages installed.\n");
		return (1);
	}

	while (*packs != NULL) {
		if (indexCheck(mport, *packs)) {
			updated += updateDown(mport, *packs);
		}
		packs++;
		total++;
	}
	mport_pkgmeta_vec_free(packs);
	printf("Packages updated: %d\nTotal: %d\n", updated, total);
	return (0);
}

int
updateDown(mportInstance *mport, mportPackageMeta *pack) {
	mportPackageMeta **depends;
	int ret = 0;

	fprintf(stderr, "Entering %s\n", pack->name);

	if (mport_pkgmeta_get_downdepends(mport, pack, &depends) == MPORT_OK) {
		if (depends == NULL) {
			if (indexCheck(mport, pack)) {
				fprintf(stderr, "Updating %s\n", pack->name); 
				if (update(mport, pack->name) !=0) {
					fprintf(stderr, "Error updating %s\n", pack->name);
					ret = 0;
				} else
					ret = 1;
			} else
				ret = 0;
		} else {
			while (*depends != NULL) {
				ret += updateDown(mport, (*depends));
				if (indexCheck(mport, *depends)) {
					fprintf(stderr, "Updating depends %s\n", (*depends)->name);
					if (update(mport, (*depends)->name) != 0) {
						fprintf(stderr, "Error updating %s\n", (*depends)->name);
					} else
						ret++;
				}
				depends++;
			}
			if (indexCheck(mport, pack)) {
				fprintf(stderr, "Updating port called %s\n", pack->name);
				if (update(mport, pack->name) != 0) {
					fprintf(stderr, "Error updating %s\n", pack->name);
				} else
					ret++;
			}
		}
		mport_pkgmeta_vec_free(depends);
	}

	return (ret);
}

int
update(mportInstance *mport, const char *packageName) {
	mportIndexEntry **indexEntry;
	char *path;

	indexEntry = lookupIndex(mport, packageName);
	if (indexEntry == NULL || *indexEntry == NULL)
		return (1);

	asprintf(&path, "%s/%s", MPORT_LOCAL_PKG_PATH, (*indexEntry)->bundlefile);

	if (!mport_file_exists(path)) {
        	if (mport_fetch_bundle(mport, (*indexEntry)->bundlefile) != MPORT_OK) {
			fprintf(stderr, "%s\n", mport_err_string());
			free(path);
			return mport_err_code();
		}
	}

	if (!mport_verify_hash(path, (*indexEntry)->hash)) {
		if (unlink(path) == 0)  /* remove file so we can try again */
			fprintf(stderr, "Package fails hash verification and was removed. Please try again.\n");
		else
			fprintf(stderr, "Package fails hash verification Please delete it manually at %s\n", path);
		free(path);
		return (1);
	}

	if (mport_update_primative(mport, path) != MPORT_OK) {
		fprintf(stderr, "%s\n", mport_err_string());
		free(path);
		return mport_err_code();
	}
	free(path);
	mport_index_entry_free_vec(indexEntry);

	return (0);
}

mportIndexEntry **
lookupIndex(mportInstance *mport, const char *packageName) {
	mportIndexEntry **indexEntries;

	if (mport_index_lookup_pkgname(mport, packageName, &indexEntries) != MPORT_OK) {
		fprintf(stderr, "Error looking up package name %s: %d %s\n",
			packageName,  mport_err_code(), mport_err_string());
		errx(mport_err_code(), "%s", mport_err_string());
	}

	return (indexEntries);
}

static void 
button_clicked(GtkButton *button, GtkWindow *parent)
{
    const gchar *query;

    query = gtk_entry_get_text( GTK_ENTRY (search) );

	GtkTreeModel * model = gtk_tree_view_get_model(GTK_TREE_VIEW(tree));
	if (model != NULL) {
		gtk_tree_store_clear(GTK_TREE_STORE(model));
		search_remote_index(GTK_TREE_STORE(model), query);
	}
}

static void 
msgbox( GtkWindow * parent, char * msg )
{
    GtkWidget *dialog, *label, *image, *hbox;

    /* Create a non-modal dialog with one OK button. */
    dialog = gtk_dialog_new_with_buttons ("Information", parent,
                                        GTK_DIALOG_DESTROY_WITH_PARENT,
					  					 ("_OK"),
                                      	GTK_RESPONSE_ACCEPT,
                                      	NULL);

    label = gtk_label_new (msg);
    image = gtk_image_new_from_icon_name("dialog-information", GTK_ICON_SIZE_DIALOG);

    hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_container_set_border_width (GTK_CONTAINER (hbox), 10);
    gtk_box_pack_start_defaults (GTK_BOX (hbox), image);
    gtk_box_pack_start_defaults (GTK_BOX (hbox), label);

    gtk_box_pack_start_defaults (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))), hbox);
    gtk_widget_show_all (dialog);

    /* Call gtk_widget_destroy() when the dialog emits the response signal. */
    g_signal_connect (G_OBJECT (dialog), "response",
                    G_CALLBACK (gtk_widget_destroy), NULL);
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

enum
{
	TITLE_COLUMN,
	VERSION_COLUMN,
	INSTALLED_COLUMN,
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

static void
setup_tree(void)
{
   GtkTreeStore *store;
   
   GtkTreeViewColumn *column;
   GtkCellRenderer *renderer;

   store = gtk_tree_store_new (N_COLUMNS,
                               G_TYPE_STRING,
                               G_TYPE_STRING,
                               G_TYPE_BOOLEAN);

   populate_remote_index(store);

   tree = gtk_tree_view_new_with_model (GTK_TREE_MODEL (store));
   g_object_unref (G_OBJECT (store));

   renderer = gtk_cell_renderer_text_new ();
   column = gtk_tree_view_column_new_with_attributes ("Title",
                                                      renderer,
                                                      "text", TITLE_COLUMN,
                                                      NULL);
   gtk_tree_view_append_column (GTK_TREE_VIEW (tree), column);

   renderer = gtk_cell_renderer_text_new ();
   g_object_set (G_OBJECT (renderer),
                 "foreground", "red",
                 NULL);

   column = gtk_tree_view_column_new_with_attributes ("Version", renderer,
                                                      "text", VERSION_COLUMN,
                                                      NULL);

   gtk_tree_view_append_column (GTK_TREE_VIEW (tree), column);

   renderer = gtk_cell_renderer_toggle_new ();
   column = gtk_tree_view_column_new_with_attributes ("Installed",
                                                      renderer,
                                                      "active", INSTALLED_COLUMN,
                                                      NULL);
   gtk_tree_view_append_column (GTK_TREE_VIEW (tree), column);
}

static void
create_installed_tree(void)
{
   GtkTreeStore *store;
   
   GtkTreeViewColumn *column;
   GtkCellRenderer *renderer;

   store = gtk_tree_store_new (INST_N_COLUMNS,
                               G_TYPE_STRING,
                               G_TYPE_STRING);

   populate_installed_packages(store);

   installedTree = gtk_tree_view_new_with_model (GTK_TREE_MODEL (store));
   /* The view now holds a reference.  We can get rid of our own
    * reference */
   g_object_unref (G_OBJECT (store));

   renderer = gtk_cell_renderer_text_new ();
   column = gtk_tree_view_column_new_with_attributes ("Title",
                                                      renderer,
                                                      "text", INST_TITLE_COLUMN,
                                                      NULL);
   gtk_tree_view_append_column (GTK_TREE_VIEW (installedTree), column);

   renderer = gtk_cell_renderer_text_new ();
   g_object_set (G_OBJECT (renderer),
                 "foreground", "red",
                 NULL);

   column = gtk_tree_view_column_new_with_attributes ("Version", renderer,
                                                      "text", INST_VERSION_COLUMN,
                                                      NULL);

   gtk_tree_view_append_column (GTK_TREE_VIEW (installedTree), column);
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

   store = gtk_tree_store_new(	UPD_N_COLUMNS,
                          	G_TYPE_STRING,
                          	G_TYPE_STRING,
			  	G_TYPE_STRING,
				G_TYPE_STRING);

   populate_update_packages(store);

   updateTree = gtk_tree_view_new_with_model (GTK_TREE_MODEL (store));
   /* The view now holds a reference.  We can get rid of our own reference */
   g_object_unref (G_OBJECT (store));

   renderer = gtk_cell_renderer_text_new ();
   column = gtk_tree_view_column_new_with_attributes ("Title",
                                                      renderer,
                                                      "text", UPD_TITLE_COLUMN,
                                                      NULL);
   gtk_tree_view_append_column (GTK_TREE_VIEW (updateTree), column);

   
   /* VERSION */
   column = gtk_tree_view_column_new_with_attributes ("Installed Version", renderer,
                                                      "text", UPD_VERSION_COLUMN,
                                                      NULL);
   gtk_tree_view_append_column (GTK_TREE_VIEW (updateTree), column);

   /* OS RELEASE */
   column = gtk_tree_view_column_new_with_attributes ("OS", renderer,
                                                      "text", UPD_OS_RELEASE_COLUMN,
                                                      NULL);
   gtk_tree_view_append_column (GTK_TREE_VIEW (updateTree), column);

   /* NEW VERSION */
	renderer = gtk_cell_renderer_text_new ();
   g_object_set (G_OBJECT (renderer),
                 "foreground", "red",
                 NULL);
   column = gtk_tree_view_column_new_with_attributes ("New Version", renderer,
                                                      "text", UPD_NEW_VERSION_COLUMN,
                                                      NULL);
   gtk_tree_view_append_column (GTK_TREE_VIEW (updateTree), column);
}


static void 
search_remote_index(GtkTreeStore *store, char *query) 
{
        mportIndexEntry **packs;

	if (query == NULL || query[0] == '\0') {
		populate_remote_index(store);
		return;
	}

	if (mport_index_load(mport) != MPORT_OK)
		errx(4, "Unable to load updates index");

        if (mport_index_search(mport, &packs, "pkg glob %Q or comment glob %Q", query, query) != MPORT_OK) {
                warnx("%s", mport_err_string());
                mport_instance_free(mport);
                exit(1);
        }

	while (*packs != NULL) {
	//	dispatch_group_async(grp, q, ^{

			GtkTreeIter   iter;
			gtk_tree_store_append(store, &iter, NULL);
			gtk_tree_store_set(store, &iter,
        	            TITLE_COLUMN, (*packs)->pkgname,
        	            VERSION_COLUMN, (*packs)->version,
        	            INSTALLED_COLUMN, TRUE,
       		            -1);
		
	//	});
		packs++;
	}

//	dispatch_group_wait(grp, DISPATCH_TIME_FOREVER);
}

static void 
populate_remote_index(GtkTreeStore *store) 
{
        mportIndexEntry **packs;

	if (mport_index_load(mport) != MPORT_OK)
		errx(4, "Unable to load updates index");

        if (mport_index_list(mport, &packs) != MPORT_OK) {
                warnx("%s", mport_err_string());
                mport_instance_free(mport);
                exit(1);
        }

	while (*packs != NULL) {
		dispatch_group_async(grp, q, ^{
			GtkTreeIter   iter;
			gtk_tree_store_append (store, &iter, NULL);
			gtk_tree_store_set (store, &iter,
        	            TITLE_COLUMN, (*packs)->pkgname,
        	            VERSION_COLUMN, (*packs)->version,
        	            INSTALLED_COLUMN, TRUE,
       		            -1);
		
		});
		packs++;
	}

	dispatch_group_wait(grp, DISPATCH_TIME_FOREVER);
}

static void 
populate_installed_packages(GtkTreeStore *store) 
{
	mportPackageMeta **packs;

        if (mport_pkgmeta_list(mport, &packs) != MPORT_OK) {
                warnx("%s", mport_err_string());
                mport_instance_free(mport);
                exit(1);
        }

	while (*packs != NULL) {
		dispatch_group_async(grp, q, ^{
			GtkTreeIter   iter;
			gtk_tree_store_append (store, &iter, NULL);
			gtk_tree_store_set (store, &iter,
        	            INST_TITLE_COLUMN, (*packs)->name,
        	            INST_VERSION_COLUMN, (*packs)->version,
       		            -1);
			
		});
		packs++;
	}

	dispatch_group_wait(grp, DISPATCH_TIME_FOREVER);
}

static void 
populate_update_packages(GtkTreeStore *store) 
{
	mportPackageMeta **packs;
	
	if (mport_pkgmeta_list(mport, &packs) != MPORT_OK) {
		warnx("%s", mport_err_string());
		mport_instance_free(mport);
		exit(1);
	}

	char *os_release = mport_get_osrelease();

	while (*packs != NULL) {
		dispatch_group_async(grp, q, ^{
			mportIndexEntry **indexEntries;
			GtkTreeIter   iter;

			if (mport_index_lookup_pkgname(mport, (*packs)->name, &indexEntries) != MPORT_OK) {
				return;
			}

			/* package is not currently available, skip */
			if (indexEntries == NULL || *indexEntries == NULL) {
				return;
			}

			mportIndexEntry **iestart = indexEntries;		
			while (*indexEntries != NULL) {
				if (((*indexEntries)->version != NULL && mport_version_cmp((*packs)->version, (*indexEntries)->version) < 0) 
					|| ((*packs)->version != NULL && mport_version_cmp((*packs)->os_release, os_release) < 0)) {
				
					gtk_tree_store_append(store, &iter, NULL);  /* Acquire an iterator */
					gtk_tree_store_set(store, &iter,
									   UPD_TITLE_COLUMN, (*packs)->name,
									   UPD_VERSION_COLUMN, (*packs)->version,
									   UPD_OS_RELEASE_COLUMN, (*packs)->os_release,
									   UPD_NEW_VERSION_COLUMN, (*indexEntries)->version,
									   -1);
				}
				indexEntries++;
			}
				
			mport_index_entry_free_vec(iestart);
			indexEntries = NULL;
		});		
			
		packs++;
	}

	dispatch_group_wait(grp, DISPATCH_TIME_FOREVER);
}
