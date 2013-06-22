/*
 * Copyright (C) 2000-2006  Heikki Orsila
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <strings.h>
#include <limits.h>
#include <libgen.h>

#include <glib.h>
#include <gtk/gtk.h>
#include <audacious/plugin.h>
#include <audacious/util.h>

#include "ossupport.h"
#include "songinfo.h"
#include "fileinfo.h"
#include "plugin.h"


static char module_filename[PATH_MAX];
static char player_filename[PATH_MAX];

static GtkWidget *fileinfowin = NULL;
static GtkWidget *modinfowin = NULL;
static GtkTooltips *fileinfo_tooltips;
static GtkWidget *fileinfo_hexinfo_button;
static GtkWidget *fileinfo_moduleinfo_button;

static GtkWidget *fileinfo_modulename_txt;
static GtkWidget *fileinfo_playername_txt;
static GtkWidget *fileinfo_maxsubsong_txt;
static GtkWidget *fileinfo_minsubsong_txt;
static GtkWidget *fileinfo_subsong_txt;


static void uade_mod_info(char *credits, int creditslen);
static void uade_mod_info_hex(void);
static void uade_mod_info_module(void);
static void uade_player_info(void);

/* File Info Window */

void uade_gui_file_info(char *filename, char *gui_player_filename,
			char *modulename, char *playername, char *formatname)
{
    GtkWidget *fileinfo_base_vbox;
    GtkWidget *fileinfo_frame;
    GtkWidget *fileinfo_table;

    GtkWidget *fileinfo_modulename_label;
    GtkWidget *fileinfo_modulename_hbox;
    GtkWidget *fileinfo_hrule1;
    GtkWidget *fileinfo_playername_hbox;
    GtkWidget *fileinfo_playername_label;
    GtkWidget *fileinfo_playerinfo_button;
    GtkWidget *fileinfo_hrule2;
    GtkWidget *fileinfo_subsong_label;
    GtkWidget *fileinfo_minsubsong_label;
    GtkWidget *fileinfo_maxsubsong_label;
    GtkWidget *fileinfo_hrule3;
    GtkWidget *fileinfo_hrule4;

    GtkWidget *fileinfo_button_box;
    GtkWidget *ok_button;
#ifdef __AUDACIOUS_INPUT_PLUGIN_API__
    char * decoded = NULL;

    if (strncmp(filename, "file:/",6) == 0) {
      decoded = g_filename_from_uri((char *) filename, NULL, NULL);
      filename = decoded;
    }
#endif
    strlcpy(module_filename, filename, sizeof module_filename);
    strlcpy(player_filename, gui_player_filename, sizeof player_filename);


    if (fileinfowin == NULL) {
	fileinfo_tooltips = gtk_tooltips_new();
	fileinfowin = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(fileinfowin), "UADE file info");
	gtk_window_set_position(GTK_WINDOW(fileinfowin),
				GTK_WIN_POS_MOUSE);
	gtk_container_set_border_width(GTK_CONTAINER(fileinfowin), 10);
	gtk_window_set_policy(GTK_WINDOW(fileinfowin), FALSE, FALSE,
			      FALSE);
	gtk_signal_connect(GTK_OBJECT(fileinfowin), "destroy",
			   GTK_SIGNAL_FUNC(gtk_widget_destroyed),
			   &fileinfowin);
	
	/* Start of Contents Box */
	
	fileinfo_base_vbox = gtk_vbox_new(FALSE, 10);
	
	gtk_container_set_border_width(GTK_CONTAINER
				       (fileinfo_base_vbox), 5);
	gtk_container_add(GTK_CONTAINER(fileinfowin),
			  fileinfo_base_vbox);
	
	/* Start of File info frame, text and option widgets */
	
	fileinfo_frame = gtk_frame_new("File info: ");
	gtk_box_pack_start(GTK_BOX(fileinfo_base_vbox), fileinfo_frame,
			   TRUE, TRUE, 0);
	
	/* Start of Fileinfotable */
	
	fileinfo_table = gtk_table_new(12, 2, FALSE);
	
	gtk_widget_show(fileinfo_table);
	gtk_container_add(GTK_CONTAINER(fileinfo_frame),
			  fileinfo_table);
	gtk_container_set_border_width(GTK_CONTAINER(fileinfo_table), 5);
	
	/* 1x1 */
	
	fileinfo_modulename_label = gtk_label_new("Module: ");
	gtk_misc_set_padding(GTK_MISC(fileinfo_modulename_label), 5,
			     5);
	/* gtk_misc_set_alignment (GTK_MISC (fileinfo_modulename_label), 1, 0); */
	gtk_table_attach(GTK_TABLE(fileinfo_table),
			 fileinfo_modulename_label, 0, 1, 0, 1,
			 (GtkAttachOptions) (GTK_FILL),
			 (GtkAttachOptions) (GTK_FILL), 0, 0);
	
/* 1x2 */
/* 1x3 */
	
	fileinfo_hrule1 = gtk_hseparator_new();
	gtk_table_attach(GTK_TABLE(fileinfo_table), fileinfo_hrule1, 0,
			 1, 2, 3, (GtkAttachOptions) (GTK_FILL),
			 (GtkAttachOptions) (0), 0, 0);
	
/* 1x4 */
	
	fileinfo_playername_label = gtk_label_new("Playerformat: ");
	gtk_misc_set_padding(GTK_MISC(fileinfo_playername_label), 5,
			     5);
	/* gtk_misc_set_alignment (GTK_MISC (fileinfo_playername_label), 1, 0); */
	gtk_table_attach(GTK_TABLE(fileinfo_table),
			 fileinfo_playername_label, 0, 1, 3, 4,
			 (GtkAttachOptions) (GTK_FILL),
			 (GtkAttachOptions) (GTK_FILL), 0, 0);
	
	
	fileinfo_hrule2 = gtk_hseparator_new();
	gtk_table_attach(GTK_TABLE(fileinfo_table), fileinfo_hrule2, 0,
			 1, 6, 7, (GtkAttachOptions) (GTK_FILL),
			 (GtkAttachOptions) (0), 0, 0);
	
/* 1x8*/
	
	fileinfo_subsong_label = gtk_label_new("Curr. subsong: ");
	gtk_misc_set_padding(GTK_MISC(fileinfo_subsong_label), 5, 5);
	/* gtk_misc_set_alignment (GTK_MISC (fileinfo_subsong_label), 1, 0); */
	gtk_table_attach(GTK_TABLE(fileinfo_table),
			 fileinfo_subsong_label, 0, 1, 7, 8,
			 (GtkAttachOptions) (GTK_FILL),
			 (GtkAttachOptions) (GTK_FILL), 0, 0);
	
/* 1x9*/
	
	fileinfo_minsubsong_label = gtk_label_new("Min. subsong: ");
	gtk_misc_set_padding(GTK_MISC(fileinfo_minsubsong_label), 5,
			     5);
	/* gtk_misc_set_alignment (GTK_MISC (fileinfo_minsubsong_label), 1, 0); */
	gtk_table_attach(GTK_TABLE(fileinfo_table),
			 fileinfo_minsubsong_label, 0, 1, 8, 9,
			 (GtkAttachOptions) (GTK_FILL),
			 (GtkAttachOptions) (GTK_FILL), 0, 0);
	
/* 1x10*/
	
	fileinfo_maxsubsong_label = gtk_label_new("Max. subsong: ");
	gtk_misc_set_padding(GTK_MISC(fileinfo_maxsubsong_label), 5,
			     5);
	/* gtk_misc_set_alignment (GTK_MISC (fileinfo_maxsubsong_label), 1, 0); */
	gtk_table_attach(GTK_TABLE(fileinfo_table),
			 fileinfo_maxsubsong_label, 0, 1, 9, 10,
			 (GtkAttachOptions) (GTK_FILL),
			 (GtkAttachOptions) (GTK_FILL), 0, 0);
	
/* 2nd Column */
/* 2x1*/
	fileinfo_modulename_hbox = gtk_hbox_new(FALSE, 10);
	gtk_table_attach(GTK_TABLE(fileinfo_table),
			 fileinfo_modulename_hbox, 1, 2, 0, 1,
			 (GtkAttachOptions) (GTK_FILL),
			 (GtkAttachOptions) (0), 0, 0);
	
	if (modulename[0] == 0) {
	    fileinfo_modulename_txt = gtk_label_new(basename(filename));
	} else {
	    fileinfo_modulename_txt = gtk_label_new(g_strdup_printf("%s\n(%s)",
							    modulename,
							    basename(filename)
							    ));
	}

	gtk_label_set_justify(GTK_LABEL(fileinfo_modulename_txt),
			      GTK_JUSTIFY_LEFT);
	gtk_label_set_line_wrap(GTK_LABEL(fileinfo_modulename_txt),
				TRUE);
	gtk_misc_set_alignment(GTK_MISC(fileinfo_modulename_txt), 0,
			       0.5);
	gtk_misc_set_padding(GTK_MISC(fileinfo_modulename_txt), 5, 5);
	

	fileinfo_hexinfo_button = gtk_button_new_with_label("hex");
	GTK_WIDGET_SET_FLAGS(fileinfo_hexinfo_button,
			     GTK_CAN_DEFAULT);
	gtk_widget_ref(fileinfo_hexinfo_button);
	gtk_object_set_data_full(GTK_OBJECT(fileinfowin),
				 "fileinfo_hexinfo_button",
				 fileinfo_hexinfo_button,
				 (GtkDestroyNotify) gtk_widget_unref);
	gtk_tooltips_set_tip(fileinfo_tooltips,
			     fileinfo_hexinfo_button,
			     g_strdup_printf("%s", filename),
			     NULL);
	
	gtk_signal_connect_object(GTK_OBJECT
				  (fileinfo_hexinfo_button),
				  "clicked",
				  GTK_SIGNAL_FUNC(uade_mod_info_hex), NULL);
	

	fileinfo_moduleinfo_button = gtk_button_new_with_label("Info");
	GTK_WIDGET_SET_FLAGS(fileinfo_moduleinfo_button,
			     GTK_CAN_DEFAULT);
	
	gtk_widget_ref(fileinfo_moduleinfo_button);
	gtk_object_set_data_full(GTK_OBJECT(fileinfowin),
				 "fileinfo_moduleinfo_button",
				 fileinfo_moduleinfo_button,
				 (GtkDestroyNotify) gtk_widget_unref);
	gtk_tooltips_set_tip(fileinfo_tooltips,
			     fileinfo_moduleinfo_button,
			     g_strdup_printf("%s", filename),
			     NULL);
	
	
	gtk_signal_connect_object(GTK_OBJECT
				  (fileinfo_moduleinfo_button),
				  "clicked",
				  GTK_SIGNAL_FUNC(uade_mod_info_module), NULL);

	
	gtk_box_pack_start(GTK_BOX(fileinfo_modulename_hbox),
			   fileinfo_modulename_txt, TRUE, TRUE, 0);
	gtk_box_pack_start_defaults(GTK_BOX(fileinfo_modulename_hbox),
				    fileinfo_hexinfo_button);
	gtk_box_pack_start_defaults(GTK_BOX(fileinfo_modulename_hbox),
				    fileinfo_moduleinfo_button);
	
/* 2x2*/
/* 2x3*/
	fileinfo_hrule3 = gtk_hseparator_new();
	gtk_table_attach(GTK_TABLE(fileinfo_table), fileinfo_hrule3, 1,
			 2, 2, 3, (GtkAttachOptions) (GTK_FILL),
			 (GtkAttachOptions) (0), 0, 0);
	
	
/* 2x4*/
	
	fileinfo_playername_hbox = gtk_hbox_new(FALSE, 10);
	gtk_table_attach(GTK_TABLE(fileinfo_table),
			 fileinfo_playername_hbox, 1, 2, 3, 4,
			 (GtkAttachOptions) (GTK_FILL),
			 (GtkAttachOptions) (0), 0, 0);
	
	
	if (formatname[0] == 0) {
	    fileinfo_playername_txt = gtk_label_new(g_strdup_printf("%s", playername));
	} else {
	    /* memory leaks using g_strdup_printf? */
	    fileinfo_playername_txt = gtk_label_new(g_strdup_printf("%s\n%s", playername, formatname));
	}
	
	/* fileinfo_playername_txt = gtk_label_new (get_playername()); */
	gtk_label_set_justify(GTK_LABEL(fileinfo_playername_txt),
			      GTK_JUSTIFY_LEFT);
	gtk_label_set_line_wrap(GTK_LABEL(fileinfo_playername_txt),
				TRUE);
	gtk_misc_set_alignment(GTK_MISC(fileinfo_playername_txt), 0,
			       0.5);
	gtk_misc_set_padding(GTK_MISC(fileinfo_playername_txt), 5, 5);
	
	fileinfo_playerinfo_button = gtk_button_new_with_label("?");
	GTK_WIDGET_SET_FLAGS(fileinfo_playerinfo_button,
			     GTK_CAN_DEFAULT);
	gtk_signal_connect_object(GTK_OBJECT
				  (fileinfo_playerinfo_button),
				  "clicked",
				  GTK_SIGNAL_FUNC(uade_player_info),
				  NULL);
	
	
	gtk_box_pack_start(GTK_BOX(fileinfo_playername_hbox),
			   fileinfo_playername_txt, TRUE, TRUE, 0);
	gtk_box_pack_start_defaults(GTK_BOX(fileinfo_playername_hbox),
				    fileinfo_playerinfo_button);
	
	
	fileinfo_hrule4 = gtk_hseparator_new();
	gtk_table_attach(GTK_TABLE(fileinfo_table), fileinfo_hrule4, 1,
			 2, 6, 7, (GtkAttachOptions) (GTK_FILL),
			 (GtkAttachOptions) (0), 0, 0);
	
	
	fileinfo_subsong_txt =
	    gtk_label_new(g_strdup_printf("%d", uade_get_cur_subsong(0)));
	
	/* gtk_widget_setusize for this widget is a bit of a cludge to set 
	   a minimal size for the FileinfoWindow. I can't get it to work either with
	   setting the usize for the window or the table... weird */
	
	gtk_widget_set_usize(fileinfo_subsong_txt, 176, -2);
	
	
	gtk_table_attach(GTK_TABLE(fileinfo_table),
			 fileinfo_subsong_txt, 1, 2, 7, 8,
			 (GtkAttachOptions) (GTK_FILL),
			 (GtkAttachOptions) (0), 0, 0);
	gtk_label_set_justify(GTK_LABEL(fileinfo_subsong_txt),
			      GTK_JUSTIFY_LEFT);
	gtk_label_set_line_wrap(GTK_LABEL(fileinfo_subsong_txt), TRUE);
	gtk_misc_set_alignment(GTK_MISC(fileinfo_subsong_txt), 0, 0.5);
	gtk_misc_set_padding(GTK_MISC(fileinfo_subsong_txt), 5, 5);
	
/* 2x9*/
	fileinfo_minsubsong_txt =
	    gtk_label_new(g_strdup_printf("%d", uade_get_min_subsong(0)));
	gtk_table_attach(GTK_TABLE(fileinfo_table),
			 fileinfo_minsubsong_txt, 1, 2, 8, 9,
			 (GtkAttachOptions) (GTK_FILL),
			 (GtkAttachOptions) (0), 0, 0);
	gtk_label_set_justify(GTK_LABEL(fileinfo_minsubsong_txt),
			      GTK_JUSTIFY_LEFT);
	gtk_label_set_line_wrap(GTK_LABEL(fileinfo_subsong_txt), TRUE);
	gtk_misc_set_alignment(GTK_MISC(fileinfo_minsubsong_txt), 0,
			       0.5);
	gtk_misc_set_padding(GTK_MISC(fileinfo_minsubsong_txt), 5, 5);
	
/* 2x10*/
	fileinfo_maxsubsong_txt =
	    gtk_label_new(g_strdup_printf("%d", uade_get_max_subsong(0)));
	gtk_table_attach(GTK_TABLE(fileinfo_table),
			 fileinfo_maxsubsong_txt, 1, 2, 9, 10,
			 (GtkAttachOptions) (GTK_FILL),
			 (GtkAttachOptions) (0), 0, 0);
	gtk_label_set_justify(GTK_LABEL(fileinfo_maxsubsong_txt),
			      GTK_JUSTIFY_LEFT);
	gtk_label_set_line_wrap(GTK_LABEL(fileinfo_maxsubsong_txt),
				TRUE);
	gtk_misc_set_alignment(GTK_MISC(fileinfo_maxsubsong_txt), 0,
			       0.5);
	gtk_misc_set_padding(GTK_MISC(fileinfo_maxsubsong_txt), 5, 5);
	
/* end of frame. */
	
/* Start of Ok and Cancel Button Box */
	
	fileinfo_button_box = gtk_hbutton_box_new();
	
	
	gtk_button_box_set_layout(GTK_BUTTON_BOX(fileinfo_button_box),
				  GTK_BUTTONBOX_END);
	gtk_button_box_set_spacing(GTK_BUTTON_BOX(fileinfo_button_box),
				   5);
	gtk_box_pack_start(GTK_BOX(fileinfo_base_vbox),
			   fileinfo_button_box, FALSE, FALSE, 0);
	
	
	ok_button = gtk_button_new_with_label("Close");
	GTK_WIDGET_SET_FLAGS(ok_button, GTK_CAN_DEFAULT);
	gtk_signal_connect_object(GTK_OBJECT(ok_button), "clicked",
				  GTK_SIGNAL_FUNC(gtk_widget_destroy),
				  GTK_OBJECT(fileinfowin));
	
	gtk_box_pack_start_defaults(GTK_BOX(fileinfo_button_box),
				    ok_button);
	
/* End of Button Box */
	
	gtk_widget_show_all(fileinfowin);
	
    } else {
	gdk_window_raise(fileinfowin->window);
    }

