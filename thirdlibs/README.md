# 第三方库

请将以下库的源码包放入此目录：

## JPEG库
需要文件：`jpegsrc.v9b.tar.gz`
下载链接：http://www.ijg.org/files/jpegsrc.v9b.tar.gz

## tslib库
需要文件：`tslib-*.tar.gz` 或 `tslib-*.tar.bz2` 或 `tslib-*.tar.xz`
推荐版本：tslib-1.21.tar.gz 或 tslib-1.22.tar.xz
下载链接：https://github.com/libts/tslib/releases/

**注意**：构建脚本支持 `.tar.gz`, `.tar.bz2`, `.tar.xz` 三种压缩格式。

## 验证文件

准备完成后，目录结构应该如下：
```
thirdlibs/
├── README.md
├── jpegsrc.v9b.tar.gz
└── tslib-1.21.tar.gz    (或其他版本)
```

构建脚本会自动解压、编译并安装到对应的 `lib/` 目录。

## 静态链接说明

本项目静态链接JPEG和tslib库，其他库（如V4L2、framebuffer等）使用系统动态库。

- **静态链接**：编译时包含到可执行文件，无运行时依赖
- **动态链接**：运行时加载，需要目标系统有对应库文件
