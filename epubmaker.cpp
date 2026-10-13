#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <sstream>
#include <cstdlib>
#include <ctime>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include <algorithm>
#include <cctype>

using namespace std;

string trim(const string &s) {
    size_t a = s.find_first_not_of(" \t\r\n");
    if (a == string::npos) return "";
    size_t b = s.find_last_not_of(" \t\r\n");
    return s.substr(a, b - a + 1);
}

void mkdir_p(const string &path) {
    mkdir(path.c_str(), 0755);
}

bool file_exists(const string &path) {
    struct stat st;
    return stat(path.c_str(), &st) == 0 && S_ISREG(st.st_mode);
}

void write_file(const string &path, const string &content) {
    ofstream f(path);
    if (!f) { cerr << "Failed to write: " << path << endl; exit(1); }
    f << content;
}

struct BookConfig {
    string title;
    string author;
    string lang;
    vector<string> chapter_files;
    string css_content;
    string project_dir;
    bool valid;
    string error_msg;
};

BookConfig parse_ini(const string &ini_path) {
    BookConfig cfg;
    cfg.valid = false;
    cfg.project_dir = ini_path.substr(0, ini_path.find_last_of('/'));
    if (cfg.project_dir.empty() || cfg.project_dir == ini_path) cfg.project_dir = ".";

    ifstream f(ini_path);
    if (!f) {
        cfg.error_msg = "Cannot open: " + ini_path;
        return cfg;
    }

    string section = "";
    string line;
    bool auto_mode = false;

    while (getline(f, line)) {
        line = trim(line);
        if (line.empty() || line[0] == '#' || line[0] == ';') continue;

        if (line[0] == '[' && line.back() == ']') {
            section = line.substr(1, line.size() - 2);
            continue;
        }

        size_t eq = line.find('=');
        if (eq == string::npos) continue;
        string key = trim(line.substr(0, eq));
        string val = trim(line.substr(eq + 1));

        if (section == "book") {
            if (key == "title") cfg.title = val;
            else if (key == "author") cfg.author = val;
            else if (key == "lang") cfg.lang = val;
        } else if (section == "chapters") {
            if (key == "list") {
                if (val == "auto" || val.empty()) {
                    auto_mode = true;
                } else {
                    // 手动列表
                    size_t start = 0;
                    while (start < val.size()) {
                        size_t comma = val.find(',', start);
                        if (comma == string::npos) comma = val.size();
                        string item = trim(val.substr(start, comma - start));
                        if (!item.empty()) cfg.chapter_files.push_back(item);
                        start = comma + 1;
                    }
                }
            }
        }
    }

    // auto 模式：扫描目录
    if (auto_mode) {
        DIR *dir = opendir(cfg.project_dir.c_str());
        if (!dir) {
            cfg.error_msg = "Cannot open project dir: " + cfg.project_dir;
            return cfg;
        }
        vector<string> html_files;
        struct dirent *entry;
        while ((entry = readdir(dir)) != NULL) {
            string fname = entry->d_name;
            if (fname.size() > 5 && fname.substr(fname.size() - 5) == ".html") {
                html_files.push_back(fname);
            }
        }
        closedir(dir);
        sort(html_files.begin(), html_files.end());
        if (html_files.empty()) {
            cfg.error_msg = "No .html files found in: " + cfg.project_dir;
            return cfg;
        }
        cfg.chapter_files = html_files;
    }

    if (cfg.title.empty()) {
        cfg.error_msg = "epub.ini: [book] title is required";
        return cfg;
    }
    if (cfg.author.empty()) cfg.author = "Anonymous";
    if (cfg.lang.empty()) cfg.lang = "zh";

    if (cfg.chapter_files.empty()) {
        cfg.error_msg = "epub.ini: [chapters] list is empty or missing (use 'list = auto' to scan)";
        return cfg;
    }
    for (const auto &cf : cfg.chapter_files) {
        string full = cfg.project_dir + "/" + cf;
        if (!file_exists(full)) {
            cfg.error_msg = "Chapter file not found: " + cf;
            return cfg;
        }
    }

    string css_path = cfg.project_dir + "/style.css";
    if (file_exists(css_path)) {
        ifstream csf(css_path);
        stringstream ss;
        ss << csf.rdbuf();
        cfg.css_content = ss.str();
    } else {
        cfg.css_content =
            "body{font-family:Georgia,serif;line-height:1.8;margin:1.5em;padding-bottom:2em}\n"
            "h1{color:#334;font-size:1.4em}\n"
            "p{margin:.8em 0}\n"
            "code{background:#eee;padding:2px 6px;border-radius:6px}\n"
            "img{max-width:100%;border-radius:8px}\n";
    }

    cfg.valid = true;
    return cfg;
}

