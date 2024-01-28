/**************************************************************************/
/*  dotnet_status_panel.h                                                 */
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

#include "./dotnet_status_indicator.h"

#include "scene/gui/box_container.h"
#include "scene/gui/popup.h"

class Label;
class Button;
class TextureRect;

namespace DotNet {

class DotNetStatusPanel : public PopupPanel {
	GDCLASS(DotNetStatusPanel, PopupPanel);

private:
	Label *title_label = nullptr;

	DotNetStatusIndicator::Severity severity = DotNetStatusIndicator::Severity::NONE;

	// Generic details show when the module is not initialized.
	VBoxContainer *basic_details = nullptr;
	TextureRect *info_icon = nullptr;
	Label *info_label = nullptr;
	Button *enable_dotnet_button = nullptr;

	// Detailed content that is available when the module is initialized,
	// it's updated from the C# side.
	VBoxContainer *full_details = nullptr;
	Control *content = nullptr;

	void _initialize_module();
	void _update_content();

	void _update_basic_content();
	void _update_detailed_content();

protected:
	void _notification(int p_what);

public:
	void set_title(const String &p_title);
	void set_message(const String &p_message);
	void set_initial_severity(DotNetStatusIndicator::Severity p_severity);

	void set_content(Control *p_content);

	void update();

	DotNetStatusPanel();
};

} // namespace DotNet
