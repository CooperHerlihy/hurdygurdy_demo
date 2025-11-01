#include "hurdygurdy.h"

#include "renderer_3d.h"

#define MOUSE_SPEED 0.003f
#define MOVE_SPEED 1.5f

int main(void) {
    hg_init();
    hg_3d_renderer_init();

    hg_window_open(&(HgWindowConfig){
        .title = "Hurdy Gurdy",
        // .width = 800,
        // .height = 600,
        // .windowed = true,
    });

    u32 window_width, window_height;
    hg_window_get_size(&window_width, &window_height);

    HgTexture* target;
    HgTexture* depth_buffer;
    hg_3d_renderer_target_create(window_width, window_height, &target, &depth_buffer);

    f32 camera_fov = (f32)HG_PI / 3.0f;
    hg_3d_renderer_update_projection(camera_fov, (f32)window_width / (f32)window_height, 0.1f, 100.0f);

    HgVec3 camera_position = (HgVec3){0.0f, 0.0f, -1.0f};
    f32 camera_zoom = 1.0f;
    HgQuat camera_rotation = (HgQuat){1.0f, 0.0f, 0.0f, 0.0f};
    hg_3d_renderer_update_view(camera_position, camera_zoom, camera_rotation);

    const HgVertex3D vertices[] = {{
        .position = {-0.5f, -0.5f, 0.0f},
        .normal = {0.0f, 0.0f, -1.0f},
        .tangent = {1.0f, 0.0f, 0.0f, 1.0f},
        .uv = {0.0f, 0.0f},
    }, {
        .position = {-0.5f, 0.5f, 0.0f},
        .normal = {0.0f, 0.0f, -1.0f},
        .tangent = {1.0f, 0.0f, 0.0f, 1.0f},
        .uv = {0.0f, 1.0f},
    }, {
        .position = {0.5f, 0.5f, 0.0f},
        .normal = {0.0f, 0.0f, -1.0f},
        .tangent = {1.0f, 0.0f, 0.0f, 1.0f},
        .uv = {1.0f, 1.0f},
    }, {
        .position = {0.5f, -0.5f, 0.0f},
        .normal = {0.0f, 0.0f, -1.0f},
        .tangent = {1.0f, 0.0f, 0.0f, 1.0f},
        .uv = {1.0f, 0.0f},
    }};
    HgBuffer* vertex_buffer = hg_3d_vertex_buffer_create(vertices, HG_ARRAY_SIZE(vertices));

    const u32 indices[] = {
        0, 1, 2, 2, 3, 0,
    };
    HgBuffer* index_buffer = hg_3d_index_buffer_create(indices, HG_ARRAY_SIZE(indices));

    const u32 texture_data[] = {
        0xffff0000,
        0xff00ff00,
        0xff0000ff,
        0xff00ffff,
    };
    HgTexture* texture = hg_3d_texture_map_create(texture_data, 2, 2, HG_FORMAT_R8G8B8A8_UNORM, false);

    HgClock game_clock;
    (void)hg_clock_tick(&game_clock);
    f64 time_elapsed = 0.0;
    u64 frame_count = 0;

    bool running = true;
    while (running) {
        f64 delta = hg_clock_tick(&game_clock);
        time_elapsed += delta;
        ++frame_count;
        if (time_elapsed > 1.0) {
            HG_LOGF("avg: %fms, fps: %" PRIu64, 1.0e3 / (f64)frame_count, frame_count);
            time_elapsed -= 1.0;
            frame_count = 0;
        }

        hg_process_events();

        if (hg_was_window_closed() || hg_was_key_pressed(HG_KEY_ESCAPE)) {
            running = false;
            continue;
        }

        if (hg_was_window_resized()) {
            hg_window_update_size();
            hg_window_get_size(&window_width, &window_height);

            hg_texture_destroy(target);
            hg_texture_destroy(depth_buffer);
            hg_3d_renderer_target_create(window_width, window_height, &target, &depth_buffer);

            f32 aspect = (f32)window_width / (f32)window_height;
            hg_3d_renderer_update_projection(camera_fov, aspect, 0.1f, 100.0f);
        }

        if (hg_is_key_down(HG_KEY_LMOUSE)) {
            f32 mouse_x, mouse_y;
            hg_get_mouse_delta(&mouse_x, &mouse_y);
            if (mouse_x != 0.0f)
                camera_rotation = hg_qmul(
                    hg_axis_angle((HgVec3){0.0f, 1.0f, 0.0f}, mouse_x * MOUSE_SPEED),
                    camera_rotation
                );
            if (mouse_y != 0.0f)
                camera_rotation = hg_qmul(
                    camera_rotation,
                    hg_axis_angle((HgVec3){-1.0f, 0.0f, 0.0f}, mouse_y * MOUSE_SPEED)
                );
        }

        if (hg_is_key_down(HG_KEY_SPACE)) {
            camera_position = hg_move_camera_first_person(
                camera_position, camera_rotation, (HgVec3){0.0f, -1.0f, 0.0f}, (f32)delta * MOVE_SPEED
            );
        }
        if (hg_is_key_down(HG_KEY_LSHIFT)) {
            camera_position = hg_move_camera_first_person(
                camera_position, camera_rotation, (HgVec3){0.0f, 1.0f, 0.0f}, (f32)delta * MOVE_SPEED
            );
        }
        if (hg_is_key_down(HG_KEY_W)) {
            camera_position = hg_move_camera_first_person(
                camera_position, camera_rotation, (HgVec3){0.0f, 0.0f, 1.0f}, (f32)delta * MOVE_SPEED
            );
        }
        if (hg_is_key_down(HG_KEY_S)) {
            camera_position = hg_move_camera_first_person(
                camera_position, camera_rotation, (HgVec3){0.0f, 0.0f, -1.0f}, (f32)delta * MOVE_SPEED
            );
        }
        if (hg_is_key_down(HG_KEY_A)) {
            camera_position = hg_move_camera_first_person(
                camera_position, camera_rotation, (HgVec3){-1.0f, 0.0f, 0.0f}, (f32)delta * MOVE_SPEED
            );
        }
        if (hg_is_key_down(HG_KEY_D)) {
            camera_position = hg_move_camera_first_person(
                camera_position, camera_rotation, (HgVec3){1.0f, 0.0f, 0.0f}, (f32)delta * MOVE_SPEED
            );
        }

        hg_3d_renderer_update_view(camera_position, camera_zoom, camera_rotation);

        HgError begin_result = hg_frame_begin();
        if (begin_result != HG_SUCCESS) {
            HG_DEBUG("Failed to begin frame");
            continue;
        }

        static f32 time = 0.0f;
        time += (f32)delta * 2.0f;
        if (time > (f32)HG_TAU) {
            time -= (f32)HG_TAU;
        }
        hg_3d_renderer_queue_directional_light((HgVec3){1.0f, 1.0f, 1.0f}, (HgVec3){1.0f, 0.3f, 0.1f}, 0.5f);
        hg_3d_renderer_queue_point_light((HgVec3){cosf(time) * 3.0f, -1.0f, -sinf(time)}, (HgVec3){1.0f, 1.0f, 1.0f}, 5.0f);

        HgModel3D model = {
            .vertex_buffer = vertex_buffer,
            .index_buffer = index_buffer,
            .color_map = texture,
            .normal_map = NULL,
        };

        hg_3d_renderer_queue_model(&model, &(HgTransform3D){
            .position = (HgVec3){-0.2f, 0.0f, -0.2f},
            .scale = (HgVec3){1.0f, 1.0f, 1.0f},
            .rotation = (HgQuat){1.0f, 0.0f, 0.0f, 0.0f},
        });
        hg_3d_renderer_queue_model(&model, &(HgTransform3D){
            .position = (HgVec3){0.2f, 0.0f, 0.2f},
            .scale = (HgVec3){1.0f, 1.0f, 1.0f},
            .rotation = (HgQuat){1.0f, 0.0f, 0.0f, 0.0f},
        });

        hg_3d_renderer_draw(target, depth_buffer);

        HgError end_result = hg_frame_end(target);
        if (end_result != HG_SUCCESS) {
            HG_DEBUG("Failed to end frame");
            continue;
        }
    }

#if !defined(NDEBUG)
    hg_graphics_wait();

    hg_buffer_destroy(index_buffer);
    hg_buffer_destroy(vertex_buffer);
    hg_texture_destroy(texture);
    hg_texture_destroy(depth_buffer);
    hg_texture_destroy(target);

    hg_window_close();
    hg_3d_renderer_shutdown();
    hg_shutdown();
#endif
}

