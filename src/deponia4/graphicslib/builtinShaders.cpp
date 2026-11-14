// Made by tools/gen_builtin_shaders.py from the shaders built into the Linux executable (`builtins` of `BuiltinShader()`): the GLSL of
// each, as the original's `GLShader::CompileFromMemory()` takes it (the sections "glsl", "vs" and "ps").
#include "graphicslib/builtinShaders.h"

const TBuiltinShader g_builtinShaders[] = {
	{"3d_ao.bin",
	 R"GLSL(//gl_100
#ifdef GL_ES
precision mediump float;
precision lowp int;
#endif
//Input Layout:
//vec4 inPosition
//vec2 inTexCoord
attribute vec4 inPosition;
attribute vec2 inTexCoord;
varying vec2 outTexCoord;

void main()
{
  outTexCoord = inTexCoord;
  gl_Position = inPosition;
}
)GLSL",
	 R"GLSL(//gl_100
#ifdef GL_ES
precision mediump float;
precision lowp int;
#endif
uniform sampler2D _texture;
uniform sampler2D randomNormalTexture;
uniform float strength;
varying vec2 outTexCoord;

void main()
{
  vec3 samples[16];
  samples[0] = vec3(-0.010736, 0.016470, 0.006243);
  samples[1] = vec3(-0.065334, 0.364701, -0.137463);
  samples[2] = vec3(-0.653923, -0.016726, -0.530010);
  samples[3] = vec3(0.409583, 0.005243, -0.559112);
  samples[4] = vec3(-0.146537, 0.098993, 0.155717);
  samples[5] = vec3(-0.441221, -0.545880, 0.049125);
  samples[6] = vec3(0.037556, -0.109613, -0.330403);
  samples[7] = vec3(0.019100, 0.296528, 0.066238);
  samples[8] = vec3(0.876532, 0.011236, 0.282660);
  samples[9] = vec3(0.292644, -0.407942, 0.159642);
  samples[10] = vec3(0.010350, -0.586983, 0.004629);
  samples[11] = vec3(-0.089729, -0.494082, 0.328790);
  samples[12] = vec3(0.711999, -0.015469, -0.091837);
  samples[13] = vec3(-0.053382, 0.059676, -0.541190);
  samples[14] = vec3(0.035268, -0.063189, 0.546027);
  samples[15] = vec3(-0.477611, 0.284791, -0.027172);
   float depth = texture2D(_texture, outTexCoord).x;
   vec4 pattern = texture2D(randomNormalTexture, vec2((outTexCoord.x * 1024.000000), (outTexCoord.y * 1024.000000)));
  pattern = normalize(((2.000000 * pattern) - 1.000000));
   float occlusion = 0.000000;
   int i = 0;
  while(i < 16)
  {
      vec2 samplePoint = (((strength / 10000.000000) * reflect(samples[i].xyz, pattern.xyz).xy) + outTexCoord.xy);
      float sampleDepth = texture2D(_texture, samplePoint).x;
      float dist = (strength * max((depth - sampleDepth), 0.000000));
     occlusion += (1.000000 / (1.000000 + (dist * dist)));
i++;
  }
  gl_FragColor.xyz = vec3(0.000000, 0.000000, 0.000000);
  occlusion /= 16.000000;
  if((occlusion >= 1.000000 || depth >= 0.900000))
  {
     gl_FragColor.w = 0.000000;
  }
  else
  {
     gl_FragColor.w = (1.000000 - occlusion);
  }
}
)GLSL"},
	{"3d_blobshadow.bin",
	 R"GLSL(//gl_100
#ifdef GL_ES
precision mediump float;
precision lowp int;
#endif
//Input Layout:
//vec4 inPosition
//vec2 inTexCoord
uniform mat4 modelViewProjectionMatrix;
attribute vec4 inPosition;
attribute vec2 inTexCoord;
varying vec2 outTexCoord;

void main()
{
  outTexCoord = inTexCoord;
  gl_Position = (modelViewProjectionMatrix * inPosition);
}
)GLSL",
	 R"GLSL(//gl_100
#ifdef GL_ES
precision mediump float;
precision lowp int;
#endif
uniform float shadowVisibility;
varying vec2 outTexCoord;

void main()
{
   vec2 dist = (vec2(0.500000, 0.500000) - outTexCoord);
   float pointDistance = sqrt(((dist.x * dist.x) + (dist.y * dist.y)));
   float translucency = (pointDistance / 0.500000);
  gl_FragColor = vec4(0.000000, 0.000000, 0.000000, clamp((shadowVisibility - translucency), 0.000000, 1.000000));
}
)GLSL"},
	{"3d_blur.bin",
	 R"GLSL(//gl_100
#ifdef GL_ES
precision mediump float;
precision lowp int;
#endif
//Input Layout:
//vec4 inPosition
//vec2 inTexCoord
attribute vec4 inPosition;
attribute vec2 inTexCoord;
varying vec2 outTexCoord;

void main()
{
  outTexCoord = inTexCoord;
  gl_Position = inPosition;
}
)GLSL",
	 R"GLSL(//gl_100
#ifdef GL_ES
precision mediump float;
precision lowp int;
#endif
uniform sampler2D _texture;
varying vec2 outTexCoord;

void main()
{
  vec2 offsets[9];
  offsets[0] = vec2(0.000977, 0.000977);
  offsets[1] = vec2(0.000977, 0.000000);
  offsets[2] = vec2(0.000977, -0.000977);
  offsets[3] = vec2(0.000000, 0.000977);
  offsets[4] = vec2(0.000000, 0.000000);
  offsets[5] = vec2(0.000000, -0.000977);
  offsets[6] = vec2(-0.000977, 0.000977);
  offsets[7] = vec2(-0.000977, 0.000000);
  offsets[8] = vec2(-0.000977, -0.000977);
  vec4 samples[9];
  samples[0] = texture2D(_texture, (outTexCoord.xy + offsets[0]));
  samples[1] = texture2D(_texture, (outTexCoord.xy + offsets[1]));
  samples[2] = texture2D(_texture, (outTexCoord.xy + offsets[2]));
  samples[3] = texture2D(_texture, (outTexCoord.xy + offsets[3]));
  samples[4] = texture2D(_texture, (outTexCoord.xy + offsets[4]));
  samples[5] = texture2D(_texture, (outTexCoord.xy + offsets[5]));
  samples[6] = texture2D(_texture, (outTexCoord.xy + offsets[6]));
  samples[7] = texture2D(_texture, (outTexCoord.xy + offsets[7]));
  samples[8] = texture2D(_texture, (outTexCoord.xy + offsets[8]));
  gl_FragColor = (((((((((samples[0] + (2.000000 * samples[1])) + samples[2]) + (2.000000 * samples[3])) + samples[4]) + (2.000000 * samples[5])) + samples[6]) + (2.000000 * samples[7])) + samples[8]) / 13.000000);
}
)GLSL"},
	{"3d_blurhori.bin",
	 R"GLSL(//gl_100
#ifdef GL_ES
precision mediump float;
precision lowp int;
#endif
//Input Layout:
//vec4 inPosition
//vec2 inTexCoord
attribute vec4 inPosition;
attribute vec2 inTexCoord;
varying vec2 outTexCoord;

void main()
{
  outTexCoord = inTexCoord;
  gl_Position = inPosition;
}
)GLSL",
	 R"GLSL(//gl_100
#ifdef GL_ES
precision mediump float;
precision lowp int;
#endif
uniform sampler2D _texture;
varying vec2 outTexCoord;

void main()
{
   vec4 sum = vec4(0.000000, 0.000000, 0.000000, 0.000000);
  sum += (texture2D(_texture, vec2((outTexCoord.x - 0.003906), outTexCoord.y)) * 0.050000);
  sum += (texture2D(_texture, vec2((outTexCoord.x - 0.002930), outTexCoord.y)) * 0.090000);
  sum += (texture2D(_texture, vec2((outTexCoord.x - 0.001953), outTexCoord.y)) * 0.120000);
  sum += (texture2D(_texture, vec2((outTexCoord.x - 0.000977), outTexCoord.y)) * 0.150500);
  sum += (texture2D(_texture, vec2(outTexCoord.x, outTexCoord.y)) * 0.178850);
  sum += (texture2D(_texture, vec2((outTexCoord.x + 0.000977), outTexCoord.y)) * 0.150500);
  sum += (texture2D(_texture, vec2((outTexCoord.x + 0.001953), outTexCoord.y)) * 0.120000);
  sum += (texture2D(_texture, vec2((outTexCoord.x + 0.002930), outTexCoord.y)) * 0.090000);
  sum += (texture2D(_texture, vec2((outTexCoord.x + 0.003906), outTexCoord.y)) * 0.050000);
  gl_FragColor = sum;
}
)GLSL"},
	{"3d_blurvert.bin",
	 R"GLSL(//gl_100
#ifdef GL_ES
precision mediump float;
precision lowp int;
#endif
//Input Layout:
//vec4 inPosition
//vec2 inTexCoord
attribute vec4 inPosition;
attribute vec2 inTexCoord;
varying vec2 outTexCoord;

void main()
{
  outTexCoord = inTexCoord;
  gl_Position = inPosition;
}
)GLSL",
	 R"GLSL(//gl_100
#ifdef GL_ES
precision mediump float;
precision lowp int;
#endif
uniform sampler2D _texture;
varying vec2 outTexCoord;

void main()
{
   vec4 sum = vec4(0.000000, 0.000000, 0.000000, 0.000000);
  sum += (texture2D(_texture, vec2(outTexCoord.x, (outTexCoord.y - 0.003906))) * 0.050000);
  sum += (texture2D(_texture, vec2(outTexCoord.x, (outTexCoord.y - 0.002930))) * 0.090000);
  sum += (texture2D(_texture, vec2(outTexCoord.x, (outTexCoord.y - 0.001953))) * 0.120000);
  sum += (texture2D(_texture, vec2(outTexCoord.x, (outTexCoord.y - 0.000977))) * 0.150500);
  sum += (texture2D(_texture, vec2(outTexCoord.x, outTexCoord.y)) * 0.178850);
  sum += (texture2D(_texture, vec2(outTexCoord.x, (outTexCoord.y + 0.000977))) * 0.150500);
  sum += (texture2D(_texture, vec2(outTexCoord.x, (outTexCoord.y + 0.001953))) * 0.120000);
  sum += (texture2D(_texture, vec2(outTexCoord.x, (outTexCoord.y + 0.002930))) * 0.090000);
  sum += (texture2D(_texture, vec2(outTexCoord.x, (outTexCoord.y + 0.003906))) * 0.050000);
  gl_FragColor = sum;
}
)GLSL"},
	{"3d_diffuse.bin",
	 R"GLSL(//gl_100
#ifdef GL_ES
precision mediump float;
precision lowp int;
#endif
//Input Layout:
//vec4 inPosition
//vec3 inNormal
//vec4 inColor
//vec2 inTexCoord
//vec4 inBoneIndices
//vec4 inBoneWeights
uniform mat4 modelMatrix;
uniform mat4 modelViewProjectionMatrix;
uniform mat4 boneMatrices[60];
attribute vec4 inPosition;
attribute vec3 inNormal;
attribute vec4 inColor;
attribute vec2 inTexCoord;
attribute vec4 inBoneIndices;
attribute vec4 inBoneWeights;
varying vec4 worldPosition;
varying vec3 worldNormal;
varying vec4 outColor;
varying vec2 outTexCoord;

void main()
{
   vec4 boneWeights = inBoneWeights;
  boneWeights.w = (1.000000 - dot(boneWeights.xyz, vec3(1.000000, 1.000000, 1.000000)));
   mat4 transformMatrix = (boneWeights.x * boneMatrices[int(inBoneIndices.x)]);
  transformMatrix += (boneWeights.y * boneMatrices[int(inBoneIndices.y)]);
  transformMatrix += (boneWeights.z * boneMatrices[int(inBoneIndices.z)]);
  transformMatrix += (boneWeights.w * boneMatrices[int(inBoneIndices.w)]);
   vec4 newPosition = (transformMatrix * inPosition);
   vec4 newNormal = (transformMatrix * vec4(inNormal, 0.000000));
  worldNormal = (modelMatrix * newNormal).xyz;
  worldNormal.y = -worldNormal.y;
  gl_Position = (modelViewProjectionMatrix * newPosition);
  worldPosition = (modelMatrix * newPosition);
  outColor = inColor;
  outTexCoord = inTexCoord;
}
)GLSL",
	 R"GLSL(//gl_100
#ifdef GL_ES
precision mediump float;
precision lowp int;
#endif
uniform vec3 lightPosition;
uniform vec4 lightAmbientColor;
uniform vec4 lightDiffuseColor;
uniform int useTexture;
uniform sampler2D _texture;
varying vec4 worldPosition;
varying vec3 worldNormal;
varying vec4 outColor;
varying vec2 outTexCoord;

void main()
{
   vec3 normal = normalize(worldNormal);
   vec3 position = (worldPosition.xyz - worldPosition.w);
   vec3 lightVector = normalize(lightPosition);
  lightVector.y = -lightVector.y;
   vec4 fragColor = outColor;
  if(useTexture == 1)
  {
     fragColor = texture2D(_texture, outTexCoord);
  }
   vec4 ambient = (fragColor * lightAmbientColor);
   vec4 diffuse = ((fragColor * lightDiffuseColor) * max(0.000000, dot(normal, lightVector)));
  gl_FragColor = (ambient + diffuse);
}
)GLSL"},
	{"3d_dilation.bin",
	 R"GLSL(//gl_100
#ifdef GL_ES
precision mediump float;
precision lowp int;
#endif
//Input Layout:
//vec4 inPosition
//vec2 inTexCoord
attribute vec4 inPosition;
attribute vec2 inTexCoord;
varying vec2 outTexCoord;

void main()
{
  outTexCoord = inTexCoord;
  gl_Position = inPosition;
}
)GLSL",
	 R"GLSL(//gl_100
