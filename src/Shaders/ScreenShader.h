#ifndef RENDERER_SCREENSHADER_H
#define RENDERER_SCREENSHADER_H

inline constexpr const char* vertexShaderSource = R"(
    #version 410 core
    layout (location = 0) in vec3 aPos;
    layout (location = 1) in vec2 aTexCoord;
    out vec2 TexCoord;
    void main()
    {
       gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);
       TexCoord = aTexCoord;
    }
    )";


inline constexpr const char* fragmentShaderSource = R"(
    #version 410 core
    out vec4 FragColor;
    in vec2 TexCoord;
    uniform sampler2D screenTexture;
    void main()
    {
       FragColor = texture(screenTexture, TexCoord);
    }
    )";

#endif //RENDERER_SCREENSHADER_H