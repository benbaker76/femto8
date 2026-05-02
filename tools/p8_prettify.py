#!/usr/bin/env python3
"""
p8_prettify.py - Pretty print PICO-8 Lua code

Takes a .lua, .p8, or .p8.png file and pretty-prints the Lua code.
"""

import sys
import re
import os

try:
    from lark import Lark, Tree, Token
except ImportError:
    print("Error: lark-parser not installed. Install with: pip install lark-parser", file=sys.stderr)
    sys.exit(1)


def extract_lua_from_p8(filename):
    """Extract Lua code section from a .p8 file."""
    content = None
    used_latin1 = False
    for encoding in ['utf-8', 'latin-1']:
        try:
            with open(filename, 'r', encoding=encoding) as f:
                content = f.read()
            used_latin1 = (encoding == 'latin-1')
            break
        except UnicodeDecodeError:
            if encoding == 'latin-1':
                raise

    lua_start = content.find('__lua__')
    if lua_start == -1:
        return None

    lua_start = content.find('\n', lua_start) + 1
    lua_end = content.find('\n__', lua_start)
    if lua_end == -1:
        lua_end = len(content)

    lua_code = content[lua_start:lua_end]

    return lua_code


def extract_full_p8(filename):
    """Extract full .p8 file content with all sections."""
    content = None
    used_latin1 = False
    for encoding in ['utf-8', 'latin-1']:
        try:
            with open(filename, 'r', encoding=encoding) as f:
                content = f.read()
            used_latin1 = (encoding == 'latin-1')
            break
        except UnicodeDecodeError:
            if encoding == 'latin-1':
                raise

    lua_start = content.find('__lua__')
    if lua_start == -1:
        return None, None, None

    header = content[:lua_start]
    lua_marker_end = content.find('\n', lua_start) + 1
    lua_end = content.find('\n__', lua_marker_end)

    if lua_end == -1:
        lua_code = content[lua_marker_end:]
        other_sections = ""
    else:
        lua_code = content[lua_marker_end:lua_end]
        other_sections = content[lua_end:]

    return header + "__lua__\n", lua_code, other_sections


def extract_full_p8png(filename):
    """Extract full .p8 content from a .p8.png file."""
    try:
        import subprocess
        import tempfile

        with tempfile.NamedTemporaryFile(mode='w', suffix='.p8', delete=False) as tmp:
            tmp_name = tmp.name

        try:
            script_dir = os.path.dirname(os.path.abspath(__file__))
            png_to_p8 = os.path.join(script_dir, '..', 'build-linux', 'png_to_p8')

            if os.path.exists(png_to_p8):
                subprocess.run([png_to_p8, filename, tmp_name], check=True,
                             capture_output=True)
                result = extract_full_p8(tmp_name)
                os.unlink(tmp_name)
                return result
        except:
            if os.path.exists(tmp_name):
                os.unlink(tmp_name)
    except:
        pass

    return None, None, None


def extract_lua_from_p8png(filename):
    """Extract Lua code from a .p8.png file by reading it as a .p8 cart."""
    try:
        import subprocess
        import tempfile

        with tempfile.NamedTemporaryFile(mode='w', suffix='.p8', delete=False) as tmp:
            tmp_name = tmp.name

        try:
            script_dir = os.path.dirname(os.path.abspath(__file__))
            png_to_p8 = os.path.join(script_dir, '..', 'build-linux', 'png_to_p8')

            if os.path.exists(png_to_p8):
                subprocess.run([png_to_p8, filename, tmp_name], check=True,
                             capture_output=True)
                lua_code = extract_lua_from_p8(tmp_name)
                os.unlink(tmp_name)
                return lua_code
        except:
            if os.path.exists(tmp_name):
                os.unlink(tmp_name)
    except:
        pass

    return None


def load_grammar():
    """Load the Lua grammar file."""
    script_dir = os.path.dirname(os.path.abspath(__file__))
    grammar_file = os.path.join(script_dir, 'lua_pico8.lark')

    if not os.path.exists(grammar_file):
        return None

    with open(grammar_file, 'r') as f:
        return f.read()


def _find_matching_paren(s, start):
    """Return the index of ')' closing the '(' at s[start], or -1."""
    depth = 0
    for i in range(start, len(s)):
        if s[i] == '(':
            depth += 1
        elif s[i] == ')':
            depth -= 1
            if depth == 0:
                return i
    return -1


def _lower_keywords_in_cond(cond):
    """Lowercase Lua boolean/logic keywords inside a condition string."""
    return re.sub(
        r'\b(not|and|or|true|false|nil)\b',
        lambda m: m.group(0).lower(),
        cond,
        flags=re.IGNORECASE,
    )


