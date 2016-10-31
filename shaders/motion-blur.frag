#version 330 core

/* Inputs */
in vec2 tex_coords;

/* Uniforms */
uniform sampler2D Tex;
uniform sampler2D MotionTex;
uniform float MotionDivisor;

/* Outputs */
out vec4 fs_color;

void main()
{
   fs_color = vec4(0.0);

   vec2 motion_vector = texture(MotionTex, tex_coords).xy - 0.5;

   /* Let's make sure that we don't apply any motion vector to
    * things that are still because of precission errors.
    */
   if (length(motion_vector) > 0.01) {
      motion_vector /= MotionDivisor;
      vec2 coords = tex_coords;

      fs_color += texture(Tex, coords) * 0.50;
      coords -= motion_vector;
      fs_color += texture(Tex, coords) * 0.25;
      coords -= motion_vector;
      fs_color += texture(Tex, coords) * 0.13;
      coords -= motion_vector;
      fs_color += texture(Tex, coords) * 0.08;
      coords -= motion_vector;
      fs_color += texture(Tex, coords) * 0.04;
   } else {
      fs_color = texture(Tex, tex_coords);
   }
}
