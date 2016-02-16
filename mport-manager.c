/* $Id: gtkjj.c,v 1.4 2008/09/27 02:06:02 laffer1 Exp $ */
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

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include <mport.h>

#define NAME "MidnightBSD Package Manager"

GtkWidget *search, *pass; /* textboxes */
GtkWidget *tree;
GtkTextBuffer *buffer;

static void button_clicked (GtkButton *button, GtkWindow *parent);
static void msgbox( GtkWindow *parent, char * msg );
static void cut_clicked (GtkButton*, GtkTextView*);
static void copy_clicked (GtkButton*, GtkTextView*);
static void paste_clicked (GtkButton*, GtkTextView*);
void setup_tree(void);

void gtk_box_pack_start_defaults(GtkBox *box, GtkWidget *widget)  {
	gtk_box_pack_start(box, widget, TRUE, TRUE, 0);
}

int main( int argc, char *argv[] )
{
    GtkWidget *window, *vbox, *authbox, *authbox2, *vauthbox, *hboxccp;
 //   GtkWidget *lbluser, *lblpass; /* labels */
    GtkWidget *submit, *cut, *copy, *paste; /* buttons */
    GtkWidget *scrolled_win, *textview = NULL;

    gtk_init( &argc, &argv );

    window = gtk_window_new( GTK_WINDOW_TOPLEVEL );
    gtk_window_set_title( GTK_WINDOW (window), NAME );
    gtk_container_set_border_width( GTK_CONTAINER (window), 10 );
    gtk_widget_set_size_request( window, 400, 400 );

    g_signal_connect (G_OBJECT (window), "destroy",
                  G_CALLBACK (gtk_main_quit), NULL);

    cut = gtk_button_new_from_icon_name("edit-cut", GTK_ICON_SIZE_SMALL_TOOLBAR);
    copy = gtk_button_new_from_icon_name("edit-copy", GTK_ICON_SIZE_SMALL_TOOLBAR);
    paste = gtk_button_new_from_icon_name("edit-paste", GTK_ICON_SIZE_SMALL_TOOLBAR);

    hboxccp = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_pack_start (GTK_BOX (hboxccp), cut, TRUE, TRUE, 0);
    gtk_box_pack_start (GTK_BOX (hboxccp), copy, TRUE, TRUE, 0);
    gtk_box_pack_start (GTK_BOX (hboxccp), paste, TRUE, TRUE, 0);


    g_signal_connect (G_OBJECT (cut), "clicked",
                    G_CALLBACK (cut_clicked),
                    (gpointer) textview);
    g_signal_connect (G_OBJECT (copy), "clicked",
                    G_CALLBACK (copy_clicked),
                    (gpointer) textview);
    g_signal_connect (G_OBJECT (paste), "clicked",
                    G_CALLBACK (paste_clicked),
                    (gpointer) textview);

    submit = gtk_button_new_with_mnemonic("_Search");
    g_signal_connect (G_OBJECT (submit), "clicked",
                    G_CALLBACK (button_clicked),
                    (gpointer) window);

setup_tree();


   /* lbluser = gtk_label_new( "Username" );
    lblpass = gtk_label_new( "Password" );
    user = gtk_entry_new();
    pass = gtk_entry_new();
    gtk_entry_set_visibility( GTK_ENTRY (pass), FALSE );
    gtk_entry_set_invisible_char( GTK_ENTRY (pass), '*' );
	*/

	 search = gtk_entry_new();
   

    /* setup textview */
    textview = gtk_text_view_new();
    buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (textview));
    scrolled_win = gtk_scrolled_window_new (NULL, NULL);
    gtk_container_add (GTK_CONTAINER (scrolled_win), textview);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_win),
                                  GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);

    /* create username hbox */
    authbox = gtk_box_new( GTK_ORIENTATION_HORIZONTAL, 5 );
    gtk_box_pack_start_defaults( GTK_BOX (authbox), search );
    gtk_box_pack_start( GTK_BOX (authbox), submit, FALSE, TRUE, 5 );


    vauthbox = gtk_box_new( GTK_ORIENTATION_VERTICAL, 5 );
    gtk_box_pack_start_defaults( GTK_BOX (vauthbox), authbox );

    /* Setup the final box for layout in the window */
    vbox = gtk_box_new( GTK_ORIENTATION_VERTICAL, 5 );
    gtk_box_pack_start( GTK_BOX (vbox), hboxccp, FALSE, TRUE, 5 );
    gtk_box_pack_start( GTK_BOX (vbox), vauthbox, FALSE, TRUE, 5 );
   // gtk_box_pack_start( GTK_BOX (vbox), scrolled_win, TRUE, TRUE, 5 );
  //  gtk_box_pack_start( GTK_BOX (vbox), submit, FALSE, TRUE, 5 );
	gtk_box_pack_start( GTK_BOX (vbox), tree, FALSE, TRUE, 5 );

	
    gtk_container_add( GTK_CONTAINER (window), vbox );
    gtk_widget_show_all( window );

    gtk_main();
    return 0;
}

