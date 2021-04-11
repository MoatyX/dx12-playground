#include "simple_input.h"

bool simple_input::is_key_down(WPARAM keycode)
{
	if (last_key_ && last_key_ == keycode && last_key_state_ == key_state::down)
	{
		last_key_ = 0;		//consume the key
		return true;
	}

	return false;
}

bool simple_input::is_key_up(WPARAM keycode)
{
	if (last_key_ && last_key_ == keycode && last_key_state_ == key_state::up)
	{
		last_key_ = 0;		//consume the key
		return true;
	}

	return false;
}
