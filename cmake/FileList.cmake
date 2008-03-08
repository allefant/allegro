set(ALLEGRO_SRC_FILES
    src/allegro.c
    src/bitmap_new.c
    src/blenders.c
    src/blit.c
    src/bmp.c
    src/clip3d.c
    src/clip3df.c
    src/colblend.c
    src/color.c
    src/config.c
    src/convert.c
    src/datafile.c
    src/dataregi.c
    src/digmid.c
    src/display.c
    src/display_new.c
    src/dither.c
    src/dispsw.c
    src/dtor.c
    src/drvlist.c
    src/events.c
    src/evtsrc.c
    src/file.c
    src/fli.c
    src/flood.c
    src/font.c
    src/fontbios.c
    src/fontbmp.c
    src/fontdat.c
    src/fontgrx.c
    src/fonttxt.c
    src/fsel.c
    src/gfx.c
    src/glyph.c
    src/graphics.c
    src/gsprite.c
    src/gui.c
    src/guiproc.c
    src/inline.c
    src/joynu.c
    src/keybdnu.c
    src/lbm.c
    src/libc.c
    src/lzss.c
    src/math.c
    src/math3d.c
    src/memblit.c
    src/memblit1.c
    src/memblit2.c
    src/memblit3.c
    src/memdraw.c
    src/memory.c
    src/midi.c
    src/mixer.c
    src/modesel.c
    src/mousenu.c
    src/pcx.c
    src/pixels.c
    src/poly3d.c
    src/polygon.c
    src/quantize.c
    src/quat.c
    src/readbmp.c
    src/readfont.c
    src/readsmp.c
    src/rle.c
    src/rotate.c
    src/scene3d.c
    src/sound.c
    src/spline.c
    src/stream.c
    src/system_new.c
    src/text.c
    src/tga.c
    src/timernu.c
    src/tls.c
    src/unicode.c
    src/vtable.c
    src/vtable15.c
    src/vtable16.c
    src/vtable24.c
    src/vtable32.c
    src/vtable8.c
    src/compat/cojoy.c
    src/compat/cokeybd.c
    src/compat/comouse.c
    src/compat/cotimer.c
    src/misc/colconv.c
    src/misc/vector.c
    )

set(ALLEGRO_SRC_C_FILES
    src/c/cblit16.c
    src/c/cblit24.c
    src/c/cblit32.c
    src/c/cblit8.c
    src/c/ccpu.c
    src/c/ccsprite.c
    src/c/cgfx15.c
    src/c/cgfx16.c
    src/c/cgfx24.c
    src/c/cgfx32.c
    src/c/cgfx8.c
    src/c/cmisc.c
    src/c/cscan15.c
    src/c/cscan16.c
    src/c/cscan24.c
    src/c/cscan32.c
    src/c/cscan8.c
    src/c/cspr15.c
    src/c/cspr16.c
    src/c/cspr24.c
    src/c/cspr32.c
    src/c/cspr8.c
    src/c/cstretch.c
    src/c/czscan15.c
    src/c/czscan16.c
    src/c/czscan24.c
    src/c/czscan32.c
    src/c/czscan8.c
    src/misc/ccolconv.c
    )

set(ALLEGRO_SRC_I386_FILES
    src/i386/iblit16.s
    src/i386/iblit24.s
    src/i386/iblit32.s
    src/i386/iblit8.s
    src/i386/icpu.c
    src/i386/icpus.s
    src/i386/icsprite.c
    src/i386/igfx15.s
    src/i386/igfx16.s
    src/i386/igfx24.s
    src/i386/igfx32.s
    src/i386/igfx8.s
    src/i386/imisc.s
    src/i386/iscan.s
    src/i386/iscanmmx.s
    src/i386/ispr15.s
    src/i386/ispr16.s
    src/i386/ispr24.s
    src/i386/ispr32.s
    src/i386/ispr8.s
    src/i386/istretch.c
    src/i386/izbuf.s
    src/misc/icolconv.s
    )

