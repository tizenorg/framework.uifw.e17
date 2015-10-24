#ifndef _E_TZSH_H_
#define _E_TZSH_H_

typedef enum _E_Tzsh_Service_Role
{
   E_TZSH_SERVICE_ROLE_UNKNOWN = 0,
   E_TZSH_SERVICE_ROLE_CALL,
   E_TZSH_SERVICE_ROLE_VOLUME,
   E_TZSH_SERVICE_ROLE_QUICKPANEL,
   E_TZSH_SERVICE_ROLE_LOCKSCREEN,
   E_TZSH_SERVICE_ROLE_INDICATOR,
   E_TZSH_SERVICE_ROLE_RESERVE_FOR_LIMIT
} E_Tzsh_Service_Role;

typedef enum _E_Tzsh_Service_Region_Type
{
   E_TZSH_SERVICE_REGION_TYPE_HANDLER,
   E_TZSH_SERVICE_REGION_TYPE_CONTENT
} E_Tzsh_Service_Region_Type;

typedef enum _E_Tzsh_Message
{
   E_TZSH_MESSAGE_QUICKPANEL_OPEN,
   E_TZSH_MESSAGE_QUICKPANEL_CLOSE,
   E_TZSH_MESSAGE_QUICKPANEL_ENABLE,
   E_TZSH_MESSAGE_QUICKPANEL_DISABLE,
} E_Tzsh_Message;

typedef struct _E_Event_Tzsh_Simple E_Event_Tzsh_Simple;
typedef struct _E_Event_Tzsh_Service_Base E_Event_Tzsh_Service_Base;
typedef struct _E_Event_Tzsh_Service_Base E_Event_Tzsh_Service_Create;
typedef struct _E_Event_Tzsh_Service_Base E_Event_Tzsh_Service_Destroy;
typedef struct _E_Event_Tzsh_Service_Region_Set E_Event_Tzsh_Service_Region_Set;
typedef struct _E_Event_Tzsh_Message E_Event_Tzsh_Service_Message;
typedef struct _E_Event_Tzsh_Message E_Event_Tzsh_Client_Message;

struct _E_Event_Tzsh_Simple
{
   E_Border *bd;
};

struct _E_Event_Tzsh_Service_Base
{
   E_Border *bd;
   E_Tzsh_Service_Role role;
};

struct _E_Event_Tzsh_Service_Region_Set
{
   E_Border *bd;
   unsigned int angle;
   Eina_Tiler *tiler;
   E_Tzsh_Service_Region_Type region_type;
};

struct _E_Event_Tzsh_Message
{
   E_Border *bd;
   E_Tzsh_Service_Role role;
   E_Tzsh_Message msg;
};

extern EAPI int E_EVENT_TZSH_SERVICE_CREATE;
extern EAPI int E_EVENT_TZSH_SERVICE_DESTROY;
extern EAPI int E_EVENT_TZSH_SERVICE_REGION_SET;
extern EAPI int E_EVENT_TZSH_SERVICE_MESSAGE;
extern EAPI int E_EVENT_TZSH_CLIENT_MESSAGE;

EINTERN int          e_tzsh_init(void);
EINTERN int          e_tzsh_shutdown(void);

#endif
