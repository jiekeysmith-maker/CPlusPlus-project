#!/usr/bin/env python3
"""从 PDF 目录生成程序可读的数据文件 + 随机合成数据补足 ≥100 条"""
import datetime, os, random, shutil

SEP = '\x1E'

def random_upload_time():
    y = random.randint(2020, 2025)
    m = random.randint(1, 12)
    d = random.randint(1, 28)
    h = random.randint(8, 22)
    mi = random.randint(0, 59)
    s = random.randint(0, 59)
    return f"{y:04d}-{m:02d}-{d:02d} {h:02d}:{mi:02d}:{s:02d}"

PDF_DIR = r"D:\大创\相关应用论文"
DATA_DIR = r"C:\Users\17154\Desktop\literature_management\CPlusPlus-project\data"

# ── 随机数据池 ──────────────────────────────────────────────
PREFIXES = [
    "A Novel", "An Improved", "Towards", "Efficient", "Robust",
    "Deep", "End-to-End", "Real-Time", "Self-Supervised",
    "Attention-Based", "Lightweight", "Multi-Scale", "Hierarchical",
    "Adaptive", "Context-Aware", "Learning-Based", "Generalized",
    "Unified", "Scalable", "Accurate", "Rethinking", "Revisiting",
    "Hybrid", "Progressive", "Cross-Modal",
]

SUBJECTS = [
    "Object Detection", "Semantic Segmentation", "Instance Segmentation",
    "Panoptic Segmentation", "Neural Radiance Field", "3D Gaussian Splatting",
    "Diffusion Model", "Graph Neural Network", "Vision Transformer",
    "Reinforcement Learning", "Contrastive Learning", "Knowledge Distillation",
    "Image Super-Resolution", "Video Understanding", "3D Reconstruction",
    "Point Cloud Processing", "Depth Estimation", "Optical Flow",
    "Pose Estimation", "Few-Shot Learning", "Domain Adaptation",
    "Anomaly Detection", "Image Inpainting", "Style Transfer",
    "Text-to-Image Generation", "Multi-Modal Learning", "Feature Matching",
    "Object Tracking", "Scene Graph Generation", "Visual Question Answering",
    "Image Captioning", "Action Recognition", "Face Recognition",
    "Person Re-Identification", "Crowd Counting", "Lane Detection",
    "Visual SLAM", "Trajectory Prediction", "Image Deblurring",
    "Image Denoising", "Novel View Synthesis", "Neural Rendering",
    "Open-Vocabulary Detection", "Referring Expression Comprehension",
    "Temporal Action Localization", "Video Object Segmentation",
    "Stereo Matching", "Monocular Depth Estimation",
]

SUFFIXES = [
    "in Complex Scenes", "with Limited Data", "via Multi-Scale Fusion",
    "for Autonomous Driving", "under Domain Shift", "using Vision Transformers",
    "in Adverse Weather", "at Night", "from RGB-D Data",
    "on Edge Devices", "with Weak Supervision", "via Self-Attention",
    "in Real Time", "with Occlusion Handling", "via Knowledge Transfer",
    "through Adversarial Training", "with Curriculum Learning",
    "in Low-Data Regimes", "on Point Clouds", "from Monocular Images",
    "via Test-Time Adaptation", "with Geometric Priors",
    "through Neural Architecture Search", "in the Wild",
    "with Noisy Labels", "via Distribution Alignment",
    "for Resource-Constrained Devices", "in Continual Learning Settings",
]

KEYWORDS = [
    "deep learning", "computer vision", "neural network", "attention mechanism",
    "feature extraction", "representation learning", "transfer learning",
    "data augmentation", "backbone network", "multi-scale",
    "transformer", "convolution", "self-attention", "cross-attention",
    "knowledge distillation", "model compression", "pruning",
    "quantization", "object detection", "segmentation",
    "generative model", "diffusion", "GAN", "VAE",
    "reinforcement learning", "contrastive learning", "self-supervised",
    "semi-supervised", "few-shot", "zero-shot",
    "domain adaptation", "generalization", "adversarial robustness",
    "federated learning", "meta-learning", "active learning",
    "image processing", "video analysis", "point cloud",
]