#ifdef GL_ES
precision mediump float;
precision lowp int;
#endif
uniform float offset;
uniform float size;
uniform sampler2D _texture;
varying vec2 outTexCoord;

void main()
{
   vec2 center = outTexCoord;
   vec4 fragColor = texture2D(_texture, center);
  if((((((outTexCoord.x * size) <= (size - 1.000000) && (outTexCoord.x * size) >= 1.000000) && (outTexCoord.y * size) <= (size - 1.000000)) && (outTexCoord.y * size) >= 1.000000) && fragColor.w < 1.000000))
  {
      vec4 c1 = texture2D(_texture, (center + vec2(0.000000, -offset)));
      vec4 c2 = texture2D(_texture, (center + vec2(-offset, 0.000000)));
      vec4 c3 = texture2D(_texture, (center + vec2(offset, 0.000000)));
      vec4 c4 = texture2D(_texture, (center + vec2(0.000000, offset)));
     if(c1.w == 1.000000)
     {
        fragColor = c1;
     }
     else
     {
        if(c2.w == 1.000000)
        {
           fragColor = c2;
        }
        else
        {
           if(c3.w == 1.000000)
           {
              fragColor = c3;
           }
           else
           {
              if(c4.w == 1.000000)
              {
                 fragColor = c4;
              }
           }
        }
     }
  }
  gl_FragColor = fragColor;
}
)GLSL"},
	{"3d_edge.bin",
	 R"GLSL(//gl_100
#ifdef GL_ES
precision mediump float;
precision lowp int;
#endif
//Input Layout:
//vec4 inPosition
//vec2 inTexCoord
attribute vec4 inPosition;
attribute vec2 inTexCoord;
varying vec2 outTexCoord;

void main()
{
  outTexCoord = inTexCoord;
  gl_Position = inPosition;
}
)GLSL",
	 R"GLSL(//gl_100
#ifdef GL_ES
precision mediump float;
precision lowp int;
#endif
uniform float offset;
uniform float size;
uniform float threshold;
uniform vec4 contourColor;
uniform sampler2D _texture;
varying vec2 outTexCoord;

void main()
{
   vec2 center = outTexCoord.xy;
   vec4 edge = vec4(0.000000, 0.000000, 0.000000, 0.000000);
  if(((((outTexCoord.x * size) <= (size - 1.000000) && (outTexCoord.x * size) >= 1.000000) && (outTexCoord.y * size) <= (size - 1.000000)) && (outTexCoord.y * size) >= 1.000000))
  {
     edge += (texture2D(_texture, (center + vec2(-offset, -offset))) * -1.000000);
     edge += (texture2D(_texture, (center + vec2(0.000000, -offset))) * -1.000000);
     edge += (texture2D(_texture, (center + vec2(offset, -offset))) * -1.000000);
     edge += (texture2D(_texture, (center + vec2(-offset, 0.000000))) * -1.000000);
     edge += (texture2D(_texture, center) * 8.000000);
     edge += (texture2D(_texture, (center + vec2(offset, 0.000000))) * -1.000000);
     edge += (texture2D(_texture, (center + vec2(-offset, offset))) * -1.000000);
     edge += (texture2D(_texture, (center + vec2(0.000000, offset))) * -1.000000);
     edge += (texture2D(_texture, (center + vec2(offset, offset))) * -1.000000);
  }
  if((sqrt((((edge.x * edge.x) + (edge.y * edge.y)) + (edge.z * edge.z))) > threshold))
  {
     gl_FragColor = contourColor;
  }
  else
  {
     gl_FragColor = vec4(0.000000, 0.000000, 0.000000, 0.000000);
  }
}
)GLSL"},
	{"3d_geomonly.bin",
	 R"GLSL(//gl_100
#ifdef GL_ES
precision mediump float;
precision lowp int;
#endif
//Input Layout:
//vec4 inPosition
//vec3 inNormal
//vec4 inColor
//vec2 inTexCoord
//vec4 inBoneIndices
//vec4 inBoneWeights
uniform mat4 modelMatrix;
uniform mat4 modelViewProjectionMatrix;
uniform mat4 boneMatrices[60];
attribute vec4 inPosition;
attribute vec3 inNormal;
attribute vec4 inColor;
attribute vec2 inTexCoord;
attribute vec4 inBoneIndices;
attribute vec4 inBoneWeights;
varying vec4 worldPosition;
varying vec3 worldNormal;
varying vec4 outColor;
varying vec2 outTexCoord;

void main()
{
   vec4 boneWeights = inBoneWeights;
  boneWeights.w = (1.000000 - dot(boneWeights.xyz, vec3(1.000000, 1.000000, 1.000000)));
   mat4 transformMatrix = (boneWeights.x * boneMatrices[int(inBoneIndices.x)]);
  transformMatrix += (boneWeights.y * boneMatrices[int(inBoneIndices.y)]);
  transformMatrix += (boneWeights.z * boneMatrices[int(inBoneIndices.z)]);
  transformMatrix += (boneWeights.w * boneMatrices[int(inBoneIndices.w)]);
   vec4 newPosition = (transformMatrix * inPosition);
   vec4 newNormal = (transformMatrix * vec4(inNormal, 0.000000));
  worldNormal = (modelMatrix * newNormal).xyz;
  worldNormal.y = -worldNormal.y;
  gl_Position = (modelViewProjectionMatrix * newPosition);
  worldPosition = (modelMatrix * newPosition);
  outColor = inColor;
  outTexCoord = inTexCoord;
}
)GLSL",
	 R"GLSL(//gl_100
#ifdef GL_ES
precision mediump float;
precision lowp int;
#endif
varying vec4 worldPosition;
varying vec3 worldNormal;
varying vec4 outColor;
varying vec2 outTexCoord;

void main()
{
  gl_FragColor = vec4(1.000000, 0.000000, 0.000000, 1.000000);
}
)GLSL"},
	{"3d_normal.bin",
	 R"GLSL(//gl_100
#ifdef GL_ES
precision mediump float;
precision lowp int;
#endif
//Input Layout:
//vec4 inPosition
//vec3 inNormal
//vec4 inColor
//vec2 inTexCoord
//vec4 inBoneIndices
//vec4 inBoneWeights
uniform mat4 modelMatrix;
uniform mat4 modelViewProjectionMatrix;
uniform mat4 boneMatrices[60];
attribute vec4 inPosition;
attribute vec3 inNormal;
attribute vec4 inColor;
attribute vec2 inTexCoord;
attribute vec4 inBoneIndices;
attribute vec4 inBoneWeights;
varying vec4 worldPosition;
varying vec3 worldNormal;
varying vec4 outColor;
varying vec2 outTexCoord;

void main()
{
   vec4 boneWeights = inBoneWeights;
  boneWeights.w = (1.000000 - dot(boneWeights.xyz, vec3(1.000000, 1.000000, 1.000000)));
   mat4 transformMatrix = (boneWeights.x * boneMatrices[int(inBoneIndices.x)]);
  transformMatrix += (boneWeights.y * boneMatrices[int(inBoneIndices.y)]);
  transformMatrix += (boneWeights.z * boneMatrices[int(inBoneIndices.z)]);
  transformMatrix += (boneWeights.w * boneMatrices[int(inBoneIndices.w)]);
   vec4 newPosition = (transformMatrix * inPosition);
   vec4 newNormal = (transformMatrix * vec4(inNormal, 0.000000));
  worldNormal = (modelMatrix * newNormal).xyz;
  worldNormal.y = -worldNormal.y;
  gl_Position = (modelViewProjectionMatrix * newPosition);
  worldPosition = (modelMatrix * newPosition);
  outColor = inColor;
  outTexCoord = inTexCoord;
}
)GLSL",
	 R"GLSL(//gl_100
#ifdef GL_ES
precision mediump float;
precision lowp int;
#endif
varying vec4 worldPosition;
varying vec3 worldNormal;
varying vec4 outColor;
varying vec2 outTexCoord;

void main()
{
  gl_FragColor.xyz = normalize(worldNormal);
  gl_FragColor.w = 1.000000;
}
)GLSL"},
	{"3d_shadowproj.bin",
	 R"GLSL(//gl_100
#ifdef GL_ES
precision mediump float;
precision lowp int;
#endif
//Input Layout:
//vec4 inPosition
//vec2 inTexCoord
uniform mat4 modelViewProjectionMatrix;
attribute vec4 inPosition;
attribute vec2 inTexCoord;
varying vec2 outTexCoord;

void main()
{
  outTexCoord = inTexCoord;
  gl_Position = (modelViewProjectionMatrix * inPosition);
}
)GLSL",
	 R"GLSL(//gl_100
#ifdef GL_ES
precision mediump float;
precision lowp int;
#endif
uniform sampler2D _texture;
uniform float shadowVisibility;
varying vec2 outTexCoord;

void main()
{
   float translucency = (1.000000 - texture2D(_texture, outTexCoord).x);
  gl_FragColor = vec4(0.000000, 0.000000, 0.000000, clamp((shadowVisibility - translucency), 0.000000, 1.000000));
}
)GLSL"},
	{"3d_toon.bin",
	 R"GLSL(//gl_100
#ifdef GL_ES
precision mediump float;
precision lowp int;
#endif
//Input Layout:
//vec4 inPosition
//vec3 inNormal
//vec4 inColor
//vec2 inTexCoord
//vec4 inBoneIndices
//vec4 inBoneWeights
uniform mat4 modelMatrix;
uniform mat4 modelViewProjectionMatrix;
uniform mat4 boneMatrices[60];
attribute vec4 inPosition;
attribute vec3 inNormal;
attribute vec4 inColor;
attribute vec2 inTexCoord;
attribute vec4 inBoneIndices;
attribute vec4 inBoneWeights;
varying vec4 worldPosition;
varying vec3 worldNormal;
varying vec4 outColor;
varying vec2 outTexCoord;

void main()
{
   vec4 boneWeights = inBoneWeights;
  boneWeights.w = (1.000000 - dot(boneWeights.xyz, vec3(1.000000, 1.000000, 1.000000)));
   mat4 transformMatrix = (boneWeights.x * boneMatrices[int(inBoneIndices.x)]);
  transformMatrix += (boneWeights.y * boneMatrices[int(inBoneIndices.y)]);
  transformMatrix += (boneWeights.z * boneMatrices[int(inBoneIndices.z)]);
  transformMatrix += (boneWeights.w * boneMatrices[int(inBoneIndices.w)]);
   vec4 newPosition = (transformMatrix * inPosition);
   vec4 newNormal = (transformMatrix * vec4(inNormal, 0.000000));
  worldNormal = (modelMatrix * newNormal).xyz;
  worldNormal.y = -worldNormal.y;
  gl_Position = (modelViewProjectionMatrix * newPosition);
  worldPosition = (modelMatrix * newPosition);
  outColor = inColor;
  outTexCoord = inTexCoord;
}
)GLSL",
	 R"GLSL(//gl_100
#ifdef GL_ES
precision mediump float;
precision lowp int;
#endif
uniform vec3 lightPosition;
uniform vec4 lightAmbientColor;
uniform vec4 lightDiffuseColor;
uniform int useTexture;
uniform sampler2D _texture;
uniform sampler2D toonTexture;
varying vec4 worldPosition;
varying vec3 worldNormal;
varying vec4 outColor;
varying vec2 outTexCoord;

void main()
{
   vec3 normal = normalize(worldNormal);
   vec3 position = (worldPosition.xyz / worldPosition.w);
   vec3 lightVector = normalize(lightPosition);
  lightVector.y = -lightVector.y;
   vec4 fragColor = outColor;
  if(useTexture == 1)
  {
     fragColor = texture2D(_texture, outTexCoord);
  }
   vec4 ambient = (fragColor * lightAmbientColor);
   vec4 diffuse = ((fragColor * lightDiffuseColor) * texture2D(toonTexture, vec2(max(0.000000, dot(normal, lightVector)), 0.000000)));
  gl_FragColor = (ambient + diffuse);
  gl_FragColor.w = 1.000000;
}
)GLSL"},
	{"bicubic.bin",
	 R"GLSL(//gl_100
#ifdef GL_ES
precision mediump float;
precision lowp int;
#endif
//Input Layout:
//vec2 position
//vec2 uv
uniform mat4 mvp_mat;
uniform sampler2D texel;
uniform float depth;
attribute vec2 position;
attribute vec2 uv;
varying vec2 uvVarying;

void main()
{
  gl_Position = (mvp_mat * vec4(position.x, position.y, depth, 1.000000));
  uvVarying = uv;
}
)GLSL",
	 R"GLSL(//gl_100
#ifdef GL_ES
precision mediump float;
precision lowp int;
#endif
uniform vec4 color;
uniform sampler2D texel;
uniform sampler2D kernelTex;
uniform vec2 resolution;
varying vec2 uvVarying;

