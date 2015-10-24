#include <Eldbus.h>
#include "e.h"
#include "dlog.h"
#include "e_test_helper.h"

#define BUS "org.enlightenment.wm"
#define PATH "/org/enlightenment/wm"
#define IFACE "org.enlightenment.wm.Test"

#undef LOG_TAG
#define LOG_TAG "E17"

typedef struct _Test_Helper_Data
{
   Eldbus_Connection *conn;
   Eldbus_Service_Interface *iface;

   Eina_List *hdlrs;

   struct
     {
        Ecore_Window win;
        E_Border *eb;
        int vis;
        Eina_Bool disuse;
     } registrant;

} Test_Helper_Data;

static Test_Helper_Data *th_data = NULL;

static Eina_Bool _e_test_helper_cb_property_get(const Eldbus_Service_Interface *iface, const char *name, Eldbus_Message_Iter *iter, const Eldbus_Message *msg, Eldbus_Message **err);

static Eldbus_Message* _e_test_helper_cb_register_window(const Eldbus_Service_Interface *iface, const Eldbus_Message *msg);
static Eldbus_Message* _e_test_helper_cb_deregister_window(const Eldbus_Service_Interface *iface, const Eldbus_Message *msg);

static Eldbus_Message* _e_test_helper_cb_get_clients(const Eldbus_Service_Interface *iface, const Eldbus_Message *msg);
static Eldbus_Message* _e_test_helper_cb_get_transientClient(const Eldbus_Service_Interface *iface, const Eldbus_Message *msg);

enum
{
   E_TEST_HELPER_SIGNAL_CHANGE_VISIBILITY = 0,
   E_TEST_HELPER_SIGNAL_RESTACK,
};

static const Eldbus_Signal signals[] = {
     [E_TEST_HELPER_SIGNAL_CHANGE_VISIBILITY] =
       {
          "VisibilityChanged",
          ELDBUS_ARGS({"ub", "window id, visibility"}),
          0
       },
     [E_TEST_HELPER_SIGNAL_RESTACK] =
       {
          "StackChanged",
          ELDBUS_ARGS({"u", "window id was restacked"}),
          0
       },
       { }
};

static const Eldbus_Method methods[] ={
       {
          "RegisterWindow",
          ELDBUS_ARGS({"u", "window id to be registered"}),
          ELDBUS_ARGS({"b", "accept or not"}),
          _e_test_helper_cb_register_window, 0
       },
       {
          "DeregisterWindow",
          ELDBUS_ARGS({"u", "window id to be deregistered"}),
          ELDBUS_ARGS({"b", "accept or not"}),
          _e_test_helper_cb_deregister_window, 0
       },
       {
          "GetClients",
          NULL,
          ELDBUS_ARGS({"a(uiiiiibu)", "array of eb"}),
          _e_test_helper_cb_get_clients, 0
       },
       {
          "GetTransientWinInfo",
          ELDBUS_ARGS({"ui", "client id and type of info"}),
          ELDBUS_ARGS({"a(uiiiiibu)", "array of eb"}),
          _e_test_helper_cb_get_transientClient, 0
       },
       { }
};

static const Eldbus_Property properties[] = {
       { "Registrant", "u", NULL, NULL, 0 },
       { }
};

static const Eldbus_Service_Interface_Desc iface_desc = {
     IFACE, methods, signals, properties, _e_test_helper_cb_property_get, NULL
};

static void
_e_test_helper_registrant_clear(void)
{
   EINA_SAFETY_ON_NULL_RETURN(th_data);

   th_data->registrant.win = 0;
   th_data->registrant.vis = -1;
   th_data->registrant.eb = NULL;
   th_data->registrant.disuse = EINA_FALSE;
}

void add_window_info(Eldbus_Message_Iter *iter , E_Border *bd)
{
   Eldbus_Message_Iter* subiter;
   LOGE("add_window_info:  start \n" );
   eldbus_message_iter_arguments_append(iter, "(uiiiiibu)", &subiter);

   eldbus_message_iter_arguments_append(subiter, "uiiiiibu",
                                        bd->client.win,
                                        bd->client.x, bd->client.y, bd->client.w, bd->client.h, bd->layer,
                                        bd->client.argb, bd->iconic);

   eldbus_message_iter_container_close(iter, subiter);

   LOGE("add_window_info: end \n");
}

void add_transient_info(Ecore_X_Window winid, Eldbus_Message_Iter* array_of_eb)
{
   Eina_List *list = NULL, *l;
   E_Border *bd;
   int i = 0;

   LOGE("%s start \n", __func__);

   E_Border *subparent = e_border_find_by_client_window(winid);

   if (subparent) list = subparent->transients;

   if (list)
     LOGE("found transient windows \n");
   else
     LOGE("Not-found transient windows \n");

   EINA_LIST_FOREACH(list, l, bd)
     {
        add_window_info(array_of_eb, bd);
        LOGE("added transient window[%d] %d \n", i, bd->client.win);
     }
   LOGE("%s end \n", __func__);
}