string title_from_filename(const string &fname) {
    string name = fname;
    size_t dot = name.rfind('.');
    if (dot != string::npos) name = name.substr(0, dot);
    size_t i = 0;
    while (i < name.size() && isdigit(name[i])) i++;
    while (i < name.size() && (name[i] == '-' || name[i] == '_' || name[i] == ' ')) i++;
    if (i > 0 && i < name.size()) name = name.substr(i);
    if (name.empty()) name = fname;
    return name;
}

void print_help() {
    cout << "\nepubmaker++ (C++/Termux) - INI mode only\n\n";
    cout << "USAGE:\n";
    cout << "  epubmaker              Scan cwd for epub.ini\n";
    cout << "  epubmaker <dir>        Project directory\n";
    cout << "  epubmaker --help       Show this help\n\n";
    cout << "epub.ini:\n";
    cout << "  [book]\n";
    cout << "  title = My Book\n";
    cout << "  author = Anonymous\n";
    cout << "  lang = zh\n\n";
    cout << "  [chapters]\n";
    cout << "  list = auto          ← auto-scan all .html files\n";
    cout << "  list = 01.html, 02.html  ← manual list\n\n";
    cout << "Output: ~/epub_out/<title>.epub\n\n";
}

void build_book(const BookConfig &cfg) {
    cout << "----------------------------------------\n";
    cout << "  epubmaker++ (INI mode)\n";
    cout << "----------------------------------------\n\n";
    cout << "Title:    " << cfg.title << "\n";
    cout << "Author:   " << cfg.author << "\n";
    cout << "Language: " << cfg.lang << "\n";
    cout << "Chapters: " << cfg.chapter_files.size() << " (auto-scan)\n\n";

    for (size_t i = 0; i < cfg.chapter_files.size(); i++)
        cout << "  [" << i+1 << "] " << cfg.chapter_files[i] << "\n";

    string home = getenv("HOME");
    string work_dir = home + "/.epubmaker_tmp";
    string build_dir = work_dir + "/epub_build";
    string oebps = build_dir + "/OEBPS";
    string meta = build_dir + "/META-INF";

    system(("rm -rf \"" + work_dir + "\"").c_str());
    mkdir_p(work_dir); mkdir_p(build_dir); mkdir_p(meta); mkdir_p(oebps);

    write_file(oebps + "/style.css", cfg.css_content);

    vector<string> chap_titles;
    int chaps = cfg.chapter_files.size();
    for (int i = 0; i < chaps; i++) {
        string src = cfg.project_dir + "/" + cfg.chapter_files[i];
        string ct = title_from_filename(cfg.chapter_files[i]);
        chap_titles.push_back(ct);

        ifstream inf(src);
        stringstream ss;
        ss << inf.rdbuf();
        string body = ss.str();

        string cpath = oebps + "/chap-" + to_string(i) + ".xhtml";
        ofstream cf(cpath);
        cf << "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
           << "<!DOCTYPE html>\n"
           << "<html xmlns=\"http://www.w3.org/1999/xhtml\" lang=\"" << cfg.lang << "\">\n"
           << "<head><meta charset=\"utf-8\"/>\n"
           << "<title>" << ct << "</title>\n"
           << "<link rel=\"stylesheet\" type=\"text/css\" href=\"style.css\"/>\n"
           << "</head>\n<body>\n"
           << "<h1>" << ct << "</h1>\n"
           << body
           << "\n</body>\n</html>";
        cf.close();
    }

    write_file(build_dir + "/mimetype", "application/epub+zip");

    write_file(meta + "/container.xml",
        "<?xml version=\"1.0\"?>\n"
        "<container version=\"1.0\" xmlns=\"urn:oasis:names:tc:opendocument:xmlns:container\">\n"
        "  <rootfiles>\n"
        "    <rootfile full-path=\"OEBPS/content.opf\" media-type=\"application/oebps-package+xml\"/>\n"
        "  </rootfiles>\n"
        "</container>\n");

    ofstream nf(oebps + "/nav.xhtml");
    nf << "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
       << "<!DOCTYPE html>\n"
       << "<html xmlns=\"http://www.w3.org/1999/xhtml\" xmlns:epub=\"http://www.idpf.org/2007/ops\" lang=\"" << cfg.lang << "\">\n"
       << "<head><meta charset=\"utf-8\"/><title>nav</title></head>\n<body>\n"
       << "<nav epub:type=\"toc\" id=\"toc\"><ol>\n";
    for (int i = 0; i < chaps; i++)
        nf << "  <li><a href=\"chap-" << i << ".xhtml\">" << chap_titles[i] << "</a></li>\n";
    nf << "</ol></nav>\n</body>\n</html>";
    nf.close();

    time_t now_t = time(NULL);
    char now[32];
    strftime(now, 32, "%Y-%m-%dT%H:%M:%SZ", gmtime(&now_t));
    string uid = "urn:uuid:" + to_string(now_t) + "-" + to_string(getpid());

    ofstream of(oebps + "/content.opf");
    of << "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
       << "<package xmlns=\"http://www.idpf.org/2007/opf\" version=\"3.0\" unique-identifier=\"bookid\">\n"
       << "<metadata xmlns:dc=\"http://purl.org/dc/elements/1.1/\">\n"
       << "  <dc:title>" << cfg.title << "</dc:title>\n"
       << "  <dc:creator>" << cfg.author << "</dc:creator>\n"
       << "  <dc:language>" << cfg.lang << "</dc:language>\n"
       << "  <dc:identifier id=\"bookid\">" << uid << "</dc:identifier>\n"
       << "  <meta property=\"dcterms:modified\">" << now << "</meta>\n"
       << "</metadata>\n<manifest>\n"
       << "  <item id=\"nav\" href=\"nav.xhtml\" media-type=\"application/xhtml+xml\" properties=\"nav\"/>\n"
       << "  <item id=\"css\" href=\"style.css\" media-type=\"text/css\"/>\n";
    for (int i = 0; i < chaps; i++)
        of << "  <item id=\"chap" << i << "\" href=\"chap-" << i << ".xhtml\" media-type=\"application/xhtml+xml\"/>\n";
    of << "</manifest>\n<spine>\n";
    for (int i = 0; i < chaps; i++)
        of << "  <itemref idref=\"chap" << i << "\"/>\n";
    of << "</spine>\n</package>";
    of.close();

    string export_dir = home + "/epub_out";
    mkdir_p(export_dir);
    string tmp_epub = work_dir + "/" + cfg.title + ".epub";

    cout << "\nPacking...\n";
    string z1 = "cd \"" + build_dir + "\" && zip -X0 \"" + tmp_epub + "\" mimetype";
    system(z1.c_str());
    string z2 = "cd \"" + build_dir + "\" && zip -r \"" + tmp_epub + "\" META-INF OEBPS";
    system(z2.c_str());

    string final_epub = export_dir + "/" + cfg.title + ".epub";
    string mv_cmd = "mv \"" + tmp_epub + "\" \"" + final_epub + "\"";
    int mv_ret = system(mv_cmd.c_str());
    if (mv_ret != 0) {
        string cp_cmd = "cp \"" + tmp_epub + "\" \"" + final_epub + "\"";
        system(cp_cmd.c_str());
    }

    struct stat epub_stat;
    if (stat(final_epub.c_str(), &epub_stat) == 0) {
        cout << "\n----------------------------------------\n";
        cout << "Done!\n";
        cout << final_epub << "\n";
        cout << (epub_stat.st_size / 1024) << " KB\n";
        cout << "----------------------------------------\n\n";
    }

    system(("rm -rf \"" + work_dir + "\"").c_str());
}

int main(int argc, char **argv) {
    for (int i = 1; i < argc; i++) {
        if (string(argv[i]) == "--help" || string(argv[i]) == "-h") {
            print_help();
            return 0;
        }
    }

    string project_dir = ".";
    if (argc >= 2) {
        string arg = argv[1];
        if (arg[0] != '-') project_dir = arg;
    }

    string ini_path = project_dir + "/epub.ini";
    if (!file_exists(ini_path)) {
        cerr << "ERROR: epub.ini not found in: " << project_dir << "\n";
        cerr << "Run 'epubmaker --help' for usage.\n";
        return 1;
    }

    cout << "Reading " << ini_path << "...\n";
    BookConfig cfg = parse_ini(ini_path);
    if (!cfg.valid) {
        cerr << "ERROR: " << cfg.error_msg << "\n";
        return 1;
    }

    build_book(cfg);
    return 0;
}
