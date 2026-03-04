#version 410 core

in vec3 fNormal;
in vec4 fPosEye;
in vec2 fTexCoords;
in vec4 fPosLightSpace;

out vec4 fColor;

// Uniforme Lighting
uniform vec3 lightDir;
uniform vec3 lightColor;
uniform sampler2D diffuseTexture;
uniform sampler2D specularTexture;
uniform sampler2D shadowMap;

// Uniforme Efecte
uniform float fogDensity;
uniform vec3 fogColor;
uniform int showFog;

// Uniforme Lumina Portal 
uniform int portalLightOn;
uniform vec3 portalLightPos; // In Eye Space
uniform float portalLightIntensity; // Pentru controlul din taste

uniform int sunLightOn;
uniform float sunLightIntensity;

vec3 ambient;
vec3 diffuse;
vec3 specular;
float ambientStrength = 0.4f;
float specularStrength = 0.5f;
float shininess = 32.0f;

void computeLightComponents() {
    vec3 cameraPosEye = vec3(0.0f);
    vec3 normalEye = normalize(fNormal);
    vec3 lightDirN = normalize(lightDir);
    vec3 viewDirN = normalize(cameraPosEye - fPosEye.xyz);

    ambient = ambientStrength * lightColor;
    diffuse = max(dot(normalEye, lightDirN), 0.0f) * lightColor;

    vec3 reflection = reflect(-lightDirN, normalEye);
    float specCoeff = pow(max(dot(viewDirN, reflection), 0.0f), shininess);
    specular = specularStrength * specCoeff * lightColor;
}

float computeShadow() {
    vec3 projCoords = fPosLightSpace.xyz / fPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;
    
    if (projCoords.z > 1.0) return 0.0;

    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
    float currentDepth = projCoords.z;
    float bias = max(0.05 * (1.0 - dot(normalize(fNormal), normalize(lightDir))), 0.005);
    for(int x = -1; x <= 1; ++x) {
        for(int y = -1; y <= 1; ++y) {
            float pcfDepth = texture(shadowMap, projCoords.xy + vec2(x, y) * texelSize).r; 
            shadow += currentDepth - bias > pcfDepth ? 1.0 : 0.0;        
        }    
    }
    return shadow / 9.0; // Media celor 9 esantioane pentru a face smooth la marginea umbrelor
}
void main() {
    computeLightComponents();

    vec4 texColor = texture(diffuseTexture, fTexCoords);
    if(texColor.a < 0.1) discard;

    float shadow = computeShadow();

    vec3 sunAmbient = ambient * texColor.rgb;
    vec3 sunDiffuse = (1.0 - shadow) * diffuse * texColor.rgb;
    vec3 sunSpecular = (1.0 - shadow) * specular * texture(specularTexture, fTexCoords).rgb;

    vec3 color = vec3(0.0);
    
    if (sunLightOn == 1) {
        color = (sunAmbient + sunDiffuse + sunSpecular) * sunLightIntensity;
    } else {
        vec3 moonAmbient = vec3(0.1, 0.1, 0.2) * texColor.rgb;
        color = moonAmbient;
    }

    if (portalLightOn == 1) {
    	vec3 pointLightColor = vec3(0.6, 1.0, 1.6);
    	vec3 lightVec = portalLightPos - fPosEye.xyz;
    	float dist = length(lightVec);
    	float attenuation = 1.0 / (1.0 + 0.0003 * dist + 0.000005 * dist * dist);
    	float diffPoint = max(dot(normalize(fNormal), normalize(lightVec)), 0.0);
    	vec3 localAmbient = pointLightColor * 0.1; 
    	float intensityBoost = 5.0;
    	color += (localAmbient + diffPoint * pointLightColor) * attenuation * portalLightIntensity * intensityBoost * texColor.rgb;
	}

    if (showFog == 1) {
        float dist = length(fPosEye.xyz);
        float fogFactor = exp(-pow(dist * fogDensity, 2));
        fogFactor = clamp(fogFactor, 0.0, 1.0);
        color = mix(fogColor, color, fogFactor);
    }

    fColor = vec4(color, 1.0f);
}