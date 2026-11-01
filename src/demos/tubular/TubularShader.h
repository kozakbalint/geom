#pragma once

static const char* tubular_vs = R"(
#version 330

in vec3 vertexPosition;
in vec3 vertexNormal;

uniform mat4 mvp;
uniform mat4 matModel;

out vec3 fragPosition;
out vec3 fragNormal;

void main()
{
    fragPosition = vec3(matModel * vec4(vertexPosition, 1.0));
    fragNormal = normalize(mat3(matModel) * vertexNormal);
    gl_Position = mvp * vec4(vertexPosition, 1.0);
}
)";

static const char* tubular_fs = R"(
#version 330

in vec3 fragPosition;
in vec3 fragNormal;

uniform vec3 lightDir;
uniform vec3 camPos;
uniform vec3 surfaceColor;
uniform float lightIntensity;
uniform float shininess;
uniform float specularStrength;

out vec4 finalColor;

void main()
{
    vec3 N = normalize(fragNormal);
    vec3 V = normalize(camPos - fragPosition);
    vec3 L = normalize(lightDir);
    vec3 LFill = normalize(vec3(-0.6, -0.4, 0.5));
    vec3 H = normalize(L + V);

    float nDotL = max(0.0, dot(N, L));
    float nDotFill = max(0.0, dot(N, LFill)) * 0.20;
    float nDotH = max(0.0, dot(N, H));
    float spec = (nDotL > 0.0) ? pow(nDotH, shininess) * specularStrength * lightIntensity : 0.0;

    float diffuse = clamp(0.22 + 0.70 * nDotL * lightIntensity + nDotFill, 0.0, 1.0);

    vec3 rgb = clamp(surfaceColor * diffuse + vec3(spec), 0.0, 1.0);
    finalColor = vec4(rgb, 1.0);
}
)";