set(ALLEGRO_SRC_DOS_FILES
    src/dos/adlib.c
    src/dos/awedata.c
    src/dos/dfile.c
    src/dos/dgfxdrv.c
    src/dos/djoydrv.c
    src/dos/dkeybd.c
    src/dos/dma.c
    src/dos/dmouse.c
    src/dos/dpmi.c
    src/dos/dsnddrv.c
    src/dos/dsystem.c
    src/dos/dtimer.c
    src/dos/emu8k.c
    src/dos/emu8kmid.c
    src/dos/essaudio.c
    src/dos/gpro.c
    src/dos/grip.c
    src/dos/gripjoy.c
    src/dos/gripfnc.s
    src/dos/ifsega.c
    src/dos/ifsega2f.c
    src/dos/ifsega2p.c
    src/dos/joystd.c
    src/dos/mpu.c
    src/dos/multijoy.c
    src/dos/n64pad.c
    src/dos/pic.c
    src/dos/psxpad.c
    src/dos/sb.c
    src/dos/sndscape.c
    src/dos/snespad.c
    src/dos/sw.c
    src/dos/swpp.c
    src/dos/swpps.s
    src/dos/vesa.c
    src/dos/vesas.s
    src/dos/wss.c
    src/dos/ww.c
    src/misc/modex.c
    src/misc/modexgfx.s
    src/misc/modexsms.c
    src/misc/pckeys.c
    src/misc/vbeaf.c
    src/misc/vbeafs.s
    src/misc/vbeafex.c
    src/misc/vga.c
    src/misc/vgaregs.c
    )

set(ALLEGRO_SRC_WIN_FILES
#    src/win/asmlock.s
    src/win/dllver.rc
    src/win/gdi.c
    src/win/wddaccel.c
    src/win/wddbmp.c
    src/win/wddbmpl.c
    src/win/wddraw.c
    src/win/wddfull.c
    src/win/wddlock.c
    src/win/wddmode.c
    src/win/wddovl.c
    src/win/wddwin.c
    src/win/wdsinput.c
    src/win/wdsndmix.c
    src/win/wdsound.c
    src/win/wsndwo.c
    src/win/wdxver.c
    src/win/wdispsw.c
    src/win/wfile.c
    src/win/wgdi.c
    src/win/wgfxdrv.c
    src/win/winput.c
    src/win/wjoydrv.c
    src/win/wjoydxnu.c
    src/win/wkeybdnu.c
    src/win/wmidi.c
    src/win/wmouse.c
    src/win/wsnddrv.c
    src/win/wsystem.c
    src/win/wthread.c
    src/win/wtime.c
    src/win/wwnd.c
    src/win/wxthread.c
    src/win/wnewsys.c
    src/win/wnewwin.c
    )

set(ALLEGRO_SRC_D3D_FILES
    src/win/d3d_bmp.c
    src/win/d3d_disp.c
    )

set(ALLEGRO_SRC_OPENGL_FILES
    src/opengl/extensions.c
    src/opengl/ogl_bitmap.c
    src/opengl/ogl_draw.c
    src/opengl/ogl_display.c
    )

set(ALLEGRO_SRC_WGL_FILES
    src/win/wgl_disp.c
    )

