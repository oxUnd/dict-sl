#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <stdint.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "config.h"

// Index entry structure
typedef struct {
    char *word;
    uint64_t offset;
    uint32_t size;
} IndexEntry;

// Dictionary info
typedef struct {
    char *bookname;
    uint32_t wordcount;
    uint32_t idxfilesize;
    int idxoffsetbits; // 32 or 64
    char *sametypesequence;
} DictInfo;

// Forward declaration
char *read_definition(const char *dict_dir, const char *dict_name, IndexEntry *entry);

// Function to read .ifo file
DictInfo *read_ifo(const char *dict_dir, const char *dict_name) {
    char ifo_path[1024];
    snprintf(ifo_path, sizeof(ifo_path), "%s/%s.ifo", dict_dir, dict_name);

    FILE *fp = fopen(ifo_path, "r");
    if (!fp) {
        fprintf(stderr, "Cannot open .ifo file: %s\n", ifo_path);
        return NULL;
    }

    DictInfo *info = calloc(1, sizeof(DictInfo));
    char line[1024];
    while (fgets(line, sizeof(line), fp)) {
        char *key = strtok(line, "=");
        char *value = strtok(NULL, "\n");
        if (!key || !value) continue;
        // Trim spaces
        while (*key == ' ' || *key == '\t') key++;
        char *end = key + strlen(key) - 1;
        while (end > key && (*end == ' ' || *end == '\t')) *end-- = '\0';
        while (*value == ' ' || *value == '\t') value++;
        end = value + strlen(value) - 1;
        while (end > value && (*end == ' ' || *end == '\t' || *end == '\n' || *end == '\r')) *end-- = '\0';

        if (strcmp(key, "bookname") == 0) info->bookname = strdup(value);
        else if (strcmp(key, "wordcount") == 0) info->wordcount = atoi(value);
        else if (strcmp(key, "idxfilesize") == 0) info->idxfilesize = atoi(value);
        else if (strcmp(key, "idxoffsetbits") == 0) info->idxoffsetbits = atoi(value);
        else if (strcmp(key, "sametypesequence") == 0) info->sametypesequence = strdup(value);
    }
    fclose(fp);
    if (!info->bookname || info->wordcount == 0) {
        free(info);
        return NULL;
    }
    if (info->idxoffsetbits == 0) info->idxoffsetbits = 32;
    return info;
}

// Function to read .idx file
IndexEntry *read_idx(const char *dict_dir, const char *dict_name, DictInfo *info) {
    char idx_path[1024];
    snprintf(idx_path, sizeof(idx_path), "%s/%s.idx", dict_dir, dict_name);

    FILE *fp = fopen(idx_path, "rb");
    if (!fp) {
        fprintf(stderr, "Cannot open .idx file: %s\n", idx_path);
        return NULL;
    }

    IndexEntry *entries = calloc(info->wordcount, sizeof(IndexEntry));
    for (uint32_t i = 0; i < info->wordcount; i++) {
        // Read word
        char word[256];
        size_t len = 0;
        while ((word[len] = fgetc(fp)) != '\0' && len < 255) len++;
        word[len] = '\0';
        entries[i].word = strdup(word);

        // Read offset (assuming 32-bit for simplicity)
        uint32_t offset32;
        fread(&offset32, 4, 1, fp);
        entries[i].offset = ntohl(offset32);

        // Read size
        uint32_t size32;
        fread(&size32, 4, 1, fp);
        entries[i].size = ntohl(size32);
    }
    fclose(fp);
    return entries;
}

// Binary search for word
int find_word(IndexEntry *entries, uint32_t count, const char *word) {
    int low = 0, high = count - 1;
    while (low <= high) {
        int mid = (low + high) / 2;
        int cmp = strcasecmp(entries[mid].word, word);
        if (cmp == 0) return mid;
        else if (cmp < 0) low = mid + 1;
        else high = mid - 1;
    }
    return -1;
}

