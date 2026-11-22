"""Check local file links in repository Markdown (no network or dependencies)."""

from pathlib import Path
import re
import subprocess
from urllib.parse import unquote, urlsplit


ROOT = Path(__file__).resolve().parents[1]


def main():
    paths = subprocess.check_output(
        ["git", "ls-files", "--cached", "--others", "--exclude-standard", "-z"],
        cwd=ROOT,
    ).decode().split("\0")
    failures = []
    count = 0
    for name in sorted(set(paths)):
        path = ROOT / name
        if path.suffix != ".md" or not path.is_file():
            continue
        fenced = False
        for number, line in enumerate(path.read_text(encoding="utf-8").splitlines(), 1):
            if line.lstrip().startswith(("```", "~~~")):
                fenced = not fenced
                continue
            if fenced:
                continue
            for match in re.finditer(r"\]\((<[^>]+>|[^)\s]+)\)", line):
                target = match.group(1).strip("<>")
                url = urlsplit(target)
                if url.scheme or url.netloc or not url.path:
                    continue
                count += 1
                if not (path.parent / unquote(url.path)).exists():
                    failures.append(f"{name}:{number}: arquivo ausente: {target}")
    if failures:
        print("\n".join(failures))
        return 1
    print(f"OK: {count} links locais para arquivos verificados.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