set(ALLEGRO_SRC_BEOS_FILES
    src/beos/baccel.cpp
    src/beos/bdispsw.cpp
    src/beos/bdwindow.cpp
    src/beos/bgfx.c
    src/beos/bgfxapi.cpp
    src/beos/bgfxdrv.c
    src/beos/bjoy.c
    src/beos/bjoydrv.c
    src/beos/bjoyapi.cpp
    src/beos/bkey.c
    src/beos/bkeyapi.cpp
    src/beos/bkeydrv.c
    src/beos/bmidi.c
    src/beos/bmidiapi.cpp
    src/beos/bmididrv.c
    src/beos/bmousapi.cpp
    src/beos/bmousdrv.c
    src/beos/bmouse.c
    src/beos/boverlay.c
    src/beos/bsnd.c
    src/beos/bsndapi.cpp
    src/beos/bsnddrv.c
    src/beos/bswitch.s
    src/beos/bsysapi.cpp
    src/beos/bsysdrv.c
    src/beos/bsystem.c
    src/beos/btimeapi.cpp
    src/beos/btimedrv.c
    src/beos/btimer.c
    src/beos/bwindow.c
    src/beos/bwscreen.cpp
    src/unix/ufile.c
    src/misc/colconv.c
    src/misc/pckeys.c
    )

set(ALLEGRO_SRC_LINUX_FILES
    src/linux/fbcon.c
    src/linux/lconsole.c
    src/linux/lgfxdrv.c
    src/linux/ljoynu.c
    src/linux/lkeybdnu.c
    src/linux/lmemory.c
    src/linux/lmsedrv.c
    src/linux/lmseev.c
    src/linux/lsystem.c
    src/linux/lvga.c
    src/linux/lvgahelp.c
    src/linux/svgalib.c
    src/linux/svgalibs.s
    src/linux/vtswitch.c
    src/misc/vbeaf.c
    src/misc/vbeafs.s
    src/misc/vgaregs.c
    src/misc/vga.c
    src/misc/modex.c
    src/misc/modexgfx.s
    )

set(ALLEGRO_SRC_UNIX_FILES
    src/unix/alsa5.c
    src/unix/alsa9.c
    src/unix/alsamidi.c
    src/unix/arts.c
    src/unix/jack.c
    src/unix/sgial.c
    src/unix/udjgpp.c
    src/unix/udrvlist.c
    src/unix/udummy.c
    src/unix/uesd.c
    src/unix/ufdwatch.c
    src/unix/ufile.c
    src/unix/ugfxdrv.c
    src/unix/ujoydrv.c
    src/unix/ukeybd.c
    src/unix/umain.c
    src/unix/umodules.c
    src/unix/umouse.c
    src/unix/uoss.c
    src/unix/uossmidi.c
    src/unix/usnddrv.c
    src/unix/usystem.c
    src/unix/uthreads.c
    src/unix/utime.c
    src/unix/uxthread.c
    src/misc/modexsms.c
    )

set(ALLEGRO_SRC_X_FILES
    src/x/xdga2.c
    src/x/xgfxdrv.c
    src/x/xkeyboard.c
    src/x/xmousenu.c
    src/x/xsystem.c
    src/x/xvtable.c
    src/x/xwin.c
    src/xglx/xcompat.c
    src/xglx/xdisplay.c
    src/xglx/xfullscreen.c
    src/xglx/xglx_config.c
    src/xglx/xsystem.c
    )
    
set(ALLEGRO_SRC_QNX_FILES
    src/qnx/qdrivers.c
    src/qnx/qkeydrv.c
    src/qnx/qmouse.c
    src/qnx/qphaccel.c
    src/qnx/qphbmp.c
    src/qnx/qphfull.c
    src/qnx/qphoton.c
    src/qnx/qphwin.c
    src/qnx/qswitch.s
    src/qnx/qsystem.c
    src/unix/alsa5.c
    src/unix/alsamidi.c
    src/unix/udjgpp.c
    src/unix/ufile.c
    src/unix/umain.c
    src/unix/usystem.c
    src/unix/uthreads.c
    src/unix/utimer.c
    src/misc/colconv.c
    src/misc/pckeys.c
    )

