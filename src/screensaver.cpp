//
//  screensaver.c
//  Open source re-implementation of the screensaver module
//  A reference for custom modules
//
//  By Yifan Lu
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

#include "screensaver.h"
#include <iostream>
#include <openlipc.h>

extern "C"
{
  // blanket requires this exact name
  const int lipcCallbackNum = 5;

  // blanket requires this exact name (lipcCallbacks)
  extern "C" const LipcCallback lipcCallbacks[] = {
      // screensaver defines these (and we want to too)
      {"com.lab126.powerd", "goingToScreenSaver", (void *)&module_screensaver_map_screensaver},
      {"com.lab126.powerd", "userShutdown", (void *)&module_screensaver_map_blank},
      {"com.lab126.powerd", "outOfScreenSaver", (void *)&module_screensaver_unmap},
      {"com.lab126.powerd", "outOfShutdown", (void *)&module_screensaver_unmap},
      {"com.lab126.hal.screensaver", "goingToScreenSaver", (void *)&module_screensaver_map_screensaver},

      // don't care about this one?
      // {"com.lab126.household", "profileGoingToSwitch", (void *)&module_screensaver_profile_switch},

      // @VisibleForTesting vibes
      // {"com.lab126.test.blanket", "goingToScreensaver", (void *)&module_screensaver_map_screensaver},
      // {"com.lab126.test.blanket", "outOfScreensaver", (void *)&module_screensaver_unmap},
      // {"com.lab126.test.blanket", "userShutdown", (void *)&module_screensaver_map_blank},
      // {"com.lab126.test.blanket", "outOfShutdown", (void *)&module_screensaver_unmap},

      // ad screensaver

      // // these are all ~the same
      // {"com.lab126.powerd", "goingToScreenSaver", (void *)&module_adscreensaver_map},
      // //   ok this one's a bit different
      // {"com.lab126.powerd", "outOfScreensaver", (void *)&module_adscreensaver_oss_handler},
      // // more normal
      // {"com.lab126.powerd", "userShutdown", (void *)&module_screensaver_map_blank},
      // {"com.lab126.powerd", "outOfShutdown", (void *)&module_screensaver_unmap_blank},
      // {"com.lab126.hal.screensaver", "goingToScreenSaver", (void *)&module_adscreensaver_map},
      // // is this a Java service?
      // {"com.lab126.adManager", "adScreensaverRotation", (void *)&module_adscreensaver_rotate},
      // // ignore this one again
      // {"com.lab126.household", "profileGoingToSwitch", (void *)&module_adscreensaver_profile_switch},
      // // TODO: what's this for?
      // {"com.lab126.volumd", "driveModeStateChanged", (void *)&module_adscreensaver_oss_handler},
      // // TODO: keep this in mind for caching dashboard too
      // {"com.lab126.volumd", "userstoreAvailable", (void *)&store_next_available_ad},
      // // TODO: should we do something too?
      // {"com.lab126.powerd", "PowerButtonHeld", (void *)&module_adscreensaver_unmap_ad_screensaver},
  };

  const X11Callback x11Callback = {
      // This is 0x8000 for everything except ad_screensaver, which is 0x800c
      .unknown = 0x8000,
      .callback = &module_screensaver_repaint,
  };

  // TODO: If we wanted screen reader support, we'd have to figure this out...
  const void *kiwiCallback = (void *)&module_kiwi_callback;

  int module_kiwi_callback(struct Module *module, struct Context *ctx)
  {
    return 0;
  }

  int init(struct Module *module, struct Context **out_ctxp)
  {
    g_blanket_llog_mask |= BLANKET_LEVEL_DEBUG;

    struct Context *ctx = (struct Context *)calloc(sizeof(struct Context), 1);
    if (ctx == 0)
    {
      BL_ERROR("E %s:OUT_OF_MEMORY::failed to allocate %s", "init", "ctx");
      return 12;
    }

    ctx->field_8 = 2;
    *out_ctxp = ctx;
    ctx->field_0 = 0;
    ctx->type = 0;
    ctx->field_20[0] = 0;
    ctx->field_20[1] = 0;
    ctx->field_20[4] = 0;
    ctx->field_20[5] = 0;
    ctx->field_20[6] = 0;
    ctx->field_20[7] = 0;

    blanket_image_get_window(module, &ctx->window, "blanket_screensaver", 0, 0xFFFFFFFF);
    ctx->surface = cairo_xlib_surface_create(
        blanket_image_screendisplay(),
        ctx->window,
        blanket_image_screenvisual(),
        blanket_image_screenwidth(), blanket_image_screenheight());
    ctx->ss_cr = cairo_create(ctx->surface);

    return 0;
  }

  int deinit(struct Module *module, struct Context *ctx)
  {
    int _errno;
    if ((_errno = module_screensaver_unmap(module, 0, ctx)) != 0)
    {
      BL_ERROR("E screensaver:UNMAP_FAILED:err=%d:%s", _errno, strerror(_errno));
      return _errno;
    }

    blanket_image_window_destroy(module, ctx->window);
    cairo_surface_destroy(ctx->surface);
    cairo_destroy(ctx->ss_cr);
    ctx->surface = NULL;
    ctx->ss_cr = NULL;
    free(ctx);

    return 0;
  }
}

