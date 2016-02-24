#include "e.h"

#include "dlog.h"
#undef LOG_TAG
#define LOG_TAG "E17"

#if ((E17_PROFILE >= LOWRES_PDA) && (E17_PROFILE <= HIRES_PDA))
#define DEF_MENUCLICK             1.25
#else
#define DEF_MENUCLICK             0.25
#endif

#define RANDR_SERIALIZED_SETUP_11 ((int)((1 << 16) | 1))
#define RANDR_SERIALIZED_SETUP_12 ((int)((1 << 16) | 2))
#define RANDR_SERIALIZED_SETUP_13 ((int)((1 << 16) | 3))

EAPI E_Config * e_config = NULL;

static int _e_config_revisions = 9;

/* local subsystem functions */
static void      _e_config_save_cb(void *data);
static void      _e_config_free(E_Config *cfg);
static Eina_Bool _e_config_cb_timer(void *data);
static int       _e_config_eet_close_handle(Eet_File *ef, char *file);
static void      _e_config_acpi_bindings_add(void);
#ifdef _F_COPY_FROM_DATA_DIR_
static void      _e_config_domain_data_update(const char *domain);
#endif

/* local subsystem globals */
static int _e_config_save_block = 0;
static E_Powersave_Deferred_Action *_e_config_save_defer = NULL;
static const char *_e_config_profile = NULL;

static E_Config_DD *_e_config_edd = NULL;
static E_Config_DD *_e_config_module_edd = NULL;
static E_Config_DD *_e_config_font_fallback_edd = NULL;
static E_Config_DD *_e_config_font_default_edd = NULL;
static E_Config_DD *_e_config_theme_edd = NULL;
static E_Config_DD *_e_config_bindings_mouse_edd = NULL;
static E_Config_DD *_e_config_bindings_key_edd = NULL;
static E_Config_DD *_e_config_bindings_edge_edd = NULL;
static E_Config_DD *_e_config_bindings_signal_edd = NULL;
static E_Config_DD *_e_config_bindings_wheel_edd = NULL;
static E_Config_DD *_e_config_bindings_acpi_edd = NULL;
static E_Config_DD *_e_config_path_append_edd = NULL;
static E_Config_DD *_e_config_desktop_bg_edd = NULL;
static E_Config_DD *_e_config_desklock_bg_edd = NULL;
static E_Config_DD *_e_config_desktop_name_edd = NULL;
#ifdef _F_USE_DESK_WINDOW_PROFILE_
static E_Config_DD *_e_config_desktop_window_profile_edd = NULL;
#endif
#ifdef _F_USE_TILED_DESK_LAYOUT_
static E_Config_DD *_e_config_desktop_layout_edd = NULL;
#endif /* end of _F_USE_TILED_DESK_LAYOUT_ */
static E_Config_DD *_e_config_remember_edd = NULL;
static E_Config_DD *_e_config_color_class_edd = NULL;
static E_Config_DD *_e_config_gadcon_edd = NULL;
static E_Config_DD *_e_config_gadcon_client_edd = NULL;
static E_Config_DD *_e_config_shelf_edd = NULL;
static E_Config_DD *_e_config_shelf_desk_edd = NULL;
static E_Config_DD *_e_config_mime_icon_edd = NULL;
static E_Config_DD *_e_config_syscon_action_edd = NULL;
static E_Config_DD *_e_config_env_var_edd = NULL;
static E_Config_DD *_e_config_randr_size_edd = NULL;
static E_Config_DD *_e_config_randr_edid_hash_edd = NULL;
static E_Config_DD *_e_config_randr_serialized_setup_edd = NULL;
static E_Config_DD *_e_config_randr_serialized_setup_11_edd = NULL;
static E_Config_DD *_e_config_randr_serialized_setup_12_edd = NULL;
static E_Config_DD *_e_config_randr_serialized_output_policy_edd = NULL;
static E_Config_DD *_e_config_randr_serialized_output_edd = NULL;
static E_Config_DD *_e_config_randr_mode_info_edd = NULL;
static E_Config_DD *_e_config_randr_serialized_crtc_edd = NULL;
static E_Config_DD *_e_config_xkb_layout_edd = NULL;
static E_Config_DD *_e_config_xkb_option_edd = NULL;

EAPI int E_EVENT_CONFIG_ICON_THEME = 0;
EAPI int E_EVENT_CONFIG_MODE_CHANGED = 0;
EAPI int E_EVENT_CONFIG_LOADED = 0;

#ifndef _F_USE_EXTN_DIALOG_
static E_Dialog *_e_config_error_dialog = NULL;

static void
_e_config_error_dialog_cb_delete(void *dia)
{
   if (dia == _e_config_error_dialog)
     _e_config_error_dialog = NULL;
}
#endif

static const char *
_e_config_profile_name_get(Eet_File *ef)
{
   /* profile config exists */
   char *data;
   const char *s = NULL;
   int data_len = 0;

   data = eet_read(ef, "config", &data_len);
   if ((data) && (data_len > 0))
     {
        int ok = 1;

        for (s = data; s < (data + data_len); s++)
          {
             // if profile is not all ascii (valid printable ascii - no
             // control codes etc.) or it contains a '/' (invalid as its a
             // directory delimiter) - then it's invalid
             if ((*s < ' ') || (*s > '~') || (*s == '/'))
               {
                  ok = 0;
                  break;
               }
          }
        s = NULL;
        if (ok)
          s = eina_stringshare_add_length(data, data_len);
        free(data);
     }
   return s;
}

