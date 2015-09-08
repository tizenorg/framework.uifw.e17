#include "e.h"

#ifndef E_LOG_LEVEL
# define E_LOG_LEVEL EINA_LOG_LEVEL_INFO
#endif

#include "dlog.h"
#undef LOG_TAG
#define LOG_TAG "E17"

EINTERN int e_log_dom = -1;

static const char *_names[] = {
   "CRI",
   "ERR",
   "WRN",
   "INF",
   "DBG",
};

static void
_e_log_cb(const Eina_Log_Domain *d, Eina_Log_Level level, const char *file, const char *fnc __UNUSED__, int line, const char *fmt, void *data __UNUSED__, va_list args)
{
   const char *color;

   color = eina_log_level_color_get(level);
   fprintf(stdout,
           "%s%s<" EINA_COLOR_RESET "%s%s>" EINA_COLOR_RESET "%s:%d" EINA_COLOR_RESET " ",
           color, _names[level > EINA_LOG_LEVEL_DBG ? EINA_LOG_LEVEL_DBG : level],
           d->domain_str, color, file, line);
   vfprintf(stdout, fmt, args);
   putc('\n', stdout);
}

EINTERN int
e_log_init(void)
{
   e_log_dom = eina_log_domain_register("e", EINA_COLOR_WHITE);
   eina_log_domain_level_set("e", E_LOG_LEVEL);
   eina_log_print_cb_set(_e_log_cb, NULL);
   return (e_log_dom != -1);
}

EINTERN int
e_log_shutdown(void)
{
   eina_log_domain_unregister(e_log_dom);
   e_log_dom = -1;
   return 0;
}

#ifdef _F_E_LOGBUF_
EAPI unsigned int e_logbuf_type = ELBT_NONE;

#define MAX_LOGS 9216
#define MAX_LOG_FUNC_STR_SIZE 20
#define MAX_LOG_STR_SIZE 80

typedef struct _E_Log
{
   unsigned int      num;
   unsigned int      type;
   unsigned int      blank;
   char              func[MAX_LOG_FUNC_STR_SIZE];
   int               line;
   unsigned int      id; // Ecore_X_Window or Zone ID
   char              str[MAX_LOG_STR_SIZE];
   double            t;
} E_Log;

typedef struct _E_Logbuf
{
   E_DBus_Interface    *iface;
   int                  cur;
   int                  num;
   Eina_Bool            to_log_watcher;
   FILE                *fp;
   Eina_List           *list; // list of E_Log
   Ecore_Event_Handler *h;
   Ecore_X_Atom         a;
   E_DBus_Connection   *conn;
} E_Logbuf;

static E_Logbuf *buf = NULL;

static void _log_disp(FILE *fp, E_Log *m, double sdt, double dt);
static void _log_type_disp(void);

EAPI void
e_logbuf_add(unsigned int type,
             const char  *func,
             int          line,
             const char  *str,
             unsigned int id)
{
   E_Log *msg = NULL;
   if (!buf) return;

   if (eina_list_count(buf->list) < MAX_LOGS)
     {
        msg = E_NEW(E_Log, 1);
        if (msg)
          buf->list = eina_list_append(buf->list, msg);
     }
   else
     {
        buf->cur = (buf->cur) % MAX_LOGS;
        msg = eina_list_nth(buf->list, buf->cur);
     }

   if (msg)
     {
        unsigned int n = 0;
        msg->num = buf->num++;
        msg->type = type;
        msg->blank = 0;

        n = sizeof(msg->func);
        if (n >= MAX_LOG_FUNC_STR_SIZE)
          {
             strncpy(msg->func, func, MAX_LOG_FUNC_STR_SIZE-1);
             msg->func[MAX_LOG_FUNC_STR_SIZE-1] = '\0';
          }
        else
          {
             strncpy(msg->func, func, n);
          }

        msg->line = line;
        msg->id = id;
        msg->t = ecore_time_get();

        n = sizeof(msg->str);
        if (n >= MAX_LOG_STR_SIZE)
          {
             strncpy(msg->str, str, MAX_LOG_STR_SIZE-1);
             msg->str[MAX_LOG_STR_SIZE-1] = '\0';
          }
        else
          {
             strncpy(msg->str, str, n);
          }

        buf->cur = (buf->num) % MAX_LOGS;

        if (buf->to_log_watcher)
          _log_disp(NULL, msg, 0.0, 0.0);
     }
}

