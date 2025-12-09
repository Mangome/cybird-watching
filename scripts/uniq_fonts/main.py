#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
字体字符提取工具
自动从 ui_texts.h 和 bird_config.csv 中提取所有需要的字符
用于生成 LVGL 字体时指定字符范围
"""

import os
import re
import csv
from pathlib import Path

# 固定包含的字符集（数字、字母、标点等）
fixed_chars = [
    '0123456789',
    'ABCDEFGHIJKLMNOPQRSTUVWXYZ',
    'abcdefghijklmnopqrstuvwxyz',
    ',. !@#$%^&*()_+/-:',
    '，。？！@#￥%……&*（）——+：',
]


def extract_chinese_from_ui_texts(ui_texts_path):
    """
    从 ui_texts.h 中提取所有中文字符串
    """
    words = []
    
    if not os.path.exists(ui_texts_path):
        print(f"⚠️  警告: ui_texts.h 文件不存在: {ui_texts_path}")
        return words
    
    with open(ui_texts_path, 'r', encoding='utf-8') as f:
        content = f.read()
        
        # 匹配 const char* XXX = "文本内容"; 或 constexpr const char* XXX = "文本内容";
        # 支持单行和多行字符串
        pattern = r'(?:constexpr\s+)?const\s+char\*\s+\w+\s*=\s*"([^"]+)"'
        matches = re.findall(pattern, content)
        
        for match in matches:
            # 过滤掉纯英文和数字的字符串
            if re.search(r'[\u4e00-\u9fff]', match):  # 包含中文
                words.append(match)
                print(f"  📝 从 ui_texts.h 提取: {match}")
    
    return words


def extract_bird_names_from_csv(csv_path):
    """
    从 bird_config.csv 中提取所有小鸟名称
    """
    bird_names = []
    
    if not os.path.exists(csv_path):
        print(f"⚠️  警告: bird_config.csv 文件不存在: {csv_path}")
        return bird_names
    
    with open(csv_path, 'r', encoding='utf-8') as f:
        # 跳过第一行标题，手动解析（处理可能的空格问题）
        lines = f.readlines()
        for i, line in enumerate(lines):
            if i == 0:  # 跳过标题行
                continue
            line = line.strip()
            if not line:
                continue
            
            # 分割 CSV (格式: id, name, weight)
            parts = [p.strip() for p in line.split(',')]
            if len(parts) >= 2:
                bird_id = parts[0]
                name = parts[1]
                if name:
                    bird_names.append(name)
                    print(f"  🐦 从 bird_config.csv 提取: [{bird_id}] {name}")
    
    return bird_names


def get_project_root():
    """
    获取项目根目录（从脚本所在位置向上查找）
    """
    current = Path(__file__).resolve().parent
    while current != current.parent:
        # 检查是否存在 platformio.ini 或 src 目录
        if (current / 'platformio.ini').exists() or (current / 'src').exists():
            return current
        current = current.parent
    return Path(__file__).resolve().parent.parent.parent


def main():
    print("=" * 60)
    print("🔤 字体字符提取工具")
    print("=" * 60)
    
    # 获取项目根目录
    project_root = get_project_root()
    print(f"📁 项目根目录: {project_root}\n")
    
    # 文件路径
    ui_texts_path = project_root / 'src' / 'config' / 'ui_texts.h'
    csv_path = project_root / 'resources' / 'configs' / 'bird_config.csv'
    
    print("📂 开始提取文本...\n")
    
    # 提取 UI 文本
    print("1️⃣  提取 UI 文本:")
    ui_words = extract_chinese_from_ui_texts(ui_texts_path)
    print(f"   ✅ 共提取 {len(ui_words)} 个 UI 文本\n")
    
    # 提取小鸟名称
    print("2️⃣  提取小鸟名称:")
    bird_names = extract_bird_names_from_csv(csv_path)
    print(f"   ✅ 共提取 {len(bird_names)} 个小鸟名称\n")
    
    # 合并所有词汇
    all_words = ui_words + bird_names + fixed_chars
    
    # 提取所有唯一字符并排序
    all_chars = set()
    for word in all_words:
        for char in word:
            all_chars.add(char)
    
    unique_chars = ''.join(sorted(all_chars))
    
    # 统计信息
    chinese_chars = [c for c in unique_chars if '\u4e00' <= c <= '\u9fff']
    print("=" * 60)
    print("📊 统计信息:")
    print(f"   • UI 文本数量: {len(ui_words)}")
    print(f"   • 小鸟名称数量: {len(bird_names)}")
    print(f"   • 总词汇数量: {len(all_words)}")
    print(f"   • 唯一字符总数: {len(unique_chars)}")
    print(f"   • 中文字符数量: {len(chinese_chars)}")
    print("=" * 60)
    
    # 输出结果
    print("\n✨ 生成的字符集 (用于 LVGL 字体转换):")
    print("=" * 60)
    print("⚠️  注意: 第一个空格字符也要复制！\n")
    print(unique_chars)
    print("\n" + "=" * 60)
    
    # 保存到文件
    output_file = project_root / 'scripts' / 'uniq_fonts' / 'font_chars.txt'
    with open(output_file, 'w', encoding='utf-8') as f:
        f.write(unique_chars)
    print(f"\n💾 字符集已保存到: {output_file}")
    
    # 显示示例词汇
    print("\n📝 示例词汇 (前10个):")
    for i, word in enumerate(all_words[:10], 1):
        if any('\u4e00' <= c <= '\u9fff' for c in word):
            print(f"   {i}. {word}")
    if len(all_words) > 10:
        print(f"   ... 还有 {len(all_words) - 10} 个词汇")


if __name__ == "__main__":
    main()
