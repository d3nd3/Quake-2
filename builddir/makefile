
# # Disable built-in implicit rules
# MAKEFLAGS += --no-builtin-rules
# # Disable built-in suffix rules
# .SUFFIXES:

# -Wall
CFLAGS = \
	-D __linux__ \
	-c \
	-g \
	-fPIC \
	-w \
	-Dstricmp=strcasecmp
# 	-O
# 	-ffast-math -funroll-loops \
# 	-fomit-frame-pointer -fexpensive-optimizations -falign-loops=2 \
# 	-falign-jumps=2 -falign-functions=2
LDFLAGS = \
-lGL \
-lglfw  \
-lm \
-ldl \
-lXext -lX11
ODIR = next

EXEBASE = quake2
EXE = ../quake2.exe
REFGL = ../ref_gl.so
GAME = ../baseq2/gamex86_64.so

.DEFAULT_GOAL = install

# -------------------SERVER+CLIENT----------------

SERVERFILES = \
$(ODIR)/sv_ccmds.o \
$(ODIR)/sv_ents.o \
$(ODIR)/sv_game.o \
$(ODIR)/sv_init.o \
$(ODIR)/sv_main.o \
$(ODIR)/sv_send.o \
$(ODIR)/sv_user.o \
$(ODIR)/sv_world.o


# old fx code choose between cl_newfx and cl_fx
# seems to want both. must be an extension, not an option.
CLIENTFILES = \
$(ODIR)/cl_cin.o \
$(ODIR)/cl_ents.o \
$(ODIR)/cl_input.o \
$(ODIR)/cl_inv.o \
$(ODIR)/cl_main.o \
$(ODIR)/cl_fx.o \
$(ODIR)/cl_newfx.o \
$(ODIR)/cl_parse.o \
$(ODIR)/cl_pred.o \
$(ODIR)/cl_scrn.o \
$(ODIR)/cl_tent.o \
$(ODIR)/cl_view.o \
$(ODIR)/console.o \
$(ODIR)/keys.o \
$(ODIR)/menu.o \
$(ODIR)/qmenu.o \
$(ODIR)/snd_dma.o \
$(ODIR)/snd_mem.o \
$(ODIR)/snd_mix.o \
$(ODIR)/m_flash.o


# commonfiles are used by both client and server
COMMONFILES = \
$(ODIR)/cmd.o \
$(ODIR)/cmodel.o \
$(ODIR)/common.o \
$(ODIR)/crc.o \
$(ODIR)/cvar.o \
$(ODIR)/files.o \
$(ODIR)/md4.o \
$(ODIR)/net_chan.o \
$(ODIR)/pmove.o

# ------------ OS SPECIFIC --------------
EXE_LINUX_FILES = \
../linux/cd_linux.o \
../linux/q_shlinux.o \
../linux/q_shlinux2.o \
../linux/vid_menu.o \
../linux/vid_so.o \
../linux/snd_linux.o \
../linux/sys_linux.o \
../linux/glob.o \
../linux/net_udp.o \
../linux/in_linux.o



EXE_LINUX_OBJS = $(addprefix $(ODIR)/, $(notdir $(EXE_LINUX_FILES)))


# -------------------REFSOFT----------------
REFSOFT_FILES = \
	$(ODIR)/r_aclip.o \
	$(ODIR)/r_alias.o \
	$(ODIR)/r_bsp.o \
	$(ODIR)/r_draw.o \
	$(ODIR)/r_edge.o \
	$(ODIR)/r_image.o \
	$(ODIR)/r_light.o \
	$(ODIR)/r_main.o \
	$(ODIR)/r_misc.o \
	$(ODIR)/r_model.o \
	$(ODIR)/r_part.o \
	$(ODIR)/r_poly.o \
	$(ODIR)/r_polyse.o \
	$(ODIR)/r_rast.o \
	$(ODIR)/r_scan.o \
	$(ODIR)/r_sprite.o \
	$(ODIR)/r_surf.o

REFSOFT_FILES_MORE = \
	../ref_soft/r_aclipa.o \
	../ref_soft/r_draw16.o \
	../ref_soft/r_drawa.o \
	../ref_soft/r_edgea.o \
	../ref_soft/r_scana.o \
	../ref_soft/r_spr8.o \
	../ref_soft/r_surf8.o \
	../ref_soft/math.o \
	../ref_soft/d_polysa.o \
	../ref_soft/r_varsa.o \
	../ref_soft/sys_dosa.o

REFSOFT_OBJS_MORE = $(addprefix $(ODIR)/, $(notdir $(REFSOFT_FILES_MORE)))

REFSOFT_LINUX_FILES = \
	../ref_soft/rw_svgalib.o \
	../ref_soft/d_copy.o \
	../ref_soft/rw_in_svgalib.o

