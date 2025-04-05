#include <iostream>
#include <sstream>
#include <vector>
#include <string>
#include <algorithm>
#include <cctype>
#include <ctime>
#include <iomanip>
#include <windows.h>
#include <fstream>

using namespace std;
string filename = "./data/cross.txt";
// 获取剪贴板文本内容
string getClipboardText() {
    string clipboardText;
    if (OpenClipboard(NULL)) {
        HANDLE hData = GetClipboardData(CF_TEXT);
        if (hData != NULL) {
            char* pszText = static_cast<char*>(GlobalLock(hData));
            if (pszText != NULL) {
                clipboardText = pszText;
                GlobalUnlock(hData);
            }
        }
        CloseClipboard();
    }
    return clipboardText;
}

// 去除字符串前后空白字符
string trim(const string& s) {
    size_t start = s.find_first_not_of(" \t\r\n");
    if (start == string::npos) return "";
    size_t end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

// 处理输入文本，去掉 "[Console]" 和 "="
vector<string> processInputLines(const string& input) {
    vector<string> lines;
    istringstream iss(input);
    string line;

    while (getline(iss, line)) {
        string trimmed = trim(line);
        if (trimmed.empty()) continue;

        size_t pos = trimmed.find("[Console]");
        if (pos != string::npos) {
            trimmed.erase(pos, 9); // "[Console]" 长度为9
            trimmed = trim(trimmed);
        }

        // 移除 "=" 并保持两侧空格
        pos = trimmed.find('=');
        while (pos != string::npos) {
            trimmed.replace(pos, 1, " ");
            pos = trimmed.find('=', pos + 1);
        }

        trimmed = trim(trimmed);
        if (!trimmed.empty())
            lines.push_back(trimmed);
    }
    return lines;
}

// 获取当前时间戳 (YYYYMMDDHHMMSS)
string getCurrentTimestamp() {
    time_t t = time(nullptr);
    struct tm lt;
#ifdef _WIN32
    localtime_s(&lt, &t);
#else
    localtime_r(&t, &lt);
#endif
    stringstream ss;
    ss << put_time(&lt, "%Y%m%d%H%M%S");
    return ss.str();
}

void replaceLastCLInFile(const std::string& filename, const std::string& k) {
    std::fstream file(filename, std::ios::in | std::ios::out);

    // 如果文件无法打开，打印错误并返回
    if (!file) {
        std::cerr << "无法找到文件，跳过替换操作。\n";
        return;  // 只停止替换，继续执行后面的代码
    }

    std::string line;
    std::streampos lastCLPos = -1;  // 记录 "CL" 的位置
    size_t lastPosInLine = std::string::npos;

    // 遍历文件内容，找到最后一个 "CL"
    while (file) {
        std::streampos currentPos = file.tellg(); // 记录每行的起始位置
        std::getline(file, line);
        size_t pos = line.rfind("CL"); // 查找该行中最后一个 "CL"

        if (pos != std::string::npos) {
            lastCLPos = currentPos;  // 记录该行起始位置
            lastPosInLine = pos;     // 记录 "CL" 在该行的索引
        }
    }

    // 如果找到了 "CL"，进行替换操作
    if (lastCLPos != -1) {
        file.clear(); // 清除 EOF 状态
        file.seekp(lastCLPos + static_cast<std::streamoff>(lastPosInLine)); // 定位到最后一个 "CL" 的位置
        file.write(k.c_str(), k.size()); // 写入新的字符串
        file.write("\"", 1); // 写入一个双引号（"）
        std::cout << "替换成功！\n";
    }
    else {
        std::cout << "未找到 'CL'。\n";
    }

    file.close();
}

// 生成 alias 命令
string generateAliasCommands(const string& input) {
    vector<string> commands = processInputLines(input);

    // 每组4行，最多生成6组
    int groupCount = min(6, (int)((commands.size() + 3) / 4));
    // 用当前时间戳生成前缀，保证别名唯一
    string timestamp = getCurrentTimestamp();
    string prefix = "C" + timestamp;


    replaceLastCLInFile(filename,prefix);


    ostringstream oss;
    oss << "alias " << prefix << " \"" << prefix << "01\"\n";

    for (int i = 1; i <= groupCount; i++) {
        ostringstream aliasNameStream;
        aliasNameStream << prefix << setw(2) << setfill('0') << i;
        string aliasName = aliasNameStream.str();

        oss << "alias " << aliasName << " \"";
        int startIdx = (i - 1) * 4;
        for (int j = startIdx; j < startIdx + 4 && j < commands.size(); j++) {
            if (j > startIdx)
                oss << ";";
            oss << commands[j];
        }

        if (i < groupCount) {
            ostringstream nextAliasStream;
            nextAliasStream << prefix << setw(2) << setfill('0') << (i + 1);
            oss << ";" << nextAliasStream.str();
        }
        else {
            // 最后一组尾部补充，使用新的时间戳生成别名替换固定的 "CL"
            oss << ";alias CRG CL" ;
            //<< getCurrentTimestamp()
        }
        oss << "\"\n";
    }

    return oss.str();
}



int main() {
    
    string lastClipboardContent;

    while (true) {
        string clipboardContent = getClipboardText();
        if (!clipboardContent.empty() && clipboardContent != lastClipboardContent) {
            lastClipboardContent = clipboardContent;
            string result = generateAliasCommands(clipboardContent);
 
            ofstream outFile(filename, ios::app);
            if (outFile.is_open()) {
                outFile << "\n" << result << endl;
                outFile.close();
                cout << "新剪贴板内容已写入：" << endl << result << endl;
            }
            else {
                cerr << "无法打开文件进行写入！\n";
            }
        }
        Sleep(1000);
    }
    return 0;
}
