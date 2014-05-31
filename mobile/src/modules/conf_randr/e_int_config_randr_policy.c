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

static void _policy_widget_mouse_up_cb(void *data, Evas_Object *obj, void *event_info);
extern E_Config_Dialog_Data *e_config_runtime_info;
extern char _theme_file_path[];

static const char *_POLICIES_STRINGS[] = {
     "ABOVE",
     "RIGHT",
     "BELOW",
     "LEFT",
     "CLONE",
     "NONE",
     "ASK"};

Eina_Bool
policy_widget_create_data(E_Config_Dialog_Data *data)
{
   E_Config_Randr_Dialog_Output_Dialog_Data *odd;
   E_Randr_Output_Info *oi = NULL;
   Eina_List *iter;

   if (!data || !data->output_dialog_data_list) return EINA_FALSE;

   EINA_LIST_FOREACH(data->output_dialog_data_list, iter, odd)
     {
        if (odd->crtc)
          oi = eina_list_data_get(odd->crtc->outputs);
        else
          oi = odd->output;

        odd->previous_policy = oi ? oi->policy : (Ecore_X_Randr_Orientation) Ecore_X_Randr_Unset;
        odd->new_policy = Ecore_X_Randr_Unset;
        if (!oi) //Not of interest for dbg output
          continue;
        fprintf(stderr, "CONF_RANDR: Read in policy of %d as %s.\n", oi->xid, ((odd->previous_policy != (Ecore_X_Randr_Output_Policy) Ecore_X_Randr_Unset) ? _POLICIES_STRINGS[odd->previous_policy - 1] : "unset"));
     }

   return EINA_TRUE;
}

void
policy_widget_free_cfdata(E_Config_Dialog *cfd __UNUSED__, E_Config_Dialog_Data *cfdata __UNUSED__)
{
}