vec4 cubicFilter(float xValue, vec4 c0, vec4 c1, vec4 c2, vec4 c3)
{
   vec4 h = texture2D(kernelTex, vec2(xValue, 0.000000));
   vec4 r = (c0 * h.x);
  r += (c1 * h.y);
  r += (c2 * h.z);
  r += (c3 * h.w);
  return r;
}
float nrand(vec2 n)
{
  return fract((sin(dot(n.xy, vec2(12.989800, 78.233000))) * 43758.545300));
}
vec4 texRECT_bicubic(vec2 uv)
{
   vec2 f = fract((uv * resolution));
   vec2 t = (floor((uv * resolution)) / resolution);
   vec4 t0 = cubicFilter(f.x, texture2D(texel, (t + (vec2(-1.000000, -1.000000) / resolution))), texture2D(texel, (t + (vec2(0.000000, -1.000000) / resolution))), texture2D(texel, (t + (vec2(1.000000, -1.000000) / resolution))), texture2D(texel, (t + (vec2(2.000000, -1.000000) / resolution))));
   vec4 t1 = cubicFilter(f.x, texture2D(texel, (t + (vec2(-1.000000, 0.000000) / resolution))), texture2D(texel, (t + (vec2(0.000000, 0.000000) / resolution))), texture2D(texel, (t + (vec2(1.000000, 0.000000) / resolution))), texture2D(texel, (t + (vec2(2.000000, 0.000000) / resolution))));
   vec4 t2 = cubicFilter(f.x, texture2D(texel, (t + (vec2(-1.000000, 1.000000) / resolution))), texture2D(texel, (t + (vec2(0.000000, 1.000000) / resolution))), texture2D(texel, (t + (vec2(1.000000, 1.000000) / resolution))), texture2D(texel, (t + (vec2(2.000000, 1.000000) / resolution))));
   vec4 t3 = cubicFilter(f.x, texture2D(texel, (t + (vec2(-1.000000, 2.000000) / resolution))), texture2D(texel, (t + (vec2(0.000000, 2.000000) / resolution))), texture2D(texel, (t + (vec2(1.000000, 2.000000) / resolution))), texture2D(texel, (t + (vec2(2.000000, 2.000000) / resolution))));
   vec4 erg = cubicFilter(f.y, t0, t1, t2, t3);
   vec4 blur = (((((((((((((((texture2D(texel, (t + (vec2(-1.000000, -1.000000) / resolution))) + texture2D(texel, (t + (vec2(0.000000, -1.000000) / resolution)))) + texture2D(texel, (t + (vec2(1.000000, -1.000000) / resolution)))) + texture2D(texel, (t + (vec2(2.000000, -1.000000) / resolution)))) + texture2D(texel, (t + (vec2(-1.000000, 0.000000) / resolution)))) + texture2D(texel, (t + (vec2(0.000000, 0.000000) / resolution)))) + texture2D(texel, (t + (vec2(1.000000, 0.000000) / resolution)))) + texture2D(texel, (t + (vec2(2.000000, 0.000000) / resolution)))) + texture2D(texel, (t + (vec2(-1.000000, 1.000000) / resolution)))) + texture2D(texel, (t + (vec2(0.000000, 1.000000) / resolution)))) + texture2D(texel, (t + (vec2(1.000000, 1.000000) / resolution)))) + texture2D(texel, (t + (vec2(2.000000, 1.000000) / resolution)))) + texture2D(texel, (t + (vec2(-1.000000, 2.000000) / resolution)))) + texture2D(texel, (t + (vec2(0.000000, 2.000000) / resolution)))) + texture2D(texel, (t + (vec2(1.000000, 2.000000) / resolution)))) + texture2D(texel, (t + (vec2(2.000000, 2.000000) / resolution))));
  return (erg + ((erg - (blur / 16.000000)) * 0.600000));
}
void main()
{
  gl_FragColor = (texRECT_bicubic(uvVarying) * color);
}
)GLSL"},
	{"color.bin",
	 R"GLSL(//gl_100
#ifdef GL_ES
precision mediump float;
precision lowp int;
#endif
//Input Layout:
//vec2 position
uniform mat4 mvp_mat;
uniform float depth;
attribute vec2 position;

void main()
{
  gl_Position = (mvp_mat * vec4(position.x, position.y, depth, 1.000000));
}
)GLSL",
	 R"GLSL(//gl_100
#ifdef GL_ES
precision mediump float;
precision lowp int;
#endif
uniform vec4 color;

void main()
{
  gl_FragColor = color;
}
)GLSL"},
	{"color2.bin",
	 R"GLSL(//gl_100
#ifdef GL_ES
precision mediump float;
precision lowp int;
#endif
//Input Layout:
//vec2 position
//vec4 color
uniform mat4 mvp_mat;
attribute vec2 position;
attribute vec4 color;
varying vec4 color_;

void main()
{
  gl_Position = (mvp_mat * vec4(position.x, position.y, 0.000000, 1.000000));
  color_ = (color / 255.000000);
}
)GLSL",
	 R"GLSL(//gl_100
#ifdef GL_ES
precision mediump float;
precision lowp int;
#endif
varying vec4 color_;

void main()
{
  gl_FragColor = color_;
}
)GLSL"},
	{"colorinterpolate.bin",
	 R"GLSL(//gl_100
#ifdef GL_ES
precision mediump float;
precision lowp int;
#endif
//Input Layout:
//vec2 position
//vec2 interpolate
uniform mat4 mvp_mat;
attribute vec2 position;
attribute vec2 interpolate;
varying vec2 _interpolate;

void main()
{
  gl_Position = (mvp_mat * vec4(position.x, position.y, 0.000000, 1.000000));
  _interpolate = interpolate;
}
)GLSL",
	 R"GLSL(//gl_100
#ifdef GL_ES
precision mediump float;
precision lowp int;
#endif
uniform vec3 color;
varying vec2 _interpolate;

void main()
{
  gl_FragColor = vec4(mix(mix(vec3(1.000000, 1.000000, 1.000000), vec3(0.000000, 0.000000, 0.000000), _interpolate.y), mix(color, vec3(0.000000, 0.000000, 0.000000), _interpolate.y), _interpolate.x), 1.000000);
}
)GLSL"},
	{"commitShift.bin",
	 R"GLSL(//gl_100
#ifdef GL_ES
precision mediump float;
precision lowp int;
#endif
//Input Layout:
//vec2 position
//vec2 uv
uniform mat4 mvp_mat;
uniform sampler2D texel;
uniform sampler2D texel2;
uniform float progress;
attribute vec2 position;
attribute vec2 uv;
varying vec2 uvVarying;

void main()
{
  gl_Position = (mvp_mat * vec4(position.x, position.y, 0.000000, 1.000000));
  uvVarying = (uv + vec2(progress, 0.000000));
}
)GLSL",
	 R"GLSL(//gl_100
#ifdef GL_ES
precision mediump float;
precision lowp int;
#endif
uniform vec4 color;
uniform sampler2D texel;
uniform sampler2D texel2;
uniform float progress;
varying vec2 uvVarying;

void main()
{
  if(uvVarying.x >= 1.000000)
  {
     gl_FragColor = (texture2D(texel2, (uvVarying - vec2(1.000000, 0.000000))) * color);
  }
  else
  {
     gl_FragColor = (texture2D(texel, uvVarying) * color);
  }
}
)GLSL"},
	{"depth.bin",
	 R"GLSL(//gl_100
#ifdef GL_ES
precision mediump float;
precision lowp int;
#endif
//Input Layout:
//vec2 position
//vec2 uv
uniform mat4 mvp_mat;
uniform sampler2D texel;
uniform float depth;
attribute vec2 position;
attribute vec2 uv;
varying vec2 uvVarying;

void main()
{
   vec4 pos = (mvp_mat * vec4(position.x, position.y, depth, 1.000000));
  pos.z = depth;
  gl_Position = pos;
  uvVarying = uv;
}
)GLSL",
	 R"GLSL(//gl_100
#ifdef GL_ES
precision mediump float;
precision lowp int;
#endif
uniform sampler2D texel;
varying vec2 uvVarying;

void main()
{
  gl_FragColor = texture2D(texel, uvVarying);
}
)GLSL"},
	{"hq2x.bin",
	 R"GLSL(//gl_100
#ifdef GL_ES
precision mediump float;
precision lowp int;
#endif
//Input Layout:
//vec2 position
//vec2 uv
uniform mat4 mvp_mat;
uniform vec2 resolution;
uniform float depth;
attribute vec2 position;
attribute vec2 uv;
varying vec4 vTexCoord;
varying vec4 vTexCoord1;
varying vec4 vTexCoord2;
varying vec4 vTexCoord3;

void main()
{
  gl_Position = (mvp_mat * vec4(position.x, position.y, depth, 1.000000));
   vec2 ps = (1.000000 / resolution);
   float dx = ps.x;
   float dy = ps.y;
  vTexCoord.zw = ps;
  vTexCoord.xy = uv.xy;
  vTexCoord1 = (uv.xxxy + vec4(-dx, 0.000000, dx, -dy));
  vTexCoord2 = (uv.xxxy + vec4(-dx, 0.000000, dx, 0.000000));
  vTexCoord3 = (uv.xxxy + vec4(-dx, 0.000000, dx, dy));
}
)GLSL",
	 R"GLSL(//gl_100
#ifdef GL_ES
precision mediump float;
precision lowp int;
#endif
uniform sampler2D tex;
uniform sampler2D LUT;
uniform vec2 resolution;
varying vec4 vTexCoord;
varying vec4 vTexCoord1;
varying vec4 vTexCoord2;
varying vec4 vTexCoord3;

bool diff(vec3 yuv1, vec3 yuv2)
{
   bvec3 res = greaterThan(abs(((yuv1 + vec3(0.000000, 0.500000, 0.500000)) - (yuv2 + vec3(0.000000, 0.500000, 0.500000)))), vec3(0.188235, 0.027451, 0.023529));
  return ((res.x || res.y) || res.z);
}
void main()
{
   vec2 fp = fract((vTexCoord.xy * resolution));
   vec2 quad = sign((-0.500000 + fp));
   vec2 fraction = (fract((vTexCoord.xy * resolution)) / resolution);
   vec2 uv = (vTexCoord.xy - fraction);
   float dx = vTexCoord.w;
   float dy = vTexCoord.z;
   vec4 p1 = texture2D(tex, uv).xyzw;
   vec4 p2 = texture2D(tex, (uv + (vec2(dx, dy) * quad))).xyzw;
   vec4 p3 = texture2D(tex, (uv + (vec2(dx, 0.000000) * quad))).xyzw;
   vec4 p4 = texture2D(tex, (uv + (vec2(0.000000, dy) * quad))).xyzw;
   vec3 w1 = (mat3(0.299000, -0.169000, 0.500000, 0.587000, -0.331000, -0.419000, 0.114000, 0.500000, -0.081000) * texture2D(tex, (vTexCoord1.xw - fraction)).xyz);
   vec3 w2 = (mat3(0.299000, -0.169000, 0.500000, 0.587000, -0.331000, -0.419000, 0.114000, 0.500000, -0.081000) * texture2D(tex, (vTexCoord1.yw - fraction)).xyz);
   vec3 w3 = (mat3(0.299000, -0.169000, 0.500000, 0.587000, -0.331000, -0.419000, 0.114000, 0.500000, -0.081000) * texture2D(tex, (vTexCoord1.zw - fraction)).xyz);
   vec3 w4 = (mat3(0.299000, -0.169000, 0.500000, 0.587000, -0.331000, -0.419000, 0.114000, 0.500000, -0.081000) * texture2D(tex, (vTexCoord2.xw - fraction)).xyz);
   vec3 w5 = (mat3(0.299000, -0.169000, 0.500000, 0.587000, -0.331000, -0.419000, 0.114000, 0.500000, -0.081000) * p1.xyz);
   vec3 w6 = (mat3(0.299000, -0.169000, 0.500000, 0.587000, -0.331000, -0.419000, 0.114000, 0.500000, -0.081000) * texture2D(tex, (vTexCoord2.zw - fraction)).xyz);
   vec3 w7 = (mat3(0.299000, -0.169000, 0.500000, 0.587000, -0.331000, -0.419000, 0.114000, 0.500000, -0.081000) * texture2D(tex, (vTexCoord3.xw - fraction)).xyz);
   vec3 w8 = (mat3(0.299000, -0.169000, 0.500000, 0.587000, -0.331000, -0.419000, 0.114000, 0.500000, -0.081000) * texture2D(tex, (vTexCoord3.yw - fraction)).xyz);
   vec3 w9 = (mat3(0.299000, -0.169000, 0.500000, 0.587000, -0.331000, -0.419000, 0.114000, 0.500000, -0.081000) * texture2D(tex, (vTexCoord3.zw - fraction)).xyz);
  bvec3 pattern[3];
  pattern[0] = bvec3(diff(w5, w1), diff(w5, w2), diff(w5, w3));
  pattern[1] = bvec3(diff(w5, w4), false, diff(w5, w6));
  pattern[2] = bvec3(diff(w5, w7), diff(w5, w8), diff(w5, w9));
   bvec4 cross = bvec4(diff(w4, w2), diff(w2, w6), diff(w8, w4), diff(w6, w8));
  vec2 index;
  index.x = ((dot(vec3(pattern[0]), vec3(1.000000, 2.000000, 4.000000)) + dot(vec3(pattern[1]), vec3(8.000000, 0.000000, 16.000000))) + dot(vec3(pattern[2]), vec3(32.000000, 64.000000, 128.000000)));
  index.y = ((dot(vec4(cross), vec4(1.000000, 2.000000, 4.000000, 8.000000)) * 4.000000) + dot(floor((fp * 2.000000)), vec2(1.000000, 2.000000)));
   vec2 step = vec2(0.003906, 0.015625);
   vec2 offset = (step / 2.000000);
   vec4 weights = texture2D(LUT, ((index * step) + offset));
   float sum = dot(weights, vec4(1.000000, 1.000000, 1.000000, 1.000000));
  weights /= sum;
   vec4 res = ((((p1 * weights.x) + (p2 * weights.y)) + (p3 * weights.z)) + (p4 * weights.w));
  gl_FragColor.xyzw = res;
}
)GLSL"},
	{"lightmap.bin",
	 R"GLSL(//gl_100
#ifdef GL_ES
precision mediump float;
precision lowp int;
#endif
//Input Layout:
//vec2 position
//vec2 uv
uniform mat4 mvp_mat;
attribute vec2 position;
attribute vec2 uv;
varying vec2 texcoord;

void main()
{
  gl_Position = (mvp_mat * vec4(position.x, position.y, 0.000000, 1.000000));
  texcoord = uv;
}
)GLSL",
	 R"GLSL(//gl_100
#ifdef GL_ES
precision mediump float;
precision lowp int;
#endif
uniform sampler2D source;
uniform sampler2D lightmap;
uniform vec4 lmrect;
uniform vec4 srect;
uniform vec2 surfacefactor;
uniform vec2 texturePos;
varying vec2 texcoord;

