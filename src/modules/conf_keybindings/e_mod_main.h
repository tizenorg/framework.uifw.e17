#ifndef E_MOD_MAIN_H
#define E_MOD_MAIN_H

#define E_TYPEDEFS 1
#include "e_int_config_keybindings.h"
#include "e_int_config_mousebindings.h"
#include "e_int_config_acpibindings.h"

#undef E_TYPEDEFS
#include "e_int_config_keybindings.h"
#include "e_int_config_mousebindings.h"
#include "e_int_config_acpibindings.h"

EAPI extern E_Module_Api e_modapi;

EAPI void *e_modapi_init     (E_Module *m);
EAPI int   e_modapi_shutdown (E_Module *m);
EAPI int   e_modapi_save     (E_Module *m);

/**
 * @addtogroup Optional_Conf
 * @{
 *
 * @defgroup Module_Conf_KeyBinding Key Bindings (Shortcuts) Configuration
 *
 * Configure global keyboard shortcuts.
 *
 * @see Module_Conf_MouseBinding
 * @see Module_Conf_EdgeBinding
 * @}
 */
#endif
