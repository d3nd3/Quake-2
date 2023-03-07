// in_null.c -- for systems without a mouse

#include "../client/client.h"
#include "../linux/rw_linux.h"


#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/keysym.h>
#include <X11/extensions/XShm.h>
#include <Xm/MwmUtil.h>

extern Key_Event_fp_t Key_Event_fp;

int min_keycodes, max_keycodes;

#include <GL/glx.h>

unsigned char XLateKey(int keycode, qboolean state);

static Display* x_disp = NULL;
static Window x_win = NULL;

cvar_t	*in_mouse;
cvar_t	*in_joystick;

extern qboolean reflib_active;
extern void (*KBD_Close_fp)(void);
extern void (*RW_IN_Shutdown_fp)(void);
extern void (*RW_IN_Commands_fp)(void);
extern void (*RW_IN_Move_fp)(usercmd_t *cmd);
extern void (*RW_IN_Frame_fp)(void);
extern void (*RW_IN_Activate_fp)(qboolean active);

Window getLastCreatedWindow(Display *display, Window rootWindow );

static qboolean        mouse_avail;
/*
static int     mouse_buttonstate;
static int     mouse_oldbuttonstate;
static int   mouse_x, mouse_y;
static int  old_mouse_x, old_mouse_y;
static float old_windowed_mouse;
static int p_mouse_x, p_mouse_y;

static cvar_t   *_windowed_mouse;
static cvar_t   *m_filter;
static cvar_t   *in_mouse;

static qboolean mlooking;

// state struct passed in Init
static in_state_t   *in_state;

*/

/*
General init function for input.
*/
void IN_Init (void)
{
    int mtype;
    int i;

    // there is earlier init rw_init on render loadRefresh
    in_mouse = Cvar_Get ("in_mouse", "1", CVAR_ARCHIVE);
    in_joystick = Cvar_Get ("in_joystick", "0", CVAR_ARCHIVE);

/*
    // mouse variables
    _windowed_mouse = ri.Cvar_Get ("_windowed_mouse", "0", CVAR_ARCHIVE);
    m_filter = ri.Cvar_Get ("m_filter", "0", 0);
    in_mouse = ri.Cvar_Get ("in_mouse", "1", CVAR_ARCHIVE);
    freelook = ri.Cvar_Get( "freelook", "0", 0 );
    lookstrafe = ri.Cvar_Get ("lookstrafe", "0", 0);
    sensitivity = ri.Cvar_Get ("sensitivity", "3", 0);
    m_pitch = ri.Cvar_Get ("m_pitch", "0.022", 0);
    m_yaw = ri.Cvar_Get ("m_yaw", "0.022", 0);
    m_forward = ri.Cvar_Get ("m_forward", "1", 0);
    m_side = ri.Cvar_Get ("m_side", "0.8", 0);
*/

    // Cmd_AddCommand ("+mlook", RW_IN_MLookDown);
    // Cmd_AddCommand ("-mlook", RW_IN_MLookUp);
    // Cmd_AddCommand ("force_centerview", Force_CenterView_f);

    // mouse_x = mouse_y = 0.0;
    mouse_avail = true;

    if ( x_win == NULL ) {
        x_disp = XOpenDisplay(NULL);
        Window root = XRootWindow(x_disp, DefaultScreen(x_disp));
        x_win = getLastCreatedWindow(x_disp,root);
        if ( x_win == NULL ) 
            Sys_Error(ERR_FATAL, "Can't get the X window\n");
        XDisplayKeycodes(x_disp,&min_keycodes,&max_keycodes);
    }
}