# 	../ref_soft/rw_x11.o

# possibly glob'
# possibly sh_linux

REFSOFT_LINUX_OBJS := $(addprefix $(ODIR)/, $(notdir $(REFSOFT_LINUX_FILES)))

REFSOFT_ALL := $(REFSOFT_LINUX_OBJS) $(REFSOFT_OBJS_MORE)


# sharedfiles are included in EVERY dll
SHAREDFILES = $(ODIR)/q_shared.o

# BELOW USE AUTO GENERATED TARGETS

# ----------GAME LIBRARY-----------
GAME_FILES = \
../game/g_ai.o \
../game/g_cmds.o \
../game/g_chase.o \
../game/g_combat.o \
../game/g_func.o \
../game/g_items.o \
../game/g_main.o \
../game/g_misc.o \
../game/g_monster.o \
../game/g_phys.o \
../game/g_save.o \
../game/g_spawn.o \
../game/g_svcmds.o \
../game/g_target.o \
../game/g_trigger.o \
../game/g_turret.o \
../game/g_utils.o \
../game/g_weapon.o \
../game/m_actor.o \
../game/m_berserk.o \
../game/m_boss2.o \
../game/m_boss3.o \
../game/m_boss31.o \
../game/m_boss32.o \
../game/m_brain.o \
../game/m_chick.o \
../game/m_flash.o \
../game/m_flipper.o \
../game/m_float.o \
../game/m_flyer.o \
../game/m_gladiator.o \
../game/m_gunner.o \
../game/m_hover.o \
../game/m_infantry.o \
../game/m_insane.o \
../game/m_medic.o \
../game/m_move.o \
../game/m_mutant.o \
../game/m_parasite.o \
../game/m_soldier.o \
../game/m_supertank.o \
../game/m_tank.o \
../game/p_client.o \
../game/p_hud.o \
../game/p_trail.o \
../game/p_view.o \
../game/p_weapon.o


GAME_OBJS = $(addprefix $(ODIR)/, $(notdir $(GAME_FILES)))

#------------CTF GAME-------------
GAME_CTF_FILES = \
../ctf/g_ai.o \
../ctf/g_chase.o \
../ctf/g_cmds.o \
../ctf/g_combat.o \
../ctf/g_ctf.o \
../ctf/g_func.o \
../ctf/g_items.o \
../ctf/g_main.o \
../ctf/g_misc.o \
../ctf/g_monster.o \
../ctf/g_phys.o \
../ctf/g_save.o \
../ctf/g_spawn.o \
../ctf/g_svcmds.o \
../ctf/g_target.o \
../ctf/g_trigger.o \
../ctf/g_utils.o \
../ctf/g_weapon.o \
../ctf/m_move.o \
../ctf/p_client.o \
../ctf/p_hud.o \
../ctf/p_menu.o \
../ctf/p_trail.o \
../ctf/p_view.o \
../ctf/p_weapon.o \
../ctf/q_shared.o

GAME_CTF_OBJS = $(addprefix $(ODIR)/, $(notdir $(GAME_CTF_FILES)))

# ----------REF_GL LIBRARY-----------
REFGL_FILES = \
../ref_gl/gl_draw.o \
../ref_gl/gl_image.o \
../ref_gl/gl_light.o \
../ref_gl/gl_mesh.o \
../ref_gl/gl_model.o \
../ref_gl/gl_rmain.o \
../ref_gl/gl_rmisc.o \
../ref_gl/gl_rsurf.o \
../ref_gl/gl_warp.o


REFGL_OBJS = $(addprefix $(ODIR)/, $(notdir $(REFGL_FILES)))

# ----------REFGL OS SPECIFIC-----------

REFGL_WIN_FILES = \
../win32/glw_imp.o \
../win32/q_shwin.o \
../win32/qgl_win.o

REFGL_LINUX_FILES = \
../linux/gl_glfw.o \
../linux/qgl_linux.o \
../linux/q_shlinux.o \
../linux/q_shlinux2.o \
../linux/glob.o

# These are user input , one is required, perhaps not. Just wants keyboard input routines.
# irix/glw_imp.c has good example
#../linux/rw_x11.o \
#../linux/rw_svgalib.c
# possibly q_shlinux
# possibly glob

# $(addprefix ) - applies to EACH in the names/s
REFGL_LINUX_OBJS = $(addprefix $(ODIR)/, $(notdir $(REFGL_LINUX_FILES)))
REFGL_WIN_OBJS = $(addprefix $(ODIR)/, $(notdir $(REFGL_WIN_FILES)))