Evas_Object *
policy_widget_basic_create_widgets(Evas *canvas)
{
   Evas_Object *widget, *ro;
   E_Radio_Group *rg;
   //char signal[29];

   if (!canvas || !e_config_runtime_info) return NULL;

   if (e_config_runtime_info->gui.widgets.policy.widget) return e_config_runtime_info->gui.widgets.policy.widget;

   if (!(widget = e_widget_framelist_add(canvas, _("Screen attachement policy"), EINA_FALSE))) return NULL;

   // Add radio buttons
   if (!(rg = e_widget_radio_group_new(&e_config_runtime_info->gui.widgets.policy.radio_val))) goto _policy_widget_radio_add_fail;

   //IMPROVABLE: use enum to determine objects via 'switch'-statement
   ro = e_widget_radio_add(canvas, _("Ask"), ECORE_X_RANDR_OUTPUT_POLICY_ASK, rg);
   evas_object_smart_callback_add(ro, "changed", _policy_widget_mouse_up_cb, NULL);
   e_widget_framelist_object_append(widget, ro);
   e_config_runtime_info->gui.widgets.policy.radio_ask = ro;

   ro = e_widget_radio_add(canvas, _("Above"), ECORE_X_RANDR_OUTPUT_POLICY_ABOVE, rg);
   evas_object_smart_callback_add(ro, "changed", _policy_widget_mouse_up_cb, NULL);
   e_widget_framelist_object_append(widget, ro);
   e_config_runtime_info->gui.widgets.policy.radio_above = ro;

   ro = e_widget_radio_add(canvas, _("Right"), ECORE_X_RANDR_OUTPUT_POLICY_RIGHT, rg);
   evas_object_smart_callback_add(ro, "changed", _policy_widget_mouse_up_cb, NULL);
   e_widget_framelist_object_append(widget, ro);
   e_config_runtime_info->gui.widgets.policy.radio_right = ro;

   ro = e_widget_radio_add(canvas, _("Below"), ECORE_X_RANDR_OUTPUT_POLICY_BELOW, rg);
   evas_object_smart_callback_add(ro, "changed", _policy_widget_mouse_up_cb, NULL);
   e_widget_framelist_object_append(widget, ro);
   e_config_runtime_info->gui.widgets.policy.radio_below = ro;

   ro = e_widget_radio_add(canvas, _("Left"), ECORE_X_RANDR_OUTPUT_POLICY_LEFT, rg);
   evas_object_smart_callback_add(ro, "changed", _policy_widget_mouse_up_cb, NULL);
   e_widget_framelist_object_append(widget, ro);
   e_config_runtime_info->gui.widgets.policy.radio_left = ro;

   ro = e_widget_radio_add(canvas, _("Clone display content"), ECORE_X_RANDR_OUTPUT_POLICY_CLONE, rg);
   evas_object_smart_callback_add(ro, "changed", _policy_widget_mouse_up_cb, NULL);
   e_widget_framelist_object_append(widget, ro);
   e_config_runtime_info->gui.widgets.policy.radio_clone = ro;

   ro = e_widget_radio_add(canvas, _("No reaction"), ECORE_X_RANDR_OUTPUT_POLICY_NONE, rg);
   evas_object_smart_callback_add(ro, "changed", _policy_widget_mouse_up_cb, NULL);
   e_widget_framelist_object_append(widget, ro);
   e_config_runtime_info->gui.widgets.policy.radio_none = ro;


   /*
      // Add policies demonstration edje
      if (!(e_config_runtime_info->gui.widgets.policy.swallowing_edje = edje_object_add(canvas)))
      {
        goto _policy_widget_edje_add_fail;

      }
      if (!edje_object_file_set(e_config_runtime_info->gui.widgets.policy.swallowing_edje, _theme_file_path, "e/conf/randr/dialog/widget/policies"))
      {
        goto _policy_widget_edje_set_fail;
      }

      e_widget_table_object_align_append(widget, e_config_runtime_info->gui.widgets.policy.swallowing_edje, 1, 0, 1, 1, 1, 1, 1, 1, 1.0, 1.0);
    */

   /*
      evas_object_show(e_config_runtime_info->gui.widgets.policy.swallowing_edje);

      //emit signal to edje so a demonstration can be shown
      snprintf(signal, sizeof(signal), "conf,randr,dialog,policies,%d", e_randr_screen_info->rrvd_info.randr_info_12->output_policy);
      edje_object_signal_emit(e_config_runtime_info->gui.widgets.policy.swallowing_edje, signal, "e");
      fprintf(stderr, "CONF_RANDR: Initial signal emitted to policy dialog: %s\n", signal);

      //Use theme's background as screen representation
      e_config_runtime_info->gui.widgets.policy.new_display = edje_object_add(canvas);
      e_theme_edje_object_set(e_config_runtime_info->gui.widgets.policy.new_display, "base/theme/widgets", "e/widgets/frame");
      e_config_runtime_info->gui.widgets.policy.new_display_background = edje_object_add(canvas);
      e_theme_edje_object_set(e_config_runtime_info->gui.widgets.policy.new_display_background, "base/theme/background", "e/desktop/background");
      edje_object_part_swallow(e_config_runtime_info->gui.widgets.policy.new_display, "e.swallow.content", e_config_runtime_info->gui.widgets.policy.new_display_background);
      edje_object_part_text_set(e_config_runtime_info->gui.widgets.policy.new_display, "e.text.label", _("New display"));
      edje_object_part_swallow(e_config_runtime_info->gui.widgets.policy.swallowing_edje, "new_display.swallow.content", e_config_runtime_info->gui.widgets.policy.new_display);
      //add theme's frame
      //for now use the theme's background for the new display as well
      e_config_runtime_info->gui.widgets.policy.current_displays_setup = edje_object_add(canvas);
      e_theme_edje_object_set(e_config_runtime_info->gui.widgets.policy.current_displays_setup, "base/theme/widgets", "e/widgets/frame");
      e_config_runtime_info->gui.widgets.policy.current_displays_setup_background = edje_object_add(canvas);
      e_theme_edje_object_set(e_config_runtime_info->gui.widgets.policy.current_displays_setup_background, "base/theme/background", "e/desktop/background");
      edje_object_part_swallow(e_config_runtime_info->gui.widgets.policy.current_displays_setup, "e.swallow.content", e_config_runtime_info->gui.widgets.policy.current_displays_setup_background);
      edje_object_part_text_set(e_config_runtime_info->gui.widgets.policy.current_displays_setup, "e.text.label", _("Used display"));
      edje_object_part_swallow(e_config_runtime_info->gui.widgets.policy.swallowing_edje, "current_displays_setup.swallow.content", e_config_runtime_info->gui.widgets.policy.current_displays_setup);
    */

   evas_object_show(widget);

   return widget;

   /*
      _policy_widget_edje_set_fail:
      evas_object_del(e_config_runtime_info->gui.widgets.policy.swallowing_edje);
      _policy_widget_edje_add_fail:
      fprintf(stderr, "CONF_RANDR: Couldn't set edj for policies widget!\n");
      evas_object_del(widget);
      return NULL;
    */
_policy_widget_radio_add_fail:
   evas_object_del(widget);
   return NULL;
}

