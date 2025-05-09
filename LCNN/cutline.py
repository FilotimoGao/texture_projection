import os
import cv2
import numpy as np
from PIL import Image, ImageDraw
from scipy.ndimage import label, gaussian_filter, binary_dilation, binary_erosion
from skimage.filters import sobel
from skimage.measure import find_contours
import shutil

def rgba_to_gray(region_pixels):
    """
    将 RGBA 图像转换为灰度图像。
    参数:
        region_pixels: RGBA 图像的 NumPy 数组，形状为 (height, width, 4)
    返回:
        gray_image: 灰度图像的 NumPy 数组，形状为 (height, width)
    """
    # 确保输入是一个有效的 RGBA 图像
    if region_pixels.ndim != 3 or region_pixels.shape[2] != 4:
        raise ValueError("输入图像必须是 RGBA 格式，形状为 (height, width, 4)")

    # 使用加权平均法计算灰度值
    gray_image = 0.299 * region_pixels[:, :, 0] + 0.587 * region_pixels[:, :, 1] + 0.114 * region_pixels[:, :, 2]

    # 确保灰度图像的数据类型是整数
    gray_image = gray_image.astype(np.uint8)

    return gray_image


def extend_line(x1, y1, x2, y2, width, height):
    """延长线段到整个图片边界"""
    if x1 == x2:  # 垂直线
        return (x1, 0), (x1, height - 1)
    if y1 == y2:  # 水平线
        return (0, y1), (width - 1, y1)

    # 计算斜率
    slope = (y2 - y1) / (x2 - x1)
    # 计算延长线的起点和终点
    x_start = 0
    y_start = int(slope * (x_start - x1) + y1)
    if y_start < 0:
        y_start = 0
        x_start = int((y_start - y1) / slope + x1)

    x_end = width - 1
    y_end = int(slope * (x_end - x1) + y1)
    if y_end >= height:
        y_end = height - 1
        x_end = int((y_end - y1) / slope + x1)

    return (x_start, y_start), (x_end, y_end)


def is_line_too_vertical(start, end, threshold=5):
    """判断线段是否过于竖直（即斜率的绝对值太大）"""
    dx = end[0] - start[0]
    dy = end[1] - start[1]
    if dx == 0:  # 垂直线
        return True
    slope = dy / dx
    return abs(slope) > threshold  # 如果斜率的绝对值大于阈值，则认为过于竖直


def draw_lines(image, lines, extended_lines, output_path):
    """绘制原始线段和延长线段到图像上"""
    draw = ImageDraw.Draw(image)
    
    # 绘制原始线段（红色）
    #for (start, end) in lines:
    #    draw.line([start, end], fill="red", width=2)

    # 绘制延长线段（蓝色）
    for (start, end) in extended_lines:
        draw.line([start, end], fill="blue", width=2)

    # 保存绘制后的图像
    image.save(output_path)


def calculate_glcm(image, distance=1, angle=0):
    """计算灰度共生矩阵（GLCM）"""
    h, w = image.shape
    max_gray = 256  # 灰度级别的数量（假设为256）
    
    # 初始化 GLCM
    glcm = np.zeros((max_gray, max_gray), dtype=np.int32)

    # 计算 GLCM
    for i in range(h):
        for j in range(w):
            if angle == 0:  # 水平
                if j + distance < w:
                    glcm[image[i, j], image[i, j + distance]] += 1
            elif angle == 90:  # 垂直
                if i + distance < h:
                    glcm[image[i, j], image[i + distance, j]] += 1
            elif angle == 45:  # 对角线
                if i + distance < h and j + distance < w:
                    glcm[image[i, j], image[i + distance, j + distance]] += 1
            elif angle == 135:  # 反对角线
                if i + distance < h and j - distance >= 0:
                    glcm[image[i, j], image[i + distance, j - distance]] += 1

    return glcm

