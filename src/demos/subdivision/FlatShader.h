#pragma once

static const char* flat_vs = R"(
#version 330

in vec3 vertexPosition;
in vec2 vertexTexCoord;
in vec3 vertexNormal;
in vec4 vertexColor;

uniform mat4 mvp;
uniform mat4 matModel;

out vec3 fragPosition;

void main()
{
    fragPosition = vec3(matModel * vec4(vertexPosition, 1.0));
    gl_Position = mvp * vec4(vertexPosition, 1.0);
}
)";

static const char* flat_fs = R"(
#version 330

in vec3 fragPosition;

uniform vec3 lightDir;
uniform vec4 colDiffuse;
uniform float lightIntensity;

out vec4 finalColor;

void main()
{
    // Compute flat normal using standard derivatives
    vec3 dpdx = dFdx(fragPosition);
    vec3 dpdy = dFdy(fragPosition);
    vec3 normal = normalize(cross(dpdx, dpdy));

    vec3 lDir = normalize(lightDir);
    float nDotL = max(dot(normal, lDir), 0.0);
    
    // Ambient term
    vec3 ambient = colDiffuse.rgb * 0.2;
    
    // Diffuse term
    vec3 diffuse = colDiffuse.rgb * nDotL * lightIntensity;

    finalColor = vec4(ambient + diffuse, colDiffuse.a);
}
)";
