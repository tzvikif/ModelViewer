#include <OpenGLES/ES2/gl.h>
#include <OpenGLES/ES2/glext.h>
#include "Interfaces.hpp"
#include "Matrix.hpp"
#include <iostream>

namespace ES2 {
    
#define STRINGIFY(A)  #A
#include "../Shaders/SimpleLighting.vert"
#include "../Shaders/Simple.es2.frag"
    
    struct UniformHandles {
        GLuint Modelview;
        GLuint Projection;
        GLuint NormalMatrix;
        GLuint LightPosition;
        GLint AmbientMaterial;
        GLint SpecularMaterial;
        GLint Shininess;
        GLint Sampler;
    };

    struct AttributeHandles {
        GLint Position;
        GLint Normal;
        GLint Ambient;
        GLint Diffuse;
        GLint Specular;
        GLint Shininess;
        GLint TextureCoord;
    };
    
    struct Drawable {
        GLuint VertexBuffer;
        GLuint IndexBuffer;
        //GLuint lineIndexBuffer;
        int IndexCount;
        //int lineIndexCount;
    }; 
    class RenderingEngine : public IRenderingEngine {
    public:
        RenderingEngine();
        void Initialize(const vector<ISurface*>& surfaces);
        void Render(const vector<Visual>& visuals) const;
    private:
        GLuint BuildShader(const char* source, GLenum shaderType) const;
        GLuint BuildProgram(const char* vShader, const char* fShader) const;
        vector<ES2::Drawable> m_drawables;
        GLuint m_colorRenderbuffer;
        GLuint m_depthRenderbuffer;
        GLuint m_gridTexture;
        mat4 m_translation;
        UniformHandles m_uniforms;
        AttributeHandles m_attributes;
        IResourceManager* m_resourceManager;
    };
    
    IRenderingEngine* CreateRenderingEngine(IResourceManager* resourceManager)
    {
        return new RenderingEngine();
    }
    
    RenderingEngine::RenderingEngine()
    {
        glGenRenderbuffers(1, &m_colorRenderbuffer);
        glBindRenderbuffer(GL_RENDERBUFFER, m_colorRenderbuffer);
    }
    