#ifdef __AUDACIOUS_INPUT_PLUGIN_API__
    if (decoded != NULL)
       free (decoded);
#endif
}

void file_info_update(char *gui_module_filename, char *gui_player_filename,
		      char *gui_modulename, char *gui_playername,
		      char *gui_formatname)
{

    if (fileinfowin != NULL) {
	strlcpy(module_filename, gui_module_filename, sizeof module_filename);
	strlcpy(player_filename, gui_player_filename, sizeof player_filename);

	gdk_window_raise(fileinfowin->window);

        if (gui_modulename[0] == 0) {
    	    gtk_label_set_text(GTK_LABEL(fileinfo_modulename_txt),
    			   g_strdup_printf("%s", basename(gui_module_filename)));
        } else {
    	    gtk_label_set_text(GTK_LABEL(fileinfo_modulename_txt),
    			   g_strdup_printf("%s\n(%s)", gui_modulename,
						basename(gui_module_filename)));
	}

	gtk_widget_show(fileinfo_modulename_txt);

        if (gui_formatname[0] == 0) {
    	    gtk_label_set_text(GTK_LABEL(fileinfo_playername_txt),
    			   g_strdup_printf("%s", gui_playername));
        } else {
    	    gtk_label_set_text(GTK_LABEL(fileinfo_playername_txt),
			   g_strdup_printf("%s\n%s", gui_playername,
						    gui_formatname));
	}

	gtk_widget_show(fileinfo_playername_txt);
	gtk_label_set_text(GTK_LABEL(fileinfo_subsong_txt),
		       g_strdup_printf("%d", uade_get_cur_subsong(0)));
	gtk_widget_show(fileinfo_subsong_txt);

	gtk_label_set_text(GTK_LABEL(fileinfo_minsubsong_txt),
		       g_strdup_printf("%d", uade_get_min_subsong(0)));
	gtk_widget_show(fileinfo_minsubsong_txt);

	gtk_label_set_text(GTK_LABEL(fileinfo_maxsubsong_txt),
		       g_strdup_printf("%d", uade_get_max_subsong(0)));
	gtk_widget_show(fileinfo_maxsubsong_txt);

	gtk_tooltips_set_tip(fileinfo_tooltips,
			     fileinfo_hexinfo_button,
			     g_strdup_printf("%s", gui_module_filename),
			     NULL);

	gtk_tooltips_set_tip(fileinfo_tooltips,
			     fileinfo_moduleinfo_button,
			     g_strdup_printf("%s", gui_module_filename),
			     NULL);

	gtk_widget_show(fileinfo_moduleinfo_button);
    }
}

