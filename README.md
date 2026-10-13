
# epubmaker - 极简 EPUB 打包工具 使用说明

## 一、是什么
epubmaker 是一个命令行工具，读取项目目录里的 epub.ini 配置文件和若干 .html
章节文件，自动打包成标准 EPUB 3.0 电子书。

特点：
  - 纯 INI 驱动，无交互输入
  - list = auto 自动扫描章节，或手动指定顺序
  - 零运行时依赖（只需 zip + C++17 编译器）
  - 生成到 ~/epub_out/书名.epub

## 二、环境要求
  必须：
    - C++17 编译器（clang++ 或 g++）
    - zip 命令

  Termux 安装：
    pkg update
    pkg install clang zip

  Debian / Ubuntu：
    sudo apt update
    sudo apt install build-essential zip

  macOS：
    brew install zip
    （clang++ 随 Xcode Command Line Tools 自带）

## 三、安装（从源码编译）
  1. 把 epubmaker.cpp 和 init_epub.sh 放到同一目录
  2. 编译：
     clang++ -std=c++17 -O2 -Wall -o epubmaker epubmaker.cpp
  3. 装到 PATH（可选）：
     cp epubmaker $PREFIX/bin/
     cp init_epub.sh $PREFIX/bin/

  验证：
     epubmaker --help

================================================================================
## 四、最快上手（3 步出书）
  第一步：初始化项目
    bash init_epub.sh mybook "我的书" "作者" "zh"

  第二步：编辑章节
    nano mybook/01-地理与面积.html
    写入正文（纯 HTML 片段，见第六节）

  第三步：打包
    epubmaker ./mybook

  输出位置：
    ~/epub_out/我的书.epub

================================================================================
## 五、epub.ini 配置详解
--------------------------------------------------------------------------------
  epub.ini 必须放在项目根目录。格式：

  ----------------------------------------------------------------------------
  [book]
  title = 书名（必填）
  author = 作者（可选，默认 Anonymous）
  lang = 语言代码（可选，默认 zh）

  [chapters]
  list = auto
  ----------------------------------------------------------------------------

 ## 5.1 list = auto（自动扫描模式）
  ----------------------------------------------------------------------------
    扫描项目目录下所有 .html 文件，按文件名排序作为章节顺序。

    排序规则：字典序。为确保正确顺序，文件名必须以两位数字开头：
      01-xxx.html
      02-xxx.html
      ...
      09-xxx.html
      10-xxx.html

    章节标题自动从文件名提取：
      文件名 "01-地理与面积.html" → 章节标题 "地理与面积"
      规则：去 .html 后缀 → 去开头数字和连字符/下划线 → 剩余部分

## 5.2 list = 手动列表（手动指定模式）
    list = 01.html, 03.html, 02.html

    严格按你写的顺序打包。逗号后可有空格。适合跳章、重排、只选部分文件。

    如果列出的文件不存在，epubmaker 报错并拒绝运行。

## 六、章节 HTML 文件格式
  每个 .html 文件是一章的正文，写纯 HTML 片段即可。

  正确写法：
    <p>这是第一段。</p>
    <p>这是第二段。</p>
    <h2>小节标题</h2>
    <p>更多内容。</p>
    <img src="image.jpg" alt="图片说明"/>

  错误写法（不要写这些）：
    <!DOCTYPE html>
    <html>
    <head><title>xx</title></head>
    <body>...</body>
    </html>

  epubmaker 会自动把你的内容包装进完整 XHTML 模板，包括 <html>、<head>、
  <body> 标签和 EPUB 3.0 声明。

  支持的标签：
    <p> <h1>~<h6> <img> <a> <em> <strong> <code> <pre>
    <ul> <ol> <li> <br> <hr> <table>

## 七、style.css 样式文件
  可选。放在项目目录。不提供则使用内置默认样式。

  默认样式：
    body { font-family: Georgia, serif; line-height: 1.8; margin: 1.5em; }
    h1 { color: #334; font-size: 1.4em; }
    p { margin: .8em 0; }
    code { background: #eee; padding: 2px 6px; border-radius: 6px; }
    img { max-width: 100%; border-radius: 8px; }

  自定义示例：
    body { font-family: "Noto Serif", serif; line-height: 2; }
    h1 { color: #8b0000; border-bottom: 2px solid #8b0000; }
    p { text-indent: 2em; }

## 八、项目目录结构
  最小项目：
    myproject/
    ├── epub.ini
    ├── 01-序.html
    └── 02-正文.html

  完整项目：
    myproject/
    ├── epub.ini
    ├── style.css
    ├── 01-地理与面积.html
    ├── 02-历史沿革.html
    ├── 03-人口与语言.html
    ├── images/
    │   ├── map.jpg
    │   └── flag.png
    └── cover.jpg

  参数默认值：
    项目目录 = macau
    书名 = 澳门简介
    作者 = Anonymous
    语言 = zh

  生成内容：
    epub.ini + style.css + 8 个示例章节 .html
