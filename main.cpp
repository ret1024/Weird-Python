#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <cctype>
#include <iomanip>
#include <cstring>
#include <algorithm>
#include <string>
using std::cout;
using std::cerr;
using std::endl;
using std::string;
using std::ifstream;
using std::ofstream;
using std::istream;
using std::ostream;
using std::istringstream;
using std::vector;
using std::cin;

// 声明函数
std::vector<std::string> split(std::string& s, char delimiter, bool skipEmpty);
string format(const string& data);
bool check(string code);
string convertDeclarations(const string& code);
string convertLine(const string& line);
string convertFunction(const string& line);
string trim(const string& s);
string convertFunction(const string& line);
bool isBasicType(const string& word);

bool addMain = false;
//---------------------
string convertFunction(const string& line) {
    string trimmed = trim(line);
    if (trimmed.empty()) return line;

    size_t firstSpace = trimmed.find_first_of(" \t");
    if (firstSpace == string::npos) return line;
    string typeWord = trimmed.substr(0, firstSpace);
    if (!isBasicType(typeWord)) return line;

    size_t parenOpen = trimmed.find('(');
    if (parenOpen == string::npos) return line;

    string beforeParen = trim(trimmed.substr(0, parenOpen));
    size_t lastSpace = beforeParen.find_last_of(" \t");
    if (lastSpace == string::npos) return line;
    string returnType = trim(beforeParen.substr(0, lastSpace));
    string funcName = trim(beforeParen.substr(lastSpace + 1));
    if (funcName.empty() || returnType.empty()) return line;

    size_t parenClose = trimmed.find_last_of(')');
    if (parenClose == string::npos || parenClose < parenOpen) return line;
    string params = trimmed.substr(parenOpen + 1, parenClose - parenOpen - 1);
    // 如果参数是 "void"，（Python 中none）
    if (trim(params) == "void") params = "None";

    string restAfterClose = trim(trimmed.substr(parenClose + 1));
    bool hasBrace = (restAfterClose.find('{') != string::npos);
    bool hasSemicolon = (restAfterClose.find(';') != string::npos);

    string result = "def " + funcName + "(" + params + ")";
    if (returnType != "void") {
        result += " -> " + returnType;
    } else {
        result += " -> None";
    }
    if (hasBrace) {
        result += " {";
    } else if (hasSemicolon) {
        result += ";";   // 函数原型保留分号
    }

    // 保留原行首缩进（如果有）
    size_t firstNonSpace = line.find_first_not_of(" \t");
    string prefix = (firstNonSpace == string::npos) ? "" : line.substr(0, firstNonSpace);
    return prefix + result;
}
// 辅助：去除字符串首尾空格
string trim(const string& s) {
    size_t start = s.find_first_not_of(" \t");
    if (start == string::npos) return "";
    size_t end = s.find_last_not_of(" \t");
    return s.substr(start, end - start + 1);
}

// 判断一个字符串是否为C基本类型（可扩展）
bool isBasicType(const string& word) {
    static const char* types[] = {"int", "float", "char", "bool", "void", "dict", "list", "str", "string", };
    for (size_t i = 0; i < sizeof(types)/sizeof(types[0]); ++i) {
        if (word == types[i]) return true;
    }
    return false;
}

string convertLine(const string& line) {
    string trimmed = trim(line);
    if (trimmed.empty()) return line;

    size_t firstSpace = trimmed.find_first_of(" \t");
    if (firstSpace == string::npos) return line;
    string typeWord = trimmed.substr(0, firstSpace);
    if (!isBasicType(typeWord)) return line;

    string rest = trim(trimmed.substr(firstSpace + 1));
    if (rest.empty()) return line;

    size_t semicolonPos = rest.find(';');
    if (semicolonPos == string::npos) return line;

    string varList = rest.substr(0, semicolonPos);
    string varListCopy = varList;
    vector<string> items = split(varListCopy, ',', true);

    if (items.empty()) return line;

    string result;
    for (size_t i = 0; i < items.size(); ++i) {
        string item = trim(items[i]);
        if (item.empty()) continue;

        string varName, initValue;
        size_t eqPos = item.find('=');
        if (eqPos != string::npos) {
            varName = trim(item.substr(0, eqPos));
            initValue = trim(item.substr(eqPos + 1));
        } else {
            varName = trim(item);
            // 根据类型设置默认值
            if (typeWord == "int" || typeWord == "float" || typeWord == "double" ||
                typeWord == "long" || typeWord == "short") {
                initValue = "0";
            } else if (typeWord == "char" || typeWord == "string") {
                typeWord = "str";
                initValue = "''";
            } else if (typeWord == "bool") {
                initValue = "false";
            } else if (typeWord == "dict") {
                initValue = "__LB____RB__";   // 空字典占位
            } else if (typeWord == "list") {
                initValue = "[]";   // 空列表占位（可改用 []，但需确保 [] 不被干扰）
            } else {
                initValue = "0";
            }
        }

        // 如果变量是 dict 或 list，且初始化值中包含花括号，进行替换
        if (typeWord == "dict" || typeWord == "list") {
            // 替换 { 和 } 为占位符
            size_t pos = 0;
            while ((pos = initValue.find('{', pos)) != string::npos) {
                initValue.replace(pos, 1, "__LB__");
                pos += 6; // 跳过占位符长度
            }
            pos = 0;
            while ((pos = initValue.find('}', pos)) != string::npos) {
                initValue.replace(pos, 1, "__RB__");
                pos += 6;
            }
        }

        varName = trim(varName);
        result += varName + ": " + typeWord + " = " + initValue + ";";
    }
    return result;
}

