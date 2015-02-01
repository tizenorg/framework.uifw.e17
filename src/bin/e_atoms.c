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
#endif /* end of _F_USE_TILED_DESK_LAYOUT_ */

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
        "_E_ATOM_WINDOW_DESKTOP_LAYOUT"
#endif /* end of _F_USE_TILED_DESK_LAYOUT_ */
   };
#ifdef _F_USE_TILED_DESK_LAYOUT_
   Ecore_X_Atom atoms[15];
   ecore_x_atoms_get(atom_names, 15, atoms);
#else
   Ecore_X_Atom atoms[12];
   ecore_x_atoms_get(atom_names, 12, atoms);
#endif /* end of _F_USE_TILED_DESK_LAYOUT_ */

   E_ATOM_MANAGED = atoms[0];
   E_ATOM_CONTAINER = atoms[1];
   E_ATOM_ZONE = atoms[2];
   E_ATOM_DESK = atoms[3];
   E_ATOM_MAPPED = atoms[4];
   E_ATOM_SHADE_DIRECTION = atoms[5];
   E_ATOM_HIDDEN = atoms[6];
   E_ATOM_BORDER_SIZE = atoms[7];
   E_ATOM_WINDOW_STATE = atoms[8];
   E_ATOM_WINDOW_STATE_CENTERED = atoms[9];
   E_ATOM_DESKTOP_FILE = atoms[10];
   E_ATOM_ZONE_GEOMETRY = atoms[11];
#ifdef _F_USE_TILED_DESK_LAYOUT_
   E_ATOM_WINDOW_DESKTOP_LAYOUT_SUPPORTED = atoms[12];
   E_ATOM_WINDOW_DESKTOP_LAYOUT_CHANGE = atoms[13];
   E_ATOM_WINDOW_DESKTOP_LAYOUT = atoms[14];
#endif /* end of _F_USE_TILED_DESK_LAYOUT_ */
   return 1;
}

EINTERN int
e_atoms_shutdown(void)
{
   /* Nothing really to do here yet, just present for consistency right now */
   return 1;
}
