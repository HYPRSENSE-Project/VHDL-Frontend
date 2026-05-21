#!/usr/bin/env python3
##

"""Expand raw STL helper class nodes in the AST PlantUML diagram.

The generated AST diagram uses hashed PlantUML aliases for template types, for
example:

    class "::std::variant<library_clause *,use_clause *>" as C_123
    class C_123 {
    __
    }

This script keeps the hashed aliases and links intact, but makes the node label
readable and moves the full template definition into the class body.
"""

from __future__ import annotations

import argparse
import re
import subprocess
import sys
from collections.abc import Callable
from pathlib import Path


CLASS_DECL_RE = re.compile(r'^class\s+"(?P<label>[^"]+)"\s+as\s+(?P<id>C_\d+)\s*$')
RELATION_RE = re.compile(r'^(?P<src>C_\d+)\s+\S+\s+(?P<dst>C_\d+)\s*:\s*\+(?P<label>[A-Za-z_]\w*)\s*$')


def strip_cpp_comments(text: str) -> str:
    text = re.sub(r"/\*.*?\*/", "", text, flags=re.DOTALL)
    return re.sub(r"//.*", "", text)


def canonical_type(type_text: str) -> str:
    type_text = type_text.strip()
    if type_text.startswith("::"):
        type_text = type_text[2:]
    type_text = re.sub(r"\bast::", "", type_text)
    return re.sub(r"\s+", "", type_text)


def strip_trailing_indirection(type_text: str) -> tuple[str, str]:
    type_text = normalize_cpp_type(type_text)
    suffix = ""
    while type_text.endswith(("*", "&")):
        suffix = f" {type_text[-1]}{suffix}"
        type_text = type_text[:-1].strip()
    return type_text, suffix


def matching_angle(text: str, open_index: int) -> int:
    depth = 0
    for index in range(open_index, len(text)):
        if text[index] == "<":
            depth += 1
        elif text[index] == ">":
            depth -= 1
            if depth == 0:
                return index
    raise ValueError(f"unbalanced template argument list near: {text[open_index:open_index + 80]!r}")


def is_plain_template(type_text: str, template_name: str) -> bool:
    compact = canonical_type(type_text)
    if not compact.startswith(f"std::{template_name}<"):
        return False
    open_index = compact.find("<")
    try:
        return matching_angle(compact, open_index) == len(compact) - 1
    except ValueError:
        return False


def is_plain_variant(type_text: str) -> bool:
    return is_plain_template(type_text, "variant")


def is_plain_vector(type_text: str) -> bool:
    return is_plain_template(type_text, "vector")


def split_template_args(type_text: str) -> list[str]:
    open_index = type_text.find("<")
    close_index = matching_angle(type_text, open_index)
    args_text = type_text[open_index + 1 : close_index]

    args: list[str] = []
    start = 0
    depth = 0
    for index, char in enumerate(args_text):
        if char == "<":
            depth += 1
        elif char == ">":
            depth -= 1
        elif char == "," and depth == 0:
            args.append(args_text[start:index].strip())
            start = index + 1

    tail = args_text[start:].strip()
    if tail:
        args.append(tail)
    return args


def normalize_cpp_type(type_text: str) -> str:
    type_text = " ".join(type_text.strip().split())
    type_text = re.sub(r"\s*\*\s*", " *", type_text)
    type_text = re.sub(r"\s*&\s*", " &", type_text)
    type_text = re.sub(r"\s*,\s*", ", ", type_text)
    return type_text


def format_variant(type_text: str) -> list[str]:
    args = split_template_args(type_text)
    lines = ["  ::std::variant<\n"]
    for index, arg in enumerate(args):
        suffix = "," if index < len(args) - 1 else ">"
        lines.append(f"      {normalize_cpp_type(arg)}{suffix}\n")
    return lines


def render_cpp_type(type_text: str, alias_by_type: dict[str, str]) -> str:
    base_type, suffix = strip_trailing_indirection(type_text)
    alias = alias_by_type.get(canonical_type(base_type))
    if alias is not None:
        return f"{alias}{suffix}"
    return f"{normalize_cpp_type(base_type)}{suffix}"


def format_vector(type_text: str, alias_by_type: dict[str, str]) -> list[str]:
    args = split_template_args(type_text)
    lines = ["  ::std::vector<\n"]
    for index, arg in enumerate(args):
        suffix = "," if index < len(args) - 1 else ">"
        lines.append(f"      {render_cpp_type(arg, alias_by_type)}{suffix}\n")
    return lines