EAPI void
e_logbuf_fmt_add(unsigned int type,
                 unsigned int blank,
                 const char  *func,
                 int          line,
                 unsigned int id,
                 const char  *fmt,
                 ...)
{
   E_Log *msg = NULL;
   va_list args;
   if (!buf) return;

   if (eina_list_count(buf->list) < MAX_LOGS)
     {
        msg = E_NEW(E_Log, 1);
        if (msg)
          buf->list = eina_list_append(buf->list, msg);
     }
   else
     {
        buf->cur = (buf->cur) % MAX_LOGS;
        msg = eina_list_nth(buf->list, buf->cur);
     }

   if (msg)
     {
        unsigned int n = 0;
        msg->num = buf->num++;
        msg->type = type;
        msg->blank = blank;

        n = sizeof(msg->func);
        if (n >= MAX_LOG_FUNC_STR_SIZE)
          {
             strncpy(msg->func, func, MAX_LOG_FUNC_STR_SIZE-1);
             msg->func[MAX_LOG_FUNC_STR_SIZE-1] = '\0';
          }
        else
          {
             strncpy(msg->func, func, n);
          }

        msg->line = line;
        msg->id = id;

        if (!blank) msg->t = ecore_time_get();
        else msg->t = 0.0f;

        va_start(args, fmt);
        vsnprintf(msg->str, sizeof(msg->str), fmt, args);
        va_end(args);

        buf->cur = (buf->num) % MAX_LOGS;

        if (buf->to_log_watcher)
          _log_disp(NULL, msg, 0.0, 0.0);
     }
}

static void
_e_logbuf_shutdown(void)
{
   e_logbuf_type = ELBT_NONE;

   if (buf)
     {
        E_Log *msg = NULL;
        EINA_LIST_FREE(buf->list, msg) E_FREE(msg);
        buf->list = NULL;
        if (buf->iface)
          {
             e_msgbus_interface_detach(buf->iface);
             e_dbus_interface_unref(buf->iface);
             buf->iface = NULL;
          }
        if (buf->h)
          ecore_event_handler_del(buf->h);
        memset(buf, 0, sizeof(E_Logbuf));
        E_FREE(buf);
        buf = NULL;
     }
}

static const char *
_type_name_get(unsigned int t)
{
   switch (t)
     {
      case ELBT_NONE:        return "NONE";   break;
      case ELBT_DFT:         return "DFT";    break;
      case ELBT_MNG:         return "MNG";    break;
      case ELBT_BD:          return "BD";     break;
      case ELBT_ROT:         return "ROT";    break;
      case ELBT_ILLUME:      return "ILLUME"; break;
      case ELBT_COMP:        return "COMP";   break;
      case ELBT_MOVE:        return "MOVE";   break;
      case ELBT_TRACE:       return "TRACE";  break;
      case ELBT_DESK_LY:     return "LY";     break;
      case ELBT_COMP_RENDER: return "RENDER"; break;
      case ELBT_ALL:         return "ALL";    break;
      default: break;
     }
   return NULL;
}

static void
_log_type_disp(void)
{
   unsigned int i;
   char str[1024] = "LOG Type: ";

   if (e_logbuf_type == ELBT_NONE)
     {
        strcat(str, _type_name_get(e_logbuf_type));
     }
   else if (e_logbuf_type == ELBT_ALL)
     {
        strcat(str, _type_name_get(e_logbuf_type));
     }
   else
     {
        for (i = 1; i < ELBT_MAX; i *= 2)
          {
             if ((e_logbuf_type) & (i))
               {
                  if (_type_name_get(i))
                    {
                       strcat(str, ", ");
                       strcat(str, _type_name_get(i));
                    }
               }
          }
     }

   ELBF(ELBT_DFT, 0, 0, "%s", str);
}

