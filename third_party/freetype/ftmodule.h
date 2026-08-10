/*
 * Trimmed FreeType module registry.
 *
 * FreeType's default config/ftmodule.h registers every driver/renderer, but
 * this port compiles only a subset (see BUILD.gn). Registering a module whose
 * *_class object isn't compiled produces undefined-symbol link errors, so this
 * list must stay in sync with BUILD.gn's `sources`. Selected via the
 * FT_CONFIG_MODULES_H define in BUILD.gn.
 */
FT_USE_MODULE( FT_Module_Class, autofit_module_class )
FT_USE_MODULE( FT_Driver_ClassRec, tt_driver_class )
FT_USE_MODULE( FT_Driver_ClassRec, cff_driver_class )
FT_USE_MODULE( FT_Module_Class, psaux_module_class )
FT_USE_MODULE( FT_Module_Class, psnames_module_class )
FT_USE_MODULE( FT_Module_Class, pshinter_module_class )
FT_USE_MODULE( FT_Module_Class, sfnt_module_class )
FT_USE_MODULE( FT_Renderer_Class, ft_smooth_renderer_class )
FT_USE_MODULE( FT_Renderer_Class, ft_raster1_renderer_class )
