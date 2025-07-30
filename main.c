#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <stdint.h>
#include <sys/types.h>

#ifdef _WIN32
    #include <direct.h>
    #include <io.h>
    #define my_mkdir(path) _mkdir(path)
    #define access _access
    #define F_OK 0
    #define extcasecmp _stricmp   // MSVC / MinGW �´�Сд�޹رȽ�
#else
    #include <unistd.h>
    #include <dirent.h>
    #include <libgen.h>
    #define my_mkdir(path) mkdir(path, 0755)
    #define extcasecmp strcasecmp // POSIX
#endif

// ��������
int is_c_file(const char *filename);
int is_archive_file(const char *filename);
int is_modified_extension_file(const char *filepath);
const char* get_filename(const char* path);
void safe_filename(char *filename);
void create_output_dirs(void);
int copy_file_with_path(const char *src, const char *dest_dir, const char *reason, const char *relative_path);
int copy_file(const char *src, const char *dest_dir, const char *reason);
int get_extract_command(const char *filepath, const char *output_dir, char *command, size_t cmd_size);
int recursive_extract(const char *archive_path, const char *extract_dir, int depth, const char *base_extract_dir, const char *current_path);
void recursive_process_files(const char *current_dir, const char *base_extract_dir, int depth, const char *current_path);
void cleanup_temp_dir(const char *temp_dir);

// ����ļ��Ƿ�ΪCԴ�ļ�
int is_c_file(const char *filename) {
    const char *ext = strrchr(filename, '.');
    if (ext != NULL) {
        return (strcmp(ext, ".c") == 0 ||
                strcmp(ext, ".h") == 0 ||
                strcmp(ext, ".cpp") == 0 ||
                strcmp(ext, ".cc") == 0 ||
                strcmp(ext, ".cxx") == 0 ||
                strcmp(ext, ".hpp") == 0 ||
                strcmp(ext, ".hxx") == 0);
    }
    return 0;
}

// ����ļ��Ƿ�Ϊѹ����
int is_archive_file(const char *filename) {
    const char *ext = strrchr(filename, '.');
    if (ext != NULL) {
        return (strcmp(ext, ".tar") == 0 ||
                strcmp(ext, ".gz") == 0 ||
                strcmp(ext, ".bz2") == 0 ||
                strcmp(ext, ".xz") == 0 ||
                strcmp(ext, ".zip") == 0 ||
                strcmp(ext, ".rar") == 0 ||
                strcmp(ext, ".7z") == 0 ||
                strcmp(ext, ".tgz") == 0 ||
                strcmp(ext, ".tbz2") == 0 ||
                strcmp(ext, ".txz") == 0);
    }

    return 0;
}

