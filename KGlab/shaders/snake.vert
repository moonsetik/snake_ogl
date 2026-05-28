#version 120

uniform int uTexGenMode;

varying vec3 vNormalEye;
varying vec3 vPosEye;
varying vec2 vTexCoord;

void main()
{
    vec4 posEye = gl_ModelViewMatrix * gl_Vertex;
    vPosEye    = posEye.xyz;
    vNormalEye = gl_NormalMatrix * gl_Normal;

    if (uTexGenMode == 1) {
        vTexCoord = gl_Vertex.xy;
    } else {
        vTexCoord = gl_MultiTexCoord0.xy;
    }

    gl_Position = gl_ProjectionMatrix * posEye;
}