VENUES = [
    "CVPR", "ICCV", "ECCV", "NeurIPS", "ICML", "ICLR",
    "AAAI", "IJCAI", "ICRA", "IROS", "WACV", "BMVC",
    "ACCV", "MICCAI", "ACL", "EMNLP", "ICASSP", "CoRL",
]

FIRST_NAMES = [
    "James", "John", "Robert", "Michael", "William", "David",
    "Richard", "Thomas", "Mark", "Steven", "Edward", "Brian",
    "Kevin", "Jason", "Matthew", "Daniel", "Christopher",
    "Andrew", "Paul", "George", "Eric", "Stephen", "Peter",
    "Henry", "Charles", "Anthony", "Joseph", "Wei", "Ming",
    "Hao", "Jun", "Lei", "Yang", "Tao", "Xin", "Bo",
    "Feng", "Gang", "Hua", "Jian", "Long", "Min",
    "Ning", "Peng", "Qiang", "Rui", "Xiao", "Yan",
    "Zhi", "Kai", "Yu", "Cheng", "Jing", "Li",
]

LAST_NAMES = [
    "Smith", "Johnson", "Williams", "Brown", "Jones",
    "Garcia", "Miller", "Davis", "Rodriguez", "Wang",
    "Zhang", "Li", "Liu", "Chen", "Yang", "Huang",
    "Zhao", "Wu", "Zhou", "Xu", "Sun", "Ma", "Zhu",
    "Hu", "Lin", "He", "Guo", "Luo", "Liang", "Song",
    "Tang", "Han", "Feng", "Yu", "Dong", "Cheng",
    "Cao", "Deng", "Xie", "Peng", "Jiang", "Shen",
    "Su", "Lu", "Ye", "Cai", "Pan", "Tian",
]

AFFILIATIONS = [
    "Tsinghua University", "Peking University",
    "Zhejiang University", "Shanghai Jiao Tong University",
    "Stanford University", "MIT", "UC Berkeley", "CMU",
    "University of Oxford", "ETH Zurich", "KAIST",
    "Nanyang Technological University", "Microsoft Research",
    "Google Research", "Meta AI", "NVIDIA Research",
    "ByteDance AI Lab", "Baidu Research",
    "Alibaba DAMO Academy", "Tencent AI Lab",
    "Huawei Noah's Ark Lab", "SenseTime Research",
    "Megvii Research", "University of Tokyo",
    "National University of Singapore",
]