/* externally accessible functions */
EINTERN int
e_config_init(void)
{
   E_EVENT_CONFIG_ICON_THEME = ecore_event_type_new();
   E_EVENT_CONFIG_MODE_CHANGED = ecore_event_type_new();
   E_EVENT_CONFIG_LOADED = ecore_event_type_new();

   /* if environment var set - use this profile name */
   _e_config_profile = eina_stringshare_add(getenv("E_CONF_PROFILE"));

   if (!_e_config_profile)
     {
        Eet_File *ef;
        char buf[PATH_MAX];

        /* try user profile config */
        e_user_dir_concat_static(buf, "config/profile.cfg");
        ef = eet_open(buf, EET_FILE_MODE_READ);
        if (ef)
          {
             _e_config_profile = _e_config_profile_name_get(ef);
             eet_close(ef);
             ef = NULL;
          }
        if (!_e_config_profile)
          {
             int i;

             for (i = 1; i <= _e_config_revisions; i++)
               {
                  e_user_dir_snprintf(buf, sizeof(buf), "config/profile.%i.cfg", i);
                  ef = eet_open(buf, EET_FILE_MODE_READ);
                  if (ef)
                    {
                       _e_config_profile = _e_config_profile_name_get(ef);
                       eet_close(ef);
                       ef = NULL;
                       if (_e_config_profile) break;
                    }
               }
             if (!_e_config_profile)
               {
                  /* use system if no user profile config */
#ifdef _F_COPY_FROM_DATA_DIR_
                  e_prefix_data_concat_static(buf, "config/e/config/profile.cfg");
#else
                  e_prefix_data_concat_static(buf, "data/config/profile.cfg");
#endif
                  ef = eet_open(buf, EET_FILE_MODE_READ);
               }
          }
        if (ef)
          {
             _e_config_profile = _e_config_profile_name_get(ef);
             eet_close(ef);
             ef = NULL;
          }
        if (!_e_config_profile)
          {
             /* no profile config - try other means */
             char *lnk = NULL;

             /* check symlink - if default is a symlink to another dir */
             e_prefix_data_concat_static(buf, "data/config/default");
             lnk = ecore_file_readlink(buf);
             /* if so use just the filename as the profile - must be a local link */
             if (lnk)
               {
                  _e_config_profile = eina_stringshare_add(ecore_file_file_get(lnk));
                  free(lnk);
               }
             else
               _e_config_profile = eina_stringshare_add("default");
          }
        if (!getenv("E_CONF_PROFILE"))
          e_util_env_set("E_CONF_PROFILE", _e_config_profile);
     }

   _e_config_gadcon_client_edd = E_CONFIG_DD_NEW("E_Config_Gadcon_Client", E_Config_Gadcon_Client);
#undef T
#undef D
#define T E_Config_Gadcon_Client
#define D _e_config_gadcon_client_edd
   E_CONFIG_VAL(D, T, name, STR);
   E_CONFIG_VAL(D, T, id, STR);
   E_CONFIG_VAL(D, T, geom.pos, INT);
   E_CONFIG_VAL(D, T, geom.size, INT);
   E_CONFIG_VAL(D, T, geom.res, INT);
   E_CONFIG_VAL(D, T, geom.pos_x, DOUBLE);
   E_CONFIG_VAL(D, T, geom.pos_y, DOUBLE);
   E_CONFIG_VAL(D, T, geom.size_w, DOUBLE);
   E_CONFIG_VAL(D, T, geom.size_h, DOUBLE);
   E_CONFIG_VAL(D, T, state_info.seq, INT);
   E_CONFIG_VAL(D, T, state_info.flags, INT);
   E_CONFIG_VAL(D, T, style, STR);
   E_CONFIG_VAL(D, T, orient, INT);
   E_CONFIG_VAL(D, T, autoscroll, UCHAR);
   E_CONFIG_VAL(D, T, resizable, UCHAR);

   _e_config_gadcon_edd = E_CONFIG_DD_NEW("E_Config_Gadcon", E_Config_Gadcon);
#undef T
#undef D
#define T E_Config_Gadcon
#define D _e_config_gadcon_edd
   E_CONFIG_VAL(D, T, name, STR);
   E_CONFIG_VAL(D, T, id, INT);
   E_CONFIG_VAL(D, T, zone, UINT);
   E_CONFIG_LIST(D, T, clients, _e_config_gadcon_client_edd);

   _e_config_shelf_desk_edd = E_CONFIG_DD_NEW("E_Config_Shelf_Desk", E_Config_Shelf_Desk);
#undef T
#undef D
#define T E_Config_Shelf_Desk
#define D _e_config_shelf_desk_edd
   E_CONFIG_VAL(D, T, x, INT);
   E_CONFIG_VAL(D, T, y, INT);

   _e_config_shelf_edd = E_CONFIG_DD_NEW("E_Config_Shelf", E_Config_Shelf);
#undef T
#undef D
#define T E_Config_Shelf
#define D _e_config_shelf_edd
   E_CONFIG_VAL(D, T, name, STR);
   E_CONFIG_VAL(D, T, id, INT);
   E_CONFIG_VAL(D, T, container, INT);
   E_CONFIG_VAL(D, T, zone, INT);
   E_CONFIG_VAL(D, T, layer, INT);
   E_CONFIG_VAL(D, T, popup, UCHAR);
   E_CONFIG_VAL(D, T, orient, INT);
   E_CONFIG_VAL(D, T, fit_along, UCHAR);
   E_CONFIG_VAL(D, T, fit_size, UCHAR);
   E_CONFIG_VAL(D, T, style, STR);
   E_CONFIG_VAL(D, T, size, INT);
   E_CONFIG_VAL(D, T, overlap, INT);
   E_CONFIG_VAL(D, T, autohide, INT);
   E_CONFIG_VAL(D, T, autohide_show_action, INT);
   E_CONFIG_VAL(D, T, hide_timeout, FLOAT);
   E_CONFIG_VAL(D, T, hide_duration, FLOAT);
   E_CONFIG_VAL(D, T, desk_show_mode, INT);
   E_CONFIG_LIST(D, T, desk_list, _e_config_shelf_desk_edd);
#ifdef _F_SHELF_INPUT_CONTROL_
   E_CONFIG_VAL(D, T, disable_menu, UCHAR);
#endif

   _e_config_desklock_bg_edd = E_CONFIG_DD_NEW("E_Config_Desklock_Background", E_Config_Desklock_Background);
#undef T
#undef D
#define T E_Config_Desklock_Background
#define D _e_config_desklock_bg_edd
   E_CONFIG_VAL(D, T, file, STR);

   _e_config_desktop_bg_edd = E_CONFIG_DD_NEW("E_Config_Desktop_Background", E_Config_Desktop_Background);
#undef T
#undef D
#define T E_Config_Desktop_Background
#define D _e_config_desktop_bg_edd
   E_CONFIG_VAL(D, T, container, INT);
   E_CONFIG_VAL(D, T, zone, INT);
   E_CONFIG_VAL(D, T, desk_x, INT);
   E_CONFIG_VAL(D, T, desk_y, INT);
   E_CONFIG_VAL(D, T, file, STR);

   _e_config_desktop_name_edd = E_CONFIG_DD_NEW("E_Config_Desktop_Name", E_Config_Desktop_Name);
#undef T
#undef D
#define T E_Config_Desktop_Name
#define D _e_config_desktop_name_edd
   E_CONFIG_VAL(D, T, container, INT);
   E_CONFIG_VAL(D, T, zone, INT);
   E_CONFIG_VAL(D, T, desk_x, INT);
   E_CONFIG_VAL(D, T, desk_y, INT);
   E_CONFIG_VAL(D, T, name, STR);

#ifdef _F_USE_DESK_WINDOW_PROFILE_
   _e_config_desktop_window_profile_edd = E_CONFIG_DD_NEW("E_Config_Desktop_Window_Profile", E_Config_Desktop_Window_Profile);
#undef T
#undef D
#define T E_Config_Desktop_Window_Profile
#define D _e_config_desktop_window_profile_edd
   E_CONFIG_VAL(D, T, container, INT);
   E_CONFIG_VAL(D, T, zone, INT);
   E_CONFIG_VAL(D, T, desk_x, INT);
   E_CONFIG_VAL(D, T, desk_y, INT);
   E_CONFIG_VAL(D, T, profile, STR);
#endif

#ifdef _F_USE_TILED_DESK_LAYOUT_
   _e_config_desktop_layout_edd = E_CONFIG_DD_NEW("E_Config_Desktop_Layout", E_Config_Desktop_Layout);
#undef T
#undef D
#define T E_Config_Desktop_Layout
#define D _e_config_desktop_layout_edd
   E_CONFIG_VAL(D, T, container, INT);
   E_CONFIG_VAL(D, T, zone, INT);
   E_CONFIG_VAL(D, T, desk_x, INT);
   E_CONFIG_VAL(D, T, desk_y, INT);
   E_CONFIG_VAL(D, T, id, INT);
   E_CONFIG_VAL(D, T, geom.x, INT);
   E_CONFIG_VAL(D, T, geom.y, INT);
   E_CONFIG_VAL(D, T, geom.w, INT);
   E_CONFIG_VAL(D, T, geom.h, INT);
#endif /* end of _F_USE_TILED_DESK_LAYOUT_ */

   _e_config_path_append_edd = E_CONFIG_DD_NEW("E_Path_Dir", E_Path_Dir);
#undef T
#undef D
#define T E_Path_Dir
#define D _e_config_path_append_edd
   E_CONFIG_VAL(D, T, dir, STR);

   _e_config_theme_edd = E_CONFIG_DD_NEW("E_Config_Theme", E_Config_Theme);
#undef T
#undef D
#define T E_Config_Theme
#define D _e_config_theme_edd
   E_CONFIG_VAL(D, T, category, STR);
   E_CONFIG_VAL(D, T, file, STR);

   _e_config_module_edd = E_CONFIG_DD_NEW("E_Config_Module", E_Config_Module);
#undef T
#undef D
#define T E_Config_Module
#define D _e_config_module_edd
   E_CONFIG_VAL(D, T, name, STR);
   E_CONFIG_VAL(D, T, enabled, UCHAR);
   E_CONFIG_VAL(D, T, delayed, UCHAR);
   E_CONFIG_VAL(D, T, priority, INT);

   _e_config_font_default_edd = E_CONFIG_DD_NEW("E_Font_Default",
                                                E_Font_Default);
#undef T
#undef D
#define T E_Font_Default
#define D _e_config_font_default_edd
   E_CONFIG_VAL(D, T, text_class, STR);
   E_CONFIG_VAL(D, T, font, STR);
   E_CONFIG_VAL(D, T, size, INT);

   _e_config_font_fallback_edd = E_CONFIG_DD_NEW("E_Font_Fallback",
                                                 E_Font_Fallback);
#undef T
#undef D
#define T E_Font_Fallback
#define D _e_config_font_fallback_edd
   E_CONFIG_VAL(D, T, name, STR);

   _e_config_bindings_mouse_edd = E_CONFIG_DD_NEW("E_Config_Binding_Mouse",
                                                  E_Config_Binding_Mouse);
#undef T
#undef D
#define T E_Config_Binding_Mouse
#define D _e_config_bindings_mouse_edd
   E_CONFIG_VAL(D, T, context, INT);
   E_CONFIG_VAL(D, T, modifiers, INT);
   E_CONFIG_VAL(D, T, action, STR);
   E_CONFIG_VAL(D, T, params, STR);
   E_CONFIG_VAL(D, T, button, UCHAR);
   E_CONFIG_VAL(D, T, any_mod, UCHAR);

   _e_config_bindings_key_edd = E_CONFIG_DD_NEW("E_Config_Binding_Key",
                                                E_Config_Binding_Key);
#undef T
#undef D
#define T E_Config_Binding_Key
#define D _e_config_bindings_key_edd
   E_CONFIG_VAL(D, T, context, INT);
   E_CONFIG_VAL(D, T, modifiers, INT);
   E_CONFIG_VAL(D, T, key, STR);
   E_CONFIG_VAL(D, T, action, STR);
   E_CONFIG_VAL(D, T, params, STR);
   E_CONFIG_VAL(D, T, any_mod, UCHAR);

   _e_config_bindings_edge_edd = E_CONFIG_DD_NEW("E_Config_Binding_Edge",
                                                 E_Config_Binding_Edge);
#undef T
#undef D
#define T E_Config_Binding_Edge
#define D _e_config_bindings_edge_edd
   E_CONFIG_VAL(D, T, context, INT);
   E_CONFIG_VAL(D, T, modifiers, INT);
   E_CONFIG_VAL(D, T, action, STR);
   E_CONFIG_VAL(D, T, params, STR);
   E_CONFIG_VAL(D, T, edge, UCHAR);
   E_CONFIG_VAL(D, T, any_mod, UCHAR);
   E_CONFIG_VAL(D, T, delay, FLOAT);

   _e_config_bindings_signal_edd = E_CONFIG_DD_NEW("E_Config_Binding_Signal",
                                                   E_Config_Binding_Signal);
#undef T
#undef D
#define T E_Config_Binding_Signal
#define D _e_config_bindings_signal_edd
   E_CONFIG_VAL(D, T, context, INT);
   E_CONFIG_VAL(D, T, signal, STR);
   E_CONFIG_VAL(D, T, source, STR);
   E_CONFIG_VAL(D, T, modifiers, INT);
   E_CONFIG_VAL(D, T, any_mod, UCHAR);
   E_CONFIG_VAL(D, T, action, STR);
   E_CONFIG_VAL(D, T, params, STR);

   _e_config_bindings_wheel_edd = E_CONFIG_DD_NEW("E_Config_Binding_Wheel",
                                                  E_Config_Binding_Wheel);
#undef T
#undef D
#define T E_Config_Binding_Wheel
#define D _e_config_bindings_wheel_edd
   E_CONFIG_VAL(D, T, context, INT);
   E_CONFIG_VAL(D, T, direction, INT);
   E_CONFIG_VAL(D, T, z, INT);
   E_CONFIG_VAL(D, T, modifiers, INT);
   E_CONFIG_VAL(D, T, any_mod, UCHAR);
   E_CONFIG_VAL(D, T, action, STR);
   E_CONFIG_VAL(D, T, params, STR);

   _e_config_bindings_acpi_edd = E_CONFIG_DD_NEW("E_Config_Binding_Acpi",
                                                 E_Config_Binding_Acpi);
#undef T
#undef D
#define T E_Config_Binding_Acpi
#define D _e_config_bindings_acpi_edd
   E_CONFIG_VAL(D, T, context, INT);
   E_CONFIG_VAL(D, T, type, INT);
   E_CONFIG_VAL(D, T, status, INT);
   E_CONFIG_VAL(D, T, action, STR);
   E_CONFIG_VAL(D, T, params, STR);

   _e_config_remember_edd = E_CONFIG_DD_NEW("E_Remember", E_Remember);
#undef T
#undef D
#define T E_Remember
#define D _e_config_remember_edd
   E_CONFIG_VAL(D, T, match, INT);
   E_CONFIG_VAL(D, T, apply_first_only, UCHAR);
   E_CONFIG_VAL(D, T, keep_settings, UCHAR);
   E_CONFIG_VAL(D, T, name, STR);
   E_CONFIG_VAL(D, T, class, STR);
   E_CONFIG_VAL(D, T, title, STR);
   E_CONFIG_VAL(D, T, role, STR);
   E_CONFIG_VAL(D, T, type, INT);
   E_CONFIG_VAL(D, T, transient, UCHAR);
   E_CONFIG_VAL(D, T, apply, INT);
   E_CONFIG_VAL(D, T, max_score, INT);
   E_CONFIG_VAL(D, T, prop.pos_x, INT);
   E_CONFIG_VAL(D, T, prop.pos_y, INT);
   E_CONFIG_VAL(D, T, prop.res_x, INT);
   E_CONFIG_VAL(D, T, prop.res_y, INT);
   E_CONFIG_VAL(D, T, prop.pos_w, INT);
   E_CONFIG_VAL(D, T, prop.pos_h, INT);
   E_CONFIG_VAL(D, T, prop.w, INT);
   E_CONFIG_VAL(D, T, prop.h, INT);
   E_CONFIG_VAL(D, T, prop.layer, INT);
   E_CONFIG_VAL(D, T, prop.lock_user_location, UCHAR);
   E_CONFIG_VAL(D, T, prop.lock_client_location, UCHAR);
   E_CONFIG_VAL(D, T, prop.lock_user_size, UCHAR);
   E_CONFIG_VAL(D, T, prop.lock_client_size, UCHAR);
   E_CONFIG_VAL(D, T, prop.lock_user_stacking, UCHAR);
   E_CONFIG_VAL(D, T, prop.lock_client_stacking, UCHAR);
   E_CONFIG_VAL(D, T, prop.lock_user_iconify, UCHAR);
   E_CONFIG_VAL(D, T, prop.lock_client_iconify, UCHAR);
   E_CONFIG_VAL(D, T, prop.lock_user_desk, UCHAR);
   E_CONFIG_VAL(D, T, prop.lock_client_desk, UCHAR);
   E_CONFIG_VAL(D, T, prop.lock_user_sticky, UCHAR);
   E_CONFIG_VAL(D, T, prop.lock_client_sticky, UCHAR);
   E_CONFIG_VAL(D, T, prop.lock_user_shade, UCHAR);
   E_CONFIG_VAL(D, T, prop.lock_client_shade, UCHAR);
   E_CONFIG_VAL(D, T, prop.lock_user_maximize, UCHAR);
   E_CONFIG_VAL(D, T, prop.lock_client_maximize, UCHAR);
   E_CONFIG_VAL(D, T, prop.lock_user_fullscreen, UCHAR);
   E_CONFIG_VAL(D, T, prop.lock_client_fullscreen, UCHAR);
   E_CONFIG_VAL(D, T, prop.lock_border, UCHAR);
   E_CONFIG_VAL(D, T, prop.lock_close, UCHAR);
   E_CONFIG_VAL(D, T, prop.lock_focus_in, UCHAR);
   E_CONFIG_VAL(D, T, prop.lock_focus_out, UCHAR);
   E_CONFIG_VAL(D, T, prop.lock_life, UCHAR);
   E_CONFIG_VAL(D, T, prop.border, STR);
   E_CONFIG_VAL(D, T, prop.sticky, UCHAR);
   E_CONFIG_VAL(D, T, prop.shaded, UCHAR);
   E_CONFIG_VAL(D, T, prop.skip_winlist, UCHAR);
   E_CONFIG_VAL(D, T, prop.skip_pager, UCHAR);
   E_CONFIG_VAL(D, T, prop.skip_taskbar, UCHAR);
   E_CONFIG_VAL(D, T, prop.fullscreen, UCHAR);
   E_CONFIG_VAL(D, T, prop.desk_x, INT);
   E_CONFIG_VAL(D, T, prop.desk_y, INT);
   E_CONFIG_VAL(D, T, prop.zone, INT);
   E_CONFIG_VAL(D, T, prop.head, INT);
   E_CONFIG_VAL(D, T, prop.command, STR);
   E_CONFIG_VAL(D, T, prop.icon_preference, UCHAR);
   E_CONFIG_VAL(D, T, prop.desktop_file, STR);
   E_CONFIG_VAL(D, T, prop.offer_resistance, UCHAR);

   _e_config_color_class_edd = E_CONFIG_DD_NEW("E_Color_Class", E_Color_Class);
#undef T
#undef D
#define T E_Color_Class
#define D _e_config_color_class_edd
   E_CONFIG_VAL(D, T, name, STR);
   E_CONFIG_VAL(D, T, r, INT);
   E_CONFIG_VAL(D, T, g, INT);
   E_CONFIG_VAL(D, T, b, INT);
   E_CONFIG_VAL(D, T, a, INT);
   E_CONFIG_VAL(D, T, r2, INT);
   E_CONFIG_VAL(D, T, g2, INT);
   E_CONFIG_VAL(D, T, b2, INT);
   E_CONFIG_VAL(D, T, a2, INT);
   E_CONFIG_VAL(D, T, r3, INT);
   E_CONFIG_VAL(D, T, g3, INT);
   E_CONFIG_VAL(D, T, b3, INT);
   E_CONFIG_VAL(D, T, a3, INT);

   _e_config_mime_icon_edd = E_CONFIG_DD_NEW("E_Config_Mime_Icon",
                                             E_Config_Mime_Icon);
#undef T
#undef D
#define T E_Config_Mime_Icon
#define D _e_config_mime_icon_edd
   E_CONFIG_VAL(D, T, mime, STR);
   E_CONFIG_VAL(D, T, icon, STR);

   _e_config_syscon_action_edd = E_CONFIG_DD_NEW("E_Config_Syscon_Action",
                                                 E_Config_Syscon_Action);
#undef T
#undef D
#define T E_Config_Syscon_Action
#define D _e_config_syscon_action_edd
   E_CONFIG_VAL(D, T, action, STR);
   E_CONFIG_VAL(D, T, params, STR);
   E_CONFIG_VAL(D, T, button, STR);
   E_CONFIG_VAL(D, T, icon, STR);
   E_CONFIG_VAL(D, T, is_main, INT);

   _e_config_env_var_edd = E_CONFIG_DD_NEW("E_Config_Env_Var",
                                           E_Config_Env_Var);
#undef T
#undef D
#define T E_Config_Env_Var
#define D _e_config_env_var_edd
   E_CONFIG_VAL(D, T, var, STR);
   E_CONFIG_VAL(D, T, val, STR);
   E_CONFIG_VAL(D, T, unset, UCHAR);

   _e_config_randr_size_edd = E_CONFIG_DD_NEW("Ecore_X_Randr_Screen_Size", Ecore_X_Randr_Screen_Size);
#undef T
#undef D
#define T Ecore_X_Randr_Screen_Size
#define D _e_config_randr_size_edd
   E_CONFIG_VAL(D, T, width, INT);
   E_CONFIG_VAL(D, T, height, INT);

   _e_config_randr_edid_hash_edd = E_CONFIG_DD_NEW("E_Randr_Edid_Hash", E_Randr_Edid_Hash);
#undef T
#undef D
#define T E_Randr_Edid_Hash
#define D _e_config_randr_edid_hash_edd
   E_CONFIG_VAL(D, T, hash, INT);

   _e_config_randr_serialized_setup_11_edd = E_CONFIG_DD_NEW("E_Randr_Serialized_Setup_11", E_Randr_Serialized_Setup_11);
#undef T
#undef D
#define T E_Randr_Serialized_Setup_11
#define D _e_config_randr_serialized_setup_11_edd
   E_CONFIG_VAL(D, T, size.width, INT);
   E_CONFIG_VAL(D, T, size.height, INT);
   E_CONFIG_VAL(D, T, size.width_mm, INT);
   E_CONFIG_VAL(D, T, size.height_mm, INT);
   E_CONFIG_VAL(D, T, orientation, INT);
   E_CONFIG_VAL(D, T, refresh_rate, SHORT);

   _e_config_randr_serialized_output_policy_edd = E_CONFIG_DD_NEW("E_Randr_Serialized_Output_Policy", E_Randr_Serialized_Output_Policy);
#undef T
#undef D
#define T E_Randr_Serialized_Output_Policy
#define D _e_config_randr_serialized_output_policy_edd
   E_CONFIG_VAL(D, T, name, STR);
   E_CONFIG_VAL(D, T, policy, INT);

   _e_config_randr_serialized_output_edd = E_CONFIG_DD_NEW("E_Randr_Serialized_Output", E_Randr_Serialized_Output);
#undef T
#undef D
#define T E_Randr_Serialized_Output
#define D _e_config_randr_serialized_output_edd
   E_CONFIG_VAL(D, T, name, STR);
   E_CONFIG_VAL(D, T, backlight_level, DOUBLE);

   _e_config_randr_mode_info_edd = E_CONFIG_DD_NEW("Ecore_X_Randr_Mode_Info", Ecore_X_Randr_Mode_Info);
#undef T
#undef D
#define T Ecore_X_Randr_Mode_Info
#define D _e_config_randr_mode_info_edd
   E_CONFIG_VAL(D, T, xid, INT);
   E_CONFIG_VAL(D, T, width, INT);
   E_CONFIG_VAL(D, T, height, INT);
   E_CONFIG_VAL(D, T, dotClock, LL);
   E_CONFIG_VAL(D, T, hSyncStart, INT);
   E_CONFIG_VAL(D, T, hSyncEnd, INT);
   E_CONFIG_VAL(D, T, hTotal, INT);
   E_CONFIG_VAL(D, T, hSkew, INT);
   E_CONFIG_VAL(D, T, vSyncStart, INT);
   E_CONFIG_VAL(D, T, vSyncEnd, INT);
   E_CONFIG_VAL(D, T, vTotal, INT);
   E_CONFIG_VAL(D, T, name, STR);
   E_CONFIG_VAL(D, T, nameLength, INT);
   /* Work around a possible ABI break due to poor type choice. */
   if (sizeof (int) == sizeof (unsigned long))
     E_CONFIG_VAL(D, T, modeFlags, INT);
   else if (sizeof (unsigned long long) == sizeof (unsigned long))
     E_CONFIG_VAL(D, T, modeFlags, LL);

   _e_config_randr_serialized_crtc_edd = E_CONFIG_DD_NEW("E_Randr_Serialized_Crtc", E_Randr_Serialized_Crtc);
#undef T
#undef D
#define T E_Randr_Serialized_Crtc
#define D _e_config_randr_serialized_crtc_edd
   E_CONFIG_LIST(D, T, outputs, _e_config_randr_serialized_output_edd);
   E_CONFIG_SUB(D, T, mode_info, _e_config_randr_mode_info_edd);
   E_CONFIG_VAL(D, T, index, INT);
   E_CONFIG_VAL(D, T, pos.x, INT);
   E_CONFIG_VAL(D, T, pos.y, INT);
   E_CONFIG_VAL(D, T, orientation, INT);

   _e_config_randr_serialized_setup_12_edd = E_CONFIG_DD_NEW("E_Randr_Serialized_Setup_12", E_Randr_Serialized_Setup_12);
#undef T
#undef D
#define T E_Randr_Serialized_Setup_12
#define D _e_config_randr_serialized_setup_12_edd
   E_CONFIG_VAL(D, T, timestamp, DOUBLE);
   E_CONFIG_LIST(D, T, crtcs, _e_config_randr_serialized_crtc_edd);
   E_CONFIG_LIST(D, T, edid_hashes, _e_config_randr_edid_hash_edd);

   _e_config_randr_serialized_setup_edd = E_CONFIG_DD_NEW("E_Randr_Serialized_Setup", E_Randr_Serialized_Setup);
#undef T
#undef D
#define T E_Randr_Serialized_Setup
#define D _e_config_randr_serialized_setup_edd
   E_CONFIG_SUB(D, T, serialized_setup_11, _e_config_randr_serialized_setup_11_edd);
   E_CONFIG_LIST(D, T, serialized_setups_12, _e_config_randr_serialized_setup_12_edd);
   E_CONFIG_LIST(D, T, outputs_policies, _e_config_randr_serialized_output_policy_edd);

   _e_config_xkb_layout_edd = E_CONFIG_DD_NEW("E_Config_XKB_Layout",
                                              E_Config_XKB_Layout);
#undef T
#undef D
#define T E_Config_XKB_Layout
#define D _e_config_xkb_layout_edd
   E_CONFIG_VAL(D, T, name, STR);
   E_CONFIG_VAL(D, T, model, STR);
   E_CONFIG_VAL(D, T, variant, STR);

   _e_config_xkb_option_edd = E_CONFIG_DD_NEW("E_Config_XKB_Option",
                                              E_Config_XKB_Option);
#undef T
#undef D
#define T E_Config_XKB_Option
#define D _e_config_xkb_option_edd
   E_CONFIG_VAL(D, T, name, STR);

   _e_config_edd = E_CONFIG_DD_NEW("E_Config", E_Config);
#undef T
#undef D
#define T E_Config
#define D _e_config_edd
   /**/ /* == already configurable via ipc */
   E_CONFIG_VAL(D, T, config_version, INT); /**/
   E_CONFIG_VAL(D, T, show_splash, INT); /**/
   E_CONFIG_VAL(D, T, init_default_theme, STR); /**/
   E_CONFIG_VAL(D, T, desktop_default_background, STR); /**/
   E_CONFIG_VAL(D, T, desktop_default_name, STR); /**/
#ifdef _F_USE_DESK_WINDOW_PROFILE_
   E_CONFIG_VAL(D, T, desktop_default_window_profile, STR); /**/
#endif
   E_CONFIG_LIST(D, T, desktop_backgrounds, _e_config_desktop_bg_edd); /**/
   E_CONFIG_LIST(D, T, desktop_names, _e_config_desktop_name_edd); /**/
#ifdef _F_USE_DESK_WINDOW_PROFILE_
   E_CONFIG_LIST(D, T, desktop_window_profiles, _e_config_desktop_window_profile_edd); /**/
#endif
#ifdef _F_USE_TILED_DESK_LAYOUT_
   E_CONFIG_LIST(D, T, desktop_layouts, _e_config_desktop_layout_edd); /**/
#endif /* end of _F_USE_TILED_DESK_LAYOUT_ */
   E_CONFIG_VAL(D, T, menus_scroll_speed, DOUBLE); /**/
   E_CONFIG_VAL(D, T, menus_fast_mouse_move_threshhold, DOUBLE); /**/
   E_CONFIG_VAL(D, T, menus_click_drag_timeout, DOUBLE); /**/
   E_CONFIG_VAL(D, T, border_shade_animate, INT); /**/
   E_CONFIG_VAL(D, T, border_shade_transition, INT); /**/
   E_CONFIG_VAL(D, T, border_shade_speed, DOUBLE); /**/
   E_CONFIG_VAL(D, T, framerate, DOUBLE); /**/
   E_CONFIG_VAL(D, T, priority, INT); /**/
   E_CONFIG_VAL(D, T, image_cache, INT); /**/
   E_CONFIG_VAL(D, T, font_cache, INT); /**/
   E_CONFIG_VAL(D, T, edje_cache, INT); /**/
   E_CONFIG_VAL(D, T, edje_collection_cache, INT); /**/
   E_CONFIG_VAL(D, T, zone_desks_x_count, INT); /**/
   E_CONFIG_VAL(D, T, zone_desks_y_count, INT); /**/
#ifdef _F_USE_WM_ZONE_HOST_
   E_CONFIG_VAL(D, T, tizen_zone_active, INT); /**/
#endif
   E_CONFIG_VAL(D, T, show_desktop_icons, INT); /**/
   E_CONFIG_VAL(D, T, edge_flip_dragging, INT); /**/
   E_CONFIG_VAL(D, T, use_composite, INT); /**/
   E_CONFIG_VAL(D, T, language, STR); /**/
   E_CONFIG_LIST(D, T, modules, _e_config_module_edd); /**/
   E_CONFIG_LIST(D, T, font_fallbacks, _e_config_font_fallback_edd); /**/
   E_CONFIG_LIST(D, T, font_defaults, _e_config_font_default_edd); /**/
   E_CONFIG_LIST(D, T, themes, _e_config_theme_edd); /**/
   E_CONFIG_LIST(D, T, mouse_bindings, _e_config_bindings_mouse_edd); /**/
   E_CONFIG_LIST(D, T, key_bindings, _e_config_bindings_key_edd); /**/
   E_CONFIG_LIST(D, T, edge_bindings, _e_config_bindings_edge_edd); /**/
   E_CONFIG_LIST(D, T, signal_bindings, _e_config_bindings_signal_edd); /**/
   E_CONFIG_LIST(D, T, wheel_bindings, _e_config_bindings_wheel_edd); /**/
   E_CONFIG_LIST(D, T, acpi_bindings, _e_config_bindings_acpi_edd); /**/
   E_CONFIG_LIST(D, T, path_append_data, _e_config_path_append_edd); /**/
   E_CONFIG_LIST(D, T, path_append_images, _e_config_path_append_edd); /**/
   E_CONFIG_LIST(D, T, path_append_fonts, _e_config_path_append_edd); /**/
   E_CONFIG_LIST(D, T, path_append_themes, _e_config_path_append_edd); /**/
   E_CONFIG_LIST(D, T, path_append_init, _e_config_path_append_edd); /**/
   E_CONFIG_LIST(D, T, path_append_icons, _e_config_path_append_edd); /**/
   E_CONFIG_LIST(D, T, path_append_modules, _e_config_path_append_edd); /**/
   E_CONFIG_LIST(D, T, path_append_backgrounds, _e_config_path_append_edd); /**/
   E_CONFIG_VAL(D, T, window_placement_policy, INT); /**/
   E_CONFIG_VAL(D, T, window_grouping, INT); /**/
   E_CONFIG_VAL(D, T, focus_policy, INT); /**/
   E_CONFIG_VAL(D, T, focus_setting, INT); /**/
   E_CONFIG_VAL(D, T, pass_click_on, INT); /**/
   E_CONFIG_VAL(D, T, always_click_to_raise, INT); /**/
   E_CONFIG_VAL(D, T, always_click_to_focus, INT); /**/
   E_CONFIG_VAL(D, T, use_auto_raise, INT); /**/
   E_CONFIG_VAL(D, T, auto_raise_delay, DOUBLE); /**/
   E_CONFIG_VAL(D, T, use_resist, INT); /**/
   E_CONFIG_VAL(D, T, drag_resist, INT); /**/
   E_CONFIG_VAL(D, T, desk_resist, INT); /**/
   E_CONFIG_VAL(D, T, window_resist, INT); /**/
   E_CONFIG_VAL(D, T, gadget_resist, INT); /**/
   E_CONFIG_VAL(D, T, geometry_auto_resize_limit, INT); /**/
   E_CONFIG_VAL(D, T, geometry_auto_move, INT); /**/
   E_CONFIG_VAL(D, T, winlist_warp_while_selecting, INT); /**/
   E_CONFIG_VAL(D, T, winlist_warp_at_end, INT); /**/
   E_CONFIG_VAL(D, T, winlist_warp_speed, DOUBLE); /**/
   E_CONFIG_VAL(D, T, winlist_scroll_animate, INT); /**/
   E_CONFIG_VAL(D, T, winlist_scroll_speed, DOUBLE); /**/
   E_CONFIG_VAL(D, T, winlist_list_show_iconified, INT); /**/
   E_CONFIG_VAL(D, T, winlist_list_show_other_desk_iconified, INT); /**/
   E_CONFIG_VAL(D, T, winlist_list_show_other_screen_iconified, INT); /**/
   E_CONFIG_VAL(D, T, winlist_list_show_other_desk_windows, INT); /**/
   E_CONFIG_VAL(D, T, winlist_list_show_other_screen_windows, INT); /**/
   E_CONFIG_VAL(D, T, winlist_list_uncover_while_selecting, INT); /**/
   E_CONFIG_VAL(D, T, winlist_list_jump_desk_while_selecting, INT); /**/
   E_CONFIG_VAL(D, T, winlist_list_focus_while_selecting, INT); /**/
   E_CONFIG_VAL(D, T, winlist_list_raise_while_selecting, INT); /**/
   E_CONFIG_VAL(D, T, winlist_pos_align_x, DOUBLE); /**/
   E_CONFIG_VAL(D, T, winlist_pos_align_y, DOUBLE); /**/
   E_CONFIG_VAL(D, T, winlist_pos_size_w, DOUBLE); /**/
   E_CONFIG_VAL(D, T, winlist_pos_size_h, DOUBLE); /**/
   E_CONFIG_VAL(D, T, winlist_pos_min_w, INT); /**/
   E_CONFIG_VAL(D, T, winlist_pos_min_h, INT); /**/
   E_CONFIG_VAL(D, T, winlist_pos_max_w, INT); /**/
   E_CONFIG_VAL(D, T, winlist_pos_max_h, INT); /**/
   E_CONFIG_VAL(D, T, maximize_policy, INT); /**/
   E_CONFIG_VAL(D, T, allow_manip, INT); /**/
   E_CONFIG_VAL(D, T, border_fix_on_shelf_toggle, INT); /**/
   E_CONFIG_VAL(D, T, allow_above_fullscreen, INT); /**/
   E_CONFIG_VAL(D, T, kill_if_close_not_possible, INT); /**/
   E_CONFIG_VAL(D, T, kill_process, INT); /**/
   E_CONFIG_VAL(D, T, kill_hung_process_by_module, INT); /**/
   E_CONFIG_VAL(D, T, kill_timer_wait, DOUBLE); /**/
   E_CONFIG_VAL(D, T, ping_clients, INT); /**/
   E_CONFIG_VAL(D, T, transition_start, STR); /**/
   E_CONFIG_VAL(D, T, transition_desk, STR); /**/
   E_CONFIG_VAL(D, T, transition_change, STR); /**/
   E_CONFIG_LIST(D, T, remembers, _e_config_remember_edd);
   E_CONFIG_VAL(D, T, remember_internal_windows, INT);
   E_CONFIG_VAL(D, T, move_info_follows, INT); /**/
   E_CONFIG_VAL(D, T, resize_info_follows, INT); /**/
   E_CONFIG_VAL(D, T, move_info_visible, INT); /**/
   E_CONFIG_VAL(D, T, resize_info_visible, INT); /**/
   E_CONFIG_VAL(D, T, focus_last_focused_per_desktop, INT); /**/
   E_CONFIG_VAL(D, T, focus_revert_on_hide_or_close, INT); /**/
   E_CONFIG_VAL(D, T, pointer_slide, INT); /**/
   E_CONFIG_VAL(D, T, use_e_cursor, INT); /**/
   E_CONFIG_VAL(D, T, cursor_size, INT); /**/
   E_CONFIG_VAL(D, T, menu_autoscroll_margin, INT); /**/
   E_CONFIG_VAL(D, T, menu_autoscroll_cursor_margin, INT); /**/
   E_CONFIG_VAL(D, T, transient.move, INT); /* FIXME: implement */
   E_CONFIG_VAL(D, T, transient.resize, INT); /* FIXME: implement */
   E_CONFIG_VAL(D, T, transient.raise, INT); /**/
   E_CONFIG_VAL(D, T, transient.lower, INT); /**/
   E_CONFIG_VAL(D, T, transient.layer, INT); /**/
   E_CONFIG_VAL(D, T, transient.desktop, INT); /**/
   E_CONFIG_VAL(D, T, transient.iconify, INT); /**/
   E_CONFIG_VAL(D, T, modal_windows, INT); /**/
   E_CONFIG_VAL(D, T, menu_eap_name_show, INT); /**/
   E_CONFIG_VAL(D, T, menu_eap_generic_show, INT); /**/
   E_CONFIG_VAL(D, T, menu_eap_comment_show, INT); /**/
   E_CONFIG_VAL(D, T, fullscreen_policy, INT); /**/
   E_CONFIG_VAL(D, T, input_method, STR); /**/
   E_CONFIG_LIST(D, T, path_append_messages, _e_config_path_append_edd); /**/
   E_CONFIG_VAL(D, T, exebuf_term_cmd, STR);
   E_CONFIG_LIST(D, T, color_classes, _e_config_color_class_edd);
   E_CONFIG_VAL(D, T, use_app_icon, INT);
   E_CONFIG_VAL(D, T, cnfmdlg_disabled, INT); /**/
   E_CONFIG_VAL(D, T, cfgdlg_auto_apply, INT); /**/
   E_CONFIG_VAL(D, T, cfgdlg_default_mode, INT); /**/
   E_CONFIG_LIST(D, T, gadcons, _e_config_gadcon_edd);
   E_CONFIG_LIST(D, T, shelves, _e_config_shelf_edd);
   E_CONFIG_VAL(D, T, font_hinting, INT); /**/
   E_CONFIG_VAL(D, T, desklock_personal_passwd, STR);
   E_CONFIG_VAL(D, T, desklock_background, STR);
   E_CONFIG_LIST(D, T, desklock_backgrounds, _e_config_desklock_bg_edd); /**/
   E_CONFIG_VAL(D, T, desklock_auth_method, INT);
   E_CONFIG_VAL(D, T, desklock_login_box_zone, INT);
   E_CONFIG_VAL(D, T, desklock_start_locked, INT);
   E_CONFIG_VAL(D, T, desklock_on_suspend, INT);
   E_CONFIG_VAL(D, T, desklock_autolock_screensaver, INT);
   E_CONFIG_VAL(D, T, desklock_post_screensaver_time, DOUBLE);
   E_CONFIG_VAL(D, T, desklock_autolock_idle, INT);
   E_CONFIG_VAL(D, T, desklock_autolock_idle_timeout, DOUBLE);
   E_CONFIG_VAL(D, T, desklock_use_custom_desklock, INT);
   E_CONFIG_VAL(D, T, desklock_custom_desklock_cmd, STR);
   E_CONFIG_VAL(D, T, desklock_ask_presentation, UCHAR);
   E_CONFIG_VAL(D, T, desklock_ask_presentation_timeout, DOUBLE);

   //randr specifics
   E_CONFIG_SUB(D, T, randr_serialized_setup, _e_config_randr_serialized_setup_edd);

   E_CONFIG_VAL(D, T, screensaver_enable, INT);
   E_CONFIG_VAL(D, T, screensaver_timeout, INT);
   E_CONFIG_VAL(D, T, screensaver_interval, INT);
   E_CONFIG_VAL(D, T, screensaver_blanking, INT);
   E_CONFIG_VAL(D, T, screensaver_expose, INT);
   E_CONFIG_VAL(D, T, screensaver_ask_presentation, UCHAR);
   E_CONFIG_VAL(D, T, screensaver_ask_presentation_timeout, DOUBLE);

   E_CONFIG_VAL(D, T, screensaver_suspend, UCHAR);
   E_CONFIG_VAL(D, T, screensaver_suspend_on_ac, UCHAR);
   E_CONFIG_VAL(D, T, screensaver_suspend_delay, DOUBLE);

   E_CONFIG_VAL(D, T, dpms_enable, INT);
   E_CONFIG_VAL(D, T, dpms_standby_enable, INT);
   E_CONFIG_VAL(D, T, dpms_suspend_enable, INT);
   E_CONFIG_VAL(D, T, dpms_off_enable, INT);
   E_CONFIG_VAL(D, T, dpms_standby_timeout, INT);
   E_CONFIG_VAL(D, T, dpms_suspend_timeout, INT);
   E_CONFIG_VAL(D, T, dpms_off_timeout, INT);

   E_CONFIG_VAL(D, T, clientlist_group_by, INT);
   E_CONFIG_VAL(D, T, clientlist_include_all_zones, INT);
   E_CONFIG_VAL(D, T, clientlist_separate_with, INT);
   E_CONFIG_VAL(D, T, clientlist_sort_by, INT);
   E_CONFIG_VAL(D, T, clientlist_separate_iconified_apps, INT);
   E_CONFIG_VAL(D, T, clientlist_warp_to_iconified_desktop, INT);
   E_CONFIG_VAL(D, T, clientlist_limit_caption_len, INT);
   E_CONFIG_VAL(D, T, clientlist_max_caption_len, INT);

   E_CONFIG_VAL(D, T, mouse_hand, INT);
   E_CONFIG_VAL(D, T, mouse_accel_numerator, INT);
   E_CONFIG_VAL(D, T, mouse_accel_denominator, INT);
   E_CONFIG_VAL(D, T, mouse_accel_threshold, INT);

   E_CONFIG_VAL(D, T, border_raise_on_mouse_action, INT);
   E_CONFIG_VAL(D, T, border_raise_on_focus, INT);
   E_CONFIG_VAL(D, T, desk_flip_wrap, INT);
   E_CONFIG_VAL(D, T, fullscreen_flip, INT);
   E_CONFIG_VAL(D, T, multiscreen_flip, INT);

   E_CONFIG_VAL(D, T, icon_theme, STR);
   E_CONFIG_VAL(D, T, icon_theme_overrides, UCHAR);

   E_CONFIG_VAL(D, T, desk_flip_animate_mode, INT);
   E_CONFIG_VAL(D, T, desk_flip_animate_interpolation, INT);
   E_CONFIG_VAL(D, T, desk_flip_animate_time, DOUBLE);

   E_CONFIG_VAL(D, T, wallpaper_import_last_dev, STR);
   E_CONFIG_VAL(D, T, wallpaper_import_last_path, STR);

   E_CONFIG_VAL(D, T, theme_default_border_style, STR);

   E_CONFIG_LIST(D, T, mime_icons, _e_config_mime_icon_edd); /**/

   E_CONFIG_VAL(D, T, desk_auto_switch, INT);

   E_CONFIG_VAL(D, T, thumb_nice, INT);

   E_CONFIG_VAL(D, T, menu_favorites_show, INT);
   E_CONFIG_VAL(D, T, menu_apps_show, INT);
   E_CONFIG_VAL(D, T, menu_gadcon_client_toplevel, INT);

   E_CONFIG_VAL(D, T, ping_clients_interval, INT);
   E_CONFIG_VAL(D, T, cache_flush_poll_interval, INT);

   E_CONFIG_VAL(D, T, thumbscroll_enable, INT);
   E_CONFIG_VAL(D, T, thumbscroll_threshhold, INT);
   E_CONFIG_VAL(D, T, thumbscroll_momentum_threshhold, DOUBLE);
   E_CONFIG_VAL(D, T, thumbscroll_friction, DOUBLE);

   E_CONFIG_VAL(D, T, filemanager_single_click, UCHAR);
   E_CONFIG_VAL(D, T, device_desktop, INT);
   E_CONFIG_VAL(D, T, device_auto_mount, INT);
   E_CONFIG_VAL(D, T, device_auto_open, INT);
   E_CONFIG_VAL(D, T, filemanager_copy, UCHAR);

   E_CONFIG_VAL(D, T, border_keyboard.timeout, DOUBLE);
   E_CONFIG_VAL(D, T, border_keyboard.move.dx, UCHAR);
   E_CONFIG_VAL(D, T, border_keyboard.move.dy, UCHAR);
   E_CONFIG_VAL(D, T, border_keyboard.resize.dx, UCHAR);
   E_CONFIG_VAL(D, T, border_keyboard.resize.dy, UCHAR);

   E_CONFIG_VAL(D, T, scale.min, DOUBLE);
   E_CONFIG_VAL(D, T, scale.max, DOUBLE);
   E_CONFIG_VAL(D, T, scale.factor, DOUBLE);
   E_CONFIG_VAL(D, T, scale.base_dpi, INT);
   E_CONFIG_VAL(D, T, scale.use_dpi, UCHAR);
   E_CONFIG_VAL(D, T, scale.use_custom, UCHAR);

   E_CONFIG_VAL(D, T, show_cursor, UCHAR);
   E_CONFIG_VAL(D, T, idle_cursor, UCHAR);

   E_CONFIG_VAL(D, T, default_system_menu, STR);

   E_CONFIG_VAL(D, T, cfgdlg_normal_wins, UCHAR);

   E_CONFIG_VAL(D, T, syscon.main.icon_size, INT);
   E_CONFIG_VAL(D, T, syscon.secondary.icon_size, INT);
   E_CONFIG_VAL(D, T, syscon.extra.icon_size, INT);
   E_CONFIG_VAL(D, T, syscon.timeout, DOUBLE);
   E_CONFIG_VAL(D, T, syscon.do_input, UCHAR);
   E_CONFIG_LIST(D, T, syscon.actions, _e_config_syscon_action_edd);

   E_CONFIG_VAL(D, T, mode.presentation, UCHAR);
   E_CONFIG_VAL(D, T, mode.offline, UCHAR);

   E_CONFIG_VAL(D, T, exec.expire_timeout, DOUBLE);
   E_CONFIG_VAL(D, T, exec.show_run_dialog, UCHAR);
   E_CONFIG_VAL(D, T, exec.show_exit_dialog, UCHAR);

   E_CONFIG_VAL(D, T, null_container_win, UCHAR);

   E_CONFIG_LIST(D, T, env_vars, _e_config_env_var_edd);

   E_CONFIG_VAL(D, T, backlight.normal, DOUBLE);
   E_CONFIG_VAL(D, T, backlight.dim, DOUBLE);
   E_CONFIG_VAL(D, T, backlight.transition, DOUBLE);
   E_CONFIG_VAL(D, T, backlight.timer, DOUBLE);
   E_CONFIG_VAL(D, T, backlight.sysdev, STR);
   E_CONFIG_VAL(D, T, backlight.idle_dim, UCHAR);

   E_CONFIG_VAL(D, T, deskenv.load_xrdb, UCHAR);
   E_CONFIG_VAL(D, T, deskenv.load_xmodmap, UCHAR);
   E_CONFIG_VAL(D, T, deskenv.load_gnome, UCHAR);
   E_CONFIG_VAL(D, T, deskenv.load_kde, UCHAR);

   E_CONFIG_VAL(D, T, powersave.none, DOUBLE);
   E_CONFIG_VAL(D, T, powersave.low, DOUBLE);
   E_CONFIG_VAL(D, T, powersave.medium, DOUBLE);
   E_CONFIG_VAL(D, T, powersave.high, DOUBLE);
   E_CONFIG_VAL(D, T, powersave.extreme, DOUBLE);
   E_CONFIG_VAL(D, T, powersave.min, INT);
   E_CONFIG_VAL(D, T, powersave.max, INT);

   E_CONFIG_VAL(D, T, xsettings.enabled, UCHAR);
   E_CONFIG_VAL(D, T, xsettings.match_e17_theme, UCHAR);
   E_CONFIG_VAL(D, T, xsettings.match_e17_icon_theme, UCHAR);
   E_CONFIG_VAL(D, T, xsettings.xft_antialias, INT);
   E_CONFIG_VAL(D, T, xsettings.xft_hinting, INT);
   E_CONFIG_VAL(D, T, xsettings.xft_hint_style, STR);
   E_CONFIG_VAL(D, T, xsettings.xft_rgba, STR);
   E_CONFIG_VAL(D, T, xsettings.net_theme_name, STR);
   E_CONFIG_VAL(D, T, xsettings.net_icon_theme_name, STR);
   E_CONFIG_VAL(D, T, xsettings.gtk_font_name, STR);

   E_CONFIG_VAL(D, T, update.check, UCHAR);
   E_CONFIG_VAL(D, T, update.later, UCHAR);

   E_CONFIG_LIST(D, T, xkb.used_layouts, _e_config_xkb_layout_edd);
   E_CONFIG_LIST(D, T, xkb.used_options, _e_config_xkb_option_edd);
   E_CONFIG_VAL(D, T, xkb.only_label, INT);
   E_CONFIG_VAL(D, T, xkb.default_model, STR);
   //E_CONFIG_VAL(D, T, xkb.cur_group, INT);

   E_CONFIG_VAL(D, T, exe_always_single_instance, UCHAR);
#ifdef _F_USE_DESK_WINDOW_PROFILE_
   E_CONFIG_VAL(D, T, use_desktop_window_profile, INT);
#endif
#ifdef _F_ZONE_WINDOW_ROTATION_
   E_CONFIG_VAL(D, T, wm_win_rotation, UCHAR);
#endif

#ifdef _F_DEICONIFY_APPROVE_
   E_CONFIG_VAL(D, T, deiconify_approve, UCHAR);
   E_CONFIG_VAL(D, T, deiconify_timeout, DOUBLE);
#endif

#ifdef _F_USE_TILED_DESK_LAYOUT_
   E_CONFIG_VAL(D, T, use_tiled_desk_layout, UCHAR);
   E_CONFIG_VAL(D, T, max_tiled_layout, UCHAR);
#endif /* end of _F_USE_TILED_DESK_LAYOUT_ */
#ifdef _F_USE_BORDER_TRANSFORM_
   E_CONFIG_VAL(D, T, use_border_transform, UCHAR);
#endif /* end of _F_USE_BORDER_TRANSFORM_ */

#ifdef _F_E_WIN_AUX_HINT_
   E_CONFIG_VAL(D, T, win_aux_hint, UCHAR);
#endif /* end of _F_E_WIN_AUX_HINT_ */

#ifdef _F_USE_ICONIFY_RESIZE_
   E_CONFIG_VAL(D, T, use_iconify_resize, UCHAR);
   E_CONFIG_VAL(D, T, iconify_resize_w, INT);
   E_CONFIG_VAL(D, T, iconify_resize_h, INT);
#endif

   E_CONFIG_VAL(D, T, border_raise_on_focus, UCHAR);
   E_CONFIG_VAL(D, T, max_hung_count, INT); /**/

   E_CONFIG_VAL(D, T, border_drag_margin_x, INT);
   E_CONFIG_VAL(D, T, border_drag_margin_y, INT);

   E_CONFIG_VAL(D, T, ping_only_visible, UCHAR);
   E_CONFIG_VAL(D, T, use_e_test_runner, INT);
   E_CONFIG_VAL(D, T, stand_by_tv_viewer_ready, INT);

   e_config_load();

   e_config_save_queue();
   return 1;
}

