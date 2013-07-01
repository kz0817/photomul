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

#include "Utils.h"

namespace Utils {

string get_path(GFile *gfile)
{
	char *path_cstr = g_file_get_path(gfile);
	string path = path_cstr;
	g_free(path_cstr);
	return path;
}

string format(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	gchar *dup = g_strdup_vprintf(fmt, ap);
	va_end(ap);
	string str = dup;
	g_free(dup);
	return str;
}

} // namespace Utils
