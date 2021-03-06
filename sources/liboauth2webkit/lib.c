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

#include <stdlib.h>
#include <gtk/gtk.h>
#include <webkit2/webkit2.h>
#include <string.h>
#include <pcre.h>

#include "oauth2webkit/oauth2webkit.h"
#include "log.h"

struct _oauth2_context {
  // Common
  char *access_token; // Allocated by the user in the sync context and by the
                      // lib in the async
  const char *url_confirm; // allocated by the user
  int error_code;
  int async; // does the lib has to manage init and qui Gtk and or not
             // in addition is the processing synchronized or asynchronized
  // Async in GtkContext
  oauth2_cb cb;
  // Private (not visible in public header) do not want to spread GTK+ symbols
  GtkWidget *window;
  WebKitWebView *view;
};

static void destroy_window(GtkWidget* widget, GtkWidget* window, gpointer
    user_data);
static void resource_load_started(WebKitWebView *web_view, WebKitWebResource
    *resource, WebKitURIRequest *request, struct _oauth2_context *ctx);

static int *oauth2_argc = NULL;
static char ***oauth2_argv = NULL;

void oauth2_init(int *argc, char ***argv) {
  oauth2_argc = argc;
  oauth2_argv = argv;
}

static void load_changed_cb(WebKitWebView *web_view, WebKitLoadEvent load_event,
    void *user_data) {
  if (load_event == WEBKIT_LOAD_FINISHED) {
    DEBUG("Page loaded\n");
  }
}

/**
 * Creates a new window in a already existing Gtk execution context (only in
 * main thread)
 * The calls back cb with the execution error code and access_token if any
 * Access_token is allocated in stack. You have to strdup it !
 */
int oauth2_get_access_token_async(const char *url, const char *url_confirm,
    oauth2_cb cb) {
  struct _oauth2_context *ctx;

  if (url ==  NULL || url_confirm == NULL) {
    return 1;
  }

  ctx = malloc(sizeof(struct _oauth2_context));
  if (ctx == NULL) {
    return OAUTH2_NO_MEM;
  }

  // Create an 800x600 window that will contain the browser instance
  GtkWidget *main_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_default_size(GTK_WINDOW(main_window), 800, 600);

  // Create a browser instance
  WebKitWebView *web_view = WEBKIT_WEB_VIEW(webkit_web_view_new());

  // Initialize context
  memset(ctx, 0, sizeof(struct _oauth2_context));
  ctx->async = 1;
  ctx->error_code = OAUTH2_OK;
  ctx->url_confirm = url_confirm;
  ctx->cb = cb;
  ctx->view = web_view;
  ctx->window = main_window;

  // Put the browser area into the main window
  gtk_container_add(GTK_CONTAINER(main_window), GTK_WIDGET(web_view));

  g_signal_connect(web_view, "resource-load-started",
      G_CALLBACK(resource_load_started), ctx);

  g_signal_connect(web_view, "load-changed", G_CALLBACK(load_changed_cb), NULL);

  // Load a web page into the browser instance
  webkit_web_view_load_uri(web_view, url);

  // Make sure that when the browser area becomes visible, it will get mouse
  // and keyboard events
  gtk_widget_grab_focus(GTK_WIDGET(web_view));

  // Make sure the main window and all its contents are visible
  gtk_widget_show_all(main_window);

  return 0;
}

/**
 * Creates a new GTK context and window.
 * This fonctions locks main thread in the gtk_main() loop
 * Access_token is written in the access_token parameter allocated by the user
 */
int oauth2_get_access_token(const char *url, const char *url_confirm, char
    *access_token) {
  struct _oauth2_context ctx;

  if (url ==  NULL || access_token == NULL) {
    return 1;
  }

  // Initialize context
  memset(&ctx, 0, sizeof(struct _oauth2_context));
  ctx.error_code = OAUTH2_OK;
  ctx.access_token = access_token;
  ctx.url_confirm = url_confirm;

  // Initialize GTK+
  gtk_init(oauth2_argc, oauth2_argv);

  // Create an 800x600 window that will contain the browser instance
  GtkWidget *main_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_default_size(GTK_WINDOW(main_window), 800, 600);

  // Create a browser instance
  WebKitWebView *web_view = WEBKIT_WEB_VIEW(webkit_web_view_new());

  // Put the browser area into the main window
  gtk_container_add(GTK_CONTAINER(main_window), GTK_WIDGET(web_view));

  // Set up callbacks so that if either the main window or the browser instance
  // is closed, the program will exit
  g_signal_connect(main_window, "destroy", G_CALLBACK(destroy_window), &ctx);

  g_signal_connect(web_view, "resource-load-started",
      G_CALLBACK(resource_load_started), &ctx);

  // Load a web page into the browser instance
  webkit_web_view_load_uri(web_view, url);

  // Make sure that when the browser area becomes visible, it will get mouse
  // and keyboard events
  gtk_widget_grab_focus(GTK_WIDGET(web_view));

  // Make sure the main window and all its contents are visible
  gtk_widget_show_all(main_window);

  // Run the main GTK+ event loop
  gtk_main();

  return ctx.error_code;
}

static void destroy_window(GtkWidget* widget, GtkWidget* window, gpointer
    user_data) {
  struct _oauth2_context *ctx = user_data;
  ctx->error_code = OAUTH2_USER_CLOSED;
  gtk_main_quit();
}

