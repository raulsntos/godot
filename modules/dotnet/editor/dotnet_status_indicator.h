/**************************************************************************/
/*  dotnet_status_indicator.h                                             */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             GODOT ENGINE                               */
/*                        https://godotengine.org                         */
/**************************************************************************/
/* Copyright (c) 2014-present Godot Engine contributors (see AUTHORS.md). */
/* Copyright (c) 2007-2014 Juan Linietsky, Ariel Manzur.                  */
/*                                                                        */
/* Permission is hereby granted, free of charge, to any person obtaining  */
/* a copy of this software and associated documentation files (the        */
/* "Software"), to deal in the Software without restriction, including    */
/* without limitation the rights to use, copy, modify, merge, publish,    */
/* distribute, sublicense, and/or sell copies of the Software, and to     */
/* permit persons to whom the Software is furnished to do so, subject to  */
/* the following conditions:                                              */
/*                                                                        */
/* The above copyright notice and this permission notice shall be         */
/* included in all copies or substantial portions of the Software.        */
/*                                                                        */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,        */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF     */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. */
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY   */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,   */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE      */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                 */
/**************************************************************************/

#pragma once

#include "scene/gui/box_container.h"

class Button;
class PanelContainer;
class StyleBoxFlat;

namespace DotNet {

class DotNetStatusPanel;

class DotNetStatusIndicator : public HBoxContainer {
	GDCLASS(DotNetStatusIndicator, HBoxContainer);

public:
	/*
	 * Indicates the overall severity of the current status.
	 * It is updated based on the module initialization state
	 * but it can also be updated from the status panel update
	 * callback.
	 */
	enum class Severity {
		NONE,
		LOADING,
		WARNING,
		ERROR,
	};

private:
	static DotNetStatusIndicator *singleton;

	Severity severity = Severity::NONE;

	bool updating = false;
	Button *main_button = nullptr;
	DotNetStatusPanel *status_panel = nullptr;

	void _update_indicator();
	void _update_panel_position();

	void _show_status_panel();
	void _hide_status_panel();

	void _draw_button();

	static bool _project_has_csproj_files();

protected:
	void _notification(int p_what);

public:
	static DotNetStatusIndicator *get_singleton();

	void set_severity(Severity p_severity);
	void set_panel_content(Control *p_content);

	void update();

	void toggle_status_panel();

	DotNetStatusIndicator();
	~DotNetStatusIndicator();
};

} // namespace DotNet
