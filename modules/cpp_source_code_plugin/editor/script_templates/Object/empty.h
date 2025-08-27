#pragma once

#include "_BASE_INCLUDE_"
#include "godot_cpp/core/class_db.hpp"

using namespace godot;

class _CLASS_ : public _BASE_ {
	GDCLASS(_CLASS_, _BASE_)

protected:
	static void _bind_methods();

public:
	_CLASS_() = default;
	~_CLASS_() override = default;
};