static void
_log_disp(FILE   *fp,
          E_Log  *m,
          double sdt,
          double dt)
{
   char tmp[256];

   if (m->blank)
     {
        if (m->id)
          snprintf(tmp, sizeof(tmp),
                   "%38.38s|0x%08x|%17s|%s",
                   "", m->id, "", m->str);
        else
          snprintf(tmp, sizeof(tmp),
                   "%67.67s|%s",
                   "", m->str);
     }
   else
     {
        if (m->id)
          snprintf(tmp, sizeof(tmp),
                   "%5d|%20.20s|%5d|%5.5s|0x%08x|%8.3f|%8.3f|%s",
                   m->num, m->func, m->line,
                   _type_name_get(m->type), m->id, sdt, dt, m->str);
        else
          snprintf(tmp, sizeof(tmp),
                   "%5d|%20.20s|%5d|%5.5s|%10.10s|%8.3f|%8.3f|%s",
                   m->num, m->func, m->line,
                   _type_name_get(m->type), "", sdt, dt, m->str);
     }

   if (fp)
     {
        fprintf(fp, "%s\n", tmp);
     }
   else if (buf->to_log_watcher)
     {
        if (buf->conn)
          {
             DBusMessage *msg;
             char *tmp2 = strdup(tmp); // TODO
             msg = dbus_message_new_method_call("org.enlightenment.elogwatcher.service",
                                                "/org/enlightenment/elogwatcher/RemoteObject",
                                                "org.enlightenment.elogwatcher.Log",
                                                "Show");
             dbus_message_append_args(msg,
                                      DBUS_TYPE_STRING, &tmp2,
                                      DBUS_TYPE_INVALID);
             free(tmp2);
             e_dbus_message_send(buf->conn, msg, NULL, 0, NULL);
             dbus_message_unref(msg);
          }
     }
}

static void
_e_log_dump(FILE *fp)
{
   Eina_List *l = NULL, *ll = NULL;
   E_Log *msg = NULL;
   unsigned int idx = 0;
   double dt = 0.0, pt = 0.0;
   double st = 0.0, sdt = 0.0;
   Eina_Bool st_set = EINA_FALSE;
   if ((!buf) || (!fp)) return;

   ll = eina_list_nth_list(buf->list, buf->cur);
   EINA_LIST_FOREACH(ll, l, msg)
     {
        if (!msg->blank)
          {
             if (!st_set)
               {
                  st = msg->t;
                  dt = msg->t - pt;
                  pt = msg->t;
                  st_set = EINA_TRUE;
               }
             else
               {
                  sdt = msg->t - st;
                  dt = msg->t - pt;
                  pt = msg->t;
               }
          }
        _log_disp(fp, msg, sdt, dt);
        idx = msg->num;
     }
   EINA_LIST_FOREACH(buf->list, l, msg)
     {
        if (msg->num < idx) break;
        if (!msg->blank)
          {
             sdt = msg->t - st;
             dt = msg->t - pt;
             pt = msg->t;
          }
        _log_disp(fp, msg, sdt, dt);
        idx = msg->num;
     }
   struct timeval tv;
   gettimeofday(&tv, NULL);
   struct tm* ptm;
   char timeBuf[32];

   ptm = localtime(&(tv.tv_sec));
   strftime(timeBuf, sizeof(timeBuf), "%m-%d %H:%M:%S", ptm);

   fprintf(fp, "Total:%d Num:%d(Printed time : %s.%03ld(%.3f))\n", eina_list_count(buf->list), buf->num, timeBuf, tv.tv_usec / 1000, ecore_time_get());
   fflush(fp);
}

static DBusMessage *
_e_log_cb_enable(E_DBus_Object *obj __UNUSED__,
                 DBusMessage       *msg)
{
   e_logbuf_type = ELBT_ALL;
   return dbus_message_new_method_return(msg);
}

static DBusMessage *
_e_log_cb_disable(E_DBus_Object *obj __UNUSED__,
                  DBusMessage       *msg)
{
   e_logbuf_type = ELBT_NONE;
   return dbus_message_new_method_return(msg);
}

static DBusMessage *
_e_log_cb_dump(E_DBus_Object *obj __UNUSED__,
               DBusMessage       *msg)
{
   DBusError err;
   const char *path = NULL;
   FILE *fp = NULL;

   dbus_error_init(&err);
   if (!dbus_message_get_args(msg, &err,
                              DBUS_TYPE_STRING, &path,
                              DBUS_TYPE_INVALID))
     {
        dbus_error_free(&err);
     }
   else if (path)
     {
        if (buf)
          {
             if (strstr(path, "watcher"))
               {
                  buf->to_log_watcher = EINA_TRUE;
               }
             else
               {
                  fp = fopen(path, "w");
                  if (fp)
                    {
                       _e_log_dump(fp);
                       fclose(fp);
                    }
               }
          }
     }

   return dbus_message_new_method_return(msg);
}