int module_screensaver_unmap(struct Module *module, int unused, struct Context *ctx)
{
  BL_DEBUG("D def:enter:%s:%d", "module_screensaver_unmap", __LINE__);

  if (ctx != NULL)
  {
    blanket_image_window_teardown(module, ctx->window);
  }

  ctx->type = 0;
  ctx->field_0 = 0;

  BL_DEBUG("D def:leave:%s:%d", "module_screensaver_unmap", __LINE__);

  return 0;
}

int module_screensaver_map_screensaver(struct Module *module, int unused, struct Context *ctx)
{
  int _errno;

  BL_DEBUG("D def:enter:%s:%d", "module_screensaver_map_screensaver", __LINE__);

  if ((_errno = module_screensaver_map_generic(module, ctx, 1)) != 0)
  {
    BL_DEBUG("D def:leave:%s:%d", "module_screensaver_map_screensaver", __LINE__);
  }

  return _errno;
}

int module_screensaver_map_blank(struct Module *module, int unused, struct Context *ctx)
{
  int ret;

  BL_DEBUG("D def:enter:%s:%d", "module_screensaver_map_blank", __LINE__);

  ret = module_screensaver_map_generic(module, ctx, 2);

  BL_DEBUG("D def:leave:%s:%d", "module_screensaver_map_blank", __LINE__);

  return ret;
}

int module_screensaver_map_generic(struct Module *module, struct Context *ctx, int type)
{
  int ret;

  BL_DEBUG("D def:enter:%s:%d", "module_screensaver_map_generic", __LINE__);

  if (module == NULL)
  {
    BL_ERROR("E %s:BAD_ARGS::argument is not valid, require %s", "module_screensaver_map_generic", "(module != NULL)");

    ret = 22;
  }
  else if (ctx == 0)
  {
    BL_ERROR("E %s:BAD_ARGS::argument is not valid, require %s", "module_screensaver_map_generic", "(ctx != NULL)");
    ret = 22;
  }
  else if (ctx->type == 0)
  {
    if ((ret = module_screensaver_prerender(module, ctx, type)) != 0)
    {
      BL_ERROR("E %s:FAILED::%s failed with %d", "module_screensaver_map_generic", "module_screensaver_prerender", ret);
    }
    else
    {
      blanket_image_window_bringup(module, ctx->window);
      ctx->type = 1;
    }
  }
  else if (ctx->type == 1)
  {
    if (ctx->type == type)
    {
      cairo_paint(ctx->ss_cr);
      ret = 0;
    }
    else
    {
      if ((ret = module_screensaver_prerender(module, ctx, type)) != 0)
      {
        BL_ERROR("E screensaver:PRERENDER_FAILED:err=%d,type=%d:%s", ret, type, strerror(ret));
      }
      else
      {
        cairo_paint(ctx->ss_cr);
      }
    }
  }

  BL_DEBUG("D def:leave:%s:%d", "module_screensaver_map_generic", __LINE__);

  return ret;
}

