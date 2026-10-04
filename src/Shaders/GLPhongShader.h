#ifndef RENDERER_GLPHONGSHADER_H
#define RENDERER_GLPHONGSHADER_H

inline constexpr const char* glModelVertexShaderSource = R"(
    #version 410 core
    layout (location = 0) in vec3 aPos;
    layout (location = 1) in vec3 aNormal;
    layout (location = 2) in vec2 aUV;
    layout (location = 3) in vec3 aTangent;
    layout (location = 4) in vec3 aBitangent;
    layout (location = 5) in vec3 aFaceNormal;

    uniform mat4 model;
    uniform mat4 view;
    uniform mat4 projection;
    uniform mat3 normalMatrix;

    uniform int shadingMode; // 0 = Flat, 1 = Gouraud, 2 = Phong
    uniform vec3 cameraPos;

    struct Light {
        int type; // 0 = Directional, 1 = Point
        vec3 direction;
        vec3 position;
        vec3 color;
        float intensity;
    };
    #define MAX_LIGHTS 8
    uniform Light lights[MAX_LIGHTS];
    uniform int numLights;

    // 0 = use the model's own UVs, 1 = planar projection onto object-space
    // XZ, 2 = spherical (latitude/longitude) projection, both canonical
    // texture-coordinate generation methods from the texture mapping tutorial.
    uniform int uvMode;
    uniform float uvScale;

    // Toon shading
    uniform bool useToonShading;
    uniform float toonLevels;

    // Silhouette pass
    uniform bool renderSilhouette;
    uniform float silhouetteThickness;

    // Vertex animation.
    uniform bool useVertexAnimation;
    uniform float vertexAnimAmplitude;
    uniform float vertexAnimSpeed;
    uniform float time;

    out vec3 worldPos;
    out vec3 objectPos;
    out vec3 smoothNormal;
    out vec2 uvOut;
    flat out vec3 flatFaceNormal;
    out vec3 vertexDiffuse;
    out vec3 vertexSpecular;
    out vec3 worldTangent;
    out vec3 worldBitangent;

    vec3 lightDirTo(Light light, vec3 pos) {
        return light.type == 0 ? normalize(light.direction) : normalize(light.position - pos);
    }

    vec2 generateUV(vec3 objectPos) {
        if (uvMode == 1) {
            return objectPos.xz * uvScale;
        }
        if (uvMode == 2) {
            vec3 n = normalize(objectPos);
            float u = 0.5 + atan(n.z, n.x) / (2.0 * 3.14159265);
            float v = 0.5 - asin(clamp(n.y, -1.0, 1.0)) / 3.14159265;
            return vec2(u, v) * uvScale;
        }
        return vec2(0.0);
    }

    void computeLighting(vec3 normal, vec3 pos, out vec3 diffuseOut, out vec3 specularOut) {
        diffuseOut = vec3(0.0);
        specularOut = vec3(0.0);
        vec3 V = normalize(cameraPos - pos);
        for (int i = 0; i < numLights; ++i) {
            vec3 L = lightDirTo(lights[i], pos);
            float dotNL = max(dot(normal, L), 0.0);
            if (useToonShading) {
                dotNL = round(dotNL * toonLevels) / toonLevels;
            }
            diffuseOut += dotNL * lights[i].intensity * lights[i].color;

            vec3 R = reflect(-L, normal);
            float spec = pow(max(dot(R, V), 0.0), 10.0);
            if (useToonShading) {
                spec = spec > 0.5 ? 1.0 : 0.0;
            }
            specularOut += spec * lights[i].intensity * lights[i].color;
        }
    }

    void main() {
        vec3 localPos = aPos + (renderSilhouette ? aNormal * silhouetteThickness : vec3(0.0));
        if (useVertexAnimation) {
            float phase = aPos.x * 3.0 + aPos.y * 1.5 + aPos.z * 2.0;
            float wave = sin(time * vertexAnimSpeed + phase);
            localPos += aNormal * wave * vertexAnimAmplitude;
        }
        objectPos = aPos;
        vec4 world = model * vec4(localPos, 1.0);
        worldPos = world.xyz;
        smoothNormal = normalize(normalMatrix * aNormal);
        flatFaceNormal = normalize(normalMatrix * aFaceNormal);
        worldTangent = normalize(normalMatrix * aTangent);
        worldBitangent = normalize(normalMatrix * aBitangent);
        uvOut = uvMode == 0 ? aUV : generateUV(aPos);

        vertexDiffuse = vec3(0.0);
        vertexSpecular = vec3(0.0);
        if (shadingMode == 1) {
            computeLighting(smoothNormal, worldPos, vertexDiffuse, vertexSpecular);
        }

        gl_Position = projection * view * world;
    }
    )";