# ── 真实论文（从 PDF 提取的元数据）───────────────────────────
real_papers = [
    {"title": "Robust Online Multi-Object Tracking based on Tracklet Confidence and Online Discriminative Appearance Learning",
     "authors": ["Seung-Hwan Bae", "Kuk-Jin Yoon"],
     "year": "2014", "venue": "CVPR",
     "file": "Bae_Robust_Online_Multi-Object_2014_CVPR_paper.pdf",
     "keywords": ["multi-object tracking", "tracklet confidence", "online learning"],
     "abstract": "In this paper, we propose a robust online multi-object tracking method based on tracklet confidence and online discriminative appearance learning."},
    {"title": "ByteTrack: Multi-Object Tracking by Associating Every Detection Box",
     "authors": ["Yifu Zhang", "Peize Sun", "Yi Jiang", "Dongdong Yu", "Fucheng Weng", "Zehuan Yuan", "Ping Luo", "Wenyu Liu", "Xinggang Wang"],
     "year": "2022", "venue": "ECCV",
     "file": "ByteTrack.pdf",
     "keywords": ["multi-object tracking", "data association", "ByteTrack"],
     "abstract": "ByteTrack is a simple yet effective multi-object tracking method that associates almost every detection box instead of only high-score ones."},
    {"title": "Simple Online and Realtime Tracking with a Deep Association Metric",
     "authors": ["Nicolai Wojke", "Alex Bewley", "Dietrich Paulus"],
     "year": "2017", "venue": "ICIP",
     "file": "DeepSort.pdf",
     "keywords": ["multi-object tracking", "deep learning", "SORT", "DeepSORT"],
     "abstract": "We integrate appearance information to improve the performance of SORT, reducing identity switches across frames."},
    {"title": "MOT20: A Benchmark for Multi Object Tracking in Crowded Scenes",
     "authors": ["Patrick Dendorfer", "Hamid Rezatofighi", "Anton Milan", "Javen Shi", "Daniel Cremers", "Ian Reid", "Stefan Roth", "Konrad Schindler", "Laura Leal-Taixe"],
     "year": "2020", "venue": "CVPRW",
     "file": "MOT20.pdf",
     "keywords": ["multi-object tracking", "benchmark", "crowded scenes", "MOT20"],
     "abstract": "MOT20 is a benchmark for multi-object tracking in crowded scenes, addressing limitations of previous datasets."},
    {"title": "Faster R-CNN: Towards Real-Time Object Detection with Region Proposal Networks",
     "authors": ["Shaoqing Ren", "Kaiming He", "Ross Girshick", "Jian Sun"],
     "year": "2015", "venue": "NeurIPS",
     "file": "NIPS-FAST-RNN.pdf",
     "keywords": ["object detection", "Faster R-CNN", "region proposal network"],
     "abstract": "We introduce a Region Proposal Network that shares convolutional features with the detection network, enabling nearly cost-free region proposals."},
    {"title": "Simple Online and Realtime Tracking",
     "authors": ["Alex Bewley", "Zongyuan Ge", "Lionel Ott", "Fabio Ramos", "Ben Upcroft"],
     "year": "2016", "venue": "ICIP",
     "file": "SORT.pdf",
     "keywords": ["multi-object tracking", "Kalman filter", "Hungarian algorithm", "SORT"],
     "abstract": "SORT is a pragmatic approach to multiple object tracking with focus on simple yet effective algorithms."},
]

TOTAL_TARGET = 105
SYNTHETIC_COUNT = TOTAL_TARGET - len(real_papers)  # 99


def make_synthetic_paper(paper_id, author_id_map, used_authors):
    """生成一条合成文献数据，返回 (paper_dict, new_author_list)"""
    prefix = random.choice(PREFIXES)
    subject = random.choice(SUBJECTS)
    suffix = random.choice(SUFFIXES)
    title = f"{prefix} {subject} {suffix}"

    year = random.randint(2015, 2025)
    venue = random.choice(VENUES)
    kw_count = random.randint(2, 5)
    keywords = random.sample(KEYWORDS, kw_count)
    abstract = f"We propose {prefix.lower()} approach for {subject.lower()} {suffix.lower()}."

    # 生成 2-6 位作者
    num_authors = random.randint(2, 6)
    new_authors = []
    author_ids_for_paper = []
    for _ in range(num_authors):
        first = random.choice(FIRST_NAMES)
        last = random.choice(LAST_NAMES)
        name = f"{first} {last}"
        if name not in author_id_map:
            author_id_map[name] = len(author_id_map) + 1
            aff = random.choice(AFFILIATIONS)
            email = f"{first.lower()}.{last.lower()}@{aff.split()[0].lower()}.edu"
            new_authors.append({
                "id": author_id_map[name],
                "name": name,
                "affiliation": aff,
                "email": email,
            })
        author_ids_for_paper.append(str(author_id_map[name]))

    return {
        "title": title,
        "year": str(year),
        "venue": venue,
        "keywords": keywords,
        "abstract": abstract,
        "author_ids": author_ids_for_paper,
    }, new_authors


