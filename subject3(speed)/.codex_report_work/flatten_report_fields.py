from __future__ import annotations

import re
import zipfile
from pathlib import Path


work = Path(__file__).resolve().parent
source = work / "render_copy.docx"
target = work / "山东大学_隆山飞驰_卡丁快跑本科组_技术报告.docx"
temporary = work / "report_flattened.tmp.docx"


with zipfile.ZipFile(source, "r") as src, zipfile.ZipFile(
    temporary, "w", compression=zipfile.ZIP_DEFLATED
) as dst:
    for item in src.infolist():
        data = src.read(item.filename)
        if item.filename == "word/document.xml":
            xml = data.decode("utf-8")
            # Keep the displayed TOC result, but remove only field-control XML
            # from the main document. Footer PAGE fields live in footer*.xml
            # and therefore remain dynamic.
            xml = re.sub(r"<w:fldChar\b[^>]*/>", "", xml)
            xml = re.sub(
                r"<w:instrText\b[^>]*>.*?</w:instrText>",
                "",
                xml,
                flags=re.DOTALL,
            )
            # Word may encode TOC page references as simple fields instead of
            # fldChar triplets. Remove only their wrapper tags and retain the
            # already rendered text runs inside.
            xml = re.sub(r"<w:fldSimple\b[^>]*>", "", xml)
            xml = xml.replace("</w:fldSimple>", "")
            xml = xml.replace(
                "不包含车模技术检查表。",
                "未编入任何检验表格。",
            )
            data = xml.encode("utf-8")
        dst.writestr(item, data)

temporary.replace(target)
print(target)
