from pathlib import Path
from zipfile import ZIP_DEFLATED, ZipFile
import re


ROOT = Path(__file__).resolve().parent
SOURCE = ROOT / "山东大学_隆山飞驰_卡丁快跑本科组_技术报告.docx"
TARGET = ROOT / "final_report_render_only.docx"


def strip_hyperlink_wrappers(xml: bytes) -> bytes:
    text = xml.decode("utf-8")
    text = re.sub(r"<w:hyperlink\b[^>]*>", "", text)
    text = text.replace("</w:hyperlink>", "")
    return text.encode("utf-8")


with ZipFile(SOURCE, "r") as src, ZipFile(TARGET, "w", ZIP_DEFLATED) as dst:
    for item in src.infolist():
        data = src.read(item.filename)
        if item.filename == "word/document.xml":
            data = strip_hyperlink_wrappers(data)
        dst.writestr(item, data)

print(TARGET)
