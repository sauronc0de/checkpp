#!/usr/bin/env python3

from __future__ import annotations

import argparse
from pathlib import Path


def chunk_bytes(data: bytes, width: int = 12) -> str:
    lines = []
    for index in range(0, len(data), width):
        chunk = data[index:index + width]
        lines.append("    " + ", ".join(f"0x{byte:02x}" for byte in chunk))
    return ",\n".join(lines)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--input", required=True)
    parser.add_argument("--output", required=True)
    parser.add_argument("--namespace", required=True)
    parser.add_argument("--data-name", required=True)
    args = parser.parse_args()

    input_path = Path(args.input)
    output_path = Path(args.output)
    data = input_path.read_bytes()

    lines = [
        "// Generated from " + input_path.name,
        "#include <cstddef>",
        "",
        f"namespace {args.namespace} {{",
        f"inline constexpr unsigned char {args.data_name}[] = {{",
        chunk_bytes(data),
        "};",
        f"inline constexpr std::size_t {args.data_name}Size = sizeof({args.data_name});",
        "} // namespace " + args.namespace,
        "",
    ]

    output_path.write_text("\n".join(lines), encoding="utf-8")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
