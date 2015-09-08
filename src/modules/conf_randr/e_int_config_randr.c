#include "e.h"
#include "e_randr.h"
#include "e_int_config_randr.h"

/*
 * BUGS:
 * - ethumb sometimes returns garbage objects leading to a segv
 *
 * TODO:
 * - write 1.2 per monitor configuration
 * - write Smart object, so crtcs representations can be properly freed (events,
 *   etc.)
 *
 * IMPROVABLE:
 *  See comments starting with 'IMPROVABLE'
 */
#ifndef  ECORE_X_RANDR_1_2
#define ECORE_X_RANDR_1_2 ((1 << 16) | 2)
#endif
#ifndef  ECORE_X_RANDR_1_3
#define ECORE_X_RANDR_1_3 ((1 << 16) | 3)
#endif

#ifdef  Ecore_X_Randr_None
#undef  Ecore_X_Randr_None
#define Ecore_X_Randr_None  0
#else
#define Ecore_X_Randr_None  0
#endif
#ifdef  Ecore_X_Randr_Unset
#undef  Ecore_X_Randr_Unset
#define Ecore_X_Randr_Unset -1
#else
#define Ecore_X_Randr_Unset -1
#endif

#define THEME_FILENAME      "/e-module-conf_randr.edj"
#define TOOLBAR_ICONSIZE    16
#define E_RANDR_12 (e_randr_screen_info.rrvd_info.randr_info_12)

