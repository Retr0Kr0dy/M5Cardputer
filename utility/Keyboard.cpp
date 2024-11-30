/**
 * @file keyboard.cpp
 * @author Forairaaaaa
 * @brief Keyboard handling class for ESP32 using ESP-IDF.
 * @version 0.1
 * @date 2023-09-22
 *
 * @copyright Copyright (c) 2023
 *
 */
#include "Keyboard.h"
#include <driver/gpio.h>
#include <vector>
#include <cstring>

#define digitalWrite(pin, level) gpio_set_level(static_cast<gpio_num_t>(pin), level)
#define digitalRead(pin)         gpio_get_level(static_cast<gpio_num_t>(pin))

void Keyboard_Class::_set_output(const std::vector<int>& pinList, uint8_t output) {
    output &= 0B00000111;  // Only the lowest 3 bits matter

    for (int i = 0; i < 3; ++i) {
        digitalWrite(pinList[i], (output >> i) & 0B00000001);
    }
}

uint8_t Keyboard_Class::_get_input(const std::vector<int>& pinList) {
    uint8_t buffer = 0x00;

    for (int i = 0; i < 7; ++i) {
        buffer |= (digitalRead(pinList[i]) == 0 ? (0x01 << i) : 0x00);
    }

    return buffer;
}

void Keyboard_Class::begin() {
    // Set output pins
    for (const auto& pin : output_list) {
        gpio_reset_pin(static_cast<gpio_num_t>(pin));
        gpio_set_direction(static_cast<gpio_num_t>(pin), GPIO_MODE_OUTPUT);
        gpio_set_pull_mode(static_cast<gpio_num_t>(pin), GPIO_PULLUP_PULLDOWN);
        digitalWrite(pin, 0);
    }

    // Set input pins
    for (const auto& pin : input_list) {
        gpio_reset_pin(static_cast<gpio_num_t>(pin));
        gpio_set_direction(static_cast<gpio_num_t>(pin), GPIO_MODE_INPUT);
        gpio_set_pull_mode(static_cast<gpio_num_t>(pin), GPIO_PULLUP_ONLY);
    }

    _set_output(output_list, 0);
}

uint8_t Keyboard_Class::getKey(Point2D_t keyCoor) {
    if (keyCoor.x < 0 || keyCoor.y < 0) {
        return 0;
    }

    return _keys_state_buffer.ctrl || _keys_state_buffer.shift || _is_caps_locked
           ? _key_value_map[keyCoor.y][keyCoor.x].value_second
           : _key_value_map[keyCoor.y][keyCoor.x].value_first;
}

void Keyboard_Class::updateKeyList() {
    _key_list_buffer.clear();
    Point2D_t coor;
    uint8_t input_value = 0;

    for (int i = 0; i < 8; ++i) {
        _set_output(output_list, i);
        input_value = _get_input(input_list);

        if (input_value) {
            for (int j = 0; j < 7; ++j) {
                if (input_value & (0x01 << j)) {
                    coor.x = (i > 3) ? X_map_chart[j].x_1 : X_map_chart[j].x_2;
                    coor.y = (i > 3) ? (i - 4) : i;

                    // Keep the same as the picture
                    coor.y = -coor.y + 3;

                    _key_list_buffer.push_back(coor);
                }
            }
        }
    }
}

uint8_t Keyboard_Class::isPressed() {
    return static_cast<uint8_t>(_key_list_buffer.size());
}

bool Keyboard_Class::isChange() {
    if (_last_key_size != _key_list_buffer.size()) {
        _last_key_size = _key_list_buffer.size();
        return true;
    }
    return false;
}

bool Keyboard_Class::isKeyPressed(char c) {
    for (const auto& key : _key_list_buffer) {
        if (getKey(key) == c) return true;
    }
    return false;
}

void Keyboard_Class::updateKeysState() {
    _keys_state_buffer.reset();
    _key_pos_print_keys.clear();
    _key_pos_hid_keys.clear();
    _key_pos_modifier_keys.clear();

    for (const auto& i : _key_list_buffer) {
        const auto keyValue = getKeyValue(i).value_first;

        // Modifier keys
        if (keyValue == KEY_FN) {
            _keys_state_buffer.fn = true; continue;
        }
        if (keyValue == KEY_OPT) {
            _keys_state_buffer.opt = true; continue;
        }
        if (keyValue == KEY_LEFT_CTRL) {
            _keys_state_buffer.ctrl = true;
            _key_pos_modifier_keys.push_back(i); continue;
        }
        if (keyValue == KEY_LEFT_SHIFT) {
            _keys_state_buffer.shift = true;
            _key_pos_modifier_keys.push_back(i); continue;
        }
        if (keyValue == KEY_LEFT_ALT) {
            _keys_state_buffer.alt = true;
            _key_pos_modifier_keys.push_back(i); continue;
        }

        // Function keys
        if (keyValue == KEY_TAB) {
            _keys_state_buffer.tab = true; _key_pos_hid_keys.push_back(i); continue;
        }
        if (keyValue == KEY_BACKSPACE) {
            _keys_state_buffer.del = true; _key_pos_hid_keys.push_back(i); continue;
        }
        if (keyValue == KEY_ENTER) {
            _keys_state_buffer.enter = true; _key_pos_hid_keys.push_back(i); continue;
        }
        if (keyValue == ' ') {
            _keys_state_buffer.space = true; 
        }

        _key_pos_hid_keys.push_back(i);
        _key_pos_print_keys.push_back(i);
    }

    for (const auto& i : _key_pos_modifier_keys) {
        uint8_t key = getKeyValue(i).value_first;
        _keys_state_buffer.modifier_keys.push_back(key);
    }

    for (auto& k : _keys_state_buffer.modifier_keys) {
        _keys_state_buffer.modifiers |= (1 << (k - 0x80));
    }

    for (const auto& i : _key_pos_hid_keys) {
        uint8_t k = getKeyValue(i).value_first;
        if (k == KEY_TAB || k == KEY_BACKSPACE || k == KEY_ENTER) {
            _keys_state_buffer.hid_keys.push_back(k);
            continue;
        }
        uint8_t key = _kb_asciimap[k];
        if (key) {
            _keys_state_buffer.hid_keys.push_back(key);
        }
    }

    for (const auto& i : _key_pos_print_keys) {
        uint8_t k = getKeyValue(i).value_first;
        if (_keys_state_buffer.ctrl || _keys_state_buffer.shift || _is_caps_locked) {
            _keys_state_buffer.word.push_back(getKeyValue(i).value_second);
        } else {
            _keys_state_buffer.word.push_back(getKeyValue(i).value_first);
        }
    }
}
