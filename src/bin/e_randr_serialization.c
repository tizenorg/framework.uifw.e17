#include "e_randr_private.h"
#include "e_randr.h"

#define ECORE_X_RANDR_1_1   ((1 << 16) | 1)
#define ECORE_X_RANDR_1_2   ((1 << 16) | 2)
#define ECORE_X_RANDR_1_3   ((1 << 16) | 3)

/**********************************************************************
 *
 * Storage/Restorage of setups
 *
 **********************************************************************
 */

   E_Randr_Serialized_Setup *
_new_serialized_setup(void)
{
   return E_NEW(E_Randr_Serialized_Setup, 1);
}

   EAPI void
e_randr_store_configuration(E_Randr_Configuration_Store_Modifier modifier)
{
   if (!e_config->randr_serialized_setup)
     e_config->randr_serialized_setup = _new_serialized_setup();

   fprintf(stderr, "E_RANDR: Configuration shall be stored using the following modifier: %d.\n", modifier);

   if (e_randr_screen_info.randr_version == ECORE_X_RANDR_1_1)
     {
        _11_store_configuration(modifier);
     }
   else if (e_randr_screen_info.randr_version >= ECORE_X_RANDR_1_2)
     {
        _12_store_configuration(modifier);
     }
   e_config_save_queue();
}

Eina_Bool
_try_restore_configuration(void)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(e_config, EINA_FALSE);
   EINA_SAFETY_ON_NULL_RETURN_VAL(e_config->randr_serialized_setup, EINA_FALSE);

   if (e_randr_screen_info.randr_version == ECORE_X_RANDR_1_1)
     return _11_try_restore_configuration();
   else if (e_randr_screen_info.randr_version >= ECORE_X_RANDR_1_2)
     return _12_try_restore_configuration();

   return EINA_FALSE;
}

EINTERN void e_randr_serialized_setup_free(E_Randr_Serialized_Setup *ss)
{
   E_Randr_Serialized_Setup_12 *serialized_setup_12 = NULL;
   E_Randr_Serialized_Output_Policy *serialized_output_policy = NULL;

   EINA_SAFETY_ON_NULL_RETURN(ss);

   e_randr_11_serialized_setup_free(ss->serialized_setup_11);
   if (ss->serialized_setups_12)
     {
        EINA_LIST_FREE(ss->serialized_setups_12, serialized_setup_12)
          {
             e_randr_12_serialized_setup_free(serialized_setup_12);
          }
     }
   EINA_LIST_FREE(ss->outputs_policies, serialized_output_policy)
     {
        e_randr_12_serialized_output_policy_free(serialized_output_policy);
     }
   free(ss);
}