REFGL_ALL_OBJS := $(REFGL_LINUX_OBJS) $(REFGL_OBJS)
REFGL_ALL_FILES := $(REFGL_LINUX_FILES) $(REFGL_FILES)


#-------------------------------------------------------------------


# cc $(CFLAGS) $$^ -o $$@
# strip dir, give new dir, replace .o with .c
define multi_line_var

$(ODIR)/$(notdir $(1)): $(1:%.o=%.c)
	cc $(CFLAGS) -o $$@ $$?
endef

# Create targets automatically using foreach
# Game Library
$(eval $(foreach an_object_file,$(GAME_FILES),$(call multi_line_var, $(an_object_file))))
# RefGL Library
$(eval $(foreach an_object_file,$(REFGL_ALL_FILES),$(call multi_line_var, $(an_object_file))))
# exe-specific Linux files
$(eval $(foreach an_object_file,$(EXE_LINUX_FILES),$(call multi_line_var, $(an_object_file))))
# RefSoft Library
$(eval $(foreach an_object_file,$(REFSOFT_ALL),$(call multi_line_var, $(an_object_file))))


# linux special
$(ODIR)/snd_mixa.o: ../linux/snd_mixa.s
	cc $(CFLAGS) -DELF -x assembler-with-cpp -o $@ $?

#----------------------------------------------------------------------

$(ODIR)/q_shared.o : ../game/q_shared.c
	cc $(CFLAGS) -o $@ $?

#----------------------------------------------------------------------

$(ODIR)/sv_ccmds.o : ../server/sv_ccmds.c
	cc $(CFLAGS) -o $@ $?

$(ODIR)/sv_ents.o : ../server/sv_ents.c
	cc $(CFLAGS) -o $@ $?

$(ODIR)/sv_game.o : ../server/sv_game.c
	cc $(CFLAGS) -o $@ $?

$(ODIR)/sv_init.o : ../server/sv_init.c
	cc $(CFLAGS) -o $@ $?

$(ODIR)/sv_main.o : ../server/sv_main.c
	cc $(CFLAGS) -o $@ $?

$(ODIR)/sv_send.o : ../server/sv_send.c
	cc $(CFLAGS) -o $@ $?

$(ODIR)/sv_user.o : ../server/sv_user.c
	cc $(CFLAGS) -o $@ $?

$(ODIR)/sv_world.o : ../server/sv_world.c
	cc $(CFLAGS) -o $@ $?

#----------------------------------------------------------------------

$(ODIR)/cl_newfx.o : ../client/cl_newfx.c
	cc $(CFLAGS) -o $@ $?
$(ODIR)/cl_demo.o : ../client/cl_demo.c
	cc $(CFLAGS) -o $@ $?

$(ODIR)/cl_ents.o : ../client/cl_ents.c
	cc $(CFLAGS) -o $@ $?

$(ODIR)/cl_fx.o : ../client/cl_fx.c
	cc $(CFLAGS) -o $@ $?

$(ODIR)/cl_input.o : ../client/cl_input.c
	cc $(CFLAGS) -o $@ $?

$(ODIR)/cl_inv.o : ../client/cl_inv.c
	cc $(CFLAGS) -o $@ $?

$(ODIR)/cl_main.o : ../client/cl_main.c
	cc $(CFLAGS) -o $@ $?

$(ODIR)/cl_parse.o : ../client/cl_parse.c
	cc $(CFLAGS) -o $@ $?

$(ODIR)/cl_pred.o : ../client/cl_pred.c
	cc $(CFLAGS) -o $@ $?	

$(ODIR)/cl_tent.o : ../client/cl_tent.c
	cc $(CFLAGS) -o $@ $?

$(ODIR)/console.o : ../client/console.c
	cc $(CFLAGS) -o $@ $?

$(ODIR)/keys.o : ../client/keys.c
	cc $(CFLAGS) -o $@ $?

$(ODIR)/menu.o : ../client/menu.c
	cc $(CFLAGS) -o $@ $?

$(ODIR)/qmenu.o : ../client/qmenu.c
	cc $(CFLAGS) -o $@ $?

$(ODIR)/sbar2.o : ../client/sbar2.c
	cc $(CFLAGS) -o $@ $?

$(ODIR)/cl_scrn.o : ../client/cl_scrn.c
	cc $(CFLAGS) -o $@ $?

$(ODIR)/cl_cin.o : ../client/cl_cin.c
	cc $(CFLAGS) -o $@ $?

$(ODIR)/snd_dma.o : ../client/snd_dma.c
	cc $(CFLAGS) -o $@ $?

$(ODIR)/snd_mem.o : ../client/snd_mem.c
	cc $(CFLAGS) -o $@ $?

$(ODIR)/snd_mix.o : ../client/snd_mix.c
	cc $(CFLAGS) -o $@ $?