// �����ļ���չ��ѡ����ʵĽ�ѹ����
int get_extract_command(const char *filepath, const char *output_dir, char *command, size_t cmd_size) {
    const char *filename = get_filename(filepath);
    const char *ext = strrchr(filename, '.');

    if (!ext) return 0;

    // ��鸴����չ��
    const char *second_ext = NULL;
    if (ext > filename) {
        const char *temp = ext - 1;
        while (temp > filename && *temp != '.') temp--;
        if (*temp == '.') second_ext = temp;
    }

    // ��������չ��
    if (second_ext && strncmp(second_ext, ".tar.gz", 7) == 0) {
#ifdef _WIN32
        snprintf(command, cmd_size, "tar -xzf \"%s\" -C \"%s\" 2>nul", filepath, output_dir);
#else
        snprintf(command, cmd_size, "tar -xzf \"%s\" -C \"%s\" 2>/dev/null", filepath, output_dir);
#endif
        return 1;
    }
    else if (second_ext && strncmp(second_ext, ".tar.bz2", 8) == 0) {
#ifdef _WIN32
        snprintf(command, cmd_size, "tar -xjf \"%s\" -C \"%s\" 2>nul", filepath, output_dir);
#else
        snprintf(command, cmd_size, "tar -xjf \"%s\" -C \"%s\" 2>/dev/null", filepath, output_dir);
#endif
        return 1;
    }
    else if (second_ext && strncmp(second_ext, ".tar.xz", 7) == 0) {
#ifdef _WIN32
        snprintf(command, cmd_size, "tar -xJf \"%s\" -C \"%s\" 2>nul", filepath, output_dir);
#else
        snprintf(command, cmd_size, "tar -xJf \"%s\" -C \"%s\" 2>/dev/null", filepath, output_dir);
#endif
        return 1;
    }
    // ����һ��չ��
    else if (strcmp(ext, ".tar") == 0) {
#ifdef _WIN32
        snprintf(command, cmd_size, "tar -xf \"%s\" -C \"%s\" 2>nul", filepath, output_dir);
#else
        snprintf(command, cmd_size, "tar -xf \"%s\" -C \"%s\" 2>/dev/null", filepath, output_dir);
#endif
        return 1;
    }
    else if (strcmp(ext, ".gz") == 0) {
#ifdef _WIN32
        snprintf(command, cmd_size, "gzip -d -c \"%s\" > \"%s\\%s\" 2>nul", filepath, output_dir, filename);
#else
        snprintf(command, cmd_size, "gzip -d -c \"%s\" > \"%s/%s\" 2>/dev/null", filepath, output_dir, filename);
#endif
        return 1;
    }
    else if (strcmp(ext, ".bz2") == 0) {
#ifdef _WIN32
        snprintf(command, cmd_size, "bzip2 -d -c \"%s\" > \"%s\\%s\" 2>nul", filepath, output_dir, filename);
#else
        snprintf(command, cmd_size, "bzip2 -d -c \"%s\" > \"%s/%s\" 2>/dev/null", filepath, output_dir, filename);
#endif
        return 1;
    }
    else if (strcmp(ext, ".xz") == 0) {
#ifdef _WIN32
        snprintf(command, cmd_size, "xz -d -c \"%s\" > \"%s\\%s\" 2>nul", filepath, output_dir, filename);
#else
        snprintf(command, cmd_size, "xz -d -c \"%s\" > \"%s/%s\" 2>/dev/null", filepath, output_dir, filename);
#endif
        return 1;
    }
    else if (strcmp(ext, ".zip") == 0) {
#ifdef _WIN32
        snprintf(command, cmd_size, "powershell -command \"Expand-Archive -Path '%s' -DestinationPath '%s' -Force\" 2>nul", filepath, output_dir);
#else
        snprintf(command, cmd_size, "unzip -q \"%s\" -d \"%s\" 2>/dev/null", filepath, output_dir);
#endif
        return 1;
    }
    else if (strcmp(ext, ".7z") == 0) {
#ifdef _WIN32
        snprintf(command, cmd_size, "7z x \"%s\" -o\"%s\" -y 2>nul", filepath, output_dir);
#else
        snprintf(command, cmd_size, "7z x \"%s\" -o\"%s\" -y 2>/dev/null", filepath, output_dir);
#endif
        return 1;
    }
    else if (strcmp(ext, ".rar") == 0) {
#ifdef _WIN32
        snprintf(command, cmd_size, "unrar x \"%s\" \"%s\\\" -y 2>nul", filepath, output_dir);
#else
        snprintf(command, cmd_size, "unrar x \"%s\" \"%s/\" -y 2>/dev/null", filepath, output_dir);
#endif
        return 1;
    }

    return 0;
}

/*========== 1. ħ������ ==========*/
typedef struct {
    const uint8_t sig[16];   // ħ���ֽڣ����㲹 0��
    uint8_t       sig_len;   // ��Ч����
    const char   *exts[8];   // ������չ��������� NULL ��β�������� 1 ������ NULL �� �ø�ʽͨ������չ��
} MagicEntry;