void main()
{
  gl_FragColor = (texture2D(source, texcoord) * texture2D(lightmap, vec2((lmrect.x + (((texcoord.x - srect.x) / texturePos.x) * surfacefactor.x)), ((lmrect.y + lmrect.w) - (((texcoord.y - srect.y) / texturePos.y) * surfacefactor.y)))));
}
)GLSL"},
	{"particleHaduken.bin",
	 R"GLSL(//gl_100
#ifdef GL_ES
precision mediump float;
precision lowp int;
#endif
//Input Layout:
//vec2 position
//vec2 uv
//vec4 color
//vec2 center
//float rotation
//float scale
uniform mat4 mvp_mat;
attribute vec2 position;
attribute vec2 uv;
attribute vec4 color;
attribute vec2 center;
attribute float rotation;
attribute float scale;
varying vec2 uvVarying;
varying vec4 color_;

void main()
{
   mat2 rot = mat2(vec2(cos(rotation), -sin(rotation)), vec2(sin(rotation), cos(rotation)));
  gl_Position = (mvp_mat * vec4(((rot * ((position - center) * scale)) + center), 0.000000, 1.000000));
  uvVarying = uv;
  color_ = (color / 255.000000);
}
)GLSL",
	 R"GLSL(//gl_100
#ifdef GL_ES
precision mediump float;
precision lowp int;
#endif
uniform sampler2D texel;
varying vec2 uvVarying;
varying vec4 color_;

void main()
{
  gl_FragColor = (texture2D(texel, uvVarying) * color_);
}
)GLSL"},
	{"particles.bin",
	 R"GLSL(//gl_100
#ifdef GL_ES
precision mediump float;
precision lowp int;
#endif
//Input Layout:
//vec3 position
//vec2 uv
//vec4 color
uniform mat4 p_mat;
uniform mat4 mv_mat;
uniform mat4 t_mat;
attribute vec3 position;
attribute vec2 uv;
attribute vec4 color;
varying vec2 uvVarying;
varying vec4 color_;

void main()
{
  gl_Position = (p_mat * (mv_mat * (t_mat * vec4(position, 1.000000))));
  uvVarying = uv;
  color_ = (color / 255.000000);
}
)GLSL",
	 R"GLSL(//gl_100
#ifdef GL_ES
precision mediump float;
precision lowp int;
#endif
uniform sampler2D texel;
varying vec2 uvVarying;
varying vec4 color_;

void main()
{
  gl_FragColor = (texture2D(texel, uvVarying) * color_);
}
)GLSL"},
	{"revAA.bin",
	 R"GLSL(//gl_100
#ifdef GL_ES
precision mediump float;
precision lowp int;
#endif
//Input Layout:
//vec2 position
//vec2 uv
uniform mat4 mvp_mat;
uniform vec2 resolution;
uniform float depth;
attribute vec2 position;
attribute vec2 uv;
varying vec2 uvVarying;
varying vec4 t1;
varying vec4 t2;
varying vec4 t3;
varying vec4 t4;
varying vec4 t5;
varying vec4 t6;
varying vec4 t7;

void main()
{
  gl_Position = (mvp_mat * vec4(position.x, position.y, depth, 1.000000));
   vec2 ps = vec2((1.000000 / resolution.x), (1.000000 / resolution.y));
   float dx = ps.x;
   float dy = ps.y;
  uvVarying = uv;
  t1 = (uv.xxxy + vec4(-dx, 0.000000, dx, (-2.000000 * dy)));
  t2 = (uv.xxxy + vec4(-dx, 0.000000, dx, -dy));
  t3 = (uv.xxxy + vec4(-dx, 0.000000, dx, 0.000000));
  t4 = (uv.xxxy + vec4(-dx, 0.000000, dx, dy));
  t5 = (uv.xxxy + vec4(-dx, 0.000000, dx, (2.000000 * dy)));
  t6 = (uv.xyyy + vec4((-2.000000 * dx), -dy, 0.000000, dy));
  t7 = (uv.xyyy + vec4((2.000000 * dx), -dy, 0.000000, dy));
}
)GLSL",
	 R"GLSL(//gl_100
#ifdef GL_ES
precision mediump float;
precision lowp int;
#endif
uniform sampler2D tex;
uniform vec2 resolution;
uniform vec2 bias;
varying vec2 uvVarying;
varying vec4 t1;
varying vec4 t2;
varying vec4 t3;
varying vec4 t4;
varying vec4 t5;
varying vec4 t6;
varying vec4 t7;

vec4 res2x(vec4 pre2, vec4 pre1, vec4 px, vec4 pos1, vec4 pos2)
{
  vec4 df[4];
  df[0] = (pre1 - pre2);
  df[1] = (px - pre1);
  df[2] = (pos1 - px);
  df[3] = (pos2 - pos1);
  vec4 m;
  m.x = (px.x < 0.500000 ? px.x : (1.000000 - px.x));
  m.y = (px.y < 0.500000 ? px.y : (1.000000 - px.y));
  m.z = (px.z < 0.500000 ? px.z : (1.000000 - px.z));
  m.w = (px.w < 0.500000 ? px.w : (1.000000 - px.w));
  m = (2.000000 * min(m, min(abs(df[1]), abs(df[2]))));
   vec4 t = (((7.000000 * (df[1] + df[2])) - (3.000000 * (df[0] + df[3]))) / 16.000000);
  t = clamp(t, -m, m);
  return t;
}
void main()
{
   vec2 fp = ((2.000000 * fract(((uvVarying * resolution) + vec2(0.500000, 0.500000)))) - 1.000000);
   vec2 fr = (fract(((uvVarying * resolution) + vec2(0.500000, 0.500000))) / resolution);
  fp *= 1.000000;
   vec4 B1 = texture2D(tex, (t1.yw - fr)).xyzw;
   vec4 B = texture2D(tex, (t2.yw - fr)).xyzw;
   vec4 D = texture2D(tex, (t3.xw - fr)).xyzw;
   vec4 E = texture2D(tex, (t3.yw - fr)).xyzw;
   vec4 F = texture2D(tex, (t3.zw - fr)).xyzw;
   vec4 H = texture2D(tex, (t4.yw - fr)).xyzw;
   vec4 H5 = texture2D(tex, (t5.yw - fr)).xyzw;
   vec4 D0 = texture2D(tex, (t6.xz - fr)).xyzw;
   vec4 F4 = texture2D(tex, (t7.xz - fr)).xyzw;
   vec4 tilt = res2x(B1, B, E, H, H5);
   vec4 s1 = (E + (tilt / 2.000000));
   vec4 s0 = (s1 - tilt);
  tilt = res2x(D0, D, s0, F, F4);
   vec4 e1 = (s0 + (tilt / 2.000000));
   vec4 e0 = (e1 - tilt);
  tilt = res2x(D0, D, s1, F, F4);
   vec4 e3 = (s1 + (tilt / 2.000000));
   vec4 e2 = (e3 - tilt);
  gl_FragColor = ((fp.y > 0.000000) ? ((fp.x > 0.000000) ? e3 : e2) : ((fp.x > 0.000000) ? e1 : e0));
}
)GLSL"},
	{"sht_ascii.bin",
	 R"GLSL(//gl_100
#ifdef GL_ES
precision mediump float;
precision lowp int;
#endif
//Input Layout:
//vec2 position
//vec2 uv
uniform mat4 mvp_mat;
uniform int renderpass;
attribute vec2 position;
attribute vec2 uv;
varying vec2 texcoord;

void main()
{
  gl_Position = (mvp_mat * vec4(position.x, position.y, 0.000000, 1.000000));
  texcoord = uv;
}
)GLSL",
	 R"GLSL(//gl_100
#ifdef GL_ES
precision mediump float;
precision lowp int;
#endif
uniform float time;
uniform sampler2D composite;
uniform int renderpass;
uniform vec2 mouse;
uniform vec2 resolution;
varying vec2 texcoord;

float character(float n, vec2 p)
{
  p = floor(((p * vec2(4.000000, -4.000000)) + 2.500000));
  if((clamp(p.x, 0.000000, 4.000000) == p.x && clamp(p.y, 0.000000, 4.000000) == p.y))
  {
     if(int(mod((n / exp2((p.x + (5.000000 * p.y)))), 2.000000)) == 1)
     {
        return 1.000000;
     }
  }
  return 0.000000;
}
void main()
{
   vec3 col = texture2D(composite, ((floor(((texcoord.xy * resolution) / 8.000000)) * 8.000000) / resolution.xy)).xyz;
   float gray = (((col.x + col.y) + col.z) / 3.000000);
   float n = 65536.000000;
  if((gray > 0.200000))
  {
     n = 65600.000000;
  }
  if((gray > 0.300000))
  {
     n = 332772.000000;
  }
  if((gray > 0.400000))
  {
     n = 15255086.000000;
  }
  if((gray > 0.500000))
  {
     n = 23385164.000000;
  }
  if((gray > 0.600000))
  {
     n = 15252014.000000;
  }
  if((gray > 0.700000))
  {
     n = 13199452.000000;
  }
  if((gray > 0.800000))
  {
     n = 11512810.000000;
  }
   vec2 p = (mod(((texcoord.xy * resolution) / 4.000000), 2.000000) - vec2(1.000000, 1.000000));
  col = (col * character(n, p));
  gl_FragColor = vec4(col, 1.000000);
}
)GLSL"},
	{"sht_blur.bin",
	 R"GLSL(//gl_100
#ifdef GL_ES
precision mediump float;
precision lowp int;
#endif
//Input Layout:
//vec2 position
//vec2 uv
uniform mat4 mvp_mat;
uniform int renderpass;
uniform int firstpass;
uniform int lastpass;
uniform float down;
uniform float upsideDown;
attribute vec2 position;
attribute vec2 uv;
varying vec2 texcoord;

void main()
{
  if(renderpass == lastpass)
  {
     gl_Position = (mvp_mat * vec4(position.x, position.y, 0.000000, 1.000000));
  }
  else
  {
     gl_Position = (mvp_mat * vec4((position.x / down), (position.y / down), 0.000000, 1.000000));
  }
  if((renderpass > firstpass))
  {
      vec2 _uv = vec3(uv, 1.000000).xy;
     if(upsideDown == 1.000000)
     {
        texcoord = vec2((_uv.x / down), (_uv.y / down));
     }
     else
     {
        texcoord = vec2((_uv.x / down), ((_uv.y / down) + (1.000000 - (1.000000 / down))));
     }
  }
  else
  {
     texcoord = vec3(uv, 1.000000).xy;
  }
}
)GLSL",
	 R"GLSL(//gl_100
#ifdef GL_ES
precision mediump float;
precision lowp int;
#endif
uniform sampler2D composite;
uniform int renderpass;
uniform float exposure;
uniform vec2 weights;
uniform int firstpass;
uniform int lastpass;
uniform vec2 resolution;
varying vec2 texcoord;

vec4 identity(vec2 uv)
{
   vec4 col = (exposure * texture2D(composite, uv.xy));
  col.w = 1.000000;
  return col;
}
vec4 blurFast(vec2 uv)
{
   vec4 sum = vec4(0.000000, 0.000000, 0.000000, 0.000000);
   float blurSize = ((1.000000 / resolution.x) * (5.000000 - float((renderpass - firstpass))));
  sum += (texture2D(composite, vec2((uv.x - blurSize), (uv.y - blurSize))) * weights.x);
  sum += (texture2D(composite, vec2((uv.x - blurSize), (uv.y + blurSize))) * weights.x);
  sum += (texture2D(composite, vec2(uv.x, uv.y)) * weights.y);
  sum += (texture2D(composite, vec2((uv.x + blurSize), (uv.y - blurSize))) * weights.x);
  sum += (texture2D(composite, vec2((uv.x + blurSize), (uv.y + blurSize))) * weights.x);
  return sum;
}
void main()
{
  if(renderpass == lastpass)
  {
     gl_FragColor = identity(texcoord);
  }
  else
  {
     gl_FragColor = blurFast(texcoord);
  }
}
)GLSL"},
	{"sht_chroma.bin",
	 R"GLSL(//gl_100
#ifdef GL_ES
precision mediump float;
precision lowp int;
#endif
//Input Layout:
//vec2 position
//vec2 uv
uniform mat4 mvp_mat;
uniform int renderpass;
attribute vec2 position;
attribute vec2 uv;
varying vec2 texcoord;

void main()
{
  gl_Position = (mvp_mat * vec4(position.x, position.y, 0.000000, 1.000000));
  texcoord = uv;
}
)GLSL",
	 R"GLSL(//gl_100
#ifdef GL_ES
precision mediump float;
precision lowp int;
#endif
uniform float time;
uniform sampler2D composite;
uniform int renderpass;
uniform vec2 mouse;
uniform vec2 resolution;
varying vec2 texcoord;

void main()
{
   vec2 uv = texcoord;
   float d = length((uv - vec2(0.500000, 0.500000)));
   float blur = 0.000000;
  blur = ((1.000000 + sin((time * 6.000000))) * 0.500000);
  blur *= (1.000000 + (sin((time * 16.000000)) * 0.500000));
  blur = pow(abs(abs(blur)), 3.000000);
  blur *= 0.050000;
  blur *= d;
  vec3 col;
  col.x = texture2D(composite, vec2((uv.x + blur), uv.y)).x;
  col.y = texture2D(composite, uv).y;
  col.z = texture2D(composite, vec2((uv.x - blur), uv.y)).z;
   float scanline = (sin((uv.y * 800.000000)) * 0.040000);
  col -= scanline;
  col *= (1.000000 - (d * 0.500000));
  gl_FragColor = vec4(col, 1.000000);
}
)GLSL"},
	{"sht_color.bin",
	 R"GLSL(//gl_100
#ifdef GL_ES
precision mediump float;
precision lowp int;
#endif
//Input Layout:
//vec2 position
//vec2 uv
uniform mat4 mvp_mat;
uniform int renderpass;
attribute vec2 position;
attribute vec2 uv;
varying vec2 texcoord;

void main()
{
  gl_Position = (mvp_mat * vec4(position.x, position.y, 0.000000, 1.000000));
  texcoord = uv;
}
)GLSL",
	 R"GLSL(//gl_100
