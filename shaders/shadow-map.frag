#version 330 core

/* Outputs */
out vec4 fs_color;

void main()
{
   /* FIXME: This does not handle transparent textures that should not cast a
    * shadow. To fix that we should sample the texture here and discard the
    * fragment if the texel is transparent (i.e. alpha channels is low enough):
    *
    * float alpha = texture(tex, tex_uv).a;
    * if (alpha < 0.5)
    *    discard();
    */
   fs_color = vec4(1.0);
}