$(ODIR)/cl_view.o : ../client/cl_view.c
	cc $(CFLAGS) -o $@ $?
#----------------------------------------------------------------------

$(ODIR)/cmd.o : ../qcommon/cmd.c
	cc $(CFLAGS) -o $@ $?

$(ODIR)/cmodel.o : ../qcommon/cmodel.c
	cc $(CFLAGS) -o $@ $?

$(ODIR)/cvar.o : ../qcommon/cvar.c
	cc $(CFLAGS) -o $@ $?

$(ODIR)/files.o : ../qcommon/files.c
	cc $(CFLAGS) -o $@ $?

$(ODIR)/md4.o : ../qcommon/md4.c
	cc $(CFLAGS) -o $@ $?

$(ODIR)/net_chan.o : ../qcommon/net_chan.c
	cc $(CFLAGS) -o $@ $?


$(ODIR)/common.o : ../qcommon/common.c
	cc $(CFLAGS) -o $@ $?

$(ODIR)/crc.o : ../qcommon/crc.c
	cc $(CFLAGS) -o $@ $?

$(ODIR)/pmove.o : ../qcommon/pmove.c
	cc $(CFLAGS) -o $@ $?

#----------------------------------------------------------------------

$(ODIR)/r_aclip.o : ../ref_soft/r_aclip.c
	cc $(CFLAGS) -o $@ $?

$(ODIR)/r_alias.o : ../ref_soft/r_alias.c
	cc $(CFLAGS) -o $@ $?

$(ODIR)/r_bsp.o : ../ref_soft/r_bsp.c
	cc $(CFLAGS) -o $@ $?

$(ODIR)/r_draw.o : ../ref_soft/r_draw.c
	cc $(CFLAGS) -o $@ $?

$(ODIR)/r_edge.o : ../ref_soft/r_edge.c
	cc $(CFLAGS) -o $@ $?

$(ODIR)/r_image.o : ../ref_soft/r_image.c
	cc $(CFLAGS) -o $@ $?

$(ODIR)/r_light.o : ../ref_soft/r_light.c
	cc $(CFLAGS) -o $@ $?

$(ODIR)/r_main.o : ../ref_soft/r_main.c
	cc $(CFLAGS) -o $@ $?

$(ODIR)/r_misc.o : ../ref_soft/r_misc.c
	cc $(CFLAGS) -o $@ $?

$(ODIR)/r_model.o : ../ref_soft/r_model.c
	cc $(CFLAGS) -o $@ $?

$(ODIR)/r_part.o : ../ref_soft/r_part.c
	cc $(CFLAGS) -o $@ $?

$(ODIR)/r_polyse.o : ../ref_soft/r_polyse.c
	cc $(CFLAGS) -o $@ $?

$(ODIR)/r_poly.o : ../ref_soft/r_poly.c
	cc $(CFLAGS) -o $@ $?

$(ODIR)/r_rast.o : ../ref_soft/r_rast.c
	cc $(CFLAGS) -o $@ $?

$(ODIR)/r_scan.o : ../ref_soft/r_scan.c
	cc $(CFLAGS) -o $@ $?

$(ODIR)/r_sprite.o : ../ref_soft/r_sprite.c
	cc $(CFLAGS) -o $@ $?

$(ODIR)/r_surf.o : ../ref_soft/r_surf.c
	cc $(CFLAGS) -o $@ $?
#----------------------------------------------------------------------

.PHONY: clean run debug rundev
clean:
	rm -f next/*
run:
	cd .. && ./quake2.exe +set console 1 +set vid_ref "gl"
rundev:
	cd .. && ./quake2.exe +set console 1 +set vid_ref "gl" +set developer 7

debug:
	cd .. && gdb -ex 'set follow-fork-mode child' --args ./quake2.exe +set vid_fullscreen 0 +set console 1 +set vid_ref "gl"
#----------------------------------------------------------------------

install: all
	sudo chown root:root ../ref_gl.so

all: $(EXE) $(GAME) $(REFGL)
	touch all

# relinking uses $^ because must include all obj files.
$(EXE) : $(SERVERFILES) $(COMMONFILES) $(CLIENTFILES) $(SHAREDFILES) $(EXE_LINUX_OBJS) $(ODIR)/snd_mixa.o
	cc -o $@ $^ $(LDFLAGS)

#always put SHAREDFILES with a library you compile!

$(REFGL) : $(REFGL_ALL_OBJS) $(SHAREDFILES)
	cc -shared -o $@ $^ $(LDFLAGS)

$(GAME) : $(SHAREDFILES) $(GAME_OBJS) 
	cc -shared -o $@ $^ $(LDFLAGS)