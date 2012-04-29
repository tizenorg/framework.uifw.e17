#include "e_int_config_randr.h"
#include "e_randr.h"

#ifndef  ECORE_X_RANDR_1_2
#define ECORE_X_RANDR_1_2   ((1 << 16) | 2)
#endif
#ifndef  ECORE_X_RANDR_1_3
#define ECORE_X_RANDR_1_3   ((1 << 16) | 3)
#endif

#ifndef  Ecore_X_Randr_Unset
#define Ecore_X_Randr_Unset -1
#endif

#define E_RANDR_12 (e_randr_screen_info.rrvd_info.randr_info_12)

Evas_Object *policy_widget_basic_create_widgets(Evas *canvas);
Eina_Bool    policy_widget_create_data(E_Config_Dialog_Data *e_config_runtime_info);
Eina_Bool    policy_widget_basic_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
Eina_Bool    policy_widget_basic_check_changed(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
Eina_Bool    policy_widget_basic_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
void         policy_widget_update_radio_buttons(Evas_Object *rep);

//static void  _policy_widget_policy_mouse_up_cb(void *data, Evas *e, Evas_Object *obj, void *event_info);
extern E_Config_Dialog_Data *e_config_runtime_info;
extern char _theme_file_path[];

static const char *_POLICIES_STRINGS[] = {
     "ABOVE",
     "RIGHT",
     "BELOW",
     "LEFT",
     "CLONE",
     "NONE"};


/*
   static void
   _policy_widget_radio_add_callbacks(void)
   {
   evas_object_event_callback_add(e_config_runtime_info->gui.widgets.policies.radio_none, EVAS_CALLBACK_MOUSE_UP, _policy_widget_policy_mouse_up_cb, NULL);
   evas_object_event_callback_add(e_config_runtime_info->gui.widgets.policies.radio_clone, EVAS_CALLBACK_MOUSE_UP, _policy_widget_policy_mouse_up_cb, NULL);
   evas_object_event_callback_add(e_config_runtime_info->gui.widgets.policies.radio_left, EVAS_CALLBACK_MOUSE_UP, _policy_widget_policy_mouse_up_cb, NULL);
   evas_object_event_callback_add(e_config_runtime_info->gui.widgets.policies.radio_below, EVAS_CALLBACK_MOUSE_UP, _policy_widget_policy_mouse_up_cb, NULL);
   evas_object_event_callback_add(e_config_runtime_info->gui.widgets.policies.radio_above, EVAS_CALLBACK_MOUSE_UP, _policy_widget_policy_mouse_up_cb, NULL);
   evas_object_event_callback_add(e_config_runtime_info->gui.widgets.policies.radio_right, EVAS_CALLBACK_MOUSE_UP, _policy_widget_policy_mouse_up_cb, NULL);
   }
 */

Eina_Bool
policy_widget_create_data(E_Config_Dialog_Data *e_config_runtime_info)
{
   E_Config_Randr_Dialog_Output_Dialog_Data *odd;
   E_Randr_Output_Info *oi = NULL;
   Eina_List *iter;

   if (!e_config_runtime_info || !e_config_runtime_info->output_dialog_data_list) return EINA_FALSE;

   EINA_LIST_FOREACH(e_config_runtime_info->output_dialog_data_list, iter, odd)
     {
        if (odd->crtc)
          oi = eina_list_data_get(odd->crtc->outputs);
        else if (odd->output)
          oi = odd->output;
        if (!oi)
          continue;
        odd->previous_policy = oi->policy;
        odd->new_policy = oi->policy;
        fprintf(stderr, "CONF_RANDR: Read in policy of %d as %s.\n", oi->xid, _POLICIES_STRINGS[odd->new_policy - 1]);
     }

   return EINA_TRUE;
}

Evas_Object *
policy_widget_basic_create_widgets(Evas *canvas)
{
   Evas_Object *widget;
   E_Radio_Group *rg;
   //char signal[29];

   if (!canvas || !e_config_runtime_info) return NULL;

   if (e_config_runtime_info->gui.widgets.policies.widget) return e_config_runtime_info->gui.widgets.policies.widget;

   if (!(widget = e_widget_framelist_add(canvas, _("Screen attachement policy"), EINA_FALSE))) return NULL;

   // Add radio buttons
   if (!(rg = e_widget_radio_group_new(&e_config_runtime_info->gui.widgets.policies.radio_val))) goto _policy_widget_radio_add_fail;

   //IMPROVABLE: use enum to determine objects via 'switch'-statement
   e_config_runtime_info->gui.widgets.policies.radio_above = e_widget_radio_add(canvas, _("Above"), ECORE_X_RANDR_OUTPUT_POLICY_ABOVE, rg);
   e_widget_framelist_object_append(widget, e_config_runtime_info->gui.widgets.policies.radio_above);

   e_config_runtime_info->gui.widgets.policies.radio_right = e_widget_radio_add(canvas, _("Right"), ECORE_X_RANDR_OUTPUT_POLICY_RIGHT, rg);
   e_widget_framelist_object_append(widget, e_config_runtime_info->gui.widgets.policies.radio_right);

   e_config_runtime_info->gui.widgets.policies.radio_below = e_widget_radio_add(canvas, _("Below"), ECORE_X_RANDR_OUTPUT_POLICY_BELOW, rg);
   e_widget_framelist_object_append(widget, e_config_runtime_info->gui.widgets.policies.radio_below);

   e_config_runtime_info->gui.widgets.policies.radio_left = e_widget_radio_add(canvas, _("Left"), ECORE_X_RANDR_OUTPUT_POLICY_LEFT, rg);
   e_widget_framelist_object_append(widget, e_config_runtime_info->gui.widgets.policies.radio_left);

   e_config_runtime_info->gui.widgets.policies.radio_clone = e_widget_radio_add(canvas, _("Clone display content"), ECORE_X_RANDR_OUTPUT_POLICY_CLONE, rg);
   e_widget_framelist_object_append(widget, e_config_runtime_info->gui.widgets.policies.radio_clone);

   e_config_runtime_info->gui.widgets.policies.radio_none = e_widget_radio_add(canvas, _("No reaction"), ECORE_X_RANDR_OUTPUT_POLICY_NONE, rg);
   e_widget_framelist_object_append(widget, e_config_runtime_info->gui.widgets.policies.radio_none);

   //_policy_widget_radio_add_callbacks();

   /*
      // Add policies demonstration edje
      if (!(e_config_runtime_info->gui.widgets.policies.swallowing_edje = edje_object_add(canvas)))
      {
        goto _policy_widget_edje_add_fail;

      }
      if (!edje_object_file_set(e_config_runtime_info->gui.widgets.policies.swallowing_edje, _theme_file_path, "e/conf/randr/dialog/widget/policies"))
      {
        goto _policy_widget_edje_set_fail;
      }

      e_widget_table_object_align_append(widget, e_config_runtime_info->gui.widgets.policies.swallowing_edje, 1, 0, 1, 1, 1, 1, 1, 1, 1.0, 1.0);
    */

   /*
      evas_object_show(e_config_runtime_info->gui.widgets.policies.swallowing_edje);

      //emit signal to edje so a demonstration can be shown
      snprintf(signal, sizeof(signal), "conf,randr,dialog,policies,%d", e_randr_screen_info->rrvd_info.randr_info_12->output_policy);
      edje_object_signal_emit(e_config_runtime_info->gui.widgets.policies.swallowing_edje, signal, "e");
      fprintf(stderr, "CONF_RANDR: Initial signal emitted to policy dialog: %s\n", signal);

      //Use theme's background as screen representation
      e_config_runtime_info->gui.widgets.policies.new_display = edje_object_add(canvas);
      e_theme_edje_object_set(e_config_runtime_info->gui.widgets.policies.new_display, "base/theme/widgets", "e/widgets/frame");
      e_config_runtime_info->gui.widgets.policies.new_display_background = edje_object_add(canvas);
      e_theme_edje_object_set(e_config_runtime_info->gui.widgets.policies.new_display_background, "base/theme/background", "e/desktop/background");
      edje_object_part_swallow(e_config_runtime_info->gui.widgets.policies.new_display, "e.swallow.content", e_config_runtime_info->gui.widgets.policies.new_display_background);
      edje_object_part_text_set(e_config_runtime_info->gui.widgets.policies.new_display, "e.text.label", _("New display"));
      edje_object_part_swallow(e_config_runtime_info->gui.widgets.policies.swallowing_edje, "new_display.swallow.content", e_config_runtime_info->gui.widgets.policies.new_display);
      //add theme's frame
      //for now use the theme's background for the new display as well
      e_config_runtime_info->gui.widgets.policies.current_displays_setup = edje_object_add(canvas);
      e_theme_edje_object_set(e_config_runtime_info->gui.widgets.policies.current_displays_setup, "base/theme/widgets", "e/widgets/frame");
      e_config_runtime_info->gui.widgets.policies.current_displays_setup_background = edje_object_add(canvas);
      e_theme_edje_object_set(e_config_runtime_info->gui.widgets.policies.current_displays_setup_background, "base/theme/background", "e/desktop/background");
      edje_object_part_swallow(e_config_runtime_info->gui.widgets.policies.current_displays_setup, "e.swallow.content", e_config_runtime_info->gui.widgets.policies.current_displays_setup_background);
      edje_object_part_text_set(e_config_runtime_info->gui.widgets.policies.current_displays_setup, "e.text.label", _("Used display"));
      edje_object_part_swallow(e_config_runtime_info->gui.widgets.policies.swallowing_edje, "current_displays_setup.swallow.content", e_config_runtime_info->gui.widgets.policies.current_displays_setup);
    */

   evas_object_show(widget);

   return widget;

   /*
      _policy_widget_edje_set_fail:
      evas_object_del(e_config_runtime_info->gui.widgets.policies.swallowing_edje);
      _policy_widget_edje_add_fail:
      fprintf(stderr, "CONF_RANDR: Couldn't set edj for policies widget!\n");
      evas_object_del(widget);
      return NULL;
    */
_policy_widget_radio_add_fail:
   evas_object_del(widget);
   return NULL;
}

/*
 * The current dialog does not demonstrate what the policies mean, so disabled
 * for now.
 *
static void
_policy_widget_policy_mouse_up_cb(void *data __UNUSED__, Evas *e __UNUSED__, Evas_Object *obj, void *event_info __UNUSED__)
{
   char signal[29];
   int policy = ECORE_X_RANDR_OUTPUT_POLICY_NONE;

    * IMPROVABLE:
    * "sadly" the evas callbacks are called before radio_val is set to its new
    * value. If that is ever changed, remove the used code below and just use the
    * 1-liner below.
    * snprintf(signal, sizeof(signal), "conf,randr,dialog,policies,%d", e_config_runtime_info->gui.widgets.policies.radio_val);
    * /
   if (obj == e_config_runtime_info->gui.widgets.policies.radio_above) policy = ECORE_X_RANDR_OUTPUT_POLICY_ABOVE;
   if (obj == e_config_runtime_info->gui.widgets.policies.radio_right) policy = ECORE_X_RANDR_OUTPUT_POLICY_RIGHT;
   if (obj == e_config_runtime_info->gui.widgets.policies.radio_below) policy = ECORE_X_RANDR_OUTPUT_POLICY_BELOW;
   if (obj == e_config_runtime_info->gui.widgets.policies.radio_left) policy = ECORE_X_RANDR_OUTPUT_POLICY_LEFT;
   if (obj == e_config_runtime_info->gui.widgets.policies.radio_clone) policy = ECORE_X_RANDR_OUTPUT_POLICY_CLONE;
   if (obj == e_config_runtime_info->gui.widgets.policies.radio_none) policy = ECORE_X_RANDR_OUTPUT_POLICY_NONE;

   snprintf(signal, sizeof(signal), "conf,randr,dialog,policies,%d", policy);

   //edje_object_signal_emit(e_config_runtime_info->gui.widgets.policies.swallowing_edje, signal, "e");

   fprintf(stderr, "CONF_RANDR: mouse button released. Emitted signal to policy: %s\n", signal);
}
*/

Eina_Bool
policy_widget_basic_apply_data(E_Config_Dialog *cfd __UNUSED__, E_Config_Dialog_Data *cfdata __UNUSED__)
{
   if (!E_RANDR_12 || !e_config_runtime_info->gui.selected_output_dd) return EINA_FALSE;

   //policy update
   e_config_runtime_info->gui.selected_output_dd->previous_policy = e_config_runtime_info->gui.selected_output_dd->new_policy;
   e_config_runtime_info->gui.selected_output_dd->new_policy = e_config_runtime_info->gui.widgets.policies.radio_val;
   fprintf(stderr, "CONF_RANDR: 'New display attached'-policy set to %s.\n", _POLICIES_STRINGS[e_config_runtime_info->gui.selected_output_dd->new_policy - 1]);

   return EINA_TRUE;
}

Eina_Bool
policy_widget_basic_check_changed(E_Config_Dialog *cfd __UNUSED__, E_Config_Dialog_Data *cfdata)
{
   if (!E_RANDR_12 || !cfdata || !cfdata->gui.selected_output_dd || !cfdata->gui.selected_output_dd->output) return EINA_FALSE;

   return (int)cfdata->gui.selected_output_dd->previous_policy != (int)cfdata->gui.widgets.policies.radio_val;
}

void
policy_widget_update_radio_buttons(Evas_Object *rep)
{
   E_Config_Randr_Dialog_Output_Dialog_Data *output_dialog_data;
   Ecore_X_Randr_Output_Policy policy;

   //disable widgets, if no rep is selected
   if (!rep || !(output_dialog_data = evas_object_data_get(rep, "rep_info")))
     {
        //Evas_Object *radio_above, *radio_right, *radio_below, *radio_left, *radio_clone, *radio_none;
        e_widget_disabled_set(e_config_runtime_info->gui.widgets.policies.radio_above, EINA_TRUE);
        e_widget_disabled_set(e_config_runtime_info->gui.widgets.policies.radio_right, EINA_TRUE);
        e_widget_disabled_set(e_config_runtime_info->gui.widgets.policies.radio_below, EINA_TRUE);
        e_widget_disabled_set(e_config_runtime_info->gui.widgets.policies.radio_left, EINA_TRUE);
        e_widget_disabled_set(e_config_runtime_info->gui.widgets.policies.radio_clone, EINA_TRUE);
        e_widget_disabled_set(e_config_runtime_info->gui.widgets.policies.radio_none, EINA_TRUE);
        return;
     }
   else
     {
        e_widget_disabled_set(e_config_runtime_info->gui.widgets.policies.radio_above, EINA_FALSE);
        e_widget_disabled_set(e_config_runtime_info->gui.widgets.policies.radio_right, EINA_FALSE);
        e_widget_disabled_set(e_config_runtime_info->gui.widgets.policies.radio_below, EINA_FALSE);
        e_widget_disabled_set(e_config_runtime_info->gui.widgets.policies.radio_left, EINA_FALSE);
        e_widget_disabled_set(e_config_runtime_info->gui.widgets.policies.radio_clone, EINA_FALSE);
        e_widget_disabled_set(e_config_runtime_info->gui.widgets.policies.radio_none, EINA_FALSE);
     }

   policy = output_dialog_data->new_policy;
   e_config_runtime_info->gui.selected_output_dd = output_dialog_data;
   //toggle the switch of the currently used policies
   switch (policy)
     {
      case ECORE_X_RANDR_OUTPUT_POLICY_RIGHT:
        e_widget_radio_toggle_set(e_config_runtime_info->gui.widgets.policies.radio_right, EINA_TRUE);
        break;

      case ECORE_X_RANDR_OUTPUT_POLICY_BELOW:
        e_widget_radio_toggle_set(e_config_runtime_info->gui.widgets.policies.radio_below, EINA_TRUE);
        break;

      case ECORE_X_RANDR_OUTPUT_POLICY_LEFT:
        e_widget_radio_toggle_set(e_config_runtime_info->gui.widgets.policies.radio_left, EINA_TRUE);
        break;

      case ECORE_X_RANDR_OUTPUT_POLICY_CLONE:
        e_widget_radio_toggle_set(e_config_runtime_info->gui.widgets.policies.radio_clone, EINA_TRUE);
        break;

      case ECORE_X_RANDR_OUTPUT_POLICY_NONE:
        e_widget_radio_toggle_set(e_config_runtime_info->gui.widgets.policies.radio_none, EINA_TRUE);
        break;

      default: //== ECORE_X_RANDR_OUTPUT_POLICY_ABOVE:
        e_widget_radio_toggle_set(e_config_runtime_info->gui.widgets.policies.radio_above, EINA_TRUE);
     }
}

void
policy_widget_keep_changes(E_Config_Dialog_Data *cfdata)
{
   E_Config_Randr_Dialog_Output_Dialog_Data *odd;
   E_Randr_Output_Info *oi;
   Eina_List *iter;

   if (!cfdata || !cfdata->gui.selected_output_dd || !cfdata->gui.selected_output_dd->output) return;

   odd = cfdata->gui.selected_output_dd;
   odd->previous_policy = odd->new_policy;
   if (odd->crtc)
     {
        EINA_LIST_FOREACH(odd->crtc->outputs, iter, oi)
          {
             oi->policy = odd->new_policy;
             fprintf(stderr, "CONF_RANDR: Policy change to %s kept for output %d.\n", _POLICIES_STRINGS[odd->new_policy - 1], oi->xid);
          }
     }
   else if (odd->output)
     {
        odd->output->policy = odd->new_policy;
        fprintf(stderr, "CONF_RANDR: Policy change to %s kept for output %d.\n", _POLICIES_STRINGS[odd->new_policy - 1], odd->output->xid);
     }
}

void
policy_widget_discard_changes(E_Config_Dialog_Data *cfdata)
{
   E_Config_Randr_Dialog_Output_Dialog_Data *odd;

   if (!cfdata || !cfdata->gui.selected_output_dd || !cfdata->gui.selected_output_dd->output) return;

   odd = cfdata->gui.selected_output_dd;
   odd->new_policy = odd->previous_policy;
}

