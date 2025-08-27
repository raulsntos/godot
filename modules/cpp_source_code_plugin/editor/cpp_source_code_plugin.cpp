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

#include "script_templates/templates.gen.h"

#include "core/config/project_settings.h"
#include "core/os/os.h"
#include "editor/editor_node.h"
#include "editor/settings/editor_settings.h"
#include "scene/main/window.h"

#include "modules/regex/regex.h"

const String EXTERNAL_EDITOR_SETTING = "cpp/editor/external_editor";
const String SOURCE_ROOT_SETTING = "cpp/source/source_root_directory";

String CppSourceCodePlugin::get_source_root() const {
	String source_root = GLOBAL_GET(SOURCE_ROOT_SETTING);
	if (source_root.is_empty()) {
		source_root = "res://../src";
	}
	return source_root;
}

bool CppSourceCodePlugin::can_handle_object(const GDExtension *p_library, const Object *p_object) const {
	if (p_library == nullptr) {
		return false;
	}

	// We'll just assume that if the object is a GDExtension, it's a C++ class and can be handled.
	return true;
}

String CppSourceCodePlugin::get_source_path(const StringName &p_class_name) const {
	// Search for the class source files in the configured source root.
	// The class name is expected to match the file name in snake_case.
	const String source_root = get_source_root();
	const String snake_name = p_class_name.operator String().to_snake_case();

	// We prefer the .cpp file since it contains the actual source code that users may want to edit.
	const String cpp_path = source_root.path_join(snake_name + ".cpp");
	if (FileAccess::exists(cpp_path)) {
		return cpp_path;
	}
	const String header_path = source_root.path_join(snake_name + ".h");
	if (FileAccess::exists(header_path)) {
		return header_path;
	}
	const String hpp_path = source_root.path_join(snake_name + ".hpp");
	if (FileAccess::exists(hpp_path)) {
		return hpp_path;
	}

	// Fallback: search under res:// in case the source root doesn't exist.
	const String fallback_header = String("res://").path_join(snake_name + ".h");
	if (FileAccess::exists(fallback_header)) {
		return fallback_header;
	}
	const String fallback_cpp = String("res://").path_join(snake_name + ".cpp");
	if (FileAccess::exists(fallback_cpp)) {
		return fallback_cpp;
	}

	return "";
}

bool CppSourceCodePlugin::get_location_in_source(const StringName &p_class_name, const StringName &p_method_name, String *r_source_path, int *r_line, int *r_col) const {
	String source_path = get_source_path(p_class_name);
	if (source_path.is_empty()) {
		return false;
	}
	if (!source_path.has_extension("cpp")) {
		return false;
	}

	// Naive implementation that looks for "ClassName::MethodName(" in the source file.
	// This won't work in all cases, but it's good enough for this mockup.
	Ref<RegEx> regex = RegEx::create_from_string(vformat(R"(\b%s\s*::\s*%s\s*\()", p_class_name, p_method_name));
	TypedArray<Ref<RegExMatch>> matches = regex->search_all(FileAccess::get_file_as_string(source_path));
	if (matches.size() == 0) {
		return false;
	}

	if (matches.size() > 1) {
		WARN_PRINT(vformat("Multiple matches found for %s::%s in %s. Returning the first one.", p_class_name, p_method_name, source_path));
	}

	Ref<RegExMatch> match = matches[0];

	// RegExMatch::get_start returns the byte offset in the file, so we need to convert it to line and column.
	int offset = match->get_start(0);
	String file_content = FileAccess::get_file_as_string(source_path);
	int line = 0;
	int column = 0;
	for (int i = 0; i < offset; i++) {
		// Assuming LF line endings.
		if (file_content[i] == '\n') {
			line++;
			column = 0;
		} else {
			column++;
		}
	}

	// Godot expects the line and column to be 0-based, so we should be good with the current values.
	*r_source_path = source_path;
	*r_line = line;
	*r_col = column;

	return true;
}

