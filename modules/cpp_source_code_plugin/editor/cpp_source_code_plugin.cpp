/**************************************************************************/
/*  cpp_source_code_plugin.cpp                                            */
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

#include "cpp_source_code_plugin.h"

#include "core/config/project_settings.h"
#include "editor/editor_node.h"
#include "editor/settings/editor_settings.h"
#include "scene/main/window.h"

const String EXTERNAL_EDITOR_SETTING = "cpp/editor/external_editor";

bool CppSourceCodePlugin::can_handle_object(const Object *p_object) const {
	const GDExtension *library = p_object->get_extension_library();
	if (library == nullptr) {
		return false;
	}

	// We'll just assume that if the object is a GDExtension, it's a C++ class and can be handled.
	return true;
}

String CppSourceCodePlugin::get_source_path(const StringName &p_class_name) const {
	// We'll probably need the GDExtension itself to provide the paths or something, but for now I'll just assume a file exists in this directory that matches the name of the class (but in snake case).
	const String dir = "res://../src/";
	const String filename = p_class_name.operator String().to_snake_case() + ".cpp";
	const String path = dir.path_join(filename);
	if (FileAccess::exists(path)) {
		return path;
	}
	return "";
}

bool CppSourceCodePlugin::overrides_external_editor() const {
	int external_editor_id = EDITOR_GET(EXTERNAL_EDITOR_SETTING);
	return external_editor_id > EXTERNAL_EDITOR_ID_NONE && external_editor_id < EXTERNAL_EDITOR_ID_MAX;
}

Error CppSourceCodePlugin::open_in_external_editor(const String &p_source_path, int p_line, int p_col) const {
	int external_editor_id = EDITOR_GET(EXTERNAL_EDITOR_SETTING);
	switch (external_editor_id) {
		case EXTERNAL_EDITOR_ID_VSCODE: {
			const String path = ProjectSettings::get_singleton()->globalize_path(p_source_path);
			List<String> args;
			args.push_back("-g");
			args.push_back(vformat("%s:%d:%d", path, p_line, p_col));
			OS::get_singleton()->execute("code", args);
			return OK;
		} break;

		default:
			return ERR_UNAVAILABLE;
	}
}

String CppSourceCodePlugin::get_language_name() const {
	return "C++";
}

Ref<Texture2D> CppSourceCodePlugin::get_language_icon() const {
	return EditorNode::get_singleton()->get_window()->get_editor_theme_icon("CppClass");
}

int CppSourceCodePlugin::get_path_count() const {
	return 2;
}

String CppSourceCodePlugin::get_path_label(int p_path_index) const {
	switch (p_path_index) {
		case PATH_ID_HEADER:
			return TTR("Path .h/.hpp:");

		case PATH_ID_SOURCE:
			return TTR("Path .cpp:");

		default:
			ERR_FAIL_V_MSG(String(), vformat("Invalid path index: %d", p_path_index));
	}
}

void CppSourceCodePlugin::configure_select_path_dialog(int p_path_index, EditorFileDialog *p_dialog) const {
	switch (p_path_index) {
		case PATH_ID_HEADER:
			p_dialog->set_title(TTR("Select C++ Header File"));
			p_dialog->set_filters({ "*.h", "*.hpp" });
			break;

		case PATH_ID_SOURCE:
			p_dialog->set_title(TTR("Select C++ Source File"));
			p_dialog->set_filters({ "*.cpp" });
			break;

		default:
			ERR_FAIL_MSG(vformat("Invalid path index: %d", p_path_index));
	}
}

