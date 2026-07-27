#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <map>
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
string trim(const string& s);
bool isBasicType(const string& word);
string normalizeType(const string& type);
string defaultValueForType(const string& type);
string protectBraces(const string& value);

bool addMain = false;
//---------------------
string trim(const string& s) {
    size_t start = s.find_first_not_of(" \t");
    if (start == string::npos) return "";
    size_t end = s.find_last_not_of(" \t");
    return s.substr(start, end - start + 1);
}

bool isBasicType(const string& word) {
    static const char* types[] = {"int", "float", "char", "bool", "void", "dict", "list", "str", "string"};
    for (size_t i = 0; i < sizeof(types)/sizeof(types[0]); ++i) {
        if (word == types[i]) return true;
    }
    return false;
}

string normalizeType(const string& type) {
    if (type == "char" || type == "string") {
        return "str";
    }
    return type;
}

string defaultValueForType(const string& type) {
    static const std::map<string, string> defaultValues = {
        {"int", "0"},
        {"float", "0"},
        {"str", "''"},
        {"bool", "false"},
        {"dict", "__LB____RB__"},
        {"list", "[]"}
    };

    auto it = defaultValues.find(type);
    if (it != defaultValues.end()) {
        return it->second;
    }
    return "0";
}

string protectBraces(const string& value) {
    string result = value;
    size_t pos = 0;
    while ((pos = result.find('{', pos)) != string::npos) {
        result.replace(pos, 1, "__LB__");
        pos += 6;
    }
    pos = 0;
    while ((pos = result.find('}', pos)) != string::npos) {
        result.replace(pos, 1, "__RB__");
        pos += 6;
    }
    return result;
}

struct LineProcessor {
    string original;
    string trimmed;
    string indent;
    string typeName;
    bool isFunction = false;
    bool isVariable = false;

    explicit LineProcessor(const string& line) : original(line) {
        parse();
    }

    void parse() {
        size_t firstNonSpace = original.find_first_not_of(" \t");
        indent = (firstNonSpace == string::npos) ? string() : original.substr(0, firstNonSpace);
        trimmed = trim(original);
        if (trimmed.empty()) return;

        istringstream stream(trimmed);
        vector<string> tokens;
        string token;
        while (stream >> token) {
            tokens.push_back(token);
        }
        if (tokens.empty()) return;

        typeName = tokens[0];
        if (!isBasicType(typeName)) return;

        if (trimmed.find('(') != string::npos && trimmed.find(')') != string::npos) {
            isFunction = true;
        } else if (trimmed.find(';') != string::npos) {
            isVariable = true;
        }
    }

    string convert() const {
        if (isFunction) {
            return convertFunction();
        }
        if (isVariable) {
            return convertVariable();
        }
        return original;
    }

private:
    string convertFunction() const {
        size_t openPos = trimmed.find('(');
        size_t closePos = trimmed.rfind(')');
        if (openPos == string::npos || closePos == string::npos || closePos < openPos) {
            return original;
        }

        string signature = trim(trimmed.substr(0, openPos));
        size_t lastSpace = signature.find_last_of(" \t");
        if (lastSpace == string::npos) {
            return original;
        }

        string returnType = normalizeType(trim(signature.substr(0, lastSpace)));
        string funcName = trim(signature.substr(lastSpace + 1));
        if (returnType.empty() || funcName.empty()) {
            return original;
        }

        string paramsStr = trim(trimmed.substr(openPos + 1, closePos - openPos - 1));
        string annotatedParams;
        if (!paramsStr.empty() && paramsStr != "void") {
            string paramsCopy = paramsStr;
            vector<string> paramItems = split(paramsCopy, ',', true);
            for (size_t i = 0; i < paramItems.size(); ++i) {
                string item = trim(paramItems[i]);
                if (item.empty()) continue;

                size_t eqPos = item.find('=');
                string typeAndName = (eqPos != string::npos) ? trim(item.substr(0, eqPos)) : item;
                string defaultValue = (eqPos != string::npos) ? trim(item.substr(eqPos + 1)) : string();

                size_t spacePos = typeAndName.find_last_of(" \t");
                if (spacePos == string::npos) continue;

                string paramType = normalizeType(trim(typeAndName.substr(0, spacePos)));
                string paramName = trim(typeAndName.substr(spacePos + 1));
                if (paramType.empty() || paramName.empty()) continue;

                if (!annotatedParams.empty()) {
                    annotatedParams += ", ";
                }
                annotatedParams += paramName + ": " + paramType;
                if (!defaultValue.empty()) {
                    annotatedParams += " = " + defaultValue;
                }
            }
        }

        string result = "def " + funcName + "(" + annotatedParams + ")";
        if (returnType != "void") {
            result += " -> " + returnType;
        } else {
            result += " -> None";
        }

        string suffix = trim(trimmed.substr(closePos + 1));
        if (suffix.find('{') != string::npos) {
            result += " {";
        } else if (suffix.find(';') != string::npos) {
            result += ";";
        }

        return indent + result;
    }

