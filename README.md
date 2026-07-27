# Weird-Python

> 将“类 C”的代码风格一键转换为 Python 风格 —— 包括变量类型注解、函数签名转换、花括号缩进、字典/列表字面量保留等。

[![Language](https://img.shields.io/badge/language-C%2B%2B11-blue.svg)](https://en.wikipedia.org/wiki/C%2B%2B11)
[![License](https://img.shields.io/badge/license-Apache%202.0-green.svg)](LICENSE)

---

## 📖 简介

**Weird-Python** 是一个轻量级的代码转换工具，它接收一段混合了 C 风格声明的代码（比如带有花括号、类型前缀、分号结尾的片段），然后将其“翻译”为更接近 Python 的写法：

- `int a = 5;` → `a: int = 5;`
- `int b;` → `b: int = 0;`
- `int c, d;` → `c: int = 0; d: int = 0;`
- `void func(int x) { ... }` → `def func(x) -> None: ...`
- `int add(int a, float b)` → `def add(a: int, b: float) -> int`
- `dict d = {"key": "value"};` → `d: dict = {"key": "value"};`
- 花括号 → 缩进 + 冒号

它原本是作者的一个“古怪”玩具项目（因此得名 **Weird**），但现在已经能处理很多日常转换任务，尤其适合想快速将一些 C 风格的“伪代码”或“实验性代码”转成 Python 可运行的格式。

---

## ✨ 功能特性

- ✅ **变量声明转换**  
  支持 `int`, `float`, `char`, `bool`, `void`, `dict`, `list`, `str` 等类型，自动添加 `: type` 注解，并为未初始化的变量提供合理的默认值（数字 → `0`，布尔 → `false`，字符/字符串 → `''`，字典/列表 → `{}`/`[]`）。

- ✅ **多变量拆分**  
  `int a, b = 3;` → `a: int = 0; b: int = 3;`

- ✅ **函数声明转换**  
  `int foo(int x, float y)` → `def foo(x: int, y: float) -> int`  
  `void bar()` → `def bar() -> None`（空参数或 `void` 参数列表直接转换为空括号）

- ✅ **花括号 → 冒号 + 缩进**  
  `{` 变成 `:` + 换行 + 缩进，`}` 减少缩进并换行，保留代码块层级。

- ✅ **分号处理**  
  每个 `;` 变为换行 + 当前缩进，使一行一条语句。

- ✅ **字面量保护**  
  字典/列表字面量（如 `{1,2}`）中的花括号**不会被**误当作代码块转换，原样保留。

- ✅ **智能检查**  
  转换前会检查花括号是否匹配，以及每行是否以 `;`、`}` 或 `{` 结尾，提前发现常见语法问题。

- ✅ **命令行友好**  
  支持指定输入/输出文件，无参数时给出帮助信息。

- ✅ **`--run-from-main` 开关**  
  在输出末尾自动添加 `if __name__ == '__main__':\n\tmain()`，方便直接运行。

---

## 🚀 快速开始

### 编译

项目使用纯 C++11 编写，无需额外依赖，只需一个支持 C++11 的编译器（如 g++）。

```bash
g++ -o weird-python main.cpp -std=c++11
```

### 使用方法

```bash
# 将 input.c 风格代码转换到 output.py
./weird-python input.c output.py
```

### 示例

假设 `exa.pyp` 包含：

```c
int main(){
    dict map = {"a": 91};
    print("hello");
    for i in range(91){
        print(i);
    }
}
```

运行后输出将类似：

```python
def main() -> None:
	map: dict = {"a": 91};
	print("hello");
	for i in range(91):
		print(i);
```

### 注意事项

- 目前代码转换基于行级简单解析，适合基本的 C/类 C 伪代码。复杂 C++ 语法、指针、引用、宏、模板等尚未全面支持。
- 字符串和字面量中的花括号会被保护，但更复杂的嵌套内容可能仍需手动检查。
- `--run-from-main` 选项会在输出末尾自动补充 `if __name__ == '__main__':\n\tmain()`，便于直接运行生成代码。