static DBusMessage *
_e_log_cb_clear(E_DBus_Object *obj __UNUSED__,
                DBusMessage       *msg)
{
   if (buf)
     {
        E_Log *_log = NULL;
        EINA_LIST_FREE(buf->list, _log) E_FREE(_log);
        buf->list = NULL;
        buf->num = 0;
     }
   return dbus_message_new_method_return(msg);
}

static DBusMessage *
_e_log_cb_type_set(E_DBus_Object *obj __UNUSED__,
                   DBusMessage       *msg)
{
   DBusError err;
   unsigned int val = 0x0;

   dbus_error_init(&err);
   if (!dbus_message_get_args(msg, &err,
                              DBUS_TYPE_INT32, &val,
                              DBUS_TYPE_INVALID))
     {
        dbus_error_free(&err);
     }
   else
     {
        e_logbuf_type = val;
        _log_type_disp();
     }
   return dbus_message_new_method_return(msg);
}

static Eina_Bool
_e_log_cb_prop(void *data __UNUSED__,
               int type   __UNUSED__,
               void      *event)
{
   Ecore_X_Event_Window_Property *ev = event;
   char *path = NULL;
   FILE *fp = NULL;

   if (!ev) return ECORE_CALLBACK_PASS_ON;
   if (!ev->atom) return ECORE_CALLBACK_PASS_ON;
   if (!buf) return ECORE_CALLBACK_PASS_ON;
   if (ev->atom != buf->a) return ECORE_CALLBACK_PASS_ON;

   path = ecore_x_window_prop_string_get(ev->win, ev->atom);
   if (!path) return ECORE_CALLBACK_PASS_ON;

   if (!strncmp(path, "--clear", 7))
     {
        if (buf)
          {
             E_Log *msg = NULL;
             EINA_LIST_FREE(buf->list, msg) E_FREE(msg);
             buf->list = NULL;
             buf->num = 0;
          }

        fprintf(stdout, "clear e17 log\n");
        free(path);
        return ECORE_CALLBACK_PASS_ON;
     }

   fp = fopen(path, "w");
   if (!fp)
     {
        free(path);
        return ECORE_CALLBACK_PASS_ON;
     }

   _e_log_dump(fp);

   fprintf(stdout,
           "dump log messages to file:%s\n",
           path);

   fclose(fp);
   free(path);
   return ECORE_CALLBACK_PASS_ON;
}

EINTERN int
e_logbuf_init(void)
{
   _e_logbuf_shutdown();

   buf = E_NEW(E_Logbuf, 1);
   if (!buf) return 0;

   buf->iface = e_dbus_interface_new("org.enlightenment.wm.Log");
   if (!buf->iface)
     {
        fprintf(stderr, "Cannot add org.enlightenment.wm.Log interface\n");
        memset(buf, 0, sizeof(E_Logbuf));
        E_FREE(buf);
        return 0;
     }

   e_dbus_interface_method_add(buf->iface, "Enable", "", "",   _e_log_cb_enable);
   e_dbus_interface_method_add(buf->iface, "Disable", "", "",  _e_log_cb_disable);
   e_dbus_interface_method_add(buf->iface, "Dump", "s", "",    _e_log_cb_dump);
   e_dbus_interface_method_add(buf->iface, "Clear", "", "",    _e_log_cb_clear);
   e_dbus_interface_method_add(buf->iface, "SetType", "i", "", _e_log_cb_type_set);

   e_msgbus_interface_attach(buf->iface);

   buf->h = ecore_event_handler_add(ECORE_X_EVENT_WINDOW_PROPERTY, _e_log_cb_prop, NULL);
   buf->a = ecore_x_atom_get("_E_LOG");

   buf->conn = e_dbus_bus_get(DBUS_BUS_SESSION);

   e_logbuf_type = (ELBT_ALL) & (~ELBT_COMP_RENDER);

   return 1;
}

EINTERN int
e_logbuf_shutdown(void)
{
   _e_logbuf_shutdown();
   return 1;
}
#endif
