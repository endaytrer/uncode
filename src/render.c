#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <sys/time.h>

#include "render.h"
#include "shaders.h"

typedef struct {
    float pos[2];
    float color[3];
} Vertex;

typedef enum {
    VERTEX_POS = 0,
    VERTEX_COLOR,
    VERTEX_ATTR_COUNT
} VertexAttr;

static const struct {
    size_t offset;
    size_t size;
} vertex_attr_param[VERTEX_ATTR_COUNT] = {
    [VERTEX_POS] = 
    {
        .size = sizeof(((Vertex*)NULL)->pos) / sizeof(float),
        .offset = offsetof(Vertex, pos),
    },
    [VERTEX_COLOR] = 
    {
        .size = sizeof(((Vertex*)NULL)->color) / sizeof(float),
        .offset = offsetof(Vertex, color),
    }
};

static_assert(VERTEX_ATTR_COUNT == 2, "unimplemented");

Vertex vertices[] = {
    {
        .pos = {0.0f, 0.5f},
        .color = {1.0f, 0.0f, 0.0f}
    },
    {
        .pos = {0.5f, -0.5f},
        .color = {0.0f, 1.0f, 0.0f}
    },
    {
        .pos = {-0.5f, -0.5f},
        .color = {0.0f, 0.0f, 1.0f}
    }
};
    


GLuint compile_shader(GLenum type, const char *shader_text, const int *shader_length_p) {
    GLuint shader = glCreateShader(type);
    const char *source[] = {(const char *)shader_text};
    glShaderSource(shader, 1, (const char *const *)source, shader_length_p);
    glCompileShader(shader);


    GLint status;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    if (status == GL_FALSE) {
        GLint max_length = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &max_length);

        char *error_log = malloc(max_length);
        glGetShaderInfoLog(shader, max_length, &max_length, error_log);
        fprintf(stderr, "Cannot compile shader:\n%s\n", error_log);
        free(error_log);
        glDeleteShader(shader);
        exit(-1);
    }
    return shader;
}

GLuint vao;
GLuint vbo;
GLuint program;

void realize(GtkGLArea *area) {
    gtk_gl_area_make_current(area);

    if (gtk_gl_area_get_error (area) != NULL)
        return;

    // compile and link shader
    GLuint vertex_shader = compile_shader(GL_VERTEX_SHADER, (const char *)shaders_vert_glsl, (const int *)&shaders_vert_glsl_len);
    GLuint fragment_shader = compile_shader(GL_FRAGMENT_SHADER, (const char *)shaders_frag_glsl, (const int *)&shaders_frag_glsl_len);
    
    program = glCreateProgram();
    glAttachShader(program, vertex_shader);
    glAttachShader(program, fragment_shader);
    glLinkProgram(program);

    GLint status;
    glGetProgramiv(program, GL_LINK_STATUS, &status);

    if (status == GL_FALSE) {
        GLint max_length = 0;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &max_length);

        char *error_log = malloc(max_length);
        glGetProgramInfoLog(program, max_length, &max_length, error_log);
        fprintf(stderr, "Cannot link program:\n%s\n", error_log);
        free(error_log);
        glDeleteProgram(program);

        glDeleteShader(vertex_shader);
        glDeleteShader(fragment_shader);

        exit(-1);
    }
    glUseProgram(program);

    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);

    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

}

void unrealize(GtkGLArea *area) {
    gtk_gl_area_make_current(area);

    if (gtk_gl_area_get_error (area) != NULL)
        return;

    glDeleteBuffers(1, &vbo);
    glDeleteProgram(program);
}

gboolean render(GtkGLArea *area, GdkGLContext *context) {
    if (gtk_gl_area_get_error(area) != NULL) 
        return FALSE;
    
    glClearColor(0.0, 0.0, 0.0, 1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glBindVertexArray(vao);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);

    for (VertexAttr attr = VERTEX_POS; attr < VERTEX_ATTR_COUNT; attr++) {
        glVertexAttribPointer(attr,
            vertex_attr_param[attr].size,
            GL_FLOAT,
            GL_FALSE,
            sizeof(Vertex),
            (void *)vertex_attr_param[attr].offset);
        glEnableVertexAttribArray(attr);
    }
    // GLint posAttrib = glGetAttribLocation(program, "position");
    // glVertexAttribPointer(posAttrib, 2, GL_FLOAT, GL_FALSE, 0, NULL);

    // glEnableVertexAttribArray(posAttrib);
    // uniforms
    GLint uniformTime = glGetUniformLocation(program, "time");
    struct timeval time;
    gettimeofday(&time, NULL);
    glUniform1f(uniformTime, (float)time.tv_usec / 1000000 * G_PI * 2);
    printf("%f\n", (float)time.tv_usec / 1000000 * G_PI * 2);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glFlush();
    gtk_gl_area_queue_render(area);
    return TRUE;
}