String CppSourceCodePlugin::adjust_path(int p_path_index, const String &p_class_name, const String &p_base_path, const String &p_old_path) const {
	String base_dir = p_base_path;
	String file_name = p_class_name.validate_unicode_identifier();
	if (!p_old_path.is_empty()) {
		base_dir = p_old_path.get_base_dir();
	}

	// The EditorNode API also looks at the editor setting, in case the user overrides the language preference.
	ScriptLanguage::ScriptNameCasing preferred_name_casing = ScriptLanguage::SCRIPT_NAME_CASING_SNAKE_CASE;
	file_name = adjust_script_name_casing(file_name, preferred_name_casing);

	switch (p_path_index) {
		case PATH_ID_HEADER:
			return base_dir.path_join(file_name + ".h");

		case PATH_ID_SOURCE:
			return base_dir.path_join(file_name + ".cpp");

		default:
			ERR_FAIL_V_MSG(String(), vformat("Invalid path index: %d", p_path_index));
	}
}

PackedStringArray CppSourceCodePlugin::get_available_templates(const String &p_base_class_name) const {
	PackedStringArray available_templates;
	if (p_base_class_name == "Object") {
		available_templates.push_back("empty");
		available_templates.push_back("hello_world");
	}
	return available_templates;
}

String CppSourceCodePlugin::get_template_display_name(const String &p_template_id) const {
	if (p_template_id == "empty") {
		return TTR("Empty C++ Class");
	} else if (p_template_id == "hello_world") {
		return TTR("Hello World C++ Class");
	}
	return String();
}

String CppSourceCodePlugin::get_template_description(const String &p_template_id) const {
	if (p_template_id == "empty") {
		return TTR("An empty C++ class template.");
	} else if (p_template_id == "hello_world") {
		return TTR("A C++ class template that prints 'Hello, World!'.");
	}
	return String();
}

TypedArray<Dictionary> CppSourceCodePlugin::get_template_options(const String &p_template_id) const {
	TypedArray<Dictionary> options;
	if (p_template_id == "hello_world") {
		Dictionary message_option = PropertyInfo(Variant::STRING, "print_message");
		message_option["default_value"] = "Hello, World!";
		options.push_back(message_option);

		Dictionary comments_option = PropertyInfo(Variant::BOOL, "include_comments");
		comments_option["default_value"] = true;
		options.push_back(comments_option);
	}
	return options;
}

Error CppSourceCodePlugin::create_class_source(const String &p_class_name, const String &p_base_class_name, const PackedStringArray &p_paths) const {
	// If no template was provided, we'll fallback to the "empty" template.
	return create_class_source_from_template_id(p_class_name, p_base_class_name, p_paths, "empty", Dictionary());
}

String get_hpp_content_for_template(const String &p_template_id, const Dictionary &p_template_options) {
	// We would have to determine the includes based on the base class, but that's complicated so I won't.
	const String includes = R"(#pragma once

#include "godot_cpp/classes/ref_counted.hpp"
#include "godot_cpp/classes/wrapped.hpp"
#include "godot_cpp/variant/variant.hpp"

using namespace godot;
)";

	const String begin_class_definition = R"(
class _CLASS_ : public _BASE_ {
	GDCLASS(_CLASS_, _BASE_)

protected:
	static void _bind_methods();
)";

	const String end_class_definition = R"(
public:
	_CLASS_() = default;
	~_CLASS_() override = default;
};
)";

	if (p_template_id == "hello_world") {
		String content = includes + begin_class_definition;
		content += R"(
	void _notification(int p_what);
)";
		content += end_class_definition;
		return content;
	} else {
		return includes + begin_class_definition + end_class_definition;
	}
}

String get_cpp_content_for_template(const String &p_template_id, const Dictionary &p_template_options) {
	if (p_template_id == "hello_world") {
		const String print_message = p_template_options["print_message"];
		bool include_comments = p_template_options["include_comments"];

		String content;
		if (include_comments) {
			content = R"(#include "_HEADER_PATH_"

void _CLASS_::_bind_methods() {
	// Bind methods here.
}

void _CLASS_::_notification(int p_what) {
	if (p_what == NOTIFICATION_READY) {
		// Called when the node is added to the scene.
		print_line("{{PRINT_MESSAGE}}");
	}
}
)";
		} else {
			content = R"(#include "_HEADER_PATH_"

void _CLASS_::_bind_methods() {
}

void _CLASS_::_notification(int p_what) {
	if (p_what == NOTIFICATION_READY) {
		print_line("{{PRINT_MESSAGE}}");
	}
}
)";
		}
		return content.replace("{{PRINT_MESSAGE}}", print_message);
	} else {
		return R"(#include "_HEADER_PATH_"

void _CLASS_::_bind_methods() {
}
)";
	}
}