    string convertVariable() const {
        string normalizedType = normalizeType(typeName);
        size_t typeEnd = trimmed.find_first_not_of(" \t", typeName.size());
        if (typeEnd == string::npos) {
            return original;
        }

        string remainder = trim(trimmed.substr(typeEnd));
        size_t semicolonPos = remainder.rfind(';');
        if (semicolonPos == string::npos) {
            return original;
        }

        string varList = trim(remainder.substr(0, semicolonPos));
        vector<string> items = split(varList, ',', true);
        if (items.empty()) {
            return original;
        }

        string result;
        for (size_t i = 0; i < items.size(); ++i) {
            string item = trim(items[i]);
            if (item.empty()) continue;

            string varName;
            string initValue;
            size_t eqPos = item.find('=');
            if (eqPos != string::npos) {
                varName = trim(item.substr(0, eqPos));
                initValue = trim(item.substr(eqPos + 1));
            } else {
                varName = trim(item);
                initValue = defaultValueForType(normalizedType);
            }

            if (normalizedType == "dict" || normalizedType == "list") {
                initValue = protectBraces(initValue);
            }

            if (!result.empty()) {
                result += " ";
            }
            result += varName + ": " + normalizedType + " = " + initValue + ";";
        }

        return trim(indent + result);
    }
};

string convertLine(const string& line) {
    LineProcessor processor(line);
    return processor.convert();
}

// 对整个代码进行声明转换（保留换行）
string convertDeclarations(const string& code) {
    istringstream stream(code);
    string line, result;
    while (getline(stream, line)) {
        result += convertLine(line);
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
    string token;
    bool inDoubleQuotes = false;
    bool inSingleQuotes = false;
    int braceDepth = 0;
    for (size_t i = 0; i < s.size(); ++i) {
        char ch = s[i];
        if (ch == '"' && !inSingleQuotes) {
            inDoubleQuotes = !inDoubleQuotes;
            token.push_back(ch);
            continue;
        }
        if (ch == '\'' && !inDoubleQuotes) {
            inSingleQuotes = !inSingleQuotes;
            token.push_back(ch);
            continue;
        }

        if (!inDoubleQuotes && !inSingleQuotes) {
            if (ch == '{' || ch == '[' || ch == '(') {
                ++braceDepth;
            } else if (ch == '}' || ch == ']' || ch == ')') {
                if (braceDepth > 0) {
                    --braceDepth;
                }
            }
        }

        if (ch == delimiter && !inDoubleQuotes && !inSingleQuotes && braceDepth == 0) {
            if (!skipEmpty || !token.empty()) {
                tokens.push_back(token);
            }
            token.clear();
        } else {
            token.push_back(ch);
        }
    }
    if (!skipEmpty || !token.empty()) {
        tokens.push_back(token);
    }
    return tokens;
}

string format(const string& data) {
    bool isinstring = false;
    bool afternext = false;
    string buffer;
    string line;
    unsigned int depth = 0;
    for (char ch : data) {
        buffer.push_back(ch);
        if (ch == '"' || ch == '\'') {
            isinstring = !isinstring;
            continue;
        }
        if (isinstring) continue;  // 字符串内不处理
        switch (ch) {
            case '{':
                ++depth;
                buffer.pop_back();
                buffer.push_back(':');
                buffer.push_back('\n');
                buffer.append(depth, '\t');
                break;
            case '}':
                --depth;
                buffer.pop_back();
                buffer.append(depth, '\t');
                buffer.push_back('\n');
                break;
            case ';':
                afternext = true;
                buffer.pop_back();
                buffer.push_back('\n');
                buffer.append(depth, '\t');   // 添加缩进
                break;
            case ' ':
                if (afternext) buffer.pop_back();
                break;
            default:
                afternext = false;
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
    if (countChar(code ,'"') % 2 != 0) {
        cout << "There is an unclosed string" << endl;
        return false;
    }
    if (countChar(code, '{') != countChar(code, '}')) {
        cout << "There is an unclosed structure" << endl;
        return false;
    }
    std::vector<std::string> result = split(code, '\n');
    for (size_t i = 0; i < result.size(); ++i) {
        // 如果一行都是空格或制表符，则跳过检查
        if (is_empty_line(result[i])) continue;
        char c = result[i][result[i].size() - 1];
        if (c != ';' && c != '}' && c != '{') {
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
        str += trim(line);
        str.push_back('\n');
    }

    // 处理流程（原有逻辑不变）
    //将四个空格替换为一个制表符
    size_t pos = 0;
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