set(ALLEGRO_SRC_MACOSX_FILES
    src/macosx/cadigi.m
    src/macosx/camidi.m
    src/macosx/drivers.m
    src/macosx/hidjoy.m
    src/macosx/hidman.m
    src/macosx/keybd.m
    src/macosx/pcpu.m
    src/macosx/qzmouse.m
    src/macosx/system.m
    src/macosx/osxgl.m
    src/unix/ufile.c
    src/unix/uthreads.c
    src/unix/utime.c
    src/unix/uxthread.c
    src/opengl/extensions.c
    src/opengl/ogl_bitmap.c
    src/opengl/ogl_draw.c
    src/opengl/ogl_display.c
    )

set(ALLEGRO_MODULE_VGA_FILES
    src/linux/lvga.c
    src/misc/modex.c
    src/misc/modexgfx.s
    src/misc/vga.c
    )

set(ALLEGRO_MODULE_SVGALIB_FILES
    src/linux/svgalib.c
    src/linux/svgalibs.s
    )

set(ALLEGRO_MODULE_FBCON_FILES
    src/linux/fbcon.c
    )

set(ALLEGRO_MODULE_DGA2_FILES
    src/x/xdga2.c
    src/x/xdga2s.s
    )
    
set(ALLEGRO_MODULE_ALSADIGI_FILES
    src/unix/alsa5.c
    src/unix/alsa9.c
    )

set(ALLEGRO_MODULE_ALSAMIDI_FILES
    src/unix/alsamidi.c
    )

set(ALLEGRO_MODULE_ESD_FILES
    src/unix/uesd.c
    )

set(ALLEGRO_MODULE_ARTS_FILES
    src/unix/arts.c
    )

set(ALLEGRO_MODULE_SGIAL_FILES
    src/unix/sgial.c
    )

set(ALLEGRO_MODULE_JACK_FILES
    src/unix/jack.c
    )

set(ALLEGRO_INCLUDE_FILES
    # No files directly in the `include' root any more.
    )

set(ALLEGRO_INCLUDE_ALLEGRO_FILES
    include/allegro5/allegro5.h
    include/allegro5/allegro.h
    include/allegro5/winalleg.h
    include/allegro5/xalleg.h
    include/allegro5/3d.h
    include/allegro5/3dmaths.h
    include/allegro5/alcompat.h
    include/allegro5/alinline.h
    include/allegro5/altime.h
    include/allegro5/base.h
    include/allegro5/bitmap_new.h
    include/allegro5/color.h
    include/allegro5/color_new.h
    include/allegro5/compiled.h
    include/allegro5/config.h
    include/allegro5/datafile.h
    include/allegro5/debug.h
    include/allegro5/digi.h
    include/allegro5/display_new.h
    include/allegro5/draw.h
    include/allegro5/events.h
    include/allegro5/file.h
    include/allegro5/fix.h
    include/allegro5/fixed.h
    include/allegro5/fli.h
    include/allegro5/fmaths.h
    include/allegro5/font.h
    include/allegro5/gfx.h
    include/allegro5/graphics.h
    include/allegro5/gui.h
    include/allegro5/joystick.h
    include/allegro5/keyboard.h
    include/allegro5/keycodes.h
    include/allegro5/lzss.h
    include/allegro5/matrix.h
    include/allegro5/memory.h
    include/allegro5/midi.h
    include/allegro5/mouse.h
    include/allegro5/palette.h
    include/allegro5/quat.h
    include/allegro5/rle.h
    include/allegro5/sound.h
    include/allegro5/stream.h
    include/allegro5/system.h
    include/allegro5/system_new.h
    include/allegro5/text.h
    include/allegro5/timer.h
    include/allegro5/unicode.h
    )

set(ALLEGRO_INCLUDE_ALLEGRO_INLINE_FILES
    include/allegro5/inline/3dmaths.inl
    include/allegro5/inline/asm.inl
    include/allegro5/inline/color.inl
    include/allegro5/inline/draw.inl
    include/allegro5/inline/file.inl
    include/allegro5/inline/fix.inl
    include/allegro5/inline/fmaths.inl
    include/allegro5/inline/gfx.inl
    include/allegro5/inline/matrix.inl
    include/allegro5/inline/rle.inl
    include/allegro5/inline/system.inl
    )

