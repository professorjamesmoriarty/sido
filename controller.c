#include <stdlib.h>
#include <stdio.h>
#include <glib.h>
#include <gtk/gtk.h>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <string.h>
#include "controller.h"

/* since we use a persistent connection, we must use a mutex
 * as we use the same socket for parallel queres 
 */
static GStaticMutex TALKING = G_STATIC_MUTEX_INIT;

/* connects and returns an open socket */
int mpd_connect (const char* host, int port) {
	int sockfd, ret;
	struct sockaddr_in sock_addr;
	struct timeval tv;
    	int yes = 1;
	char reply[16]; /* enough for MPD OK 99.99.99 */

	tv.tv_sec = 1;
	tv.tv_usec = 0;

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (-1 == sockfd)
		return -1;
    	setsockopt (sockfd, SOL_SOCKET, SO_REUSEADDR, (void*)&yes, sizeof(yes));
    	setsockopt (sockfd, SOL_SOCKET, SO_SNDTIMEO, (void*)&tv, sizeof(tv));
    	setsockopt (sockfd, SOL_SOCKET, SO_RCVTIMEO, (void*)&tv, sizeof(tv));
#if defined(TCP_NODELAY) && (defined(IPPROTO_TCP) || defined(SOL_TCP))
 #ifdef SOL_TCP
	setsockopt(sockfd,SOL_TCP,TCP_NODELAY,(void *)&yes,sizeof(yes));
 #else
  #ifdef IPPROTO_TCP
	setsockopt(sockfd,IPPROTO_TCP,TCP_NODELAY,(void *)&yes,sizeof(yes));
  #endif
 #endif
#endif
	sock_addr.sin_family = AF_INET;
	sock_addr.sin_port = htons(port);
	sock_addr.sin_addr.s_addr = inet_addr(host);
	memset(&(sock_addr.sin_zero), '\0', 8);

	ret = connect(sockfd, (struct sockaddr *)&sock_addr,
	              sizeof(struct sockaddr));
	if (-1 == ret)
		return -1;
	/* read the MPD hello messsage */	
	ret = recv (sockfd, reply, 16, MSG_WAITALL); /* should be MSG_WAITALL */
	if (-1 == ret) {
		close (sockfd);
		return -1;
	} else {
		return sockfd;
	}
}

/* disconnect from MPD */
void mpd_disconnect (int sockfd) {

	if (sockfd == -1)
		return;
	g_static_mutex_lock(&TALKING);
	send(sockfd, "close\n", 6, 0);
	close(sockfd);
	g_static_mutex_unlock(&TALKING);
}

/* send arbitrary command to MPD */
gboolean mpd_control(int sockfd, const gchar *ctl) {
	gchar* msg = NULL;
	int ret;

	if (sockfd == -1) {
		return FALSE;
	}

	msg = g_strdup_printf("%s\n", ctl);
	ret = send(sockfd, msg, strlen(msg), 0);
	g_free(msg);
	if (-1 == ret)
		return FALSE;
	return TRUE;
}

/* get current song title and artist */
gchar* mpd_get_song(int sockfd) {
	int i;
	char* buf = malloc(BUFSIZ);
	char* b1 = NULL;
	char* b2 = NULL;
	char* dsp = NULL;
	char* title = NULL;
	char* artist = NULL;
	int len = 0;

	if (sockfd == -1) {
		return g_strdup("not connected to MPD");
	}
	memset(buf, '\0', BUFSIZ);
	g_static_mutex_lock(&TALKING);
	/* send command */
	len = send(sockfd, "currentsong\n", 12, 0);
	if (-1 == len) {
		g_static_mutex_unlock(&TALKING);
		return g_strdup("MPD not responding");
	}
	while ( -1 != (len = recv (sockfd, buf, BUFSIZ, 0))) { 
		/* should be MSG_WAITALL though */
		if (!strcmp(buf, "OK\n")) { /* empty response */
			g_static_mutex_unlock(&TALKING);
			return g_strdup("MPD not playing");
		}
		/* else find artist an title */
		if((b2 = strstr (buf, "Artist:"))) {
			b1 = strstr(b2, ":")+2; /* skip ': ' */
			if (!b1)
				b1 = &buf[len-1];
			b2 = strstr(b1, "\n");
			if (!b2)
				b2 = &buf[len-1];
			artist = malloc (b2-b1+1);
			for (i = 0; i < b2-b1; ++i)
				artist[i] = b1[i];
			artist[b2-b1] = '\0';
		}
		if((b2 = strstr (buf, "Title:"))) {
			b1 = strstr(b2, ":")+2; /* skip ': ' */
			if (!b1)
				b1 = &buf[len-1];
			b2 = strstr(b1, "\n");
			if (!b2)
				b2 = &buf[len-1];
			title = malloc (b2-b1+1);
			for (i = 0; i< b2-b1; ++i)
				title[i] = b1[i];
			title[b2-b1] = '\0';
		}
		if((b2 = strstr (buf, "\nOK"))) break; /* end of reponse */
	}
	/* FIXME if artist = NULL : no message */
	dsp = g_strconcat(artist,": ", title, NULL);
	free (artist);
	free (title);
	g_static_mutex_unlock(&TALKING);
	return dsp;
}