def _split_comment(s):
    """Split a Lua source fragment into (code_part, comment_part).

    Handles '--' only if not inside a string literal.  Returns
    (s, '') if no comment is found.
    """
    in_str = None
    i = 0
    while i < len(s):
        c = s[i]
        if in_str:
            if c == '\\':
                i += 2
                continue
            if c == in_str:
                in_str = None
        else:
            if c in ('"', "'"):
                in_str = c
            elif s[i:i+2] == '--':
                return s[:i], s[i:]
        i += 1
    return s, ''


def expand_shorthand_one_line_if_while(code):
    """Expand single-line shorthand forms:
    IF (cond) stmt...  -> if cond then stmt... end
    WHILE (cond) stmt... -> while cond do stmt... end

    Brackets around the condition are required (and may contain nested
    parentheses such as function calls). This is case-insensitive and
    preserves indentation.
    """
    out_lines = []
    for line in code.splitlines():
        stripped = line.lstrip()
        indent = line[: len(line) - len(stripped)]
        low = stripped.lower()

        for keyword in ('if', 'while'):
            klen = len(keyword)
            # keyword must be followed by a non-identifier character
            if not (low.startswith(keyword) and
                    len(stripped) > klen and
                    not (stripped[klen].isalnum() or stripped[klen] == '_')):
                continue

            after_kw = stripped[klen:].lstrip()
            if not after_kw.startswith('('):
                break

            # position of '(' in the stripped string
            paren_pos = len(stripped) - len(after_kw)
            close = _find_matching_paren(stripped, paren_pos)
            if close < 0:
                break

            cond = stripped[paren_pos + 1: close]
            body = stripped[close + 1:].lstrip()

            body_low = body.lower()
            if keyword == 'if' and (body_low.startswith('then') or
                                    body_low.startswith('do')):
                break
            if keyword == 'while' and body_low.startswith('do'):
                break
            # If the character after the closing paren looks like a binary
            # operator continuation, this is NOT shorthand — the parens just
            # group part of the condition expression.
            if body and body[0] in '=<>!~+*/%^&|.\\':
                break

            cond = _lower_keywords_in_cond(cond)
            joiner = 'then' if keyword == 'if' else 'do'
            # Split body into code + trailing line-comment so the closing
            # 'end' is not swallowed inside the comment.
            body_code, body_comment = _split_comment(body)
            body_code = body_code.rstrip()
            if body_comment:
                line = f"{indent}{keyword} {cond} {joiner} {body_code} end {body_comment}"
            else:
                line = f"{indent}{keyword} {cond} {joiner} {body_code} end"
            break

        out_lines.append(line)

    return '\n'.join(out_lines) + ('\n' if code.endswith('\n') else '')


def prettify_lua(code):
    """Pretty print Lua code using Lark parser."""
    grammar = load_grammar()
    if grammar is None:
        print("Error: lua_pico8.lark grammar file not found", file=sys.stderr)
        sys.exit(1)

    # expand PICO-8 shorthand if/while forms before parsing
    code = expand_shorthand_one_line_if_while(code)

    try:
        parser = Lark(grammar, start='chunk', parser='lalr')
        tree = parser.parse(code)
        return format_tree(tree, 0)
    except Exception as e:
        print(f"Error: Parse error: {e}", file=sys.stderr)
        sys.exit(1)


