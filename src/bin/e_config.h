#ifdef E_TYPEDEFS

#define E_CONFIG_LIMIT(v, min, max) {if (v > max) v = max; else if (v < min) v = min;}

typedef struct _E_Config                    E_Config;
typedef struct _E_Config_Module             E_Config_Module;
typedef struct _E_Config_Theme              E_Config_Theme;
typedef struct _E_Config_Binding_Mouse      E_Config_Binding_Mouse;
typedef struct _E_Config_Binding_Key        E_Config_Binding_Key;
typedef struct _E_Config_Binding_Edge       E_Config_Binding_Edge;
typedef struct _E_Config_Binding_Signal     E_Config_Binding_Signal;
typedef struct _E_Config_Binding_Wheel      E_Config_Binding_Wheel;
typedef struct _E_Config_Binding_Acpi       E_Config_Binding_Acpi;
typedef struct _E_Config_Desktop_Background E_Config_Desktop_Background;
typedef struct _E_Config_Desktop_Name       E_Config_Desktop_Name;
typedef struct _E_Config_Gadcon             E_Config_Gadcon;
typedef struct _E_Config_Gadcon_Client      E_Config_Gadcon_Client;
typedef struct _E_Config_Shelf              E_Config_Shelf;
typedef struct _E_Config_Shelf_Desk         E_Config_Shelf_Desk;
typedef struct _E_Config_Mime_Icon          E_Config_Mime_Icon;
typedef struct _E_Config_Syscon_Action      E_Config_Syscon_Action;
typedef struct _E_Config_Env_Var            E_Config_Env_Var;
typedef struct _E_Event_Config_Icon_Theme   E_Event_Config_Icon_Theme;

#else
#ifndef E_CONFIG_H
#define E_CONFIG_H

/* increment this whenever we change config enough that you need new
 * defaults for e to work.
 */
#define E_CONFIG_FILE_EPOCH      0x0001
/* increment this whenever a new set of config values are added but the users
 * config doesn't need to be wiped - simply new values need to be put in
 */
#define E_CONFIG_FILE_GENERATION 0x0142
#define E_CONFIG_FILE_VERSION    ((E_CONFIG_FILE_EPOCH << 16) | E_CONFIG_FILE_GENERATION)

#define E_EVAS_ENGINE_DEFAULT         0
#define E_EVAS_ENGINE_SOFTWARE_X11    1
#define E_EVAS_ENGINE_GL_X11          2
#define E_EVAS_ENGINE_XRENDER_X11     3
#define E_EVAS_ENGINE_SOFTWARE_X11_16 4

typedef enum _E_Engine_Context
{
   E_ENGINE_CONTEXT_INIT,
   E_ENGINE_CONTEXT_CONTAINER,
   E_ENGINE_CONTEXT_ZONE,
   E_ENGINE_CONTEXT_BORDER,
   E_ENGINE_CONTEXT_MENU,
   E_ENGINE_CONTEXT_ERROR,
   E_ENGINE_CONTEXT_WIN,
   E_ENGINE_CONTEXT_POPUP,
   E_ENGINE_CONTEXT_DRAG
} E_Engine_Context;