/* ֻҪ����Ҫ�ĸ�ʽ׷�ӽ������� */
static const MagicEntry magic_tbl[] = {
    /* ͼƬ */
    {{0x89,0x50,0x4E,0x47},                  4, {".png", NULL}},
    {{0xFF,0xD8,0xFF},                       3, {".jpg",".jpeg", NULL}},
    {{0x47,0x49,0x46,0x38},                  4, {".gif", NULL}},
    {{0x42,0x4D},                            2, {".bmp", NULL}},
    {{0x49,0x49,0x2A,0x00},                  4, {".tif",".tiff", NULL}},   // TIFF little?endian
    {{0x4D,0x4D,0x00,0x2A},                  4, {".tif",".tiff", NULL}},   // TIFF big?endian
    {{0x52,0x49,0x46,0x46},                  4, {".webp" /* ����ٶ����ж� ��WEBP�� chunk */, NULL}},

    /* ��Ƶ / ��Ƶ ���� */
    {{0x52,0x49,0x46,0x46},                  4, {".wav",".avi",".webp", NULL}}, // RIFF �����ܶ��ʽ
    {{0x00,0x00,0x00,0x18,0x66,0x74,0x79,0x70}, 8, {".mp4",".mov",".m4a",".m4v", NULL}},
    {{0x1A,0x45,0xDF,0xA3},                  4, {".mkv",".webm", NULL}},   // Matroska / WebM
    {{0xFF,0xFB},                            2, {".mp3", NULL}},           // ����� ID3 �ж�
    {{0x4F,0x67,0x67,0x53},                  4, {".ogg",".oga", NULL}},
    {{0x66,0x4C,0x61,0x43},                  4, {".flac", NULL}},

    /* �ĵ� / ���� */
    {{0x25,0x50,0x44,0x46},                  4, {".pdf", NULL}},
    {{0x50,0x4B,0x03,0x04},                  4, {".zip",".docx",".xlsx",".pptx",".jar",".apk", NULL}},
    {{0xD0,0xCF,0x11,0xE0,0xA1,0xB1,0x1A,0xE1}, 8, {".doc",".xls",".ppt",".msi", NULL}}, // �ɰ� OLE

    /* ѹ�� / �鵵 */
    {{0x1F,0x8B,0x08},                       3, {".gz", NULL}},
    {{0x42,0x5A,0x68},                       3, {".bz2", NULL}},
    {{0xFD,0x37,0x7A,0x58,0x5A,0x00},        6, {".xz", NULL}},
    {{0x28,0xB5,0x2F,0xFD},                  4, {".zst", NULL}},
    {{0x37,0x7A,0xBC,0xAF,0x27,0x1C},        6, {".7z", NULL}},
    {{0x52,0x61,0x72,0x21,0x1A,0x07},        6, {".rar", NULL}},           // RAR v4
    {{0x52,0x61,0x72,0x21,0x1A,0x07,0x00},   7, {".rar", NULL}},           // RAR v5

    /* ��ִ�� / �� */
    {{0x7F,0x45,0x4C,0x46},                  4, {".so", NULL}},                   // ELF, ͨ������չ
    {{0x4D,0x5A},                            2, {".exe",".dll",".sys",".ocx",".scr",".drv", NULL}}, // PE (DOS MZ)

    /* ������� */
    {{0}, 0, {NULL}}
};

/*========== 2. ����ʵ�� ==========*/
int is_modified_extension_file(const char *filepath)
{
    /* ��ǰ 16 �ֽ� */
    uint8_t buf[16] = {0};
    FILE *fp = fopen(filepath, "rb");
    if (!fp) return 0;
    size_t n = fread(buf, 1, sizeof(buf), fp);
    fclose(fp);
    if (n == 0) return 0;

    /* �õ�ǰ��չ���������㣩������Ϊ NULL */
    const char *ext = strrchr(filepath, '.');

    /* ����ħ���� */
    for (int i = 0; magic_tbl[i].sig_len; ++i)
    {
        const MagicEntry *m = &magic_tbl[i];
        if (n < m->sig_len) continue;
        if (memcmp(buf, m->sig, m->sig_len) != 0) continue;   // ħ����ƥ��

        /*== ħ��ƥ��ɹ� ==*/

        /* 1) �ø�ʽͨ������չ�� */
        if (m->exts[0] == NULL)
            return (ext != NULL);   // ����չ�� ? ������

        /* 2) ԭ��Ӧ������չ�� */
        if (ext == NULL) return 1;  // û��չ�� ? ������

        /* 3) ����Ƿ����������� */
        for (int k = 0; m->exts[k]; ++k) {
            if (extcasecmp(ext, m->exts[k]) == 0)
                return 0;           // ������չ��
        }
        return 1;                   // ���������� ? ������
    }

    /* û�ҵ���Ӧħ�� => ���жϣ���Ϊ���� 0 */
    return 0;
}

