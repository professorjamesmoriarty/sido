/*     sido.c
 *     
 *     Copyright 2009 Benoit Rouits <brouits@free.fr>
 *     
 *     This program is free software; you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation; either version 2 of the License, or
 *     (at your option) any later version.
 *     
 *     This program is distributed in the hope that it will be useful,
 *     but WITHOUT ANY WARRANTY; without even the implied warranty of
 *     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *     GNU General Public License for more details.
 *    
 *     You should have received a copy of the GNU General Public License
 *     along with this program; if not, write to the Free Software
 *     Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 *     MA 02110-1301, USA.
 */

#include <stdlib.h>
#include <glib.h>
#include <gtk/gtk.h>
#include <libintl.h>
#include <locale.h>
#include "callbacks.h"
#include "controller.h"
#include "menu.h"
#include "version.h"
#define _(string) gettext (string)

int main(int argc, char** argv)
{
	GtkStatusIcon* status_icon = NULL;
	GtkMenu* right_menu = NULL;
	const gchar* const * data_dirs;
	gchar* icon_path = NULL;
	AppData* app_data = NULL;
	int i;
	
	app_data = (AppData*) malloc (sizeof(AppData));
	app_data->config_path = NULL;
	app_data->ip = g_strdup("127.0.0.1");
	app_data->port = 6600;
	app_data->sock = -1;
	app_data->right_menu = NULL;
	app_data->status_icon = NULL;
	app_data->pref_window = NULL;
	app_data->ip_entry = NULL;
	app_data->port_spin = NULL;
	app_data->pref_item = NULL;
	app_data->volume = NULL;
	app_data->vol_window = NULL;

	/*  internationalization */
	setlocale ( LC_ALL, "" );
	bindtextdomain ("sido", LOCALEDIR);
	textdomain ( "sido" );

	/* set $HOME/.sido as config file */
	app_data->config_path = g_build_path ("/", g_get_home_dir(), ".sido", NULL);
	read_config(app_data);

	gtk_init(&argc, &argv);

	/* search for status icon */	
	data_dirs = g_get_system_data_dirs();
	for (i = 0; data_dirs[i]; ++i) {
		icon_path = g_strconcat (data_dirs[i],"/icons/sido.png", NULL);
		if (g_file_test (icon_path, G_FILE_TEST_EXISTS)) {
			break;
		}
		g_free(icon_path);
		icon_path = NULL;
	}
	if (!icon_path) {
		g_error("no icon found");
		return 1;
	}

	/* create right-click menu */
	right_menu = menu_new();
	app_data->right_menu = right_menu;

	status_icon = gtk_status_icon_new();
	app_data->status_icon = status_icon;
	gtk_status_icon_set_from_file(status_icon, icon_path);
	gtk_window_set_default_icon_from_file(icon_path, NULL);
	g_free(icon_path);
	gtk_status_icon_set_tooltip(status_icon, "Sido");
	gtk_status_icon_set_visible(status_icon, TRUE);
	
        g_signal_connect(G_OBJECT(status_icon), "activate", 
                         G_CALLBACK(status_icon_on_left_click), app_data);
	g_signal_connect(G_OBJECT(status_icon), "popup-menu",
                         G_CALLBACK(status_icon_on_right_click), app_data);

	/* right menu items callbacks */
	menu_append_image_item(right_menu, GTK_STOCK_MEDIA_PLAY, G_CALLBACK(menu_item_on_play), app_data);
	menu_append_image_item(right_menu, GTK_STOCK_MEDIA_PAUSE, G_CALLBACK(menu_item_on_pause), app_data);
	menu_append_image_item(right_menu, GTK_STOCK_MEDIA_NEXT, G_CALLBACK(menu_item_on_next), app_data);
	menu_append_image_item(right_menu, GTK_STOCK_MEDIA_PREVIOUS, G_CALLBACK(menu_item_on_prev), app_data);
	menu_append_image_item(right_menu, GTK_STOCK_MEDIA_STOP, G_CALLBACK(menu_item_on_stop), app_data);
	menu_append_item(right_menu, _("Shuffle"), G_CALLBACK(menu_item_on_shuffle), app_data);
	app_data->pref_item = menu_append_image_item(right_menu, GTK_STOCK_PREFERENCES, G_CALLBACK(menu_item_on_pref), app_data);
	menu_append_image_item(right_menu, GTK_STOCK_ABOUT, G_CALLBACK(menu_item_on_about), app_data);
	menu_append_image_item(right_menu, GTK_STOCK_QUIT, G_CALLBACK(menu_item_on_quit), app_data);

	app_data->sock = mpd_connect(app_data->ip, app_data->port);
	g_timeout_add_seconds(3, tooltip_display, (gpointer)app_data);

	gtk_main();

	mpd_disconnect(app_data->sock);
	free (app_data);
	return 0;
}
