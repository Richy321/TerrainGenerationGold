//////////////////////////////////////////////////////////////////////////////////////////
//
// default frament shader for solid colours
//

// constant parameters
uniform vec4 lighting[17];
uniform int num_lights;
uniform vec4 diffuse;

// inputs
varying vec2 uv_;
varying vec3 normal_;
varying vec3 camera_pos_;
varying vec4 color_;
varying vec3 model_pos_;

void main() 
{
  vec3 nnormal = normalize(normal_);

  float fRange1 = 0.33;
  vec4 cRange1 = vec4(0.0, 0.0, 1.0, 1.0); 
  float fRange2 = 0.66;  
  vec4 cRange2 = vec4(0.5, 0.5, 0.0, 1.0);
  float fRange3 = 1.0;
  vec4 cRange3 = vec4(0.0, 0.0, 0.0, 1.0);
  
  float minHeight = -100;
  float maxHeight = 100;
  float range = maxHeight - minHeight;
  
  vec3 npos = camera_pos_;
  vec3 diffuse_light = lighting[0].xyz;
  for (int i = 0; i != num_lights; ++i) {
    vec3 light_pos = lighting[i * 4 + 1].xyz;
    vec3 light_direction = lighting[i * 4 + 2].xyz;
    vec3 light_color = lighting[i * 4 + 3].xyz;
    vec3 light_atten = lighting[i * 4 + 4].xyz;
    float diffuse_factor = max(dot(light_direction, nnormal), 0.0);
    diffuse_light += diffuse_factor * light_color;
  }

  float heightPercent = (model_pos_.y - minHeight) / range;

  diffuse = mix(cRange1, cRange2, heightPercent);
  gl_FragColor = vec4(diffuse.xyz * diffuse_light, 1.0);

}