/* get volume */
gint mpd_get_vol(int sockfd) {
	int ret, vol = 0;
	char buf [BUFSIZ];
	char* b1 = NULL;

	if (sockfd == -1) {
		return 0;
	}

	memset(buf, '\0', BUFSIZ);
	g_static_mutex_lock(&TALKING);
	ret = send(sockfd, "status\n", 7, 0);
	if (-1 == ret) {
		g_static_mutex_unlock(&TALKING);
		return 0;
	}
	while (-1 != (ret = recv(sockfd, buf, BUFSIZ, 0))) {
		if (11 > strlen(buf)) { /* get enough bytes for volume: xxx */
			continue;
		}
		b1 = strstr(buf, "volume: ");
		if (b1) {
			b1 += strlen("volume: "); /* position after ': ' */
			if ((b1-buf) < BUFSIZ) /* avoid reading outside */
				vol = atoi(b1);
		}
		if (strstr(buf, "\nOK")) break; /* end of response reached */
	}
	g_static_mutex_unlock(&TALKING);
	return vol;
}

/* free everything and quit gtk+ loop */
void quit(AppData* d) {
	g_free(d->config_path);
	g_free(d->ip);
	gtk_main_quit();
}

/* read configuration file */
void read_config (AppData* d) {
	GKeyFile* kf = NULL;
	GError* error = NULL;

	kf = g_key_file_new();
	g_key_file_load_from_file(kf, d->config_path, G_KEY_FILE_NONE, &error);
	if(error) {
		g_free(d->ip);
		d->ip = g_strdup("127.0.0.1");
		d->port = 6600;
		write_config(d);
		g_error_free(error);
		error = NULL;
		g_key_file_load_from_file(kf, d->config_path, G_KEY_FILE_NONE, &error);
	}
	d->ip = g_strdup(g_key_file_get_value (kf, "mpd", "ip", &error));
	if(error) {
		g_warning("read_config: %s\n", error->message);
		g_error_free(error);
		error = NULL;
	}
	d->port = g_key_file_get_integer(kf, "mpd", "port", &error);
	if(error) {
		g_warning("read_config: %s\n", error->message);
		g_error_free(error);
		error = NULL;
	}
	g_key_file_free(kf);
	if (!d->ip || d->ip[0] == '\0') {
		g_free(d->ip);
		d->ip = g_strdup("127.0.0.1");
	}
	if (!d->port) {
		d->port = 6600;
	}
}

/* write configuration to file */
void write_config (AppData* d) {
	GKeyFile* kf = NULL;
	gchar* data = NULL;
	gsize len;
	FILE* fp = NULL;
	
	kf = g_key_file_new();
	g_key_file_set_value (kf, "mpd", "ip", d->ip);
	g_key_file_set_integer (kf, "mpd", "port", d->port);
	data = g_key_file_to_data(kf, &len, NULL);
	fp = fopen(d->config_path, "w");
	/* FIXME: check error (disk full, e.g.) */
	fputs(data, fp);
	fclose(fp);
	g_free(data);
}