/**
 * dbus method to get the transient window info
 */
static Eldbus_Message*
_e_test_helper_cb_get_transientClient(const Eldbus_Service_Interface *iface EINA_UNUSED,
                                      const Eldbus_Message *msg)
{
   Eldbus_Message *reply;
   Eldbus_Message_Iter *iter;
   Eldbus_Message_Iter *array_of_eb;

   const char *name = NULL, *text = NULL;
   Eina_Bool res;

   int infotype = -1;
   Ecore_X_Window winid;

   LOGE("%s start \n", __func__);

   reply = eldbus_message_method_return_new(msg);

   res = eldbus_message_error_get(msg, &name, &text);
   if (EINA_TRUE == res)
     {
        LOGE("errname:%s errmsg:%s\n", name, text);
        return reply;
     }
   res = eldbus_message_arguments_get(msg, "ui", &winid, &infotype);
   if (EINA_FALSE == res)
     {
        LOGE("Error DBUS Arg Get \n");
        return reply;
     }
   if (winid<1 || infotype<0 || infotype>2)
     {
        LOGE("Invalid Arguments \n");
        return reply;
     }

   iter = eldbus_message_iter_get(reply);
   eldbus_message_iter_arguments_append(iter, "a(uiiiiibu)", &array_of_eb);

   E_Border *parent = NULL;
   E_Border *bd = e_border_find_by_client_window(winid);
   if (bd)
     {
        parent = bd->parent;
        if (parent) LOGE("====== Parent Window ID = %d ====== \n", parent->client.win);
     }

   switch (infotype)
     {
      case 0:
        if (parent) add_window_info(array_of_eb, parent);
        break;

      case 1:
        LOGE("============SWITCH 1 START============\n");
        add_transient_info(winid, array_of_eb);
        LOGE("Switch 1: updated transient childs info \n");

        LOGE("============SWITCH 1 END ============ \n");
        break;

      case 2:
        LOGE("============SWITCH 2 START============\n");
        if (parent)
          {
             add_window_info(array_of_eb, parent);
             LOGE("Switch 2: updated transient parent %d \n", parent->client.win);

             add_transient_info(winid, array_of_eb);
             LOGE("Switch 2: updated transient childs \n");

             LOGE("============SWITCH 2 END ============ \n");
          }
        break;
     }
   eldbus_message_iter_container_close(iter, array_of_eb);

   LOGE("%s END ... \n", __func__);

   return reply;
}

/**
 * dbus method to get the client window stack
 *
 * @return DbusMessage* - client window stack
 */
static Eldbus_Message *_e_test_helper_cb_get_clients(const Eldbus_Service_Interface *iface EINA_UNUSED, const Eldbus_Message *msg)
{
   Eldbus_Message *reply;
   Eldbus_Message_Iter *iter;
   Eldbus_Message_Iter *array_of_eb;

   reply = eldbus_message_method_return_new(msg);

   EINA_SAFETY_ON_NULL_GOTO(th_data, finish);

   iter = eldbus_message_iter_get(reply);
   eldbus_message_iter_arguments_append(iter, "a(uiiiiibu)", &array_of_eb);

   E_Container *cont = e_container_current_get(e_manager_current_get());
   E_Border_List *bl;
   E_Border *bd2;

   bl = e_container_border_list_last(cont);

   if (bl)
     {
        while (bd2 = e_container_border_list_prev(bl))
          {
             Eldbus_Message_Iter *struct_of_eb;

             eldbus_message_iter_arguments_append(array_of_eb, "(uiiiiibu)", &struct_of_eb);
             eldbus_message_iter_arguments_append(struct_of_eb, "uiiiiibu",
                                                  bd2->client.win,
                                                  bd2->client.x, bd2->client.y, bd2->client.w, bd2->client.h, bd2->layer,
                                                  bd2->client.argb, bd2->iconic);

             eldbus_message_iter_container_close(array_of_eb, struct_of_eb);
          }
        e_container_border_list_free(bl);
     }
   eldbus_message_iter_container_close(iter, array_of_eb);

finish:
   LOGE("============eldbus: _e_test_helper_cb_get_clients end============ \n");
   return reply;
}

