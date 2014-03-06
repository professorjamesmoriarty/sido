/* 
 * callbacks.c Copyright © 2009 by Benoît Rouits <brouits@free.fr>
 * Published under the terms of the GNU General Public License v2 (GPLv2).
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, 
 * Boston, MA  02110-1301, USA.
 * 
 * see the COPYING file included in the jackie package or
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt for the full licence
 * 
 */

#include <gtk/gtk.h>
#include <libintl.h>
#include <locale.h>
#include "controller.h"
#include "menu.h"
#include "about.h"
#include "version.h"
#define _(string) gettext (string)

/* refresh the tooltip */
gboolean tooltip_display (gpointer app_data) {
	AppData* d = (AppData*) app_data;
	gchar* tooltip;

	tooltip = mpd_get_song(d->sock);
	gtk_status_icon_set_tooltip(d->status_icon, tooltip);
	g_free(tooltip);
	return TRUE;
}

/* handler for the volume button drag */
void volume_on_change (GtkWidget* instance, gpointer app_data) {
	gchar* cmd = NULL;
	gdouble value = 0;
	AppData* d = (AppData*) app_data;

	value = gtk_range_get_value(GTK_RANGE(d->volume));
	cmd = g_strdup_printf ("setvol %.0f", value);
	mpd_control (d->sock, cmd);
	g_free (cmd);
	instance = NULL;
}

/* handler for volume slider leaving  (close) */
gboolean volume_on_leave (GtkWidget* instance, GdkEventFocus* event, gpointer app_data)
{
	AppData* d = (AppData*) app_data;

	gtk_widget_hide (d->vol_window);
	gtk_widget_destroy (d->vol_window);
	d->vol_window = NULL;
	instance = NULL;
	event = NULL;
	return FALSE;
}

/* handler for left-button click */
void status_icon_on_left_click(GtkStatusIcon* instance, gpointer app_data) {
	AppData* d = (AppData*) app_data;
	gdouble curvol;

	if (NULL != d->vol_window) /* already set up */
		return;
	d->vol_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_decorated (GTK_WINDOW(d->vol_window), FALSE);
	gtk_window_set_default_size(GTK_WINDOW(d->vol_window), 25, 128);
	gtk_window_set_skip_taskbar_hint(GTK_WINDOW(d->vol_window), TRUE);
	gtk_window_set_position(GTK_WINDOW(d->vol_window), GTK_WIN_POS_MOUSE);
	d->volume = gtk_vscale_new_with_range (0, 100, 1);
	gtk_range_set_inverted(GTK_RANGE(d->volume), TRUE);
	curvol = mpd_get_vol(d->sock);
	gtk_range_set_value(GTK_RANGE(d->volume), curvol);
	gtk_container_add (GTK_CONTAINER(d->vol_window), d->volume);
	g_signal_connect(G_OBJECT(d->volume), "value-changed", G_CALLBACK(volume_on_change), app_data);
	g_signal_connect(G_OBJECT(d->volume), "focus-out-event", G_CALLBACK(volume_on_leave), app_data);
	gtk_widget_show_all(d->vol_window);
	instance = NULL;
}

/* handler for right-button click */
void status_icon_on_right_click(GtkStatusIcon* instance, guint button, guint activate_time, gpointer app_data) {
	AppData* d = (AppData*) app_data;

	if (app_data)
		menu_show (GTK_MENU(d->right_menu), button, activate_time);
	instance = NULL;
}

/* handler for the "Play" menu item */
void menu_item_on_play(GtkMenuItem* instance, gpointer app_data) {
	AppData* d = (AppData*) app_data;

	mpd_control(d->sock, "play");
	instance = NULL;
}

/* handler for the "Pause" menu item */
void menu_item_on_pause(GtkMenuItem* instance, gpointer app_data) {
	AppData* d = (AppData*) app_data;

	mpd_control(d->sock, "pause");
	instance = NULL;
}

void menu_item_on_next(GtkMenuItem* instance, gpointer app_data) {
	AppData* d = (AppData*) app_data;

	mpd_control(d->sock, "next");
	instance = NULL;
}

void menu_item_on_prev(GtkMenuItem* instance, gpointer app_data) {
	AppData* d = (AppData*) app_data;

	mpd_control(d->sock, "previous");
	instance = NULL;
}

void menu_item_on_stop(GtkMenuItem* instance, gpointer app_data) {
	AppData* d = (AppData*) app_data;

	mpd_control(d->sock, "stop");
	instance = NULL;
}

void menu_item_on_shuffle(GtkMenuItem* instance, gpointer app_data) {
	AppData* d = (AppData*) app_data;

	mpd_control(d->sock, "shuffle");
	instance = NULL;
}

/* handler for the "About" menu item (see version.h) */
void menu_item_on_about(GtkMenuItem* instance, gpointer unused) {
	GtkAboutDialog* about;
	const gchar* authors [] = {
		PROG_AUTHOR0,
		NULL
	};

	about = about_create (PROG_NAME, PROG_VERSION, PROG_COPYRIGHT,
				PROG_COMMENT, PROG_LICENSE, PROG_WEBSITE,
				authors);
	about_show (about);
	unused = NULL; /* avoid compiler warnings */
	instance = NULL; /* _ */
	return;
}