EINTERN int
e_config_shutdown(void)
{
   eina_stringshare_del(_e_config_profile);
   E_CONFIG_DD_FREE(_e_config_edd);
   E_CONFIG_DD_FREE(_e_config_module_edd);
   E_CONFIG_DD_FREE(_e_config_font_default_edd);
   E_CONFIG_DD_FREE(_e_config_font_fallback_edd);
   E_CONFIG_DD_FREE(_e_config_theme_edd);
   E_CONFIG_DD_FREE(_e_config_bindings_mouse_edd);
   E_CONFIG_DD_FREE(_e_config_bindings_key_edd);
   E_CONFIG_DD_FREE(_e_config_bindings_edge_edd);
   E_CONFIG_DD_FREE(_e_config_bindings_signal_edd);
   E_CONFIG_DD_FREE(_e_config_bindings_wheel_edd);
   E_CONFIG_DD_FREE(_e_config_bindings_acpi_edd);
   E_CONFIG_DD_FREE(_e_config_path_append_edd);
   E_CONFIG_DD_FREE(_e_config_desktop_bg_edd);
   E_CONFIG_DD_FREE(_e_config_desklock_bg_edd);
   E_CONFIG_DD_FREE(_e_config_desktop_name_edd);
#ifdef _F_USE_DESK_WINDOW_PROFILE_
   E_CONFIG_DD_FREE(_e_config_desktop_window_profile_edd);
#endif
#ifdef _F_USE_TILED_DESK_LAYOUT_
   E_CONFIG_DD_FREE(_e_config_desktop_layout_edd);
#endif /* end of _F_USE_TILED_DESK_LAYOUT_ */
   E_CONFIG_DD_FREE(_e_config_remember_edd);
   E_CONFIG_DD_FREE(_e_config_gadcon_edd);
   E_CONFIG_DD_FREE(_e_config_gadcon_client_edd);
   E_CONFIG_DD_FREE(_e_config_shelf_edd);
   E_CONFIG_DD_FREE(_e_config_shelf_desk_edd);
   E_CONFIG_DD_FREE(_e_config_mime_icon_edd);
   E_CONFIG_DD_FREE(_e_config_syscon_action_edd);
   E_CONFIG_DD_FREE(_e_config_env_var_edd);
   E_CONFIG_DD_FREE(_e_config_xkb_layout_edd);
   E_CONFIG_DD_FREE(_e_config_xkb_option_edd);
   //E_CONFIG_DD_FREE(_e_config_randr_serialized_setup_edd);
   return 1;
}

