#!/usr/bin/env python3
"""从 PDF 目录生成程序可读的数据文件"""
import os, shutil, subprocess, sys

SEP = '\x1E'  # FIELD_SEP
PDF_DIR = r"D:\大创\相关应用论文"
DATA_DIR = r"C:\Users\17154\Desktop\literature_management\CPlusPlus-project\data"
PDFTOOL = r"C:\Users\17154\Desktop\literature_management\CPlusPlus-project\ui\build\Desktop_Qt_6_11_1_MinGW_64_bit-Debug\tools\pdftotext.exe"

# 预先定义6篇论文的信息 (从pdftotext提取)
papers = [
    {"title": "Robust Online Multi-Object Tracking based on Tracklet Confidence and Online Discriminative Appearance Learning",
     "authors": ["Seung-Hwan Bae", "Kuk-Jin Yoon"],
     "year": "2014", "venue": "CVPR",
     "file": "Bae_Robust_Online_Multi-Object_2014_CVPR_paper.pdf",
     "keywords": ["multi-object tracking", "tracklet confidence", "online learning"]},
    {"title": "ByteTrack: Multi-Object Tracking by Associating Every Detection Box",
     "authors": ["Yifu Zhang", "Peize Sun", "Yi Jiang", "Dongdong Yu", "Fucheng Weng", "Zehuan Yuan", "Ping Luo", "Wenyu Liu", "Xinggang Wang"],
     "year": "2022", "venue": "ECCV",
     "file": "ByteTrack.pdf",
     "keywords": ["multi-object tracking", "data association", "ByteTrack"]},
    {"title": "Simple Online and Realtime Tracking with a Deep Association Metric",
     "authors": ["Nicolai Wojke", "Alex Bewley", "Dietrich Paulus"],
     "year": "2017", "venue": "ICIP",
     "file": "DeepSort.pdf",
     "keywords": ["multi-object tracking", "deep learning", "SORT", "DeepSORT"]},
    {"title": "MOT20: A Benchmark for Multi Object Tracking in Crowded Scenes",
     "authors": ["Patrick Dendorfer", "Hamid Rezatofighi", "Anton Milan", "Javen Shi", "Daniel Cremers", "Ian Reid", "Stefan Roth", "Konrad Schindler", "Laura Leal-Taixe"],
     "year": "2020", "venue": "CVPRW",
     "file": "MOT20.pdf",
     "keywords": ["multi-object tracking", "benchmark", "crowded scenes", "MOT20"]},
    {"title": "Faster R-CNN: Towards Real-Time Object Detection with Region Proposal Networks",
     "authors": ["Shaoqing Ren", "Kaiming He", "Ross Girshick", "Jian Sun"],
     "year": "2015", "venue": "NeurIPS",
     "file": "NIPS-FAST-RNN.pdf",
     "keywords": ["object detection", "Faster R-CNN", "region proposal network"]},
    {"title": "Simple Online and Realtime Tracking",
     "authors": ["Alex Bewley", "Zongyuan Ge", "Lionel Ott", "Fabio Ramos", "Ben Upcroft"],
     "year": "2016", "venue": "ICIP",
     "file": "SORT.pdf",
     "keywords": ["multi-object tracking", "Kalman filter", "Hungarian algorithm", "SORT"]},
]

def main():
    os.makedirs(DATA_DIR, exist_ok=True)
    pdfs_dir = os.path.join(DATA_DIR, "pdfs")
    os.makedirs(pdfs_dir, exist_ok=True)

    # 收集所有作者 (去重用 dict)
    author_map = {}  # name -> id
    author_id = 1
    for p in papers:
        for name in p["authors"]:
            if name not in author_map:
                author_map[name] = author_id
                author_id += 1

    # 生成 authors.txt
    with open(os.path.join(DATA_DIR, "authors", "authors.txt"), "w", encoding="utf-8") as f:
        for name, aid in author_map.items():
            parts = [str(aid), name, "", "", "", ""]  # id, name, gender, affiliation, email, researchAreas
            f.write(SEP.join(parts) + "\n")

    # 复制 PDF 并生成 papers.txt
    with open(os.path.join(DATA_DIR, "papers", "papers.txt"), "w", encoding="utf-8") as f:
        for i, p in enumerate(papers):
            paper_id = i + 1
            src = os.path.join(PDF_DIR, p["file"])
            dst = os.path.join(pdfs_dir, p["file"])
            if os.path.exists(src) and not os.path.exists(dst):
                shutil.copy2(src, dst)

            author_ids = [str(author_map[name]) for name in p["authors"]]
            parts = [
                str(paper_id),           # id
                "",                       # code (DOI)
                p["title"],              # title
                ";".join(p["keywords"]), # keywords
                "",                       # abstract
                p["year"],               # publishDate
                "-1",                     # sourceId
                "",                       # issue
                p["venue"],              # issueNumber (venue)
                "",                       # pageRange
                ";".join(author_ids),    # authorIds
                "",                       # attachmentIds
                dst,                      # filePath
                "2026-06-18 00:00:00",   # uploadTime
                "",                       # remark
            ]
            f.write(SEP.join(parts) + "\n")

    # 生成 catalogs.txt — 两个目录
    with open(os.path.join(DATA_DIR, "catalogs", "catalogs.txt"), "w", encoding="utf-8") as f:
        # 目录1: 多目标跟踪 (id=1, papers 1-5)
        mot_paper_ids = [str(i+1) for i in range(5)]  # papers 1-5
        parts = ["1", "多目标跟踪", "0", "MOT相关论文", "-1", "", ";".join(mot_paper_ids)]
        f.write(SEP.join(parts) + "\n")
        # 目录2: 目标检测 (id=2, paper 6)
        parts = ["2", "目标检测", "0", "Object Detection相关论文", "-1", "", "6"]
        f.write(SEP.join(parts) + "\n")

    # 生成 sources.txt (空)
    open(os.path.join(DATA_DIR, "sources", "sources.txt"), "w").close()
    # 生成 attachments.txt (空)
    open(os.path.join(DATA_DIR, "attachments", "attachments.txt"), "w").close()

    # 生成 library/library_data.txt (旧格式备份)
    print(f"生成完成: {len(papers)} 篇论文, {len(author_map)} 位作者, 2 个目录")
    print(f"数据路径: {DATA_DIR}")

if __name__ == "__main__":
    main()