def main():
    random.seed(2026)

    os.makedirs(os.path.join(DATA_DIR, "authors"), exist_ok=True)
    os.makedirs(os.path.join(DATA_DIR, "papers"), exist_ok=True)
    os.makedirs(os.path.join(DATA_DIR, "catalogs"), exist_ok=True)
    os.makedirs(os.path.join(DATA_DIR, "sources"), exist_ok=True)
    os.makedirs(os.path.join(DATA_DIR, "attachments"), exist_ok=True)
    os.makedirs(os.path.join(DATA_DIR, "pdfs"), exist_ok=True)
    pdfs_dir = os.path.join(DATA_DIR, "pdfs")

    # ── 第一遍：收集真实论文的作者 ─────────────────────────
    author_map = {}  # name -> id (int, 1-based)
    for p in real_papers:
        for name in p["authors"]:
            if name not in author_map:
                author_map[name] = len(author_map) + 1

    # ── 第二遍：生成合成文献 + 追加作者 ──────────────────────
    all_papers = []
    paper_id = 1

    # 真实论文
    for i, p in enumerate(real_papers):
        author_ids = [str(author_map[name]) for name in p["authors"]]
        all_papers.append({
            "id": paper_id,
            "title": p["title"],
            "year": p["year"],
            "venue": p["venue"],
            "keywords": p["keywords"],
            "abstract": p.get("abstract", ""),
            "author_ids": author_ids,
            "file": p["file"],
            "is_real": True,
        })
        paper_id += 1

    # 合成论文
    for _ in range(SYNTHETIC_COUNT):
        syn, new_authors = make_synthetic_paper(paper_id, author_map, set())
        for a in new_authors:
            pass  # author_map already updated inside make_synthetic_paper
        syn["id"] = paper_id
        syn["file"] = ""
        syn["is_real"] = False
        all_papers.append(syn)
        paper_id += 1

    # ── 输出 authors.txt ────────────────────────────────────
    with open(os.path.join(DATA_DIR, "authors", "authors.txt"), "w", encoding="utf-8") as f:
        for name, aid in author_map.items():
            # 查找该作者的 affiliation 和 email（可能在合成作者信息中不完整）
            parts = [str(aid), name, "", "", "", ""]
            f.write(SEP.join(parts) + "\n")

    print(f"[authors] {len(author_map)} 位作者")

    # ── 输出 papers.txt ────────────────────────────────────
    paper_count = 0
    with open(os.path.join(DATA_DIR, "papers", "papers.txt"), "w", encoding="utf-8") as f:
        for p in all_papers:
            # 复制真实 PDF（如果存在且未复制过）
            dst = ""
            if p["is_real"] and p["file"]:
                src = os.path.join(PDF_DIR, p["file"])
                dst = os.path.join(pdfs_dir, p["file"])
                if os.path.exists(src) and not os.path.exists(dst):
                    shutil.copy2(src, dst)

            parts = [
                str(p["id"]),                    # id
                "",                              # code (DOI)
                p["title"],                      # title
                ";".join(p["keywords"]),         # keywords
                p.get("abstract", ""),           # abstract
                p["year"],                       # publishDate
                "-1",                            # sourceId
                "",                              # issue
                p["venue"],                      # issueNumber (存会议名)
                "",                              # pageRange
                ";".join(p["author_ids"]),       # authorIds
                "",                              # attachmentIds
                dst,                             # filePath
                random_upload_time(),           # uploadTime
                "",                              # remark
            ]
            f.write(SEP.join(parts) + "\n")
            paper_count += 1

    print(f"[papers] {paper_count} 篇文献")

    # ── 生成出版物（期刊 & 会议）─────────────────────────────
    SOURCE_JOURNALS = [
        "Nature", "Science", "IEEE TPAMI", "IJCV", "CVPR", "TNNLS",
        "Pattern Recognition", "Neurocomputing", "Neural Networks",
        "IEEE TIP", "JMLR", "ACM Computing Surveys", "AI Journal",
    ]
    SOURCE_CONFERENCES = [
        "CVPR", "ICCV", "ECCV", "NeurIPS", "ICML", "ICLR",
        "AAAI", "IJCAI", "ACL", "EMNLP", "ICRA", "IROS",
        "SIGGRAPH", "SIGKDD", "WWW", "ACM MM", "WACV", "BMVC",
    ]

    sources = []  # (id, type, shortName)
    source_id = 1
    for name in SOURCE_JOURNALS:
        sources.append((source_id, "Journal", name))
        source_id += 1
    for name in SOURCE_CONFERENCES:
        sources.append((source_id, "Conference", name))
        source_id += 1

    # 随机给 ~60% 的论文分配出版物
    paper_source = {}  # paper_id -> source_id
    for p in all_papers:
        if random.random() < 0.6:
            sid, stype, sname = random.choice(sources)
            paper_source[p["id"]] = sid

    # 输出 sources.txt
    with open(os.path.join(DATA_DIR, "sources", "sources.txt"), "w", encoding="utf-8") as f:
        for sid, stype, sname in sources:
            if stype == "Journal":
                parts = [str(sid), sname, "", "", "", "", "", "", "0.0"]
            else:
                parts = [str(sid), sname, "", "", "", "", "", "", ""]
            f.write(SEP.join(parts) + "\n")
    print(f"[sources] {len(sources)} 个出版物")

    # 重写 papers.txt（带上 sourceId）
    with open(os.path.join(DATA_DIR, "papers", "papers.txt"), "w", encoding="utf-8") as f:
        for p in all_papers:
            dst = ""
            if p["is_real"] and p["file"]:
                src = os.path.join(PDF_DIR, p["file"])
                dst = os.path.join(pdfs_dir, p["file"])
                if os.path.exists(src) and not os.path.exists(dst):
                    shutil.copy2(src, dst)
            sid = str(paper_source.get(p["id"], -1))
            parts = [
                str(p["id"]),                    # id
                "",                              # code
                p["title"],                      # title
                ";".join(p["keywords"]),         # keywords
                p.get("abstract", ""),           # abstract
                p["year"],                       # publishDate
                sid,                             # sourceId
                "",                              # issue
                p["venue"],                      # issueNumber
                "",                              # pageRange
                ";".join(p["author_ids"]),       # authorIds
                "",                              # attachmentIds
                dst,                             # filePath
                random_upload_time(),           # uploadTime
                "",                              # remark
            ]
            f.write(SEP.join(parts) + "\n")

    # ── 输出 catalogs.txt ──────────────────────────────────
    catalog_lines = []  # 收集目录行，后续传给 write_combined
    catalog_id = 1

    mot_ids = [str(i) for i in range(1, 6)]
    catalog_lines.append(SEP.join([str(catalog_id), "多目标跟踪", "0", "MOT相关论文", "-1", "", ";".join(mot_ids)]))
    catalog_id += 1

    det_ids = ["6"]
    for p in all_papers:
        if p["id"] > 6 and "detection" in p["title"].lower():
            det_ids.append(str(p["id"]))
            if len(det_ids) >= 25: break
    catalog_lines.append(SEP.join([str(catalog_id), "目标检测", "0", "Object Detection相关论文", "-1", "", ";".join(det_ids)]))
    catalog_id += 1

    seg_ids = [str(p["id"]) for p in all_papers if "segmentation" in p["title"].lower()]
    if seg_ids:
        catalog_lines.append(SEP.join([str(catalog_id), "图像分割", "0", "Segmentation相关论文", "-1", "", ";".join(seg_ids)]))
        catalog_id += 1

    gen_ids = [str(p["id"]) for p in all_papers
               if any(kw in p["title"].lower() for kw in ["diffusion", "nerf", "generat", "splatting", "gan"])]
    if gen_ids:
        catalog_lines.append(SEP.join([str(catalog_id), "生成模型", "0", "生成式AI相关论文", "-1", "", ";".join(gen_ids)]))
        catalog_id += 1

    trans_ids = [str(p["id"]) for p in all_papers
                 if any(kw in p["title"].lower() for kw in ["transformer", "attention", "self-attention"])]
    if trans_ids:
        catalog_lines.append(SEP.join([str(catalog_id), "Transformer架构", "0", "Transformer相关论文", "-1", "", ";".join(trans_ids)]))
        catalog_id += 1

    all_ids = [str(p["id"]) for p in all_papers]
    catalog_lines.append(SEP.join([str(catalog_id), "全部文献", "0", "所有文献汇总", "-1", "", ";".join(all_ids)]))

    with open(os.path.join(DATA_DIR, "catalogs", "catalogs.txt"), "w", encoding="utf-8") as f:
        for line in catalog_lines:
            f.write(line + "\n")

    print(f"[catalogs] {len(catalog_lines)} 个目录")

    # ── 空文件 ─────────────────────────────────────────────
    open(os.path.join(DATA_DIR, "attachments", "attachments.txt"), "w").close()

    # ── 输出合并版 library_data.txt ─────────────────────────
    write_combined(author_map, all_papers, sources, paper_source, catalog_lines)

    # ── 数据有效性校验 ─────────────────────────────────────
    validate(all_papers, author_map)

    print(f"\n[完成] 生成: {paper_count} 篇论文, {len(author_map)} 位作者")
    print(f"   数据路径: {DATA_DIR}")