// �������Ŀ¼
void create_output_dirs() {
    my_mkdir("result");
    my_mkdir("result/extracted_c_files");
    my_mkdir("result/extracted_archives");
    my_mkdir("result/extracted_modified_files");
    my_mkdir("result/extracted_other_files");  // ���������ļ�Ŀ¼
}

// ��ȡ�ļ�������ƽ̨��
const char* get_filename(const char* path) {
    const char* filename = strrchr(path, '/');
    if (!filename) filename = strrchr(path, '\\');
    return filename ? filename + 1 : path;
}

// ��ȫ���ļ��������滻·���ָ���Ϊ����ָ�����
void safe_filename(char *filename) {
    for (int i = 0; filename[i]; i++) {
        if (filename[i] == '/' || filename[i] == '\\') {
            filename[i] = '@';  // ʹ��@��Ϊ·���ָ������
        } else if (filename[i] == ':' || filename[i] == '*' ||
                   filename[i] == '?' || filename[i] == '"' ||
                   filename[i] == '<' || filename[i] == '>' || filename[i] == '|') {
            filename[i] = '#';  // ���������ַ���#���
        }
    }
}

// �����ļ���ָ��Ŀ¼����·��ǰ׺��
int copy_file_with_path(const char *src, const char *dest_dir, const char *reason, const char *relative_path) {
    char dest_path[2048];
    char safe_path[1024];
    const char *filename = get_filename(src);

    // ������ȫ��·��ǰ׺
    if (relative_path && strlen(relative_path) > 0) {
        strncpy(safe_path, relative_path, sizeof(safe_path) - 1);
        safe_path[sizeof(safe_path) - 1] = '\0';
        safe_filename(safe_path);
        snprintf(dest_path, sizeof(dest_path), "%s/[%s]%s", dest_dir, safe_path, filename);
    } else {
        snprintf(dest_path, sizeof(dest_path), "%s/%s", dest_dir, filename);
    }

    FILE *src_file = fopen(src, "rb");
    if (!src_file) {
        printf("Error: Unable to open the source file, %s\n", src);
        return 0;
    }

    FILE *dest_file = fopen(dest_path, "wb");
    if (!dest_file) {
        printf("Error: Unable to open the source file, %s\n", dest_path);
        fclose(src_file);
        return 0;
    }

    char buffer[4096];
    size_t bytes;
    while ((bytes = fread(buffer, 1, sizeof(buffer), src_file)) > 0) {
        fwrite(buffer, 1, bytes, dest_file);
    }

    fclose(src_file);
    fclose(dest_file);

    printf("Extracted %s: %s -> %s\n", reason, src, dest_path);
    return 1;
}

// �����ļ���ָ��Ŀ¼
int copy_file(const char *src, const char *dest_dir, const char *reason) {
    return copy_file_with_path(src, dest_dir, reason, NULL);
}

