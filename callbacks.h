
void status_icon_on_left_click(GtkStatusIcon* instance, gpointer app_data);
void status_icon_on_right_click(GtkStatusIcon* instance, guint button, guint activate_time, gpointer app_data);
void menu_item_on_play(GtkMenuItem* instance, gpointer app_data);
void menu_item_on_pause(GtkMenuItem* instance, gpointer app_data);
void menu_item_on_next(GtkMenuItem* instance, gpointer app_data);
void menu_item_on_prev(GtkMenuItem* instance, gpointer app_data);
void menu_item_on_stop(GtkMenuItem* instance, gpointer app_data);
void menu_item_on_shuffle(GtkMenuItem* instance, gpointer app_data);
void menu_item_on_pref(GtkMenuItem* instance, gpointer app_data);
void menu_item_on_about(GtkMenuItem* instance, gpointer unused);
void menu_item_on_quit(GtkMenuItem* instance, gpointer app_data);
gboolean tooltip_display(gpointer app_data);
