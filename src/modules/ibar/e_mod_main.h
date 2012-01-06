#ifndef E_MOD_MAIN_H
#define E_MOD_MAIN_H

typedef struct _Config      Config;
typedef struct _Config_Item Config_Item;

struct _Config
{
   /* saved * loaded config values */
   Eina_List       *items;
   /* just config state */
   E_Module        *module;
   E_Config_Dialog *config_dialog;
   Eina_List       *instances;
   E_Menu          *menu;
   Eina_List       *handlers;
};

struct _Config_Item
{
   const char *id;
   const char *dir;
   int show_label;
   int eap_label;
};

EAPI extern E_Module_Api e_modapi;

EAPI void *e_modapi_init     (E_Module *m);
EAPI int   e_modapi_shutdown (E_Module *m);
EAPI int   e_modapi_save     (E_Module *m);

void _ibar_config_update(Config_Item *ci);
void _config_ibar_module(Config_Item *ci);
extern Config *ibar_config;

#endif
