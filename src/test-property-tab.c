#include <gtk/gtk.h>
#include "ideviceinfo-property-page.h"

static gboolean
delete_event (GtkWidget *widget)
{
	gtk_main_quit ();

	return TRUE;
}

int
main (int argc, char **argv)
{
	GtkWidget *window, *tab;
	char *uuid, *uri, *path;
	GFile *file;

	g_thread_init (NULL);
	gdk_threads_init ();
	gtk_init (&argc, &argv);

	if (argc != 2) {
		g_message ("Usage: %s <UUID>", argv[0]);
		return 1;
	}
	uuid = argv[1];
	uri = g_strdup_printf ("afc://%s/", uuid);
	file = g_file_new_for_uri (uri);
	g_free (uri);
	path = g_file_get_path (file);
	g_object_unref (file);

	window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	g_signal_connect (G_OBJECT (window), "destroy-event",
			  G_CALLBACK (delete_event), NULL);
	g_signal_connect (G_OBJECT (window), "delete-event",
			  G_CALLBACK (delete_event), NULL);
	tab = nautilus_ideviceinfo_page_new (uuid, path);
	gtk_container_add (GTK_CONTAINER (window), tab);
	gtk_widget_show_all (window);

	gtk_main ();

	gtk_widget_destroy (window);

	return 0;
}
