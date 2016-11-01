/*
Copyright (C) 2016  Benoît Morgan

This file is part of libcinder.

libcinder is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

libcinder is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with libcinder.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef __DB_H__
#define __DB_H__

#include <cinder/cinder.h>

#define DB_FILENAME "cinder.db"

int db_init(void);
int db_cleanup(void);

int db_delete_person(const char *pid);
int db_update_match(const struct cinder_match *m);

#endif//__DB_H__