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

#include "match_list.h"
#include "match.h"
#include "user.h"

extern GListStore *matches;
extern GListStore *recs;
extern GListStore *messages;
extern GListStore *notes;
extern Match *selected;
extern User *user;

void model_init(void);
void model_populate(void);
void model_cleanup(void);

#endif//__MODEL_H__