static void        *create_data(E_Config_Dialog *cfd);
static void         free_cfdata(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static int          basic_check_changed(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static int          basic_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static Evas_Object *basic_create_widgets(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata);
static Eina_Bool    _deferred_noxrandr_error(void *data);
static void         _e_conf_randr_confirmation_dialog_discard_cb(void *data, E_Dialog *dia);

/* actual module specifics */
E_Config_Dialog_Data *e_config_runtime_info = NULL;
extern E_Module *conf_randr_module;
char _theme_file_path[PATH_MAX];

E_Config_Randr_Dialog_Output_Dialog_Data *
_dialog_output_dialog_data_new(E_Randr_Crtc_Info *crtc_info, E_Randr_Output_Info *output_info)
{
   E_Config_Randr_Dialog_Output_Dialog_Data *dialog_data;

   EINA_SAFETY_ON_NULL_RETURN_VAL(output_info, NULL);

   dialog_data = E_NEW(E_Config_Randr_Dialog_Output_Dialog_Data, 1);

   fprintf(stderr, "CONF_RANDR: Added output data struct for Output %d/CRTC %d.\n", output_info->xid, (output_info->crtc ? output_info->crtc->xid : Ecore_X_Randr_None));
   if (crtc_info)
     {
        //already enabled screen, output info is already available in crtc
        //struct
        dialog_data->crtc = crtc_info;
     }
   else
     {
        //disabled monitor
        dialog_data->output = output_info;
     }
   return dialog_data;
}

static void *
create_data(E_Config_Dialog *cfd)
{
   Eina_List *iter;
   E_Randr_Output_Info *output_info;
   E_Config_Randr_Dialog_Output_Dialog_Data *odd;

   // Prove we got all things to get going
   EINA_SAFETY_ON_TRUE_RETURN_VAL(!E_RANDR_12, NULL);
   //e_randr_screen_info_refresh();
   e_config_runtime_info = E_NEW(E_Config_Dialog_Data, 1);

   e_config_runtime_info->cfd = cfd;

   //Compose theme's file path and name
   snprintf(_theme_file_path, sizeof(_theme_file_path), "%s%s", conf_randr_module->dir, THEME_FILENAME);

   e_config_runtime_info->manager = e_manager_current_get();
   e_config_runtime_info->output_dialog_data_list = NULL;
   EINA_LIST_FOREACH(e_randr_screen_info.rrvd_info.randr_info_12->outputs, iter, output_info)
     {
        if (!output_info)
          {
             fprintf(stderr, "CONF_RANDR: WWWWWWWWWWWWOOOOOOOOOOOOOTTTT an output_info of the central struct is NULL!\n");
             continue;
          }
        if ((odd = _dialog_output_dialog_data_new(output_info->crtc, output_info)))
          e_config_runtime_info->output_dialog_data_list = eina_list_append(e_config_runtime_info->output_dialog_data_list, odd);
     }
   //FIXME: Properly (stack-like) free data when creation fails
   EINA_SAFETY_ON_FALSE_GOTO(resolution_widget_create_data(e_config_runtime_info), _e_conf_randr_create_data_failed_free_data);
   EINA_SAFETY_ON_FALSE_GOTO(arrangement_widget_create_data(e_config_runtime_info), _e_conf_randr_create_data_failed_free_data);
   EINA_SAFETY_ON_FALSE_GOTO(policy_widget_create_data(e_config_runtime_info), _e_conf_randr_create_data_failed_free_data);
   EINA_SAFETY_ON_FALSE_GOTO(orientation_widget_create_data(e_config_runtime_info), _e_conf_randr_create_data_failed_free_data);

   return e_config_runtime_info;

_e_conf_randr_create_data_failed_free_data:
   free(e_config_runtime_info);
   return NULL;
}

static void
free_cfdata(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata)
{
   E_Config_Randr_Dialog_Output_Dialog_Data *dialog_data;

   EINA_SAFETY_ON_TRUE_RETURN(!E_RANDR_12);

   arrangement_widget_free_cfdata(cfd, cfdata);
   policy_widget_free_cfdata(cfd, cfdata);
   resolution_widget_free_cfdata(cfd, cfdata);
   orientation_widget_free_cfdata(cfd, cfdata);

   /*
   evas_object_del(cfdata->gui.widgets.arrangement.widget);
   evas_object_del(cfdata->gui.widgets.policy.widget);
   evas_object_del(cfdata->gui.widgets.resolution.widget);
   evas_object_del(cfdata->gui.widgets.orientation.widget);
   */

   EINA_LIST_FREE(cfdata->output_dialog_data_list, dialog_data)
     {
        free(dialog_data);
     }
   cfdata->output_dialog_data_list = NULL;

   free(cfdata);
}

static Eina_Bool
_e_conf_randr_confirmation_dialog_timer_cb(void *data)
{
   E_Config_Randr_Dialog_Confirmation_Dialog_Data *cdd = (E_Config_Randr_Dialog_Confirmation_Dialog_Data *)data;
   char buf[4096];

   if (!cdd) return ECORE_CALLBACK_CANCEL;

   --cdd->countdown;

   if (cdd->countdown > 0)
     {
        snprintf(buf, sizeof(buf),
                 _("Does this look OK? Click <hilight>Keep</hilight> if it does, or Restore if not.<ps>"
                   "If you do not press a button, the previous settings will be<ps>"
                   "restored in %d seconds."), cdd->countdown);
     }
   else
     {
        snprintf(buf, sizeof(buf),
                 _("Does this look OK? Click <hilight>Keep</hilight> if it does, or Restore if not.<ps>"
                   "If you do not press a button, the previous settings will be<ps>"
                   "restored <highlight>IMMEDIATELY</highlight>."));
     }

   e_dialog_text_set(cdd->dialog, buf);

   if (cdd->countdown == 0)
      {
         _e_conf_randr_confirmation_dialog_discard_cb(cdd, cdd->dialog);
         return ECORE_CALLBACK_CANCEL;
      }
   return ECORE_CALLBACK_RENEW;
}

static void
_e_conf_randr_confirmation_dialog_delete_cb(E_Win *win)
{
   E_Dialog *dia;
   E_Config_Randr_Dialog_Confirmation_Dialog_Data *cd;
   E_Config_Dialog *cfd;

   dia = win->data;
   cd = dia->data;
   cd->cfdata->gui.confirmation_dialog = NULL;
   cfd = cd->cfdata->cfd;
   if (cd->timer) ecore_timer_del(cd->timer);
   cd->timer = NULL;
   free(cd);
   e_object_del(E_OBJECT(dia));
   e_object_unref(E_OBJECT(cfd));
}

static void
_e_conf_randr_confirmation_dialog_keep_cb(void *data, E_Dialog *dia)
{
   E_Config_Randr_Dialog_Confirmation_Dialog_Data *cdd = (E_Config_Randr_Dialog_Confirmation_Dialog_Data *)data;

   if (!cdd) return;

   //ordinary "keep" functionality
   arrangement_widget_keep_changes(cdd->cfdata);
   orientation_widget_keep_changes(cdd->cfdata);
   policy_widget_keep_changes(cdd->cfdata);
   resolution_widget_keep_changes(cdd->cfdata);

   //cleanup dialog
   _e_conf_randr_confirmation_dialog_delete_cb(dia->win);
}

static void
_e_conf_randr_confirmation_dialog_discard_cb(void *data, E_Dialog *dia)
{
   E_Config_Randr_Dialog_Confirmation_Dialog_Data *cdd = (E_Config_Randr_Dialog_Confirmation_Dialog_Data *)data;

   if (!cdd) return;

   arrangement_widget_discard_changes(cdd->cfdata);
   orientation_widget_discard_changes(cdd->cfdata);
   policy_widget_discard_changes(cdd->cfdata);
   resolution_widget_discard_changes(cdd->cfdata);
   _e_conf_randr_confirmation_dialog_delete_cb(dia->win);
}

static void
_e_conf_randr_confirmation_dialog_store_cb(void *data, E_Dialog *dia)
{
   E_Config_Randr_Dialog_Confirmation_Dialog_Data *cdd = (E_Config_Randr_Dialog_Confirmation_Dialog_Data *)data;
   E_Randr_Configuration_Store_Modifier modifier = 0;

   if (!cdd) return;

   //Create modifier
   if (policy_widget_basic_check_changed(NULL, e_config_runtime_info))
     modifier |= E_RANDR_CONFIGURATION_STORE_POLICIES;

   if (resolution_widget_basic_check_changed(NULL, e_config_runtime_info))
     modifier |= E_RANDR_CONFIGURATION_STORE_RESOLUTIONS;

   if (arrangement_widget_basic_check_changed(NULL, e_config_runtime_info))
     modifier |= E_RANDR_CONFIGURATION_STORE_ARRANGEMENT;

   if (orientation_widget_basic_check_changed(NULL, e_config_runtime_info))
     modifier |= E_RANDR_CONFIGURATION_STORE_ORIENTATIONS;

   _e_conf_randr_confirmation_dialog_keep_cb(data, dia);

   //but actually trigger saving the stuff
   e_randr_store_configuration(modifier);

}

static void
_e_conf_randr_confirmation_dialog_new(E_Config_Dialog *cfd)
{
   E_Config_Randr_Dialog_Confirmation_Dialog_Data *cd = E_NEW(E_Config_Randr_Dialog_Confirmation_Dialog_Data, 1);

   char buf[4096];

   if (!cd) return;

   cd->cfd = cfd;

   if ((cd->dialog = e_dialog_new(cfd->con, "E", "e_randr_confirmation_dialog")))
     {
        e_dialog_title_set(cd->dialog, _("New settings confirmation"));
        cd->cfdata = cfd->cfdata;
        cd->timer = ecore_timer_add(1.0, _e_conf_randr_confirmation_dialog_timer_cb, cd);
        cd->countdown = 15;
        cd->dialog->data = cd;
        e_dialog_icon_set(cd->dialog, "preferences-system-screen-resolution", 48);
        e_win_delete_callback_set(cd->dialog->win, _e_conf_randr_confirmation_dialog_delete_cb);
        snprintf(buf, sizeof(buf),
                 _("Does this look OK? Click <hilight>Keep</hilight> if it does, or Restore if not.<ps>"
                   "If you do not press a button, the previous settings will be<ps>"
                   "restored in %d seconds."), cd->countdown);
        e_dialog_text_set(cd->dialog, buf);
        e_dialog_button_add(cd->dialog, _("Keep"), NULL, _e_conf_randr_confirmation_dialog_keep_cb, cd);
        e_dialog_button_add(cd->dialog, _("Store Permanently"), NULL, _e_conf_randr_confirmation_dialog_store_cb, cd);
        e_dialog_button_add(cd->dialog, _("Restore"), NULL, _e_conf_randr_confirmation_dialog_discard_cb, cd);
        e_dialog_button_focus_num(cd->dialog, 1);
        e_win_centered_set(cd->dialog->win, 1);
        e_win_borderless_set(cd->dialog->win, 1);
        e_win_layer_set(cd->dialog->win, 6);
        e_win_sticky_set(cd->dialog->win, 1);
        e_dialog_show(cd->dialog);
        e_object_ref(E_OBJECT(cfd));
     }
}

static Evas_Object *
basic_create_widgets(E_Config_Dialog *cfd, Evas *canvas, E_Config_Dialog_Data *cfdata)
{
   Evas_Object *table = NULL, *wl = NULL;

   EINA_SAFETY_ON_TRUE_RETURN_VAL (!E_RANDR_12 || (e_randr_screen_info.randr_version < ECORE_X_RANDR_1_2), NULL);
   EINA_SAFETY_ON_TRUE_RETURN_VAL((!canvas || !cfdata), NULL);

   e_config_runtime_info->gui.canvas = canvas;

   if (!(cfdata->gui.widgets.arrangement.widget = arrangement_widget_basic_create_widgets(canvas))) goto _dialog_create_widget_arrangement_fail;
   if (!(cfdata->gui.widgets.policy.widget = policy_widget_basic_create_widgets(canvas))) goto _dialog_create_widget_policies_fail;
   if (!(cfdata->gui.widgets.resolution.widget = resolution_widget_basic_create_widgets(canvas))) goto _dialog_create_widget_resolutions_fail;
   if (!(cfdata->gui.widgets.orientation.widget = orientation_widget_basic_create_widgets(canvas))) goto _dialog_create_widget_orientation_fail;

   EINA_SAFETY_ON_FALSE_GOTO((table = e_widget_table_add(canvas, EINA_FALSE)), _dialog_create_widgets_fail);
   EINA_SAFETY_ON_FALSE_GOTO((wl = e_widget_list_add(canvas, EINA_FALSE, EINA_TRUE)), _dialog_create_widget_list_fail);

   //e_widget_table_object_append(Evas_Object *obj, Evas_Object *sobj, int col, int row, int colspan, int rowspan, int fill_w, int fill_h, int expand_w, int expand_h);
   e_widget_table_object_append(table, cfdata->gui.widgets.arrangement.widget, 1, 1, 1, 1, 1, 1, 1, 1);
   /*
      e_widget_table_object_append(table, cfdata->gui.widgets.policy.widget, 1, 2, 1, 1, 0, 0, 0, 0);
      e_widget_table_object_append(table, cfdata->gui.widgets.orientation.widget, 2, 2, 1, 1, 0, 0, 0, 0);
      e_widget_table_object_append(table, cfdata->gui.widgets.resolution.widget, 3, 2, 1, 1, EVAS_HINT_FILL, EVAS_HINT_FILL, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
    */
   //e_widget_list_object_append(Evas_Object *obj, Evas_Object *sobj, int fill, int expand, double align);
   e_widget_list_object_append(wl, cfdata->gui.widgets.policy.widget, 0, 0, 0.0);
   e_widget_list_object_append(wl, cfdata->gui.widgets.orientation.widget, 0, 0, 0.0);
   e_widget_list_object_append(wl, cfdata->gui.widgets.resolution.widget, EVAS_HINT_FILL, EVAS_HINT_EXPAND, 1.0);
   e_widget_table_object_append(table, wl, 1, 2, 1, 1, EVAS_HINT_FILL, EVAS_HINT_FILL, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   cfdata->gui.widget_list = wl;

   cfdata->gui.dialog = table;

   e_dialog_resizable_set(cfd->dia, EINA_TRUE);

   return cfdata->gui.dialog;

_dialog_create_widget_list_fail:
   evas_object_del(table);
_dialog_create_widgets_fail:
   evas_object_del(cfdata->gui.widgets.orientation.widget);
_dialog_create_widget_orientation_fail:
   evas_object_del(cfdata->gui.widgets.resolution.widget);
_dialog_create_widget_resolutions_fail:
   evas_object_del(cfdata->gui.widgets.policy.widget);
_dialog_create_widget_policies_fail:
   evas_object_del(cfdata->gui.widgets.arrangement.widget);
_dialog_create_widget_arrangement_fail:
   return NULL;
}

static int
basic_apply_data
  (E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata)
{
   Eina_Bool ret = EINA_TRUE;

   fprintf(stderr, "CONF_RANDR: New configuration is beeing applied.\n");
   //this is a special case, where the function is called, before the
   //configuration data is created.
   if (!cfdata) return EINA_FALSE;

   //the order matters except for policies!
   if (policy_widget_basic_check_changed(cfd, cfdata))
     {
        ret &= policy_widget_basic_apply_data(cfd, cfdata);
        if (!ret) return EINA_FALSE;
     }

   if (resolution_widget_basic_check_changed(cfd, cfdata))
     {
        ret &= resolution_widget_basic_apply_data(cfd, cfdata);
        if (!ret) return EINA_FALSE;
     }

   if (arrangement_widget_basic_check_changed(cfd, cfdata))
     {
        ret &= arrangement_widget_basic_apply_data(cfd, cfdata);
        if (!ret) return EINA_FALSE;
     }

   if (orientation_widget_basic_check_changed(cfd, cfdata))
     ret &= orientation_widget_basic_apply_data(cfd, cfdata);

   _e_conf_randr_confirmation_dialog_new(cfd);

   return ret;
}

E_Config_Dialog *
e_int_config_randr(E_Container *con, const char *params __UNUSED__){
   E_Config_Dialog *cfd;
   E_Config_Dialog_View *v;

   if (!E_RANDR_12 || (e_randr_screen_info.randr_version < ECORE_X_RANDR_1_2))
     {
        ecore_timer_add(0.5, _deferred_noxrandr_error, NULL);
        fprintf(stderr, "CONF_RANDR: XRandR version >= 1.2 necessary to work.\n");
        return NULL;
     }

   //Dialog already opened?
   if (e_config_dialog_find("E", "screen/screen_setup")) return NULL;

   v = E_NEW(E_Config_Dialog_View, 1);
   v->create_cfdata = create_data;
   v->free_cfdata = free_cfdata;
   v->basic.apply_cfdata = basic_apply_data;
   v->basic.create_widgets = basic_create_widgets;
   v->basic.check_changed = basic_check_changed;
   //v->override_auto_apply = 0;

   cfd = e_config_dialog_new(con, _("Screen Setup"),
                             "E", "screen/screen_setup",
                             "preferences-system-screen-setup", 0, v, NULL);
   return cfd;
}

static Eina_Bool
_deferred_noxrandr_error(void *data __UNUSED__)
{
   e_util_dialog_show(_("Missing Features"),
                      _("Your X Display Server is missing support for<ps>"
                        "the <hilight>XRandR</hilight> (X Resize and Rotate) extension version 1.2 or above.<ps>"
                        "You cannot change screen resolutions without<ps>"
                        "the support of this extension. It could also be<ps>"
                        "that at the time <hilight>ecore</hilight> was built, there<ps>"
                        "was no XRandR support detected."));
   return ECORE_CALLBACK_CANCEL;
}

static int
basic_check_changed(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata)
{
   if (!cfdata)
     return EINA_FALSE;

   return (arrangement_widget_basic_check_changed(cfd, cfdata)
         || policy_widget_basic_check_changed(cfd, cfdata)
         || orientation_widget_basic_check_changed(cfd, cfdata)
         || resolution_widget_basic_check_changed(cfd, cfdata));
}

