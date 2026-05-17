import sys
from clang.cindex import Index, CursorKind, Config

# Optional, depending on your system:
# Config.set_library_file("/usr/lib/llvm-17/lib/libclang.so")

def walk(cursor, namespace_stack=None):
    if namespace_stack is None:
        namespace_stack = []

    for child in cursor.get_children():
        if child.kind == CursorKind.NAMESPACE:
            yield from walk(child, namespace_stack + [child.spelling])

        elif child.kind == CursorKind.STRUCT_DECL:
            # Skip anonymous structs and pure forward declarations
            if not child.spelling:
                continue

            # Only emit actual definitions from the target file
            if not child.is_definition():
                continue

            yield namespace_stack, child.spelling

        else:
            yield from walk(child, namespace_stack)

def main():
    filename = sys.argv[1]

    index = Index.create()
    tu = index.parse(
        filename,
        args=[
            "-std=c++17",
            "-x", "c++",
        ],
    )
    lst = tu.cursor.get_children()
    emitted_namespaces = []
    current_ns = []

    for namespaces, struct_name in sorted(walk(tu.cursor)):
        if len(namespaces)==0 or namespaces[0] in ['std', '__gnu_cxx']:
            continue
        # Close namespaces that are no longer active
        common = 0
        for a, b in zip(current_ns, namespaces):
            if a == b:
                common += 1
            else:
                break

        for ns in reversed(current_ns[common:]):
            print(f"}} // namespace {ns}")

        for ns in namespaces[common:]:
            print(f"namespace {ns} {{")

        current_ns = namespaces
        print(f"struct {struct_name};")

    for ns in reversed(current_ns):
        print(f"}} // namespace {ns}")

if __name__ == "__main__":
    main()