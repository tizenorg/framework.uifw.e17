#include "e.h"

/* local subsystem functions */
static void _e_msgbus_request_name_cb(void        *data,
                                      DBusMessage *msg,
                                      DBusError   *err);

static DBusMessage *_e_msgbus_core_restart_cb(E_DBus_Object *obj,
                                              DBusMessage   *msg);
static DBusMessage *_e_msgbus_core_shutdown_cb(E_DBus_Object *obj,
                                               DBusMessage   *msg);

static DBusMessage *_e_msgbus_module_load_cb(E_DBus_Object *obj,
                                             DBusMessage   *msg);
static DBusMessage *_e_msgbus_module_unload_cb(E_DBus_Object *obj,
                                               DBusMessage   *msg);
static DBusMessage *_e_msgbus_module_enable_cb(E_DBus_Object *obj,
                                               DBusMessage   *msg);
static DBusMessage *_e_msgbus_module_disable_cb(E_DBus_Object *obj,
                                                DBusMessage   *msg);
static DBusMessage *_e_msgbus_module_list_cb(E_DBus_Object *obj,
                                             DBusMessage   *msg);

static DBusMessage *_e_msgbus_profile_set_cb(E_DBus_Object *obj,
                                             DBusMessage   *msg);
static DBusMessage *_e_msgbus_profile_get_cb(E_DBus_Object *obj,
                                             DBusMessage   *msg);
static DBusMessage *_e_msgbus_profile_list_cb(E_DBus_Object *obj,
                                              DBusMessage   *msg);
static DBusMessage *_e_msgbus_profile_add_cb(E_DBus_Object *obj,
                                             DBusMessage   *msg);
static DBusMessage *_e_msgbus_profile_delete_cb(E_DBus_Object *obj,
                                                DBusMessage   *msg);

/* local subsystem globals */
static E_Msgbus_Data *_e_msgbus_data = NULL;

/* externally accessible functions */
EINTERN int
e_msgbus_init(void)
{
   E_DBus_Interface *iface;

   _e_msgbus_data = E_NEW(E_Msgbus_Data, 1);

   e_dbus_init();
#ifdef HAVE_HAL
   e_hal_init();
#endif

   _e_msgbus_data->conn = e_dbus_bus_get(DBUS_BUS_SESSION);
   if (!_e_msgbus_data->conn)
     {
        printf("WARNING: Cannot get DBUS_BUS_SESSION\n");
        return 0;
     }
   e_dbus_request_name(_e_msgbus_data->conn, "org.enlightenment.wm.service", 0, _e_msgbus_request_name_cb, NULL);
   _e_msgbus_data->obj = e_dbus_object_add(_e_msgbus_data->conn, "/org/enlightenment/wm/RemoteObject", NULL);

   iface = e_dbus_interface_new("org.enlightenment.wm.Core");
   if (!iface)
     {
        printf("WARNING: Cannot add org.enlightenment.wm.Core interface\n");
        return 0;
     }
   e_dbus_object_interface_attach(_e_msgbus_data->obj, iface);
   e_dbus_interface_unref(iface);

   /* Hardcore methods */
   e_dbus_interface_method_add(iface, "Restart", "", "", _e_msgbus_core_restart_cb);
   e_dbus_interface_method_add(iface, "Shutdown", "", "", _e_msgbus_core_shutdown_cb);

   iface = e_dbus_interface_new("org.enlightenment.wm.Module");
   if (!iface)
     {
        printf("WARNING: Cannot add org.enlightenment.wm.Module interface\n");
        return 0;
     }
   e_dbus_object_interface_attach(_e_msgbus_data->obj, iface);
   e_dbus_interface_unref(iface);

   /* Module methods */
   e_dbus_interface_method_add(iface, "Load", "s", "", _e_msgbus_module_load_cb);
   e_dbus_interface_method_add(iface, "Unload", "s", "", _e_msgbus_module_unload_cb);
   e_dbus_interface_method_add(iface, "Enable", "s", "", _e_msgbus_module_enable_cb);
   e_dbus_interface_method_add(iface, "Disable", "s", "", _e_msgbus_module_disable_cb);
   e_dbus_interface_method_add(iface, "List", "", "a(si)", _e_msgbus_module_list_cb);

   iface = e_dbus_interface_new("org.enlightenment.wm.Profile");
   if (!iface)
     {
        printf("WARNING: Cannot add org.enlightenment.wm.Profile interface\n");
        return 0;
     }
   e_dbus_object_interface_attach(_e_msgbus_data->obj, iface);
   e_dbus_interface_unref(iface);

   /* Profile methods */
   e_dbus_interface_method_add(iface, "Set", "s", "", _e_msgbus_profile_set_cb);
   e_dbus_interface_method_add(iface, "Get", "", "s", _e_msgbus_profile_get_cb);
   e_dbus_interface_method_add(iface, "List", "", "as", _e_msgbus_profile_list_cb);
   e_dbus_interface_method_add(iface, "Add", "s", "", _e_msgbus_profile_add_cb);
   e_dbus_interface_method_add(iface, "Delete", "s", "", _e_msgbus_profile_delete_cb);

   return 1;
}