static void
_policy_widget_mouse_up_cb(void *data __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   //char signal[29];
   if (!e_config_runtime_info->gui.selected_output_dd)
     return;

   e_config_runtime_info->gui.selected_output_dd->new_policy = e_config_runtime_info->gui.widgets.policy.radio_val;
/*
 * The current dialog does not demonstrate what the policies mean, so disabled
 * for now.
 *
 * snprintf(signal, sizeof(signal), "conf,randr,dialog,policies,%d", e_config_runtime_info->gui.widgets.policy.radio_val);
   //edje_object_signal_emit(e_config_runtime_info->gui.widgets.policy.swallowing_edje, signal, "e");
   fprintf(stderr, "CONF_RANDR: mouse button released. Emitted signal to policy: %s\n", signal);
*/
}

Eina_Bool
policy_widget_basic_apply_data(E_Config_Dialog *cfd __UNUSED__, E_Config_Dialog_Data *cfdata)
{
   E_Config_Randr_Dialog_Output_Dialog_Data *odd;
   Eina_List *it, *it2;
   E_Randr_Output_Info *oi = NULL;

   if (!E_RANDR_12 || !cfdata->output_dialog_data_list) return EINA_FALSE;

   EINA_LIST_FOREACH(cfdata->output_dialog_data_list, it, odd)
     {
       if (odd->new_policy == (Ecore_X_Randr_Output_Policy) Ecore_X_Randr_Unset)
          continue;
        if (odd->crtc)
          {
             EINA_LIST_FOREACH(odd->crtc->outputs, it2, oi)
               {
                  oi->policy = odd->new_policy;
		  fprintf(stderr, "CONF_RANDR: 'New display attached'-policy for output %d set to %s.\n", odd->output ? odd->output->xid : 0, _POLICIES_STRINGS[odd->new_policy - 1]);
               }
          }
        else if (odd->output)
          {
             odd->output->policy = odd->new_policy;
             fprintf(stderr, "CONF_RANDR: 'New display attached'-policy for output %d set to %s.\n", odd->output->xid, _POLICIES_STRINGS[odd->new_policy - 1]);
          }
     }

   return EINA_TRUE;
}

Eina_Bool
policy_widget_basic_check_changed(E_Config_Dialog *cfd __UNUSED__, E_Config_Dialog_Data *cfdata)
{
   E_Config_Randr_Dialog_Output_Dialog_Data *odd;
   Eina_List *it;

   if (!E_RANDR_12 || !cfdata->output_dialog_data_list) return EINA_FALSE;

   EINA_LIST_FOREACH(cfdata->output_dialog_data_list, it, odd)
     {
        if ((odd->new_policy == (Ecore_X_Randr_Output_Policy) Ecore_X_Randr_Unset) ||
            (odd->previous_policy == (Ecore_X_Randr_Output_Policy) Ecore_X_Randr_Unset))
          continue;
        if (odd->new_policy != odd->previous_policy)
          return EINA_TRUE;
     }

   return EINA_FALSE;
}