EAPI void
e_config_load(void)
{
   E_Config *tcfg = NULL;

#ifdef _F_COPY_FROM_DATA_DIR_
   char buf[4096];
   char buf1[4096], buf2[4096];
   FILE *f;

   e_user_dir_snprintf(buf, sizeof(buf), "config/version.txt");
   LOGE("%s", buf);
   if (!ecore_file_exists(buf))
     {
        LOGE("FILE %s : does not exist, copy files and load e_config...", buf);
        e_prefix_data_snprintf(buf, sizeof(buf), "config/e/config/version.txt");
        if (ecore_file_exists(buf))
          {
             _e_config_domain_data_update("e");
             ELB(ELBT_DFT, "DOMAIN_LOAD", 0);
          }
     }
   else
     {
        memset(buf1, 0, sizeof(buf1));
        memset(buf2, 0, sizeof(buf2));

        // read and compare version of
        // "/usr/share/enlightenment/config/e/config/version.txt" &
        // "/home/app/.e/e/config/version.txt"
        // buf already points to -> "/home/app/.e/e/config/version.txt"
        LOGE("FILE %s", buf);
        f = fopen(buf, "r");
        if (f)
          {
             fgets(buf1, sizeof(buf1), f);
             LOGE("FILE %s : version %s", buf, buf1);
             fclose(f);
          }
        e_prefix_data_snprintf(buf, sizeof(buf), "config/e/config/version.txt");
        LOGE("FILE %s", buf);
        f = fopen(buf, "r");
        if (f)
          {
             fgets(buf2, sizeof(buf2), f);
             LOGE("FILE %s : version %s", buf, buf2);
             fclose(f);
          }

        if (strncmp(buf1, buf2, strlen(buf1)))
          {
             LOGE("Versions mismatch, copy e_config files...");
             _e_config_domain_data_update("e");
             ELB(ELBT_DFT, "DOMAIN_LOAD", 0);
          }
     }
#endif

   e_config = e_config_domain_load("e", _e_config_edd);

#ifndef _F_COPY_FROM_DATA_DIR_
   if (e_config)
     {
        int reload = 0;

        /* major version change - that means wipe and restart */
        if ((e_config->config_version >> 16) < E_CONFIG_FILE_EPOCH)
          {
             /* your config is too old - need new defaults */
             _e_config_free(e_config);
             e_config = NULL;
             reload = 1;
             ecore_timer_add(1.0, _e_config_cb_timer,
                             _("Settings data needed upgrading. Your old settings have<br>"
                               "been wiped and a new set of defaults initialized. This<br>"
                               "will happen regularly during development, so don't report a<br>"
                               "bug. This simply means Enlightenment needs new settings<br>"
                               "data by default for usable functionality that your old<br>"
                               "settings simply lack. This new set of defaults will fix<br>"
                               "that by adding it in. You can re-configure things now to your<br>"
                               "liking. Sorry for the hiccup in your settings.<br>"));
          }
        /* config is too new? odd! suspect corruption? */
        else if (e_config->config_version > E_CONFIG_FILE_VERSION)
          {
             /* your config is too new - what the fuck??? */
             _e_config_free(e_config);
             e_config = NULL;
             reload = 1;
             ecore_timer_add(1.0, _e_config_cb_timer,
                             _("Your settings are NEWER than Enlightenment. This is very<br>"
                               "strange. This should not happen unless you downgraded<br>"
                               "Enlightenment or copied the settings from a place where<br>"
                               "a newer version of Enlightenment was running. This is bad and<br>"
                               "as a precaution your settings have been now restored to<br>"
                               "defaults. Sorry for the inconvenience.<br>"));
          }
        /* oldest minor version supported */
        else if ((e_config->config_version & 0xffff) < 0x0124)
          {
             /* your config is so old - we don't even bother supporting an
              * upgrade path - brand new config for you! */
             _e_config_free(e_config);
             e_config = NULL;
             reload = 1;
             ecore_timer_add(1.0, _e_config_cb_timer,
                             _("Settings data needed upgrading. Your old settings have<br>"
                               "been wiped and a new set of defaults initialized. This<br>"
                               "will happen regularly during development, so don't report a<br>"
                               "bug. This simply means Enlightenment needs new settings<br>"
                               "data by default for usable functionality that your old<br>"
                               "settings simply lack. This new set of defaults will fix<br>"
                               "that by adding it in. You can re-configure things now to your<br>"
                               "liking. Sorry for the hiccup in your settings.<br>"));
          }
        if (reload)
          {
             e_config_profile_del(e_config_profile_get());
             e_config = e_config_domain_load("e", _e_config_edd);
          }
     }
#endif /* _F_COPY_FROM_DATA_DIR_ */
   if (!e_config)
     {
        ERR("EEEK! no config of any sort! abort abort abort!");
        e_error_message_show("Enlightenment was started without any configuration\n"
                             "files available for the given profile (normally\n"
                             "default or the last profile used or provided on the\n"
                             "command-line with -profile etc.)\n\n"
                             "Cannot contiue without configuration to work with.\n"
                             "Please ensure you have system or user configuration\n"
                             "for the profile you are using before proceeeding.");
        abort();
     }
#ifndef _F_COPY_FROM_DATA_DIR_
   if (e_config->config_version < E_CONFIG_FILE_VERSION)
     {
        /* we need an upgrade of some sort */
        tcfg = e_config_domain_system_load("e", _e_config_edd);
        if (!tcfg)
          {
             const char *pprofile;

             pprofile = e_config_profile_get();
             if (pprofile) pprofile = eina_stringshare_add(pprofile);
             e_config_profile_set("standard");
             tcfg = e_config_domain_system_load("e", _e_config_edd);
             e_config_profile_set(pprofile);
             if (pprofile) eina_stringshare_del(pprofile);
          }
        /* can't find your profile or standard or default - try default after
         * a wipe */
        if (!tcfg)
          {
             E_Action *a;

             e_config_profile_set("default");
             e_config_profile_del(e_config_profile_get());
             e_config_save_block_set(1);
             a = e_action_find("restart");
             if ((a) && (a->func.go)) a->func.go(NULL, NULL);
          }
     }
#define IFCFG(v)   if ((e_config->config_version & 0xffff) < (v)) {
#define IFCFGELSE } else {
#define IFCFGEND  }
#define COPYVAL(x) do {e_config->x = tcfg->x; } while (0)
#define COPYPTR(x) do {e_config->x = tcfg->x; tcfg->x = NULL; } while (0)
#define COPYSTR(x) COPYPTR(x)
     if (tcfg)
       {
          /* some sort of upgrade is needed */
          IFCFG(0x0124);
          COPYVAL(thumbscroll_enable);
          COPYVAL(thumbscroll_threshhold);
          COPYVAL(thumbscroll_momentum_threshhold);
          COPYVAL(thumbscroll_friction);
          IFCFGEND;

          IFCFG(0x0125);
          COPYVAL(mouse_hand);
          IFCFGEND;

          IFCFG(0x0126);
          COPYVAL(border_keyboard.timeout);
          COPYVAL(border_keyboard.move.dx);
          COPYVAL(border_keyboard.move.dy);
          COPYVAL(border_keyboard.resize.dx);
          COPYVAL(border_keyboard.resize.dy);
          IFCFGEND;

          IFCFG(0x0127);
          COPYVAL(scale.min);
          COPYVAL(scale.max);
          COPYVAL(scale.factor);
          COPYVAL(scale.base_dpi);
          COPYVAL(scale.use_dpi);
          COPYVAL(scale.use_custom);
          IFCFGEND;

          IFCFG(0x0128);
          COPYVAL(show_cursor);
          COPYVAL(idle_cursor);
          IFCFGEND;

          IFCFG(0x0129);
          COPYSTR(default_system_menu);
          IFCFGEND;

          IFCFG(0x012a);
          COPYVAL(desklock_start_locked);
          IFCFGEND;

          IFCFG(0x012b);
          COPYVAL(cfgdlg_normal_wins);
          IFCFGEND;

          IFCFG(0x012c);
          COPYVAL(syscon.main.icon_size);
          COPYVAL(syscon.secondary.icon_size);
          COPYVAL(syscon.extra.icon_size);
          COPYVAL(syscon.timeout);
          COPYVAL(syscon.do_input);
          COPYPTR(syscon.actions);
          IFCFGEND;

          IFCFG(0x012d);
          COPYVAL(priority);
          IFCFGEND;

          IFCFG(0x012e);
          COPYVAL(fullscreen_flip);
          IFCFGEND;

          IFCFG(0x012f);
          COPYVAL(icon_theme_overrides);
          IFCFGEND;

          IFCFG(0x0130);
          COPYVAL(mode.presentation);
          COPYVAL(mode.offline);
          IFCFGEND;

          IFCFG(0x0131);
          COPYVAL(desklock_post_screensaver_time);
          IFCFGEND;

          IFCFG(0x0132);
          COPYVAL(desklock_ask_presentation);
          COPYVAL(desklock_ask_presentation_timeout);
          COPYVAL(screensaver_ask_presentation);
          COPYVAL(screensaver_ask_presentation_timeout);
          IFCFGEND;

          IFCFG(0x0134);
          COPYVAL(exec.expire_timeout);
          COPYVAL(exec.show_run_dialog);
          COPYVAL(exec.show_exit_dialog);
          IFCFGEND;
          IFCFG(0x0136);
          _e_config_acpi_bindings_add();
          IFCFGEND;

          IFCFG(0x0137);
          COPYVAL(desklock_on_suspend);
          IFCFGEND;

          IFCFG(0x0138);
          COPYVAL(geometry_auto_resize_limit);
          COPYVAL(geometry_auto_move);
          IFCFGEND;

          IFCFG(0x0142);
          COPYVAL(backlight.normal);
          COPYVAL(backlight.dim);
          COPYVAL(backlight.transition);
          COPYVAL(backlight.idle_dim);
          COPYVAL(backlight.timer);
          IFCFGEND;

          IFCFG(0x0145);
          COPYVAL(xsettings.enabled);
          COPYVAL(xsettings.match_e17_theme);
          COPYVAL(xsettings.match_e17_icon_theme);
          IFCFGEND;

          IFCFG(0x0147);
          COPYVAL(update.check);
          COPYVAL(update.later);
          IFCFGEND;

          IFCFG(0x0149);
          COPYVAL(powersave.none);
          COPYVAL(powersave.low);
          COPYVAL(powersave.medium);
          COPYVAL(powersave.high);
          COPYVAL(powersave.extreme);
          COPYVAL(powersave.min);
          COPYVAL(powersave.max);
          IFCFGEND;

          IFCFG(0x0150);
          COPYVAL(multiscreen_flip);
          IFCFGEND;

          IFCFG(0x0151);
          if (tcfg->desklock_background)
            {
               E_Config_Desklock_Background *cbg;
               cbg = E_NEW(E_Config_Desklock_Background, 1);
               cbg->file = tcfg->desklock_background;
               e_config->desklock_backgrounds = eina_list_append(e_config->desklock_backgrounds, cbg);
            }
          if (e_config->desklock_backgrounds && (!e_config->desklock_backgrounds->data))
            e_config->desklock_backgrounds = eina_list_free(e_config->desklock_backgrounds);
          tcfg->desklock_background = NULL;
          IFCFGEND;

          IFCFG(0x0152);
          COPYVAL(window_grouping);
          IFCFGEND;

#ifdef _F_USE_DESK_WINDOW_PROFILE_
          IFCFG(0x0162);
          COPYSTR(desktop_default_window_profile);
          COPYVAL(use_desktop_window_profile);
          IFCFGEND;
#endif

          e_config->config_version = E_CONFIG_FILE_VERSION;
          _e_config_free(tcfg);
       }

     /* limit values so they are sane */
     E_CONFIG_LIMIT(e_config->menus_scroll_speed, 1.0, 20000.0);
     E_CONFIG_LIMIT(e_config->show_splash, 0, 1);
     E_CONFIG_LIMIT(e_config->menus_fast_mouse_move_threshhold, 1.0, 2000.0);
     E_CONFIG_LIMIT(e_config->menus_click_drag_timeout, 0.0, 10.0);
     E_CONFIG_LIMIT(e_config->border_shade_animate, 0, 1);
     E_CONFIG_LIMIT(e_config->border_shade_transition, 0, 8);
     E_CONFIG_LIMIT(e_config->border_shade_speed, 1.0, 20000.0);
     E_CONFIG_LIMIT(e_config->framerate, 1.0, 200.0);
     E_CONFIG_LIMIT(e_config->priority, 0, 19);
     E_CONFIG_LIMIT(e_config->image_cache, 0, 256 * 1024);
     E_CONFIG_LIMIT(e_config->font_cache, 0, 32 * 1024);
     E_CONFIG_LIMIT(e_config->edje_cache, 0, 256);
     E_CONFIG_LIMIT(e_config->edje_collection_cache, 0, 512);
     E_CONFIG_LIMIT(e_config->cache_flush_poll_interval, 8, 32768);
     E_CONFIG_LIMIT(e_config->zone_desks_x_count, 1, 64);
     E_CONFIG_LIMIT(e_config->zone_desks_y_count, 1, 64);
#ifdef _F_USE_WM_ZONE_HOST_
     E_CONFIG_LIMIT(e_config->tizen_zone_active, 0, 1);
#endif
     E_CONFIG_LIMIT(e_config->show_desktop_icons, 0, 1);
     E_CONFIG_LIMIT(e_config->edge_flip_dragging, 0, 1);
     E_CONFIG_LIMIT(e_config->window_placement_policy, E_WINDOW_PLACEMENT_SMART, E_WINDOW_PLACEMENT_MANUAL);
     E_CONFIG_LIMIT(e_config->window_grouping, 0, 1);
     E_CONFIG_LIMIT(e_config->focus_policy, 0, 2);
#ifdef _F_FOCUS_WINDOW_IF_TOP_STACK_
     E_CONFIG_LIMIT(e_config->focus_setting, 0, 4);
#else
     E_CONFIG_LIMIT(e_config->focus_setting, 0, 3);
#endif
     E_CONFIG_LIMIT(e_config->pass_click_on, 0, 1);
     E_CONFIG_LIMIT(e_config->always_click_to_raise, 0, 1);
     E_CONFIG_LIMIT(e_config->always_click_to_focus, 0, 1);
     E_CONFIG_LIMIT(e_config->use_auto_raise, 0, 1);
     E_CONFIG_LIMIT(e_config->auto_raise_delay, 0.0, 5.0);
     E_CONFIG_LIMIT(e_config->use_resist, 0, 1);
     E_CONFIG_LIMIT(e_config->drag_resist, 0, 100);
     E_CONFIG_LIMIT(e_config->desk_resist, 0, 100);
     E_CONFIG_LIMIT(e_config->window_resist, 0, 100);
     E_CONFIG_LIMIT(e_config->gadget_resist, 0, 100);
     E_CONFIG_LIMIT(e_config->geometry_auto_move, 0, 1);
     E_CONFIG_LIMIT(e_config->geometry_auto_resize_limit, 0, 1);
     E_CONFIG_LIMIT(e_config->winlist_warp_while_selecting, 0, 1);
     E_CONFIG_LIMIT(e_config->winlist_warp_at_end, 0, 1);
     E_CONFIG_LIMIT(e_config->winlist_warp_speed, 0.0, 1.0);
     E_CONFIG_LIMIT(e_config->winlist_scroll_animate, 0, 1);
     E_CONFIG_LIMIT(e_config->winlist_scroll_speed, 0.0, 1.0);
     E_CONFIG_LIMIT(e_config->winlist_list_show_iconified, 0, 1);
     E_CONFIG_LIMIT(e_config->winlist_list_show_other_desk_iconified, 0, 1);
     E_CONFIG_LIMIT(e_config->winlist_list_show_other_screen_iconified, 0, 1);
     E_CONFIG_LIMIT(e_config->winlist_list_show_other_desk_windows, 0, 1);
     E_CONFIG_LIMIT(e_config->winlist_list_show_other_screen_windows, 0, 1);
     E_CONFIG_LIMIT(e_config->winlist_list_uncover_while_selecting, 0, 1);
     E_CONFIG_LIMIT(e_config->winlist_list_jump_desk_while_selecting, 0, 1);
     E_CONFIG_LIMIT(e_config->winlist_pos_align_x, 0.0, 1.0);
     E_CONFIG_LIMIT(e_config->winlist_pos_align_y, 0.0, 1.0);
     E_CONFIG_LIMIT(e_config->winlist_pos_size_w, 0.0, 1.0);
     E_CONFIG_LIMIT(e_config->winlist_pos_size_h, 0.0, 1.0);
     E_CONFIG_LIMIT(e_config->winlist_pos_min_w, 0, 4000);
     E_CONFIG_LIMIT(e_config->winlist_pos_min_h, 0, 4000);
     E_CONFIG_LIMIT(e_config->winlist_pos_max_w, 8, 4000);
     E_CONFIG_LIMIT(e_config->winlist_pos_max_h, 8, 4000);
     E_CONFIG_LIMIT(e_config->maximize_policy, E_MAXIMIZE_FULLSCREEN, E_MAXIMIZE_DIRECTION);
     E_CONFIG_LIMIT(e_config->allow_manip, 0, 1);
     E_CONFIG_LIMIT(e_config->border_fix_on_shelf_toggle, 0, 1);
     E_CONFIG_LIMIT(e_config->allow_above_fullscreen, 0, 1);
     E_CONFIG_LIMIT(e_config->kill_if_close_not_possible, 0, 1);
     E_CONFIG_LIMIT(e_config->kill_process, 0, 1);
     E_CONFIG_LIMIT(e_config->kill_hung_process_by_module, 0, 1);
     E_CONFIG_LIMIT(e_config->kill_timer_wait, 0.0, 120.0);
     E_CONFIG_LIMIT(e_config->ping_clients, 0, 1);
     E_CONFIG_LIMIT(e_config->move_info_follows, 0, 1);
     E_CONFIG_LIMIT(e_config->resize_info_follows, 0, 1);
     E_CONFIG_LIMIT(e_config->move_info_visible, 0, 1);
     E_CONFIG_LIMIT(e_config->resize_info_visible, 0, 1);
     E_CONFIG_LIMIT(e_config->focus_last_focused_per_desktop, 0, 1);
     E_CONFIG_LIMIT(e_config->focus_revert_on_hide_or_close, 0, 1);
     E_CONFIG_LIMIT(e_config->pointer_slide, 0, 1);
     E_CONFIG_LIMIT(e_config->show_cursor, 0, 1);
     E_CONFIG_LIMIT(e_config->use_e_cursor, 0, 1);
     E_CONFIG_LIMIT(e_config->cursor_size, 0, 1024);
     E_CONFIG_LIMIT(e_config->menu_autoscroll_margin, 0, 50);
     E_CONFIG_LIMIT(e_config->menu_autoscroll_cursor_margin, 0, 50);
     E_CONFIG_LIMIT(e_config->menu_eap_name_show, 0, 1);
     E_CONFIG_LIMIT(e_config->menu_eap_generic_show, 0, 1);
     E_CONFIG_LIMIT(e_config->menu_eap_comment_show, 0, 1);
     E_CONFIG_LIMIT(e_config->use_app_icon, 0, 1);
     E_CONFIG_LIMIT(e_config->cnfmdlg_disabled, 0, 1);
     E_CONFIG_LIMIT(e_config->cfgdlg_auto_apply, 0, 1);
     E_CONFIG_LIMIT(e_config->cfgdlg_default_mode, 0, 1);
     E_CONFIG_LIMIT(e_config->font_hinting, 0, 2);
     E_CONFIG_LIMIT(e_config->desklock_login_box_zone, -2, 1000);
     E_CONFIG_LIMIT(e_config->desklock_autolock_screensaver, 0, 1);
     E_CONFIG_LIMIT(e_config->desklock_post_screensaver_time, 0.0, 300.0);
     E_CONFIG_LIMIT(e_config->desklock_autolock_idle, 0, 1);
     E_CONFIG_LIMIT(e_config->desklock_autolock_idle_timeout, 1.0, 5400.0);
     E_CONFIG_LIMIT(e_config->desklock_use_custom_desklock, 0, 1);
     E_CONFIG_LIMIT(e_config->desklock_ask_presentation, 0, 1);
     E_CONFIG_LIMIT(e_config->desklock_ask_presentation_timeout, 1.0, 300.0);
     E_CONFIG_LIMIT(e_config->border_raise_on_mouse_action, 0, 1);
     E_CONFIG_LIMIT(e_config->border_raise_on_focus, 0, 1);
     E_CONFIG_LIMIT(e_config->desk_flip_wrap, 0, 1);
     E_CONFIG_LIMIT(e_config->fullscreen_flip, 0, 1);
     E_CONFIG_LIMIT(e_config->icon_theme_overrides, 0, 1);
     E_CONFIG_LIMIT(e_config->remember_internal_windows, 0, 3);
     E_CONFIG_LIMIT(e_config->desk_auto_switch, 0, 1);

     E_CONFIG_LIMIT(e_config->dpms_enable, 0, 1);
     E_CONFIG_LIMIT(e_config->dpms_standby_enable, 0, 1);
     E_CONFIG_LIMIT(e_config->dpms_suspend_enable, 0, 1);
     E_CONFIG_LIMIT(e_config->dpms_off_enable, 0, 1);
     E_CONFIG_LIMIT(e_config->dpms_standby_timeout, 0, 5400);
     E_CONFIG_LIMIT(e_config->dpms_suspend_timeout, 0, 5400);
     E_CONFIG_LIMIT(e_config->dpms_off_timeout, 0, 5400);

     E_CONFIG_LIMIT(e_config->screensaver_timeout, 0, 5400);
     E_CONFIG_LIMIT(e_config->screensaver_interval, 0, 5400);
     E_CONFIG_LIMIT(e_config->screensaver_blanking, 0, 2);
     E_CONFIG_LIMIT(e_config->screensaver_expose, 0, 2);
     E_CONFIG_LIMIT(e_config->screensaver_ask_presentation, 0, 1);
     E_CONFIG_LIMIT(e_config->screensaver_ask_presentation_timeout, 1.0, 300.0);

     E_CONFIG_LIMIT(e_config->clientlist_group_by, 0, 2);
     E_CONFIG_LIMIT(e_config->clientlist_include_all_zones, 0, 1);
     E_CONFIG_LIMIT(e_config->clientlist_separate_with, 0, 2);
     E_CONFIG_LIMIT(e_config->clientlist_sort_by, 0, 3);
     E_CONFIG_LIMIT(e_config->clientlist_separate_iconified_apps, 0, 2);
     E_CONFIG_LIMIT(e_config->clientlist_warp_to_iconified_desktop, 0, 1);
     E_CONFIG_LIMIT(e_config->mouse_hand, 0, 1);
     E_CONFIG_LIMIT(e_config->clientlist_limit_caption_len, 0, 1);
     E_CONFIG_LIMIT(e_config->clientlist_max_caption_len, 2, E_CLIENTLIST_MAX_CAPTION_LEN);

     E_CONFIG_LIMIT(e_config->mouse_accel_numerator, 1, 10);
     E_CONFIG_LIMIT(e_config->mouse_accel_denominator, 1, 10);
     E_CONFIG_LIMIT(e_config->mouse_accel_threshold, 1, 10);

     E_CONFIG_LIMIT(e_config->menu_favorites_show, 0, 1);
     E_CONFIG_LIMIT(e_config->menu_apps_show, 0, 1);
     E_CONFIG_LIMIT(e_config->menu_gadcon_client_toplevel, 0, 1);

     E_CONFIG_LIMIT(e_config->ping_clients_interval, 16, 1024);

     E_CONFIG_LIMIT(e_config->mode.presentation, 0, 1);
     E_CONFIG_LIMIT(e_config->mode.offline, 0, 1);

     E_CONFIG_LIMIT(e_config->exec.expire_timeout, 0.1, 1000);
     E_CONFIG_LIMIT(e_config->exec.show_run_dialog, 0, 1);
     E_CONFIG_LIMIT(e_config->exec.show_exit_dialog, 0, 1);

     E_CONFIG_LIMIT(e_config->null_container_win, 0, 1);

     E_CONFIG_LIMIT(e_config->powersave.none, 0.01, 5400.00);
     E_CONFIG_LIMIT(e_config->powersave.low, 0.01, 5400.00);
     E_CONFIG_LIMIT(e_config->powersave.medium, 0.01, 5400.00);
     E_CONFIG_LIMIT(e_config->powersave.high, 0.01, 5400.00);
     E_CONFIG_LIMIT(e_config->powersave.extreme, 0.01, 5400.00);
     E_CONFIG_LIMIT(e_config->powersave.min, E_POWERSAVE_MODE_NONE, E_POWERSAVE_MODE_EXTREME);
     E_CONFIG_LIMIT(e_config->powersave.max, E_POWERSAVE_MODE_NONE, E_POWERSAVE_MODE_EXTREME);

     E_CONFIG_LIMIT(e_config->multiscreen_flip, 0, 1);

     E_CONFIG_LIMIT(e_config->priority_raise_on_focus, 0, 1);
     E_CONFIG_LIMIT(e_config->max_hung_count, 0, 32);

#ifdef _F_USE_ICONIFY_RESIZE_
     E_CONFIG_LIMIT(e_config->iconify_resize_w, 1, 320);
     E_CONFIG_LIMIT(e_config->iconify_resize_h, 1, 320);
#endif /* _F_USE_ICONIFY_RESIZE_ */
#endif /* _F_COPY_FROM_DATA_DIR_ */

     /* FIXME: disabled auto apply because it causes problems */
     e_config->cfgdlg_auto_apply = 0;
     /* FIXME: desklock personalized password id disabled for security reasons */
     e_config->desklock_auth_method = 0;
     if (e_config->desklock_personal_passwd)
       eina_stringshare_del(e_config->desklock_personal_passwd);
     e_config->desklock_personal_passwd = NULL;

     ecore_event_add(E_EVENT_CONFIG_LOADED, NULL, NULL, NULL);
}

