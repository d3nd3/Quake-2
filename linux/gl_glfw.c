/*
** GLW_IMP.C
**
** This file contains ALL Linux specific stuff having to do with the
** OpenGL refresh.  When a port is being made the following functions
** must be implemented by the port:
**
** GLimp_EndFrame
** GLimp_Init
** GLimp_Shutdown
** GLimp_SwitchFullscreen
**
*/

#include <termios.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/vt.h>
#include <stdarg.h>
#include <stdio.h>
#include <signal.h>

#include "../ref_gl/gl_local.h"
#include "../client/keys.h"
#include "../linux/rw_linux.h"

#include <GLFW/glfw3.h>

// un-necessary, will remove later
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/keysym.h>
#include <X11/extensions/XShm.h>
#include <Xm/MwmUtil.h>

Key_Event_fp_t Key_Event_fp;

int min_keycodes, max_keycodes;

#include <GL/glx.h>

unsigned char XLateKey(int keycode, qboolean state);
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);

GLXContext              gl_cx;

static qboolean         doShm;
static Display          *x_disp;
static Colormap         x_cmap;
Window           x_win;
static GC               x_gc;
static Visual           *x_vis;
static XVisualInfo      *x_visinfo;

static int              StudlyRGBattributes[] =
{
    GLX_DOUBLEBUFFER,
    GLX_RGBA,
    GLX_RED_SIZE, 4,
    GLX_GREEN_SIZE, 4,
    GLX_BLUE_SIZE, 4,
    GLX_DEPTH_SIZE, 1,
    GLX_SAMPLES_SGIS, 4, /* for better AA */
    None,
};

static int              RGBattributes[] =
{
    GLX_DOUBLEBUFFER,
    GLX_RGBA,
    GLX_RED_SIZE, 4,
    GLX_GREEN_SIZE, 4,
    GLX_BLUE_SIZE, 4,
    GLX_DEPTH_SIZE, 1,
    None,
};

#define STD_EVENT_MASK (StructureNotifyMask | KeyPressMask \
         | KeyReleaseMask | ExposureMask | PointerMotionMask | \
         ButtonPressMask | ButtonReleaseMask)

int current_framebuffer;
static int              x_shmeventtype;
//static XShmSegmentInfo    x_shminfo;

static qboolean         oktodraw = false;
static qboolean         X11_active = false;

struct
{
    int key;
    int down;
} keyq[64];
int keyq_head=0;
int keyq_tail=0;

static int      mx, my;
static int p_mouse_x, p_mouse_y;
static cvar_t   *_windowed_mouse;

static cvar_t *sensitivity;
static cvar_t *lookstrafe;
static cvar_t *m_side;
static cvar_t *m_yaw;
static cvar_t *m_pitch;
static cvar_t *m_forward;
static cvar_t *freelook;

int config_notify=0;
int config_notify_width;
int config_notify_height;
                              
typedef unsigned short PIXEL;

// Console variables that we need to access from this module

/*****************************************************************************/
/* MOUSE                                                                     */
/*****************************************************************************/

// this is inside the renderer shared lib, so these are called from vid_so

static qboolean        mouse_avail;
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



/*****************************************************************************/

static qboolean GLimp_SwitchFullscreen( int width, int height );
qboolean GLimp_InitGL (void);

GLFWwindow* window;
extern cvar_t *vid_fullscreen;
extern cvar_t *vid_ref;



#define NUM_RESOLUTIONS 3

#define GR_RESOLUTION_MIN   GR_RESOLUTION_320x200
#define GR_RESOLUTION_320x200   0x0
#define GR_RESOLUTION_320x240   0x1
#define GR_RESOLUTION_400x256   0x2
#define GR_RESOLUTION_512x384   0x3
#define GR_RESOLUTION_640x200   0x4
#define GR_RESOLUTION_640x350   0x5
#define GR_RESOLUTION_640x400   0x6
#define GR_RESOLUTION_640x480   0x7
#define GR_RESOLUTION_800x600   0x8
#define GR_RESOLUTION_960x720   0x9
#define GR_RESOLUTION_856x480   0xa
#define GR_RESOLUTION_512x256   0xb
#define GR_RESOLUTION_MAX   GR_RESOLUTION_512x256
#define GR_RESOLUTION_NONE      0xff


static resolutions[NUM_RESOLUTIONS][3]={ 
  { 512, 384, GR_RESOLUTION_512x384 },
  { 640, 400, GR_RESOLUTION_640x400 },
  { 640, 480, GR_RESOLUTION_640x480 }
};