/*
    Client shutdown
*/
void IN_Shutdown (void)
{
    mouse_avail = false;
    if ( reflib_active )
    {
        if (KBD_Close_fp)
            KBD_Close_fp(); 
        if (RW_IN_Shutdown_fp)
            RW_IN_Shutdown_fp();
        KBD_Close_fp = NULL;
        RW_IN_Shutdown_fp = NULL;
    }
    XCloseDisplay(x_disp);
}
/*
called by:
cl_main.c
    @CL_SendCommand
    @CL_Frame

This one is before cbuf_execute.
Its meant to adds commands to the console buffer based on keyPresses.
*/
void IN_Commands (void)
{
    if (RW_IN_Commands_fp)
        RW_IN_Commands_fp();
}
/*
called by:
cl_input.c
    @CL_CreateCmd
    @CL_SendCmd
cl_main.c
    @CL_SendCommand

Read mouse position.
*/
void IN_Move (usercmd_t *cmd)
{
    if (RW_IN_Move_fp)
        RW_IN_Move_fp(cmd);
}

/*
The window/renderer is meant to call this when window gains or loses focus.
*/
void IN_Activate (qboolean active)
{
    if (RW_IN_Activate_fp)
        RW_IN_Activate_fp(active);
}

/*
start of CL_Frame.
*/
void IN_Frame (void)
{
    if (RW_IN_Frame_fp)
        RW_IN_Frame_fp();
}

/*
vid_so.c calls KBD_Init which in turn sets Key_Event_fp
It uses dlsym to get the symbol from the reflibrary(us)

VID_LoadRefresh calls KBD_Init_fp, with argument Do_Key_Event()
Do_Key_Event() lives in  in_linux.c

So when we call Key_Event_fp ,. its internally calling Key_Event defined in client/keys.c
*/
void IN_OnKeyPress( int key, int scancode, int key_state, int mods)
{
    unsigned char  key_char = 0;
    // returns 1 char, 0 if unsupported
    key_char = XLateKey(scancode, key_state);
    
    if ( key_char > 0 ) {
        // Com_Printf("key_char = %hhd, state = %i\n",key_char,key_state);
        // GLFW3 will get keycode and pressed state
        Key_Event(key_char, key_state, Sys_Milliseconds());
    }
}

