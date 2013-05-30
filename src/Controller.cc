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

#include "Controller.h"

// ----------------------------------------------------------------------------
// Public methods
// ----------------------------------------------------------------------------
Controller::Controller(void)
: m_widget(NULL)
{
}

Controller::~Controller()
{
}

GtkWidget *Controller::get_widget(void)
{
	if (!m_widget) {
		m_widget = gtk_event_box_new();
		gtk_widget_set_can_focus(m_widget, TRUE); 
		connect_signals();
	}
	return m_widget;
}

// ----------------------------------------------------------------------------
// Private methods
// ----------------------------------------------------------------------------
void Controller::connect_signals(void)
{
	g_signal_connect(m_widget, "key-press-event",
	                 G_CALLBACK(_key_press_event), this);
}

gboolean Controller::_key_press_event(GtkWidget *widget, GdkEvent *event,
                                      gpointer user_data)
{
	guint keyval;
	gboolean succeeded = gdk_event_get_keyval(event, &keyval);
	if (!succeeded) {
		g_warning("Failed to get key value");
		return FALSE;
	}
	g_debug("Key press event: %u", keyval);
	return TRUE;
}