String replace_template_placeholders(const String &p_template_content, const String &p_class_name, const String &p_base_class_name, const String &p_header_file_path) {
	return p_template_content
			.replace("_BASE_", p_base_class_name)
			.replace("_CLASS_", p_class_name)
			.replace("_HEADER_PATH_", p_header_file_path);
}

Error CppSourceCodePlugin::create_class_source_from_template_id(const String &p_class_name, const String &p_base_class_name, const PackedStringArray &p_paths, const String &p_template_id, const Dictionary &p_template_options) const {
	ERR_FAIL_COND_V_MSG(p_paths.size() != 2, ERR_INVALID_PARAMETER, "Expected exactly 2 paths for the C++ files.");
	String header_file_path = p_paths[PATH_ID_HEADER];
	String source_file_path = p_paths[PATH_ID_SOURCE];
	ERR_FAIL_COND_V_MSG(!header_file_path.ends_with(".h") && !header_file_path.ends_with(".hpp"), ERR_INVALID_PARAMETER, "Expected a .h/.hpp file.");
	ERR_FAIL_COND_V_MSG(!source_file_path.ends_with(".cpp"), ERR_INVALID_PARAMETER, "Expected a .cpp file.");

	{
		String content = get_hpp_content_for_template(p_template_id, p_template_options);
		content = replace_template_placeholders(content, p_class_name, p_base_class_name, header_file_path);

		Ref<FileAccess> header_file = FileAccess::create_for_path(header_file_path);
		header_file->reopen(header_file_path, FileAccess::WRITE);
		bool ok = header_file->store_string(content);
		ERR_FAIL_COND_V_MSG(!ok, ERR_CANT_CREATE, vformat("Failed to create class header file: %s", header_file_path));
	}

	{
		String content = get_cpp_content_for_template(p_template_id, p_template_options);
		content = replace_template_placeholders(content, p_class_name, p_base_class_name, header_file_path);

		Ref<FileAccess> source_file = FileAccess::create_for_path(source_file_path);
		source_file->reopen(source_file_path, FileAccess::WRITE);
		bool ok = source_file->store_string(content);
		ERR_FAIL_COND_V_MSG(!ok, ERR_CANT_CREATE, vformat("Failed to create class source file: %s", source_file_path));
	}

	{
		// We'll assume where the C++ project and its 'register_types.cpp' file are located.
		Error err;
		Ref<FileAccess> register_types_file = FileAccess::open("res://../src/register_types.cpp", FileAccess::ModeFlags::READ_WRITE, &err);
		if (err != OK) {
			// If the file doesn't exist, we'll try the fallback we create to avoid crashing but this is just a mockup anyway.
			register_types_file = FileAccess::open("res://register_types.cpp", FileAccess::ModeFlags::READ_WRITE, &err);
		}
		if (err != OK) {
			// If the file doesn't exist, we'll create it to avoid crashing but this is just a mock up anyway.
			register_types_file = FileAccess::create_for_path("res://register_types.cpp");
			register_types_file->reopen("res://register_types.cpp", FileAccess::ModeFlags::WRITE);
		} else {
			// Make sure we append to the end of the file.
			register_types_file->seek_end();
		}
		bool ok = register_types_file->store_string(vformat("GDREGISTER_CLASS(%s);\n", p_class_name));
		ERR_FAIL_COND_V_MSG(!ok, ERR_CANT_CREATE, vformat("Failed to register class in register_types.cpp: %s", register_types_file->get_path()));
	}

	return OK;
}

