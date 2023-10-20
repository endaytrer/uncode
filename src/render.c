#define GL_GLEXT_PROTOTYPES
#include <GL/glew.h>
#include <GL/gl.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "render.h"
#include "freetype.h"
#include "editor.h"



GLuint font_tex;

GLuint text_vao;
GLuint text_vbo;
GLuint text_program;

GLuint rect_vao;
GLuint rect_vbo;
GLuint rect_program;

time_t start_sec;
float last_edit;
float delta_t;

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
GLuint create_shader_program(const char *vs_source, unsigned int vs_length, const char *fs_source, unsigned int fs_length) {
    GLuint program = glCreateProgram();
    const GLint signed_vs_length = (GLint)vs_length;
    const GLint signed_fs_length = (GLint)fs_length;
    GLuint vertex_shader = compile_shader(GL_VERTEX_SHADER, vs_source, &signed_vs_length);
    GLuint fragment_shader = compile_shader(GL_FRAGMENT_SHADER, fs_source, &signed_fs_length);
    
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
    return program;
    glUseProgram(program);

    glGenBuffers(1, &text_vbo);
    glGenVertexArrays(1, &text_vao);
}

void update(GdkFrameClock *self, GtkGLArea *area) {
    (void)self;

    double fps = gdk_frame_clock_get_fps(self);
    if (fps > EPS) {
        delta_t = 1.0f / fps;
    } else {
        delta_t = 1.0f / 60;
    }
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
    
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // compile and link shader
    text_program = create_shader_program((const char *)shaders_glyph_vert_glsl, shaders_glyph_vert_glsl_len, (const char *)shaders_glyph_frag_glsl, shaders_glyph_frag_glsl_len);
    glUseProgram(text_program);

    glGenBuffers(1, &text_vbo);
    glGenVertexArrays(1, &text_vao);

    rect_program = create_shader_program((const char *)shaders_rect_vert_glsl, shaders_rect_vert_glsl_len, (const char *)shaders_rect_frag_glsl, shaders_rect_frag_glsl_len);
    glUseProgram(rect_program);

    glGenBuffers(1, &rect_vao);
    glGenVertexArrays(1, &rect_vbo);

    struct timeval start_time;
    gettimeofday(&start_time, NULL);
    start_sec = start_time.tv_sec;
    
    GdkFrameClock *frame_clock = gtk_widget_get_frame_clock(GTK_WIDGET(area));
    g_signal_connect(frame_clock, "update", G_CALLBACK(update), area);
    gdk_frame_clock_begin_updating(frame_clock);
    delta_t = 1.0f / 60;
}

void unrealize(GtkGLArea *area) {
    gtk_gl_area_make_current(area);

    if (gtk_gl_area_get_error (area) != NULL)
        return;


    GdkFrameClock *frame_clock = gtk_widget_get_frame_clock(GTK_WIDGET(area));
    gdk_frame_clock_end_updating(frame_clock);

    glDeleteBuffers(1, &text_vbo);
    glDeleteProgram(text_program);

    glDeleteBuffers(1, &rect_vbo);
    glDeleteProgram(rect_program);
}

gboolean render(GtkGLArea *area, GdkGLContext *context) {
    (void)context;
    if (gtk_gl_area_get_error(area) != NULL) 
        return FALSE;
    viewport_size[0] = gtk_widget_get_width(GTK_WIDGET(area));
    viewport_size[1] = gtk_widget_get_height(GTK_WIDGET(area));
    if (viewport_velocity[0] * viewport_velocity[0] + viewport_velocity[1] * viewport_velocity[1] > 0.01) {
        viewport_pos[0] += delta_t * viewport_velocity[0];
        viewport_pos[1] += delta_t * viewport_velocity[1];
        // apply drag force

        viewport_velocity[0] *= DRAG;
        viewport_velocity[1] *= DRAG;

        layout_updated = true;
        
    } else {
        viewport_velocity[0] = 0;
        viewport_velocity[1] = 0;
    }

    struct timeval time;
    gettimeofday(&time, NULL);
    time.tv_sec -= start_sec;

    adjust_screen_text_area(&main_editor);
    calculate(&main_editor, (float)time.tv_usec / 1000000 + time.tv_sec - last_edit);

    const float bg_color[] = BG_COLOR;
    glClearColor(bg_color[0], bg_color[1], bg_color[2], 1.0);
    glClear(GL_COLOR_BUFFER_BIT);


    if (calculated_character_size > 0) {
        glUseProgram(text_program);
        glBindVertexArray(text_vao);

        glBindBuffer(GL_ARRAY_BUFFER, text_vbo);
        glBufferData(GL_ARRAY_BUFFER, calculated_character_size * sizeof(Glyph), calculated_characters, GL_DYNAMIC_DRAW);

        FILL_ATTR_GLYPH();

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
    }
    
    // draw rects
    if (num_rects > 0) {

        glUseProgram(rect_program);

        glBindVertexArray(rect_vao);
        glBindBuffer(GL_ARRAY_BUFFER, rect_vbo);
        glBufferData(GL_ARRAY_BUFFER, num_rects * sizeof(Rect), rects, GL_DYNAMIC_DRAW);

        // cursor attribs
        FILL_ATTR_RECT();

        // uniforms: viewport_size, viewport_pos, viewport_scale

        GLint uniformResolution = glGetUniformLocation(rect_program, "viewport_size");
        glUniform2f(uniformResolution, viewport_size[0], viewport_size[1]);

        GLint uniformViewportPos = glGetUniformLocation(rect_program, "viewport_pos");
        glUniform2f(uniformViewportPos, viewport_pos[0], viewport_pos[1]);

        GLint uniformViewportScale = glGetUniformLocation(rect_program, "viewport_scale");
        glUniform1i(uniformViewportScale, viewport_scale);

        glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, num_rects);
    }

    glFlush();
    return TRUE;
}