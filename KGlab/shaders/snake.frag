#version 120

uniform sampler2D uTexture;
uniform int       uUseTexture;

varying vec3 vNormalEye;
varying vec3 vPosEye;
varying vec2 vTexCoord;

void main()
{
    vec3 N = normalize(vNormalEye);
    vec3 V = normalize(-vPosEye);

    if (dot(N, V) < 0.0) N = -N;

    vec3  Lvec = gl_LightSource[0].position.xyz - vPosEye;
    float d    = length(Lvec);
    vec3  L    = Lvec / max(d, 0.0001);

    vec3 H = normalize(L + V);

    float NdotL = max(dot(N, L), 0.0);
    float NdotH = max(dot(N, H), 0.0);

    vec4 ambient  = gl_FrontMaterial.ambient  * gl_LightSource[0].ambient;
    vec4 diffuse  = gl_FrontMaterial.diffuse  * gl_LightSource[0].diffuse  * NdotL;
    vec4 specular = gl_FrontMaterial.specular * gl_LightSource[0].specular *
                    pow(NdotH, max(gl_FrontMaterial.shininess, 1.0));

    float att = 1.0 / (gl_LightSource[0].constantAttenuation +
                       gl_LightSource[0].linearAttenuation  * d +
                       gl_LightSource[0].quadraticAttenuation * d * d);

    vec4 lightColor = ambient + att * (diffuse + specular);

    vec4 baseColor = (uUseTexture == 1)
        ? texture2D(uTexture, vTexCoord)
        : vec4(1.0);

    gl_FragColor = lightColor * baseColor;
}