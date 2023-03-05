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
    signal(SIGSEGV, signal_handler);
    signal(SIGTERM, signal_handler);
}
void debug_callback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *message, const void *userParam)
{
    Com_Printf("GLFW3:OpenGL Error: %s\n",message);
}

glfw_onFramebufferSize(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}

/*
R_Init
    GL_SetDefaultState
*/


/*
** GLimp_SetMode
*/
int GLimp_SetMode( int *pwidth, int *pheight, int mode, qboolean fullscreen )
{
    int width, height;
    Com_Printf("GL_imp_SetMode: +\n");
    fullscreen = false;
    ri.Con_Printf( PRINT_ALL, "Initializing OpenGL display\n");

    ri.Con_Printf (PRINT_ALL, "...setting mode %d:", mode );

    if ( !ri.Vid_GetModeInfo( &width, &height, mode ) )
    {
        ri.Con_Printf( PRINT_ALL, " invalid mode\n" );
        return rserr_invalid_mode;
    }

    ri.Con_Printf( PRINT_ALL, " %d %d\n", width, height );

    // destroy the existing window
    GLimp_Shutdown ();

    // create a window and OpenGL context using GLFW library
    glfwInit();
    // glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    if (fullscreen) {
        GLFWmonitor* monitor = glfwGetPrimaryMonitor();
        // const GLFWvidmode* vidmode = glfwGetVideoMode(monitor);
        window = glfwCreateWindow(width, height, "My Game", NULL, NULL);
    } else {
        window = glfwCreateWindow(640, 480, "My Game", NULL, NULL);
    }

    if (!window) {
        glfwTerminate();
        return rserr_invalid_mode;
    }

    glfwMakeContextCurrent(window);

    glEnable(GL_DEBUG_OUTPUT);
    glDebugMessageCallback(debug_callback, NULL);
    
    glfwSwapInterval(0);
    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);


    // glfwSetFramebufferSizeCallback(window, glfw_onFramebufferSize);

    // glClearColor(0.0, 0.0f, 0.0f, 1.0f);
    // set viewport
    glViewport(0, 0, width, height);

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
    Com_Printf("GLimp_Shutdown: +\n");
    glfwDestroyWindow(window);
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