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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <getopt.h>

#include <cinder/cinder.h>
#include <oauth2webkit/oauth2webkit.h>

#include "api.h"
#include "io.h"
#include "db.h"
#include "log.h"

#define FB_TOKEN_NAME "cinder_fb_token"
#define PID_NAME "cinder_user_pid"
#define TOKEN_NAME "cinder_token"
#define LAST_ACTIVITY_DATE "last_activity_date"

/**
 * Static flags and vars
 */
static int auth = 0;
static char access_token[0x100];
static char pid[CINDER_SIZE_ID];

void cb_message(struct cinder_match *m, void *data) {
  int i;
  NOTE("New message for match %s\n", m->mid);
  cinder_match_print(m);
  for (i = 0; i < m->messages_count; i++) {
    if (db_update_message(&m->messages[i], m->mid) == -1) {
      ERROR("Failed to update insert a new message\n");
    }
  }
  cinder_match_free(m);
}

void cb_match(struct cinder_match *m, void *data) {
  NOTE("Update for match [%s]%s\n", m->pid, m->name);
  db_update_match(m);
  cinder_match_free(m);
}

void cb_rec(struct cinder_match *m, void *data) {
  cinder_match_print(m);
  NOTE("Update for rec[%s]%s\n", m->pid, m->name);
  db_update_rec(m);
  cinder_match_free(m);
}

int user_auth(int *argc, char ***argv) {
  char fb_access_token[0x1000];
  int error_code;

  // We have to auth again
  oauth2_init(argc, argv);
  error_code = oauth2_get_access_token(FB_OAUTH2_URL, FB_OAUTH2_URL_CONFIRM,
      &fb_access_token[0]);

  if (error_code) {
    fprintf(stderr, "Failed to get facebook access token : %d\n", error_code);
    return 1;
  }

  // Save the token
  str_write(FB_TOKEN_NAME, fb_access_token);

  error_code = cinder_auth(fb_access_token, access_token, pid);

  if (error_code) {
    fprintf(stderr, "Failed to get access token : %d\n", error_code);
    return 1;
  }

  // Save the token
  if (str_write(TOKEN_NAME, access_token) != 0) {
    ERROR("Failed to write the access_token to %s\n", TOKEN_NAME);
  }

  // Save the token
  if (str_write(PID_NAME, pid) != 0) {
    ERROR("Failed to write the user pid to %s\n", PID_NAME);
  }

  // Set the tokens if any use if made after
  cinder_set_access_token(access_token, pid);

  return 0;
}

static inline int auth_check(void) {
  if (auth == 0) {
    ERROR("User must authenticate first !\n");
    return -1;
  }
  return 0;
}

/**
 * Options
 */

#define OPT_LIST_POSSIBLE_ARGUMENTS 1

#define OPT_STR "vdqp:"

static struct option long_options[] = {
  {"verbose", no_argument, 0, 'v'},
  {"quiet", no_argument, 0, 'q'},
  {"debug", no_argument, 0, 'd'},
  {"list-possible-arguments", no_argument, 0, OPT_LIST_POSSIBLE_ARGUMENTS},
  {0, 0, 0, 0}
};

/**
 * Commands
 */

int cmd_update(int argc, char **argv) {
  int ret;
  time_t last_activity_date;
  char str[0x100];
  if (auth_check() != 0) {
    return -1;
  }
  struct cinder_updates_callbacks cbu = {
    cb_match,
    cb_message,
  };
  if (str_read(LAST_ACTIVITY_DATE, &str[0], 32) != 0) {
    NOTE("Failed to read last activity date, set it to 0\n");
    last_activity_date = 0;
  } else {
    last_activity_date = atoi(&str[0]);
    NOTE("Last activity was %u\n", last_activity_date);
  }
  ret = cinder_updates(&cbu, NULL, &last_activity_date);
  if (ret != 0) {
    ERROR("Failed to get the updates\n");
  }
  NOTE("Last activity %u\n", (unsigned int)last_activity_date);
  sprintf(&str[0], "%u", (unsigned int)last_activity_date);
  if (str_write(LAST_ACTIVITY_DATE, &str[0]) != 0) {
    ERROR("Failed to write last activity date\n");
  }
  return ret;
}