def detect_text_in_fragment(image_fragment):
    """判断图像碎片中是否存在文字"""
    # 转换为灰度图（假设输入是 RGB 图像）
    if len(image_fragment.shape) == 3:
        gray = np.mean(image_fragment, axis=2).astype(np.uint8)
    else:
        gray = image_fragment

    # 边缘检测（使用 Sobel 算子）
    edges = sobel(gray)

    # 二值化边缘
    edges_binary = (edges > 0.1).astype(np.uint8)

    # 形态学操作（膨胀，增强文字连通性）
    kernel = np.ones((3, 3), dtype=np.uint8)
    dilated_edges = binary_dilation(edges_binary, structure=kernel)

    # 查找轮廓
    contours = find_contours(dilated_edges, level=0.5)

    # 分析轮廓特征
    text_like_contours = 0
    for contour in contours:
        # 计算轮廓面积
        area = contour.shape[0]
        # 计算轮廓的边界矩形
        x_min, y_min = np.min(contour, axis=0)
        x_max, y_max = np.max(contour, axis=0)
        w = x_max - x_min
        h = y_max - y_min
        # 计算长宽比
        aspect_ratio = float(w) / h if h != 0 else 0
        # 计算紧凑度（面积与边界矩形面积的比值）
        compactness = area / (w * h) if w * h != 0 else 0
        # 判断是否为文字轮廓
        if area < 100 and 0.2 < aspect_ratio < 5 and compactness > 0.2:
            text_like_contours += 1

    # 判断是否存在文字
    print("text_like_contours: ", text_like_contours)
    return text_like_contours > 0  # 设定阈值

def calculate_entropy(glcm):
    """计算 GLCM 的熵"""
    glcm_normalized = glcm / np.sum(glcm)  # 归一化
    entropy = -np.sum(glcm_normalized * np.log2(glcm_normalized + 1e-10))  # 添加小数以避免 log(0)
    return entropy


def clear_output_directory(output_dir):
    """清空输出目录中的文件"""
    if os.path.exists(output_dir):
        shutil.rmtree(output_dir)  # 删除该目录及其内容
    os.makedirs(output_dir)  # 重新创建目录


def segment_image_by_extended_lines(image_path, lines, output_dir, min_ratio=10e-4, slope_threshold=5):
    """根据延长线段分割图片并保存为透明背景的部分"""
    img = Image.open(image_path).convert("RGBA")
    width, height = img.size
    min_size = min_ratio * width * height
    img_data = np.array(img)
    clear_output_directory(output_dir)  # 清空输出文件夹

    # 创建一张蒙版图像
    mask_img = Image.new("L", (width, height), 0)  # 黑白图像
    draw = ImageDraw.Draw(mask_img)

    extended_lines = []
    # 生成延长线段并绘制
    for (start, end) in lines:
        if is_line_too_vertical(start, end, slope_threshold):
            print(f"线段 {start} 到 {end} 被过滤掉，因为它过于竖直。")
            continue  # 如果线段过于竖直则跳过
        
        extended_start, extended_end = extend_line(start[0], start[1], end[0], end[1], width, height)
        extended_lines.append((extended_start, extended_end))
        draw.line([extended_start, extended_end], fill=255, width=3)

    mask_data = np.array(mask_img)

    # 连通区域检测
    labeled, num_features = label(mask_data == 0)

    # 提取和保存每个区域
    os.makedirs(output_dir, exist_ok=True)
    regions = []  # 存储区域信息，包含最小 y 坐标和区域标签

    for region_label in range(1, num_features + 1):
        region_mask = labeled == region_label
        y_indices, x_indices = np.where(region_mask)
        
        if len(x_indices) == 0 or len(y_indices) == 0:
            continue
        
        x_min, x_max = x_indices.min(), x_indices.max()
        y_min, y_max = y_indices.min(), y_indices.max()

        # 计算区域的面积
        area = (x_max - x_min + 1) * (y_max - y_min + 1)

        # 过滤小区域
        if area < min_size:
            continue

        # 将区域信息（包括 y_min 和 y_max）存储到 regions 列表中
        regions.append((y_min, y_max, region_label, x_min, x_max))

    # 根据 y_min 和 y_max 的平均值排序
    regions.sort(key=lambda r: (r[0] + r[1]) / 2, reverse=True)  # r[0] 是 y_min, r[1] 是 y_max

    # 使用排序后的索引重新命名区域
    for new_index, (y_min, y_max, region_label, x_min, x_max) in enumerate(regions, start=1):
        # 创建与原图同样大小的透明背景图像
        full_image = Image.new("RGBA", (width, height), (0, 0, 0, 0))
        
        # 将裁剪区域复制到全图的正确位置
        region_pixels = img_data[y_min:y_max + 1, x_min:x_max + 1]
        region_mask_cropped = labeled[y_min:y_max + 1, x_min:x_max + 1] == region_label
        
        # 确保原图的裁剪区域放置在全图的正确位置
        for i in range(y_min, y_max + 1):
            for j in range(x_min, x_max + 1):
                if region_mask_cropped[i - y_min, j - x_min]:
                    full_image.putpixel((j, i), tuple(region_pixels[i - y_min, j - x_min]))

        # 计算该区域的熵
        gray_image = rgba_to_gray(region_pixels)
        glcm = calculate_glcm(gray_image)  # 计算 GLCM
        entropy = calculate_entropy(glcm)  # 计算熵

        print(f"区域 {new_index} 的熵 {entropy:.2f}")
        if entropy < 1.0: # 5.0
            print(f"区域 {new_index} 的熵 {entropy:.2f} 小于 5.0，已被过滤。")
            continue  # 如果熵小于5.0，则跳过该区域
        '''
        # 判断是否存在文字
        if detect_text_in_fragment(region_pixels):
            print("图像碎片中存在文字")
        else:
            print("图像碎片中不存在文字")
        '''

        # 保存图片，文件名使用 y_min 和 y_max 的平均值排序的顺序
        full_image.save(os.path.join(output_dir, f"{new_index}.png"))

    print(f"分割完成，共生成 {len(regions)} 个区域，并保存到 '{output_dir}' 文件夹中。")


