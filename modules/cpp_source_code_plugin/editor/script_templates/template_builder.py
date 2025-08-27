"""Functions used to generate C++ script template source files during build time."""

import os

import methods


def _to_snake_case(name):
    result = ""
    for i, c in enumerate(name):
        if c.isupper() and i > 0 and name[i - 1].islower():
            result += "_"
        result += c.lower()
    return result


def make_templates(target, source, env):
    def escape(s):
        return s.replace("\\", "\\\\").replace('"', '\\"').replace("\n", "\\n").replace("\t", "\\t")

    def read(path):
        with open(path, "r", encoding="utf-8") as f:
            return f.read()

    def parse_meta(path):
        meta = {"name": "", "description": "", "parameters": []}
        with open(path, "r", encoding="utf-8") as f:
            lines = f.readlines()
        i = 0
        while i < len(lines):
            line = lines[i].rstrip("\n")
            if not line.strip():
                i += 1
                continue
            if line == "parameter:":
                param = {}
                i += 1
                while i < len(lines) and (not lines[i].strip() or lines[i].startswith("    ")):
                    pline = lines[i].rstrip("\n")
                    if pline.startswith("    "):
                        k, _, v = pline[4:].partition(": ")
                        param[k] = v
                    i += 1
                meta["parameters"].append(param)
            else:
                k, _, v = line.partition(": ")
                meta[k] = v
                i += 1
        return meta

    # Group files by (inherits, basename) → {suffix: path}.
    groups = {}
    for node in source:
        path = str(node)
        inherits = os.path.basename(os.path.dirname(path))
        filename = os.path.basename(path)
        dot = filename.find(".")
        basename, suffix = filename[:dot], filename[dot:]
        groups.setdefault((inherits, basename), {})[suffix] = path

    # TEMPLATE_FILES: all non-meta files keyed by "Inherits/basename.ext".
    file_lines = []
    for (inherits, basename), files in sorted(groups.items()):
        for suffix, path in sorted(files.items()):
            if suffix != ".metadata":
                file_lines.append(f'\tf["{inherits}/{basename}{suffix}"] = "{escape(read(path))}";')

    # TEMPLATES: one entry per group with a .metadata file.
    template_entries = []
    for (inherits, basename), files in sorted(groups.items()):
        if ".metadata" not in files:
            continue
        meta = parse_meta(files[".metadata"])
        if not meta["name"]:
            meta["name"] = basename.replace("_", " ").title()
        template_id = _to_snake_case(inherits) + "_" + basename
        params = ", ".join(
            f'{{ Variant::{p["type"]}, "{p["name"]}", {p["default_value"]} }}'
            for p in meta["parameters"]
        )
        template_entries.append(
            f'\t{{ "{inherits}", "{meta["name"]}", "{meta["description"]}", "{template_id}",\n'
            f'\t\tTEMPLATE_FILES["{inherits}/{basename}.h"], TEMPLATE_FILES["{inherits}/{basename}.cpp"],\n'
            f"\t\t{{ {params} }} }}"
        )

    with methods.generated_wrapper(str(target[0])) as file:
        file.write(
            '#include "cpp_script_template.h"\n\n'
            '#include "core/templates/hash_map.h"\n\n'
            "static HashMap<String, String> TEMPLATE_FILES = []() {\n"
            "\tHashMap<String, String> f;\n"
            + "\n".join(file_lines)
            + "\n\treturn f;\n"
            "}();\n\n"
            f"inline constexpr int TEMPLATES_ARRAY_SIZE = {len(template_entries)};\n"
            "static CppScriptTemplate TEMPLATES[TEMPLATES_ARRAY_SIZE] = {\n"
            + ",\n".join(template_entries)
            + "\n};\n"
        )