    void RenderingEngine::Initialize(const vector<ISurface*>& surfaces)
    {
        vector<ISurface*>::const_iterator surface;
        for (surface = surfaces.begin(); surface != surfaces.end(); ++surface) {
            
            // Create the VBO for the vertices.
            vector<float> vertices;
            (*surface)->GenerateVertices(vertices, VertexFlagsNormals|VertexFlagsTexCoords);
            GLuint vertexBuffer;
            glGenBuffers(1, &vertexBuffer);
            glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
            glBufferData(GL_ARRAY_BUFFER,
                         vertices.size() * sizeof(vertices[0]),
                         &vertices[0],
                         GL_STATIC_DRAW);
            
            // Create a new VBO for the indices if needed.
            int indexCount = (*surface)->GetTriangleIndexCount();
            GLuint indexBuffer;
            if (!m_drawables.empty() && indexCount == m_drawables[0].IndexCount) {
                indexBuffer = m_drawables[0].IndexBuffer;
            } else {
                vector<GLushort> indices(indexCount);
                (*surface)->GenerateTriangleIndices(indices);
                glGenBuffers(1, &indexBuffer);
                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);
                glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                             indexCount * sizeof(GLushort),
                             &indices[0],
                             GL_STATIC_DRAW);
            }
            int lineIndexCount = (*surface)->GetLineIndexCount();
            GLuint lineIndexBuffer;
            vector<GLushort> lineIndices(lineIndexCount);
            (*surface)->GenerateLineIndices(lineIndices);
            glGenBuffers(1, &lineIndexBuffer);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, lineIndexBuffer);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                         lineIndexCount * sizeof(GLushort),
                         &lineIndices[0],
                         GL_STATIC_DRAW);
            
            Drawable drawable = { vertexBuffer, indexBuffer , indexCount};
            m_drawables.push_back(drawable);
        }
        
        // Extract width and height.
        int width, height;
        glGetRenderbufferParameteriv(GL_RENDERBUFFER,
                                     GL_RENDERBUFFER_WIDTH, &width);
        glGetRenderbufferParameteriv(GL_RENDERBUFFER,
                                     GL_RENDERBUFFER_HEIGHT, &height);
        
        // Create a depth buffer that has the same size as the color buffer.
        glGenRenderbuffers(1, &m_depthRenderbuffer);
        glBindRenderbuffer(GL_RENDERBUFFER, m_depthRenderbuffer);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, width, height);
        
        // Create the framebuffer object.
        GLuint framebuffer;
        glGenFramebuffers(1, &framebuffer);
        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                  GL_RENDERBUFFER, m_colorRenderbuffer);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                                  GL_RENDERBUFFER, m_depthRenderbuffer);
        glBindRenderbuffer(GL_RENDERBUFFER, m_colorRenderbuffer);
        
        // Create the GLSL program.
        GLuint program = BuildProgram(SimpleVertexShader, SimpleFragmentShader);
        glUseProgram(program);
        
        // Extract the handles to attributes and uniforms.
        m_attributes.Position = glGetAttribLocation(program, "Position");
        m_attributes.Normal = glGetAttribLocation(program, "Normal");
        m_attributes.Diffuse = glGetAttribLocation(program, "DiffuseMaterial");
        m_attributes.TextureCoord = glGetAttribLocation(program, "TextureCoord");
        m_uniforms.Projection = glGetUniformLocation(program, "Projection");
        m_uniforms.Modelview = glGetUniformLocation(program, "Modelview");
        m_uniforms.NormalMatrix = glGetUniformLocation(program, "NormalMatrix");
        m_uniforms.LightPosition = glGetUniformLocation(program, "LightPosition");
        m_uniforms.AmbientMaterial = glGetUniformLocation(program, "AmbientMaterial");
        m_uniforms.SpecularMaterial = glGetUniformLocation(program, "SpecularMaterial");
        m_uniforms.Shininess = glGetUniformLocation(program, "Shininess"); 
        
        // Set up some default material parameters.
        glUniform3f(m_uniforms.AmbientMaterial, 0.04f, 0.04f, 0.04f);
        glUniform3f(m_uniforms.SpecularMaterial, 0.5, 0.5, 0.5);
        glUniform1f(m_uniforms.Shininess, 50);
        // Set the active sampler to stage 0.  Not really necessary since the uniform
        // defaults to zero anyway, but good practice.
        glActiveTexture(GL_TEXTURE0);
        glUniform1i(m_uniforms.Sampler, 0);
        // Load the texture.
        glGenTextures(1,&m_gridTexture);
        glBindTexture(GL_TEXTURE_2D, m_gridTexture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        m_resourceManager = CreateResourceManager();
        m_resourceManager->LoadPngImage("Grid16.png");
        void* pixels = m_resourceManager->GetImageData();
        ivec2 size = m_resourceManager->GetImageSize();
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, size.x, 
                     size.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
        m_resourceManager->UnloadImage();
        // Initialize various state.
        glEnableVertexAttribArray(m_attributes.Position);
        glEnableVertexAttribArray(m_attributes.Normal);
        glEnableVertexAttribArray(m_attributes.TextureCoord);
        glEnable(GL_DEPTH_TEST);
        
        // Set up transforms.
        m_translation = mat4::Translate(0, 0, -7);
    }
    
    void RenderingEngine::Render(const vector<Visual>& visuals) const
    {
        glClearColor(0.5f, 0.5f, 0.5f, 1);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        vector<Visual>::const_iterator visual = visuals.begin();
        for (int visualIndex = 0; visual != visuals.end(); ++visual, ++visualIndex) {
            
            // Set the viewport transform.
            ivec2 size = visual->ViewportSize;
            ivec2 lowerLeft = visual->LowerLeft;
            glViewport(lowerLeft.x, lowerLeft.y, size.x, size.y);
            
            // Set the light position.
            vec4 lightPosition(0.25, 0.25, 1, 0);
            glUniform3fv(m_uniforms.LightPosition, 1, lightPosition.Pointer());
            
            // Set the model-view transform.
            mat4 rotation = visual->Orientation.ToMatrix();
            mat4 modelview = rotation * m_translation;
            glUniformMatrix4fv(m_uniforms.Modelview, 1, 0, modelview.Pointer());
            
            // Set the normal matrix.
            // It's orthogonal, so its Inverse-Transpose is itself!
            mat3 normalMatrix = modelview.ToMat3();
            glUniformMatrix3fv(m_uniforms.NormalMatrix, 1, 0, normalMatrix.Pointer());
            
            // Set the projection transform.
            float h = 4.0f * size.y / size.x;
            mat4 projectionMatrix = mat4::Frustum(-2, 2, -h / 2, h / 2, 5, 10);
            glUniformMatrix4fv(m_uniforms.Projection, 1, 0, projectionMatrix.Pointer());
            
            // Set the diffuse color.
            vec3 color = visual->Color * 0.75f;
            glVertexAttrib4f(m_attributes.Diffuse, color.x, color.y, color.z, 1);
            
            // Draw the surface.
            int stride = sizeof(vec3) + sizeof(vec3) + sizeof(vec2);
            const GLvoid* normalOffset = (const GLvoid*) sizeof(vec3);
            const GLvoid* texCoordOffset = (const GLvoid*) (2 * sizeof(vec3));
            GLint position = m_attributes.Position;
            GLint normal = m_attributes.Normal;
            GLint texCoord = m_attributes.TextureCoord;
            const Drawable& drawable = m_drawables[visualIndex];
            glBindBuffer(GL_ARRAY_BUFFER, drawable.VertexBuffer);
            glVertexAttribPointer(position, 3, GL_FLOAT, GL_FALSE, stride, 0);
            glVertexAttribPointer(normal, 3, GL_FLOAT, GL_FALSE, stride, normalOffset);
            glVertexAttribPointer(texCoord, 2, GL_FLOAT, GL_FALSE, stride, texCoordOffset);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, drawable.IndexBuffer);
            glDrawElements(GL_TRIANGLES, drawable.IndexCount, GL_UNSIGNED_SHORT, 0);
        }
    }
    
    GLuint RenderingEngine::BuildShader(const char* source, GLenum shaderType) const
    {
        GLuint shaderHandle = glCreateShader(shaderType);
        glShaderSource(shaderHandle, 1, &source, 0);
        glCompileShader(shaderHandle);
        
        GLint compileSuccess;
        glGetShaderiv(shaderHandle, GL_COMPILE_STATUS, &compileSuccess);
        
        if (compileSuccess == GL_FALSE) {
            GLchar messages[256];
            glGetShaderInfoLog(shaderHandle, sizeof(messages), 0, &messages[0]);
            std::cout << messages;
            exit(1);
        }
        
        return shaderHandle;
    }
    
    GLuint RenderingEngine::BuildProgram(const char* vertexShaderSource,
                                         const char* fragmentShaderSource) const
    {
        GLuint vertexShader = BuildShader(vertexShaderSource, GL_VERTEX_SHADER);
        GLuint fragmentShader = BuildShader(fragmentShaderSource, GL_FRAGMENT_SHADER);
        
        GLuint programHandle = glCreateProgram();
        glAttachShader(programHandle, vertexShader);
        glAttachShader(programHandle, fragmentShader);
        glLinkProgram(programHandle);
        
        GLint linkSuccess;
        glGetProgramiv(programHandle, GL_LINK_STATUS, &linkSuccess);
        if (linkSuccess == GL_FALSE) {
            GLchar messages[256];
            glGetProgramInfoLog(programHandle, sizeof(messages), 0, &messages[0]);
            std::cout << messages;
            exit(1);
        }
        
        return programHandle;
    }
}