EAPI int
e_config_save(void)
{
   if (_e_config_save_defer)
     {
        e_powersave_deferred_action_del(_e_config_save_defer);
        _e_config_save_defer = NULL;
     }
   _e_config_save_cb(NULL);
   return e_config_domain_save("e", _e_config_edd, e_config);
}

EAPI void
e_config_save_flush(void)
{
   if (_e_config_save_defer)
     {
        e_powersave_deferred_action_del(_e_config_save_defer);
        _e_config_save_defer = NULL;
        _e_config_save_cb(NULL);
     }
}

EAPI void
e_config_save_queue(void)
{
   if (_e_config_save_defer)
     e_powersave_deferred_action_del(_e_config_save_defer);
   _e_config_save_defer = e_powersave_deferred_action_add(_e_config_save_cb,
                                                          NULL);
}

EAPI const char *
e_config_profile_get(void)
{
   return _e_config_profile;
}

EAPI void
e_config_profile_set(const char *prof)
{
   eina_stringshare_replace(&_e_config_profile, prof);
   e_util_env_set("E_CONF_PROFILE", _e_config_profile);
}

EAPI char *
e_config_profile_dir_get(const char *prof)
{
   char buf[PATH_MAX];

   e_user_dir_snprintf(buf, sizeof(buf), "config/%s", prof);
   if (ecore_file_is_dir(buf)) return strdup(buf);
   e_prefix_data_snprintf(buf, sizeof(buf), "data/config/%s", prof);
   if (ecore_file_is_dir(buf)) return strdup(buf);
   return NULL;
}