static void uade_player_info(void)
{
    char credits[16384];
	if ((uade_song_info(credits, sizeof credits, player_filename, UADE_HEX_DUMP_INFO))) {
	  strcpy(credits, "No info for player.\n");
	}
        uade_mod_info(credits, sizeof credits);
}

static void uade_mod_info_hex(void)
{
    char credits[16384];
    if (uade_song_info(credits, sizeof credits, module_filename, UADE_HEX_DUMP_INFO))
        snprintf(credits, sizeof credits, "Unable to process file %s\n", module_filename);


    uade_mod_info(credits, sizeof credits);
}

static void uade_mod_info_module(void)
{
    char credits[16384];
    if (uade_song_info(credits, sizeof credits, module_filename, UADE_MODULE_INFO))
	snprintf(credits, sizeof credits, "Unable to process file %s\n", module_filename);

    uade_mod_info(credits, sizeof credits);
}

static void uade_mod_info(char *credits, int creditslen)
{
    GtkWidget *modinfo_button_box;
    GtkWidget *close_button;
    GtkWidget *modinfo_base_vbox;

    GtkWidget *uadeplay_scrolledwindow;
    GtkWidget *uadeplay_textview;

    GtkTextBuffer *uadeplay_textbuffer;
    GtkTextIter textIter;

    gchar *text =NULL;
    GError *error = NULL;

    if (!modinfowin) {

	/* *sigh* code inflation to get just a simple fixed font text view*/
	/* first we have to convert to utf-8 */
	/* then we have to mess around with Iters,Tags, Views and Buffer */
	/* gtk1.2 was easier to use in that aspect */
	
	if (!g_utf8_validate (credits, -1, NULL))
	{
	 text = g_locale_to_utf8 (credits, -1, NULL, NULL, &error);
	 if (!text)
	    {
		g_error_free (error);
		text = g_convert_with_fallback(credits, creditslen, "UTF8", "ISO-8859-1", ".", NULL, NULL, NULL);
		if (!text) text = g_strdup ("unable to convert to UTF-8");
	    
	     }
	  } else text = g_strdup (credits);

	modinfowin = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(modinfowin), "UADE Modinfo");
	gtk_window_set_position(GTK_WINDOW(modinfowin), GTK_WIN_POS_MOUSE);
	gtk_container_set_border_width(GTK_CONTAINER(modinfowin), 10);
	gtk_window_set_policy(GTK_WINDOW(modinfowin), FALSE, FALSE, FALSE);
	gtk_signal_connect(GTK_OBJECT(modinfowin), "destroy",
			   GTK_SIGNAL_FUNC(gtk_widget_destroyed),
			   &modinfowin);
//Start of Contents Box

	modinfo_base_vbox = gtk_vbox_new(FALSE, 10);

	gtk_container_set_border_width(GTK_CONTAINER(modinfo_base_vbox),
				       5);
	gtk_container_add(GTK_CONTAINER(modinfowin), modinfo_base_vbox);

	uadeplay_scrolledwindow = gtk_scrolled_window_new(NULL, NULL);
	gtk_widget_set_usize(uadeplay_scrolledwindow, 600, 256);

	gtk_container_add(GTK_CONTAINER(modinfo_base_vbox),
			  uadeplay_scrolledwindow);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW
				       (uadeplay_scrolledwindow),
				       GTK_POLICY_AUTOMATIC,
				       GTK_POLICY_AUTOMATIC);

	uadeplay_textview = gtk_text_view_new();
	gtk_text_view_set_editable(GTK_TEXT_VIEW(uadeplay_textview), FALSE);
	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(uadeplay_scrolledwindow), uadeplay_textview);
	


	uadeplay_textbuffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(uadeplay_textview));
	gtk_text_view_set_buffer(GTK_TEXT_VIEW(uadeplay_textview), uadeplay_textbuffer);	

	gtk_text_buffer_get_iter_at_offset(GTK_TEXT_BUFFER(uadeplay_textbuffer), &textIter,0);
	gtk_text_buffer_create_tag (uadeplay_textbuffer, "monospaced", 
				    "family", "Monospace",
				     "wrap_mode", GTK_WRAP_NONE,
				     NULL);

	gtk_text_buffer_insert_with_tags_by_name(uadeplay_textbuffer,
					&textIter,
					text, -1,
					"monospaced", NULL);
					
				 
	gtk_text_view_set_buffer(GTK_TEXT_VIEW(uadeplay_textview), uadeplay_textbuffer);	


// Start of Close Button Box

	modinfo_button_box = gtk_hbutton_box_new();

	gtk_button_box_set_layout(GTK_BUTTON_BOX(modinfo_button_box),
				  GTK_BUTTONBOX_END);
	gtk_button_box_set_spacing(GTK_BUTTON_BOX(modinfo_button_box), 5);
	gtk_box_pack_start(GTK_BOX(modinfo_base_vbox), modinfo_button_box,
			   FALSE, FALSE, 0);

	close_button = gtk_button_new_with_label("Close");
	GTK_WIDGET_SET_FLAGS(close_button, GTK_CAN_DEFAULT);
	gtk_signal_connect_object(GTK_OBJECT(close_button), "clicked",
				  GTK_SIGNAL_FUNC(gtk_widget_destroy),
				  GTK_OBJECT(modinfowin));

	gtk_box_pack_start_defaults(GTK_BOX(modinfo_button_box),
				    close_button);
	gtk_widget_show_all(modinfowin);
    } else {
	gdk_window_raise(modinfowin->window);
    }
}