int cmd_message(int argc, char **argv) {
  struct cinder_match *m;
  char mid[CINDER_SIZE_ID];
  if (auth_check() != 0) {
    return -1;
  }
  if (argc < 2) {
    ERROR("You have to specify the person to send a message !\n");
    return -1;
  }
  if (db_select_match(argv[0], &m) != 0) {
    ERROR("Failed to get the match\n");
    return -1;
  }
  strcpy(&mid[0], m->mid);
  cinder_match_free(m);
  return cinder_message(&mid[0], argv[1]);
}

int cmd_scan(int argc, char **argv) {
  if (auth_check() != 0) {
    return -1;
  }
  struct cinder_recs_callbacks cbr = {
    cb_rec,
  };
  return cinder_recs(&cbr, NULL);
}

int cmd_authenticate(int argc, char **argv) {
  DEBUG("Authenticate the user!\n");
  if (user_auth(&argc, &argv)) {
    ERROR("Failed to authenticate the user !\n");
    return 1;
  }
  return 0;
}

int cmd_print_access_token(int argc, char **argv) {
  if (auth_check() != 0) {
    return -1;
  }
  printf("%s\n", &access_token[0]);
  return 0;
}

int cmd_logout(int argc, char **argv) {
  if (auth_check() != 0) {
    return -1;
  }
  DEBUG("Remove access_token file !\n");
  file_unlink(FB_TOKEN_NAME);
  file_unlink(TOKEN_NAME);
  return 0;
}

void cb_match_list(struct cinder_match *m) {
  printf("[%s]%s\n", m->pid, m->name);
}

int cmd_list(int argc, char **argv) {
  db_select_matches(cb_match_list);
  return 0;
}

void cb_recs_list(struct cinder_match *m) {
  printf("[%s]%s\n", m->pid, m->name);
}

int cmd_list_recs(int argc, char **argv) {
  db_select_recs(cb_recs_list);
  return 0;
}

void cb_swipe_match(struct cinder_match *m, void *data) {
  int *new_match = data;
  *new_match = 1;
  cb_match(m, NULL);
}

int cmd_unlike(int argc, char **argv) {
  unsigned int rl;
  int new_match;
  if (auth_check() != 0) {
    return -1;
  }
  if (argc < 1) {
    ERROR("please select a person\n");
    return -1;
  }
  struct cinder_updates_callbacks cbu = {
    cb_swipe_match,
  };
  if (cinder_swipe(argv[0], 0, &rl, &cbu, &new_match) != 0) {
    ERROR("Failed to unlike %s\n", argv[0]);
    return -1;
  }
  NOTE("Remaining likes %u, new_match %d\n", rl, new_match);
  // We can remove the recommendation
  if (new_match == 0) {
    if (rl > 0) {
      // We can remove the recommendation
      if (db_delete_person(argv[0]) != 0) {
        ERROR("Failed to delete the recommendation\n");
      }
    }
  }
  return 0;
}

int cmd_like(int argc, char **argv) {
  unsigned int rl;
  int new_match = 0;
  if (auth_check() != 0) {
    return -1;
  }
  if (argc < 1) {
    ERROR("Please select a person\n");
    return -1;
  }
  struct cinder_updates_callbacks cbu = {
    cb_swipe_match,
  };
  if (cinder_swipe(argv[0], 1, &rl, &cbu, &new_match) != 0) {
    ERROR("Failed to unlike %s\n", argv[0]);
    return -1;
  }
  NOTE("Remaining likes %u, new_match %d\n", rl, new_match);
  if (new_match == 0) {
    if (rl > 0) {
      // We can remove the recommendation
      if (db_delete_person(argv[0]) != 0) {
        ERROR("Failed to delete the recommendation\n");
      }
    }
  }
  return 0;
}