EINTERN int
e_msgbus_shutdown(void)
{
   if (_e_msgbus_data->obj)
     {
        e_dbus_object_free(_e_msgbus_data->obj);
     }
   if (_e_msgbus_data->conn)
     {
        e_dbus_connection_close(_e_msgbus_data->conn);
     }
#ifdef HAVE_HAL
   e_hal_shutdown();
#endif
   e_dbus_shutdown();

   E_FREE(_e_msgbus_data);
   _e_msgbus_data = NULL;
   return 1;
}

EAPI void
e_msgbus_interface_attach(E_DBus_Interface *iface)
{
   if (!_e_msgbus_data->obj) return;
   e_dbus_object_interface_attach(_e_msgbus_data->obj, iface);
}

EAPI void
e_msgbus_interface_detach(E_DBus_Interface *iface)
{
   if (!_e_msgbus_data->obj) return;
   e_dbus_object_interface_detach(_e_msgbus_data->obj, iface);
}

static void
_e_msgbus_request_name_cb(void        *data __UNUSED__,
                          DBusMessage *msg __UNUSED__,
                          DBusError   *err __UNUSED__)
{
//TODO Handle Errors
}

/* Core Handlers */
static DBusMessage *
_e_msgbus_core_restart_cb(E_DBus_Object *obj __UNUSED__,
                          DBusMessage   *msg)
{
   e_sys_action_do(E_SYS_RESTART, NULL);
   return dbus_message_new_method_return(msg);
}

static DBusMessage *
_e_msgbus_core_shutdown_cb(E_DBus_Object *obj __UNUSED__,
                           DBusMessage   *msg)
{
   e_sys_action_do(E_SYS_EXIT, NULL);
   return dbus_message_new_method_return(msg);
}

/* Modules Handlers */
static DBusMessage *
_e_msgbus_module_load_cb(E_DBus_Object *obj __UNUSED__,
                         DBusMessage   *msg)
{
   DBusMessageIter iter;
   char *module;

   dbus_message_iter_init(msg, &iter);
   dbus_message_iter_get_basic(&iter, &module);

   if (!e_module_find(module))
     {
        e_module_new(module);
        e_config_save_queue();
     }

   return dbus_message_new_method_return(msg);
}

static DBusMessage *
_e_msgbus_module_unload_cb(E_DBus_Object *obj __UNUSED__,
                           DBusMessage   *msg)
{
   DBusMessageIter iter;
   char *module;
   E_Module *m;

   dbus_message_iter_init(msg, &iter);
   dbus_message_iter_get_basic(&iter, &module);

   if ((m = e_module_find(module)))
     {
        e_module_disable(m);
        e_object_del(E_OBJECT(m));
        e_config_save_queue();
     }

   return dbus_message_new_method_return(msg);
}

static DBusMessage *
_e_msgbus_module_enable_cb(E_DBus_Object *obj __UNUSED__,
                           DBusMessage   *msg)
{
   DBusMessageIter iter;
   char *module;
   E_Module *m;

   dbus_message_iter_init(msg, &iter);
   dbus_message_iter_get_basic(&iter, &module);

   if ((m = e_module_find(module)))
     {
        e_module_enable(m);
        e_config_save_queue();
     }

   return dbus_message_new_method_return(msg);
}