// �ݹ��ѹѹ����
int recursive_extract(const char *archive_path, const char *extract_dir, int depth, const char *base_extract_dir, const char *current_path) {
    if (depth > 10) {  // ��ֹ���޵ݹ飬����������Ϊ10
        printf("Warning: Maximum recursion depth reached, stopping decompression. %s\n", archive_path);
        return 0;
    }

    printf("decompressing (depth %d): %s\n", depth, get_filename(archive_path));

    // Ϊ��ǰѹ����������ʱ��ѹĿ¼
    char temp_extract_dir[1024];
    snprintf(temp_extract_dir, sizeof(temp_extract_dir), "%s_temp_%d", extract_dir, depth);
    my_mkdir(temp_extract_dir);

    // ��ȡ��ѹ����
    char extract_command[1024];
    if (!get_extract_command(archive_path, temp_extract_dir, extract_command, sizeof(extract_command))) {
        printf("Waring: Can not decompress the file: %s\n", archive_path);
        cleanup_temp_dir(temp_extract_dir);
        return 0;
    }

    // ִ�н�ѹ����
    int result = system(extract_command);
    if (result != 0) {
        printf("Waring: decompress failed %s\n", archive_path);
        cleanup_temp_dir(temp_extract_dir);
        return 0;
    }

    // ������ǰ·��ǰ׺
    char new_path[1024];
    if (current_path && strlen(current_path) > 0) {
        snprintf(new_path, sizeof(new_path), "%s/%s", current_path, get_filename(archive_path));
    } else {
        strncpy(new_path, get_filename(archive_path), sizeof(new_path) - 1);
        new_path[sizeof(new_path) - 1] = '\0';
    }

    // �����ѹ�����ļ�
    recursive_process_files(temp_extract_dir, base_extract_dir, depth + 1, new_path);

    // ������ʱĿ¼
    cleanup_temp_dir(temp_extract_dir);
    return 1;
}

// �ݹ鴦���ļ���������һ����ѹ��
void recursive_process_files(const char *current_dir, const char *base_extract_dir, int depth, const char *current_path) {
#ifdef _WIN32
    char search_path[1024];
    snprintf(search_path, sizeof(search_path), "%s\\*", current_dir);

    struct _finddata_t file_info;
    intptr_t handle = _findfirst(search_path, &file_info);

    if (handle == -1) {
        return;
    }

    do {
        if (strcmp(file_info.name, ".") == 0 || strcmp(file_info.name, "..") == 0) {
            continue;
        }

        char filepath[1024];
        snprintf(filepath, sizeof(filepath), "%s\\%s", current_dir, file_info.name);

        // �������·������һ�εݹ����
        char relative_path[1024];
        if (current_path && strlen(current_path) > 0) {
            snprintf(relative_path, sizeof(relative_path), "%s/%s", current_path, file_info.name);
        } else {
            strncpy(relative_path, file_info.name, sizeof(relative_path) - 1);
            relative_path[sizeof(relative_path) - 1] = '\0';
        }

        // ������ǰ·��ǰ׺����������ǰ�ļ�����
        char path_prefix[1024];
        if (current_path && strlen(current_path) > 0) {
            strncpy(path_prefix, current_path, sizeof(path_prefix) - 1);
            path_prefix[sizeof(path_prefix) - 1] = '\0';
        } else {
            path_prefix[0] = '\0';
        }

        if (file_info.attrib & _A_SUBDIR) {
            // �ݹ鴦����Ŀ¼
            recursive_process_files(filepath, base_extract_dir, depth, relative_path);
        } else {
            // �����ļ�
            if (is_archive_file(file_info.name)) {
                // ����ȡѹ��������archivesĿ¼
                copy_file_with_path(filepath, "result/extracted_archives", "Archive file", path_prefix);
                // Ȼ��ݹ��ѹ
                recursive_extract(filepath, base_extract_dir, depth, base_extract_dir, path_prefix);
            } else {
                // �������ͨ�ļ����������ͷ���
                if (is_c_file(file_info.name)) {
                    copy_file_with_path(filepath, "result/extracted_c_files", "C file", path_prefix);
                } else if (is_modified_extension_file(filepath)) {
                    copy_file_with_path(filepath, "result/extracted_modified_files", "modified extension file", path_prefix);
                } else {
                    // �����ļ�Ҳ��ȡ����
                    copy_file_with_path(filepath, "result/extracted_other_files", "other file", path_prefix);
                }
            }
        }
    } while (_findnext(handle, &file_info) == 0);

    _findclose(handle);
#else
    DIR *dir = opendir(current_dir);
    if (!dir) {
        return;
    }

    struct dirent *entry;
    char filepath[1024];

    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        snprintf(filepath, sizeof(filepath), "%s/%s", current_dir, entry->d_name);

        // �������·��
        char relative_path[1024];
        if (current_path && strlen(current_path) > 0) {
            snprintf(relative_path, sizeof(relative_path), "%s/%s", current_path, entry->d_name);
        } else {
            strncpy(relative_path, entry->d_name, sizeof(relative_path) - 1);
            relative_path[sizeof(relative_path) - 1] = '\0';
        }

        // ������ǰ·��ǰ׺����������ǰ�ļ�����
        char path_prefix[1024];
        if (current_path && strlen(current_path) > 0) {
            strncpy(path_prefix, current_path, sizeof(path_prefix) - 1);
            path_prefix[sizeof(path_prefix) - 1] = '\0';
        } else {
            path_prefix[0] = '\0';
        }

        struct stat file_stat;
        if (stat(filepath, &file_stat) != 0) {
            continue;
        }

        if (S_ISDIR(file_stat.st_mode)) {
            // �ݹ鴦����Ŀ¼
            recursive_process_files(filepath, base_extract_dir, depth, relative_path);
        } else {
            // �����ļ�
            if (is_archive_file(entry->d_name)) {
                // ����ȡѹ��������archivesĿ¼
                copy_file_with_path(filepath, "result/extracted_archives", "ѹ����", path_prefix);
                // Ȼ��ݹ��ѹ
                recursive_extract(filepath, base_extract_dir, depth, base_extract_dir, path_prefix);
            } else {
                // �������ͨ�ļ����������ͷ���
                if (is_c_file(entry->d_name)) {
                    copy_file_with_path(filepath, "result/extracted_c_files", "CԴ�ļ�", path_prefix);
                } else if (is_modified_extension_file(filepath)) {
                    copy_file_with_path(filepath, "result/extracted_modified_files", "�޸���չ�����ļ�", path_prefix);
                } else {
                    // �����ļ�Ҳ��ȡ����
                    copy_file_with_path(filepath, "result/extracted_other_files", "�����ļ�", path_prefix);
                }
            }
        }
    }

    closedir(dir);