// Function to find all words starting with prefix and display with partial translation
void list_words(IndexEntry *entries, uint32_t count, const char *prefix, const char *dict_dir, const char *dict_name) {
    size_t prefix_len = strlen(prefix);
    // Find the first word that starts with prefix
    int low = 0, high = count - 1;
    uint32_t start = count; // invalid
    while (low <= high) {
        int mid = (low + high) / 2;
        int cmp = strncasecmp(entries[mid].word, prefix, prefix_len);
        if (cmp >= 0) {
            high = mid - 1;
            if (cmp == 0) start = mid;
        } else {
            low = mid + 1;
        }
    }
    if (start == count) return; // No match
    // Now find the end
    uint32_t end = start;
    while (end < count && strncasecmp(entries[end].word, prefix, prefix_len) == 0) {
        char *definition = read_definition(dict_dir, dict_name, &entries[end]);
        if (definition) {
            // Extract first line or truncate to 100 chars
            char *newline = strchr(definition, '\n');
            if (newline) *newline = '\0';
            if (strlen(definition) > 100) definition[100] = '\0';
            printf("%s: %s\n", entries[end].word, definition);
            free(definition);
        } else {
            printf("%s: (no definition)\n", entries[end].word);
        }
        end++;
    }
}

// Function to read definition from .dict
char *read_definition(const char *dict_dir, const char *dict_name, IndexEntry *entry) {
    char dict_path[1024];
    snprintf(dict_path, sizeof(dict_path), "%s/%s.dict", dict_dir, dict_name);

    FILE *fp = fopen(dict_path, "rb");
    if (!fp) {
        fprintf(stderr, "Cannot open .dict file: %s\n", dict_path);
        return NULL;
    }

    fseek(fp, entry->offset, SEEK_SET);
    char *data = malloc(entry->size + 1);
    fread(data, 1, entry->size, fp);
    data[entry->size] = '\0';
    fclose(fp);
    return data;
}

// Main function
int main(int argc, char *argv[]) {
    const char *dict_dir;
    const char *dict_name;
    const char *mode;
    const char *query;

    if (argc == 5) {
        dict_dir = argv[1];
        dict_name = argv[2];
        mode = argv[3];
        query = argv[4];
    } else if (argc == 3) {
        dict_dir = default_dict_dir;
        dict_name = default_dict_name;
        mode = argv[1];
        query = argv[2];
    } else {
        fprintf(stderr, "Usage: %s [<dict_dir> <dict_name>] --query <prefix> | --word <word>\n", argv[0]);
        return 1;
    }

    if (strcmp(mode, "--query") != 0 && strcmp(mode, "-q") != 0 && strcmp(mode, "--word") != 0 && strcmp(mode, "-w") != 0) {
        fprintf(stderr, "Invalid mode: %s\n", mode);
        return 1;
    }

    // Read .ifo
    DictInfo *info = read_ifo(dict_dir, dict_name);
    if (!info) {
        fprintf(stderr, "Failed to read .ifo\n");
        return 1;
    }

    // Read .idx
    IndexEntry *entries = read_idx(dict_dir, dict_name, info);
    if (!entries) {
        fprintf(stderr, "Failed to read .idx\n");
        free(info);
        return 1;
    }

    // Handle query
    if (strcmp(mode, "--query") == 0 || strcmp(mode, "-q") == 0) {
        list_words(entries, info->wordcount, query, dict_dir, dict_name);
    } else if (strcmp(mode, "--word") == 0 || strcmp(mode, "-w") == 0) {
        int idx = find_word(entries, info->wordcount, query);
        if (idx != -1) {
            char *definition = read_definition(dict_dir, dict_name, &entries[idx]);
            if (definition) {
                printf("%s\n", definition);
                free(definition);
            } else {
                printf("No definition found.\n");
            }
        } else {
            printf("Word not found.\n");
        }
    }

    // Cleanup
    for (uint32_t i = 0; i < info->wordcount; i++) {
        free(entries[i].word);
    }
    free(entries);
    free(info->bookname);
    if (info->sametypesequence) free(info->sametypesequence);
    free(info);

    return 0;
}