struct _E_Config
{
   int         config_version; // INTERNAL
   int         show_splash; // GUI
   const char *init_default_theme; // GUI
   const char *desktop_default_background; // GUI
   Eina_List  *desktop_backgrounds; // GUI
   const char *desktop_default_name;
   Eina_List  *desktop_names; // GUI
   double      menus_scroll_speed; // GUI
   double      menus_fast_mouse_move_threshhold; // GUI
   double      menus_click_drag_timeout; // GUI
   int         border_shade_animate; // GUI
   int         border_shade_transition; // GUI
   double      border_shade_speed; // GUI
   double      framerate; // GUI
   int         priority; // GUI
   int         image_cache; // GUI
   int         font_cache; // GUI
   int         edje_cache; // GUI
   int         edje_collection_cache; // GUI
   int         zone_desks_x_count; // GUI
   int         zone_desks_y_count; // GUI
   int         use_virtual_roots; // NO GUI - maybe remove?
   int         show_desktop_icons; // GUI
   int         edge_flip_dragging; // GUI
   int         evas_engine_default; // GUI
   int         evas_engine_container; // NO GUI - maybe remove?
   int         evas_engine_init; // NO GUI - maybe remove?
   int         evas_engine_menus; // NO GUI - maybe remove?
   int         evas_engine_borders; // NO GUI - maybe remove?
   int         evas_engine_errors; // NO GUI - maybe remove?
   int         evas_engine_popups; // NO GUI - maybe remove?
   int         evas_engine_drag; // NO GUI - maybe remove?
   int         evas_engine_win; // NO GUI - maybe remove?
   int         evas_engine_zone; // NO GUI - maybe remove?
   int	       use_composite; // GUI
   const char *language; // GUI
   Eina_List  *modules; // GUI
   Eina_List  *font_fallbacks; // GUI
   Eina_List  *font_defaults; // GUI
   Eina_List  *themes; // GUI
   Eina_List  *mouse_bindings; // GUI
   Eina_List  *key_bindings; // GUI
   Eina_List  *edge_bindings; // GUI
   Eina_List  *signal_bindings;
   Eina_List  *wheel_bindings; // GUI
   Eina_List  *acpi_bindings; // GUI
   Eina_List  *path_append_data; // GUI
   Eina_List  *path_append_images; // GUI
   Eina_List  *path_append_fonts; // GUI
   Eina_List  *path_append_themes; // GUI
   Eina_List  *path_append_init; // GUI
   Eina_List  *path_append_icons; // GUI
   Eina_List  *path_append_modules; // GUI
   Eina_List  *path_append_backgrounds; // GUI
   Eina_List  *path_append_messages; // GUI
   int         window_placement_policy; // GUI
   int         focus_policy; // GUI
   int         focus_setting; // GUI
   int         pass_click_on; // GUI
   int         always_click_to_raise; // GUI
   int         always_click_to_focus; // GUI
   int         use_auto_raise; // GUI
   double      auto_raise_delay; // GUI
   int         use_resist; // GUI
   int         drag_resist;
   int         desk_resist; // GUI
   int         window_resist; // GUI
   int         gadget_resist; // GUI
   int         geometry_auto_move; // GUI
   int         geometry_auto_resize_limit; // GUI
   int         winlist_warp_while_selecting; // GUI
   int         winlist_warp_at_end; // GUI
   double      winlist_warp_speed; // GUI
   int         winlist_scroll_animate; // GUI
   double      winlist_scroll_speed; // GUI
   int         winlist_list_show_iconified; // GUI
   int         winlist_list_show_other_desk_iconified; // GUI
   int         winlist_list_show_other_screen_iconified; // GUI
   int         winlist_list_show_other_desk_windows; // GUI
   int         winlist_list_show_other_screen_windows; // GUI
   int         winlist_list_uncover_while_selecting; // GUI
   int         winlist_list_jump_desk_while_selecting; // GUI
   int         winlist_list_focus_while_selecting; // GUI
   int         winlist_list_raise_while_selecting; // GUI
   double      winlist_pos_align_x; // GUI
   double      winlist_pos_align_y; // GUI
   double      winlist_pos_size_w; // GUI
   double      winlist_pos_size_h; // GUI
   int         winlist_pos_min_w; // GUI
   int         winlist_pos_min_h; // GUI
   int         winlist_pos_max_w; // GUI
   int         winlist_pos_max_h; // GUI
   int         maximize_policy; // GUI
   int         allow_manip; // GUI
   int         border_fix_on_shelf_toggle; // GUI
   int         allow_above_fullscreen; // GUI
   int         kill_if_close_not_possible;
   int         kill_process;
   double      kill_timer_wait;
   int         ping_clients;
   const char *transition_start; // GUI
   const char *transition_desk; // GUI
   const char *transition_change; // GUI
   Eina_List  *remembers;
   int         remember_internal_windows; // GUI
   int         move_info_follows; // GUI
   int         resize_info_follows; // GUI
   int         move_info_visible; // GUI
   int         resize_info_visible; // GUI
   int         focus_last_focused_per_desktop; // GUI
   int         focus_revert_on_hide_or_close; // GUI
   int         pointer_slide; // GUI
   int         use_e_cursor; // GUI
   int         cursor_size; // GUI
   int         menu_autoscroll_margin; // GUI
   int         menu_autoscroll_cursor_margin; // GUI
   const char *input_method; // GUI
   struct {
	int    move;
	int    resize;
	int    raise;
	int    lower;
	int    layer;
	int    desktop;
	int    iconify;
   } transient;
   int         modal_windows;
   int         menu_eap_name_show; // GUI
   int         menu_eap_generic_show; // GUI
   int         menu_eap_comment_show; // GUI
   int         menu_favorites_show; // GUI
   int         menu_apps_show; // GUI
   int         menu_gadcon_client_toplevel; // GUI
   int         fullscreen_policy; // GUI
   const char *exebuf_term_cmd; // GUI
   Eina_List  *color_classes; // GUI
   int         use_app_icon; // GUI
   int         cnfmdlg_disabled; // GUI
   int         cfgdlg_auto_apply; // GUI
   int         cfgdlg_default_mode; // GUI
   Eina_List  *gadcons; // GUI
   Eina_List  *shelves; // GUI
   int         font_hinting; // GUI

