// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include <engine/platform/platform_system.h>
#include <engine/renderer/renderer_system.h>

auto static constexpr fs_shader = R"(
#version 330

out vec4 outColor;
uniform vec3 position;

const vec2 uResolution = vec2(1280.0, 720.0);

const int MAX_MARCHING_STEPS = 255;
const float MIN_DIST = 0.0;
const float MAX_DIST = 100.0;
const float EPSILON = 0.0001;


// SDF Functions
float sphere_sdf(vec3 sample_point, float radius)
{
  return length(sample_point) - radius;
}

float intersect_sdf(float dist0, float dist1)
{
  return max(dist0, dist1);
}

float union_sdf(float dist0, float dist1)
{
  return min(dist0, dist1);
}

float difference_sdf(float dist0, float dist1)
{
  return max(dist0, -dist1);
}


// Scene
float scene(vec3 sample_point)
{
  float sphere_a = sphere_sdf(sample_point, 0.4);
  float sphere_b = sphere_sdf(sample_point - vec3(-0.3, 0.0, 0.0), 0.4);
  return union_sdf(sphere_a, sphere_b);
}


// Raymarching
float raymarch(vec3 eye, vec3 raymarch_direction, float start, float end)
{
  float depth = start;
  for (int i = 0; i < MAX_MARCHING_STEPS; ++i)
  {
    float dist = scene(eye + depth * raymarch_direction);

    // We're inside the surface
    if (dist < EPSILON) return depth;

    // Step along the ray
    depth += dist;

    // Reached the end
    if (depth >= end) break;
  }

  return end;
}

vec3 ray_direction(float fov, vec2 size, vec2 frag_coord)
{
  vec2 xy = frag_coord - size / 2.0;
  float z = size.y * 0.5 / tan(radians(fov) / 2.0);
  return normalize(vec3(xy, -z));
}



// Lighting
vec3 estimate_normal(vec3 p)
{
  return normalize(vec3(
    scene(vec3(p.x + EPSILON, p.y, p.z)) - scene(vec3(p.x - EPSILON, p.y, p.z)),
    scene(vec3(p.x, p.y + EPSILON, p.z)) - scene(vec3(p.x, p.y - EPSILON, p.z)),
    scene(vec3(p.x, p.y, p.z + EPSILON)) - scene(vec3(p.x, p.y, p.z - EPSILON))
  ));
}


// k_a: Ambient
// k_d: Diffuse
// k_s: Specular
// alpha: specular
// p: point being lit
// eye: camera position
vec3 phong_contribution(vec3 k_d, vec3 k_s, float alpha, vec3 p, vec3 eye, vec3 light_position, vec3 light_intensity)
{
  vec3 N = estimate_normal(p);
  vec3 L = normalize(light_position - p);
  vec3 V = normalize(eye - p);
  vec3 R = normalize(reflect(-L, N));

  float dot_ln = clamp(dot(L, N), 0.0, 1.0);
  float dot_rv = dot(R, V);

  // Light not visible from this point
  if (dot_ln < 0.0) return vec3(0.0, 0.0, 0.0);

  if (dot_rv < 0.0) return light_intensity * (k_d * dot_ln);

  return light_intensity * (k_d * dot_ln + k_s * pow(dot_rv, alpha));
}

vec3 phong(vec3 k_a, vec3 k_d, vec3 k_s, float alpha, vec3 p, vec3 eye)
{
  const vec3 light_position = vec3(4.0, 2.0, 4.0);
  const vec3 light_intensity = vec3(0.4, 0.4, 0.4);

  const vec3 ambient_light = vec3(0.25, 0.25, 0.25);

  vec3 color = ambient_light * k_a;

  color += phong_contribution(k_d, k_s, alpha, p, eye, light_position, light_intensity);

 return color;
}


// Camera
mat4 look_at(vec3 position, vec3 target, vec3 up)
{
  vec3 z = normalize(target - position);
  vec3 x = normalize(cross(z, up));
  vec3 y = cross(x, z);

  return mat4(
    vec4(x, 0.0),
    vec4(y, 0.0),
    vec4(-z, 0.0),
    vec4(0.0, 0.0, 0.0, 1.0)
  );
}


// Main
void main() {
  vec3 camera_direction = ray_direction(45.0, uResolution, gl_FragCoord.xy);
  vec3 camera_position = vec3(0.0, 0.0, 5.0) - position;

  mat4 view_to_world = look_at(camera_position, vec3(0.0, 0.0, 0.0), vec3(0.0, 1.0, 0.0));
  vec3 world_direction = (view_to_world * vec4(camera_direction, 0.0)).xyz;

  float dist = raymarch(camera_position, camera_direction, MIN_DIST, MAX_DIST);

  if (dist > MAX_DIST - EPSILON)
  {
    // didn't hit anything
    vec3 color = vec3(0.0, 0.0, 0.0);
    return;
  }

  vec3 p = camera_position + dist * camera_direction;


  // Light colors
  const vec3 ambient = vec3(0.2, 0.2, 0.2);
  const vec3 diffuse = vec3(0.7, 0.2, 0.2);
  const vec3 specular = vec3(1.0, 1.0, 1.0);
  const float power = 10.0;

  vec3 color = phong(ambient, diffuse, specular, power, p, camera_direction);

  outColor = vec4(color, 1.0);
}
)";

extern bool running;

extern "C" auto entry() -> void {
    if (!xc::platform::initialize()) xc::platform::exit(-1);
    if (!xc::renderer::initialize()) xc::platform::exit(-1);

    auto shader = xc::renderer::create_shader({}, fs_shader);

    xc::renderer::bind_shader(shader);
    xc::renderer::set_shader_uniform(shader, "position", xc::vector3{0.f, 0.5f, 0.f});

    while (running) {
        xc::platform::tick();
        xc::renderer::tick();
    }

    xc::platform::uninitialize();

    xc::platform::exit(0);
}


// TODO:
// Wrap if (!thing) print() lines in a macro that removes the generated code in release builds
// small buffer optimization for array
// event system
// replace extern bool running with events?