int module_screensaver_prerender(struct Module *module, struct Context *ctx, int type)
{
  int ret;
  char current_name[5];
  char *image_path;
  cairo_surface_t *surface;
  cairo_status_t c_err;
  int width;
  int height;
  cairo_t *context;
  int s_width;
  int s_height;

  BL_DEBUG("D def:enter:%s:%d", "module_screensaver_prerender", __LINE__);

  if (module == NULL)
  {
    BL_ERROR("E %s:BAD_ARGS::argument is not valid, require %s", "module_screensaver_prerender", "(module != NULL)");
    ret = 22;
  }
  else if (ctx == NULL)
  {
    BL_ERROR("E %s:BAD_ARGS::argument is not valid, require %s", "module_screensaver_prerender", "(ctx != NULL)");
    ret = 22;
  }
  else if (type != 1)
  {
    ctx->type = type;
    if (type < 1)
    {
      BL_ERROR("E screensaver:INVALID_SCREEN_SELECTED:type=%d:screen type is not valid", type);
      ret = 22;
    }
    else
    {
      if (type == 2)
      {
        cairo_set_source_rgb(ctx->ss_cr, 0.0, 0.0, 1.875);
        cairo_rectangle(ctx->ss_cr, 0.0, 0.0, (double)blanket_image_screenwidth(), (double)blanket_image_screenheight());
        cairo_fill(ctx->ss_cr);
        cairo_paint(ctx->ss_cr);
      }
      ret = 0;
    }
  }
  else
  {
    module_screensaver_nextScreenSaverName(current_name);
    image_path = blanket_image_get_asset_name("screensaver", current_name, "bg", "png");
    surface = cairo_image_surface_create_from_png(image_path);
    width = 0;
    height = 0;
    context = NULL;
    if ((c_err = cairo_surface_status(surface)) != 0)
    {
      BL_ERROR("E %s:CAIRO_FAILED:sts=%d:%s", "module_screensaver_prerender", c_err, cairo_status_to_string(c_err));
      BL_ERROR("E %s:CAIRO_SURFACE_CREATE_FAILED:png=%s:", "module_screensaver_prerender", image_path);
    }
    else
    {
      width = cairo_image_surface_get_width(surface);
      height = cairo_image_surface_get_height(surface);
      context = cairo_create(surface);
      BL_DEBUG("D def:Image %s is %dx%d pixels (%s:%s:%d)", image_path, width, height, "module_screensaver_prerender", "", __LINE__);
    }
    free(image_path);
    s_width = blanket_image_screenwidth();
    s_height = blanket_image_screenheight();
    cairo_set_source_surface(ctx->ss_cr, surface, s_width - width, s_height - height);
    cairo_paint(ctx->ss_cr);
    cairo_surface_destroy(surface);
    cairo_destroy(context);
    ret = 0;
  }

  BL_DEBUG("D def:leave:%s:%d", "module_screensaver_prerender", __LINE__);
  return ret;
}