static int parse_result(const char *data, char *access_token) {
  pcre *re_comp;
  char *re_reg = "access_token=([^&]*)";
  const char *re_err_str;
  int re_err_offset;
  pcre_extra *re_extra;
  int re_ret;
  int re_sub_str_ret[30] = {};

  re_comp = pcre_compile(re_reg, 0, &re_err_str, &re_err_offset, NULL);
  if (re_comp == NULL) {
    fprintf(stderr, "ERROR: Failed to build the regular expression %s: %s\n",
        re_reg, re_err_str);
    exit(1);
  }

  re_extra = pcre_study(re_comp, 0, &re_err_str);
  if (re_err_str != NULL) {
    fprintf(stderr, "ERROR: Could not study '%s': %s\n", re_reg, re_err_str);
    exit(1);
  }

  re_ret = pcre_exec(re_comp, re_extra, data, strlen(data), 0, 0,
      re_sub_str_ret, 30);

  if(re_ret < 0) {
    switch(re_ret) {
      case PCRE_ERROR_NOMATCH :
        fprintf(stderr, "String did not match the pattern\n");
        break;
      case PCRE_ERROR_NULL :
        fprintf(stderr, "Something was null\n");
        break;
      case PCRE_ERROR_BADOPTION :
        fprintf(stderr, "A bad option was passed\n");
        break;
      case PCRE_ERROR_BADMAGIC :
        fprintf(stderr, "Magic number bad (compiled re corrupt?)\n");
        break;
      case PCRE_ERROR_UNKNOWN_NODE :
        fprintf(stderr, "Something kooky in the compiled re\n");
        break;
      case PCRE_ERROR_NOMEMORY :
        fprintf(stderr, "Ran out of memory\n");
        break;
      default:
        fprintf(stderr, "Unknown error\n");
        break;
    }
    exit(1);
  } else {
    strncpy(access_token, &data[re_sub_str_ret[2]], re_sub_str_ret[3] -
        re_sub_str_ret[2]);
    access_token[re_sub_str_ret[3] - re_sub_str_ret[2]] = '\0';
    NOTE("access_token %s\n", access_token);
  }
  return 0;
}

static void finish(struct _oauth2_context *ctx) {
  if (ctx->async == 0) {
    // We exit Gtk main loop
    gtk_main_quit();
  } else {
    // we call the callback
    ctx->cb((struct oauth2_context *)ctx);
    free(ctx);
    // We close the window
    gtk_widget_destroy(ctx->window);
  }
}

static void process_data(char *data, struct _oauth2_context *ctx) {
  // In the sync context, there is no callback, we can close the gtk window
  if (ctx->async == 0) {
    ctx->error_code = parse_result(data, ctx->access_token);
  } else {
    char access_token[0x1000];
    ctx->access_token = &access_token[0];
    ctx->error_code = parse_result(data, ctx->access_token);
  }
}

static void get_data_finished(WebKitWebResource *resource, GAsyncResult *result,
    struct _oauth2_context *ctx) {
  guchar *data;
  gsize data_length;
  GError *error = NULL;
  WebKitURIResponse *uri_resp = webkit_web_resource_get_response(resource);
  int http_error_code = webkit_uri_response_get_status_code(uri_resp);
  int http_content_length = webkit_uri_response_get_content_length(uri_resp);
  const char *uri = webkit_uri_response_get_uri(uri_resp);
  DEBUG("HTTP Error code(%d), content length(0x%08x), uri(%s)\n",
      http_error_code, http_content_length, uri);
  data = webkit_web_resource_get_data_finish(resource, result,
      &data_length, &error);
  DEBUG("Data %p, lenght(0x%08x)\n", data, data_length);
  DEBUG("Error %p\n", error);
  if (error != NULL) {
    ERROR("%s\n", error->message);
    g_error_free(error);
    ctx->error_code = OAUTH2_NETWORK;
  // Sometime the downloaded document is empty...
  } else if (data == NULL) {
    ERROR("Error while getting ressource data : there is no data\n");
    ctx->error_code = OAUTH2_NO_DATA;
  } else {
    process_data((char *)data, ctx);
  }
  // End sync and async usages
  finish(ctx);
}

static void resource_load_failed(WebKitWebResource *resource, GError *error,
    gpointer user_data) {
  const gchar *uri = webkit_web_resource_get_uri(resource);
  ERROR("Failed to get resource datai %s\n", uri);
  if (error != NULL) {
    ERROR("reason: %s", error->message);
    g_error_free(error);
  }
}

static void resource_load_finished(WebKitWebResource *resource, struct
    _oauth2_context *ctx) {
  const gchar *uri = webkit_web_resource_get_uri(resource);
  DEBUG("Resource loading finished %s\n", uri);
  webkit_web_resource_get_data(resource, NULL,
      (GAsyncReadyCallback)get_data_finished, ctx);
}

void resource_received_data(WebKitWebResource *resource, guint64 data_length,
    struct _oauth2_context *ctx) {
  DEBUG("Received data 0x%016llx\n", data_length);
}

static void resource_load_started(WebKitWebView *web_view, WebKitWebResource
    *resource, WebKitURIRequest *request, struct _oauth2_context *ctx) {
  const gchar *uri = webkit_uri_request_get_uri(request);

  if (strcmp(uri, ctx->url_confirm) == 0) {
    DEBUG("resource uri : %s\n", uri);
    g_signal_connect(resource, "finished", G_CALLBACK(resource_load_finished),
        ctx);
    g_signal_connect(resource, "received-data",
        G_CALLBACK(resource_received_data), ctx);
    g_signal_connect(resource, "failed", G_CALLBACK(resource_load_failed),
        ctx);
  }
}

void oauth2_log_level(int l) {
  log_level(l);
}