static int findres(int *width, int *height)
{
    int i;

    for(i=0;i<NUM_RESOLUTIONS;i++)
        if((*width<=resolutions[i][0]) && (*height<=resolutions[i][1])) {
            *width = resolutions[i][0];
            *height = resolutions[i][1];
            return resolutions[i][2];
        }
        
    *width = 640;
    *height = 480;
    return GR_RESOLUTION_640x480;
}

static void signal_handler(int sig)
{
    printf("Received signal %d, exiting...\n", sig);
    GLimp_Shutdown();
    _exit(0);
}

static void InitSig(void)
{
    signal(SIGHUP, signal_handler);
    signal(SIGQUIT, signal_handler);
    signal(SIGILL, signal_handler);
    signal(SIGTRAP, signal_handler);
    signal(SIGIOT, signal_handler);
    signal(SIGBUS, signal_handler);
    signal(SIGFPE, signal_handler);
    // signal 7 = segfault
    signal(SIGSEGV, signal_handler);
    signal(SIGTERM, signal_handler);
}
void debug_callback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *message, const void *userParam)
{
    Com_Printf("GLFW3:OpenGL Error: %s\n",message);
}

Window getLastCreatedWindow(Display *display, Window rootWindow) {
    Window parent, *children;
    unsigned int nChildren;
    Status status = XQueryTree(display, rootWindow, &rootWindow, &parent, &children, &nChildren);

    if (status && nChildren > 0) {
        Window lastChild = children[nChildren - 1];
        XFree(children);
        return getLastCreatedWindow(display, lastChild);
    } else {
        return rootWindow;
    }
}


void getVideoModes(int setMode, int *width, int *height) {
    GLFWmonitor* primary = glfwGetPrimaryMonitor();
    if (!primary) {
        fprintf(stderr, "Failed to get primary monitor\n");
        glfwTerminate();
        exit(EXIT_FAILURE);
    }

    const GLFWvidmode* mode = glfwGetVideoMode(primary);
    if (!mode) {
        fprintf(stderr, "Failed to get video mode\n");
        glfwTerminate();
        exit(EXIT_FAILURE);
    }

    Com_Printf("Primary monitor resolution: %dx%d\n", mode->width, mode->height);
    if ( setMode == 0 ) {
        *width=mode->width;
        *height=mode->height;
        return;
    }
    int count;
    const GLFWvidmode* modes = glfwGetVideoModes(primary, &count);
    if (!modes) {
       fprintf(stderr, "Failed to get video modes\n");
       glfwTerminate();
       exit(EXIT_FAILURE);
    }

    printf("Supported resolutions:\n");
    for (int i = 0; i < count; i++) {
       Com_Printf("- %dx%d\n", modes[i].width, modes[i].height);
    }
    *width=modes[setMode].width;
    *height=modes[setMode].height;
}

/*
Called by R_Init
    GL_SetDefaultState in gl_rmisc, initializes GL stuff.
*/
/*
** GLimp_SetMode
*/
int GLimp_SetMode( int *pwidth, int *pheight, int mode, qboolean fullscreen )
{
    int width, height;

    ri.Con_Printf( PRINT_ALL, "Initializing OpenGL display\n");

    // destroy the existing window
    GLimp_Shutdown ();

    // create a window and OpenGL context using GLFW library
    glfwInit();

    getVideoModes(mode, &width,&height);

    // glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    // glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
    // glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    if (fullscreen) {
        window = glfwCreateWindow(width, height, "My Game", NULL, NULL);
    } else {
        window = glfwCreateWindow(640, 480, "My Game", NULL, NULL);
    }

    if (!window) {
        glfwTerminate();
        return rserr_invalid_mode;
    }
    
    // keyboard stuff
    x_disp = XOpenDisplay(NULL);
    Window root;
    root = XRootWindow(x_disp, DefaultScreen(x_disp));
    x_win = getLastCreatedWindow(x_disp,root);
    XDisplayKeycodes(x_disp,&min_keycodes,&max_keycodes);

     /* Set the key callback, calls keys.c KeyEvent */
    glfwSetKeyCallback(window, key_callback);

    glfwMakeContextCurrent(window);
    Com_Printf("GLFW: GL_VERSION IS : %s\n",glGetString(GL_VERSION));
    glViewport(0, 0, width, height);

    // VERY USEFUL DEBUGGING MESSAGES.
    /*glEnable(GL_DEBUG_OUTPUT);
    glDebugMessageCallback(debug_callback, NULL);
    */
    
    // glfwSwapInterval(0);
    // glEnable(GL_CULL_FACE);
    // glEnable(GL_DEPTH_TEST);


    // glClearColor(0.0, 0.0f, 0.0f, 1.0f);


    /*
    GLuint vao = 0;
     glCreateVertexArrays( 1, &vao );
     glBindVertexArray( vao );

     float positions[6] = {
        -0.5f, -0.5f,
         0.0f,  0.5f,
         0.5f, -0.5f
     };
     unsigned int buffer;
     glGenBuffers(1, &buffer);
     glBindBuffer(GL_ARRAY_BUFFER, buffer);
     glBufferData(GL_ARRAY_BUFFER, 6 * sizeof(float), positions, GL_STATIC_DRAW);
     glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), 0);
     glEnableVertexAttribArray(0);
    */

    
    // let the sound and input subsystems know about the new window
    ri.Vid_NewWindow (width, height);

    x_disp = XOpenDisplay(NULL);
    if (!x_disp) {
        Com_Printf("GLFW ERROR: Couldn't Open X Display\n");
        return rserr_unknown;
    }

    *pwidth = width;
    *pheight = height;

    return rserr_ok;
}



