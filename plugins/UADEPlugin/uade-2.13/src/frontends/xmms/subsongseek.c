#include <glib.h>
#include <gtk/gtk.h>
#include <xmms/plugin.h>
#include <xmms/configfile.h>
#include <xmms/util.h>

#include "subsongseek.h"
#include "plugin.h"


static GtkWidget *seekpopup = NULL;
static GtkObject *subsong_adj;
static int seekpopup_open = 0;


static void uade_seek_directly(void);
static void uade_seek_next(void);
static void uade_seek_previous(void);
static void uade_ffwd(void);
static void focus_out_event();
static void uade_seek_update_display(int subsong);


static int get_next_subsong(void)
{
    int ispaused;
    uade_lock();
    ispaused = uade_is_paused;
    uade_unlock();
    if (ispaused == 0) {
	int newsubsong;
	newsubsong = uade_get_cur_subsong(-1);
	if (newsubsong == -1)
	    return -1;
	newsubsong++;
	return newsubsong;
    }
    return -1;
}


static int get_previous_subsong(void)
{
    int ispaused;
    uade_lock();
    ispaused = uade_is_paused;
    uade_unlock();
    if (ispaused == 0) {
	int newsubsong;
	newsubsong = uade_get_cur_subsong(-1);
	if (newsubsong == -1)
	    return -1;
	if (newsubsong > uade_get_min_subsong(-1)) {
	    newsubsong--;
	    return newsubsong;
	}
    }
    return -1;
}


/* popup for seeking to a subsong*/

void uade_gui_seek_subsong(int to)
{
    GtkWidget *seek_button_box;
    GtkWidget *prev_next_button_box;
    GtkWidget *seek_button_vbox;
    GtkWidget *seek_slider_box;
    GtkWidget *hscale;

    GtkWidget *prev_button;
    GtkWidget *prev_button_frame;
    GtkWidget *frame;
    GtkWidget *maxsong_label;
    GtkWidget *next_button,*ffwd_button;
    GtkWidget *ffwd_button_frame;

    if (!uade_thread_running) {
	fprintf(stderr, "uade: BUG! Seek not possible.\n");
	return;
    }

    if (seekpopup == NULL) {
	/* uade's subsong popup */
	seekpopup = gtk_window_new(GTK_WINDOW_DIALOG);
	gtk_window_set_title(GTK_WINDOW(seekpopup), "UADE seek subsong");
	gtk_window_set_position(GTK_WINDOW(seekpopup), GTK_WIN_POS_MOUSE);
	gtk_container_set_border_width(GTK_CONTAINER(seekpopup), 0);

	gtk_window_set_policy(GTK_WINDOW(seekpopup), FALSE, FALSE, FALSE);

	gtk_signal_connect(GTK_OBJECT(seekpopup), "destroy",
			   GTK_SIGNAL_FUNC(gtk_widget_destroyed),
			   &seekpopup);

	gtk_signal_connect(GTK_OBJECT(seekpopup), "focus_out_event",
			   GTK_SIGNAL_FUNC(focus_out_event), NULL);

	gtk_widget_realize(seekpopup);
	gdk_window_set_decorations(seekpopup->window, 0);

        /* define Slider code, will be used by all styles of the popup */

	if (uade_get_max_subsong(-1) >= 0) {

	    subsong_adj =
		gtk_adjustment_new(uade_get_cur_subsong(0), uade_get_min_subsong(0),
				   uade_get_max_subsong(0), 1, 0, 0);	/*our scale for the subsong slider */
	    maxsong_label =
		gtk_label_new(g_strdup_printf("%d", uade_get_max_subsong(0))); /* until we can't get the reliable maximum number of subsongs this has to do :-) */
	    gtk_widget_set_usize(maxsong_label, 24, -1);

	} else {
	    subsong_adj =
		gtk_adjustment_new(uade_get_cur_subsong(0), uade_get_min_subsong(0),
				   (uade_get_max_subsong(0)) + 10, 1, 0, 0);	/*our scale for the subsong slider */
	    /*currently: min - min+10  */
	    maxsong_label = gtk_label_new("..."); /* until we can't get the reliable maximum number of subsongs this has to do :-) */
	    gtk_widget_set_usize(maxsong_label, 24, -1);
	}

	hscale = gtk_hscale_new(GTK_ADJUSTMENT(subsong_adj));
	gtk_widget_set_usize(hscale, 160, -1);
	gtk_scale_set_digits(GTK_SCALE(hscale), 0);
	gtk_scale_set_value_pos(GTK_SCALE(hscale), GTK_POS_LEFT);
	gtk_scale_set_draw_value(GTK_SCALE(hscale), TRUE);
	gtk_range_set_update_policy(GTK_RANGE(hscale),
				    GTK_UPDATE_DISCONTINUOUS);
	gtk_signal_connect_object(GTK_OBJECT(subsong_adj), "value_changed",
				  GTK_SIGNAL_FUNC(uade_seek_directly),
				  NULL);


        /* previous subsong button, will be used by all styles of the seek popup*/
	prev_button = gtk_button_new_with_label("<");
	gtk_widget_set_usize(prev_button, 27, -1);
	gtk_signal_connect_object(GTK_OBJECT(prev_button), "clicked",
				  GTK_SIGNAL_FUNC(uade_seek_previous),
				  NULL);

	prev_button_frame = gtk_frame_new(NULL);
	gtk_frame_set_shadow_type(GTK_FRAME(prev_button_frame),
				  GTK_SHADOW_IN);


        /* next subsong button, will be used by all styles of the seek popup*/
	next_button = gtk_button_new_with_label(">");
	gtk_widget_set_usize(next_button, 27, -1);
	gtk_signal_connect_object(GTK_OBJECT(next_button), "clicked",
				  GTK_SIGNAL_FUNC(uade_seek_next), NULL);

	ffwd_button_frame = gtk_frame_new(NULL);
	gtk_frame_set_shadow_type(GTK_FRAME(ffwd_button_frame),
				  GTK_SHADOW_IN);

	ffwd_button = gtk_button_new_with_label("10s fwd");
	gtk_widget_set_usize(ffwd_button, 27, -1);
	gtk_signal_connect_object(GTK_OBJECT(ffwd_button), "clicked",
				  GTK_SIGNAL_FUNC(uade_ffwd), NULL);

	/* with the alternative styles of the subsongseeker,
	 * following suggestions made by David Le Corfec*/
	seek_button_box = gtk_hbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(seekpopup), seek_button_box);

	frame = gtk_frame_new(NULL);
	gtk_box_pack_start_defaults(GTK_BOX(seek_button_box), frame);
	gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_OUT);


	seek_button_vbox = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(frame), seek_button_vbox);
	gtk_signal_connect(GTK_OBJECT(seek_button_vbox), "focus_out_event",
			   GTK_SIGNAL_FUNC(focus_out_event), NULL);

	prev_next_button_box = gtk_hbox_new(FALSE, 0);

	/* use the previous defined buttons here */

	gtk_box_pack_start_defaults(GTK_BOX(seek_button_vbox),
			   prev_button_frame);

	gtk_container_add(GTK_CONTAINER(prev_button_frame),
			   prev_next_button_box);

	gtk_box_pack_start_defaults(GTK_BOX(prev_next_button_box),
			   prev_button);
	gtk_box_pack_start_defaults(GTK_BOX(prev_next_button_box),
			   next_button);



	seek_slider_box = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(seek_button_vbox), seek_slider_box,
			   FALSE, FALSE, 0);

	/* use the previous defined slider and label here */
	gtk_box_pack_start(GTK_BOX(seek_slider_box), hscale, FALSE, FALSE,
			   0);
	gtk_box_pack_start(GTK_BOX(seek_slider_box), maxsong_label, FALSE,
			   FALSE, 0);

	/* use the previous defined buttons here */
	gtk_box_pack_start_defaults(GTK_BOX(seek_button_vbox),
			   ffwd_button_frame);
	gtk_container_add(GTK_CONTAINER(ffwd_button_frame), 		 
			   ffwd_button);

	gtk_widget_show_all(seekpopup);
	seekpopup_open = 1;

    } else {
	gdk_window_raise(seekpopup->window);
    }
}