#endif
}

// ������ʱĿ¼
void cleanup_temp_dir(const char *temp_dir) {
    char command[512];
#ifdef _WIN32
    snprintf(command, sizeof(command), "rmdir /s /q \"%s\" 2>nul", temp_dir);
#else
    snprintf(command, sizeof(command), "rm -rf \"%s\"", temp_dir);
#endif
    system(command);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("use: %s <archive file path>\n", argv[0]);
        return 1;
    }

    const char *tar_path = argv[1];

    // ���tar���Ƿ����
    struct stat file_stat;
    if (stat(tar_path, &file_stat) != 0) {
        printf("Error: Can not find the file: %s\n", tar_path);
        return 1;
    }

    printf("Begin to process the archive file: %s\n", tar_path);

    // �������Ŀ¼
    create_output_dirs();

    // ������ʱ��ȡĿ¼
    const char *temp_dir = "temp_extract_dir";
    my_mkdir(temp_dir);

    // ��ȡtar��
    char extract_command[512];
#ifdef _WIN32
    // Windows��ʹ��tar���Windows 10�����ϰ汾�Դ���
    snprintf(extract_command, sizeof(extract_command), "tar -xf \"%s\" -C \"%s\" 2>nul", tar_path, temp_dir);
#else
    snprintf(extract_command, sizeof(extract_command), "tar -xf \"%s\" -C \"%s\" 2>/dev/null", tar_path, temp_dir);
#endif

    int extract_result = system(extract_command);
    if (extract_result != 0) {
        printf("Error: Unable to extract the tar archive; it may not be a valid tar file.\n");
        cleanup_temp_dir(temp_dir);
        return 1;
    }

    printf("The tar archive has been extracted. Starting recursive analysis and file decompression...\n");

    // �ݹ鴦����ȡ���ļ���������һ����ѹǶ�׵�ѹ������
    recursive_process_files(temp_dir, temp_dir, 0, "");

    // ������ʱĿ¼
    cleanup_temp_dir(temp_dir);

    printf("\nProcess Done��\n");
    printf("- C file has been saved to: result/extracted_c_files/\n");
    printf("- archive file has been saved to: result/extracted_archives/\n");
    printf("- The file with the modified extension has been saved to: result/extracted_modified_files/\n");
    printf("- Other file has been saved to: result/extracted_other_files/\n");
    return 0;
}