#ifdef GL_ES
precision mediump float;
precision lowp int;
#endif
uniform sampler2D composite;
uniform int renderpass;
uniform float noise;
uniform float noiseFactor;
uniform float iTime;
uniform vec2 weights;
uniform vec4 shader_coeff;
varying vec2 texcoord;

vec4 identity(vec2 uv)
{
  return texture2D(composite, uv.xy);
}
float rand(vec2 co)
{
  return fract((sin(dot(co.xy, vec2(12.989800, 78.233000))) * 43758.545300));
}
vec3 rgb2hsv(vec3 c)
{
   vec4 K = vec4(0.000000, -0.333333, 0.666667, -1.000000);
   vec4 p = mix(vec4(c.zy, K.wz), vec4(c.yz, K.xy), step(c.z, c.y));
   vec4 q = mix(vec4(p.xyw, c.x), vec4(c.x, p.yzx), step(p.x, c.x));
   float d = (q.x - min(q.w, q.y));
   float e = 0.000000;
  return vec3(abs((q.z + ((q.w - q.y) / ((6.000000 * d) + e)))), (d / (q.x + e)), q.x);
}
vec3 hsv2rgb(vec3 c)
{
   vec4 K = vec4(1.000000, 0.666667, 0.333333, 3.000000);
   vec3 p = abs(((fract((c.xxx + K.xyz)) * 6.000000) - K.www));
  return (c.z * mix(K.xxx, clamp((p - K.xxx), 0.000000, 1.000000), c.y));
}
vec4 sat(vec2 uv)
{
   vec4 textureColor = texture2D(composite, uv.xy);
   vec3 fragRGB = textureColor.xyz;
   vec3 fragHSV = rgb2hsv(fragRGB);
  fragHSV.x = clamp(mod((fragHSV.x + shader_coeff.x), 1.000000), 0.000000, 0.990000);
  fragHSV.y *= shader_coeff.y;
  fragHSV.z = clamp(((((fragHSV.z - 0.500000) * shader_coeff.z) + 0.500000) + shader_coeff.w), 0.000000, 1.000000);
  fragRGB = hsv2rgb(fragHSV);
  fragHSV.x = weights.y;
   vec3 fragRGBC = hsv2rgb(fragHSV);
   vec4 color = vec4(mix(fragRGB, fragRGBC, weights.x), textureColor.w);
  if(noise == 1.000000)
  {
     color += (noiseFactor * rand((uv.xy + vec2((iTime * 2.000000), 0.000000))));
  }
  return color;
}
void main()
{
  gl_FragColor = sat(texcoord);
}
)GLSL"},
	{"sht_edgeglow.bin",
	 R"GLSL(//gl_100
#ifdef GL_ES
precision mediump float;
precision lowp int;
#endif
//Input Layout:
//vec2 position
//vec2 uv
uniform mat4 mvp_mat;
uniform int renderpass;
attribute vec2 position;
attribute vec2 uv;
varying vec2 texcoord;

void main()
{
  gl_Position = (mvp_mat * vec4(position.x, position.y, 0.000000, 1.000000));
  texcoord = uv;
}
)GLSL",
	 R"GLSL(//gl_100
#ifdef GL_ES
precision mediump float;
precision lowp int;
#endif
uniform float time;
uniform sampler2D composite;
uniform int renderpass;
uniform vec2 mouse;
uniform vec2 resolution;
varying vec2 texcoord;

float lookup(vec2 p, float dx, float dy, float d)
{
   vec2 uv = ((p.xy + vec2((dx * d), (dy * d))) / resolution.xy);
   vec4 c = texture2D(composite, uv.xy);
  return (((0.212600 * c.x) + (0.715200 * c.y)) + (0.072200 * c.z));
}
void main()
{
   float d = ((sin((time * 5.000000)) * 0.500000) + 1.500000);
   vec2 p = (texcoord * resolution);
   float gx = 0.000000;
  gx += (-1.000000 * lookup(p, -1.000000, -1.000000, d));
  gx += (-2.000000 * lookup(p, -1.000000, 0.000000, d));
  gx += (-1.000000 * lookup(p, -1.000000, 1.000000, d));
  gx += (1.000000 * lookup(p, 1.000000, -1.000000, d));
  gx += (2.000000 * lookup(p, 1.000000, 0.000000, d));
  gx += (1.000000 * lookup(p, 1.000000, 1.000000, d));
   float gy = 0.000000;
  gy += (-1.000000 * lookup(p, -1.000000, -1.000000, d));
  gy += (-2.000000 * lookup(p, 0.000000, -1.000000, d));
  gy += (-1.000000 * lookup(p, 1.000000, -1.000000, d));
  gy += (1.000000 * lookup(p, -1.000000, 1.000000, d));
  gy += (2.000000 * lookup(p, 0.000000, 1.000000, d));
  gy += (1.000000 * lookup(p, 1.000000, 1.000000, d));
   float g = ((gx * gx) + (gy * gy));
   float g2 = (g * ((sin(time) / 2.000000) + 0.500000));
   vec4 col = texture2D(composite, (p / resolution.xy));
  col += vec4(0.000000, g, g2, 1.000000);
  gl_FragColor = col;
}
)GLSL"},
	{"sht_fourbit.bin",
	 R"GLSL(//gl_100
#ifdef GL_ES
precision mediump float;
precision lowp int;
#endif
//Input Layout:
//vec2 position
//vec2 uv
uniform mat4 mvp_mat;
uniform int renderpass;
attribute vec2 position;
attribute vec2 uv;
varying vec2 texcoord;

void main()
{
  gl_Position = (mvp_mat * vec4(position.x, position.y, 0.000000, 1.000000));
  texcoord = uv;
}
)GLSL",
	 R"GLSL(//gl_100
#ifdef GL_ES
precision mediump float;
precision lowp int;
#endif
uniform float time;
uniform sampler2D composite;
uniform int renderpass;
uniform vec2 mouse;
uniform vec2 resolution;
varying vec2 texcoord;

float hash(vec2 p)
{
  return fract(((10000.000000 * sin(((17.000000 * p.x) + (p.y * 0.100000)))) * (0.100000 + abs(sin(((p.y * 13.000000) + p.x))))));
}
float compare(vec3 a, vec3 b)
{
  a = max(vec3(0.000000, 0.000000, 0.000000), (a - (min(a.x, min(a.y, a.z)) * 0.250000)));
  b = max(vec3(0.000000, 0.000000, 0.000000), (b - (min(b.x, min(b.y, b.z)) * 0.250000)));
  a *= (a * a);
  b *= (b * b);
   vec3 diff = (a - b);
  return dot(diff, diff);
}
void main()
{
   vec2 c = floor(((texcoord * resolution) / 4.000000));
   vec2 coord = (c * 4.000000);
   vec3 src = texture2D(composite, (coord / resolution.xy)).xyz;
   vec3 dst0 = vec3(0.000000, 0.000000, 0.000000);
   vec3 dst1 = vec3(0.000000, 0.000000, 0.000000);
   float best0 = 1000.000000;
   float best1 = 1000.000000;
   float err = compare(src, vec3(0.078431, 0.047059, 0.109804));
  if(err < best0)
  {
     best1 = best0;
     dst1 = dst0;
     best0 = err;
     dst0 = vec3(0.078431, 0.047059, 0.109804);
  }
  err = compare(src, vec3(0.266667, 0.141176, 0.203922));
  if(err < best0)
  {
     best1 = best0;
     dst1 = dst0;
     best0 = err;
     dst0 = vec3(0.266667, 0.141176, 0.203922);
  }
  err = compare(src, vec3(0.188235, 0.203922, 0.427451));
  if(err < best0)
  {
     best1 = best0;
     dst1 = dst0;
     best0 = err;
     dst0 = vec3(0.188235, 0.203922, 0.427451);
  }
  err = compare(src, vec3(0.305882, 0.290196, 0.305882));
  if(err < best0)
  {
     best1 = best0;
     dst1 = dst0;
     best0 = err;
     dst0 = vec3(0.305882, 0.290196, 0.305882);
  }
  err = compare(src, vec3(0.521569, 0.298039, 0.188235));
  if(err < best0)
  {
     best1 = best0;
     dst1 = dst0;
     best0 = err;
     dst0 = vec3(0.521569, 0.298039, 0.188235);
  }
  err = compare(src, vec3(0.203922, 0.396078, 0.141176));
  if(err < best0)
  {
     best1 = best0;
     dst1 = dst0;
     best0 = err;
     dst0 = vec3(0.203922, 0.396078, 0.141176);
  }
  err = compare(src, vec3(0.815686, 0.274510, 0.282353));
  if(err < best0)
  {
     best1 = best0;
     dst1 = dst0;
     best0 = err;
     dst0 = vec3(0.815686, 0.274510, 0.282353);
  }
  err = compare(src, vec3(0.458824, 0.443137, 0.380392));
  if(err < best0)
  {
     best1 = best0;
     dst1 = dst0;
     best0 = err;
     dst0 = vec3(0.458824, 0.443137, 0.380392);
  }
  err = compare(src, vec3(0.349020, 0.490196, 0.807843));
  if(err < best0)
  {
     best1 = best0;
     dst1 = dst0;
     best0 = err;
     dst0 = vec3(0.349020, 0.490196, 0.807843);
  }
  err = compare(src, vec3(0.823529, 0.490196, 0.172549));
  if(err < best0)
  {
     best1 = best0;
     dst1 = dst0;
     best0 = err;
     dst0 = vec3(0.823529, 0.490196, 0.172549);
  }
  err = compare(src, vec3(0.521569, 0.584314, 0.631373));
  if(err < best0)
  {
     best1 = best0;
     dst1 = dst0;
     best0 = err;
     dst0 = vec3(0.521569, 0.584314, 0.631373);
  }
  err = compare(src, vec3(0.427451, 0.666667, 0.172549));
  if(err < best0)
  {
     best1 = best0;
     dst1 = dst0;
     best0 = err;
     dst0 = vec3(0.427451, 0.666667, 0.172549);
  }
  err = compare(src, vec3(0.823529, 0.666667, 0.600000));
  if(err < best0)
  {
     best1 = best0;
     dst1 = dst0;
     best0 = err;
     dst0 = vec3(0.823529, 0.666667, 0.600000);
  }
  err = compare(src, vec3(0.427451, 0.760784, 0.792157));
  if(err < best0)
  {
     best1 = best0;
     dst1 = dst0;
     best0 = err;
     dst0 = vec3(0.427451, 0.760784, 0.792157);
  }
  err = compare(src, vec3(0.854902, 0.831373, 0.368627));
  if(err < best0)
  {
     best1 = best0;
     dst1 = dst0;
     best0 = err;
     dst0 = vec3(0.854902, 0.831373, 0.368627);
  }
  err = compare(src, vec3(0.870588, 0.933333, 0.839216));
  if(err < best0)
  {
     best1 = best0;
     dst1 = dst0;
     best0 = err;
     dst0 = vec3(0.870588, 0.933333, 0.839216);
  }
  best0 = sqrt(best0);
  best1 = sqrt(best1);
  gl_FragColor = vec4(((mod((c.x + c.y), 2.000000) > ((hash(((c * 2.000000) + fract(sin(vec2(floor(time), floor((time * 1.700000))))))) * 0.750000) + (best1 / (best0 + best1)))) ? dst1 : dst0), 1.000000);
}
)GLSL"},
	{"sht_glow.bin",
	 R"GLSL(//gl_100
#ifdef GL_ES
precision mediump float;
precision lowp int;
#endif
//Input Layout:
//vec2 position
//vec2 uv
uniform mat4 mvp_mat;
uniform int renderpass;
uniform int firstpass;
uniform int lastpass;
uniform float down;
uniform float upsideDown;
attribute vec2 position;
attribute vec2 uv;
varying vec2 texcoord;

void main()
{
  if(renderpass == lastpass)
  {
     gl_Position = (mvp_mat * vec4(position.x, position.y, 0.000000, 1.000000));
  }
  else
  {
     gl_Position = (mvp_mat * vec4((position.x / down), (position.y / down), 0.000000, 1.000000));
  }
  if((renderpass > firstpass))
  {
      vec2 _uv = vec3(uv, 1.000000).xy;
     if(upsideDown == 1.000000)
     {
        texcoord = vec2((_uv.x / down), (_uv.y / down));
     }
     else
     {
        texcoord = vec2((_uv.x / down), ((_uv.y / down) + (1.000000 - (1.000000 / down))));
     }
  }
  else
  {
     texcoord = vec3(uv, 1.000000).xy;
  }
}
)GLSL",
	 R"GLSL(//gl_100
#ifdef GL_ES
precision mediump float;
precision lowp int;
#endif
uniform sampler2D composite;
uniform int renderpass;
uniform float exposure;
uniform vec2 weights;
uniform int firstpass;
uniform int lastpass;
uniform vec2 resolution;
uniform float down;
varying vec2 texcoord;

