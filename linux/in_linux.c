// in_null.c -- for systems without a mouse

#include "../client/client.h"

cvar_t	*in_mouse;
cvar_t	*in_joystick;

extern qboolean reflib_active;
extern void (*KBD_Close_fp)(void);
extern void (*RW_IN_Shutdown_fp)(void);
extern void (*RW_IN_Commands_fp)(void);
extern void (*RW_IN_Move_fp)(usercmd_t *cmd);
extern void (*RW_IN_Frame_fp)(void);
extern void (*RW_IN_Activate_fp)(qboolean active);

void IN_Init (void)
{
    // there is earlier init rw_init on render loadRefresh
    in_mouse = Cvar_Get ("in_mouse", "1", CVAR_ARCHIVE);
    in_joystick = Cvar_Get ("in_joystick", "0", CVAR_ARCHIVE);
}

void IN_Shutdown (void)
{
    if ( reflib_active )
    {
        if (KBD_Close_fp)
            KBD_Close_fp(); 
        if (RW_IN_Shutdown_fp)
            RW_IN_Shutdown_fp();
        KBD_Close_fp = NULL;
        RW_IN_Shutdown_fp = NULL;
    }
}

void IN_Commands (void)
{
    if (RW_IN_Commands_fp)
        RW_IN_Commands_fp();
}

void IN_Move (usercmd_t *cmd)
{
    if (RW_IN_Move_fp)
        RW_IN_Move_fp(cmd);
}

void IN_Activate (qboolean active)
{
    if (RW_IN_Activate_fp)
        RW_IN_Activate_fp(active);
}

void IN_Frame (void)
{
    if (RW_IN_Frame_fp)
        RW_IN_Frame_fp();
}

void Do_Key_Event(int key, qboolean down)
{
    Key_Event(key, down, Sys_Milliseconds());
}

