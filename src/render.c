#define GL_GLEXT_PROTOTYPES
#include <GL/glew.h>
#include <GL/gl.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
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

time_t start_sec;
float last_edit;
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

void update(GdkFrameClock *self, GtkGLArea *area) {
    (void)self;
    gtk_gl_area_queue_render(area);
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

    viewport_scale = gtk_widget_get_scale_factor(GTK_WIDGET(area));

    font_tex = freetype_init();
    calculate_render_length(&main_editor);
    

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

    struct timeval start_time;
    gettimeofday(&start_time, NULL);
    start_sec = start_time.tv_sec;
    
    GdkFrameClock *frame_clock = gtk_widget_get_frame_clock(GTK_WIDGET(area));
    gdk_frame_clock_begin_updating(frame_clock);
    g_signal_connect(frame_clock, "update", G_CALLBACK(update), area);
}

void unrealize(GtkGLArea *area) {
    gtk_gl_area_make_current(area);

    if (gtk_gl_area_get_error (area) != NULL)
        return;


    GdkFrameClock *frame_clock = gtk_widget_get_frame_clock(GTK_WIDGET(area));
    gdk_frame_clock_end_updating(frame_clock);

    glDeleteBuffers(1, &text_vbo);
    glDeleteProgram(text_program);

    glDeleteBuffers(1, &cursor_vbo);
    glDeleteProgram(cursor_program);
}

gboolean render(GtkGLArea *area, GdkGLContext *context) {
    (void)context;
    if (gtk_gl_area_get_error(area) != NULL) 
        return FALSE;
    viewport_size[0] = gtk_widget_get_width(GTK_WIDGET(area));
    viewport_size[1] = gtk_widget_get_height(GTK_WIDGET(area));
    calculate(&main_editor);

    const float bg_color[] = BG_COLOR;
    glClearColor(bg_color[0], bg_color[1], bg_color[2], 1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    struct timeval time;
    gettimeofday(&time, NULL);
    time.tv_sec -= start_sec;

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
    glUniform1f(uniformTime, (float)time.tv_usec / 1000000 + time.tv_sec);

    GLint uniformResolution = glGetUniformLocation(text_program, "viewport_size");
    glUniform2f(uniformResolution, viewport_size[0], viewport_size[1]);

    GLint uniformViewportPos = glGetUniformLocation(text_program, "viewport_pos");
    glUniform2f(uniformViewportPos, viewport_pos[0], viewport_pos[1]);

    GLint uniformViewportScale = glGetUniformLocation(text_program, "viewport_scale");
    glUniform1i(uniformViewportScale, viewport_scale);

    glBindTexture(GL_TEXTURE_2D, font_tex);
    GLuint uniformFont = glGetUniformLocation(text_program, "font");
    glUniform1i(uniformFont, FONT_TEXTURE - GL_TEXTURE0);
    
    glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, calculated_character_size);
    
    // draw cursor
    if (num_cursors > 0) {
        glUseProgram(cursor_program);

        glBindVertexArray(cursor_vao);
        glBindBuffer(GL_ARRAY_BUFFER, cursor_vbo);
        glBufferData(GL_ARRAY_BUFFER, num_cursors * sizeof(Cursor), cursors, GL_DYNAMIC_DRAW);

        // cursor attribs
        GLint posAttrib = glGetAttribLocation(cursor_program, "position");
        glVertexAttribPointer(posAttrib, 2, GL_FLOAT, GL_FALSE, sizeof(Cursor), NULL);
        glEnableVertexAttribArray(posAttrib);
        glVertexAttribDivisor(posAttrib, 1);

        // uniforms: time, viewport_size, color, size
        uniformTime = glGetUniformLocation(cursor_program, "time");
        glUniform1f(uniformTime, (float)time.tv_usec / 1000000 + time.tv_sec - last_edit);

        uniformResolution = glGetUniformLocation(cursor_program, "viewport_size");
        glUniform2f(uniformResolution, viewport_size[0], viewport_size[1]);

        uniformViewportPos = glGetUniformLocation(cursor_program, "viewport_pos");
        glUniform2f(uniformViewportPos, viewport_pos[0], viewport_pos[1]);

        uniformViewportScale = glGetUniformLocation(cursor_program, "viewport_scale");
        glUniform1i(uniformViewportScale, viewport_scale);

        GLuint uniformFgColor = glGetUniformLocation(cursor_program, "fg_color");
        glUniform4f(uniformFgColor, cursor_color[0], cursor_color[1], cursor_color[2], cursor_color[3]);

        const float bg_color[] = BG_COLOR;
        GLuint uniformBgColor = glGetUniformLocation(cursor_program, "bg_color");
        glUniform4f(uniformBgColor, bg_color[0], bg_color[1], bg_color[2], 1.0);

        GLuint uniformSize = glGetUniformLocation(cursor_program, "size");
        glUniform2f(uniformSize, cursor_size[0], cursor_size[1]);

        glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, num_cursors);
    }

    glFlush();
    gtk_gl_area_queue_render(area);
    return TRUE;
}