   const char *desklock_personal_passwd; // GUI
   const char *desklock_background; // GUI
   int         desklock_auth_method; // GUI
   int         desklock_login_box_zone; // GUI
   int         desklock_start_locked; // GUI
   int         desklock_on_suspend; // GUI
   int         desklock_autolock_screensaver; // GUI
   double      desklock_post_screensaver_time; // GUI
   int         desklock_autolock_idle; // GUI
   double      desklock_autolock_idle_timeout; // GUI
   int         desklock_use_custom_desklock; // GUI
   const char *desklock_custom_desklock_cmd; // GUI
   Eina_Bool   desklock_ask_presentation; // GUI
   double      desklock_ask_presentation_timeout; // GUI

   int         screensaver_enable; // GUI
   int         screensaver_timeout; // GUI
   int         screensaver_interval; // GUI
   int         screensaver_blanking; // GUI
   int         screensaver_expose; // GUI
   Eina_Bool   screensaver_ask_presentation; // GUI
   double      screensaver_ask_presentation_timeout; // GUI

   int         dpms_enable; // GUI
   int         dpms_standby_enable; // GUI
   int         dpms_standby_timeout; // GUI
   int         dpms_suspend_enable; // GUI
   int         dpms_suspend_timeout; // GUI
   int         dpms_off_enable; // GUI
   int         dpms_off_timeout; // GUI

   int         clientlist_group_by; // GUI
   int         clientlist_include_all_zones; // GUI
   int         clientlist_separate_with; // GUI
   int         clientlist_sort_by; // GUI
   int         clientlist_separate_iconified_apps; // GUI
   int         clientlist_warp_to_iconified_desktop; // GUI
   int         clientlist_limit_caption_len; // GUI
   int         clientlist_max_caption_len; // GUI

   int         mouse_hand; //GUI
   int         mouse_accel_numerator; // GUI
   int         mouse_accel_denominator; // GUI
   int         mouse_accel_threshold; // GUI

   Eina_List   *screen_info; // GUI

   int         border_raise_on_mouse_action; // GUI
   int         border_raise_on_focus; // GUI
   int         desk_flip_wrap; // GUI
   int         fullscreen_flip; // GUI

   const char *icon_theme; // GUI
   Eina_Bool   icon_theme_overrides; // GUI

   int         desk_flip_animate_mode; // GUI
   int         desk_flip_animate_interpolation; // GUI
   double      desk_flip_animate_time; // GUI
   Eina_Bool   desk_flip_pan_bg;
   double      desk_flip_pan_x_axis_factor;
   double      desk_flip_pan_y_axis_factor;

   const char *wallpaper_import_last_dev; // INTERNAL
   const char *wallpaper_import_last_path; // INTERNAL

   int wallpaper_grad_c1_r; // INTERNAL
   int wallpaper_grad_c1_g; // INTERNAL
   int wallpaper_grad_c1_b; // INTERNAL
   int wallpaper_grad_c2_r; // INTERNAL
   int wallpaper_grad_c2_g; // INTERNAL
   int wallpaper_grad_c2_b; // INTERNAL

   const char *theme_default_border_style; // GUI

   Eina_List *mime_icons; // GUI
   int desk_auto_switch; // GUI;

   int thumb_nice;

   int ping_clients_interval;
   int cache_flush_poll_interval; // GUI

   int thumbscroll_enable; // GUI
   int thumbscroll_threshhold; // GUI
   double thumbscroll_momentum_threshhold; // GUI
   double thumbscroll_friction; // GUI

   int device_desktop;
   int device_auto_mount;
   int device_auto_open;

   struct {
      double timeout;
      struct {
	 unsigned char dx;
	 unsigned char dy;
      } move;
      struct {
	 unsigned char dx;
	 unsigned char dy;
      } resize;
   } border_keyboard;

   struct {
      double min; // GUI
      double max; // GUI
      double factor; // GUI
      int base_dpi; // GUI
      unsigned char use_dpi; // GUI
      unsigned char use_custom; // GUI
   } scale;

   unsigned char show_cursor; // GUI
   unsigned char idle_cursor; // GUI

   const char *default_system_menu;

   unsigned char cfgdlg_normal_wins; // GUI

   struct {
      struct {
         int icon_size;
      } main, secondary, extra;
      double timeout;
      unsigned char do_input;
      Eina_List *actions;
   } syscon;

   struct {
      Eina_Bool presentation;
      Eina_Bool offline;
   } mode;

   struct {
      double    expire_timeout;
      Eina_Bool show_run_dialog;
      Eina_Bool show_exit_dialog;
   } exec;
   
   unsigned char null_container_win;
   
   Eina_List *env_vars;
};

struct _E_Config_Env_Var
{
   const char *var;
   const char *val;
   unsigned char unset;
};

struct _E_Config_Syscon_Action
{
   const char *action;
   const char *params;
   const char *button;
   const char *icon;
   int         is_main;
};