def vector_element_label(type_text: str, alias_by_type: dict[str, str]) -> str:
    base_type, _ = strip_trailing_indirection(type_text)
    alias = alias_by_type.get(canonical_type(base_type))
    if alias is not None:
        return alias

    base_type = re.sub(r"^(?:::)?(?:std|ast)::", "", base_type)
    base_type = re.sub(r"\b(?:std|ast)::", "", base_type)
    if "<" in base_type:
        base_type = base_type[: base_type.find("<")]
    base_type = base_type.replace("::", "_")
    base_type = re.sub(r"[^0-9A-Za-z_]+", "_", base_type)
    base_type = re.sub(r"_+", "_", base_type).strip("_")
    return base_type or "type"


def vector_display_label(type_text: str, alias_by_type: dict[str, str]) -> str:
    args = split_template_args(type_text)
    element_type = args[0] if args else "type"
    return f"{vector_element_label(element_type, alias_by_type)}_vector"


def parse_variant_aliases(header_text: str) -> dict[str, str]:
    aliases: dict[str, str] = {}
    text = strip_cpp_comments(header_text)
    for match in re.finditer(r"\busing\s+([A-Za-z_]\w*)\s*=\s*std::variant\s*<", text):
        name = match.group(1)
        open_index = text.find("<", match.start())
        close_index = matching_angle(text, open_index)
        variant_type = f"std::variant<{text[open_index + 1:close_index]}>"
        aliases.setdefault(canonical_type(variant_type), name)
    return aliases


def infer_single_incoming_labels(puml_text: str) -> dict[str, str]:
    incoming: dict[str, set[str]] = {}
    for line in puml_text.splitlines():
        match = RELATION_RE.match(line)
        if match:
            incoming.setdefault(match.group("dst"), set()).add(match.group("label"))
    return {class_id: next(iter(labels)) for class_id, labels in incoming.items() if len(labels) == 1}


def template_type_from_body(
    body_lines: list[str], predicate: Callable[[str], bool], prefixes: tuple[str, ...]
) -> str | None:
    first_content_index = None
    for index, line in enumerate(body_lines):
        stripped = line.strip()
        if stripped:
            first_content_index = index
            break

    if first_content_index is None:
        return None

    first = body_lines[first_content_index].strip()
    if not predicate(first) and not first.startswith(prefixes):
        return None

    collected: list[str] = []
    depth = 0
    for line in body_lines[first_content_index:]:
        stripped = line.strip()
        if stripped in {"==", "__"}:
            break
        collected.append(stripped)
        depth += stripped.count("<")
        depth -= stripped.count(">")
        if collected and depth == 0:
            candidate = "".join(collected)
            return candidate if predicate(candidate) else None
    return None


def variant_type_from_body(body_lines: list[str]) -> str | None:
    return template_type_from_body(body_lines, is_plain_variant, ("::std::variant<", "std::variant<"))


def vector_type_from_body(body_lines: list[str]) -> str | None:
    return template_type_from_body(body_lines, is_plain_vector, ("::std::vector<", "std::vector<"))


def rewrite_puml(puml_text: str, alias_by_type: dict[str, str]) -> tuple[str, dict[str, int], int]:
    incoming_labels = infer_single_incoming_labels(puml_text)
    lines = puml_text.splitlines(keepends=True)
    rewritten: list[str] = []
    changed = 0
    matched = {"std::variant": 0, "std::vector": 0}
    index = 0

    while index < len(lines):
        match = CLASS_DECL_RE.match(lines[index].rstrip("\n"))
        if not match or index + 1 >= len(lines):
            rewritten.append(lines[index])
            index += 1
            continue

        class_id = match.group("id")
        body_header = lines[index + 1].rstrip("\n")
        if body_header != f"class {class_id} {{":
            rewritten.append(lines[index])
            index += 1
            continue

        end_index = index + 2
        while end_index < len(lines) and lines[end_index].rstrip("\n") != "}":
            end_index += 1
        if end_index >= len(lines):
            rewritten.append(lines[index])
            index += 1
            continue

        label = match.group("label")
        body_lines = lines[index + 2 : end_index]
        variant_type = label if is_plain_variant(label) else variant_type_from_body(body_lines)
        vector_type = None if variant_type is not None else label if is_plain_vector(label) else vector_type_from_body(body_lines)

        if variant_type is not None:
            matched["std::variant"] += 1
            display_label = alias_by_type.get(canonical_type(variant_type), incoming_labels.get(class_id, label))
            new_block = [
                f'class "{display_label}" as {class_id}\n',
                f"class {class_id} {{\n",
                *format_variant(variant_type),
                "==\n",
                "__\n",
                "}\n",
            ]
        elif vector_type is not None:
            matched["std::vector"] += 1
            display_label = vector_display_label(vector_type, alias_by_type)
            new_block = [
                f'class "{display_label}" as {class_id}\n',
                f"class {class_id} {{\n",
                *format_vector(vector_type, alias_by_type),
                "==\n",
                "__\n",
                "}\n",
            ]
        else:
            rewritten.extend(lines[index : end_index + 1])
            index = end_index + 1
            continue

        old_block = lines[index : end_index + 1]
        if old_block != new_block:
            changed += 1
        rewritten.extend(new_block)
        index = end_index + 1

    return "".join(rewritten), matched, changed