/*
client/keys.c KeyEvent
*/
unsigned char XLateKey(int keycode, qboolean state)
{
    // Com_Printf("1\n");
    // KeyCode keycode = XKeysymToKeycode(display, XK_space);
    int keysyms_per_keycode_return;
    if ( keycode < min_keycodes || !x_disp )
        return NULL;
    //  Com_Printf("2\n");
    KeySym *keysym = XGetKeyboardMapping(x_disp, keycode, 1, &keysyms_per_keycode_return);
    if (!keysym) {
        Com_Printf("WARNING: Failed to get keyboard mapping\n");
        return '0';
    }
    // Com_Printf("3\n");
    unsigned char buf[64];
    XKeyEvent ev = { 0 };
    ev.display = x_disp;
    ev.window = x_win;
    ev.state = state;
    ev.keycode = keycode;
    XLookupString(&ev, buf, sizeof buf, keysym, 0);
    // Com_Printf("4\n");
    unsigned char * key_str = buf;
    /*
    unsigned char * key_str = XKeysymToString(*keysym);
    // not all keys can be represented as strings.
    if ( !key_str ) {
        Com_Printf("WARNING: XLatekey; no str for keycode %i,keysym %i\n",keycode,*keysym);
    }*/

    unsigned char retKey = 0;
    // HANDLE non-string versions?
    switch(*keysym)
    {
        case XK_KP_Page_Up:  retKey = K_KP_PGUP; break;
        case XK_Page_Up:     retKey = K_PGUP; break;

        case XK_KP_Page_Down: retKey = K_KP_PGDN; break;
        case XK_Page_Down:   retKey = K_PGDN; break;

        case XK_KP_Home: retKey = K_KP_HOME; break;
        case XK_Home:    retKey = K_HOME; break;

        case XK_KP_End:  retKey = K_KP_END; break;
        case XK_End:     retKey = K_END; break;

        case XK_KP_Left: retKey = K_KP_LEFTARROW; break;
        case XK_Left:    retKey = K_LEFTARROW; break;

        case XK_KP_Right: retKey = K_KP_RIGHTARROW; break;
        case XK_Right:  retKey = K_RIGHTARROW;     break;

        case XK_KP_Down: retKey = K_KP_DOWNARROW; break;
        case XK_Down:    retKey = K_DOWNARROW; break;

        case XK_KP_Up:   retKey = K_KP_UPARROW; break;
        case XK_Up:      retKey = K_UPARROW;    break;

        case XK_Escape: retKey = K_ESCAPE;     break;

        case XK_KP_Enter: retKey = K_KP_ENTER; break;
        case XK_Return: retKey = K_ENTER;       break;

        case XK_Tab:        retKey = K_TAB;             break;

        case XK_F1:      retKey = K_F1;                break;

        case XK_F2:      retKey = K_F2;                break;

        case XK_F3:      retKey = K_F3;                break;

        case XK_F4:      retKey = K_F4;                break;

        case XK_F5:      retKey = K_F5;                break;

        case XK_F6:      retKey = K_F6;                break;

        case XK_F7:      retKey = K_F7;                break;

        case XK_F8:      retKey = K_F8;                break;

        case XK_F9:      retKey = K_F9;                break;

        case XK_F10:        retKey = K_F10;             break;

        case XK_F11:        retKey = K_F11;             break;

        case XK_F12:        retKey = K_F12;             break;

        case XK_BackSpace: retKey = K_BACKSPACE; break;

        case XK_KP_Delete: retKey = K_KP_DEL; break;
        case XK_Delete: retKey = K_DEL; break;

        case XK_Pause:  retKey = K_PAUSE;       break;

        case XK_Shift_L:
        case XK_Shift_R:    retKey = K_SHIFT;      break;

        case XK_Execute: 
        case XK_Control_L: 
        case XK_Control_R:  retKey = K_CTRL;        break;

        case XK_Alt_L:  
        case XK_Meta_L: 
        case XK_Alt_R:  
        case XK_Meta_R: retKey = K_ALT;            break;

        case XK_KP_Begin: retKey = K_KP_5; break;

        case XK_Insert:retKey = K_INS; break;
        case XK_KP_Insert: retKey = K_KP_INS; break;

        case XK_KP_Multiply: retKey = '*'; break;
        case XK_KP_Add:  retKey = K_KP_PLUS; break;
        case XK_KP_Subtract: retKey = K_KP_MINUS; break;
        case XK_KP_Divide: retKey = K_KP_SLASH; break;

#if 1
        case 0x021: retKey = '1';break;/* [!] */
        case 0x040: retKey = '2';break;/* [@] */
        case 0x023: retKey = '3';break;/* [#] */
        case 0x024: retKey = '4';break;/* [$] */
        case 0x025: retKey = '5';break;/* [%] */
        case 0x05e: retKey = '6';break;/* [^] */
        case 0x026: retKey = '7';break;/* [&] */
        case 0x02a: retKey = '8';break;/* [*] */
        case 0x028: retKey = '9';;break;/* [(] */
        case 0x029: retKey = '0';break;/* [)] */
        case 0x05f: retKey = '-';break;/* [_] */
        case 0x02b: retKey = '=';break;/* [+] */
        case 0x07c: retKey = '\'';break;/* [|] */
        case 0x07d: retKey = '[';break;/* [}] */
        case 0x07b: retKey = ']';break;/* [{] */
        case 0x022: retKey = '\'';break;/* ["] */
        case 0x03a: retKey = ';';break;/* [:] */
        case 0x03f: retKey = '/';break;/* [?] */
        case 0x03e: retKey = '.';break;/* [>] */
        case 0x03c: retKey = ',';break;/* [<] */
        case 0x7e: retKey = '`'; break;/* [~] */
#endif
        // Its likely a string?
        default:
            if ( key_str ) {
                // caps? , makes it lower case equality.
                // retKey equal to first letter of string. lol 'apostrophe' = 'a'
                // strange that it does that. I think the return from XLookupString is different
                retKey = key_str[0];
                if (retKey >= 'A' && retKey <= 'Z')
                    retKey = retKey - 'A' + 'a';
                // no caps
            } else {
                // retKey is unset (default 0 here)
            }
            break;
    } 
    // returns a char, 0 represents unsupported
    return retKey;
}


