import numpy as np
import matplotlib.pyplot as plt

# 从文件中读取数据
filename = 'plotdata.txt'  # 文件名，假设文件与程序在同一目录下

# 读取文件内容，每行数据通过空格分隔
data = np.loadtxt(filename)

# 创建一个足够大的图像
plt.figure(figsize=(10, len(data) * 5))  # 图像大小根据数据行数调整

# 遍历每一行数据，绘制独立的散点图
for i, row in enumerate(data):
    plt.subplot(len(data), 1, i+1)  # 将图形分成 len(data) 行 1 列
    plt.scatter(range(len(row)), row, color='b', marker='o')

    # 设置标题和标签
    plt.title(f'Scatter Plot for Line {i+1}')
    plt.xlabel('Index')
    plt.ylabel(f'Data from Line {i+1}')
    plt.grid(True)

# 保存图像为一个文件
plt.tight_layout()  # 自动调整子图间距
plt.savefig('all_scatter_plots.png', format='png')

# 显示图形
plt.show()
