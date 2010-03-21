#include <stdio.h>
#include <gtk/gtk.h>

int main(int argc, char **argv)
{
	GtkWidget *window;
	GtkBuilder *builder;
	GtkWidget *vbox;

	gtk_init (&argc, &argv);

	window = gtk_window_new (GTK_WINDOW_TOPLEVEL);

	builder = gtk_builder_new();
	gtk_builder_add_from_file (builder, "nautilus-ideviceinfo.ui", NULL);
	gtk_builder_connect_signals (builder, NULL);

	vbox = GTK_WIDGET(gtk_builder_get_object(builder, "ideviceinfo"));

	gtk_container_add(GTK_CONTAINER(window), vbox);

	g_signal_connect (window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

	gtk_widget_show_all(window);

	gtk_main();

	return 0;
}
