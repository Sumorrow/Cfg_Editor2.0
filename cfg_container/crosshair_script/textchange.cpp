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
// ��ȡ�������ı�����
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

// ȥ���ַ���ǰ��հ��ַ�
string trim(const string& s) {
    size_t start = s.find_first_not_of(" \t\r\n");
    if (start == string::npos) return "";
    size_t end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

// ���������ı���ȥ�� "[Console]" �� "="
vector<string> processInputLines(const string& input) {
    vector<string> lines;
    istringstream iss(input);
    string line;

    while (getline(iss, line)) {
        string trimmed = trim(line);
        if (trimmed.empty()) continue;

        size_t pos = trimmed.find("[Console]");
        if (pos != string::npos) {
            trimmed.erase(pos, 9); // "[Console]" ����Ϊ9
            trimmed = trim(trimmed);
        }

        // �Ƴ� "=" ����������ո�
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

// ��ȡ��ǰʱ��� (YYYYMMDDHHMMSS)
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

    // ����ļ��޷��򿪣���ӡ���󲢷���
    if (!file) {
        std::cerr << "�޷��ҵ��ļ��������滻������\n";
        return;  // ֹֻͣ�滻������ִ�к���Ĵ���
    }

    std::string line;
    std::streampos lastCLPos = -1;  // ��¼ "CL" ��λ��
    size_t lastPosInLine = std::string::npos;

    // �����ļ����ݣ��ҵ����һ�� "CL"
    while (file) {
        std::streampos currentPos = file.tellg(); // ��¼ÿ�е���ʼλ��
        std::getline(file, line);
        size_t pos = line.rfind("CL"); // ���Ҹ��������һ�� "CL"

        if (pos != std::string::npos) {
            lastCLPos = currentPos;  // ��¼������ʼλ��
            lastPosInLine = pos;     // ��¼ "CL" �ڸ��е�����
        }
    }

    // ����ҵ��� "CL"�������滻����
    if (lastCLPos != -1) {
        file.clear(); // ��� EOF ״̬
        file.seekp(lastCLPos + static_cast<std::streamoff>(lastPosInLine)); // ��λ�����һ�� "CL" ��λ��
        file.write(k.c_str(), k.size()); // д���µ��ַ���
        file.write("\"", 1); // д��һ��˫���ţ�"��
        std::cout << "�滻�ɹ���\n";
    }
    else {
        std::cout << "δ�ҵ� 'CL'��\n";
    }

    file.close();
}

// ���� alias ����
string generateAliasCommands(const string& input) {
    vector<string> commands = processInputLines(input);

    // ÿ��4�У��������6��
    int groupCount = min(6, (int)((commands.size() + 3) / 4));
    // �õ�ǰʱ�������ǰ׺����֤����Ψһ
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
            // ���һ��β�����䣬ʹ���µ�ʱ������ɱ����滻�̶��� "CL"
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
                cout << "�¼�����������д�룺" << endl << result << endl;
            }
            else {
                cerr << "�޷����ļ�����д�룡\n";
            }
        }
        Sleep(1000);
    }
    return 0;
}
