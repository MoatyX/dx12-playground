#pragma once
#include <Windows.h>

/// <summary>
/// simple keyboard input system
/// </summary>
class simple_input
{
public:

	static simple_input* instance()
	{
		static simple_input local_instance;
		return &local_instance;
	}
	
	bool is_key_down(WPARAM keycode);
	bool is_key_up(WPARAM keycode);

	enum class key_state { down, up };

	friend void update_keys_state(simple_input& input, WPARAM new_key, simple_input::key_state new_state);

private:

	simple_input() {}

	WPARAM last_key_;
	key_state last_key_state_;
};

