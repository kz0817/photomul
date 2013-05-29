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

#include <cstdlib>
#include <gtk/gtk.h>

static gboolean
delete_event(GtkWidget *widget, GdkEvent *event, gpointer user_data) 
{
	gtk_main_quit();
	return TRUE;
}

int main(int argc, char *argv[])
{
	gtk_init(&argc, &argv);
	GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_widget_show(window);

	g_signal_connect(window, "delete-event",
	                 G_CALLBACK( delete_event), NULL);
	gtk_main();
	return EXIT_SUCCESS;
}
