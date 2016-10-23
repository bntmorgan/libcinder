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

#ifndef __CINDER_H__
#define __CINDER_H__

#include <stdint.h>

enum cinder_message_direction {
  CINDER_MESSAGE_INPUT,
  CINDER_MESSAGE_OUTPUT
};

struct cinder_message {
  enum cinder_message_direction dir;
  char *message;
};

struct cinder_picture {
  char *url;
};

struct cinder_match {
  char *name;
  char *id;
  unsigned short int age;
  unsigned int messages_count;
  struct message *messages;
  unsigned int pictures_count;
  struct picture *pictures;
};

void cinder_set_credentials(const char *access_token);
void cinder_init(void);
void cinder_cleanup(void);
void cinder_fb_login(void);
void test(void);

#endif//__CINDER_H__