/*
** GLimp_Shutdown
**
** This routine does all OS specific shutdown procedures for the OpenGL
** subsystem.  Under OpenGL this means NULLing out the current DC and
** HGLRC, deleting the rendering context, and releasing the DC acquired
** for the window.  The state structure is also nulled out.
**
*/
void GLimp_Shutdown( void )
{
    // Com_Printf("GLimp_Shutdown: +\n");

    if ( x_disp ) {
        XCloseDisplay(x_disp);
        x_disp = NULL;
    }

    //glfwDestroyWindow(window);
    glfwTerminate();
    window = NULL;
}

/*
** GLimp_Init
**
** This routine is responsible for initializing the OS specific portions
** of OpenGL.  
*/
int GLimp_Init( void *hinstance, void *wndproc )
{
    InitSig();

    return true;
}

/*
** GLimp_BeginFrame
*/
void GLimp_BeginFrame( float camera_seperation )
{
    // Com_Printf("GLimp_BeginFrame: +\n");
    // also called by SCR_UpdateScreen
}

/*
** GLimp_EndFrame
** 
** Responsible for doing a swapbuffers and possibly for other stuff
** as yet to be determined.  Probably better not to make this a GLimp
** function and instead do a call to GLimp_SwapBuffers.
*/
void GLimp_EndFrame(void)
{
    glFlush();
    glfwSwapBuffers(window);

    // called by SCR_UpdateScreen
    // Com_Printf("GL_imp_EndFrame: Swapping buffers\n");
}

/*
** GLimp_AppActivate
*/
void GLimp_AppActivate( qboolean active )
{
    Com_Printf("GLimp_AppActivate: +\n");
}

// extern void gl3DfxSetPaletteEXT(GLuint *pal);

void Fake_glColorTableEXT( GLenum target, GLenum internalformat,
                             GLsizei width, GLenum format, GLenum type,
                             const GLvoid *table )
{
    byte temptable[256][4];
    byte *intbl;
    int i;

    for (intbl = (byte *)table, i = 0; i < 256; i++) {
        temptable[i][2] = *intbl++;
        temptable[i][1] = *intbl++;
        temptable[i][0] = *intbl++;
        temptable[i][3] = 255;
    }
    // gl3DfxSetPaletteEXT((GLuint *)temptable);
}

/*****************************************************************************/

/*
vid_so.c calls KBD_Init which in turn sets Key_Event_fp
It uses dlsym to get the symbol from the reflibrary(us)

VID_LoadRefresh calls KBD_Init_fp, with argument 
void Do_Key_Event(int key, qboolean down)
{
    Key_Event(key, down, Sys_Milliseconds());
}

So when we call Key_Event_fp ,. its internally calling Key_Event defined in client/keys.c
*/
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    // if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
    //     glfwSetWindowShouldClose(window, GLFW_TRUE);
    // if (action == GLFW_PRESS)
    //     printf("Key %d pressed.\n", key);
    // else if (action == GLFW_RELEASE)
    //     printf("Key %d released.\n", key);

    qboolean key_state = 0;
    unsigned char  key_char = 0;

    if ( action == GLFW_RELEASE ) 
        key_state = 0;
    else if ( action == GLFW_PRESS || action == GLFW_REPEAT )
        key_state = 1;

    // returns 1 char, 0 if unsupported
    key_char = XLateKey(scancode, key_state);
    
    if ( key_char > 0 ) {
        // Com_Printf("key_char = %c, state = %i\n",key_char,key_state);
        // GLFW3 will get keycode and pressed state
        Key_Event_fp( key_char , key_state);
    }
}

