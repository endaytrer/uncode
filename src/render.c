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
    VERTEX_UV_OFFSET_X,
    VERTEX_UV_SIZE,
    VERTEX_FG_COLOR,
    VERTEX_BG_COLOR,
    VERTEX_ATTR_COUNT
} VertexAttr;

static const struct {
    size_t offset;
    size_t size;
} vertex_attr_param[VERTEX_ATTR_COUNT] = {
    [VERTEX_POS] = 
    {
        .size = sizeof(((Glyph*)NULL)->pos) / sizeof(float),
        .offset = offsetof(Glyph, pos),
    },
    [VERTEX_SIZE] = 
    {
        .size = sizeof(((Glyph*)NULL)->size) / sizeof(float),
        .offset = offsetof(Glyph, size),
    },
    [VERTEX_UV_OFFSET_X] = 
    {
        .size = sizeof(((Glyph*)NULL)->uv_offset_x) / sizeof(float),
        .offset = offsetof(Glyph, uv_offset_x),
    },
    [VERTEX_UV_SIZE] = 
    {
        .size = sizeof(((Glyph*)NULL)->uv_size) / sizeof(float),
        .offset = offsetof(Glyph, uv_size),
    },
    [VERTEX_FG_COLOR] = 
    {
        .size = sizeof(((Glyph*)NULL)->fg_color) / sizeof(float),
        .offset = offsetof(Glyph, fg_color),
    },
    [VERTEX_BG_COLOR] = 
    {
        .size = sizeof(((Glyph*)NULL)->bg_color) / sizeof(float),
        .offset = offsetof(Glyph, bg_color),
    }
};

static_assert(VERTEX_ATTR_COUNT == 6, "unimplemented");

GLuint font_tex;

GLuint text_vao;
GLuint text_vbo;

GLuint text_program;

GLuint cursor_vao;
GLuint cursor_vbo;

GLuint cursor_program;

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

    font_tex = freetype_init();

    init_editor(&main_editor);
    calculate(&main_editor);

    // compile and link shader
    text_program = glCreateProgram();
    GLuint vertex_shader = compile_shader(GL_VERTEX_SHADER, (const char *)shaders_text_vert_glsl, (const int *)&shaders_text_vert_glsl_len);
    GLuint fragment_shader = compile_shader(GL_FRAGMENT_SHADER, (const char *)shaders_text_frag_glsl, (const int *)&shaders_text_frag_glsl_len);
    
    glAttachShader(text_program, vertex_shader);
    glAttachShader(text_program, fragment_shader);
    glLinkProgram(text_program);


    GLint status;
    glGetProgramiv(text_program, GL_LINK_STATUS, &status);

    if (status == GL_FALSE) {
        GLint max_length = 0;
        glGetProgramiv(text_program, GL_INFO_LOG_LENGTH, &max_length);

        char *error_log = malloc(max_length);
        glGetProgramInfoLog(text_program, max_length, &max_length, error_log);
        fprintf(stderr, "Cannot link text_program:\n%s\n", error_log);
        free(error_log);
        glDeleteProgram(text_program);

        glDeleteShader(vertex_shader);
        glDeleteShader(fragment_shader);

        exit(-1);
    }
    glUseProgram(text_program);

    glGenBuffers(1, &text_vbo);
    glGenVertexArrays(1, &text_vao);


    cursor_program = glCreateProgram();
    vertex_shader = compile_shader(GL_VERTEX_SHADER, (const char *)shaders_cursor_vert_glsl, (const int *)&shaders_cursor_vert_glsl_len);
    fragment_shader = compile_shader(GL_FRAGMENT_SHADER, (const char *)shaders_cursor_frag_glsl, (const int *)&shaders_cursor_frag_glsl_len);
    
    glAttachShader(cursor_program, vertex_shader);
    glAttachShader(cursor_program, fragment_shader);
    glLinkProgram(cursor_program);

    glGetProgramiv(cursor_program, GL_LINK_STATUS, &status);

    if (status == GL_FALSE) {
        GLint max_length = 0;
        glGetProgramiv(cursor_program, GL_INFO_LOG_LENGTH, &max_length);

        char *error_log = malloc(max_length);
        glGetProgramInfoLog(cursor_program, max_length, &max_length, error_log);
        fprintf(stderr, "Cannot link cursor_program:\n%s\n", error_log);
        free(error_log);
        glDeleteProgram(cursor_program);

        glDeleteShader(vertex_shader);
        glDeleteShader(fragment_shader);

        exit(-1);
    }
    glUseProgram(cursor_program);

    glGenBuffers(1, &cursor_vbo);
    glGenVertexArrays(1, &cursor_vao);
}

