#include "e.h"

/* Atoms */
EAPI Ecore_X_Atom E_ATOM_MANAGED = 0;
EAPI Ecore_X_Atom E_ATOM_CONTAINER = 0;
EAPI Ecore_X_Atom E_ATOM_ZONE = 0;
EAPI Ecore_X_Atom E_ATOM_DESK = 0;
EAPI Ecore_X_Atom E_ATOM_MAPPED = 0;
EAPI Ecore_X_Atom E_ATOM_SHADE_DIRECTION = 0;
EAPI Ecore_X_Atom E_ATOM_HIDDEN = 0;
EAPI Ecore_X_Atom E_ATOM_BORDER_SIZE = 0;
EAPI Ecore_X_Atom E_ATOM_WINDOW_STATE = 0;
EAPI Ecore_X_Atom E_ATOM_WINDOW_STATE_CENTERED = 0;
EAPI Ecore_X_Atom E_ATOM_DESKTOP_FILE = 0;
EAPI Ecore_X_Atom E_ATOM_ZONE_GEOMETRY = 0;
#ifdef _F_USE_TILED_DESK_LAYOUT_
EAPI Ecore_X_Atom E_ATOM_WINDOW_DESKTOP_LAYOUT_SUPPORTED = 0;
EAPI Ecore_X_Atom E_ATOM_WINDOW_DESKTOP_LAYOUT_CHANGE = 0;
EAPI Ecore_X_Atom E_ATOM_WINDOW_DESKTOP_LAYOUT = 0;
EAPI Ecore_X_Atom E_ATOM_DESK_LAYOUT_MAX_TILE = 0;
#endif /* end of _F_USE_TILED_DESK_LAYOUT_ */
#ifdef _F_USE_VIRT_RESOLUTION_
EAPI Ecore_X_Atom E_ATOM_WM_APP_RESOLUTION = 0;
EAPI Ecore_X_Atom E_ATOM_WM_PLANE_MUTE_REQUEST = 0;
EAPI Ecore_X_Atom E_ATOM_WM_SUPPORT_EPOP = 0;
#endif /* end of _F_USE_VIRT_RESOLUTION_ */

/* externally accessible functions */
EINTERN int
e_atoms_init(void)
{
   const char *atom_names[] = {
      "__E_WINDOW_MANAGED",
      "__E_WINDOW_CONTAINER",
      "__E_WINDOW_ZONE",
      "__E_WINDOW_DESK",
      "__E_WINDOW_MAPPED",
      "__E_WINDOW_SHADE_DIRECTION",
      "__E_WINDOW_HIDDEN",
      "__E_WINDOW_BORDER_SIZE",
      "__E_ATOM_WINDOW_STATE",
      "__E_ATOM_WINDOW_STATE_CENTERED",
      "__E_ATOM_DESKTOP_FILE",
      "E_ZONE_GEOMETRY"
#ifdef _F_USE_TILED_DESK_LAYOUT_
      ,
      "_E_ATOM_WINDOW_DESKTOP_LAYOUT_SUPPORTED",
      "_E_ATOM_WINDOW_DESKTOP_LAYOUT_CHANGE",
      "_E_ATOM_WINDOW_DESKTOP_LAYOUT",
      "_E_ATOM_DESK_LAYOUT_MAX_TILE",
#endif /* end of _F_USE_TILED_DESK_LAYOUT_ */
#ifdef _F_USE_VIRT_RESOLUTION_
        "WM_APP_RESOLUTION",
        "WM_PLANE_MUTE_REQUEST",
        "WM_SUPPORT_EPOP"
#endif /* end of _F_USE_VIRT_RESOLUTION_ */
   };

   int index = 12;
#ifdef _F_USE_TILED_DESK_LAYOUT_
   index += 4;
#endif /* end of _F_USE_TILED_DESK_LAYOUT_ */
#ifdef _F_USE_VIRT_RESOLUTION_
   index += 3;
#endif /* end of _F_USE_VIRT_RESOLUTION_ */
   Ecore_X_Atom* atoms = (Ecore_X_Atom*)malloc(sizeof(Ecore_X_Atom) * index);
   if (!atoms) return 0;
   ecore_x_atoms_get(atom_names, index, atoms);
   index = 0;

   E_ATOM_MANAGED = atoms[index++];
   E_ATOM_CONTAINER = atoms[index++];
   E_ATOM_ZONE = atoms[index++];
   E_ATOM_DESK = atoms[index++];
   E_ATOM_MAPPED = atoms[index++];
   E_ATOM_SHADE_DIRECTION = atoms[index++];
   E_ATOM_HIDDEN = atoms[index++];
   E_ATOM_BORDER_SIZE = atoms[index++];
   E_ATOM_WINDOW_STATE = atoms[index++];
   E_ATOM_WINDOW_STATE_CENTERED = atoms[index++];
   E_ATOM_DESKTOP_FILE = atoms[index++];
   E_ATOM_ZONE_GEOMETRY = atoms[index++];
#ifdef _F_USE_TILED_DESK_LAYOUT_
   E_ATOM_WINDOW_DESKTOP_LAYOUT_SUPPORTED = atoms[index++];
   E_ATOM_WINDOW_DESKTOP_LAYOUT_CHANGE = atoms[index++];
   E_ATOM_WINDOW_DESKTOP_LAYOUT = atoms[index++];
   E_ATOM_DESK_LAYOUT_MAX_TILE = atoms[index++];
#endif /* end of _F_USE_TILED_DESK_LAYOUT_ */
#ifdef _F_USE_VIRT_RESOLUTION_
   E_ATOM_WM_APP_RESOLUTION = atoms[index++];
   E_ATOM_WM_PLANE_MUTE_REQUEST = atoms[index++];
   E_ATOM_WM_SUPPORT_EPOP = atoms[index++];
#endif /* end of _F_USE_VIRT_RESOLUTION_ */
   free(atoms);
   return 1;
}

EINTERN int
e_atoms_shutdown(void)
{
   /* Nothing really to do here yet, just present for consistency right now */
   return 1;
}