/*
client/keys.c KeyEvent
*/
unsigned char XLateKey(int keycode, qboolean state)
{
    // KeyCode keycode = XKeysymToKeycode(display, XK_space);
    int keysyms_per_keycode_return;
    if ( keycode < min_keycodes || !x_disp )
        return NULL;
    KeySym *keysym = XGetKeyboardMapping(x_disp, keycode, 1, &keysyms_per_keycode_return);
    if (!keysym) {
        Com_Printf("WARNING: Failed to get keyboard mapping\n");
        return '0';
    }
    unsigned char buf[64];
    XKeyEvent ev = { 0 };
    ev.display = x_disp;
    ev.window = x_win;
    ev.state = state;
    ev.keycode = keycode;
    XLookupString(&ev, buf, sizeof buf, keysym, 0);

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

/*****************************************************************************/

/*****************************************************************************/
/* KEYBOARD                                                                  */
/*****************************************************************************/



void KBD_Init(Key_Event_fp_t fp)
{
    _windowed_mouse = ri.Cvar_Get ("_windowed_mouse", "0", CVAR_ARCHIVE);
    Key_Event_fp = fp;
}

// called by SYS_SendKeyEvents in sys_linux.so
// vid_so.c sets KBD_Update_fp to this. Not called in dedicated mode strange.

// key.key = keycode , key.down = pressed 1/0
void KBD_Update(void)
{

    // poll keyboard etc.
    glfwPollEvents();

}

void KBD_Close(void)
{
}


static void Force_CenterView_f (void)
{
    in_state->viewangles[PITCH] = 0;
}

static void RW_IN_MLookDown (void) 
{ 
    mlooking = true; 
}

static void RW_IN_MLookUp (void) 
{
    mlooking = false;
    in_state->IN_CenterView_fp ();
}

/*
client/cl_main.c @CL_Frame
linux/vid_so.c
    @VID_CheckChanges
    @VID_LoadRefresh
        @Real_IN_Init
ref_gl/gl_rmain.c 
    @R_Init
    @R_SetMode
linux/gl_glfw.c
    @GLimp_SetMode

*/
void RW_IN_Init(in_state_t *in_state_p)
{
    int mtype;
    int i;

    fprintf(stderr, "GL RW_IN_Init\n");

    in_state = in_state_p;

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

    Com_Printf("RW_IN_INIT: Adding Commands ??\n");
    ri.Cmd_AddCommand ("+mlook", RW_IN_MLookDown);
    ri.Cmd_AddCommand ("-mlook", RW_IN_MLookUp);

    ri.Cmd_AddCommand ("force_centerview", Force_CenterView_f);

    mouse_x = mouse_y = 0.0;
    mouse_avail = true;
}

void RW_IN_Shutdown(void)
{
    mouse_avail = false;

    ri.Cmd_RemoveCommand ("force_centerview");
    ri.Cmd_RemoveCommand ("+mlook");
    ri.Cmd_RemoveCommand ("-mlook");
}

/*
===========
IN_Commands
===========
*/
void RW_IN_Commands (void)
{
    int i;
   
    if (!mouse_avail) 
        return;
   
    for (i=0 ; i<3 ; i++) {
        if ( (mouse_buttonstate & (1<<i)) && !(mouse_oldbuttonstate & (1<<i)) )
            in_state->Key_Event_fp (K_MOUSE1 + i, true);

        if ( !(mouse_buttonstate & (1<<i)) && (mouse_oldbuttonstate & (1<<i)) )
            in_state->Key_Event_fp (K_MOUSE1 + i, false);
    }
    mouse_oldbuttonstate = mouse_buttonstate;
}

/*
===========
IN_Move
===========
*/
void RW_IN_Move (usercmd_t *cmd)
{
    if (!mouse_avail)
        return;
   
    if (m_filter->value)
    {
        mouse_x = (mx + old_mouse_x) * 0.5;
        mouse_y = (my + old_mouse_y) * 0.5;
    } else {
        mouse_x = mx;
        mouse_y = my;
    }

    old_mouse_x = mx;
    old_mouse_y = my;

    if (!mouse_x && !mouse_y)
        return;

    mouse_x *= sensitivity->value;
    mouse_y *= sensitivity->value;

// add mouse X/Y movement to cmd
    if ( (*in_state->in_strafe_state & 1) || 
        (lookstrafe->value && mlooking ))
        cmd->sidemove += m_side->value * mouse_x;
    else
        in_state->viewangles[YAW] -= m_yaw->value * mouse_x;

    if ( (mlooking || freelook->value) && 
        !(*in_state->in_strafe_state & 1))
    {
        in_state->viewangles[PITCH] += m_pitch->value * mouse_y;
    }
    else
    {
        cmd->forwardmove -= m_forward->value * mouse_y;
    }
    mx = my = 0;
}

void RW_IN_Frame (void)
{
}

void RW_IN_Activate(void)
{
}