#ifdef E_TYPEDEFS

#else
#ifndef E_DATASTORE_H
#define E_DATASTORE_H

#ifndef _F_DISABLE_E_DATASTORE

EAPI void        e_datastore_set(char *key, void *data);
EAPI void       *e_datastore_get(char *key);
EAPI void        e_datastore_del(char *key);

#endif
#endif
#endif
