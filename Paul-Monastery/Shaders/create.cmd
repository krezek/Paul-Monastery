call "C:\Program Files\Microsoft Visual Studio\2022\Professional\Common7\Tools\VsDevCmd.bat"

fxc "sky.hlsl" /T vs_5_1 /E "VS" /Fo "sky_vs.cso" /Fc "sky_vs.asm"
fxc "sky.hlsl" /T ps_5_1 /E "PS" /Fo "sky_ps.cso" /Fc "sky_ps.asm"

fxc "Fixed.hlsl" /T vs_5_1 /E "VS" /Fo "fixed_vs.cso" /Fc "fixed_vs.asm"
fxc "Fixed.hlsl" /T ps_5_1 /E "PS" /Fo "fixed_ps.cso" /Fc "fixed_ps.asm"

fxc "Default.hlsl" /T vs_5_1 /E "VS" /Fo "default_vs.cso" /Fc "default_vs.asm"
fxc "Default.hlsl" /T ps_5_1 /E "PS" /Fo "default_ps.cso" /Fc "default_ps.asm"


pause