/* Method Handlers */
static Eldbus_Message *
_e_test_helper_cb_register_window(const Eldbus_Service_Interface *iface EINA_UNUSED,
                                  const Eldbus_Message *msg)
{
   Eldbus_Message *reply;
   Ecore_Window id;

   LOGE("============_e_test_helper_cb_register_window start============ \n");

   reply = eldbus_message_method_return_new(msg);
   EINA_SAFETY_ON_NULL_GOTO(th_data, finish);

   if (!eldbus_message_arguments_get(msg, "u", &id))
     {
        LOGE("Error on eldbus_message_arguments_get()\n");
        return reply;
     }

   //printf("============ %s ============ regwin(0x%08x) \n", __func__, id);

   eldbus_message_arguments_append(reply, "b", !th_data->registrant.win);

   if (!th_data->registrant.win) th_data->registrant.win = id;

finish:
   LOGE("============_e_test_helper_cb_register_window end============ \n");
   return reply;
}

static Eldbus_Message *
_e_test_helper_cb_deregister_window(const Eldbus_Service_Interface *iface EINA_UNUSED,
                                    const Eldbus_Message *msg)
{
   Eldbus_Message *reply;
   Ecore_Window id;

   LOGE("============_e_test_helper_cb_deregister_window start============ \n");

   reply = eldbus_message_method_return_new(msg);

   EINA_SAFETY_ON_NULL_GOTO(th_data, finish);

   if (!eldbus_message_arguments_get(msg, "u", &id))
     {
        LOGE("Error on eldbus_message_arguments_get()\n");
        return reply;
     }

   //printf("============ %s ============ regwin(0x%08x) \n", __func__, id);

   eldbus_message_arguments_append(reply, "b", ((!th_data->registrant.win) ||
                                                ((th_data->registrant.win == id) &&
                                                 (th_data->registrant.vis != 1))));

   if (th_data->registrant.win == id)
     {
        th_data->registrant.disuse = EINA_TRUE;
        if (th_data->registrant.vis != 1) _e_test_helper_registrant_clear();
     }

finish:
   LOGE("============_e_test_helper_cb_deregister_window end============ \n");
   return reply;
}

/* Handle Client Visibility Changed Event & send signal*/
static Eina_Bool
_e_test_helper_cb_visibility_change(void *data EINA_UNUSED,
                                    int type EINA_UNUSED,
                                    void *event)
{
   E_Border *eb;
   Ecore_X_Event_Window_Visibility_Change *ev = event;
   Eldbus_Message *signal;

   eb = e_border_find_by_client_window(ev->win);

   EINA_SAFETY_ON_NULL_RETURN_VAL(th_data, ECORE_CALLBACK_PASS_ON);
   EINA_SAFETY_ON_NULL_RETURN_VAL(th_data->registrant.win, ECORE_CALLBACK_PASS_ON);

   if (th_data->registrant.win != ev->win)
     return ECORE_CALLBACK_PASS_ON;

   //printf("============ %s ============ regwin(0x%08x) \n", __func__, th_data->registrant.win);
   //printf("============ %s ============ window(0x%08x) \n", __func__, ev->win);

   if (!th_data->registrant.eb)
     th_data->registrant.eb = eb;

   if (th_data->registrant.vis != !ev->fully_obscured)
     {
        th_data->registrant.vis = !ev->fully_obscured;

        signal = eldbus_service_signal_new(th_data->iface, E_TEST_HELPER_SIGNAL_CHANGE_VISIBILITY);
        eldbus_message_arguments_append(signal, "ub", th_data->registrant.win, th_data->registrant.vis);
        eldbus_service_signal_send(th_data->iface, signal);
     }

   if ((th_data->registrant.disuse) && (!th_data->registrant.vis))
     _e_test_helper_registrant_clear();

   return ECORE_CALLBACK_PASS_ON;
}

/* Handle Client Restack Event & send signal*/
static Eina_Bool
_e_test_helper_cb_client_restack(void *data EINA_UNUSED, int type EINA_UNUSED, void *event)
{
   E_Event_Border_Stack *ev = event;
   E_Border *eb;
   Eldbus_Message *sig;
   Ecore_Window win;

   EINA_SAFETY_ON_NULL_RETURN_VAL(th_data, ECORE_CALLBACK_PASS_ON);
   EINA_SAFETY_ON_NULL_RETURN_VAL(th_data->registrant.eb, ECORE_CALLBACK_PASS_ON);

   eb = ev->border;

   if ((win = eb->client.win))
     {
        //printf("============ %s ============ regwin(0x%08x) \n", __func__, th_data->registrant.win);
        //printf("============ %s ============ window(0x%08x) \n", __func__, win);

        sig = eldbus_service_signal_new(th_data->iface, E_TEST_HELPER_SIGNAL_RESTACK);
        eldbus_message_arguments_append(sig, "u", win);
        eldbus_service_signal_send(th_data->iface, sig);
     }

   return ECORE_CALLBACK_PASS_ON;
}