bool is_lowercase_ascii(char32_t p_char) {
	return p_char >= 'a' && p_char <= 'z';
}

void CppSourceCodePlugin::validate_class_name(ValidationContext *p_validation_context, const String &p_class_name) const {
	if (!p_class_name.is_valid_ascii_identifier()) {
		p_validation_context->add_validation(ValidationContext::VALIDATION_SEVERITY_ERROR, TTR("Class name is not a valid ASCII identifier."));
		return;
	}

	if (p_class_name.begins_with("Editor")) {
		p_validation_context->add_validation(ValidationContext::VALIDATION_SEVERITY_WARNING, TTR("Class name begins with 'Editor'. This may conflict with built-in editor classes."));
	}

	if (p_class_name.begins_with("_")) {
		p_validation_context->add_validation(ValidationContext::VALIDATION_SEVERITY_WARNING, TTR("Class name begins with '_' which is not a recommended naming convention."));
	}
}

void CppSourceCodePlugin::validate_path(ValidationContext *p_validation_context, int p_path_index, const String &p_path) const {
	const String extension = p_path.get_extension().to_lower();
	switch (p_path_index) {
		case PATH_ID_HEADER:
			if (extension != "h" && extension != "hpp") {
				p_validation_context->add_validation(ValidationContext::VALIDATION_SEVERITY_ERROR, TTR("Header file path must have a '.h' or '.hpp' extension."));
			}
			break;
		case PATH_ID_SOURCE:
			if (extension != "cpp") {
				p_validation_context->add_validation(ValidationContext::VALIDATION_SEVERITY_ERROR, TTR("Source file path must have a '.cpp' extension."));
			}
			break;
	}
}

void CppSourceCodePlugin::validate_template_option(ValidationContext *p_validation_context, const String &p_template_id, const String &p_option_name, const Variant &p_value) const {
	if (p_template_id == "hello_world" && p_option_name == "print_message") {
		String value = p_value;
		if (value.is_empty()) {
			p_validation_context->add_validation(ValidationContext::VALIDATION_SEVERITY_ERROR, TTR("Print message must be a string."));
			return;
		}
		if (value.begins_with(" ")) {
			p_validation_context->add_validation(ValidationContext::VALIDATION_SEVERITY_WARNING, TTR("Print message should not start with a space."));
		}
	}
}

Error CppSourceCodePlugin::add_method_func(const StringName &p_class_name, const String &p_method_name, const PackedStringArray &p_args) const {
	Ref<FileAccess> file = FileAccess::open(get_source_path(p_class_name), FileAccess::READ_WRITE);
	file->seek_end();

	Vector<String> args;
	for (int i = 0; i < p_args.size(); i++) {
		String arg = p_args[i];
		String name = arg.get_slicec(':', 0).strip_edges();
		String type = arg.get_slicec(':', 1).strip_edges();
		args.push_back(vformat("%s %s", type, name));
	}

	String method = R"(
void _CLASS_::_METHOD_(_ARGS_) {
	// Replace with method body.
}
)";

	method = method.replace("_CLASS_", p_class_name)
					 .replace("_METHOD_", p_method_name)
					 .replace("_ARGS_", String(", ").join(args));

	// We'll just append the function to the .cpp, we should also update the header file but that's more complicated.
	bool success = file->store_string(method);
	return success ? OK : ERR_UNAVAILABLE;
}

CppSourceCodePlugin::CppSourceCodePlugin() {
	EDITOR_DEF_BASIC(EXTERNAL_EDITOR_SETTING, EXTERNAL_EDITOR_ID_NONE);
	EditorSettings::get_singleton()->add_property_hint(PropertyInfo(Variant::INT, EXTERNAL_EDITOR_SETTING, PROPERTY_HINT_ENUM, "Disabled:0,Visual Studio Code:1"));
}
