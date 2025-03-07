import os
import os.path as osp
import pprint
import random

import matplotlib as mpl
import matplotlib.pyplot as plt
import numpy as np
import skimage.io
import skimage.transform
import torch
import yaml
from docopt import docopt
from PIL import Image
from PIL import ImageDraw

from cutline import segment_image_by_extended_lines
from cutline import clear_output_directory

import lcnn
from lcnn.config import C, M
from lcnn.models.line_vectorizer import LineVectorizer
from lcnn.models.multitask_learner import MultitaskHead, MultitaskLearner
from lcnn.postprocess import postprocess
from lcnn.utils import recursive_to

PLTOPTS = {"color": "#33FFFF", "s": 15, "edgecolors": "none", "zorder": 5}
cmap = plt.get_cmap("jet")
norm = mpl.colors.Normalize(vmin=0.9, vmax=1.0)
sm = plt.cm.ScalarMappable(cmap=cmap, norm=norm)
sm.set_array([])


def c(x):
    return sm.to_rgba(x)


def cut(imname):
    print("cut start!")
    config_file = "D:/CG/Texture_projection/LCNN/config/wireframe.yaml"
    checkpoint_file = "D:/CG/Texture_projection/LCNN/checkpoint/checkpoint_best_3.pth"
    print("C.update!")
    C.update(C.from_yaml(filename=config_file))
    print("M.update!")
    M.update(C.model)
    pprint.pprint(C, indent=4)

    random.seed(0)
    np.random.seed(0)
    torch.manual_seed(0)

    print("choose device!")
    device_name = "cpu"
    os.environ["CUDA_VISIBLE_DEVICES"] = "0"
    if torch.cuda.is_available():
        device_name = "cuda"
        torch.backends.cudnn.deterministic = True
        torch.cuda.manual_seed(0)
        print("Let's use", torch.cuda.device_count(), "GPU(s)!")
    else:
        print("CUDA is not available")
    device = torch.device(device_name)
    checkpoint = torch.load(checkpoint_file, map_location=device)

    # Load model
    print("load model!")
    model = lcnn.models.hg(
        depth=M.depth,
        head=lambda c_in, c_out: MultitaskHead(c_in, c_out),
        num_stacks=M.num_stacks,
        num_blocks=M.num_blocks,
        num_classes=sum(sum(M.head_size, [])),
    )
    model = MultitaskLearner(model)
    model = LineVectorizer(model)
    model.load_state_dict(checkpoint["model_state_dict"])
    model = model.to(device)
    model.eval()

    print(f"Processing {imname}")
    im = skimage.io.imread(imname)
    if im.ndim == 2:
        im = np.repeat(im[:, :, None], 3, 2)
    im = im[:, :, :3]
    im_resized = skimage.transform.resize(im, (512, 512)) * 255
    image = (im_resized - M.image.mean) / M.image.stddev
    image = torch.from_numpy(np.rollaxis(image, 2)[None].copy()).float()
    with torch.no_grad():
        input_dict = {
            "image": image.to(device),
            "meta": [
                {
                    "junc": torch.zeros(1, 2).to(device),
                    "jtyp": torch.zeros(1, dtype=torch.uint8).to(device),
                    "Lpos": torch.zeros(2, 2, dtype=torch.uint8).to(device),
                    "Lneg": torch.zeros(2, 2, dtype=torch.uint8).to(device),
                }
            ],
            "target": {
                "jmap": torch.zeros([1, 1, 128, 128]).to(device),
                "joff": torch.zeros([1, 1, 2, 128, 128]).to(device),
            },
            "mode": "testing",
        }
        H = model(input_dict)["preds"]

    lines = H["lines"][0].cpu().numpy() / 128 * im.shape[:2]
    scores = H["score"][0].cpu().numpy()
    for i in range(1, len(lines)):
        if (lines[i] == lines[0]).all():
            lines = lines[:i]
            scores = scores[:i]
            break

    # postprocess lines to remove overlapped lines
    diag = (im.shape[0] ** 2 + im.shape[1] ** 2) ** 0.5
    nlines, nscores = postprocess(lines, scores, diag * 0.01, 0, False)

    flines = []
    fscores = []
    for (a, b), s in zip(nlines, nscores):
        if s < 0.9:
            continue
        flines.append(((a[1], a[0]), (b[1], b[0])))
        fscores.append(nscores)
    
    # 调用分割函数
    output_directory = "temp_output"
    segment_image_by_extended_lines(imname, flines, output_directory, min_ratio=5e-4)


def target_range(image_path, points):
    print("range start!")
    image = Image.open(image_path).convert("RGBA")
    print(image_path)
    
    img_width, img_height = image.size
    pixel_points = [(int(x * img_width), int(y * img_height)) for x, y in points]
    # 计算目标矩形的最小和最大坐标
    min_x = min(x for x, y in pixel_points)
    min_y = min(y for x, y in pixel_points)
    max_x = max(x for x, y in pixel_points)
    max_y = max(y for x, y in pixel_points)
    # 计算裁剪区域的宽和高
    target_width = max_x - min_x
    target_height = max_y - min_y
    # 创建一个透明的图像，尺寸为最小矩形
    transparent_image = Image.new("RGBA", (target_width, target_height), (0, 0, 0, 0))
    # 创建一个掩码，用于保留框选区域
    mask = Image.new("L", (target_width, target_height), 0)  # 全黑的掩码
    draw = ImageDraw.Draw(mask)
    # 将框选区域的点转换为相对于最小矩形的坐标
    relative_points = [(x - min_x, y - min_y) for x, y in pixel_points]
    # 在掩码上绘制多边形，填充为白色（表示保留区域）
    draw.polygon(relative_points, fill=255)
    # 从原图中裁剪出选中区域
    selected_region = image.crop((min_x, min_y, max_x, max_y))
    # 将选中区域粘贴到透明图像上，使用掩码保留框选区域
    transparent_image.paste(selected_region, (0, 0), mask=mask)
    # 保存结果
    output_dir = "temp_output"
    os.makedirs(output_dir, exist_ok=True)
    output_path = os.path.join(output_dir, "range.png")
    print(output_path)
    transparent_image.save(output_path)
    print("range finished!")