set(ALLEGRO_INCLUDE_ALLEGRO_INTERNAL_FILES
    include/allegro5/internal/aintern.h
    include/allegro5/internal/aintern_bitmap.h
    include/allegro5/internal/aintern_display.h
    include/allegro5/internal/aintern_dtor.h
    include/allegro5/internal/aintern_events.h
    include/allegro5/internal/aintern_joystick.h
    include/allegro5/internal/aintern_keyboard.h
    include/allegro5/internal/aintern_mouse.h
    include/allegro5/internal/aintern_system.h
    include/allegro5/internal/aintern_thread.h
    include/allegro5/internal/aintern_vector.h
    include/allegro5/internal/aintern_opengl.h
    include/allegro5/internal/aintvga.h
    include/allegro5/internal/alconfig.h
    )

set(ALLEGRO_INCLUDE_ALLEGRO_OPENGL_FILES
    include/allegro5/opengl/gl_ext.h
    include/allegro5/opengl/GLext/gl_ext_alias.h
    include/allegro5/opengl/GLext/gl_ext_defs.h
    include/allegro5/opengl/GLext/glx_ext_alias.h
    include/allegro5/opengl/GLext/glx_ext_defs.h
    include/allegro5/opengl/GLext/wgl_ext_alias.h
    include/allegro5/opengl/GLext/wgl_ext_defs.h
    include/allegro5/opengl/GLext/gl_ext_api.h
    include/allegro5/opengl/GLext/gl_ext_list.h
    include/allegro5/opengl/GLext/glx_ext_api.h
    include/allegro5/opengl/GLext/glx_ext_list.h
    include/allegro5/opengl/GLext/wgl_ext_api.h
    include/allegro5/opengl/GLext/wgl_ext_list.h
   )

set(ALLEGRO_INCLUDE_ALLEGRO_PLATFORM_FILES
    include/allegro5/platform/aintbeos.h
    include/allegro5/platform/aintdos.h
    include/allegro5/platform/aintlnx.h
    include/allegro5/platform/aintmac.h
    include/allegro5/platform/aintosx.h
    include/allegro5/platform/aintqnx.h
    include/allegro5/platform/aintunix.h
    include/allegro5/platform/aintuthr.h
    include/allegro5/platform/aintwin.h
    include/allegro5/platform/aintwthr.h
    include/allegro5/platform/al386gcc.h
    include/allegro5/platform/al386vc.h
    include/allegro5/platform/al386wat.h
    include/allegro5/platform/albcc32.h
    include/allegro5/platform/albecfg.h
    include/allegro5/platform/albeos.h
    include/allegro5/platform/aldjgpp.h
    include/allegro5/platform/aldos.h
    include/allegro5/platform/almac.h
    include/allegro5/platform/almaccfg.h
    include/allegro5/platform/almngw32.h
    include/allegro5/platform/almsvc.h
    include/allegro5/platform/alosx.h
    include/allegro5/platform/alosxcfg.h
    include/allegro5/platform/alqnx.h
    include/allegro5/platform/alqnxcfg.h
    include/allegro5/platform/alucfg.h
    include/allegro5/platform/alunix.h
    include/allegro5/platform/alwatcom.h
    include/allegro5/platform/alwin.h
    include/allegro5/platform/astdbool.h
    include/allegro5/platform/astdint.h
    include/allegro5/platform/macdef.h
    )

set(ALLEGRO_INCLUDE_ALLEGRO_PLATFORM_FILES_GENERATED
    include/allegro5/platform/alplatf.h
    )

if(UNIX)
   LIST(APPEND ALLEGRO_INCLUDE_ALLEGRO_PLATFORM_FILES_GENERATED include/allegro5/platform/alunixac.h)
endif(UNIX)
