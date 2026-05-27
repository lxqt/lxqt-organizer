#!/usr/bin/env python3
import pathlib
import re
import sys

ROOT = pathlib.Path(__file__).resolve().parents[1]
SRC = ROOT / "src"
SOURCE_SUFFIXES = {".h", ".hpp", ".cpp", ".cc"}

SERVICES = SRC / "services"
WARNING_RE = re.compile(r"\bqWarning\s*\(")
LOCAL_INCLUDE_RE = re.compile(r'^\s*#\s*include\s+"([^"]+)"')

LAYER_PREFIXES = (
    ("app", "src/app/"),
    ("ui", "src/ui/"),
    ("services", "src/services/"),
    ("storage_repo", "src/storage/repo/"),
    ("storage_fs", "src/storage/fs/"),
    ("storage_api", "src/storage/api/"),
    ("domain", "src/domain/"),
)

ALLOWED_LAYER_INCLUDES = {
    "app": {
        "app",
        "ui",
        "services",
        "storage_repo",
        "storage_fs",
        "storage_api",
        "domain",
    },
    "ui": {"ui", "services", "storage_api", "domain"},
    "services": {"services", "storage_repo", "storage_fs", "storage_api", "domain"},
    "storage_repo": {"storage_repo", "storage_fs", "storage_api", "domain"},
    "storage_fs": {"storage_fs", "storage_api", "domain"},
    "storage_api": {"storage_api", "domain"},
    "domain": {"domain"},
}

ALLOWED_SNAPSHOT_CAST_FILES = {
    "src/domain/calendarsnapshot.cpp",
}
SNAPSHOT_CONST_CAST_RE = re.compile(
    r"qSharedPointerConstCast\s*<[^>]*\b"
    r"(KCalendarCore::(?:Event|Todo|MemoryCalendar)|KContacts::Addressee)\b"
)


def iter_sources(base):
    return sorted(path for path in base.rglob("*") if path.suffix in SOURCE_SUFFIXES)


def rel(path):
    return path.relative_to(ROOT).as_posix()


def layer_for_relpath(rel_file):
    for layer, prefix in LAYER_PREFIXES:
        if rel_file.startswith(prefix):
            return layer
    return None


def include_index():
    by_name = {}
    for path in iter_sources(SRC):
        rel_file = rel(path)
        by_name.setdefault(path.name, []).append(rel_file)
        by_name.setdefault(rel_file.removeprefix("src/"), []).append(rel_file)
    return by_name


def check_service_warnings():
    failures = []
    for path in iter_sources(SERVICES):
        for line_number, line in enumerate(
            path.read_text(encoding="utf-8").splitlines(), 1
        ):
            if WARNING_RE.search(line):
                failures.append(
                    f"{path.relative_to(ROOT).as_posix()}:{line_number}: use qCWarning(storageLog)"
                )
    return "Service warning category check", failures


def check_snapshot_aliasing():
    failures = []
    for path in iter_sources(SRC):
        rel_file = rel(path)
        if rel_file in ALLOWED_SNAPSHOT_CAST_FILES:
            continue
        for line_number, line in enumerate(
            path.read_text(encoding="utf-8").splitlines(), 1
        ):
            if SNAPSHOT_CONST_CAST_RE.search(line):
                failures.append(
                    f"{rel_file}:{line_number}: forbidden qSharedPointerConstCast onto a snapshot type"
                )
    return "Snapshot aliasing check", failures


def check_layer_includes():
    failures = []
    includes = include_index()
    for path in iter_sources(SRC):
        source_rel = rel(path)
        source_layer = layer_for_relpath(source_rel)
        if source_layer is None:
            continue
        allowed_layers = ALLOWED_LAYER_INCLUDES[source_layer]
        for line_number, line in enumerate(
            path.read_text(encoding="utf-8").splitlines(), 1
        ):
            match = LOCAL_INCLUDE_RE.match(line)
            if not match:
                continue
            include = match.group(1)
            candidates = includes.get(include, [])
            if not candidates:
                candidates = includes.get(pathlib.PurePosixPath(include).name, [])
            target_layers = {
                layer_for_relpath(candidate)
                for candidate in candidates
                if layer_for_relpath(candidate) is not None
            }
            if not target_layers:
                continue
            disallowed = sorted(target_layers - allowed_layers)
            if disallowed:
                failures.append(
                    f"{source_rel}:{line_number}: {source_layer} may not include "
                    f"{include} from {', '.join(disallowed)}"
                )
    return "Layer include check", failures


def main():
    exit_code = 0
    for check in (
        check_service_warnings,
        check_snapshot_aliasing,
        check_layer_includes,
    ):
        name, failures = check()
        if failures:
            exit_code = 1
            print(f"{name} failed:", file=sys.stderr)
            for failure in failures:
                print(failure, file=sys.stderr)
        else:
            print(f"{name} passed.")
    return exit_code


if __name__ == "__main__":
    raise SystemExit(main())