vec4 identity(vec2 uv)
{
   vec4 col = (exposure * texture2D(composite, uv.xy));
  col.w = 1.000000;
  return col;
}
vec4 edgeidentity(vec2 uv)
{
   vec4 col = (exposure * texture2D(composite, uv.xy));
   vec2 tc = uv;
  tc.x = (abs((tc.x - (0.500000 / down))) * 2.000000);
  tc.y = (abs((tc.y - (1.000000 - (0.500000 / down)))) * 2.000000);
  col.w = (3.000000 * (tc.x + tc.y));
  return col;
}
vec4 blurFast(vec2 uv)
{
   vec4 sum = vec4(0.000000, 0.000000, 0.000000, 0.000000);
   float blurSize = ((1.000000 / resolution.x) * (5.000000 - float(((renderpass - firstpass) + 1))));
  sum += (texture2D(composite, vec2((uv.x - blurSize), (uv.y - blurSize))) * weights.x);
  sum += (texture2D(composite, vec2((uv.x - blurSize), (uv.y + blurSize))) * weights.x);
  sum += (texture2D(composite, vec2(uv.x, uv.y)) * weights.y);
  sum += (texture2D(composite, vec2((uv.x + blurSize), (uv.y - blurSize))) * weights.x);
  sum += (texture2D(composite, vec2((uv.x + blurSize), (uv.y + blurSize))) * weights.x);
  return sum;
}
void main()
{
  if(renderpass == lastpass)
  {
     gl_FragColor = identity(texcoord);
  }
  else
  {
     if(renderpass == firstpass)
     {
        gl_FragColor = texture2D(composite, texcoord);
     }
     else
     {
        gl_FragColor = blurFast(texcoord);
     }
  }
}
)GLSL"},
	{"sht_glowradial.bin",
	 R"GLSL(//gl_100
#ifdef GL_ES
precision mediump float;
precision lowp int;
#endif
//Input Layout:
//vec2 position
//vec2 uv
uniform mat4 mvp_mat;
uniform int renderpass;
uniform int firstpass;
uniform int lastpass;
uniform float down;
uniform float upsideDown;
attribute vec2 position;
attribute vec2 uv;
varying vec2 texcoord;

void main()
{
  gl_Position = (mvp_mat * vec4(position.x, position.y, 0.000000, 1.000000));
  texcoord = vec3(uv, 1.000000).xy;
}
)GLSL",
	 R"GLSL(//gl_100
#ifdef GL_ES
precision mediump float;
precision lowp int;
#endif
uniform sampler2D composite;
uniform int renderpass;
uniform float exposure;
uniform vec2 weights;
uniform int firstpass;
uniform int lastpass;
uniform vec2 resolution;
uniform float down;
varying vec2 texcoord;

vec4 identity(vec2 uv)
{
   vec4 col = (exposure * texture2D(composite, uv.xy));
  col.w = 1.000000;
  return col;
}
vec4 edgeidentity(vec2 uv)
{
   vec4 col = (exposure * texture2D(composite, uv.xy));
   vec2 tc = uv;
  tc.x = (abs((tc.x - (0.500000 / down))) * 2.000000);
  tc.y = (abs((tc.y - (1.000000 - (0.500000 / down)))) * 2.000000);
  col.w = (3.000000 * (tc.x + tc.y));
  return col;
}
vec4 blurFast(vec2 uv)
{
   vec4 sum = vec4(0.000000, 0.000000, 0.000000, 0.000000);
   float off = 0.050000;
  if((renderpass - firstpass) == 2)
  {
     off = 0.010000;
  }
   vec2 tc = vec2(0.900000, 0.700000);
   vec2 dir = (uv.xy - tc);
   float dist = sqrt(((dir.x * dir.x) + (dir.y * dir.y)));
  dir = (dir / dist);
   int i = 0;
  while(i < 20)
  {
     sum += texture2D(composite, (uv + (dir * (((float(-i) * off) * 1.000000) / down))));
i++;
  }
  sum *= 0.050000;
  return sum;
}
void main()
{
  if(renderpass == lastpass)
  {
     gl_FragColor = identity(texcoord);
  }
  else
  {
     if(renderpass == firstpass)
     {
        gl_FragColor = (8.000000 * max(vec4(0.000000, 0.000000, 0.000000, 0.000000), (texture2D(composite, texcoord).xyzw - 0.500000)));
     }
     else
     {
        gl_FragColor = blurFast(texcoord);
     }
  }
}
)GLSL"},
	{"sht_highlight.bin",
	 R"GLSL(//gl_100
#ifdef GL_ES
precision mediump float;
precision lowp int;
#endif
//Input Layout:
//vec2 position
//vec2 uv
uniform mat4 mvp_mat;
uniform int renderpass;
attribute vec2 position;
attribute vec2 uv;
varying vec2 texcoord;

void main()
{
  gl_Position = (mvp_mat * vec4(position.x, position.y, 0.000000, 1.000000));
  texcoord = uv;
}
)GLSL",
	 R"GLSL(//gl_100
#ifdef GL_ES
precision mediump float;
precision lowp int;
#endif
uniform float time;
uniform sampler2D composite;
uniform int renderpass;
uniform vec2 mouse;
uniform vec2 resolution;
uniform float strength;
varying vec2 texcoord;

void main()
{
   float slope1 = 5.000000;
   float slope2 = 7.000000;
   vec2 uv = texcoord;
   float bright = ((((-sin((((uv.y * slope1) + (uv.x * 30.000000)) + (time * 3.100000))) * 0.200000) - (sin((((uv.y * slope2) + (uv.x * 37.000000)) + (time * 3.100000))) * 0.100000)) - (cos((((uv.x * 2.000000) * 1.000000) + (time * 2.100000))) * 0.100000)) - (sin((((-uv.x * 5.000000) * 1.000000) + (time * 2.000000))) * 0.300000));
   float modulate = (abs(((cos((time * 0.300000)) * 0.500000) + sin((time * 0.700000)))) * 0.500000);
  bright *= modulate;
   vec4 pix = texture2D(composite, uv);
  pix.xyz += (1.000000 * clamp((bright / 1.000000), 0.000000, 1.000000));
  gl_FragColor = pix;
}
)GLSL"},
	{"sht_light.bin",
	 R"GLSL(//gl_100
#ifdef GL_ES
precision mediump float;
precision lowp int;
#endif
//Input Layout:
//vec2 position
//vec2 uv
uniform mat4 mvp_mat;
uniform int renderpass;
attribute vec2 position;
attribute vec2 uv;
varying vec2 texcoord;

void main()
{
  gl_Position = (mvp_mat * vec4(position.x, position.y, 0.000000, 1.000000));
  texcoord = uv;
}
)GLSL",
	 R"GLSL(//gl_100
#ifdef GL_ES
precision mediump float;
precision lowp int;
#endif
struct light
{
  int type;
  vec3 position;
  vec3 targetpos;
  vec3 lightfalloff;
  vec3 ambient;
  vec3 diffuse;
  float diffusefactor;
  float exponent;
  float cutoff;
};
uniform float time;
uniform sampler2D composite;
uniform int renderpass;
uniform vec2 mouse;
uniform vec2 resolution;
uniform vec3 lightpos;
uniform vec3 targetpos;
uniform float lights_count;
uniform light lights[6];
varying vec2 texcoord;

void main()
{
   vec3 n = vec3(0.000000, 0.000000, 1.000000);
   vec3 diffuse = texture2D(composite, texcoord).xyz;
   float att = 0.000000;
   vec3 color = vec3(0.000000, 0.000000, 0.000000);
   int i_lights_count = int(lights_count);
   int i = 0;
  while(i < i_lights_count)
  {
      vec3 lightDir = vec3((lights[i].position - vec3((texcoord * resolution.xy), 0.000000)));
      float NdotL = max(dot(n, normalize(lightDir)), 0.000000);
      float dist = length(lightDir);
     if(lights[i].type == 0)
     {
        if((NdotL > 0.000000))
        {
            float spotEffect = dot(normalize((lights[i].targetpos - lights[i].position)), normalize(-lightDir));
           if((spotEffect > lights[i].cutoff))
           {
              spotEffect = pow(abs(abs(spotEffect)), lights[i].exponent);
              att = (spotEffect / ((lights[i].lightfalloff.x + (lights[i].lightfalloff.y * dist)) + ((lights[i].lightfalloff.z * dist) * dist)));
              color += (att * (((diffuse * lights[i].diffuse) * (NdotL * lights[i].diffusefactor)) + lights[i].ambient));
           }
        }
     }
     else
     {
        att = (1.000000 / ((lights[i].lightfalloff.x + (lights[i].lightfalloff.y * dist)) + ((lights[i].lightfalloff.z * dist) * dist)));
        color += (att * (((diffuse * lights[i].diffuse) * (NdotL * lights[i].diffusefactor)) + lights[i].ambient));
     }
i++;
  }
  gl_FragColor = vec4(color, 1.000000);
}
)GLSL"},
	{"sht_lut.bin",
	 R"GLSL(//gl_100
#ifdef GL_ES
precision mediump float;
precision lowp int;
#endif
//Input Layout:
//vec2 position
//vec2 uv
uniform mat4 mvp_mat;
uniform int renderpass;
attribute vec2 position;
attribute vec2 uv;
varying vec2 texcoord;

void main()
{
  gl_Position = (mvp_mat * vec4(position.x, position.y, 0.000000, 1.000000));
  texcoord = uv;
}
)GLSL",
	 R"GLSL(//gl_100
#ifdef GL_ES
precision mediump float;
precision lowp int;
#endif
uniform float time;
uniform sampler2D composite;
uniform int renderpass;
uniform vec2 mouse;
uniform vec2 resolution;
uniform float amount;
uniform sampler2D LUT;
varying vec2 texcoord;

vec4 lookup(vec4 textureColor)
{
  textureColor = clamp(textureColor, 0.000000, 1.000000);
   float blueColor = (textureColor.z * 63.000000);
  vec2 quad1;
  quad1.y = floor((floor(blueColor) / 8.000000));
  quad1.x = (floor(blueColor) - (quad1.y * 8.000000));
  vec2 quad2;
  quad2.y = floor((ceil(blueColor) / 8.000000));
  quad2.x = (ceil(blueColor) - (quad2.y * 8.000000));
  vec2 texPos1;
  texPos1.x = (((quad1.x * 0.125000) + 0.000977) + (0.123047 * textureColor.x));
  texPos1.y = (((quad1.y * 0.125000) + 0.000977) + (0.123047 * textureColor.y));
  vec2 texPos2;
  texPos2.x = (((quad2.x * 0.125000) + 0.000977) + (0.123047 * textureColor.x));
  texPos2.y = (((quad2.y * 0.125000) + 0.000977) + (0.123047 * textureColor.y));
   vec4 newColor1 = texture2D(LUT, texPos1);
   vec4 newColor2 = texture2D(LUT, texPos2);
   vec4 newColor = mix(newColor1, newColor2, fract(blueColor));
  return newColor;
}
void main()
{
  gl_FragColor = lookup(texture2D(composite, texcoord));
}
)GLSL"},
	{"sht_pearls.bin",
	 R"GLSL(//gl_100
#ifdef GL_ES
precision mediump float;
precision lowp int;
#endif
//Input Layout:
//vec2 position
//vec2 uv
uniform mat4 mvp_mat;
uniform int renderpass;
attribute vec2 position;
attribute vec2 uv;
varying vec2 texcoord;

void main()
{
  gl_Position = (mvp_mat * vec4(position.x, position.y, 0.000000, 1.000000));
  texcoord = uv;
}
)GLSL",
	 R"GLSL(//gl_100
#ifdef GL_ES
precision mediump float;
precision lowp int;
#endif
uniform float time;
uniform sampler2D composite;
uniform int renderpass;
uniform vec2 mouse;
uniform vec2 resolution;
varying vec2 texcoord;

void main()
{
   vec2 quadPos = (floor(((texcoord * resolution) / 7.000000)) * 7.000000);
   vec2 quad = (quadPos / resolution.xy);
   vec2 quadCenter = (quadPos + 3.500000);
   float dist = length((quadCenter - (texcoord * resolution)));
   vec4 texel = texture2D(composite, quad);
  if((dist > 2.625000))
  {
     gl_FragColor = vec4(0.250000, 0.250000, 0.250000, 0.250000);
  }
  else
  {
     gl_FragColor = texel;
  }
}
)GLSL"},
	{"sht_ripple1.bin",
	 R"GLSL(//gl_100
#ifdef GL_ES
precision mediump float;
precision lowp int;
#endif
//Input Layout:
//vec2 position
//vec2 uv
uniform mat4 mvp_mat;
uniform int renderpass;
attribute vec2 position;
attribute vec2 uv;
varying vec2 texcoord;

void main()
{
  gl_Position = (mvp_mat * vec4(position.x, position.y, 0.000000, 1.000000));
  texcoord = uv;
}
)GLSL",
	 R"GLSL(//gl_100
#ifdef GL_ES
precision mediump float;
precision lowp int;
#endif
uniform float time;
uniform sampler2D composite;
uniform int renderpass;
uniform vec2 mouse;
uniform vec2 resolution;
uniform float speed;
uniform float speed_x;
uniform float speed_y;
uniform float intensity;
uniform float frequency;
varying vec2 texcoord;

float col(vec2 coord)
{
   float delta_theta = 0.897598;
   float _col = 0.000000;
   float theta = 0.000000;
   int i = 0;
  while(i < 8)
  {
      vec2 adjc = coord;
     theta = (delta_theta * float(i));
     adjc.x += (((cos(theta) * time) * speed) + (time * speed_x));
     adjc.y -= (((sin(theta) * time) * speed) - (time * speed_y));
     _col = (_col + (cos((((adjc.x * cos(theta)) - (adjc.y * sin(theta))) * frequency)) * intensity));
i++;
  }
  return cos(_col);
}
void main()
{
   vec2 p = texcoord;
   vec2 c1 = p;
   vec2 c2 = p;
   float cc1 = col(c1);
  c2.x += (resolution.x / 60.000000);
   float dx = ((0.500000 * (cc1 - col(c2))) / 60.000000);
  c2.x = p.x;
  c2.y += (resolution.y / 60.000000);
   float dy = ((0.500000 * (cc1 - col(c2))) / 60.000000);
  c1.x += (dx * 2.000000);
  c1.y = (c1.y + (dy * 2.000000));
   float alpha = (1.000000 + (dot(dx, dy) * 700.000000));
   float _ddx = (dx - 0.012000);
   float _ddy = (dy - 0.012000);
  if(((_ddx > 0.000000) && (_ddy > 0.000000)))
  {
     alpha = pow(abs(abs(alpha)), ((_ddx * _ddy) * 200000.000000));
  }
  c1 = mod(c1, 1.000000);
  gl_FragColor = (texture2D(composite, c1) * alpha);
}
)GLSL"},
	{"sht_ripple2.bin",
	 R"GLSL(//gl_100
#ifdef GL_ES
precision mediump float;
precision lowp int;
#endif
//Input Layout:
//vec2 position
//vec2 uv
uniform mat4 mvp_mat;
uniform int renderpass;
attribute vec2 position;
attribute vec2 uv;
varying vec2 texcoord;

void main()
{
  gl_Position = (mvp_mat * vec4(position.x, position.y, 0.000000, 1.000000));
  texcoord = uv;
}
)GLSL",
	 R"GLSL(//gl_100