def segment(image_path, lines):
    """根据延长线段分割图片并保存为透明背景的部分"""
    output_dir = "temp_output"
    img = Image.open(image_path).convert("RGBA")
    width, height = img.size
    img_data = np.array(img)
    clear_output_directory(output_dir)  # 清空输出文件夹

    # 创建一张蒙版图像
    mask_img = Image.new("L", (width, height), 0)  # 黑白图像
    draw = ImageDraw.Draw(mask_img)

    extended_lines = []
    # 生成延长线段并绘制
    for (start, end) in lines:
        extended_start, extended_end = extend_line(start[0], start[1], end[0], end[1], width, height)
        extended_lines.append((extended_start, extended_end))
        draw.line([extended_start, extended_end], fill=255, width=3)

    mask_data = np.array(mask_img)

    # 连通区域检测
    labeled, num_features = label(mask_data == 0)

    # 提取和保存每个区域
    os.makedirs(output_dir, exist_ok=True)
    regions = []  # 存储区域信息，包含最小 y 坐标和区域标签

    for region_label in range(1, num_features + 1):
        region_mask = labeled == region_label
        y_indices, x_indices = np.where(region_mask)
        
        if len(x_indices) == 0 or len(y_indices) == 0:
            continue
        
        x_min, x_max = x_indices.min(), x_indices.max()
        y_min, y_max = y_indices.min(), y_indices.max()

        # 将区域信息（包括 y_min 和 y_max）存储到 regions 列表中
        regions.append((y_min, y_max, region_label, x_min, x_max))

    # 根据 y_min 和 y_max 的平均值排序
    regions.sort(key=lambda r: (r[0] + r[1]) / 2, reverse=True)  # r[0] 是 y_min, r[1] 是 y_max

    # 使用排序后的索引重新命名区域
    for new_index, (y_min, y_max, region_label, x_min, x_max) in enumerate(regions, start=1):
        # 创建与原图同样大小的透明背景图像
        full_image = Image.new("RGBA", (width, height), (0, 0, 0, 0))
        
        # 将裁剪区域复制到全图的正确位置
        region_pixels = img_data[y_min:y_max + 1, x_min:x_max + 1]
        region_mask_cropped = labeled[y_min:y_max + 1, x_min:x_max + 1] == region_label
        
        # 确保原图的裁剪区域放置在全图的正确位置
        for i in range(y_min, y_max + 1):
            for j in range(x_min, x_max + 1):
                if region_mask_cropped[i - y_min, j - x_min]:
                    full_image.putpixel((j, i), tuple(region_pixels[i - y_min, j - x_min]))

        # 保存图片，文件名使用 y_min 和 y_max 的平均值排序的顺序
        full_image.save(os.path.join(output_dir, f"{new_index}.png"))

    print(f"分割完成，共生成 {len(regions)} 个区域，并保存到 '{output_dir}' 文件夹中。")