inline constexpr const char* glModelFragmentShaderSource = R"(
    #version 410 core
    in vec3 worldPos;
    in vec3 objectPos;
    in vec3 smoothNormal;
    in vec2 uvOut;
    flat in vec3 flatFaceNormal;
    in vec3 vertexDiffuse;
    in vec3 vertexSpecular;
    in vec3 worldTangent;
    in vec3 worldBitangent;

    out vec4 FragColor;

    uniform int shadingMode; // 0 = Flat, 1 = Gouraud, 2 = Phong
    uniform vec3 cameraPos;
    uniform vec3 ambientLight;

    struct Light {
        int type;
        vec3 direction;
        vec3 position;
        vec3 color;
        float intensity;
    };
    #define MAX_LIGHTS 8
    uniform Light lights[MAX_LIGHTS];
    uniform int numLights;

    uniform vec3 matDiffuse;
    uniform vec3 matSpecular;
    uniform vec3 matEmissive;
    uniform bool matUseNonUniform;
    uniform vec3 matDiffuseSecondary;

    uniform bool useDiffuseTex;
    uniform sampler2D diffuseTex;
    uniform bool useSpecularTex;
    uniform sampler2D specularTex;
    uniform bool useNormalMapTex;
    uniform sampler2D normalMapTex;

    uniform int envMapMode; // 0 = Off, 1 = Reflect, 2 = Refract
    uniform float envMapStrength;
    uniform float envMapIOR;
    uniform samplerCube envMap;

    uniform bool useToonShading;
    uniform float toonLevels;

    uniform bool renderSilhouette;
    uniform vec3 silhouetteColor;

    // Procedural texture: 0 = off, 1 = marble, 2 = wood.
    uniform int proceduralTexMode;
    uniform float proceduralTexScale;

    // Color animation: 0 = off, 1 = cycles the material's hue over time, 2 = pulses its brightness.
    uniform int colorAnimMode;
    uniform float colorAnimSpeed;
    uniform float time;

    vec3 rgb2hsv(vec3 c) {
        vec4 K = vec4(0.0, -1.0 / 3.0, 2.0 / 3.0, -1.0);
        vec4 p = mix(vec4(c.bg, K.wz), vec4(c.gb, K.xy), step(c.b, c.g));
        vec4 q = mix(vec4(p.xyw, c.r), vec4(c.r, p.yzx), step(p.x, c.r));
        float d = q.x - min(q.w, q.y);
        float e = 1.0e-10;
        return vec3(abs(q.z + (q.w - q.y) / (6.0 * d + e)), d / (q.x + e), q.x);
    }

    vec3 hsv2rgb(vec3 c) {
        vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
        vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
        return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
    }

    vec3 animateColor(vec3 baseColor) {
        if (colorAnimMode == 1) {
            vec3 hsv = rgb2hsv(baseColor);
            hsv.x = fract(hsv.x + time * colorAnimSpeed * 0.1);
            return hsv2rgb(hsv);
        }
        if (colorAnimMode == 2) {
            float pulse = 0.5 + 0.5 * sin(time * colorAnimSpeed * 2.0);
            return baseColor * mix(0.4, 1.0, pulse);
        }
        return baseColor;
    }

    vec3 lightDirTo(Light light, vec3 pos) {
        return light.type == 0 ? normalize(light.direction) : normalize(light.position - pos);
    }

    void computeLighting(vec3 normal, vec3 pos, out vec3 diffuseOut, out vec3 specularOut) {
        diffuseOut = vec3(0.0);
        specularOut = vec3(0.0);
        vec3 V = normalize(cameraPos - pos);
        for (int i = 0; i < numLights; ++i) {
            vec3 L = lightDirTo(lights[i], pos);
            float dotNL = max(dot(normal, L), 0.0);
            if (useToonShading) {
                dotNL = round(dotNL * toonLevels) / toonLevels;
            }
            diffuseOut += dotNL * lights[i].intensity * lights[i].color;

            vec3 R = reflect(-L, normal);
            float spec = pow(max(dot(R, V), 0.0), 10.0);
            if (useToonShading) {
                spec = spec > 0.5 ? 1.0 : 0.0;
            }
            specularOut += spec * lights[i].intensity * lights[i].color;
        }
    }

    // GLSL core profile dropped the old built-in noise() functions (and no
    // driver actually implemented them), so procedural texturing needs its
    // own noise: a standard hash-based 3D value noise, smoothed with the
    // usual quintic-free Hermite (3t^2-2t^3) interpolation between the 8
    // corners of the cell containing p.
    float hash3(vec3 p) {
        p = fract(p * 0.3183099 + vec3(0.1, 0.2, 0.3));
        p *= 17.0;
        return fract(p.x * p.y * p.z * (p.x + p.y + p.z));
    }

    float valueNoise(vec3 p) {
        vec3 i = floor(p);
        vec3 f = fract(p);
        f = f * f * (3.0 - 2.0 * f);

        float n000 = hash3(i + vec3(0.0, 0.0, 0.0));
        float n100 = hash3(i + vec3(1.0, 0.0, 0.0));
        float n010 = hash3(i + vec3(0.0, 1.0, 0.0));
        float n110 = hash3(i + vec3(1.0, 1.0, 0.0));
        float n001 = hash3(i + vec3(0.0, 0.0, 1.0));
        float n101 = hash3(i + vec3(1.0, 0.0, 1.0));
        float n011 = hash3(i + vec3(0.0, 1.0, 1.0));
        float n111 = hash3(i + vec3(1.0, 1.0, 1.0));

        float nx00 = mix(n000, n100, f.x);
        float nx10 = mix(n010, n110, f.x);
        float nx01 = mix(n001, n101, f.x);
        float nx11 = mix(n011, n111, f.x);
        float nxy0 = mix(nx00, nx10, f.y);
        float nxy1 = mix(nx01, nx11, f.y);
        return mix(nxy0, nxy1, f.z) * 2.0 - 1.0; // remap [0,1] -> [-1,1]
    }

    // Turbulence
    float turbulence(vec3 p) {
        float sum = 0.0;
        float freq = 1.0;
        float amp = 1.0;
        for (int i = 0; i < 4; ++i) {
            sum += abs(valueNoise(p * freq)) * amp;
            freq *= 2.0;
            amp *= 0.5;
        }
        return sum;
    }

    // marble(p): x = p.x + turbulence(p), color banded by sin(x).
    vec3 marbleTexture(vec3 p) {
        float x = p.x + turbulence(p) * 4.0;
        float band = sin(x * 3.14159265);
        vec3 veinColor = vec3(0.15, 0.15, 0.18);
        vec3 baseColor = vec3(0.9, 0.88, 0.85);
        return mix(veinColor, baseColor, smoothstep(-0.2, 0.6, band));
    }

    // wood(p): x = (p.x^2 + p.z^2) + turbulence(p), same idea but radial
    // around the object's up axis so the rings curve like real growth
    // rings instead of running in straight bands.
    vec3 woodTexture(vec3 p) {
        float x = (p.x * p.x + p.z * p.z) + turbulence(p) * 2.0;
        float rings = fract(sin(x * 3.14159265) * 0.5 + 0.5);
        vec3 darkColor = vec3(0.32, 0.18, 0.07);
        vec3 lightColor = vec3(0.62, 0.42, 0.22);
        return mix(darkColor, lightColor, smoothstep(0.0, 1.0, rings));
    }

    void main() {
        if (renderSilhouette) {
            FragColor = vec4(silhouetteColor, 1.0);
            return;
        }

        vec3 diffuseLight;
        vec3 specularLight;
        vec3 N;

        if (shadingMode == 1) {
            diffuseLight = vertexDiffuse;
            specularLight = vertexSpecular;
            N = normalize(smoothNormal);
        } else {
            N = (shadingMode == 0) ? flatFaceNormal : normalize(smoothNormal);
            if (useNormalMapTex) {
                vec3 T = normalize(worldTangent);
                vec3 B = normalize(worldBitangent);
                vec3 mapNormal = texture(normalMapTex, uvOut).rgb * 2.0 - 1.0;
                N = normalize(T * mapNormal.x + B * mapNormal.y + N * mapNormal.z);
            }
            computeLighting(N, worldPos, diffuseLight, specularLight);
        }

        vec3 texColor;
        if (proceduralTexMode == 1) {
            texColor = marbleTexture(objectPos * proceduralTexScale);
        } else if (proceduralTexMode == 2) {
            texColor = woodTexture(objectPos * proceduralTexScale);
        } else {
            texColor = useDiffuseTex ? texture(diffuseTex, uvOut).rgb : vec3(1.0);
        }
        float specSample = useSpecularTex ? texture(specularTex, uvOut).r : 1.0;

        vec3 effectiveDiffuse = matUseNonUniform
            ? mix(matDiffuse, matDiffuseSecondary, uvOut.x)
            : matDiffuse;
        effectiveDiffuse = animateColor(effectiveDiffuse);

        vec3 ambientTerm = ambientLight * effectiveDiffuse * texColor;
        vec3 diffuseTerm = diffuseLight * effectiveDiffuse * texColor;
        vec3 specularTerm = specularLight * matSpecular * specSample;
        vec3 emissiveTerm = matEmissive;

        vec3 total = ambientTerm + diffuseTerm + specularTerm + emissiveTerm;

        if (envMapMode != 0) {
            vec3 V = normalize(cameraPos - worldPos);
            vec3 sampleDir = (envMapMode == 1) ? reflect(-V, N) : refract(-V, N, 1.0 / envMapIOR);
            vec3 envColor = texture(envMap, sampleDir).rgb;
            total = mix(total, envColor, envMapStrength);
        }

        FragColor = vec4(min(total, vec3(1.0)), 1.0);
    }
    )";

#endif //RENDERER_GLPHONGSHADER_H
