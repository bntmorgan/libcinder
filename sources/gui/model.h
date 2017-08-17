/*
Copyright (C) 2016  Benoît Morgan

This file is part of libpickup.

libpickup is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

libpickup is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with libpickup.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef __MODEL_H__
#define __MODEL_H__

#include <gio/gio.h>

#include <pickup/pickup.h>

typedef struct {
  GObject parent;
  struct pickup_match m;
} MatchList;

typedef struct {
  GObjectClass parent_class;
} MatchListClass;

extern GListStore *matches;

void model_init(void);
void model_populate(void);
void model_destroy(void);

#endif//__MODEL_H__