static DBusMessage *
_e_msgbus_module_disable_cb(E_DBus_Object *obj __UNUSED__,
                            DBusMessage   *msg)
{
   DBusMessageIter iter;
   char *module;
   E_Module *m;

   dbus_message_iter_init(msg, &iter);
   dbus_message_iter_get_basic(&iter, &module);

   if ((m = e_module_find(module)))
     {
        e_module_disable(m);
        e_config_save_queue();
     }

   return dbus_message_new_method_return(msg);
}

static DBusMessage *
_e_msgbus_module_list_cb(E_DBus_Object *obj __UNUSED__,
                         DBusMessage   *msg)
{
   Eina_List *l;
   E_Module *mod;
   DBusMessage *reply;
   DBusMessageIter iter;
   DBusMessageIter arr;

   reply = dbus_message_new_method_return(msg);
   dbus_message_iter_init_append(reply, &iter);
   dbus_message_iter_open_container(&iter, DBUS_TYPE_ARRAY, "(si)", &arr);

   EINA_LIST_FOREACH(e_module_list(), l, mod)
     {
        DBusMessageIter sub;
        const char *name;
        int enabled;

        name = mod->name;
        enabled = mod->enabled;
        dbus_message_iter_open_container(&arr, DBUS_TYPE_STRUCT, NULL, &sub);
        dbus_message_iter_append_basic(&sub, DBUS_TYPE_STRING, &(name));
        dbus_message_iter_append_basic(&sub, DBUS_TYPE_INT32, &(enabled));
        dbus_message_iter_close_container(&arr, &sub);
     }
   dbus_message_iter_close_container(&iter, &arr);

   return reply;
}

/* Profile Handlers */
static DBusMessage *
_e_msgbus_profile_set_cb(E_DBus_Object *obj __UNUSED__,
                         DBusMessage   *msg)
{
   DBusMessageIter iter;
   char *profile;

   dbus_message_iter_init(msg, &iter);
   dbus_message_iter_get_basic(&iter, &profile);

   e_config_save_flush();
   e_config_profile_set(profile);
   e_config_profile_save();
   e_config_save_block_set(1);
   e_sys_action_do(E_SYS_RESTART, NULL);

   return dbus_message_new_method_return(msg);
}

static DBusMessage *
_e_msgbus_profile_get_cb(E_DBus_Object *obj __UNUSED__,
                         DBusMessage   *msg)
{
   DBusMessageIter iter;
   DBusMessage *reply;
   const char *profile;

   profile = e_config_profile_get();

   reply = dbus_message_new_method_return(msg);
   dbus_message_iter_init_append(reply, &iter);
   dbus_message_iter_append_basic(&iter, DBUS_TYPE_STRING, &profile);

   return reply;
}

static DBusMessage *
_e_msgbus_profile_list_cb(E_DBus_Object *obj __UNUSED__,
                          DBusMessage   *msg)
{
   Eina_List *l;
   const char *name;
   DBusMessage *reply;
   DBusMessageIter iter;
   DBusMessageIter arr;

   reply = dbus_message_new_method_return(msg);
   dbus_message_iter_init_append(reply, &iter);
   dbus_message_iter_open_container(&iter, DBUS_TYPE_ARRAY, "s", &arr);

   EINA_LIST_FOREACH(e_config_profile_list(), l, name)
     {
        dbus_message_iter_append_basic(&arr, DBUS_TYPE_STRING, &name);
     }
   dbus_message_iter_close_container(&iter, &arr);

   return reply;
}

static DBusMessage *
_e_msgbus_profile_add_cb(E_DBus_Object *obj __UNUSED__,
                         DBusMessage   *msg)
{
   DBusMessageIter iter;
   char *profile;

   dbus_message_iter_init(msg, &iter);
   dbus_message_iter_get_basic(&iter, &profile);

   e_config_profile_add(profile);

   return dbus_message_new_method_return(msg);
}

static DBusMessage *
_e_msgbus_profile_delete_cb(E_DBus_Object *obj __UNUSED__,
                            DBusMessage   *msg)
{
   DBusMessageIter iter;
   char *profile;

   dbus_message_iter_init(msg, &iter);
   dbus_message_iter_get_basic(&iter, &profile);
   if (!strcmp(e_config_profile_get(), profile))
     {
        DBusMessage *ret;

        ret = dbus_message_new_error(msg, "org.enlightenment.DBus.InvalidArgument",
                                     "Can't delete active profile");
        return ret;
     }
   e_config_profile_del(profile);

   return dbus_message_new_method_return(msg);
}