static int
_cb_sort_files(char *f1, char *f2)
{
   return strcmp(f1, f2);
}

EAPI Eina_List *
e_config_profile_list(void)
{
   Eina_List *files;
   char buf[PATH_MAX], *p;
   Eina_List *flist = NULL;
   size_t len;

   len = e_user_dir_concat_static(buf, "config");
   if (len + 1 >= (int)sizeof(buf))
     return NULL;

   files = ecore_file_ls(buf);

   buf[len] = '/';
   len++;

   p = buf + len;
   len = sizeof(buf) - len;
   if (files)
     {
        char *file;

        files = eina_list_sort(files, 0, (Eina_Compare_Cb)_cb_sort_files);
        EINA_LIST_FREE(files, file)
          {
             if (eina_strlcpy(p, file, len) >= len)
               {
                  free(file);
                  continue;
               }
             if (ecore_file_is_dir(buf))
               flist = eina_list_append(flist, file);
             else
               free(file);
          }
     }
   len = e_prefix_data_concat_static(buf, "data/config");
   if (len + 1 >= sizeof(buf))
     return NULL;

   files = ecore_file_ls(buf);

   buf[len] = '/';
   len++;

   p = buf + len;
   len = sizeof(buf) - len;
   if (files)
     {
        char *file;
        files = eina_list_sort(files, 0, (Eina_Compare_Cb)_cb_sort_files);
        EINA_LIST_FREE(files, file)
          {
             if (eina_strlcpy(p, file, len) >= len)
               {
                  free(file);
                  continue;
               }
             if (ecore_file_is_dir(buf))
               {
                  const Eina_List *l;
                  const char *tmp;
                  EINA_LIST_FOREACH(flist, l, tmp)
                    if (!strcmp(file, tmp)) break;

                  if (!l) flist = eina_list_append(flist, file);
                  else free(file);
               }
             else
               free(file);
          }
     }
   return flist;
}

EAPI void
e_config_profile_add(const char *prof)
{
   char buf[4096];
   if (e_user_dir_snprintf(buf, sizeof(buf), "config/%s", prof) >= sizeof(buf))
     return;
   ecore_file_mkdir(buf);
}

EAPI void
e_config_profile_del(const char *prof)
{
   char buf[4096];
   if (e_user_dir_snprintf(buf, sizeof(buf), "config/%s", prof) >= sizeof(buf))
     return;
   ecore_file_recursive_rm(buf);
}

EAPI void
e_config_save_block_set(int block)
{
   _e_config_save_block = block;
}

EAPI int
e_config_save_block_get(void)
{
   return _e_config_save_block;
}

/**
 * Loads configurations from file located in the working profile
 * The configurations are stored in a struct declated by the
 * macros E_CONFIG_DD_NEW and E_CONFIG_<b>TYPE</b>
 *
 * @param domain of the configuration file.
 * @param edd to struct definition
 * @return returns allocated struct on success, if unable to find config returns null
 */
EAPI void *
e_config_domain_load(const char *domain, E_Config_DD *edd)
{
   Eet_File *ef;
   char buf[4096];
   void *data = NULL;
   int i;

   e_user_dir_snprintf(buf, sizeof(buf), "config/%s/%s.cfg",
                       _e_config_profile, domain);
   ELBF(ELBT_DFT, 0, 0, "domain:%s buf:%s", domain, buf);
   ef = eet_open(buf, EET_FILE_MODE_READ);
   if (ef)
     {
        data = eet_data_read(ef, edd, "config");
        eet_close(ef);
        if (data) return data;
     }

   for (i = 1; i <= _e_config_revisions; i++)
     {
        e_user_dir_snprintf(buf, sizeof(buf), "config/%s/%s.%i.cfg",
                            _e_config_profile, domain, i);
        ef = eet_open(buf, EET_FILE_MODE_READ);
        if (ef)
          {
             data = eet_data_read(ef, edd, "config");
             eet_close(ef);
             if (data) return data;
          }
     }

#ifdef _F_COPY_FROM_DATA_DIR_
   data = e_config_domain_system_load(domain, edd);
   if (data)
     {
        _e_config_domain_data_update(domain);
        ELB(ELBT_DFT, "DOMAIN_LOAD", 0);
        return e_config_domain_load(domain, edd);
     }
#endif
   ELB(ELBT_DFT, "DOMAIN_SYSTEM_LOAD", 0);
   return e_config_domain_system_load(domain, edd);
}

EAPI void *
e_config_domain_system_load(const char *domain, E_Config_DD *edd)
{
   Eet_File *ef;
   char buf[4096];
   void *data = NULL;

#ifdef _F_COPY_FROM_DATA_DIR_
   e_prefix_data_snprintf(buf, sizeof(buf), "config/e/config/%s/%s.cfg",
                          _e_config_profile, domain);
#else
   e_prefix_data_snprintf(buf, sizeof(buf), "data/config/%s/%s.cfg",
                          _e_config_profile, domain);
#endif
   ef = eet_open(buf, EET_FILE_MODE_READ);
   if (ef)
     {
        data = eet_data_read(ef, edd, "config");
        eet_close(ef);
        return data;
     }

   return data;
}

