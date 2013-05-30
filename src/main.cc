/* photomul
   Copyright (C) 2013 Kazuhiro Yamato
 
   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <string>
#include <cstdlib>
#include <gtk/gtk.h>
#include "Controller.h"

using namespace std;

static gboolean
delete_event(GtkWidget *widget, GdkEvent *event, gpointer user_data) 
{
	gtk_main_quit();
	return TRUE;
}

int main(int argc, char *argv[])
{
	gtk_init(&argc, &argv);

	// parse argument
	string path;
	for (int i = 1; i < argc; i++)
		path = argv[i];

	// make a window
	GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	g_signal_connect(window, "delete-event",
	                 G_CALLBACK(delete_event), NULL);

	Controller controller;
	if (!path.empty())
		controller.set_path(path);
	GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	gtk_box_pack_start(GTK_BOX(hbox), controller.get_widget(),
	                   TRUE, TRUE, 0);
	gtk_container_add(GTK_CONTAINER(window), hbox);
	gtk_widget_show_all(window);
	gtk_main();
	return EXIT_SUCCESS;
}
