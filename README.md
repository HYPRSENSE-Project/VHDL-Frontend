[![CMake Build  & Test](https://github.com/HYPRSENSE-Project/VHDL-Frontend/actions/workflows/cmake-single-platform.yml/badge.svg)](https://github.com/HYPRSENSE-Project/VHDL-Frontend/actions/workflows/cmake-single-platform.yml)

# VHDL-Frontend

The current state uses the grammar from [hdlConvertor](https://github.com/Nic30/hdlConvertor.git).
The parser is a complete reimplementation with the goal to create an elaborated versions of the primary unit(s).
The AST can be found [src/ast/ast_nodes.h](https://github.com/HYPRSENSE-Project/VHDL-Frontend/blob/feature/standalone_parser/src/ast/ast_nodes.h).
It holds the parsed elements as well as the resolved references after elaboration.

## Embedded VHDL packages

The build embeds every `.vhdl` file under `contrib` into the
`vhdl_fe::embedded_vhdl` static library. The parser links this library publicly,
so applications and tests linking `vhdl_fe::parser` can use it directly:

```cpp
#include <embedded_vhdl.h>
#include <parser.h>
#include <sstream>
#include <string>

auto source = embedded_vhdl::get("ieee/std_logic_1164.vhdl");
std::istringstream stream{std::string(source)};

parser::parser parser;
auto* package = parser.parse_str(std::string(source), parser::encoding::UTF_8,
                                 "ieee", "ieee/std_logic_1164.vhdl");
```

Resource names use `/` separators and are relative to `contrib`. `get()` returns
a `std::string_view` with static lifetime and throws `std::out_of_range` for an
unknown name. Strings and stringstreams can be constructed from the view as
shown above. The contents retain their original bytes and encoding. The optional
fourth argument of `parse_str()` supplies a filename for source locations and
diagnostics; it defaults to `<string>`. Use `parser::encoding::ISO_8859_1` when
parsing `ieee/numeric_bit.vhdl` or `ieee/numeric_std.vhdl`, which contain Latin-1
comments.

CMake runs `scripts/embed_vhdl.py` to generate a single C++ source file in the
build directory, and regenerates it when the inputs change. No package files
are needed at runtime, and packages are parsed only when requested by the caller.
The embedded sources retain their copyright notices; see `contrib/LICENSE` for
their license.

The grammar is missing the following keywords which are reserved in VHDL2008:

 * assume
 * assume_guarantee
 * cover
 * default (wrong syntax)
 * fairness
 * restrict
 * restrict_guarantee
 * strong
 * vmode
 * vprop