/* Handle client remove Event & deregister window*/
static Eina_Bool
_e_test_helper_cb_client_remove(void *data EINA_UNUSED, int type EINA_UNUSED, void *event)
{
   E_Border *eb;
   E_Event_Border_Remove *ev = event;

   EINA_SAFETY_ON_NULL_RETURN_VAL(th_data, ECORE_CALLBACK_PASS_ON);

   eb = ev->border;

   if (!th_data->registrant.eb) return ECORE_CALLBACK_PASS_ON;
   if (eb != th_data->registrant.eb) return ECORE_CALLBACK_PASS_ON;

   printf("============ %s ============ window(0x%08x)\n", __func__, eb->client.win);

   _e_test_helper_registrant_clear();

   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_test_helper_cb_property_get(const Eldbus_Service_Interface *iface EINA_UNUSED, const char *name, Eldbus_Message_Iter *iter, const Eldbus_Message *msg EINA_UNUSED, Eldbus_Message **err EINA_UNUSED)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(th_data, EINA_FALSE);

   if (!e_util_strcmp(name, "Registrant"))
     {
        eldbus_message_iter_basic_append(iter, 'u', th_data->registrant.win);
     }

   return EINA_TRUE;
}

/**
 * to handle dbus errors
 */
static void
_e_test_helper_cb_name_request(void *data EINA_UNUSED,
                               const Eldbus_Message *msg,
                               Eldbus_Pending *pending EINA_UNUSED)
{
   unsigned int flag;
   const char *errname, *errmsg;

   if (eldbus_message_error_get(msg, &errname, &errmsg))
     {
        ERR("error on _e_test_helper_cb_name_request %s %s\n", errname, errmsg);
        e_test_helper_shutdown();
        return;
     }

   if (!eldbus_message_arguments_get(msg, "u", &flag))
     {
        ERR("error geting arguments on _e_test_helper_cb_name_request\n");
        e_test_helper_shutdown();
        return;
     }

   if (flag != ELDBUS_NAME_REQUEST_REPLY_PRIMARY_OWNER)
     {
        ERR("error name already in use\n");
        e_test_helper_shutdown();
     }
}

/* externally accessible functions */

/**
 * dbus connection initialization. Also adds dbus object and methods to communicate with client
 */
EINTERN int
e_test_helper_init(void)
{
   LOGE("============TEST HELPER Start============ \n");

   eldbus_init();

   th_data = E_NEW(Test_Helper_Data, 1);
   EINA_SAFETY_ON_NULL_RETURN_VAL(th_data, 0);

   th_data->conn = eldbus_connection_get(ELDBUS_CONNECTION_TYPE_SYSTEM);
   if (th_data->conn)
     {
        th_data->iface = eldbus_service_interface_register(th_data->conn,
                                                           PATH,
                                                           &iface_desc);
        if (!th_data->iface)
          {
             ERR("error registering eldbus interface in e_test_helper_init() \n");
             return 0;
          }

        eldbus_name_request(th_data->conn, BUS, ELDBUS_NAME_REQUEST_FLAG_DO_NOT_QUEUE,
                            _e_test_helper_cb_name_request, th_data->iface);

        th_data->hdlrs = eina_list_append(th_data->hdlrs, ecore_event_handler_add(ECORE_X_EVENT_WINDOW_VISIBILITY_CHANGE, _e_test_helper_cb_visibility_change, NULL));
        th_data->hdlrs = eina_list_append(th_data->hdlrs, ecore_event_handler_add(E_EVENT_BORDER_REMOVE,                  _e_test_helper_cb_client_remove, NULL));
        th_data->hdlrs = eina_list_append(th_data->hdlrs, ecore_event_handler_add(E_EVENT_BORDER_STACK,                   _e_test_helper_cb_client_restack, NULL));

        _e_test_helper_registrant_clear();
     }

   LOGE("============TEST HELPER End============ \n");

   return 1;
}

/**
 * close dbus connection and release object
 */
EINTERN int
e_test_helper_shutdown(void)
{
   LOGE("============TEST HELPER SHUTDOWN Start============\n");
   if (th_data)
     {
        if (th_data->hdlrs)
          {
             E_FREE_LIST(th_data->hdlrs, ecore_event_handler_del);
          }
        if (th_data->iface)
          {
             eldbus_service_interface_unregister(th_data->iface);
          }
        if (th_data->conn)
          {
             eldbus_connection_unref(th_data->conn);
          }
        eldbus_shutdown();

        E_FREE(th_data);
        th_data = NULL;
     }
   LOGE("============TEST HELPER SHUTDOWN End============\n");
   return 1;
}