#ifdef GL_ES
precision mediump float;
precision lowp int;
#endif
uniform float time;
uniform sampler2D composite;
uniform int renderpass;
uniform vec2 mouse;
uniform vec2 resolution;
varying vec2 texcoord;

  float count = 10.000000;
  float strength = 0.900000;
void main()
{
   vec2 uv = texcoord;
   float w = ((0.500000 - uv.x) * (resolution.x / resolution.y));
   float h = (0.500000 - uv.y);
   float distanceFromCenter = sqrt(((w * w) + (h * h)));
   float sinArg = ((distanceFromCenter * count) - (time * 10.000000));
   float slope = cos(sinArg);
   vec4 color = texture2D(composite, (uv + (((strength * normalize(vec2(w, h))) * slope) * 0.050000)));
  gl_FragColor = color;
}
)GLSL"},
	{"sht_ripple3.bin",
	 R"GLSL(//gl_100
#ifdef GL_ES
precision mediump float;
precision lowp int;
#endif
//Input Layout:
//vec2 position
//vec2 uv
uniform mat4 mvp_mat;
uniform int renderpass;
attribute vec2 position;
attribute vec2 uv;
varying vec2 texcoord;

void main()
{
  gl_Position = (mvp_mat * vec4(position.x, position.y, 0.000000, 1.000000));
  texcoord = uv;
}
)GLSL",
	 R"GLSL(//gl_100
#ifdef GL_ES
precision mediump float;
precision lowp int;
#endif
uniform float time;
uniform sampler2D composite;
uniform int renderpass;
uniform vec2 mouse;
uniform vec2 resolution;
varying vec2 texcoord;

void main()
{
   vec2 uv = texcoord;
  uv.x += ((sin(((uv.y + (time * 0.070000)) * 45.000000)) * 0.009000) + (sin(((uv.y + (time * 0.100000)) * 35.000000)) * 0.005000));
   vec4 texColor = texture2D(composite, uv);
  gl_FragColor = texColor;
}
)GLSL"},
	{"sht_ripple4.bin",
	 R"GLSL(//gl_100
#ifdef GL_ES
precision mediump float;
precision lowp int;
#endif
//Input Layout:
//vec2 position
//vec2 uv
uniform mat4 mvp_mat;
uniform int renderpass;
attribute vec2 position;
attribute vec2 uv;
varying vec2 texcoord;

void main()
{
  gl_Position = (mvp_mat * vec4(position.x, position.y, 0.000000, 1.000000));
  texcoord = uv;
}
)GLSL",
	 R"GLSL(//gl_100
#ifdef GL_ES
precision mediump float;
precision lowp int;
#endif
uniform float time;
uniform sampler2D composite;
uniform int renderpass;
uniform vec2 mouse;
uniform vec2 resolution;
varying vec2 texcoord;

void main()
{
   vec2 uv = texcoord;
   float w = ((0.500000 - uv.x) * (resolution.x / resolution.y));
   float h = (0.500000 - uv.y);
   vec2 dv = vec2(w, h);
   float distanceFromCenter = sqrt(dot(dv, dv));
   float sinArg = ((distanceFromCenter * 10.000000) - (time * 10.000000));
   float slope = cos(sinArg);
   vec4 color = texture2D(composite, (uv + ((normalize(vec2(w, h)) * slope) * 0.010000)));
  gl_FragColor = color;
}
)GLSL"},
	{"sht_transition.bin",
	 R"GLSL(//gl_100
#ifdef GL_ES
precision mediump float;
precision lowp int;
#endif
//Input Layout:
//vec2 position
//vec2 uv
uniform mat4 mvp_mat;
uniform int renderpass;
attribute vec2 position;
attribute vec2 uv;
varying vec2 texcoord;

void main()
{
  gl_Position = (mvp_mat * vec4(position.x, position.y, 0.000000, 1.000000));
  texcoord = uv;
}
)GLSL",
	 R"GLSL(//gl_100
#ifdef GL_ES
precision mediump float;
precision lowp int;
#endif
uniform float time;
uniform sampler2D composite;
uniform int renderpass;
uniform vec2 mouse;
uniform vec2 resolution;
uniform float progress;
uniform sampler2D tex2;
uniform sampler2D transition_tex;
uniform float progress_size;
varying vec2 texcoord;

void main()
{
   vec2 uv = texcoord;
   float result = (1.000000 - clamp(((texture2D(transition_tex, vec2(texcoord.x, (1.000000 - texcoord.y))).x - progress) / progress_size), 0.000000, 1.000000));
   vec3 color = texture2D(composite, texcoord).xyz;
   vec3 color2 = texture2D(tex2, texcoord).xyz;
  gl_FragColor = vec4(((color2 * result) + (color * (1.000000 - result))), 1.000000);
}
)GLSL"},
	{"sht_tv1.bin",
	 R"GLSL(//gl_100
#ifdef GL_ES
precision mediump float;
precision lowp int;
#endif
//Input Layout:
//vec2 position
//vec2 uv
uniform mat4 mvp_mat;
uniform int renderpass;
attribute vec2 position;
attribute vec2 uv;
varying vec2 texcoord;

void main()
{
  gl_Position = (mvp_mat * vec4(position.x, position.y, 0.000000, 1.000000));
  texcoord = uv;
}
)GLSL",
	 R"GLSL(//gl_100
#ifdef GL_ES
precision mediump float;
precision lowp int;
#endif
uniform float time;
uniform sampler2D composite;
uniform int renderpass;
uniform vec2 mouse;
uniform vec2 resolution;
varying vec2 texcoord;

float rand(vec2 co)
{
  return fract((sin(dot(co.xy, vec2(12.989800, 78.233000))) * 43758.545300));
}
float rand(float c)
{
  return rand(vec2(c, 1.000000));
}
float randomLine(float seed, vec2 uv)
{
   float b = (0.010000 * rand(seed));
   float a = rand((seed + 1.000000));
   float c = (rand((seed + 2.000000)) - 0.500000);
   float mu = rand((seed + 3.000000));
   float l = 1.000000;
  if((mu > 0.200000))
  {
     l = pow(abs(abs(abs((((a * uv.x) + (b * uv.y)) + c)))), 0.125000);
  }
  else
  {
     l = (2.000000 - pow(abs(abs(abs((((a * uv.x) + (b * uv.y)) + c)))), 0.125000));
  }
  return mix(0.500000, 1.000000, l);
}
float randomBlotch(float seed, vec2 uv)
{
   float x = rand(seed);
   float y = rand((seed + 1.000000));
   float s = (0.010000 * rand((seed + 2.000000)));
   vec2 p = (vec2(x, y) - uv);
  p.x *= (resolution.x / resolution.y);
   float a = atan(p.y, p.x);
   float v = 1.000000;
   float ss = ((s * s) * ((sin(((6.283100 * a) * x)) * 0.100000) + 1.000000));
  if(dot(p, p) < ss)
  {
     v = 0.200000;
  }
  else
  {
     v = pow(abs(abs((dot(p, p) - ss))), 0.062500);
  }
  return mix((0.300000 + (0.200000 * (1.000000 - (s / 0.020000)))), 1.000000, v);
}
void main()
{
   vec2 uv = texcoord;
   float t = float(int((time * 15.000000)));
   vec2 suv = (uv + (0.002000 * vec2(rand(t), rand((t + 23.000000)))));
   vec3 image = texture2D(composite, vec2(suv.x, suv.y)).xyz;
   float luma = dot(vec3(0.212600, 0.715200, 0.072200), image);
   vec3 oldImage = (luma * vec3(0.700000, 0.700000, 0.700000));
   float vI = (16.000000 * (((uv.x * (1.000000 - uv.x)) * uv.y) * (1.000000 - uv.y)));
  vI *= mix(0.700000, 1.000000, rand((t + 0.500000)));
  vI += (1.000000 + (0.400000 * rand((t + 8.000000))));
  vI *= pow(abs(abs(((((16.000000 * uv.x) * (1.000000 - uv.x)) * uv.y) * (1.000000 - uv.y)))), 0.400000);
   int l = int((8.000000 * rand((t + 7.000000))));
  if(0 < l)
  {
     vI *= randomLine(((t + 6.000000) + 0.000000), uv);
  }
  if(1 < l)
  {
     vI *= randomLine(((t + 6.000000) + 17.000000), uv);
  }
  if(2 < l)
  {
     vI *= randomLine(((t + 6.000000) + 34.000000), uv);
  }
  if(3 < l)
  {
     vI *= randomLine(((t + 6.000000) + 51.000000), uv);
  }
  if(4 < l)
  {
     vI *= randomLine(((t + 6.000000) + 68.000000), uv);
  }
  if(5 < l)
  {
     vI *= randomLine(((t + 6.000000) + 85.000000), uv);
  }
  if(6 < l)
  {
     vI *= randomLine(((t + 6.000000) + 102.000000), uv);
  }
  if(7 < l)
  {
     vI *= randomLine(((t + 6.000000) + 119.000000), uv);
  }
   int s = int(max(((8.000000 * rand((t + 18.000000))) - 2.000000), 0.000000));
  if(0 < s)
  {
     vI *= randomBlotch(((t + 6.000000) + 0.000000), uv);
  }
  if(1 < s)
  {
     vI *= randomBlotch(((t + 6.000000) + 19.000000), uv);
  }
  if(2 < s)
  {
     vI *= randomBlotch(((t + 6.000000) + 38.000000), uv);
  }
  if(3 < s)
  {
     vI *= randomBlotch(((t + 6.000000) + 57.000000), uv);
  }
  if(4 < s)
  {
     vI *= randomBlotch(((t + 6.000000) + 76.000000), uv);
  }
  if(5 < s)
  {
     vI *= randomBlotch(((t + 6.000000) + 95.000000), uv);
  }
  gl_FragColor.xyz = (oldImage * vI);
  gl_FragColor.xyz *= (1.000000 + ((rand((uv + (t * 0.010000))) - 0.200000) * 0.150000));
  gl_FragColor.w = 1.000000;
}
)GLSL"},
	{"sht_tv2.bin",
	 R"GLSL(//gl_100
#ifdef GL_ES
precision mediump float;
precision lowp int;
#endif
//Input Layout:
//vec2 position
//vec2 uv
uniform mat4 mvp_mat;
uniform int renderpass;
attribute vec2 position;
attribute vec2 uv;
varying vec2 texcoord;

void main()
{
  gl_Position = (mvp_mat * vec4(position.x, position.y, 0.000000, 1.000000));
  texcoord = uv;
}
)GLSL",
	 R"GLSL(//gl_100
#ifdef GL_ES
precision mediump float;
precision lowp int;
#endif
uniform float time;
uniform sampler2D composite;
uniform int renderpass;
uniform vec2 mouse;
uniform vec2 resolution;
varying vec2 texcoord;

float rand(vec2 co)
{
  return fract((sin(dot(co.xy, vec2(12.989800, 78.233000))) * 43758.545300));
}
void main()
{
   vec2 uv = texcoord;
   float screenRatio = (resolution.x / resolution.y);
   vec4 col = texture2D(composite, uv).xyzw;
   float barHeight = 6.000000;
   float barSpeed = 5.600000;
   float barOverflow = 1.200000;
   float blurBar = clamp((sin(((uv.y * barHeight) + (time * barSpeed))) + 1.250000), 0.000000, 1.000000);
   float bar = clamp(floor((sin(((uv.y * barHeight) + (time * barSpeed))) + 1.950000)), 0.000000, barOverflow);
   float noiseIntensity = 0.750000;
   float pixelDensity = 250.000000;
   float noise = clamp(((rand(((vec2(floor(((uv.x * pixelDensity) * screenRatio)), floor((uv.y * pixelDensity))) * time) / 1000.000000)) + 1.000000) - noiseIntensity), 0.000000, 1.000000);
   vec3 color = vec3(noise, noise, noise);
  color = mix((color - (noiseIntensity * vec3(0.250000, 0.250000, 0.250000))), color, blurBar);
  color = mix((color - (noiseIntensity * vec3(0.080000, 0.080000, 0.080000))), color, bar);
  color = mix(vec3(0.000000, 0.000000, 0.000000), col.xyz, color);
  color.z += 0.042000;
  noise = (1.000000 - (pow(abs(abs(distance(uv, vec2(0.500000, 0.500000)))), 2.100000) * 2.800000));
  color *= vec3(noise, noise, noise);
  gl_FragColor = vec4(color.xyz, col.w);
}
)GLSL"},
	{"sht_tv3.bin",
	 R"GLSL(//gl_100
#ifdef GL_ES
precision mediump float;
precision lowp int;
#endif
//Input Layout:
//vec2 position
//vec2 uv
uniform mat4 mvp_mat;
uniform int renderpass;
attribute vec2 position;
attribute vec2 uv;
varying vec2 texcoord;

void main()
{
  gl_Position = (mvp_mat * vec4(position.x, position.y, 0.000000, 1.000000));
  texcoord = uv;
}
)GLSL",
	 R"GLSL(//gl_100
#ifdef GL_ES
precision mediump float;
precision lowp int;
#endif
uniform float time;
uniform sampler2D composite;
uniform int renderpass;
uniform vec2 mouse;
uniform vec2 resolution;
varying vec2 texcoord;