static void
_e_config_mv_error(const char *from, const char *to)
{
#if _F_USE_EXTN_DIALOG_
   char buf[8192];
   snprintf(buf, sizeof(buf),
            _("Enlightenment has had an error while moving config files<br>"
              "from:<br>"
              "%s<br>"
              "<br>"
              "to:<br>"
              "%s<br>"
              "<br>"
              "The rest of the write has been aborted for safety.<br>"),
            from, to);
   e_util_extn_dialog_show(_("Enlightenment Settings Write Problems"), buf);
#else
   if (!_e_config_error_dialog)
     {
        E_Dialog *dia;

        dia = e_dialog_new(e_container_current_get(e_manager_current_get()),
                           "E", "_sys_error_logout_slow");
        if (dia)
          {
             char buf[8192];

             e_dialog_title_set(dia, _("Enlightenment Settings Write Problems"));
             e_dialog_icon_set(dia, "dialog-error", 64);
             snprintf(buf, sizeof(buf),
                      _("Enlightenment has had an error while moving config files<br>"
                        "from:<br>"
                        "%s<br>"
                        "<br>"
                        "to:<br>"
                        "%s<br>"
                        "<br>"
                        "The rest of the write has been aborted for safety.<br>"),
                      from, to);
             e_dialog_text_set(dia, buf);
             e_dialog_button_add(dia, _("OK"), NULL, NULL, NULL);
             e_dialog_button_focus_num(dia, 0);
             e_win_centered_set(dia->win, 1);
             e_object_del_attach_func_set(E_OBJECT(dia),
                                          _e_config_error_dialog_cb_delete);
             e_dialog_show(dia);
             _e_config_error_dialog = dia;
          }
     }
#endif
}

EAPI int
e_config_profile_save(void)
{
   Eet_File *ef;
   char buf[4096], buf2[4096];
   int ok = 0;

   if (_e_config_save_block) return 0;
   /* FIXME: check for other sessions fo E running */
   e_user_dir_concat_static(buf, "config/profile.cfg");
   e_user_dir_concat_static(buf2, "config/profile.cfg.tmp");

   ef = eet_open(buf2, EET_FILE_MODE_WRITE);
   if (ef)
     {
        ok = eet_write(ef, "config", _e_config_profile,
                       strlen(_e_config_profile), 0);
        if (_e_config_eet_close_handle(ef, buf2))
          {
             Eina_Bool ret = EINA_TRUE;

             if (_e_config_revisions > 0)
               {
                  int i;
                  char bsrc[4096], bdst[4096];

                  for (i = _e_config_revisions; i > 1; i--)
                    {
                       e_user_dir_snprintf(bsrc, sizeof(bsrc), "config/profile.%i.cfg", i - 1);
                       e_user_dir_snprintf(bdst, sizeof(bdst), "config/profile.%i.cfg", i);
                       if ((ecore_file_exists(bsrc)) &&
                           (ecore_file_size(bsrc)))
                         {
                            ret = ecore_file_mv(bsrc, bdst);
                            if (!ret)
                              {
                                 _e_config_mv_error(bsrc, bdst);
                                 break;
                              }
                         }
                    }
                  if (ret)
                    {
                       e_user_dir_snprintf(bsrc, sizeof(bsrc), "config/profile.cfg");
                       e_user_dir_snprintf(bdst, sizeof(bdst), "config/profile.1.cfg");
                       ret = ecore_file_mv(bsrc, bdst);
//                       if (!ret)
//                          _e_config_mv_error(bsrc, bdst);
                    }
               }
             ret = ecore_file_mv(buf2, buf);
             if (!ret) _e_config_mv_error(buf2, buf);
          }
        ecore_file_unlink(buf2);
     }
   return ok;
}

/**
  * Saves configurations to file located in the working profile
 * The configurations are read from a struct declated by the
 * macros E_CONFIG_DD_NEW and E_CONFIG_<b>TYPE</b>
 *
 * @param domain  name of the configuration file.
 * @param edd pointer to struct definition
 * @param data struct to save as configuration file
 * @return 1 if save success, 0 on failure
 */
EAPI int
e_config_domain_save(const char *domain, E_Config_DD *edd, const void *data)
{
   Eet_File *ef;
   char buf[4096], buf2[4096];
   int ok = 0, ret;
   size_t len, len2;

   if (_e_config_save_block) return 0;
   /* FIXME: check for other sessions fo E running */
   len = e_user_dir_snprintf(buf, sizeof(buf), "config/%s", _e_config_profile);
   if (len + 1 >= sizeof(buf)) return 0;

   ecore_file_mkdir(buf);

   buf[len] = '/';
   len++;

   len2 = eina_strlcpy(buf + len, domain, sizeof(buf) - len);
   if (len2 + sizeof(".cfg") >= sizeof(buf) - len) return 0;

   len += len2;

   memcpy(buf + len, ".cfg", sizeof(".cfg"));
   len += sizeof(".cfg") - 1;

   if (len + sizeof(".tmp") >= sizeof(buf)) return 0;
   memcpy(buf2, buf, len);
   memcpy(buf2 + len, ".tmp", sizeof(".tmp"));

   ef = eet_open(buf2, EET_FILE_MODE_WRITE);
   if (ef)
     {
        ok = eet_data_write(ef, edd, "config", data, 1);
        if (_e_config_eet_close_handle(ef, buf2))
          {
             if (_e_config_revisions > 0)
               {
                  int i;
                  char bsrc[4096], bdst[4096];

                  for (i = _e_config_revisions; i > 1; i--)
                    {
                       e_user_dir_snprintf(bsrc, sizeof(bsrc), "config/%s/%s.%i.cfg", _e_config_profile, domain, i - 1);
                       e_user_dir_snprintf(bdst, sizeof(bdst), "config/%s/%s.%i.cfg", _e_config_profile, domain, i);
                       if ((ecore_file_exists(bsrc)) &&
                           (ecore_file_size(bsrc)))
                         {
                            ecore_file_mv(bsrc, bdst);
                         }
                    }
                  e_user_dir_snprintf(bsrc, sizeof(bsrc), "config/%s/%s.cfg", _e_config_profile, domain);
                  e_user_dir_snprintf(bdst, sizeof(bdst), "config/%s/%s.1.cfg", _e_config_profile, domain);
                  ecore_file_mv(bsrc, bdst);
               }
             ret = ecore_file_mv(buf2, buf);
             if (!ret)
               ERR("*** Error saving config. ***");
          }
        ecore_file_unlink(buf2);
     }
   return ok;
}

EAPI E_Config_Binding_Mouse *
e_config_binding_mouse_match(E_Config_Binding_Mouse *eb_in)
{
   Eina_List *l;
   E_Config_Binding_Mouse *eb;

   EINA_LIST_FOREACH(e_config->mouse_bindings, l, eb)
     {
        if ((eb->context == eb_in->context) &&
            (eb->button == eb_in->button) &&
            (eb->modifiers == eb_in->modifiers) &&
            (eb->any_mod == eb_in->any_mod) &&
            (((eb->action) && (eb_in->action) && (!strcmp(eb->action, eb_in->action))) ||
             ((!eb->action) && (!eb_in->action))) &&
            (((eb->params) && (eb_in->params) && (!strcmp(eb->params, eb_in->params))) ||
             ((!eb->params) && (!eb_in->params))))
          return eb;
     }
   return NULL;
}

EAPI E_Config_Binding_Key *
e_config_binding_key_match(E_Config_Binding_Key *eb_in)
{
   Eina_List *l;
   E_Config_Binding_Key *eb;

   EINA_LIST_FOREACH(e_config->mouse_bindings, l, eb)
     {
        if ((eb->context == eb_in->context) &&
            (eb->modifiers == eb_in->modifiers) &&
            (eb->any_mod == eb_in->any_mod) &&
            (((eb->key) && (eb_in->key) && (!strcmp(eb->key, eb_in->key))) ||
             ((!eb->key) && (!eb_in->key))) &&
            (((eb->action) && (eb_in->action) && (!strcmp(eb->action, eb_in->action))) ||
             ((!eb->action) && (!eb_in->action))) &&
            (((eb->params) && (eb_in->params) && (!strcmp(eb->params, eb_in->params))) ||
             ((!eb->params) && (!eb_in->params))))
          return eb;
     }
   return NULL;
}

EAPI E_Config_Binding_Edge *
e_config_binding_edge_match(E_Config_Binding_Edge *eb_in)
{
   Eina_List *l;
   E_Config_Binding_Edge *eb;

   EINA_LIST_FOREACH(e_config->edge_bindings, l, eb)
     {
        if ((eb->context == eb_in->context) &&
            (eb->modifiers == eb_in->modifiers) &&
            (eb->any_mod == eb_in->any_mod) &&
            (eb->edge == eb_in->edge) &&
            (eb->delay == eb_in->delay) &&
            (((eb->action) && (eb_in->action) && (!strcmp(eb->action, eb_in->action))) ||
             ((!eb->action) && (!eb_in->action))) &&
            (((eb->params) && (eb_in->params) && (!strcmp(eb->params, eb_in->params))) ||
             ((!eb->params) && (!eb_in->params))))
          return eb;
     }
   return NULL;
}

EAPI E_Config_Binding_Signal *
e_config_binding_signal_match(E_Config_Binding_Signal *eb_in)
{
   Eina_List *l;
   E_Config_Binding_Signal *eb;

   EINA_LIST_FOREACH(e_config->signal_bindings, l, eb)
     {
        if ((eb->context == eb_in->context) &&
            (eb->modifiers == eb_in->modifiers) &&
            (eb->any_mod == eb_in->any_mod) &&
            (((eb->signal) && (eb_in->signal) && (!strcmp(eb->signal, eb_in->signal))) ||
             ((!eb->signal) && (!eb_in->signal))) &&
            (((eb->source) && (eb_in->source) && (!strcmp(eb->source, eb_in->source))) ||
             ((!eb->source) && (!eb_in->source))) &&
            (((eb->action) && (eb_in->action) && (!strcmp(eb->action, eb_in->action))) ||
             ((!eb->action) && (!eb_in->action))) &&
            (((eb->params) && (eb_in->params) && (!strcmp(eb->params, eb_in->params))) ||
             ((!eb->params) && (!eb_in->params))))
          return eb;
     }
   return NULL;
}

EAPI E_Config_Binding_Wheel *
e_config_binding_wheel_match(E_Config_Binding_Wheel *eb_in)
{
   Eina_List *l;
   E_Config_Binding_Wheel *eb;

   EINA_LIST_FOREACH(e_config->wheel_bindings, l, eb)
     {
        if ((eb->context == eb_in->context) &&
            (eb->direction == eb_in->direction) &&
            (eb->z == eb_in->z) &&
            (eb->modifiers == eb_in->modifiers) &&
            (eb->any_mod == eb_in->any_mod) &&
            (((eb->action) && (eb_in->action) && (!strcmp(eb->action, eb_in->action))) ||
             ((!eb->action) && (!eb_in->action))) &&
            (((eb->params) && (eb_in->params) && (!strcmp(eb->params, eb_in->params))) ||
             ((!eb->params) && (!eb_in->params))))
          return eb;
     }
   return NULL;
}

EAPI E_Config_Binding_Acpi *
e_config_binding_acpi_match(E_Config_Binding_Acpi *eb_in)
{
   Eina_List *l;
   E_Config_Binding_Acpi *eb;

   EINA_LIST_FOREACH(e_config->acpi_bindings, l, eb)
     {
        if ((eb->context == eb_in->context) &&
            (eb->type == eb_in->type) &&
            (eb->status == eb_in->status) &&
            (((eb->action) && (eb_in->action) &&
              (!strcmp(eb->action, eb_in->action))) ||
             ((!eb->action) && (!eb_in->action))) &&
            (((eb->params) && (eb_in->params) &&
              (!strcmp(eb->params, eb_in->params))) ||
             ((!eb->params) && (!eb_in->params))))
          return eb;
     }
   return NULL;
}

EAPI void
e_config_mode_changed(void)
{
   ecore_event_add(E_EVENT_CONFIG_MODE_CHANGED, NULL, NULL, NULL);
}

/* local subsystem functions */
static void
_e_config_save_cb(void *data __UNUSED__)
{
   e_config_profile_save();
   e_module_save_all();
   e_config_domain_save("e", _e_config_edd, e_config);
   _e_config_save_defer = NULL;
}

