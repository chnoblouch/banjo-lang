import utils

TO_SNAKE_CASE_TESTS = [
    ("glfwWindowShouldClose", "glfw_window_should_close"),
    ("commandBufferCount", "command_buffer_count"),
    ("vkDeviceWaitIdle", "vk_device_wait_idle"),
    ("vkAcquireNextImage2KHR", "vk_acquire_next_image_2_khr"),
    ("SDL_GetScancodeFromName", "sdl_get_scancode_from_name"),
    ("SDL_AcquireGPUCommandBuffer", "sdl_acquire_gpu_command_buffer"),
    ("SDL_GPUSupportsShaderFormats", "sdl_gpu_supports_shader_formats"),
]


def test_to_snake_case():
    for input_ident, expected_output in TO_SNAKE_CASE_TESTS:
        print(f"{input_ident} ... ", end="")
        
        output_ident = utils.to_snake_case(input_ident)

        if output_ident == expected_output:
            print("ok")
        else:
            print(f"error\n    result: {output_ident}\n  expected: {expected_output}")
        

if __name__ == "__main__":
    test_to_snake_case()