#if 0
void GetEvent(void)
{
    XEvent x_event;
    int b;
   
    XNextEvent(x_disp, &x_event);
    switch(x_event.type) {
    case KeyPress:
        // keyq[keyq_head].key = XLateKey(&x_event.xkey);
        keyq[keyq_head].down = true;
        keyq_head = (keyq_head + 1) & 63;
        break;
    case KeyRelease:
        // keyq[keyq_head].key = XLateKey(&x_event.xkey);
        keyq[keyq_head].down = false;
        keyq_head = (keyq_head + 1) & 63;
        break;

    case MotionNotify:
        if (_windowed_mouse->value) {
            mx += ((int)x_event.xmotion.x - (int)(vid.width/2));
            my += ((int)x_event.xmotion.y - (int)(vid.height/2));

            /* move the mouse to the window center again */
            XSelectInput(x_disp,x_win, STD_EVENT_MASK & ~PointerMotionMask);
            XWarpPointer(x_disp,None,x_win,0,0,0,0, 
                (vid.width/2),(vid.height/2));
            XSelectInput(x_disp,x_win, STD_EVENT_MASK);
        } else {
            mx = ((int)x_event.xmotion.x - (int)p_mouse_x);
            my = ((int)x_event.xmotion.y - (int)p_mouse_y);
            p_mouse_x=x_event.xmotion.x;
            p_mouse_y=x_event.xmotion.y;
        }
        break;

    case ButtonPress:
        b=-1;
        if (x_event.xbutton.button == 1)
            b = 0;
        else if (x_event.xbutton.button == 2)
            b = 2;
        else if (x_event.xbutton.button == 3)
            b = 1;
        if (b>=0)
            mouse_buttonstate |= 1<<b;
        break;

    case ButtonRelease:
        b=-1;
        if (x_event.xbutton.button == 1)
            b = 0;
        else if (x_event.xbutton.button == 2)
            b = 2;
        else if (x_event.xbutton.button == 3)
            b = 1;
        if (b>=0)
            mouse_buttonstate &= ~(1<<b);
        break;
    
    case ConfigureNotify:
        config_notify_width = x_event.xconfigure.width;
        config_notify_height = x_event.xconfigure.height;
        config_notify = 1;
        break;

    default:
        if (doShm && x_event.type == x_shmeventtype)
            oktodraw = true;
    }
   
    if (old_windowed_mouse != _windowed_mouse->value) {
        old_windowed_mouse = _windowed_mouse->value;

        if (!_windowed_mouse->value) {
            /* ungrab the pointer */
            XUngrabPointer(x_disp,CurrentTime);
        } else {
            /* grab the pointer */
            XGrabPointer(x_disp,x_win,True,0,GrabModeAsync,
                GrabModeAsync,x_win,None,CurrentTime);
        }
    }
}
#endif
Window getLastCreatedWindow(Display *display, Window rootWindow ) {
    Window parent, *children;
    unsigned int nChildren;
    Status status = XQueryTree(display, rootWindow, &rootWindow, &parent, &children, &nChildren);

    if (status && nChildren > 0) {
        Window lastChild = children[nChildren - 1];
        XTextProperty windowName;
        XGetWMName(display, lastChild, &windowName);

        if (windowName.value && strncmp((char *) windowName.value, "Quake 2 -glfwLinux", windowName.nitems) == 0) {
            XFree(windowName.value);
            XFree(children);
            return lastChild;
        }

        XFree(windowName.value);
        XFree(children);
        return getLastCreatedWindow(display, lastChild);
    } else {
        return rootWindow;
    }
}
