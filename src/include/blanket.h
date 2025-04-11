//
//  blanket.h
//  libBlanket header
//
//  By Yifan Lu
//  Modified by Clint Harrison to support newer versions (2025-04)
// (Sorry, I'm not sure yet which versions! 5.17 has been tested, but none prior.)
//
//  This program is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <http://www.gnu.org/licenses/>.
//

#pragma once

#include <X11/Xlib.h>
#include <cairo/cairo.h>
#include <pango/pango.h>

#define BLANKET_LEVEL_DEBUG 0x8000
#define BLANKET_LEVEL_INFO 0x800000
#define BLANKET_LEVEL_WARNING 0x1000000
#define BLANKET_LEVEL_ERROR 0x2000000

#define BLANKET_LOG_LEVEL_ENABLED(level) ((g_blanket_llog_mask & level) == level)

#define BL_DEBUG(fmt, ...)                            \
  if (BLANKET_LOG_LEVEL_ENABLED(BLANKET_LEVEL_DEBUG)) \
  syslog(LOG_DEBUG, fmt, __VA_ARGS__)

#define BL_INFO(fmt, ...)                            \
  if (BLANKET_LOG_LEVEL_ENABLED(BLANKET_LEVEL_INFO)) \
  syslog(LOG_INFO, fmt, __VA_ARGS__)

#define BL_WARNING(fmt, ...)                            \
  if (BLANKET_LOG_LEVEL_ENABLED(BLANKET_LEVEL_WARNING)) \
  syslog(LOG_WARNING, fmt, __VA_ARGS__)

#define BL_ERROR(fmt, ...)                            \
  if (BLANKET_LOG_LEVEL_ENABLED(BLANKET_LEVEL_ERROR)) \
  syslog(LOG_ERR, fmt, __VA_ARGS__)

// This struct represents the loaded blanket module.
// Not all of its fields are known, but these are sufficient for a basic screensaver.
//
// A blanket module is expected to have the following exported symbols:
// int init(Module *module, struct Context **ctx);
// int deinit(Module *module, struct Context *ctx);
// extern const int lipcCallbackNum;
// extern const struct LipcCallback lipcCallbacks[];
// extern const struct X11Callback x11Callback;
struct Module
{
  char *name;
  // This is the handle returned by dlopen()
  int32_t dl_handle;
  // This is the symbol for the init function
  void *sym_init;
  // This is the symbol for the deinit function
  void *sym_deinit;
  // This is the array of LIPC callbacks
  struct lipc_event *lipc_callbacks;
  // This is the number of LIPC callbacks
  int32_t lipc_callbacks_num;
  // This is the X11 callback
  struct X11Callback *x11_callback;
  // This is the Kiwi callback
  void *kiwi_callback;

  // This is the name of the window, as last used by blanket
  char *window_name;
  struct Context *ctxp;
  int32_t x11_handler_mutex;
  void *field_2c;
  // There is an int in the X11 callback struct that I can't find a use for.
  // It's always set to 0x8000, except for ad_screensaver, which is 0x800c.
  int32_t x11_callback_param1;
  // This is the function pointer for the X11 callback
  void *x11_handler_func;
  int32_t field_38;
  int32_t field_3c;
  // This is a pointer to the blanket module itself?
  struct Module *self_mod;
  char padding_44[4];
  void *field_48;
  char padding_4c[4];
  int32_t field_50;
  char padding_54[4];
  int32_t field_58;
  char padding_5c[4];
  int32_t field_60;
};

// This struct represents a callback for a given LIPC event.
// Export `LipcCallback[] lipcCallbacks` and `int lipcCallbackNum` in your module.
// This is required by blanket in order for your module to be loaded.
struct LipcCallback
{
  const char *lipc_source;
  const char *lipc_eventName;
  void *lipc_callback;
  // I haven't figured out where these are accessed separately :(
  struct
  {
    void *s;
    void *unk;
  } param;
};

// This struct represents a callback for a given X11 event.
// Not all events seem to be forwarded, but Expose, ButtonPress, and
// ButtonRelease are known to work.
struct X11Callback
{
  uint32_t unknown;
  int (*callback)(Module *module, XEvent *xev, struct Context *ctx);
};

extern "C"
{
  // This can be used to determine the blanket log level
  int g_blanket_llog_mask;

  extern const void *kiwiCallback;
  char *blanket_image_get_asset_name(const char *stem, char *name, const char *type, const char *ext);

  void blanket_image_get_window(struct Module *module, Window *windowp, const char *name, int, int);

  int blanket_image_gettext_draw_defaults(cairo_t *ss_cr, const char *prefix, PangoAlignment alignment, int, void *);
  int blanket_image_gettext_pango_rect(const char *mid, PangoRectangle *rect);

  void blanket_image_progressbar_destroy(void *handle);
  cairo_surface_t *blanket_image_progressbar_get_surface(void *handle);
  int blanket_image_progressbar_init(void **handlep, int width, int height, int border); // TODO: Find struct for handle's type
  void blanket_image_progressbar_update(void *handle, int width);

  Display *blanket_image_screendisplay();
  int blanket_image_screenheight();
  Visual *blanket_image_screenvisual();
  int blanket_image_screenwidth();
  int blanket_image_set_fbdev_mode(int mode);

  int blanket_image_window_bringup(struct Module *module, Window win);
  void blanket_image_window_destroy(struct Module *module, Window subwin);
  void blanket_image_window_teardown(struct Module *module, Window win);

  // Unsure of this one
  int blanket_util_gettext_list(const char *mid, char **values, int max_idx, void *convert_func);
  // Also unsure of this one
  int blanket_util_gettext_value(const char *mid, char **values, void *convert_func);
}
