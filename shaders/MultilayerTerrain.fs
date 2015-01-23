//////////////////////////////////////////////////////////////////////////////////////////
//
// default frament shader for solid colours
//

// constant parameters
uniform vec4 lighting[17];
uniform int num_lights;
uniform vec4 diffuse;

uniform vec2 heightRange;

// inputs
varying vec2 uv_;
varying vec3 normal_;
varying vec3 camera_pos_;
varying vec4 color_;
varying vec3 model_pos_;

void main() 
{
  vec3 nnormal = normalize(normal_);

  vec4 cRange0 = vec4(34.0/255.0,139.0/255.0,34.0/255.0, 1.0); //floor
  float fRange1 = 0.25;
  vec4 cRange1 = vec4(102.0/255.0, 51.0/255.0, 0.0, 1.0); 
  float fRange2 = 0.5;  
  vec4 cRange2 = vec4(128.0/255.0, 109.0/255.0, 109.0/255.0, 1.0);
  float fRange3 = 0.75;
  vec4 cRange3 = vec4(168.0/255.0, 167.0/255.0, 167.0/255.0, 1.0);
  vec4 cRange4 = vec4(1.0, 1.0, 1.0, 1.0);

  float range = heightRange.y - heightRange.x;
  
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

  float heightPercentage = (model_pos_.y - heightRange.x) / range;
  if(heightPercentage < fRange1)
    diffuse = mix(cRange0, cRange1, (heightPercentage - 0) / (fRange1 - 0));
  else if(heightPercentage < fRange2)
    diffuse = mix(cRange1, cRange2, (heightPercentage - fRange1) / (fRange2 - fRange1));
  else if(heightPercentage < fRange3)
   diffuse = mix(cRange2, cRange3, (heightPercentage - fRange2) / (fRange3 - fRange2));
  else
   diffuse = mix(cRange3, cRange4, (heightPercentage - fRange3) / (1.0 - fRange3));      

  gl_FragColor = vec4(diffuse.xyz * diffuse_light, 1.0);

}

