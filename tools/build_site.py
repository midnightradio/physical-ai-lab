#!/usr/bin/env python3
"""Build the static lab and Arduino ZIPs using only Python's standard library."""

from html.parser import HTMLParser
from io import BytesIO
from pathlib import Path
import shutil
from urllib.parse import unquote, urlsplit
from zipfile import ZIP_DEFLATED, ZipFile, ZipInfo


ROOT = Path(__file__).resolve().parents[1]
OUTPUT = ROOT / "_site"
PROJECTS = {
    "motion": "esp32_motion_capture",
    "image": "esp32_image_capture",
    "audio": "esp32_audio_capture",
}


def code_zip(project, sketch):
    folder = ROOT / project
    files = {
        "README.md": (folder / "README.md").read_bytes(),
        f"{sketch}/{sketch}.ino": (folder / sketch / f"{sketch}.ino").read_bytes(),
    }
    stream = BytesIO()
    with ZipFile(stream, "w", compression=ZIP_DEFLATED) as archive:
        for name, data in sorted(files.items()):
            entry = ZipInfo(name, date_time=(2026, 1, 1, 0, 0, 0))
            entry.create_system = 3
            entry.external_attr = 0o100644 << 16
            entry.compress_type = ZIP_DEFLATED
            archive.writestr(entry, data)
    result = stream.getvalue()
    with ZipFile(BytesIO(result)) as archive:
        if archive.testzip() is not None or set(archive.namelist()) != set(files):
            raise ValueError(f"Invalid ZIP for {project}")
        for name, expected in files.items():
            if archive.read(name) != expected:
                raise ValueError(f"Source mismatch in {project}: {name}")
    return result


class PageLinks(HTMLParser):
    def __init__(self):
        super().__init__()
        self.links = []
        self.ids = set()

    def handle_starttag(self, tag, attrs):
        attrs = dict(attrs)
        if attrs.get("id"):
            if attrs["id"] in self.ids:
                raise ValueError(f"Duplicate HTML id: {attrs['id']}")
            self.ids.add(attrs["id"])
        for key in ("href", "src"):
            if attrs.get(key):
                self.links.append(attrs[key])


def validate_links():
    pages = {}
    for page in OUTPUT.rglob("*.html"):
        parser = PageLinks()
        parser.feed(page.read_text(encoding="utf-8"))
        pages[page] = parser
    checked = 0
    for page, parser in pages.items():
        for link in parser.links:
            url = urlsplit(link)
            if url.scheme or url.netloc:
                continue
            path = unquote(url.path)
            target = ((OUTPUT / path.lstrip("/")) if path.startswith("/")
                      else (page.parent / path) if path else page).resolve()
            if not target.is_relative_to(OUTPUT):
                raise ValueError(f"Link leaves site: {page.name}: {link}")
            if target.is_dir():
                target /= "index.html"
            if not target.is_file():
                raise ValueError(f"Missing target in {page.relative_to(OUTPUT)}: {link}")
            if url.fragment and target in pages and unquote(url.fragment) not in pages[target].ids:
                raise ValueError(f"Missing section in {page.relative_to(OUTPUT)}: {link}")
            checked += 1
    return len(pages), checked


def main():
    # Only this generated directory is cleared; source files are never rewritten.
    if OUTPUT.exists():
        shutil.rmtree(OUTPUT)
    OUTPUT.mkdir()
    shutil.copy2(ROOT / "index.html", OUTPUT / "index.html")
    shutil.copytree(ROOT / "assets", OUTPUT / "assets")
    (OUTPUT / ".nojekyll").touch()
    for project, sketch in PROJECTS.items():
        source = ROOT / project
        target = OUTPUT / project
        target.mkdir()
        for name in ("index.html", "README.md"):
            shutil.copy2(source / name, target / name)
        (target / sketch).mkdir()
        shutil.copy2(source / sketch / f"{sketch}.ino", target / sketch / f"{sketch}.ino")
        downloads = target / "downloads"
        downloads.mkdir()
        (downloads / f"{project}-code.zip").write_bytes(code_zip(project, sketch))
    pages, links = validate_links()
    print(f"Built _site: {pages} pages, {links} local links, 3 verified Arduino code ZIPs.")


if __name__ == "__main__":
    main()