void
policy_widget_update_radio_buttons(E_Config_Randr_Dialog_Output_Dialog_Data *odd)
{
   Ecore_X_Randr_Output_Policy policy;
   Eina_Bool enable = !odd;

   //disable widgets, if no rep is selected
   e_widget_disabled_set(e_config_runtime_info->gui.widgets.policy.radio_ask, enable);
   e_widget_disabled_set(e_config_runtime_info->gui.widgets.policy.radio_above, enable);
   e_widget_disabled_set(e_config_runtime_info->gui.widgets.policy.radio_right, enable);
   e_widget_disabled_set(e_config_runtime_info->gui.widgets.policy.radio_below, enable);
   e_widget_disabled_set(e_config_runtime_info->gui.widgets.policy.radio_left, enable);
   e_widget_disabled_set(e_config_runtime_info->gui.widgets.policy.radio_clone, enable);
   e_widget_disabled_set(e_config_runtime_info->gui.widgets.policy.radio_none, enable);

   if (!odd) return;

   policy = (odd->new_policy != (Ecore_X_Randr_Output_Policy) Ecore_X_Randr_Unset) ? odd->new_policy : odd->previous_policy;
   //toggle the switch of the currently used policies
   switch (policy)
     {
      case ECORE_X_RANDR_OUTPUT_POLICY_ASK:
        e_widget_radio_toggle_set(e_config_runtime_info->gui.widgets.policy.radio_ask, EINA_TRUE);
        break;

      case ECORE_X_RANDR_OUTPUT_POLICY_RIGHT:
        e_widget_radio_toggle_set(e_config_runtime_info->gui.widgets.policy.radio_right, EINA_TRUE);
        break;

      case ECORE_X_RANDR_OUTPUT_POLICY_BELOW:
        e_widget_radio_toggle_set(e_config_runtime_info->gui.widgets.policy.radio_below, EINA_TRUE);
        break;

      case ECORE_X_RANDR_OUTPUT_POLICY_LEFT:
        e_widget_radio_toggle_set(e_config_runtime_info->gui.widgets.policy.radio_left, EINA_TRUE);
        break;

      case ECORE_X_RANDR_OUTPUT_POLICY_CLONE:
        e_widget_radio_toggle_set(e_config_runtime_info->gui.widgets.policy.radio_clone, EINA_TRUE);
        break;

      case ECORE_X_RANDR_OUTPUT_POLICY_NONE:
        e_widget_radio_toggle_set(e_config_runtime_info->gui.widgets.policy.radio_none, EINA_TRUE);
        break;

      case ECORE_X_RANDR_OUTPUT_POLICY_ABOVE:
        e_widget_radio_toggle_set(e_config_runtime_info->gui.widgets.policy.radio_above, EINA_TRUE);
        break;

      default:
        break;
     }
}

void
policy_widget_keep_changes(E_Config_Dialog_Data *cfdata)
{
   E_Config_Randr_Dialog_Output_Dialog_Data *odd;
   Eina_List *it;

   if (!E_RANDR_12 || !cfdata->output_dialog_data_list)
     return;

   EINA_LIST_FOREACH(cfdata->output_dialog_data_list, it, odd)
     {
        odd->previous_policy = odd->new_policy;
        odd->new_policy = Ecore_X_Randr_Unset;
     }
}

void
policy_widget_discard_changes(E_Config_Dialog_Data *cfdata)
{
   E_Config_Randr_Dialog_Output_Dialog_Data *odd;
   Eina_List *it, *it2;
   E_Randr_Output_Info *oi = NULL;

   if (!E_RANDR_12 || !cfdata->output_dialog_data_list)
     return;

   EINA_LIST_FOREACH(cfdata->output_dialog_data_list, it, odd)
     {
        if (odd->crtc)
          {
             EINA_LIST_FOREACH(odd->crtc->outputs, it2, oi)
               {
                  oi->policy = odd->previous_policy;
                  fprintf(stderr, "CONF_RANDR: 'New display attached'-policy for output %d restored to %s.\n", oi->xid, _POLICIES_STRINGS[odd->previous_policy - 1]);
               }
          }
        else if (odd->output)
          {
             odd->output->policy = odd->previous_policy;
             fprintf(stderr, "CONF_RANDR: 'New display attached'-policy for output %d restored to %s.\n", odd->output->xid, _POLICIES_STRINGS[odd->previous_policy - 1]);
          }
     }

   return;
}