StringName CppSourceCodePlugin::get_class_name_from_source_path(const String &p_source_path) const {
	// Look for GDCLASS(ClassName, BaseClass) in header files.
	String header_path = p_source_path;
	if (p_source_path.has_extension("cpp")) {
		header_path = p_source_path.substr(0, p_source_path.length() - 3) + "h";
		if (!FileAccess::exists(header_path)) {
			// Try with .hpp as well.
			header_path = p_source_path.substr(0, p_source_path.length() - 3) + "hpp";
		}
	}
	if (!FileAccess::exists(header_path)) {
		return StringName();
	}

	Error err;
	const String content = FileAccess::get_file_as_string(header_path, &err);
	if (err != OK) {
		return StringName();
	}

	// Find the class name from the GDCLASS macro.
	// This is just a naive implementation and may not work in all cases, but it's good enough for this mockup.
	int gdclass_pos = content.find("GDCLASS(");
	if (gdclass_pos == -1) {
		return StringName();
	}
	int name_start = gdclass_pos + String("GDCLASS(").length();
	int comma_pos = content.find(",", name_start);
	if (comma_pos == -1) {
		return StringName();
	}
	return content.substr(name_start, comma_pos - name_start).strip_edges();
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

PackedStringArray CppSourceCodePlugin::get_language_extensions() const {
	PackedStringArray extensions;
	extensions.push_back("h");
	extensions.push_back("hpp");
	extensions.push_back("cpp");
	return extensions;
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
	for (int i = 0; i < TEMPLATES_ARRAY_SIZE; i++) {
		if (p_base_class_name == TEMPLATES[i].inherit) {
			available_templates.push_back(TEMPLATES[i].id);
		}
	}
	return available_templates;
}

String CppSourceCodePlugin::get_template_display_name(const String &p_template_id) const {
	for (int i = 0; i < TEMPLATES_ARRAY_SIZE; i++) {
		if (p_template_id == TEMPLATES[i].id) {
			return TTR(TEMPLATES[i].name);
		}
	}
	return String();
}

String CppSourceCodePlugin::get_template_description(const String &p_template_id) const {
	for (int i = 0; i < TEMPLATES_ARRAY_SIZE; i++) {
		if (p_template_id == TEMPLATES[i].id) {
			return TTR(TEMPLATES[i].description);
		}
	}
	return String();
}

TypedArray<Dictionary> CppSourceCodePlugin::get_template_options(const String &p_template_id) const {
	TypedArray<Dictionary> options;
	for (int i = 0; i < TEMPLATES_ARRAY_SIZE; i++) {
		if (p_template_id != TEMPLATES[i].id) {
			continue;
		}
		for (uint32_t j = 0; j < TEMPLATES[i].parameters.size(); j++) {
			const CppScriptTemplateParameter &param = TEMPLATES[i].parameters[j];
			Variant::Type type = param.type;
			Dictionary option = PropertyInfo(type, param.name);
			if (param.default_value.get_type() != Variant::NIL) {
				option["default_value"] = param.default_value;
			}
			options.push_back(option);
		}
		break;
	}
	return options;
}

Error CppSourceCodePlugin::create_class_source(const String &p_class_name, const String &p_base_class_name, const PackedStringArray &p_paths) const {
	// Find the first template matching the base class; fallback to the generic empty Object template.
	for (int i = 0; i < TEMPLATES_ARRAY_SIZE; i++) {
		if (p_base_class_name == TEMPLATES[i].inherit) {
			return create_class_source_from_template_id(p_class_name, p_base_class_name, p_paths, String(TEMPLATES[i].id), Dictionary());
		}
	}
	return create_class_source_from_template_id(p_class_name, p_base_class_name, p_paths, "object_empty", Dictionary());
}

static String replace_template_placeholders(const String &p_template_content, const String &p_class_name, const String &p_base_class_name, const String &p_header_file_path) {
	// Derive the godot-cpp include path from the base class name.
	// e.g. "Node" -> "godot_cpp/classes/node.hpp"
	const String base_include = "godot_cpp/classes/" + p_base_class_name.to_snake_case() + ".hpp";
	return p_template_content
			.replace("_BASE_INCLUDE_", base_include)
			.replace("_BASE_", p_base_class_name)
			.replace("_CLASS_", p_class_name)
			.replace("_HEADER_PATH_", p_header_file_path.get_file());
}

Error CppSourceCodePlugin::create_class_source_from_template_id(const String &p_class_name, const String &p_base_class_name, const PackedStringArray &p_paths, const String &p_template_id, const Dictionary &p_template_options) const {
	ERR_FAIL_COND_V_MSG(p_paths.size() != 2, ERR_INVALID_PARAMETER, "Expected exactly 2 paths for the C++ files.");
	const String header_file_path = p_paths[PATH_ID_HEADER];
	const String source_file_path = p_paths[PATH_ID_SOURCE];
	ERR_FAIL_COND_V_MSG(!header_file_path.ends_with(".h") && !header_file_path.ends_with(".hpp"), ERR_INVALID_PARAMETER, "Expected a .h/.hpp file.");
	ERR_FAIL_COND_V_MSG(!source_file_path.ends_with(".cpp"), ERR_INVALID_PARAMETER, "Expected a .cpp file.");

	int template_index = -1;
	for (int i = 0; i < TEMPLATES_ARRAY_SIZE; i++) {
		if (p_template_id == TEMPLATES[i].id) {
			template_index = i;
			break;
		}
	}
	ERR_FAIL_COND_V_MSG(template_index == -1, ERR_INVALID_PARAMETER, vformat("Unknown template ID: %s", p_template_id));

	String hpp_content = TEMPLATES[template_index].header_content;
	String cpp_content = TEMPLATES[template_index].source_content;

	// For the hello_world template, the "include_comments" parameter selects between two file variants.
	if (p_template_id == "object_hello_world") {
		bool include_comments = p_template_options.get("include_comments", Variant(true));
		if (!include_comments) {
			hpp_content = TEMPLATE_FILES["Object/hello_world.nocomments.h"];
			cpp_content = TEMPLATE_FILES["Object/hello_world.nocomments.cpp"];
		}
	}

	// Apply string parameter substitutions using the ___name___ placeholder convention.
	for (uint32_t j = 0; j < TEMPLATES[template_index].parameters.size(); j++) {
		if (TEMPLATES[template_index].parameters[j].type != Variant::STRING) {
			continue;
		}
		const String &parameter_name = TEMPLATES[template_index].parameters[j].name;
		const String parameter_default = TEMPLATES[template_index].parameters[j].default_value;
		const String placeholder = "___" + parameter_name + "___";
		const String value = p_template_options.get(parameter_name, parameter_default);
		hpp_content = hpp_content.replace(placeholder, value);
		cpp_content = cpp_content.replace(placeholder, value);
	}

	hpp_content = replace_template_placeholders(hpp_content, p_class_name, p_base_class_name, header_file_path);
	cpp_content = replace_template_placeholders(cpp_content, p_class_name, p_base_class_name, header_file_path);

	{
		Error err;
		Ref<FileAccess> header_file = FileAccess::open(header_file_path, FileAccess::WRITE, &err);
		ERR_FAIL_COND_V_MSG(err != OK || header_file.is_null(), ERR_CANT_CREATE, vformat("Failed to create class header file: %s", header_file_path));
		bool ok = header_file->store_string(hpp_content);
		ERR_FAIL_COND_V_MSG(!ok, ERR_CANT_CREATE, vformat("Failed to write class header file: %s", header_file_path));
	}

	{
		Error err;
		Ref<FileAccess> source_file = FileAccess::open(source_file_path, FileAccess::WRITE, &err);
		ERR_FAIL_COND_V_MSG(err != OK || source_file.is_null(), ERR_CANT_CREATE, vformat("Failed to create class source file: %s", source_file_path));
		bool ok = source_file->store_string(cpp_content);
		ERR_FAIL_COND_V_MSG(!ok, ERR_CANT_CREATE, vformat("Failed to write class source file: %s", source_file_path));
	}

	// Update register_types.cpp: add an #include for the new header and a GDREGISTER_CLASS call.
	{
		const String source_root = get_source_root();
		const String register_types_path_primary = source_root.path_join("register_types.cpp");
		const String register_types_path_fallback = "res://register_types.cpp";

		String register_types_path;
		Error err;
		String content = FileAccess::get_file_as_string(register_types_path_primary, &err);
		if (err == OK) {
			register_types_path = register_types_path_primary;
		} else {
			content = FileAccess::get_file_as_string(register_types_path_fallback, &err);
			if (err == OK) {
				register_types_path = register_types_path_fallback;
			}
		}

		if (err != OK) {
			// No register_types.cpp found; create a minimal one at the fallback path.
			register_types_path = register_types_path_fallback;
			content = "";
		}

		// Insert '#include "header.h"' after the last existing #include line.
		const String header_filename = header_file_path.get_file();
		const String new_include = vformat("#include \"%s\"", header_filename);
		int last_include_pos = content.rfind("#include");
		if (last_include_pos != -1) {
			int end_of_line = content.find("\n", last_include_pos);
			if (end_of_line != -1) {
				content = content.substr(0, end_of_line + 1) + new_include + "\n" + content.substr(end_of_line + 1);
			} else {
				content += "\n" + new_include + "\n";
			}
		} else {
			content = new_include + "\n" + content;
		}

		// Insert 'GDREGISTER_CLASS(ClassName);' after the last existing GDREGISTER_CLASS call.
		const String new_registration = vformat("\tGDREGISTER_CLASS(%s);", p_class_name);
		int last_register_pos = content.rfind("GDREGISTER_CLASS(");
		if (last_register_pos != -1) {
			int end_of_line = content.find("\n", last_register_pos);
			if (end_of_line != -1) {
				content = content.substr(0, end_of_line + 1) + new_registration + "\n" + content.substr(end_of_line + 1);
			} else {
				content += "\n" + new_registration + "\n";
			}
		} else {
			content += "\n" + new_registration + "\n";
		}

		Ref<FileAccess> register_types_file = FileAccess::open(register_types_path, FileAccess::WRITE, &err);
		ERR_FAIL_COND_V_MSG(err != OK || register_types_file.is_null(), ERR_CANT_CREATE, vformat("Failed to open register_types.cpp for writing: %s", register_types_path));
		bool ok = register_types_file->store_string(content);
		ERR_FAIL_COND_V_MSG(!ok, ERR_CANT_CREATE, vformat("Failed to write register_types.cpp: %s", register_types_path));
	}

	return OK;
}

bool is_lowercase_ascii(char32_t p_char) {
	return p_char >= 'a' && p_char <= 'z';
}

void CppSourceCodePlugin::validate_class_name(EditorValidationContext *p_validation_context, const String &p_class_name) const {
	if (!p_class_name.is_valid_ascii_identifier()) {
		p_validation_context->add_validation(EditorValidationContext::VALIDATION_SEVERITY_ERROR, TTR("Class name is not a valid ASCII identifier."));
		return;
	}

	if (p_class_name.begins_with("Editor")) {
		p_validation_context->add_validation(EditorValidationContext::VALIDATION_SEVERITY_WARNING, TTR("Class name begins with 'Editor'. This may conflict with built-in editor classes."));
	}

	if (p_class_name.begins_with("_")) {
		p_validation_context->add_validation(EditorValidationContext::VALIDATION_SEVERITY_WARNING, TTR("Class name begins with '_' which is not a recommended naming convention."));
	}
}

void CppSourceCodePlugin::validate_path(EditorValidationContext *p_validation_context, int p_path_index, const String &p_path) const {
	const String extension = p_path.get_extension().to_lower();
	switch (p_path_index) {
		case PATH_ID_HEADER:
			if (extension != "h" && extension != "hpp") {
				p_validation_context->add_validation(EditorValidationContext::VALIDATION_SEVERITY_ERROR, TTR("Header file path must have a '.h' or '.hpp' extension."));
			}
			break;
		case PATH_ID_SOURCE:
			if (extension != "cpp") {
				p_validation_context->add_validation(EditorValidationContext::VALIDATION_SEVERITY_ERROR, TTR("Source file path must have a '.cpp' extension."));
			}
			break;
	}

	// Warn if the path is outside the source root, since the file won't be included in the compilation.
	const String source_root = ProjectSettings::get_singleton()->globalize_path(get_source_root());
	const String global_path = ProjectSettings::get_singleton()->globalize_path(p_path);
	if (!global_path.begins_with(source_root)) {
		String path_name = "File";
		switch (p_path_index) {
			case PATH_ID_HEADER:
				path_name = "Header file";
				break;
			case PATH_ID_SOURCE:
				path_name = "Source file";
				break;
		}
		p_validation_context->add_validation(EditorValidationContext::VALIDATION_SEVERITY_WARNING, vformat(TTR("%s path is outside the source root ('%s'). The file may not be included in the build."), path_name, source_root));
	}
}

void CppSourceCodePlugin::validate_template_option(EditorValidationContext *p_validation_context, const String &p_template_id, const String &p_option_name, const Variant &p_value) const {
	if (p_template_id == "object_hello_world" && p_option_name == "print_message") {
		String value = p_value;
		if (value.is_empty()) {
			p_validation_context->add_validation(EditorValidationContext::VALIDATION_SEVERITY_ERROR, TTR("Print message must be a non-empty string."));
			return;
		}
		if (value.begins_with(" ")) {
			p_validation_context->add_validation(EditorValidationContext::VALIDATION_SEVERITY_WARNING, TTR("Print message should not start with a space."));
		}
	}
}

Error CppSourceCodePlugin::add_method_func(const StringName &p_class_name, const String &p_method_name, const PackedStringArray &p_args) const {
	const String source_path = get_source_path(p_class_name);
	ERR_FAIL_COND_V_MSG(source_path.is_empty(), ERR_FILE_NOT_FOUND, vformat("Source file not found for class: %s", p_class_name));

	Vector<String> args;
	for (int i = 0; i < p_args.size(); i++) {
		String arg = p_args[i];
		String name = arg.get_slicec(':', 0).strip_edges();
		String type = arg.get_slicec(':', 1).strip_edges();
		args.push_back(vformat("%s %s", type, name));
	}

	const String args_str = String(", ").join(args);

	// Find both header and source files from the source_path, which may itself be a header.
	String header_path;
	String cpp_path;
	if (source_path.ends_with(".h") || source_path.ends_with(".hpp")) {
		header_path = source_path;
		cpp_path = source_path.get_basename() + ".cpp";
	} else {
		cpp_path = source_path;
		const String base = source_path.get_basename();
		if (FileAccess::exists(base + ".h")) {
			header_path = base + ".h";
		} else if (FileAccess::exists(base + ".hpp")) {
			header_path = base + ".hpp";
		}
	}

	// Append the method definition to the .cpp file.
	if (FileAccess::exists(cpp_path)) {
		Error err;
		Ref<FileAccess> cpp_file = FileAccess::open(cpp_path, FileAccess::READ_WRITE, &err);
		ERR_FAIL_COND_V_MSG(err != OK || cpp_file.is_null(), ERR_CANT_OPEN, vformat("Cannot open source file for writing: %s", cpp_path));
		cpp_file->seek_end();

		String method = R"(
void _CLASS_::_METHOD_(_ARGS_) {
	// Replace with method body.
}
)";

		method = method.replace("_CLASS_", p_class_name)
						 .replace("_METHOD_", p_method_name)
						 .replace("_ARGS_", args_str);

		bool ok = cpp_file->store_string(method);
		ERR_FAIL_COND_V_MSG(!ok, ERR_UNAVAILABLE, vformat("Failed to write to source file: %s", cpp_path));
	}

	// Insert the method declaration into the header file.
	// We'll just find the last '};' and insert the declaration before it. That should be good enough for a mockup.
	if (!header_path.is_empty() && FileAccess::exists(header_path)) {
		Error err;
		String header_content = FileAccess::get_file_as_string(header_path, &err);
		ERR_FAIL_COND_V_MSG(err != OK, ERR_CANT_OPEN, vformat("Cannot read header file: %s", header_path));

		String method = R"(
	void _METHOD_(_ARGS_);
)";

		method = method.replace("_METHOD_", p_method_name)
						 .replace("_ARGS_", args_str);

		const int last_brace = header_content.rfind("};");
		if (last_brace != -1) {
			header_content = header_content.substr(0, last_brace) + method + header_content.substr(last_brace);
			Ref<FileAccess> header_file = FileAccess::open(header_path, FileAccess::WRITE, &err);
			ERR_FAIL_COND_V_MSG(err != OK || header_file.is_null(), ERR_CANT_OPEN, vformat("Cannot open header file for writing: %s", header_path));
			bool ok = header_file->store_string(header_content);
			ERR_FAIL_COND_V_MSG(!ok, ERR_UNAVAILABLE, vformat("Failed to write to header file: %s", header_path));
		}
	}

	return OK;
}

CppSourceCodePlugin::CppSourceCodePlugin() {
	EDITOR_DEF_BASIC(EXTERNAL_EDITOR_SETTING, EXTERNAL_EDITOR_ID_NONE);
	EditorSettings::get_singleton()->add_property_hint(PropertyInfo(Variant::INT, EXTERNAL_EDITOR_SETTING, PROPERTY_HINT_ENUM, "Disabled:0,Visual Studio Code:1"));

	GLOBAL_DEF_BASIC(SOURCE_ROOT_SETTING, "res://../src");
	EditorSettings::get_singleton()->add_property_hint(PropertyInfo(Variant::STRING, SOURCE_ROOT_SETTING, PROPERTY_HINT_DIR));
}