static void button_clicked( GtkButton *button, GtkWindow *parent )
{
     const gchar *query, *p; /* username, password */
    char *c;
    GtkTextIter start, end;
    char *errmsg = NULL;

    query = gtk_entry_get_text( GTK_ENTRY (search) );
   // p = gtk_entry_get_text( GTK_ENTRY (pass) );
    gtk_text_buffer_get_bounds (buffer, &start, &end);
    c = gtk_text_iter_get_text (&start, &end);

   
    /* if we get here it worked.  Clear the entry data */
    gtk_entry_set_text( GTK_ENTRY (search), "" );
 //   gtk_entry_set_text( GTK_ENTRY (pass), "" );
    gtk_text_buffer_set_text (buffer, "", -1);

cleanup:
    g_free(c);
}

static void msgbox( GtkWindow * parent, char * msg )
{
    GtkWidget *dialog, *label, *image, *hbox;

    /* Create a non-modal dialog with one OK button. */
    dialog = gtk_dialog_new_with_buttons ("Information", parent,
                                        GTK_DIALOG_DESTROY_WITH_PARENT,
					   ("_OK"),
                                      	GTK_RESPONSE_ACCEPT,
                                      	NULL);

    //gtk_dialog_set_has_separator (GTK_DIALOG (dialog), FALSE);

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
cut_clicked (GtkButton *cut,
             GtkTextView *textview)
{
    GtkClipboard *clipboard = gtk_clipboard_get (GDK_SELECTION_CLIPBOARD);
    //GtkTextBuffer *buffer2 = gtk_text_view_get_buffer (textview);

    gtk_text_buffer_cut_clipboard (buffer, clipboard, TRUE);
}

/* Copy the selected text to the clipboard. */
static void
copy_clicked (GtkButton *copy,
              GtkTextView *textview)
{
    GtkClipboard *clipboard = gtk_clipboard_get (GDK_SELECTION_CLIPBOARD);

    gtk_text_buffer_copy_clipboard (buffer, clipboard);
}

/* Insert the text from the clipboard into the text buffer. */
static void
paste_clicked (GtkButton *paste,
               GtkTextView *textview)
{
    GtkClipboard *clipboard = gtk_clipboard_get (GDK_SELECTION_CLIPBOARD);
    //GtkTextBuffer *buffer2 = gtk_text_view_get_buffer (textview);

    gtk_text_buffer_paste_clipboard (buffer, clipboard, NULL, TRUE);
}

enum
{
   TITLE_COLUMN,
   AUTHOR_COLUMN,
   CHECKED_COLUMN,
   N_COLUMNS
};

void
setup_tree (void)
{
   GtkTreeStore *store;
   
   GtkTreeViewColumn *column;
   GtkCellRenderer *renderer;

   /* Create a model.  We are using the store model for now, though we
    * could use any other GtkTreeModel */
   store = gtk_tree_store_new (N_COLUMNS,
                               G_TYPE_STRING,
                               G_TYPE_STRING,
                               G_TYPE_BOOLEAN);

   /* custom function to fill the model with data */
   //populate_tree_model (store);

   /* Create a view */
   tree = gtk_tree_view_new_with_model (GTK_TREE_MODEL (store));

   /* The view now holds a reference.  We can get rid of our own
    * reference */
   g_object_unref (G_OBJECT (store));

   /* Create a cell render and arbitrarily make it red for demonstration
    * purposes */
   renderer = gtk_cell_renderer_text_new ();
   g_object_set (G_OBJECT (renderer),
                 "foreground", "red",
                 NULL);

   /* Create a column, associating the "text" attribute of the
    * cell_renderer to the first column of the model */
   column = gtk_tree_view_column_new_with_attributes ("Author", renderer,
                                                      "text", AUTHOR_COLUMN,
                                                      NULL);

   /* Add the column to the view. */
   gtk_tree_view_append_column (GTK_TREE_VIEW (tree), column);

   /* Second column.. title of the book. */
   renderer = gtk_cell_renderer_text_new ();
   column = gtk_tree_view_column_new_with_attributes ("Title",
                                                      renderer,
                                                      "text", TITLE_COLUMN,
                                                      NULL);
   gtk_tree_view_append_column (GTK_TREE_VIEW (tree), column);

   /* Last column.. whether a book is checked out. */
   renderer = gtk_cell_renderer_toggle_new ();
   column = gtk_tree_view_column_new_with_attributes ("Checked out",
                                                      renderer,
                                                      "active", CHECKED_COLUMN,
                                                      NULL);
   gtk_tree_view_append_column (GTK_TREE_VIEW (tree), column);

   /* Now we can manipulate the view just like any other GTK widget */

}