void unrealize(GtkGLArea *area) {
    gtk_gl_area_make_current(area);

    if (gtk_gl_area_get_error (area) != NULL)
        return;

    glDeleteBuffers(1, &text_vbo);
    glDeleteProgram(text_program);

    glDeleteBuffers(1, &cursor_vbo);
    glDeleteProgram(cursor_program);
}

gboolean render(GtkGLArea *area, GdkGLContext *context) {
    if (gtk_gl_area_get_error(area) != NULL) 
        return FALSE;
    
    glClearColor(0.0, 0.0, 0.0, 1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    struct timeval time;
    gettimeofday(&time, NULL);

    glUseProgram(text_program);
    glBindVertexArray(text_vao);

    glBindBuffer(GL_ARRAY_BUFFER, text_vbo);
    glBufferData(GL_ARRAY_BUFFER, calculated_character_size * sizeof(Glyph), calculated_characters, GL_DYNAMIC_DRAW);
    for (VertexAttr attr = VERTEX_POS; attr < VERTEX_ATTR_COUNT; attr++) {
        glVertexAttribPointer(attr,
            vertex_attr_param[attr].size,
            GL_FLOAT,
            GL_FALSE,
            sizeof(Glyph),
            (void *)vertex_attr_param[attr].offset);
        glEnableVertexAttribArray(attr);
        glVertexAttribDivisor(attr, 1);
    }

    // uniforms
    GLint uniformTime = glGetUniformLocation(text_program, "time");
    glUniform1f(uniformTime, (float)time.tv_usec / 1000000 * G_PI * 2);

    GLint uniformResolution = glGetUniformLocation(text_program, "resolution");
    glUniform2f(uniformResolution, gtk_widget_get_width(GTK_WIDGET(area)), gtk_widget_get_height(GTK_WIDGET(area)));

    glBindTexture(GL_TEXTURE_2D, font_tex);
    GLuint uniformFont = glGetUniformLocation(text_program, "font");
    glUniform1i(uniformFont, FONT_TEXTURE - GL_TEXTURE0);
    
    glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, calculated_character_size);

    // draw cursor
    glUseProgram(cursor_program);

    glBindVertexArray(cursor_vao);
    glBindBuffer(GL_ARRAY_BUFFER, cursor_vbo);
    glBufferData(GL_ARRAY_BUFFER, num_cursors * sizeof(Cursor), cursors, GL_DYNAMIC_DRAW);

    // cursor attribs
    GLint posAttrib = glGetAttribLocation(cursor_program, "position");
    glVertexAttribPointer(posAttrib, 2, GL_FLOAT, GL_FALSE, sizeof(Cursor), NULL);
    glEnableVertexAttribArray(posAttrib);
    glVertexAttribDivisor(posAttrib, 1);

    // uniforms: time, resolution, color, size
    uniformTime = glGetUniformLocation(cursor_program, "time");
    glUniform1f(uniformTime, (float)time.tv_usec / 1000000 * G_PI * 2);

    uniformResolution = glGetUniformLocation(cursor_program, "resolution");
    glUniform2f(uniformResolution, gtk_widget_get_width(GTK_WIDGET(area)), gtk_widget_get_height(GTK_WIDGET(area)));

    GLuint uniformColor = glGetUniformLocation(cursor_program, "color");
    glUniform4f(uniformColor, cursor_color[0], cursor_color[1], cursor_color[2], cursor_color[3]);
    
    GLuint uniformSize = glGetUniformLocation(cursor_program, "size");
    glUniform2f(uniformSize, cursor_size[0], cursor_size[1]);

    glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, num_cursors);
    
    glFlush();
    return TRUE;
}