/* callback on Preferences window closed */
void pref_window_on_close (gpointer app_data) {
	AppData* d = (AppData*) app_data;

	g_free(d->ip);
	d->ip = g_strdup (gtk_entry_get_text(d->ip_entry));

	d->port = gtk_spin_button_get_value_as_int (d->port_spin);
	
	/* write it to $HOME/.fala */
	write_config (d);
	gtk_widget_hide (GTK_WIDGET(d->pref_window));
	gtk_widget_destroy (GTK_WIDGET(d->pref_window));
	gtk_widget_set_sensitive (GTK_WIDGET(d->pref_item), TRUE);
	d->pref_window = NULL;
	mpd_disconnect (d->sock);
	d->sock = -1;
	d->sock = mpd_connect (d->ip, d->port);
}

/* handler for the "Preference" menu item */
void menu_item_on_pref(GtkMenuItem* instance, gpointer app_data) {
	AppData* d = (AppData*) app_data;
	GtkVBox* vbox;
	GtkHBox* hbox1;
	GtkHBox* hbox2;
	GtkHBox* hbox3;
	GtkLabel* ip_label;
	GtkLabel* port_label;
	GtkLabel* empty_label;
	GtkButton* close_button;

	d->pref_window = GTK_WINDOW(gtk_window_new (GTK_WINDOW_TOPLEVEL));
	gtk_window_set_title (GTK_WINDOW(d->pref_window), _("Preferences"));
	gtk_window_set_default_size (GTK_WINDOW(d->pref_window), 300, 200);
	gtk_window_set_resizable (GTK_WINDOW(d->pref_window), TRUE);

	vbox = GTK_VBOX(gtk_vbox_new (TRUE, 0));
	gtk_container_add (GTK_CONTAINER(d->pref_window), GTK_WIDGET(vbox));

	hbox1 = GTK_HBOX(gtk_hbox_new (TRUE, 0));
	gtk_box_pack_start (GTK_BOX(vbox), GTK_WIDGET(hbox1), TRUE, FALSE, 10);

	hbox2 = GTK_HBOX(gtk_hbox_new (TRUE, 0));
	gtk_box_pack_start (GTK_BOX(vbox), GTK_WIDGET(hbox2), TRUE, FALSE, 10);

	hbox3 = GTK_HBOX(gtk_hbox_new (TRUE, 0));
	gtk_box_pack_start (GTK_BOX(vbox), GTK_WIDGET(hbox3), TRUE, FALSE, 10);

	ip_label = GTK_LABEL(gtk_label_new (_("MPD IP Address:")));
	gtk_box_pack_start (GTK_BOX(hbox1), GTK_WIDGET(ip_label), TRUE, FALSE, 10);
	port_label = GTK_LABEL(gtk_label_new (_("MPD TCP Port:")));
	gtk_box_pack_start (GTK_BOX(hbox2), GTK_WIDGET(port_label), TRUE, FALSE, 10);

	d->ip_entry = GTK_ENTRY(gtk_entry_new ());
	gtk_entry_set_max_length (d->ip_entry, 15);
	gtk_entry_set_width_chars (d->ip_entry, 15);
	gtk_entry_set_text (d->ip_entry, d->ip); /* MPD ip address */
	gtk_box_pack_start( GTK_BOX(hbox1), GTK_WIDGET(d->ip_entry), TRUE, FALSE, 10);
	d->port_spin = GTK_SPIN_BUTTON(gtk_spin_button_new_with_range (1, 65535, 1));
	gtk_spin_button_set_value (d->port_spin, (gdouble)d->port); /* MPD tcp port */
	gtk_box_pack_start (GTK_BOX(hbox2), GTK_WIDGET(d->port_spin), TRUE, FALSE, 10);

	empty_label = GTK_LABEL(gtk_label_new (""));
	gtk_box_pack_start (GTK_BOX(hbox3), GTK_WIDGET(empty_label), TRUE, FALSE, 10);

	close_button = GTK_BUTTON(gtk_button_new_from_stock (GTK_STOCK_CLOSE));
	gtk_box_pack_start (GTK_BOX(hbox3), GTK_WIDGET(close_button), FALSE, FALSE, 10);

	g_signal_connect_swapped (G_OBJECT(close_button), "clicked", G_CALLBACK(pref_window_on_close), (gpointer)d);
	g_signal_connect_swapped (G_OBJECT(d->pref_window), "delete-event", G_CALLBACK(pref_window_on_close), (gpointer)d);

	gtk_widget_show_all (GTK_WIDGET(d->pref_window));
	gtk_widget_set_sensitive (GTK_WIDGET(instance), FALSE);
}

/* handler for the "Quit" menu item */
void menu_item_on_quit(GtkMenuItem* instance, gpointer app_data) {
	AppData* d = (AppData*) app_data;
	quit (d);
	instance = NULL; /* useless but does not warn at compile time */
}