def write_combined(author_map, all_papers, sources=None, paper_source=None, catalog_lines=None):
    """输出合并单文件 library/library_data.txt 作为备份"""
    if paper_source is None:
        paper_source = {}
    if catalog_lines is None:
        catalog_lines = []
    lib_dir = os.path.join(DATA_DIR, "library")
    os.makedirs(lib_dir, exist_ok=True)
    with open(os.path.join(lib_dir, "library_data.txt"), "w", encoding="utf-8") as f:
        f.write("[AUTHORS]\n")
        for name, aid in author_map.items():
            parts = [str(aid), name, "", "", "", ""]
            f.write(SEP.join(parts) + "\n")

        f.write("[SOURCES]\n")
        if sources:
            for sid, stype, sname in sources:
                if stype == "Journal":
                    inner = SEP.join([str(sid), sname, "", "", "", "", "", "", "0.0"])
                else:
                    inner = SEP.join([str(sid), sname, "", "", "", "", "", "", ""])
                f.write(stype + SEP + inner + "\n")

        f.write("[PAPERS]\n")
        for p in all_papers:
            sid = str(paper_source.get(p["id"], -1))
            parts = [
                str(p["id"]), "", p["title"],
                ";".join(p["keywords"]), p.get("abstract", ""),
                p["year"], sid, "", p["venue"], "",
                ";".join(p["author_ids"]), "", "",
                random_upload_time(), "",
            ]
            f.write(SEP.join(parts) + "\n")

        f.write("[ATTACHMENTS]\n")
        f.write("[CATALOGS]\n")
        for line in catalog_lines:
            f.write(line + "\n")


def validate(all_papers, author_map):
    """校验生成数据的有效性"""
    errors = []
    paper_ids = set()
    for p in all_papers:
        pid = p["id"]
        if pid in paper_ids:
            errors.append(f"文献ID重复: {pid}")
        paper_ids.add(pid)

        if not p["title"].strip():
            errors.append(f"文献 {pid}: 标题为空")
        if not p["author_ids"]:
            errors.append(f"文献 {pid}: 无作者")
        for aid_str in p["author_ids"]:
            aid = int(aid_str)
            if aid < 1 or aid > len(author_map):
                errors.append(f"文献 {pid}: 作者ID {aid} 越界")

    if len(all_papers) < 100:
        errors.append(f"文献总数 {len(all_papers)} < 100，不满足课程要求")

    if errors:
        print("\n[警告] 校验发现问题:")
        for e in errors:
            print(f"  - {e}")
    else:
        print(f"[校验] 通过 — {len(all_papers)} 条文献，{len(author_map)} 位作者，数据完整")


if __name__ == "__main__":
    main()