def format_tree(tree, indent=0):
    """Format a Lark parse tree into pretty-printed Lua code."""
    if isinstance(tree, Token):
        return str(tree.value if hasattr(tree, 'value') else tree)

    if not isinstance(tree, Tree):
        return str(tree)

    ind = '  ' * indent
    rule = tree.data
    children = tree.children

    if rule == 'chunk':
        lines = []
        for child in children:
            formatted = format_tree(child, indent)
            if formatted:
                lines.append(formatted)
        return '\n'.join(lines) + '\n' if lines else ''

    elif rule == 'assignment':
        varlist = format_tree(children[0], indent)
        explist = format_tree(children[1], indent)
        return f"{ind}{varlist} = {explist}"

    elif rule == 'compound_assign':
        suffixedexp = format_tree(children[0], indent)
        op = format_tree(children[1], indent)
        exp = format_tree(children[2], indent)
        return f"{ind}{suffixedexp} {op} {exp}"

    elif rule == 'print_shorthand':
        explist = format_tree(children[0], indent)
        return f"{ind}print({explist})"

    elif rule == 'call_stat' or rule == 'expr_stat':
        expr = format_tree(children[0], indent)
        return f"{ind}{expr}"

    elif rule == 'do_block':
        block = format_tree(children[0], indent + 1).rstrip('\n')
        return f"{ind}do\n{block}\n{ind}end"

    elif rule == 'while_loop':
        exp = format_tree(children[0], indent)
        block = format_tree(children[1], indent + 1).rstrip('\n')
        return f"{ind}while {exp} do\n{block}\n{ind}end"

    elif rule == 'repeat_loop':
        block = format_tree(children[0], indent + 1).rstrip('\n')
        exp = format_tree(children[1], indent)
        return f"{ind}repeat\n{block}\n{ind}until {exp}"

    elif rule == 'if_stat':
        result = [f"{ind}if {format_tree(children[0], indent)} then"]
        result.append(format_tree(children[1], indent + 1).rstrip('\n'))

        i = 2
        while i < len(children):
            child = children[i]
            if isinstance(child, Tree):
                if i + 1 < len(children) and children[i + 1].data == 'block':
                    result.append(f"{ind}elseif {format_tree(child, indent)} then")
                    result.append(format_tree(children[i + 1], indent + 1).rstrip('\n'))
                    i += 2
                elif child.data == 'block':
                    result.append(f"{ind}else")
                    result.append(format_tree(child, indent + 1).rstrip('\n'))
                    i += 1
                else:
                    i += 1
            else:
                i += 1

        result.append(f"{ind}end")
        return '\n'.join(result)

    elif rule == 'for_num':
        name = format_tree(children[0], indent)
        start = format_tree(children[1], indent)
        stop = format_tree(children[2], indent)
        if len(children) == 5:
            step = format_tree(children[3], indent)
            block = format_tree(children[4], indent + 1).rstrip('\n')
            return f"{ind}for {name} = {start}, {stop}, {step} do\n{block}\n{ind}end"
        else:
            block = format_tree(children[3], indent + 1).rstrip('\n')
            return f"{ind}for {name} = {start}, {stop} do\n{block}\n{ind}end"

    elif rule == 'for_in':
        namelist = format_tree(children[0], indent)
        explist = format_tree(children[1], indent)
        block = format_tree(children[2], indent + 1).rstrip('\n')
        return f"{ind}for {namelist} in {explist} do\n{block}\n{ind}end"

    elif rule == 'func_def':
        funcname = format_tree(children[0], indent)
        funcbody = format_tree(children[1], indent)
        return f"{ind}function {funcname}{funcbody}"

    elif rule == 'local_func':
        name = str(children[0])
        funcbody = format_tree(children[1], indent)
        return f"{ind}local function {name}{funcbody}"

    elif rule == 'local_assign_init':
        namelist = format_tree(children[0], indent)
        explist = format_tree(children[1], indent)
        return f"{ind}local {namelist} = {explist}"

    elif rule == 'local_assign':
        namelist = format_tree(children[0], indent)
        return f"{ind}local {namelist}"

    elif rule == 'return_stat':
        if children:
            explist = format_tree(children[0], indent)
            return f"{ind}return {explist}"
        return f"{ind}return"

    elif rule == 'break_stat':
        return f"{ind}break"

    elif rule == 'label':
        name = format_tree(children[0], indent)
        return f"{ind}::{name}::"

    elif rule == 'goto_stat':
        name = format_tree(children[0], indent)
        return f"{ind}goto {name}"

    elif rule == 'block':
        lines = []
        for child in children:
            formatted = format_tree(child, indent)
            if formatted:
                lines.append(formatted)
        return '\n'.join(lines) + '\n' if lines else ''

    elif rule == 'funcbody':
        params = format_tree(children[0], indent) if children[0].data != 'block' else ''
        block_idx = 1 if children[0].data != 'block' else 0
        block = format_tree(children[block_idx], indent + 1).rstrip('\n')
        return f"({params})\n{block}\n{ind}end"

    elif rule == 'func_exp':
        return format_tree(children[0], indent)

    elif rule == 'function':
        funcbody = format_tree(children[0], indent)
        return f"function{funcbody}"

    elif rule == 'varlist' or rule == 'namelist' or rule == 'explist':
        return ', '.join(format_tree(child, indent) for child in children)

    elif rule == 'funcname_method':
        parts = [format_tree(child, indent) for child in children]
        return '.'.join(parts[:-1]) + ':' + parts[-1]

    elif rule == 'funcname_plain':
        parts = [format_tree(child, indent) for child in children]
        return '.'.join(parts)

    else:
        parts = [format_tree(child, indent) for child in children]

        if rule == 'call':
            return ''.join(parts)
        elif rule == 'method_call':
            return f"{parts[0]}:{parts[1]}{parts[2]}"
        elif rule == 'binop_exp':
            return f"{parts[0]} {parts[1]} {parts[2]}"
        elif rule == 'unop_exp':
            op = parts[0]
            if op == 'not':
                return f"{op} {parts[1]}"
            else:
                return f"{op}{parts[1]}"
        elif rule == 'paren_exp':
            return f"({parts[0]})"
        elif rule == 'args_explist':
            return f"({parts[0]})" if parts else "()"
        elif rule == 'args_string':
            return parts[0] if parts else ""
        elif rule == 'args_table':
            return parts[0] if parts else ""
        elif rule == 'tableconstructor':
            if parts:
                return '{' + parts[0] + '}'
            return '{}'
        elif rule == 'fieldlist':
            return ', '.join(p for p in parts if p)
        elif rule.startswith('field_'):
            if rule == 'field_index':
                return f"[{parts[0]}] = {parts[1]}"
            elif rule == 'field_named':
                return f"{parts[0]} = {parts[1]}"
            else:
                return parts[0]
        elif rule == 'var_name':
            return ''.join(parts)
        elif rule == 'var_index':
            return f"{parts[0]}[{parts[1]}]"
        elif rule == 'var_attr':
            return f"{parts[0]}.{parts[1]}"
        elif rule == 'suffixed_name':
            return ''.join(parts)
        elif rule == 'suffixed_paren':
            return f"({parts[0]})"
        elif rule == 'suffixed_index':
            return f"{parts[0]}[{parts[1]}]"
        elif rule == 'suffixed_attr':
            return f"{parts[0]}.{parts[1]}"
        elif rule == 'suffixed_call':
            return f"{parts[0]}{parts[1]}"
        elif rule == 'suffixed_method':
            return f"{parts[0]}:{parts[1]}{parts[2]}"
        elif rule == 'parlist_names_vararg':
            return ', '.join(parts) + ', ...'
        elif rule == 'parlist_names':
            return ', '.join(parts)
        elif rule == 'parlist_vararg':
            return '...'
        elif rule == 'nil_exp':
            return 'nil'
        elif rule == 'false_exp':
            return 'false'
        elif rule == 'true_exp':
            return 'true'
        elif rule == 'vararg_exp':
            return '...'
        else:
            return ''.join(parts)