float rand(vec2 co)
{
   float a = 12.989800;
   float b = 78.233000;
   float c = 43758.545300;
   float dt = dot(co.xy, vec2(a, b));
   float sn = mod(dt, 3.140000);
  return fract((sin(sn) * c));
}
void main()
{
   vec2 uv = texcoord;
   float magnitude = 0.000900;
   vec2 offsetRedUV = uv;
  offsetRedUV.x = (uv.x + (rand(vec2((time * 0.030000), (uv.y * 0.420000))) * 0.001000));
  offsetRedUV.x += (sin(rand(vec2((time * 0.200000), uv.y))) * magnitude);
   vec2 offsetGreenUV = uv;
  offsetGreenUV.x = (uv.x + (rand(vec2((time * 0.004000), (uv.y * 0.002000))) * 0.004000));
  offsetGreenUV.x += (sin((time * 9.000000)) * magnitude);
   vec2 offsetBlueUV = uv;
  offsetBlueUV.x = uv.y;
  offsetBlueUV.x += rand(vec2(cos((time * 0.010000)), sin(uv.y)));
   float r = texture2D(composite, offsetRedUV).x;
   float g = texture2D(composite, offsetGreenUV).y;
   float b = texture2D(composite, uv).z;
  gl_FragColor = vec4(r, g, b, 0.000000);
}
)GLSL"},
	{"sht_tv4.bin",
	 R"GLSL(//gl_100
#ifdef GL_ES
precision mediump float;
precision lowp int;
#endif
//Input Layout:
//vec2 position
//vec2 uv
uniform mat4 mvp_mat;
uniform int renderpass;
attribute vec2 position;
attribute vec2 uv;
varying vec2 texcoord;

void main()
{
  gl_Position = (mvp_mat * vec4(position.x, position.y, 0.000000, 1.000000));
  texcoord = uv;
}
)GLSL",
	 R"GLSL(//gl_100
#ifdef GL_ES
precision mediump float;
precision lowp int;
#endif
uniform float time;
uniform sampler2D composite;
uniform int renderpass;
uniform vec2 mouse;
uniform vec2 resolution;
varying vec2 texcoord;

vec3 mod289(vec3 x)
{
  return (x - (floor((x * 0.003460)) * 289.000000));
}
vec2 mod289(vec2 x)
{
  return (x - (floor((x * 0.003460)) * 289.000000));
}
vec3 permute(vec3 x)
{
  return mod289((((x * 34.000000) + 1.000000) * x));
}
float snoise(vec2 v)
{
   vec2 i = floor((v + dot(v, vec2(0.366025, 0.366025))));
   vec2 x0 = ((v - i) + dot(i, vec2(0.211325, 0.211325)));
   vec2 i1 = ((x0.x > x0.y) ? vec2(1.000000, 0.000000) : vec2(0.000000, 1.000000));
   vec4 x12 = (x0.xyxy + vec4(0.211325, 0.211325, -0.577350, -0.577350));
  x12.xy -= i1;
  i = mod289(i);
   vec3 p = permute(((permute((i.y + vec3(0.000000, i1.y, 1.000000))) + i.x) + vec3(0.000000, i1.x, 1.000000)));
   vec3 m = max((0.500000 - vec3(dot(x0, x0), dot(x12.xy, x12.xy), dot(x12.zw, x12.zw))), 0.000000);
  m = (m * m);
  m = (m * m);
   vec3 x = ((2.000000 * fract((p * vec3(0.024390, 0.024390, 0.024390)))) - 1.000000);
   vec3 h = (abs(x) - 0.500000);
   vec3 ox = floor((x + 0.500000));
   vec3 a0 = (x - ox);
  m *= (1.792843 - (0.853735 * ((a0 * a0) + (h * h))));
  vec3 g;
  g.x = ((a0.x * x0.x) + (h.x * x0.y));
  g.yz = ((a0.yz * x12.xz) + (h.yz * x12.yw));
  return (130.000000 * dot(m, g));
}
void main()
{
   vec2 uv = texcoord;
   float jerkOffset = ((1.000000 - step(snoise(vec2((time * 1.300000), 5.000000)), 0.800000)) * 0.050000);
   float wiggleOffset = (snoise(vec2((time * 15.000000), (uv.y * 80.000000))) * 0.003000);
   float largeWiggleOffset = (snoise(vec2((time * 1.000000), (uv.y * 25.000000))) * 0.004000);
   float xOffset = ((wiggleOffset + largeWiggleOffset) + jerkOffset);
   float red = texture2D(composite, vec2(((uv.x + xOffset) - 0.010000), uv.y)).x;
   float green = texture2D(composite, vec2((uv.x + xOffset), uv.y)).y;
   float blue = texture2D(composite, vec2(((uv.x + xOffset) + 0.010000), uv.y)).z;
   vec3 color = vec3(red, green, blue);
   float scanline = (sin((uv.y * 800.000000)) * 0.040000);
  color -= scanline;
  gl_FragColor = vec4(color, 1.000000);
}
)GLSL"},
	{"sht_warp1.bin",
	 R"GLSL(//gl_100
#ifdef GL_ES
precision mediump float;
precision lowp int;
#endif
//Input Layout:
//vec2 position
//vec2 uv
uniform mat4 mvp_mat;
uniform int renderpass;
attribute vec2 position;
attribute vec2 uv;
varying vec2 texcoord;

void main()
{
  gl_Position = (mvp_mat * vec4(position.x, position.y, 0.000000, 1.000000));
  texcoord = uv;
}
)GLSL",
	 R"GLSL(//gl_100
#ifdef GL_ES
precision mediump float;
precision lowp int;
#endif
uniform float time;
uniform sampler2D composite;
uniform int renderpass;
uniform vec2 mouse;
uniform vec2 resolution;
uniform float strength;
varying vec2 texcoord;

void main()
{
   float t = (time * 0.500000);
   vec2 uv = texcoord;
   float aspect = (resolution.x / resolution.y);
   mat3 xform = mat3(cos(sin(t)), sin((t * 0.250000)), 0.000000, -sin((t * 0.250000)), cos(cos(t)), 0.000000, (cos((t / 2.000000)) * 0.200000), (sin(t) * 0.200000), 1.000000);
  uv = ((xform * vec3(uv, 1.000000)).xy * vec2(aspect, 1.000000));
  uv.x -= (sin(t) + (cos(((t * 2.000000) + ((cos(uv.x) * sin((t * 2.000000))) * 2.000000))) / 2.000000));
  uv.y += ((cos((t + (uv.y * 0.500000))) + sin((uv.y * cos(t)))) + sin((cos((t * 0.500000)) * length(uv))));
  uv = mod(uv, 1.000000);
  uv = mix(texcoord, uv, strength);
   vec4 color = texture2D(composite, uv);
  gl_FragColor = color;
}
)GLSL"},
	{"sht_warp2.bin",
	 R"GLSL(//gl_100
#ifdef GL_ES
precision mediump float;
precision lowp int;
#endif
//Input Layout:
//vec2 position
//vec2 uv
uniform mat4 mvp_mat;
uniform int renderpass;
attribute vec2 position;
attribute vec2 uv;
varying vec2 texcoord;

void main()
{
  gl_Position = (mvp_mat * vec4(position.x, position.y, 0.000000, 1.000000));
  texcoord = uv;
}
)GLSL",
	 R"GLSL(//gl_100
#ifdef GL_ES
precision mediump float;
precision lowp int;
#endif
uniform float time;
uniform sampler2D composite;
uniform int renderpass;
uniform vec2 mouse;
uniform vec2 resolution;
uniform float strength;
varying vec2 texcoord;

void main()
{
   vec2 uv = texcoord;
   float y = ((((0.700000 * sin(((uv.y + time) * 4.000000))) * 0.038000) + ((0.300000 * sin(((uv.y + time) * 8.000000))) * 0.010000)) + ((0.050000 * sin(((uv.y + time) * 40.000000))) * 0.050000));
   float x = ((((0.500000 * sin(((uv.y + time) * 5.000000))) * 0.100000) + ((0.200000 * sin(((uv.x + time) * 10.000000))) * 0.050000)) + ((0.200000 * sin(((uv.x + time) * 30.000000))) * 0.020000));
  gl_FragColor = texture2D(composite, mix(uv, (0.790000 * (uv + vec2((y + 0.110000), (x + 0.110000)))), strength));
}
)GLSL"},
	{"stipple.bin",
	 R"GLSL(//gl_100
#ifdef GL_ES
precision mediump float;
precision lowp int;
#endif
//Input Layout:
//vec2 position
uniform mat4 mvp_mat;
uniform float depth;
attribute vec2 position;
varying vec2 pos;

void main()
{
  pos = position;
  gl_Position = (mvp_mat * vec4(position.x, position.y, depth, 1.000000));
}
)GLSL",
	 R"GLSL(//gl_100
#ifdef GL_ES
precision mediump float;
precision lowp int;
#endif
uniform vec4 color1;
uniform vec4 color2;
varying vec2 pos;

void main()
{
   float x = floor((mod((pos.x + pos.y), 4.000000) / 2.000000));
  gl_FragColor = ((color1 * x) + (color2 * (1.000000 - x)));
}
)GLSL"},
	{"text.bin",
	 R"GLSL(//gl_100
#ifdef GL_ES
precision mediump float;
precision lowp int;
#endif
//Input Layout:
//vec2 position
//vec2 uv
uniform mat4 mvp_mat;
uniform sampler2D texel;
uniform vec4 color;
uniform vec4 bg;
uniform vec2 offset;
attribute vec2 position;
attribute vec2 uv;
varying vec2 uvVarying;

void main()
{
  gl_Position = (mvp_mat * vec4((position.x + offset.x), (position.y + offset.y), 0.000000, 1.000000));
  uvVarying = uv;
}
)GLSL",
	 R"GLSL(//gl_100
#ifdef GL_ES
precision mediump float;
precision lowp int;
#endif
uniform vec4 color;
uniform vec4 bg;
uniform sampler2D texel;
varying vec2 uvVarying;

void main()
{
   vec4 col = texture2D(texel, uvVarying);
  if(((col.x == 0.000000 && col.y == 0.000000) && col.z == 0.000000))
  {
     gl_FragColor = vec4(0.000000, 0.000000, 0.000000, 0.000000);
  }
  else
  {
     gl_FragColor = vec4(((color.x * col.x) + (bg.x * (1.000000 - col.x))), ((color.y * col.y) + (bg.y * (1.000000 - col.y))), ((color.z * col.z) + (bg.z * (1.000000 - col.z))), color.w);
  }
}
)GLSL"},
	{"textured.bin",
	 R"GLSL(//gl_100
#ifdef GL_ES
precision mediump float;
precision lowp int;
#endif
//Input Layout:
//vec2 position
//vec2 uv
uniform mat4 mvp_mat;
uniform sampler2D texel;
uniform float depth;
attribute vec2 position;
attribute vec2 uv;
varying vec2 uvVarying;

void main()
{
  gl_Position = (mvp_mat * vec4(position.x, position.y, depth, 1.000000));
  uvVarying = uv;
}
)GLSL",
	 R"GLSL(//gl_100
#ifdef GL_ES
precision mediump float;
precision lowp int;
#endif
uniform vec4 color;
uniform sampler2D texel;
varying vec2 uvVarying;

void main()
{
  gl_FragColor = (texture2D(texel, uvVarying) * color);
}
)GLSL"},
	{"textured_grayscale.bin",
	 R"GLSL(//gl_100
#ifdef GL_ES
precision mediump float;
precision lowp int;
#endif
//Input Layout:
//vec2 position
//vec2 uv
uniform mat4 mvp_mat;
uniform sampler2D texel;
uniform float depth;
attribute vec2 position;
attribute vec2 uv;
varying vec2 uvVarying;

void main()
{
  gl_Position = (mvp_mat * vec4(position.x, position.y, depth, 1.000000));
  uvVarying = uv;
}
)GLSL",
	 R"GLSL(//gl_100
#ifdef GL_ES
precision mediump float;
precision lowp int;
#endif
uniform vec4 color;
uniform sampler2D texel;
varying vec2 uvVarying;

void main()
{
   vec4 col = texture2D(texel, uvVarying);
   float graycolor = (((col.x + col.y) + col.z) / 3.000000);
  gl_FragColor = (vec4(graycolor, graycolor, graycolor, col.w) * color);
}
)GLSL"},
	{"yuv.bin",
	 R"GLSL(//gl_100
#ifdef GL_ES
precision mediump float;
precision lowp int;
#endif
//Input Layout:
//vec2 position
//vec2 texcoord
uniform mat4 mvp_mat;
attribute vec2 position;
attribute vec2 texcoord;
varying vec2 uv;

void main()
{
  gl_Position = (mvp_mat * vec4(position.x, position.y, 0.000000, 1.000000));
  uv = texcoord;
}
)GLSL",
	 R"GLSL(//gl_100
#ifdef GL_ES
precision mediump float;
precision lowp int;
#endif
uniform sampler2D texy;
uniform sampler2D texu;
uniform sampler2D texv;
uniform vec2 size;
uniform vec2 uvSize;
varying vec2 uv;

void main()
{
   vec2 _uv = uv;
   vec4 _y = texture2D(texy, _uv).xyzw;
   vec4 _u = texture2D(texu, _uv).xyzw;
   vec4 _v = texture2D(texv, _uv).xyzw;
   int p1 = int(mod((_uv.x * size.x), 4.000000));
   int p2 = int(mod((_uv.x * uvSize.x), 4.000000));
   float y = _y[p1];
   float u = _u[p2];
   float v = _v[p2];
  u = (u - 0.500000);
  v = (v - 0.500000);
  vec3 rgb;
  rgb.x = (((y + (1.403000 * v)) - 0.066410) * 1.071134);
  rgb.y = ((((y - (0.344000 * u)) - (0.714000 * v)) - 0.058590) * 1.062236);
  rgb.z = (((y + (1.770000 * u)) - 0.066410) * 1.071134);
  gl_FragColor = vec4(rgb, 1.000000);
}
)GLSL"},
	{nullptr, nullptr, nullptr}
};

const int g_builtinShaderCount = 51;