void uade_gui_subsong_changed(int subsong)
{
    if (seekpopup_open) {
	GTK_ADJUSTMENT(subsong_adj)->value = subsong;
        gtk_adjustment_changed(GTK_ADJUSTMENT(subsong_adj));	/*here GTK gets the signal */
	}
}


static void uade_seek_directly(void)
{
    /* get the subsong the user selected from scale */
    int subsong = (gint) GTK_ADJUSTMENT(subsong_adj)->value;
    int cursub = uade_get_cur_subsong(-1);
    if (cursub >= 0 && cursub != subsong)
	uade_select_sub = subsong;
}


/* uade_seek_next() and uade_seek_previous ()
 *
 * Some desc: the functions get_next_subsong() and get_previous_previous() in
 * uade.c only do some checking and return the new valid subsongnumber.
 * the subsongchanging is done in uade_seek_directly()!
 *
 * uade_seek_directly itself is not (!) called literally from uade_seek_next()
 * and previous, but we are updating the value of hscale and thanks to 
 * GTK signalling the uade_seek_directly is invoked automatically.
 * A bit tricky but it works ;-)
 */

static void uade_ffwd(void)
{
    uade_lock();
    uade_seek_forward += 10;
    uade_unlock();
}


static void uade_seek_next(void)
{
    int subsong = get_next_subsong();
    if (subsong != -1) {
	/* Call update scale with new subsong */
	uade_seek_update_display(subsong);
    }
}

static void uade_seek_previous(void)
{
    int subsong = get_previous_subsong();	/* just returns subsong-- */
    if (subsong != -1) {
	/* Call update scale with new subsong */
	uade_seek_update_display(subsong);
    }
}

static void uade_seek_update_display(int subsong)
{
    /*update scale with new subsong value */
    GTK_ADJUSTMENT(subsong_adj)->value = subsong;
    gtk_adjustment_value_changed(GTK_ADJUSTMENT(subsong_adj));	/*here GTK gets the signal */
}


static void focus_out_event(void)
{
    seekpopup_open = 0;
    gtk_widget_destroy(seekpopup);
}

/* Don't know. Is it a good thing to clean up our mess (aka windows)
 * after stop() was called ??? 
 */
void uade_gui_close_subsong_win(void)
{
    if (seekpopup_open){
    	seekpopup_open = 0;
	gtk_widget_destroy(seekpopup);
    }
}