def main():
    if len(sys.argv) < 2:
        print("Usage: p8_prettify.py [--lua-only] <input.p8|input.p8.png|input.lua> [output]")
        print("\nPretty prints PICO-8 Lua code.")
        print("\nOptions:")
        print("  --lua-only    Output only Lua code (default preserves all sections for .p8/.p8.png)")
        sys.exit(1)

    lua_only = False
    args = sys.argv[1:]

    if args and args[0] == '--lua-only':
        lua_only = True
        args = args[1:]

    if len(args) < 1:
        print("Error: Input file required")
        sys.exit(1)

    input_file = args[0]
    output_file = args[1] if len(args) > 1 else None

    if not os.path.exists(input_file):
        print(f"Error: File not found: {input_file}")
        sys.exit(1)

    is_p8_input = input_file.endswith('.p8') or input_file.endswith('.p8.png')
    preserve_sections = is_p8_input and not lua_only

    if output_file is None:
        if lua_only:
            if input_file.endswith('.lua'):
                print("Error: Output filename required when input is .lua (default would overwrite input)")
                sys.exit(1)
            elif input_file.endswith('.p8.png'):
                output_file = input_file[:-7] + '.lua'
            elif input_file.endswith('.p8'):
                output_file = input_file[:-3] + '.lua'
            else:
                output_file = input_file + '.lua'
        else:
            if input_file.endswith('.p8'):
                print("Error: Output filename required when input is .p8 (default would overwrite input)")
                sys.exit(1)
            elif input_file.endswith('.p8.png'):
                output_file = input_file[:-7] + '.p8'
            else:
                output_file = input_file + '.p8'

    header = None
    other_sections = None
    lua_code = None

    if input_file.endswith('.p8.png'):
        if preserve_sections:
            header, lua_code, other_sections = extract_full_p8png(input_file)
        else:
            lua_code = extract_lua_from_p8png(input_file)
        if lua_code is None:
            print(f"Error: Could not extract Lua code from {input_file}")
            sys.exit(1)
    elif input_file.endswith('.p8'):
        if preserve_sections:
            header, lua_code, other_sections = extract_full_p8(input_file)
        else:
            lua_code = extract_lua_from_p8(input_file)
        if lua_code is None:
            print(f"Error: Could not find __lua__ section in {input_file}")
            sys.exit(1)
    elif input_file.endswith('.lua'):
        with open(input_file, 'r', encoding='utf-8', errors='ignore') as f:
            lua_code = f.read()
    else:
        lua_code = extract_lua_from_p8(input_file)
        if lua_code is None:
            with open(input_file, 'r', encoding='utf-8', errors='ignore') as f:
                lua_code = f.read()

    pretty_code = prettify_lua(lua_code)

    with open(output_file, 'w', encoding='utf-8') as f:
        if preserve_sections:
            f.write(header)
            f.write(pretty_code)
            if other_sections:
                f.write(other_sections)
        else:
            f.write(pretty_code)

    if preserve_sections:
        print(f"Pretty printed .p8 cart written to: {output_file}")
    else:
        print(f"Pretty printed Lua code written to: {output_file}")


if __name__ == '__main__':
    main()
