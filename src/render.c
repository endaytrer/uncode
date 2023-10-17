#define GL_GLEXT_PROTOTYPES
#include <GL/glew.h>
#include <GL/gl.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <sys/time.h>
#include "render.h"
#include "shaders.h"
#include "freetype.h"
#include "editor.h"
typedef enum {
    VERTEX_POS = 0,
    VERTEX_SIZE,
    VERTEX_CH,
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
    [VERTEX_SIZE] = 
    {
        .size = sizeof(((Vertex*)NULL)->size) / sizeof(float),
        .offset = offsetof(Vertex, size),
    },
    [VERTEX_CH] = 
    {
        .size = sizeof(((Vertex*)NULL)->ch) / sizeof(float),
        .offset = offsetof(Vertex, ch),
    },
    [VERTEX_COLOR] = 
    {
        .size = sizeof(((Vertex*)NULL)->color) / sizeof(float),
        .offset = offsetof(Vertex, color),
    }
};

static_assert(VERTEX_ATTR_COUNT == 4, "unimplemented");

GLuint vao;
GLuint vbo;
GLuint program;
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



void realize(GtkGLArea *area) {
    gtk_gl_area_make_current(area);

    if (gtk_gl_area_get_error (area) != NULL)
        return;

    if (glewInit() != GLEW_OK) {
        fprintf(stderr, "Could not init glew!");
        exit(-1);
    }
    // check GLEW extensions
    if (!GLEW_ARB_draw_instanced || !GLEW_ARB_instanced_arrays) {
        fprintf(stderr, "Unsupported GLEW extension\n");
        exit(-1);
    }
    int major, minor;
    GdkGLContext *context = gtk_gl_area_get_context(area);
    gdk_gl_context_get_version(context, &major, &minor);
    printf("OpenGL Version: %d.%d\n", major, minor);

    freetype_init();

    init_editor(NULL);
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
    glBufferData(GL_ARRAY_BUFFER, calculated_character_size * sizeof(Vertex), calculated_characters, GL_DYNAMIC_DRAW);

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
        glVertexAttribDivisor(attr, 1);
    }
    // GLint posAttrib = glGetAttribLocation(program, "position");
    // glVertexAttribPointer(posAttrib, 2, GL_FLOAT, GL_FALSE, 0, NULL);

    // glEnableVertexAttribArray(posAttrib);
    // uniforms
    GLint uniformTime = glGetUniformLocation(program, "time");
    struct timeval time;
    gettimeofday(&time, NULL);
    glUniform1f(uniformTime, (float)time.tv_usec / 1000000 * G_PI * 2);

    GLint uniformResolution = glGetUniformLocation(program, "resolution");
    glUniform2f(uniformResolution, gtk_widget_get_width(GTK_WIDGET(area)), gtk_widget_get_height(GTK_WIDGET(area)));

    glDrawArrays(GL_TRIANGLES, 0, 3);
    glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, calculated_character_size);
    glFlush();
    gtk_gl_area_queue_render(area);
    return TRUE;
}