def run_command(command: list[str]) -> None:
    print("+ " + " ".join(command), flush=True)
    subprocess.run(command, check=True)


def run_clang_uml(args: argparse.Namespace, generator: str) -> None:
    command = [
        args.clang_uml_bin,
        "-c",
        args.clang_uml_config,
        "-n",
        args.diagram_name,
        "-g",
        generator,
    ]
    run_command(command)


def mermaid_output_path(args: argparse.Namespace) -> Path:
    return args.puml.parent / f"{args.diagram_name}.mmd"


def run_mermaid(args: argparse.Namespace) -> None:
    run_clang_uml(args, "mermaid")
    output_path = mermaid_output_path(args)
    if output_path.exists():
        print(f"{output_path}: generated Mermaid source")


def run_plantuml(args: argparse.Namespace) -> None:
    command = [args.plantuml_bin, f"-t{args.plantuml_format}", str(args.puml)]
    run_command(command)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "puml",
        nargs="?",
        default="docs/diagrams/ast_class_diagram.puml",
        help="PlantUML file to rewrite in place",
    )
    parser.add_argument(
        "--ast-header",
        default="src/ast/ast_nodes.h",
        help="C++ AST header used to map std::variant types to using aliases",
    )
    parser.add_argument(
        "--clang-uml-config",
        default=".clang-uml",
        help="clang-uml configuration file used to regenerate the PlantUML source",
    )
    parser.add_argument(
        "--diagram-name",
        default="ast_class_diagram",
        help="clang-uml diagram name to regenerate",
    )
    parser.add_argument("--clang-uml-bin", default="clang-uml", help="clang-uml executable")
    parser.add_argument("--plantuml-bin", default="plantuml", help="PlantUML executable")
    parser.add_argument("--plantuml-format", default="svg", help="PlantUML output format")
    parser.add_argument("--skip-clang-uml", action="store_true", help="rewrite the existing PUML file only")
    parser.add_argument("--skip-mermaid", action="store_true", help="do not generate Mermaid source")
    parser.add_argument("--skip-plantuml", action="store_true", help="do not render the rewritten PUML file")
    parser.add_argument(
        "--check",
        action="store_true",
        help="exit with status 1 if the current PUML file would change; does not run external tools",
    )
    parser.add_argument("--dry-run", action="store_true", help="show the rewrite summary without writing changes")
    args = parser.parse_args()

    puml_path = Path(args.puml)
    args.puml = puml_path

    if not args.check and not args.dry_run and not args.skip_clang_uml:
        run_clang_uml(args, "plantuml")
        if not args.skip_mermaid:
            run_mermaid(args)

    puml_text = puml_path.read_text()

    alias_by_type: dict[str, str] = {}
    ast_header = Path(args.ast_header)
    if ast_header.exists():
        alias_by_type.update(parse_variant_aliases(ast_header.read_text()))

    new_text, matched, changed = rewrite_puml(puml_text, alias_by_type)
    summary = (
        f"matched {matched['std::variant']} std::variant and "
        f"{matched['std::vector']} std::vector class block(s), changed {changed}"
    )

    if args.check:
        if new_text != puml_text:
            print(f"{puml_path}: needs rewrite ({summary})", file=sys.stderr)
            return 1
        print(f"{puml_path}: already up to date ({summary})")
        return 0

    if args.dry_run:
        print(f"{puml_path}: dry run ({summary})")
        return 0

    if new_text != puml_text:
        puml_path.write_text(new_text)
    print(f"{puml_path}: {summary}")

    if not args.skip_plantuml:
        run_plantuml(args)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
