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

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/keysym.h>
#include <X11/extensions/XShm.h>
#include <Xm/MwmUtil.h>

Key_Event_fp_t Key_Event_fp;

int min_keycodes, max_keycodes;

#include <GL/glx.h>

GLFWwindow * GLFWwin = NULL;


/*****************************************************************************/

static qboolean GLimp_SwitchFullscreen( int width, int height );
qboolean GLimp_InitGL (void);


extern cvar_t *vid_fullscreen;
extern cvar_t *vid_ref;


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



/*
-------------------------------------------------
WE ARE NOT CREATING NEW WINDOW ON MODE CHANGE
SO NO NEED TO REINIT LOTS OF THINGS.. ZZ
-------------------------------------------------
Called by gl_rmain.c @ R_Init
    GL_SetDefaultState in gl_rmisc, initializes GL stuff.

    typedef enum
    {
        rserr_ok,

        rserr_invalid_fullscreen,
        rserr_invalid_mode,

        rserr_unknown
    } rserr_t;

** GLimp_SetMode
pwidth and pheight are the new width and height set by this mode change.
*/
int GLimp_SetMode( int *exportWidth, int *exportHeight, int mode, qboolean fullscreen )
{ 
    int requestedWidth, requestedHeight;

    GLFWwin = glfwGetCurrentContext();
    if ( GLFWwin == NULL ) {
        Sys_Error(ERR_FATAL,"Cant' get window\n");
    }

    ri.Con_Printf( PRINT_ALL, "Initializing OpenGL display\n");
    ri.Con_Printf (PRINT_ALL, "...setting mode : %d\n", mode );

    if ( !ri.Vid_GetModeInfo( &requestedWidth, &requestedHeight, mode ) )
    {
        ri.Con_Printf( PRINT_ALL, " invalid mode\n" );
        return rserr_invalid_mode;
    }

    Com_Printf("Requesting Resolution : %i x %i\n",requestedWidth,requestedHeight);
    *exportWidth = requestedWidth;
    *exportHeight = requestedHeight;
   
    // destroy the existing window
    GLimp_Shutdown ();

    // Try fullscreen and/or resize
    glfwSetWindowMonitor( GLFWwin,
        fullscreen ? glfwGetPrimaryMonitor() : NULL,
        0,0,
        requestedWidth,
        requestedHeight,
        GLFW_DONT_CARE);

    // Resize failed?
    int nowWidth,nowHeight;
    qboolean resize_fail = false;
    glfwGetWindowSize(GLFWwin,&nowWidth,&nowHeight);
    /*if ( requestedWidth != nowWidth || requestedHeight != nowHeight )
    {
        ri.Con_Printf( PRINT_ALL, "invalid mode!\n");
        // short-circuit bad mode !
        return rserr_invalid_mode;
    }*/

    // mode has to be good here

    // Fullscreen failed?
    if ( fullscreen && glfwGetWindowMonitor(GLFWwin) == NULL ) {
        ri.Con_Printf( PRINT_ALL, " failed\n" );
        ri.Con_Printf( PRINT_ALL, "...setting windowed mode\n" );

        gl_state.fullscreen = false;
        return rserr_invalid_fullscreen;
    }

    // let the sound and input subsystems know about the new window size
    ri.Vid_NewWindow (requestedWidth, requestedHeight);

    // fullscreen good , mode good

    Com_Printf("GLFW: GL_VERSION IS : %s\n",glGetString(GL_VERSION));
    // glViewport(0, 0, requestedWidth, requestedHeight);

    // VERY USEFUL DEBUGGING MESSAGES.
    /*glEnable(GL_DEBUG_OUTPUT);
    glDebugMessageCallback(debug_callback, NULL);
    */
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

    // this means that other systems cannot call glfwGetCurrentContext to get the current window.
    // glfwMakeContextCurrent(NULL);


    // if ( x_disp ) {
    //     XCloseDisplay(x_disp);
    //     x_disp = NULL;
    // }
}

/*
** GLimp_Init
**
** This routine is responsible for initializing the OS specific portions
** of OpenGL.  
*/
int GLimp_Init( void *hinstance, void *wndproc )
{
    // InitSig();

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
    glfwSwapBuffers(GLFWwin);

    // called by SCR_UpdateScreen
    // Com_Printf("GL_imp_EndFrame: Swapping buffers\n");
}

/*
 GLimp_AppActivate
 Exported by ref_gl.so as re.AppActivate.
 Not called anywhere yet, will call it here.
 Called also where in_linux.c 's AppActivate is.
 Its supposed to be the renderer's code for what to do on focus change.
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


#if 0
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
#endif