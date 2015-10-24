#ifdef E_TYPEDEFS
#else
#ifndef E_ATOMS_H
#define E_ATOMS_H

/* an "overall" atom to see that we recognise the window */
extern EAPI Ecore_X_Atom E_ATOM_MANAGED;

/* basic window properties */
extern EAPI Ecore_X_Atom E_ATOM_CONTAINER;
extern EAPI Ecore_X_Atom E_ATOM_ZONE;
extern EAPI Ecore_X_Atom E_ATOM_DESK;
extern EAPI Ecore_X_Atom E_ATOM_MAPPED;
extern EAPI Ecore_X_Atom E_ATOM_SHADE_DIRECTION;
extern EAPI Ecore_X_Atom E_ATOM_HIDDEN;
extern EAPI Ecore_X_Atom E_ATOM_BORDER_SIZE;
extern EAPI Ecore_X_Atom E_ATOM_DESKTOP_FILE;
/* extra e window states */
/* if we add more states, we need to fix
 * * e_hints_window_e_state_get()
 * * e_hints_window_e_state_set()
 * * _e_win_state_update() + e_win
 */
extern EAPI Ecore_X_Atom E_ATOM_WINDOW_STATE;
extern EAPI Ecore_X_Atom E_ATOM_WINDOW_STATE_CENTERED;

extern EAPI Ecore_X_Atom E_ATOM_ZONE_GEOMETRY;
#ifdef _F_USE_TILED_DESK_LAYOUT_
extern EAPI Ecore_X_Atom E_ATOM_WINDOW_DESKTOP_LAYOUT_SUPPORTED;
extern EAPI Ecore_X_Atom E_ATOM_WINDOW_DESKTOP_LAYOUT_CHANGE;
extern EAPI Ecore_X_Atom E_ATOM_WINDOW_DESKTOP_LAYOUT;
extern EAPI Ecore_X_Atom E_ATOM_DESK_LAYOUT_MAX_TILE;
#endif /* end of _F_USE_TILED_DESK_LAYOUT_ */
#ifdef _F_USE_VIRT_RESOLUTION_
extern EAPI Ecore_X_Atom E_ATOM_WM_APP_RESOLUTION;
extern EAPI Ecore_X_Atom E_ATOM_WM_PLANE_MUTE_REQUEST;
extern EAPI Ecore_X_Atom E_ATOM_WM_SUPPORT_EPOP;
#endif /* end of _F_USE_VIRT_RESOLUTION_ */

EINTERN int    e_atoms_init(void);
EINTERN int    e_atoms_shutdown(void);

#endif
#endif