// 对整个代码进行声明转换（保留换行）
string convertDeclarations(const string& code) {
    istringstream stream(code);
    string line, result;
    while (getline(stream, line)) {
        string newLine = convertFunction(line);
        if (newLine == line) {   // 未被函数转换
            newLine = convertLine(line);
        }
        result += newLine;
        result.push_back('\n');
    }
    return result;
}
//----------------------
size_t countChar(const std::string& str, char ch) {
    size_t count = 0;
    for (size_t i = 0; i < str.size(); ++i) {
        if (str[i] == ch) ++count;
    }
    return count;
}

vector<string> split(string& s,
                               char delimiter,
                               bool skipEmpty = true) {
    vector<std::string> tokens;
    istringstream stream(s);
    string token;
    while (getline(stream, token, delimiter)) {
        if (!skipEmpty || !token.empty()) {
            tokens.push_back(token);
        }
    }
    return tokens;
}

string format(const string& data) {
    bool isinstring = false;
    string buffer;
    string line;
    unsigned int depth = 1;
    for (char ch : data) {
        if (ch == '\n') continue;  // 忽略原有换行
        buffer.push_back(ch);
        if (ch == '"' || ch == '\'') {
            isinstring = !isinstring;
            continue;
        }
        if (isinstring) continue;  // 字符串内不处理
        switch (ch) {
            case '{':
                buffer.pop_back();
                buffer.push_back(':');
                buffer.push_back('\n');
                buffer.append(depth, '\t');
                ++depth;
                break;
            case '}':
                --depth;
                buffer.pop_back();
                buffer.push_back('\n');
                break;
            case ';':
                buffer.pop_back();
                buffer.push_back('\n');
                buffer.append(depth - 1, '\t');   // 添加缩进
                break;
            default:
                break;
        }
    }
    // 还原字典/列表字面量的花括号
    string str = buffer;
    size_t pos = 0;
    while ((pos = str.find("__LB__", pos)) != string::npos) {
        str.replace(pos, 6, "{");
        pos += 1;
    }
    pos = 0;
    while ((pos = str.find("__RB__", pos)) != string::npos) {
        str.replace(pos, 6, "}");
        pos += 1;
    }
    return str;
}
bool is_empty_line(const std::string& line) {
    for (char ch : line) {
        if (ch != ' ' && ch != '\t') {
            return false;
        }
    }
    return true;
}
bool check(string code) {
    if (countChar(code, '{') != countChar(code, '}')) {
        cout << "There is an unclosed structure" << endl;
        return false;
    }
    std::vector<std::string> result = split(code, '\n');
    for (size_t i = 0; i < result.size(); ++i) {
        // 如果一行都是空格或制表符，则跳过检查
        if (is_empty_line(result[i])) continue;
        char c = result[i][result[i].size() - 1];
        if (c != ';' && c != '}') {
            if(c == '{') continue;
            cout << "Conversion failed at line " << i << ": Missing ';'" << endl;
            return false;
        }
    }
    return true;
}

int main(int argc, char* argv[]) {
    if (argc == 1) {
        cerr << "no input file specified." << endl;
        cerr << "Usage: " << argv[0] << " [input_file] [output_file]" << endl;
        return 0;   // 无参数直接退出
    }
    // 解析命令行
    bool runFromMain = false;
    std::vector<std::string> fileArgs;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--run-from-main") {
            runFromMain = true;
        } else if (arg.find("--") == 0) {
            // 未知选项，报错退出（不改变原有提示风格）
            cerr << "Unknown option: " << arg << endl;
            cerr << "Usage: " << argv[0] << " [input_file] [output_file]" << endl;
            return 1;
        } else {
            fileArgs.push_back(arg);
        }
    }

    if (fileArgs.size() > 2) {
        cerr << "Too many arguments." << endl;
        cerr << "Usage: " << argv[0] << " [input_file] [output_file]" << endl;
        return 1;
    }

    // 确定输入输出流
    ifstream fin;
    ofstream fout;
    istream* in = &cin;
    ostream* out = &cout;

    if (fileArgs.size() >= 1) {
        fin.open(fileArgs[0].c_str());
        if (!fin.is_open()) {
            cerr << "Error: Cannot open input file '" << fileArgs[0] << "'" << endl;
            return 1;
        }
        in = &fin;
    }

    if (fileArgs.size() >= 2) {
        fout.open(fileArgs[1].c_str());
        if (!fout.is_open()) {
            cerr << "Error: Cannot open output file '" << fileArgs[1] << "'" << endl;
            return 1;
        }
        out = &fout;
    }

    // 读取所有输入
    string str, line;
    while (getline(*in, line)) {
        str += line;
        str.push_back('\n');
    }

    // 处理流程（原有逻辑不变）
    //将四个空格替换为一个制表符
    size_t pos = 0;
    while ((pos = str.find("    ")) != std::string::npos) {
        str.replace(pos, 4, "\t");
    }
    str.erase(remove(str.begin(), str.end(), '\t'), str.end());
    if (!check(str)) return 1;
    str = convertDeclarations(str);
    str.erase(remove(str.begin(), str.end(), '\n'), str.end());
    str = format(str);

    // 如果指定了 --run-from-main，追加两行
    if (runFromMain) {
        str += "\nif __name__ == '__main__':\n\tmain()";
    }

    // 输出结果
    *out << str << endl;


    if (fin.is_open()) fin.close();
    return 0;
}