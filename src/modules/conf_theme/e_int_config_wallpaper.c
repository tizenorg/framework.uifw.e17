#include "e.h"
#include "e_mod_main.h"

static void *_create_data(E_Config_Dialog *cfd);
static void _free_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static void _fill_data(E_Config_Dialog_Data *cfdata);
static int _basic_apply(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static Evas_Object *_basic_create(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata);
static int _adv_apply(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static Evas_Object *_adv_create(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata);

static E_Config_Dialog *_e_int_config_wallpaper_desk(E_Container *con, int con_num, int zone_num, int desk_x, int desk_y);

static void _cb_button_up(void *data1, void *data2);
static void _cb_files_changed(void *data, Evas_Object *obj, void *event_info);
static void _cb_files_selection_change(void *data, Evas_Object *obj, void *event_info);
static void _cb_files_files_changed(void *data, Evas_Object *obj, void *event_info);
static void _cb_files_files_deleted(void *data, Evas_Object *obj, void *event_info);
static void _cb_theme_wallpaper(void *data, Evas_Object *obj, void *event_info);
static void _cb_dir(void *data, Evas_Object *obj, void *event_info);
static void _cb_import(void *data1, void *data2);

#define E_CONFIG_WALLPAPER_ALL 0
#define E_CONFIG_WALLPAPER_DESK 1
#define E_CONFIG_WALLPAPER_SCREEN 2

struct _E_Config_Wallpaper
{
   int specific_config;
   int con_num, zone_num;
   int desk_x, desk_y;
};

struct _E_Config_Dialog_Data
{
   E_Config_Dialog *cfd;
   Evas_Object *o_fm;
   Evas_Object *o_up_button;
   Evas_Object *o_preview;
   Evas_Object *o_theme_bg;
   Evas_Object *o_personal;
   Evas_Object *o_system;
   int fmdir, use_theme_bg;
   char *bg;

   /* advanced */
   int all_this_desk_screen;

   /* dialogs */
   E_Win *win_import;
#ifdef HAVE_EXCHANGE
   E_Dialog *dia_web;
#endif
};

E_Config_Dialog *
e_int_config_wallpaper(E_Container *con, const char *params __UNUSED__)
{
   return _e_int_config_wallpaper_desk(con, -1, -1, -1, -1);
}

E_Config_Dialog *
e_int_config_wallpaper_desk(E_Container *con, const char *params)
{
   int con_num, zone_num, desk_x, desk_y;

   if (!params) return NULL;
   con_num = zone_num = desk_x = desk_y = -1;
   if (sscanf(params, "%i %i %i %i", &con_num, &zone_num, &desk_x, &desk_y) != 4)
     return NULL;
   return _e_int_config_wallpaper_desk(con, con_num, zone_num, desk_x, desk_y);
}

static E_Config_Dialog *
_e_int_config_wallpaper_desk(E_Container *con, int con_num, int zone_num, int desk_x, int desk_y)
{
   E_Config_Dialog *cfd;
   E_Config_Dialog_View *v;
   E_Config_Wallpaper *cw;

   if (e_config_dialog_find("E", "appearance/wallpaper")) return NULL;
   v = E_NEW(E_Config_Dialog_View, 1);
   cw = E_NEW(E_Config_Wallpaper, 1);

   v->create_cfdata = _create_data;
   v->free_cfdata = _free_data;
   v->basic.apply_cfdata = _basic_apply;
   v->basic.create_widgets = _basic_create;

   if (!(con_num == -1 && zone_num == -1 && desk_x == -1 && desk_y == -1))
     cw->specific_config = 1;
   else
     {
        v->advanced.apply_cfdata   = _adv_apply;
        v->advanced.create_widgets = _adv_create;
     }

   v->override_auto_apply = 1;

   cw->con_num = con_num;
   cw->zone_num = zone_num;
   cw->desk_x = desk_x;
   cw->desk_y = desk_y;

   cfd = e_config_dialog_new(con, _("Wallpaper Settings"), "E",
                             "appearance/wallpaper",
                             "preferences-desktop-wallpaper", 0, v, cw);
   return cfd;
}

static void
_bg_set(E_Config_Dialog_Data *cfdata)
{
   if (!cfdata->o_preview) return;
   if (cfdata->bg)
     {
        if (eina_str_has_extension(cfdata->bg, ".edj"))
           e_widget_preview_edje_set(cfdata->o_preview, cfdata->bg,
                                     "e/desktop/background");
        else
           e_widget_preview_file_set(cfdata->o_preview, cfdata->bg,
                                     NULL);
     }
   else
     {
        const char *f;

        f = e_theme_edje_file_get("base/theme/backgrounds",
                                  "e/desktop/background");
        e_widget_preview_edje_set(cfdata->o_preview, f,
                                  "e/desktop/background");
     }
}

void
e_int_config_wallpaper_update(E_Config_Dialog *dia, char *file)
{
   E_Config_Dialog_Data *cfdata;
   char path[PATH_MAX];

   cfdata = dia->cfdata;
   cfdata->fmdir = 1;
   e_widget_radio_toggle_set(cfdata->o_personal, 1);
   e_user_dir_concat_static(path, "backgrounds");
   E_FREE(cfdata->bg);
   cfdata->bg = strdup(file);
   cfdata->use_theme_bg = 0;
   if (cfdata->o_theme_bg)
     e_widget_check_checked_set(cfdata->o_theme_bg, cfdata->use_theme_bg);
   if (cfdata->o_fm) e_widget_flist_path_set(cfdata->o_fm, path, "/");
   _bg_set(cfdata);
   if (cfdata->o_fm) e_widget_change(cfdata->o_fm);
}

void
e_int_config_wallpaper_import_done(E_Config_Dialog *dia)
{
   E_Config_Dialog_Data *cfdata;

   cfdata = dia->cfdata;
   cfdata->win_import = NULL;
}

#ifdef HAVE_EXCHANGE
void
e_int_config_wallpaper_web_done(E_Config_Dialog *dia)
{
   E_Config_Dialog_Data *cfdata;

   cfdata = dia->cfdata;
   cfdata->dia_web = NULL;
}
#endif

static void
_cb_button_up(void *data1, void *data2 __UNUSED__)
{
   E_Config_Dialog_Data *cfdata;

   cfdata = data1;
   if (cfdata->o_fm) e_widget_flist_parent_go(cfdata->o_fm);
}

static void
_cb_files_changed(void *data, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   E_Config_Dialog_Data *cfdata;

   if (!(cfdata = data)) return;
   if (!cfdata->o_fm) return;
   if (cfdata->o_up_button)
     e_widget_disabled_set(cfdata->o_up_button,
                           !e_widget_flist_has_parent_get(cfdata->o_fm));
}

static void
_cb_files_selection_change(void *data, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   E_Config_Dialog_Data *cfdata;
   Eina_List *selected;
   E_Fm2_Icon_Info *ici;
   const char *realpath;
   char buf[PATH_MAX];

   cfdata = data;
   if (!cfdata->o_fm) return;
   if (!(selected = e_widget_flist_selected_list_get(cfdata->o_fm))) return;
   ici = selected->data;
   realpath = e_widget_flist_real_path_get(cfdata->o_fm);
   if (!strcmp(realpath, "/"))
     snprintf(buf, sizeof(buf), "/%s", ici->file);
   else
     snprintf(buf, sizeof(buf), "%s/%s", realpath, ici->file);
   eina_list_free(selected);
   if (ecore_file_is_dir(buf)) return;

   E_FREE(cfdata->bg);

   cfdata->bg = strdup(buf);
   _bg_set(cfdata);
   if (cfdata->o_theme_bg)
     e_widget_check_checked_set(cfdata->o_theme_bg, 0);
   cfdata->use_theme_bg = 0;
   e_widget_change(cfdata->o_fm);
}

static void
_cb_files_files_changed(void *data, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   E_Config_Dialog_Data *cfdata;
   const char *p = NULL;
   char buf[PATH_MAX];
   size_t len;

   cfdata = data;
   if ((!cfdata->bg) || (!cfdata->o_fm)) return;
   p = e_widget_flist_real_path_get(cfdata->o_fm);
   if (p)
     {
        if (strncmp(p, cfdata->bg, strlen(p))) return;
     }
   else return;

   len = e_user_dir_concat_static(buf, "backgrounds");
   if (!strncmp(cfdata->bg, buf, len))
     p = cfdata->bg + len + 1;
   else
     {
        len = e_prefix_data_concat_static(buf, "data/backgrounds");
        if (!strncmp(cfdata->bg, buf, len))
          p = cfdata->bg + len + 1;
        else
          p = cfdata->bg;
     }

   e_widget_flist_select_set(cfdata->o_fm, p, 1);
   e_widget_flist_file_show(cfdata->o_fm, p);
}

static void
_cb_files_files_deleted(void *data, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   E_Config_Dialog_Data *cfdata;
   Eina_List *sel, *all, *n;
   E_Fm2_Icon_Info *ici, *ic;

   cfdata = data;
   if ((!cfdata->bg) || (!cfdata->o_fm)) return;

   if (!(all = e_widget_flist_all_list_get(cfdata->o_fm))) return;
   if (!(sel = e_widget_flist_selected_list_get(cfdata->o_fm))) return;

   ici = sel->data;
   all = eina_list_data_find_list(all, ici);
   n = eina_list_next(all);
   if (!n)
     {
        n = eina_list_prev(all);
        if (!n) return;
     }

   if (!(ic = n->data)) return;

   e_widget_flist_select_set(cfdata->o_fm, ic->file, 1);
   e_widget_flist_file_show(cfdata->o_fm, ic->file);

   eina_list_free(n);

   evas_object_smart_callback_call(cfdata->o_fm, "selection_change", cfdata);
}

static void
_cb_theme_wallpaper(void *data, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   E_Config_Dialog_Data *cfdata;
   const char *f;

   cfdata = data;
   if (cfdata->use_theme_bg)
     {
        f = e_theme_edje_file_get("base/theme/backgrounds",
                                  "e/desktop/background");
        E_FREE(cfdata->bg);
        cfdata->bg = strdup(f);
        _bg_set(cfdata);
     }
   else
     {
        evas_object_smart_callback_call(cfdata->o_fm, "selection_change",
                                        cfdata);
        _bg_set(cfdata);
     }
}

static void
_cb_dir(void *data, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   E_Config_Dialog_Data *cfdata;
   char path[PATH_MAX];

   cfdata = data;
   if (cfdata->fmdir == 1)
     e_prefix_data_concat_static(path, "data/backgrounds");
   else
     e_user_dir_concat_static(path, "backgrounds");
   e_widget_flist_path_set(cfdata->o_fm, path, "/");
}

static void
_cb_import(void *data1, void *data2 __UNUSED__)
{
   E_Config_Dialog_Data *cfdata;

   cfdata = data1;
   if (cfdata->win_import)
     e_win_raise(cfdata->win_import);
   else
     cfdata->win_import = e_int_config_wallpaper_fsel(cfdata->cfd);
}

#ifdef HAVE_EXCHANGE
static void
_cb_web(void *data1, void *data2 __UNUSED__)
{
   E_Config_Dialog_Data *cfdata;

   cfdata = data1;
   if (cfdata->dia_web)
      e_win_raise(cfdata->dia_web->win);
   else
      cfdata->dia_web = e_int_config_wallpaper_web(cfdata->cfd);
}
#endif

static void
_fill_data(E_Config_Dialog_Data *cfdata)
{
   E_Config_Wallpaper *cw;
   const E_Config_Desktop_Background *cfbg;
   char path[PATH_MAX];

   cw = cfdata->cfd->data;
   if (cw->specific_config)
     {
        const char *bg;

        /* specific config passed in. set for that only */
        bg = e_bg_file_get(cw->con_num, cw->zone_num, cw->desk_x, cw->desk_y);
        if (bg) cfdata->bg = strdup(bg);
     }
   else
     {
        /* get current desk. advanced mode allows selecting all, screen or desk */
        E_Container *c;
        E_Zone *z;
        E_Desk *d;

        c = e_container_current_get(e_manager_current_get());
        z = e_zone_current_get(c);
        d = e_desk_current_get(z);

        cfbg = e_bg_config_get(c->num, z->id, d->x, d->y);
        /* if we have a config for this bg, use it. */
        if (cfbg)
          {
             if (cfbg->container >= 0 && cfbg->zone >= 0)
               {
                  if (cfbg->desk_x >= 0 && cfbg->desk_y >= 0)
                    cfdata->all_this_desk_screen = E_CONFIG_WALLPAPER_DESK;
                  else
                    cfdata->all_this_desk_screen = E_CONFIG_WALLPAPER_SCREEN;
               }
             E_FREE(cfdata->bg);
             cfdata->bg = strdup(cfbg->file);
          }
     }

   if ((!cfdata->bg) && (e_config->desktop_default_background))
     cfdata->bg = strdup(e_config->desktop_default_background);

   if (cfdata->bg)
     {
        const char *f;
        size_t len;

        f = e_theme_edje_file_get("base/theme/backgrounds",
                                  "e/desktop/background");
        if (!strcmp(cfdata->bg, f)) cfdata->use_theme_bg = 1;
        len = e_prefix_data_concat_static(path, "data/backgrounds");
        if (!strncmp(cfdata->bg, path, len)) cfdata->fmdir = 1;
     }
   else
     cfdata->use_theme_bg = 1;
}

static void *
_create_data(E_Config_Dialog *cfd)
{
   E_Config_Dialog_Data *cfdata;

   cfdata = E_NEW(E_Config_Dialog_Data, 1);
   cfd->cfdata = cfdata;
   cfdata->cfd = cfd;
   _fill_data(cfdata);
   return cfdata;
}

static void
_free_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata)
{
   if (cfdata->win_import)
     e_int_config_wallpaper_fsel_del(cfdata->win_import);
#ifdef HAVE_EXCHANGE
   if (cfdata->dia_web)
     e_int_config_wallpaper_web_del(cfdata->dia_web);
#endif
   E_FREE(cfdata->bg);
   E_FREE(cfd->data);
   E_FREE(cfdata);
}

static Evas_Object *
_basic_create(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata)
{
   Evas_Object *o, *rt, *ot;
   Evas_Object *ow;
   E_Zone *zone = NULL;
   E_Radio_Group *rg;
   char path[PATH_MAX];
   int mw, mh, online;

   online = ecore_file_download_protocol_available("http://");

   zone = e_zone_current_get(cfd->con);
   o = e_widget_list_add(evas, 0, 1);

   rg = e_widget_radio_group_new(&(cfdata->fmdir));
   ot = e_widget_table_add(evas, 0);
   rt = e_widget_table_add(evas, 1);

   /* create dir radios */
   ow = e_widget_radio_add(evas, _("Personal"), 0, rg);
   cfdata->o_personal = ow;
   evas_object_smart_callback_add(ow, "changed", _cb_dir, cfdata);
   e_widget_table_object_append(rt, ow, 0, 0, 1, 1, 1, 1, 0, 0);
   ow = e_widget_radio_add(evas, _("System"), 1, rg);
   cfdata->o_system = ow;
   evas_object_smart_callback_add(ow, "changed", _cb_dir, cfdata);
   e_widget_table_object_append(rt, ow, 1, 0, 1, 1, 1, 1, 0, 0);
   e_widget_table_object_append(ot, rt, 0, 0, 1, 1, 0, 0, 0, 0);

   ow = e_widget_button_add(evas, _("Go up a Directory"), "go-up",
                            _cb_button_up, cfdata, NULL);
   cfdata->o_up_button = ow;
   e_widget_table_object_append(ot, ow, 0, 1, 1, 1, 0, 0, 0, 0);

   if (cfdata->fmdir == 1)
     e_prefix_data_concat_static(path, "data/backgrounds");
   else
     e_user_dir_concat_static(path, "backgrounds");

   ow = e_widget_flist_add(evas);
   cfdata->o_fm = ow;
   evas_object_smart_callback_add(ow, "dir_changed",
                                  _cb_files_changed, cfdata);
   evas_object_smart_callback_add(ow, "selection_change",
                                  _cb_files_selection_change, cfdata);
   evas_object_smart_callback_add(ow, "changed",
                                  _cb_files_files_changed, cfdata);
   evas_object_smart_callback_add(ow, "files_deleted",
                                  _cb_files_files_deleted, cfdata);
   e_widget_flist_path_set(ow, path, "/");

   e_widget_size_min_set(ow, 160, 160);
   e_widget_table_object_append(ot, ow, 0, 2, 1, 1, 1, 1, 1, 1);
   e_widget_list_object_append(o, ot, 1, 1, 0.0);

   ot = e_widget_table_add(evas, 0);
   ow = e_widget_check_add(evas, _("Use Theme Wallpaper"),
                           &cfdata->use_theme_bg);
   cfdata->o_theme_bg = ow;
   evas_object_smart_callback_add(ow, "changed", _cb_theme_wallpaper, cfdata);
   e_widget_table_object_append(ot, ow, 0, 0, 2 + online, 1, 1, 0, 0, 0);
   ow = e_widget_button_add(evas, _("Picture..."), "folder-image",
                            _cb_import, cfdata, NULL);
   e_widget_table_object_append(ot, ow, 0, 1, 1, 1, 1, 0, 0, 0);

#ifdef HAVE_EXCHANGE
   if (online)
     {
        ow = e_widget_button_add(evas, _("Online..."), "network-website",
                                 _cb_web, cfdata, NULL);
        e_widget_table_object_append(ot, ow, 2, 1, 1, 1, 1, 0, 0, 0);
     }
#endif

   mw = 320;
   mh = (320 * zone->h) / zone->w;
   ow = e_widget_preview_add(evas, mw, mh);
   cfdata->o_preview = ow;
   _bg_set(cfdata);
   evas_object_show(ow);
   e_widget_table_object_append(ot, ow, 0, 2, 2 + online, 1, 1, 1, 1, 1);
   e_widget_list_object_append(o, ot, 1, 1, 0.5);
   e_dialog_resizable_set(cfd->dia, 0);
   return o;
}

static int
_basic_apply(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata)
{
   E_Config_Wallpaper *cw;

   cw = cfd->data;
   if (cw->specific_config)
     {
        /* update a specific config */
        e_bg_del(cw->con_num, cw->zone_num, cw->desk_x, cw->desk_y);
        e_bg_add(cw->con_num, cw->zone_num, cw->desk_x, cw->desk_y, cfdata->bg);
     }
   else
     {
        /* set the default and nuke individual configs */
        while (e_config->desktop_backgrounds)
          {
             E_Config_Desktop_Background *cfbg;

             cfbg = e_config->desktop_backgrounds->data;
             e_bg_del(cfbg->container, cfbg->zone, cfbg->desk_x, cfbg->desk_y);
          }
        if ((cfdata->use_theme_bg) || (!cfdata->bg))
          e_bg_default_set(NULL);
        else
          e_bg_default_set(cfdata->bg);

        cfdata->all_this_desk_screen = 0;
     }

   e_bg_update();
   e_config_save_queue();
   return 1;
}

static Evas_Object *
_adv_create(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata)
{
   Evas_Object *o, *rt, *ot;
   Evas_Object *ow, *of;
   E_Zone *zone = NULL;
   E_Radio_Group *rg;
   char path[PATH_MAX];
   int mw, mh, online;

   online = ecore_file_download_protocol_available("http://");

   zone = e_zone_current_get(cfd->con);
   o = e_widget_list_add(evas, 0, 1);

   rg = e_widget_radio_group_new(&(cfdata->fmdir));
   ot = e_widget_table_add(evas, 0);
   rt = e_widget_table_add(evas, 1);

   /* create dir radios */
   ow = e_widget_radio_add(evas, _("Personal"), 0, rg);
   cfdata->o_personal = ow;
   evas_object_smart_callback_add(ow, "changed", _cb_dir, cfdata);
   e_widget_table_object_append(rt, ow, 0, 0, 1, 1, 1, 1, 0, 0);
   ow = e_widget_radio_add(evas, _("System"), 1, rg);
   cfdata->o_system = ow;
   evas_object_smart_callback_add(ow, "changed", _cb_dir, cfdata);
   e_widget_table_object_append(rt, ow, 1, 0, 1, 1, 1, 1, 0, 0);
   e_widget_table_object_append(ot, rt, 0, 0, 1, 1, 0, 0, 0, 0);

   ow = e_widget_button_add(evas, _("Go up a Directory"), "go-up",
                            _cb_button_up, cfdata, NULL);
   cfdata->o_up_button = ow;
   e_widget_table_object_append(ot, ow, 0, 1, 1, 1, 0, 0, 0, 0);

   if (cfdata->fmdir == 1)
     e_prefix_data_concat_static(path, "data/backgrounds");
   else
     e_user_dir_concat_static(path, "backgrounds");

   ow = e_widget_flist_add(evas);
   cfdata->o_fm = ow;
   evas_object_smart_callback_add(ow, "dir_changed",
                                  _cb_files_changed, cfdata);
   evas_object_smart_callback_add(ow, "selection_change",
                                  _cb_files_selection_change, cfdata);
   evas_object_smart_callback_add(ow, "changed",
                                  _cb_files_files_changed, cfdata);
   e_widget_flist_path_set(ow, path, "/");

   e_widget_size_min_set(ow, 160, 160);
   e_widget_table_object_append(ot, ow, 0, 2, 1, 1, 1, 1, 1, 1);
   e_widget_list_object_append(o, ot, 1, 1, 0.0);

   ot = e_widget_table_add(evas, 0);
   ow = e_widget_check_add(evas, _("Use Theme Wallpaper"),
                           &cfdata->use_theme_bg);
   cfdata->o_theme_bg = ow;
   evas_object_smart_callback_add(ow, "changed", _cb_theme_wallpaper, cfdata);
   e_widget_table_object_append(ot, ow, 0, 0, 2 + online, 1, 1, 0, 0, 0);
   ow = e_widget_button_add(evas, _("Picture..."), "folder-image",
                            _cb_import, cfdata, NULL);
   e_widget_table_object_append(ot, ow, 0, 1, 1, 1, 1, 0, 0, 0);

#ifdef HAVE_EXCHANGE
   if (online)
     {
        ow = e_widget_button_add(evas, _("Online..."), "network-website",
                                 _cb_web, cfdata, NULL);
        e_widget_table_object_append(ot, ow, 2, 1, 1, 1, 1, 0, 0, 0);
     }
#endif

   mw = 320;
   mh = (320 * zone->h) / zone->w;
   ow = e_widget_preview_add(evas, mw, mh);
   evas_object_size_hint_aspect_set(ow, EVAS_ASPECT_CONTROL_BOTH, zone->w, zone->h); 
   cfdata->o_preview = ow;
   _bg_set(cfdata);
   e_widget_table_object_append(ot, ow, 0, 2, 2 + online, 1, 1, 1, 1, 1);

   rg = e_widget_radio_group_new(&(cfdata->all_this_desk_screen));
   of = e_widget_frametable_add(evas, _("Where to place the Wallpaper"), 0);
   ow = e_widget_radio_add(evas, _("All Desktops"), E_CONFIG_WALLPAPER_ALL, rg);
   e_widget_frametable_object_append(of, ow, 0, 0, 1, 1, 1, 0, 1, 0);
   ow = e_widget_radio_add(evas, _("This Desktop"), E_CONFIG_WALLPAPER_DESK, rg);
   e_widget_frametable_object_append(of, ow, 0, 1, 1, 1, 1, 0, 1, 0);
   ow = e_widget_radio_add(evas, _("This Screen"), E_CONFIG_WALLPAPER_SCREEN, rg);
   if (!(e_util_container_zone_number_get(0, 1) ||
         (e_util_container_zone_number_get(1, 0))))
     e_widget_disabled_set(ow, 1);
   e_widget_frametable_object_append(of, ow, 0, 2, 1, 1, 1, 0, 1, 0);
   e_widget_table_object_append(ot, of, 0, 3, 2 + online, 1, 1, 0, 1, 0);

   e_widget_list_object_append(o, ot, 1, 1, 0.0);
   e_dialog_resizable_set(cfd->dia, 0);
   
   return o;
}

static int
_adv_apply(E_Config_Dialog *cfd __UNUSED__, E_Config_Dialog_Data *cfdata)
{
   Eina_List *fl = NULL, *l;
   E_Zone *z;
   E_Desk *d;

   if (!(z = e_zone_current_get(cfdata->cfd->con))) return 0;
   if (!(d = e_desk_current_get(z))) return 0;
   if (cfdata->use_theme_bg)
     {
        while (e_config->desktop_backgrounds)
          {
             E_Config_Desktop_Background *cfbg;

             cfbg = e_config->desktop_backgrounds->data;
             e_bg_del(cfbg->container, cfbg->zone, cfbg->desk_x, cfbg->desk_y);
          }
        e_bg_default_set(NULL);
     }
   else
     {
        if (cfdata->all_this_desk_screen == E_CONFIG_WALLPAPER_ALL)
          {
             while (e_config->desktop_backgrounds)
               {
                  E_Config_Desktop_Background *cfbg;

                  cfbg = e_config->desktop_backgrounds->data;
                  e_bg_del(cfbg->container, cfbg->zone, cfbg->desk_x, cfbg->desk_y);
               }
             e_bg_default_set(cfdata->bg);
          }
        else if (cfdata->all_this_desk_screen == E_CONFIG_WALLPAPER_DESK)
          {
             e_bg_del(z->container->num, z->num, d->x, d->y);
             e_bg_del(z->container->num, -1, d->x, d->y);
             e_bg_del(-1, z->num, d->x, d->y);
             e_bg_del(-1, -1, d->x, d->y);
             e_bg_add(z->container->num, z->num, d->x, d->y, cfdata->bg);
          }
        else if (cfdata->all_this_desk_screen == E_CONFIG_WALLPAPER_SCREEN)
          {
             for (l = e_config->desktop_backgrounds; l; l = l->next)
               {
                  E_Config_Desktop_Background *cfbg;

                  cfbg = l->data;
                  if ((cfbg->container == (int)z->container->num) &&
                      (cfbg->zone == (int)z->num))
                    fl = eina_list_append(fl, cfbg);
               }
             while (fl)
               {
                  E_Config_Desktop_Background *cfbg;

                  cfbg = fl->data;
                  e_bg_del(cfbg->container, cfbg->zone, cfbg->desk_x,
                           cfbg->desk_y);
                  fl = eina_list_remove_list(fl, fl);
               }
             e_bg_add(z->container->num, z->num, -1, -1, cfdata->bg);
          }
     }
   e_bg_update();
   e_config_save_queue();
   return 1;
}