void module_screensaver_nextScreenSaverName(char current_namep[5])
{
  int _errno;
  int fd;
  char current_name[5];
  char next_name[5];
  struct stat sb;
  char *image_path;

  BL_DEBUG("D def:enter:%s:%d", "module_screensaver_nextScreenSaverName", __LINE__);

  if ((_errno = g_mkdir_with_parents("/var/local/blanket/screensaver", S_IRWXU)) < 0)
  {
    BL_ERROR("E screensaver:MKDIR_WITH_PARENTS_FAILED:err=%d,path=%s,mode=%d:%s", _errno, "/var/local/blanket/screensaver", S_IRWXU, strerror(_errno));
  }
  if ((fd = open("/var/local/blanket/screensaver/last_ss", O_CREAT | O_RDWR, S_IRUSR | S_IWUSR)) < 0)
  {
    BL_ERROR("E %s:OPEN_FILE_FAILED:err=%d,path=%s,mode=%d:%s", "module_screensaver_nextScreenSaverName", fd, "/var/local/blanket/screensaver/last_ss", O_CREAT | O_RDWR, strerror(fd));
  }
  else
  {
    ssize_t bytesread = read(fd, current_name, 4);
    close(fd);
    if (!(bytesread >= 4 && current_name[0] == 's' && current_name[1] == 's'))
    {
      strcpy(current_name, "ss00");
    }
    current_name[4] = 0;
    image_path = blanket_image_get_asset_name("screensaver", current_name, "bg", "png");
    if (image_path == NULL)
    {
      BL_ERROR("E %s:DOES_NOT_EXIST:object=%s:", "module_screensaver_nextScreenSaverName", "bg_path");
      strcpy(current_name, "ss00");
    }
    else
    {
      if ((_errno = stat(image_path, &sb)) < 0)
      {
        BL_ERROR("E screensaver:STAT_FAILED:err=%d,path=%s:%s", _errno, image_path, strerror(_errno));
        strcpy(current_name, "ss00");
      }
      else
      {
        if (!S_ISREG(sb.st_mode))
        {
          BL_ERROR("E screensaver:FILE_IS_NOT_REGULAR:path=%s,mode=%d:", image_path, S_ISREG(sb.st_mode));
          strcpy(current_name, "ss00");
        }
      }
      free(image_path);
      current_name[4] = 0;
      strncpy(next_name, current_name, 5);
      next_name[4] = 0;
      if (next_name[3] == '9')
      {
        next_name[3] = '0';
        if (next_name[2] == '9')
        {
          next_name[2] = '0';
        }
        else
        {
          next_name[2]++;
        }
      }
      else
      {
        next_name[3]++;
      }
      if ((fd = open("/var/local/blanket/screensaver/last_ss", O_CREAT | O_RDWR, S_IRUSR | S_IWUSR)) < 0)
      {
        BL_ERROR("E %s:OPEN_FILE_FAILED:err=%d,path=%s,mode=%d:%s", "module_screensaver_nextScreenSaverName", fd, "/var/local/blanket/screensaver/last_ss", O_CREAT | O_RDWR, strerror(fd));
      }
      else
      {
        write(fd, next_name, 4);
        close(fd);
      }
    }
  }

  strcpy(current_namep, current_name);

  BL_DEBUG("D def:leave:%s:%d", "module_screensaver_nextScreenSaverName", __LINE__);
}

int module_screensaver_repaint(struct Module *module, XEvent *xev, struct Context *ctx)
{
  int ret;

  BL_DEBUG("D def:enter:%s:%d", "module_screensaver_repaint", __LINE__);

  if (ctx->type == 0)
  {
    BL_DEBUG("D screensaver:XEVENT_WHEN_UNMAPPED:type=%d:exiting repaint", xev->type);
    ret = 0;
  }
  else if (xev->type == Expose)
  {
    auto ev = (XExposeEvent *)xev;
    // Simple applications that do not want to optimize redisplay by
    // distinguishing between subareas of its window can just ignore all Expose
    // events with nonzero counts and perform full redisplays on events with
    // zero counts.
    // https://www.x.org/archive/X11R7.5/doc/man/man3/XExposeEvent.3.html
    if (ev->count > 0)
    {
    }
    else if (ctx->ss_cr == NULL)
    {
      BL_ERROR("E %s:DOES_NOT_EXIST:object=%s:", "module_screensaver_repaint", "ctx->ss_cr");
    }
    else
    {
      cairo_paint(ctx->ss_cr);
    }
    ret = 0;
  }
  else
  {
    BL_WARNING("W screensaver:UNHANDLED_X11_EVENT:type=%d:", xev->type);
    ret = 38;
  }

  BL_DEBUG("D def:leave:%s:%d", "module_screensaver_repaint", __LINE__);

  return ret;
}