struct _E_Config_Module
{
   const char    *name;
   unsigned char  enabled;
   unsigned char  delayed;
   int            priority;
};

struct _E_Config_Theme
{
   const char    *category;
   const char    *file;
};

struct _E_Config_Binding_Mouse
{
   int            context;
   int            modifiers;
   const char    *action;
   const char    *params;
   unsigned char  button;
   unsigned char  any_mod;
};

struct _E_Config_Binding_Key
{
   int            context;
   int            modifiers;
   const char    *key;
   const char    *action;
   const char    *params;
   unsigned char  any_mod;
};

struct _E_Config_Binding_Edge
{
   int            context;
   int            modifiers;
   float	  delay;
   const char    *action;
   const char    *params;
   unsigned char  edge;
   unsigned char  any_mod;
};

struct _E_Config_Binding_Signal
{
   int            context;
   const char    *signal;
   const char    *source;
   int            modifiers;
   unsigned char  any_mod;
   const char    *action;
   const char    *params;
};

struct _E_Config_Binding_Wheel
{
   int            context;
   int            direction;
   int            z;
   int            modifiers;
   unsigned char  any_mod;
   const char    *action;
   const char    *params;
};

struct _E_Config_Binding_Acpi
{
   int context, type, status;
   const char *action, *params;
};

struct _E_Config_Desktop_Background
{
   int            container;
   int            zone;
   int            desk_x;
   int            desk_y;
   const char    *file;
};

struct _E_Config_Desktop_Name
{
   int            container;
   int            zone;
   int            desk_x;
   int            desk_y;
   const char    *name;
};

struct _E_Config_Gadcon
{
   const char *name;
   int         id;
   int         zone;
   Eina_List  *clients;
};

struct _E_Config_Gadcon_Client
{
   const char    *name;
   const char    *id;
   struct {
      int pos, size, res;                   //gadcon
      double pos_x, pos_y, size_w, size_h;  //gadman
   } geom;
   struct {
      int seq, flags;
   } state_info;
   const char    *style;
   int            orient;
   unsigned char  autoscroll;
   unsigned char  resizable;
};

struct _E_Config_Shelf
{
   const char   *name;
   int           id;
   int           container, zone;
   int           layer;
   unsigned char popup;
   int           orient;
   unsigned char fit_along;
   unsigned char fit_size;
   const char   *style;
   int           size;
   int           overlap;
   int		 autohide;
   int           autohide_show_action;
   float	 hide_timeout;
   float	 hide_duration;
   int		 desk_show_mode;
   Eina_List    *desk_list;
};

struct _E_Config_Shelf_Desk
{
   int x, y;
};

struct _E_Config_Mime_Icon
{
   const char *mime;
   const char *icon;
};

struct _E_Event_Config_Icon_Theme
{
   const char *icon_theme;
};

EINTERN int        e_config_init(void);
EINTERN int        e_config_shutdown(void);

EAPI void       e_config_load(void);

EAPI int        e_config_save(void);
EAPI void       e_config_save_flush(void);
EAPI void       e_config_save_queue(void);

EAPI const char*e_config_profile_get(void);
EAPI char      *e_config_profile_dir_get(const char *prof);
EAPI void       e_config_profile_set(const char *prof);
EAPI Eina_List *e_config_profile_list(void);
EAPI void       e_config_profile_add(const char *prof);
EAPI void       e_config_profile_del(const char *prof);

EAPI Eina_List *e_config_engine_list(void);

EAPI void       e_config_save_block_set(int block);
EAPI int        e_config_save_block_get(void);

EAPI void      *e_config_domain_load(const char *domain, E_Config_DD *edd);
EAPI void      *e_config_domain_system_load(const char *domain, E_Config_DD *edd);
EAPI int        e_config_profile_save(void);
EAPI int        e_config_domain_save(const char *domain, E_Config_DD *edd, const void *data);

EAPI E_Config_Binding_Mouse  *e_config_binding_mouse_match(E_Config_Binding_Mouse *eb_in);
EAPI E_Config_Binding_Key    *e_config_binding_key_match(E_Config_Binding_Key *eb_in);
EAPI E_Config_Binding_Edge   *e_config_binding_edge_match(E_Config_Binding_Edge *eb_in);
EAPI E_Config_Binding_Signal *e_config_binding_signal_match(E_Config_Binding_Signal *eb_in);
EAPI E_Config_Binding_Wheel  *e_config_binding_wheel_match(E_Config_Binding_Wheel *eb_in);
EAPI E_Config_Binding_Acpi   *e_config_binding_acpi_match(E_Config_Binding_Acpi *eb_in);
EAPI void                     e_config_mode_changed(void);

extern EAPI E_Config *e_config;

extern EAPI int E_EVENT_CONFIG_ICON_THEME;
extern EAPI int E_EVENT_CONFIG_MODE_CHANGED;

#endif
#endif