int cmd_print(int argc, char **argv) {
  struct cinder_match *m;
  if (argc < 1) {
    ERROR("Please select a person\n");
    return -1;
  }
  if (db_select_match(argv[0], &m) != 0) {
    ERROR("Error accessing the match in database\n");
    return -1;
  }
  cinder_match_print(m);
  cinder_match_free(m);
  return 0;
}

/**
 * Command management
 */

// Commands and callbacks
// thanks to iproute2-4.8.0

int matches(const char *cmd, const char *pattern) {
  int len = strlen(cmd);
  if (len > strlen(pattern)) {
    return -1;
  }
  return memcmp(pattern, cmd, len);
}

static const struct cmd {
  const char *cmd;
  int (*func)(int argc, char **argv);
} cmds[] = {
  {"update", cmd_update},
  {"message", cmd_message},
  {"scan", cmd_scan},
  {"authenticate", cmd_authenticate},
  {"print", cmd_print},
  {"print-access-token", cmd_print_access_token},
  {"list", cmd_list},
  {"list-recs", cmd_list_recs},
  {"like", cmd_like},
  {"unlike", cmd_unlike},
  {"logout", cmd_logout},
  { 0 }
};

static int do_cmd(const char *argv0, int argc, char **argv) {
  const struct cmd *c;

  for (c = cmds; c->cmd; ++c) {
    if (matches(argv0, c->cmd) == 0) {
      DEBUG("Match for %s\n", c->cmd);
      return -(c->func(argc-1, argv+1));
    }
  }

  ERROR("Object \"%s\" is unknown, try \"xml --help\".\n", argv0);
  return -1;
}

int main(int argc, char *argv[]) {
  int c;
  int option_index = 0;

  /**
   * First configuration options
   */
  while (1) {
    c = getopt_long (argc, argv, OPT_STR, long_options, &option_index);

    if (c == -1) {
      break;
    }

    switch (c) {
      case OPT_LIST_POSSIBLE_ARGUMENTS: {
        const struct cmd *c = &cmds[0];
        int i = 0;
        while (c->cmd != NULL) {
          printf("%s\n", c->cmd);
          i++;
          c = &cmds[i];
        }
        return 0;
      }
      case 'q':
        log_level(LOG_LEVEL_NONE);
        cinder_log_level(CINDER_LOG_LEVEL_NONE);
        oauth2_log_level(CINDER_LOG_LEVEL_NONE);
        break;
      case 'v':
        log_level(LOG_LEVEL_NOTE);
        cinder_log_level(CINDER_LOG_LEVEL_NOTE);
        oauth2_log_level(CINDER_LOG_LEVEL_NOTE);
        break;
      case 'd':
        log_level(LOG_LEVEL_DEBUG);
        cinder_log_level(CINDER_LOG_LEVEL_DEBUG);
        oauth2_log_level(CINDER_LOG_LEVEL_DEBUG);
        break;
      case '?':
        /* getopt_long already printed an error message. */
        break;
      default:
        // The second stage getopt will handle it
        NOTE("%d optind\n", optind);
        break;
    }
  }

  /**
   * Then initialize the libraries
   */

  // Init cinder lib
  cinder_init();

  // Init DB connection
  db_init();

  // First ! We get the former access token in your pussy
  if (str_read(TOKEN_NAME, access_token, 0x100)) {
    NOTE("No access token found in dir ~/%s\n", IO_CONFIG_DIR);
  } else {
    NOTE("Access token found is %s\n", &access_token[0]);
    if (str_read(PID_NAME, pid, 0x100)) {
      NOTE("No access token found in dir ~/%s\n", IO_CONFIG_DIR);
    } else {
      NOTE("User pid found is %s\n", &pid[0]);
      // Set the access token and pid
      cinder_set_access_token(access_token, pid);
      auth = 1;
    }
  }

  /**
   * Finally, execute commands
   */

  if (optind < argc) {
    return do_cmd(argv[optind], argc-optind, argv+optind);
  }

  /**
   * Then clean the libraries
   */

  cinder_cleanup();
  db_cleanup();

  return 0;
}
