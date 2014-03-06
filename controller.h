#ifndef CONTROLLER_H
#define CONTROLLER_H
typedef struct {
	gchar* config_path;
	gchar* ip;	/* MPD ip address */
	gint port;	/* MPD tcp port */
	int sock;
	GtkMenu* right_menu;
	GtkStatusIcon* status_icon;
	GtkWindow* pref_window;
	GtkEntry* ip_entry;
	GtkSpinButton* port_spin;
	GtkMenuItem* pref_item;
	GtkWidget* volume;
	GtkWidget* vol_window;
} AppData;

void quit(AppData* d);
void read_config (AppData* d);
void write_config (AppData* d);
int mpd_connect (const char* host, int port);
void mpd_disconnect (int sockfd);
gboolean mpd_control (int sockfd, const gchar* ctl);
gint mpd_get_vol(int sockfd);
gchar* mpd_get_song(int sockfd);
#endif
