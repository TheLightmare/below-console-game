#include "vk_input.hpp"

namespace VkInput {

Key sdl_keycode_to_key(SDL_Keycode code) {
    switch (code) {
        case SDLK_UP:     return Key::UP;
        case SDLK_DOWN:   return Key::DOWN;
        case SDLK_LEFT:   return Key::LEFT;
        case SDLK_RIGHT:  return Key::RIGHT;
        case SDLK_ESCAPE: return Key::ESCAPE;
        case SDLK_RETURN: return Key::ENTER;
        case SDLK_SPACE:  return Key::SPACE;
        case SDLK_TAB:    return Key::TAB;
        case SDLK_PAGEUP:   return Key::PAGE_UP;
        case SDLK_PAGEDOWN: return Key::PAGE_DOWN;

        case SDLK_a: return Key::A;
        case SDLK_b: return Key::B;
        case SDLK_c: return Key::C;
        case SDLK_d: return Key::D;
        case SDLK_e: return Key::E;
        case SDLK_f: return Key::F;
        case SDLK_g: return Key::G;
        case SDLK_h: return Key::H;
        case SDLK_i: return Key::I;
        case SDLK_j: return Key::J;
        case SDLK_k: return Key::K;
        case SDLK_l: return Key::L;
        case SDLK_m: return Key::M;
        case SDLK_n: return Key::N;
        case SDLK_o: return Key::O;
        case SDLK_p: return Key::P;
        case SDLK_q: return Key::Q;
        case SDLK_r: return Key::R;
        case SDLK_s: return Key::S;
        case SDLK_t: return Key::T;
        case SDLK_u: return Key::U;
        case SDLK_v: return Key::V;
        case SDLK_w: return Key::W;
        case SDLK_x: return Key::X;
        case SDLK_y: return Key::Y;
        case SDLK_z: return Key::Z;

        case SDLK_1: return Key::NUM_1;
        case SDLK_2: return Key::NUM_2;
        case SDLK_3: return Key::NUM_3;
        case SDLK_4: return Key::NUM_4;
        case SDLK_5: return Key::NUM_5;
        case SDLK_6: return Key::NUM_6;
        case SDLK_7: return Key::NUM_7;
        case SDLK_8: return Key::NUM_8;
        case SDLK_9: return Key::NUM_9;

        case SDLK_PERIOD:       return Key::GREATER;  // '.'/>
        case SDLK_COMMA:        return Key::LESS;     // ','/<
        case SDLK_PLUS:
        case SDLK_EQUALS:       return Key::PLUS;
        case SDLK_MINUS:        return Key::MINUS;
        case SDLK_LEFTBRACKET:  return Key::BRACKET_LEFT;
        case SDLK_RIGHTBRACKET: return Key::BRACKET_RIGHT;

        default: return Key::UNKNOWN;
    }
}

Key poll_events(bool& quit, bool& window_resized) {
    Key last_key = Key::NONE;
    SDL_Event event;

    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_QUIT:
                quit = true;
                break;

            case SDL_WINDOWEVENT:
                if (event.window.event == SDL_WINDOWEVENT_RESIZED ||
                    event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
                    window_resized = true;
                }
                break;

            case SDL_KEYDOWN:
                if (!event.key.repeat) {
                    last_key = sdl_keycode_to_key(event.key.keysym.sym);
                }
                break;

            default:
                break;
        }
    }

    return last_key;
}

} // namespace VkInput