static void
_e_config_free(E_Config *ecf)
{
   E_Config_Binding_Signal *ebs;
   E_Config_Binding_Mouse *ebm;
   E_Config_Binding_Wheel *ebw;
   E_Config_Syscon_Action *sca;
   E_Config_Binding_Key *ebk;
   E_Config_Binding_Edge *ebe;
   E_Config_Binding_Acpi *eba;
   E_Font_Fallback *eff;
   E_Config_Module *em;
   E_Font_Default *efd;
   E_Config_Theme *et;
   E_Color_Class *cc;
   E_Path_Dir *epd;
   E_Remember *rem;
   E_Config_Env_Var *evr;
   E_Config_XKB_Layout *cl;
   E_Config_XKB_Option *op;
#ifdef _F_USE_DESK_WINDOW_PROFILE_
   E_Config_Desktop_Window_Profile *wp;
#endif

   if (!ecf) return;

#ifdef _F_USE_DESK_WINDOW_PROFILE_
   EINA_LIST_FREE(ecf->desktop_window_profiles, wp)
     {
        eina_stringshare_del(wp->profile);
        E_FREE(wp);
     }
#endif

   if (e_config->xkb.default_model)
     eina_stringshare_del(e_config->xkb.default_model);

   EINA_LIST_FREE(e_config->xkb.used_layouts, cl)
     {
        eina_stringshare_del(cl->name);
        eina_stringshare_del(cl->model);
        eina_stringshare_del(cl->variant);
        E_FREE(cl);
     }

   EINA_LIST_FREE(e_config->xkb.used_options, op)
     {
        eina_stringshare_del(op->name);
        E_FREE(op);
     }

   EINA_LIST_FREE(ecf->modules, em)
     {
        if (em->name) eina_stringshare_del(em->name);
        E_FREE(em);
     }
   EINA_LIST_FREE(ecf->font_fallbacks, eff)
     {
        if (eff->name) eina_stringshare_del(eff->name);
        E_FREE(eff);
     }
   EINA_LIST_FREE(ecf->font_defaults, efd)
     {
        if (efd->text_class) eina_stringshare_del(efd->text_class);
        if (efd->font) eina_stringshare_del(efd->font);
        E_FREE(efd);
     }
   EINA_LIST_FREE(ecf->themes, et)
     {
        if (et->category) eina_stringshare_del(et->category);
        if (et->file) eina_stringshare_del(et->file);
        E_FREE(et);
     }
   EINA_LIST_FREE(ecf->mouse_bindings, ebm)
     {
        if (ebm->action) eina_stringshare_del(ebm->action);
        if (ebm->params) eina_stringshare_del(ebm->params);
        E_FREE(ebm);
     }
   EINA_LIST_FREE(ecf->key_bindings, ebk)
     {
        if (ebk->key) eina_stringshare_del(ebk->key);
        if (ebk->action) eina_stringshare_del(ebk->action);
        if (ebk->params) eina_stringshare_del(ebk->params);
        E_FREE(ebk);
     }
   EINA_LIST_FREE(ecf->edge_bindings, ebe)
     {
        if (ebe->action) eina_stringshare_del(ebe->action);
        if (ebe->params) eina_stringshare_del(ebe->params);
        E_FREE(ebe);
     }
   EINA_LIST_FREE(ecf->signal_bindings, ebs)
     {
        if (ebs->signal) eina_stringshare_del(ebs->signal);
        if (ebs->source) eina_stringshare_del(ebs->source);
        if (ebs->action) eina_stringshare_del(ebs->action);
        if (ebs->params) eina_stringshare_del(ebs->params);
        E_FREE(ebs);
     }
   EINA_LIST_FREE(ecf->wheel_bindings, ebw)
     {
        if (ebw->action) eina_stringshare_del(ebw->action);
        if (ebw->params) eina_stringshare_del(ebw->params);
        E_FREE(ebw);
     }
   EINA_LIST_FREE(ecf->acpi_bindings, eba)
     {
        if (eba->action) eina_stringshare_del(eba->action);
        if (eba->params) eina_stringshare_del(eba->params);
        E_FREE(eba);
     }
   EINA_LIST_FREE(ecf->path_append_data, epd)
     {
        if (epd->dir) eina_stringshare_del(epd->dir);
        E_FREE(epd);
     }
   EINA_LIST_FREE(ecf->path_append_images, epd)
     {
        if (epd->dir) eina_stringshare_del(epd->dir);
        E_FREE(epd);
     }
   EINA_LIST_FREE(ecf->path_append_fonts, epd)
     {
        if (epd->dir) eina_stringshare_del(epd->dir);
        E_FREE(epd);
     }
   EINA_LIST_FREE(ecf->path_append_themes, epd)
     {
        if (epd->dir) eina_stringshare_del(epd->dir);
        E_FREE(epd);
     }
   EINA_LIST_FREE(ecf->path_append_init, epd)
     {
        if (epd->dir) eina_stringshare_del(epd->dir);
        E_FREE(epd);
     }
   EINA_LIST_FREE(ecf->path_append_icons, epd)
     {
        if (epd->dir) eina_stringshare_del(epd->dir);
        E_FREE(epd);
     }
   EINA_LIST_FREE(ecf->path_append_modules, epd)
     {
        if (epd->dir) eina_stringshare_del(epd->dir);
        E_FREE(epd);
     }
   EINA_LIST_FREE(ecf->path_append_backgrounds, epd)
     {
        if (epd->dir) eina_stringshare_del(epd->dir);
        E_FREE(epd);
     }
   EINA_LIST_FREE(ecf->path_append_messages, epd)
     {
        if (epd->dir) eina_stringshare_del(epd->dir);
        E_FREE(epd);
     }
   EINA_LIST_FREE(ecf->remembers, rem)
     {
        if (rem->name) eina_stringshare_del(rem->name);
        if (rem->class) eina_stringshare_del(rem->class);
        if (rem->title) eina_stringshare_del(rem->title);
        if (rem->role) eina_stringshare_del(rem->role);
        if (rem->prop.border) eina_stringshare_del(rem->prop.border);
        if (rem->prop.command) eina_stringshare_del(rem->prop.command);
        E_FREE(rem);
     }
   EINA_LIST_FREE(ecf->color_classes, cc)
     {
        if (cc->name) eina_stringshare_del(cc->name);
        E_FREE(cc);
     }
   if (ecf->init_default_theme) eina_stringshare_del(ecf->init_default_theme);
   if (ecf->desktop_default_background) eina_stringshare_del(ecf->desktop_default_background);
   if (ecf->desktop_default_name) eina_stringshare_del(ecf->desktop_default_name);
#ifdef _F_USE_DESK_WINDOW_PROFILE_
   if (ecf->desktop_default_window_profile) eina_stringshare_del(ecf->desktop_default_window_profile);
#endif
   if (ecf->language) eina_stringshare_del(ecf->language);
   if (ecf->transition_start) eina_stringshare_del(ecf->transition_start);
   if (ecf->transition_desk) eina_stringshare_del(ecf->transition_desk);
   if (ecf->transition_change) eina_stringshare_del(ecf->transition_change);
   if (ecf->input_method) eina_stringshare_del(ecf->input_method);
   if (ecf->exebuf_term_cmd) eina_stringshare_del(ecf->exebuf_term_cmd);
   if (ecf->desklock_personal_passwd) eina_stringshare_del(ecf->desklock_personal_passwd);
   if (ecf->desklock_background) eina_stringshare_del(ecf->desklock_background);
   if (ecf->icon_theme) eina_stringshare_del(ecf->icon_theme);
   if (ecf->wallpaper_import_last_dev) eina_stringshare_del(ecf->wallpaper_import_last_dev);
   if (ecf->wallpaper_import_last_path) eina_stringshare_del(ecf->wallpaper_import_last_path);
   if (ecf->theme_default_border_style) eina_stringshare_del(ecf->theme_default_border_style);
   if (ecf->desklock_custom_desklock_cmd) eina_stringshare_del(ecf->desklock_custom_desklock_cmd);
   EINA_LIST_FREE(ecf->syscon.actions, sca)
     {
        if (sca->action) eina_stringshare_del(sca->action);
        if (sca->params) eina_stringshare_del(sca->params);
        if (sca->button) eina_stringshare_del(sca->button);
        if (sca->icon) eina_stringshare_del(sca->icon);
        E_FREE(sca);
     }
   if (ecf->randr_serialized_setup)
     {
        e_randr_serialized_setup_free(ecf->randr_serialized_setup);
     }
   EINA_LIST_FREE(ecf->env_vars, evr)
     {
        if (evr->var) eina_stringshare_del(evr->var);
        if (evr->val) eina_stringshare_del(evr->val);
        E_FREE(evr);
     }
   if (ecf->xsettings.net_icon_theme_name)
     eina_stringshare_del(ecf->xsettings.net_icon_theme_name);
   if (ecf->xsettings.net_theme_name)
     eina_stringshare_del(ecf->xsettings.net_theme_name);
   if (ecf->xsettings.gtk_font_name)
     eina_stringshare_del(ecf->xsettings.gtk_font_name);
   if (ecf->backlight.sysdev)
     eina_stringshare_del(ecf->backlight.sysdev);

   E_FREE(ecf);
}

static Eina_Bool
_e_config_cb_timer(void *data)
{
   e_util_dialog_show(_("Settings Upgraded"), "%s", (char *)data);
   return 0;
}

static int
_e_config_eet_close_handle(Eet_File *ef, char *file)
{
   Eet_Error err;
   char *erstr = NULL;

   err = eet_close(ef);
   switch (err)
     {
      case EET_ERROR_NONE:
        /* all good - no error */
        break;

      case EET_ERROR_BAD_OBJECT:
        erstr = _("The EET file handle is bad.");
        break;

      case EET_ERROR_EMPTY:
        erstr = _("The file data is empty.");
        break;

      case EET_ERROR_NOT_WRITABLE:
        erstr = _("The file is not writable. Perhaps the disk is read-only<br>or you lost permissions to your files.");
        break;

      case EET_ERROR_OUT_OF_MEMORY:
        erstr = _("Memory ran out while preparing the write.<br>Please free up memory.");
        break;

      case EET_ERROR_WRITE_ERROR:
        erstr = _("This is a generic error.");
        break;

      case EET_ERROR_WRITE_ERROR_FILE_TOO_BIG:
        erstr = _("The settings file is too large.<br>It should be very small (a few hundred KB at most).");
        break;

      case EET_ERROR_WRITE_ERROR_IO_ERROR:
        erstr = _("You have I/O errors on the disk.<br>Maybe it needs replacing?");
        break;

      case EET_ERROR_WRITE_ERROR_OUT_OF_SPACE:
        erstr = _("You ran out of space while writing the file");
        break;

      case EET_ERROR_WRITE_ERROR_FILE_CLOSED:
        erstr = _("The file was closed on it while writing.");
        break;

      case EET_ERROR_MMAP_FAILED:
        erstr = _("Memory-mapping (mmap) of the file failed.");
        break;

      case EET_ERROR_X509_ENCODING_FAILED:
        erstr = _("X509 Encoding failed.");
        break;

      case EET_ERROR_SIGNATURE_FAILED:
        erstr = _("Signature failed.");
        break;

      case EET_ERROR_INVALID_SIGNATURE:
        erstr = _("The signature was invalid.");
        break;

      case EET_ERROR_NOT_SIGNED:
        erstr = _("Not signed.");
        break;

      case EET_ERROR_NOT_IMPLEMENTED:
        erstr = _("Feature not implemented.");
        break;

      case EET_ERROR_PRNG_NOT_SEEDED:
        erstr = _("PRNG was not seeded.");
        break;

      case EET_ERROR_ENCRYPT_FAILED:
        erstr = _("Encryption failed.");
        break;

      case EET_ERROR_DECRYPT_FAILED:
        erstr = _("Decryption failed.");
        break;

      default: /* if we get here eet added errors we don't know */
        erstr = _("The error is unknown to Enlightenment.");
        break;
     }
   if (erstr)
     {
        /* delete any partially-written file */
        ecore_file_unlink(file);
#if _F_USE_EXTN_DIALOG_
        char buf[8192];
        snprintf(buf, sizeof(buf),
                 _("Enlightenment has had an error while writing<br>"
                   "its config file.<br>"
                   "%s<br>"
                   "<br>"
                   "The file where the error occurred was:<br>"
                   "%s<br>"
                   "<br>"
                   "This file has been deleted to avoid corrupt data.<br>"),
                 erstr, file);
        e_util_extn_dialog_show(_("Enlightenment Settings Write Problems"), buf);
#else
        /* only show dialog for first error - further ones are likely */
        /* more of the same error */
        if (!_e_config_error_dialog)
          {
             E_Dialog *dia;

             dia = e_dialog_new(e_container_current_get(e_manager_current_get()),
                                "E", "_sys_error_logout_slow");
             if (dia)
               {
                  char buf[8192];

                  e_dialog_title_set(dia, _("Enlightenment Settings Write Problems"));
                  e_dialog_icon_set(dia, "dialog-error", 64);
                  snprintf(buf, sizeof(buf),
                           _("Enlightenment has had an error while writing<br>"
                             "its config file.<br>"
                             "%s<br>"
                             "<br>"
                             "The file where the error occurred was:<br>"
                             "%s<br>"
                             "<br>"
                             "This file has been deleted to avoid corrupt data.<br>"),
                           erstr, file);
                  e_dialog_text_set(dia, buf);
                  e_dialog_button_add(dia, _("OK"), NULL, NULL, NULL);
                  e_dialog_button_focus_num(dia, 0);
                  e_win_centered_set(dia->win, 1);
                  e_object_del_attach_func_set(E_OBJECT(dia),
                                               _e_config_error_dialog_cb_delete);
                  e_dialog_show(dia);
                  _e_config_error_dialog = dia;
               }
          }
#endif
        return 0;
     }
   return 1;
}

static void
_e_config_acpi_bindings_add(void)
{
   E_Config_Binding_Acpi *binding;

   binding = E_NEW(E_Config_Binding_Acpi, 1);
   binding->context = E_BINDING_CONTEXT_NONE;
   binding->type = E_ACPI_TYPE_AC_ADAPTER;
   binding->status = 0;
   binding->action = eina_stringshare_add("dim_screen");
   binding->params = NULL;
   e_config->acpi_bindings = eina_list_append(e_config->acpi_bindings, binding);

   binding = E_NEW(E_Config_Binding_Acpi, 1);
   binding->context = E_BINDING_CONTEXT_NONE;
   binding->type = E_ACPI_TYPE_AC_ADAPTER;
   binding->status = 1;
   binding->action = eina_stringshare_add("undim_screen");
   binding->params = NULL;
   e_config->acpi_bindings = eina_list_append(e_config->acpi_bindings, binding);

   binding = E_NEW(E_Config_Binding_Acpi, 1);
   binding->context = E_BINDING_CONTEXT_NONE;
   binding->type = E_ACPI_TYPE_LID;
   binding->status = 0;
   binding->action = eina_stringshare_add("suspend");
   binding->params = eina_stringshare_add("now");
   e_config->acpi_bindings = eina_list_append(e_config->acpi_bindings, binding);

   binding = E_NEW(E_Config_Binding_Acpi, 1);
   binding->context = E_BINDING_CONTEXT_NONE;
   binding->type = E_ACPI_TYPE_POWER;
   binding->status = -1;
   binding->action = eina_stringshare_add("halt_now");
   binding->params = eina_stringshare_add("now");
   e_config->acpi_bindings = eina_list_append(e_config->acpi_bindings, binding);

   binding = E_NEW(E_Config_Binding_Acpi, 1);
   binding->context = E_BINDING_CONTEXT_NONE;
   binding->type = E_ACPI_TYPE_SLEEP;
   binding->status = -1;
   binding->action = eina_stringshare_add("suspend");
   binding->params = eina_stringshare_add("now");
   e_config->acpi_bindings = eina_list_append(e_config->acpi_bindings, binding);
}

#ifdef _F_COPY_FROM_DATA_DIR_
static void
_e_config_domain_data_update(const char *domain)
{
   char buf[4096];
   int i;
   char *home_path;

   e_prefix_data_snprintf(buf, sizeof(buf), "config");
   ELBF(ELBT_DFT, 0, 0, "domain:%s buf:%s", domain, buf);

   home_path = getenv("HOME");
   if (!home_path)
     home_path = "/opt/home/app";

   char *src_base = buf;
   char tgt_base[PATH_MAX];
   snprintf(tgt_base, sizeof(tgt_base), "%s/.e", home_path);

   Eina_Bool res = ecore_file_mkdir(tgt_base);
   ELBF(ELBT_DFT, 0, 0, "MKDIR(%d) %s", res, tgt_base);
   if (!res)
     {
        int _errno = errno;
        char buffer[1024];
        strerror_r(_errno, buffer, sizeof(buffer));
        ELBF(ELBT_DFT, 0, 0, "Error(%d) %s", _errno, buffer);
     }

   const char *dir = eina_stringshare_add(src_base);
   Eina_List *list = NULL;
   list = eina_list_append(list, dir);

   while (eina_list_count(list) > 0)
     {
        Eina_List *l = eina_list_last(list);
        dir = eina_list_data_get(l);
        list = eina_list_remove(list, dir);

        if ((l) && (dir))
          {
             Eina_File_Direct_Info *info;
             Eina_Iterator *ls = eina_file_direct_ls(dir);
             if (ls)
               {
                  EINA_ITERATOR_FOREACH(ls, info)
                    {
                       Eina_Bool d = EINA_FALSE;
                       if (info->type == EINA_FILE_DIR) d = EINA_TRUE;

                       char **result = NULL;
                       unsigned int elements;
                       result = eina_str_split_full(info->path, src_base, -1, &elements);

                       if (elements == 2)
                         {
                            Eina_Strbuf *strbuf = eina_strbuf_new();
                            eina_strbuf_append(strbuf, tgt_base);
                            eina_strbuf_append(strbuf, result[1]);
                            const char *f = eina_strbuf_string_get(strbuf);

                            if (d)
                              {
                                 const char *sub = eina_stringshare_add(info->path);
                                 list = eina_list_append(list, sub);

                                 /* make sub directory */
                                 if (f) res = ecore_file_mkdir(f);
                                 else res = EINA_FALSE;

                                 ELBF(ELBT_DFT, 0, 0, "MKDIR(%d) %s", res, f);
                              }
                            else
                              {
                                 /* copy file */
                                 res = ecore_file_cp(info->path, f);
                                 ELBF(ELBT_DFT, 0, 0, "COPY(%d) %s -> %s", res, info->path, f);
                              }

                            if (!res)
                              {
                                 int _errno = errno;
                                 char buffer[4096];
                                 strerror_r(_errno, buffer, sizeof(buffer));
                                 ELBF(ELBT_DFT, 0, 0, "Error(%d) %s", _errno, buffer);
                              }

                            eina_strbuf_free(strbuf);
                         }
                       else
                         {
                            ELBF(ELBT_DFT, 0, 0, "ERROR elements:%d", elements);
                         }

                       if (result)
                         {
                            free(result[0]);
                            free(result);
                         }
                    }
               }

             eina_iterator_free(ls);
          }

        if (dir)
          eina_stringshare_del(dir);
     }

   eina_list_free(list);
}
#endif

