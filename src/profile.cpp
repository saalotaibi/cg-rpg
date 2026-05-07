#include "profile.h"

#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>

// =====================================================================
// Tiny hand-rolled JSON-ish reader/writer.
//
// We don't need a real JSON parser: the only producer of these files
// is profile_save() (or a careful human editor). We accept a strict
// subset:
//   - Top-level object with keys at any whitespace.
//   - "name": string
//   - "character_index": integer
//   - "tasks": array of objects with "name", "category", "stat_gain".
//
// The parser is forgiving about extra whitespace and trailing commas
// but will silently ignore anything it doesn't understand.
// =====================================================================

namespace {

const char* CATEGORY_NAMES[4] = { "FOCUS", "BODY", "MIND", "LIFE" };

const char* category_name(ItemCategory c) {
    int i = (int)c;
    if (i < 0 || i > 3) i = 0;
    return CATEGORY_NAMES[i];
}

ItemCategory parse_category(const char* s) {
    if (!s) return CAT_LIFE;
    if (std::strcmp(s, "FOCUS") == 0) return CAT_FOCUS;
    if (std::strcmp(s, "BODY")  == 0) return CAT_BODY;
    if (std::strcmp(s, "MIND")  == 0) return CAT_MIND;
    if (std::strcmp(s, "LIFE")  == 0) return CAT_LIFE;
    return CAT_LIFE;
}

bool ensure_dir(const char* dir) {
    struct stat st;
    if (stat(dir, &st) == 0) {
        return S_ISDIR(st.st_mode);
    }
    return mkdir(dir, 0755) == 0;
}

const char* home_dir() {
    const char* h = std::getenv("HOME");
    if (h && *h) return h;
    return "/tmp";
}

std::string profile_dir() {
    std::string d = home_dir();
    d += "/.cozy-room";
    return d;
}

// ---------- mini JSON parser -----------------------------------------

struct Parser {
    const char* p;
    const char* end;
    bool ok;
};

void skip_ws(Parser& P) {
    while (P.p < P.end && (*P.p == ' ' || *P.p == '\t' ||
                           *P.p == '\n' || *P.p == '\r' ||
                           *P.p == ',')) {
        P.p++;
    }
}

bool match_char(Parser& P, char c) {
    skip_ws(P);
    if (P.p < P.end && *P.p == c) { P.p++; return true; }
    return false;
}

// Reads a JSON string into out (truncated to out_cap-1 chars). Assumes
// the opening quote is the next non-ws char.
bool parse_string(Parser& P, char* out, int out_cap) {
    skip_ws(P);
    if (P.p >= P.end || *P.p != '"') { P.ok = false; return false; }
    P.p++;
    int n = 0;
    while (P.p < P.end && *P.p != '"') {
        char c = *P.p++;
        if (c == '\\' && P.p < P.end) {
            char esc = *P.p++;
            if      (esc == 'n')  c = '\n';
            else if (esc == 't')  c = '\t';
            else if (esc == 'r')  c = '\r';
            else                  c = esc;
        }
        if (n + 1 < out_cap) out[n++] = c;
    }
    if (n < out_cap) out[n] = '\0';
    if (P.p < P.end && *P.p == '"') P.p++;
    return true;
}

bool parse_int(Parser& P, int* out) {
    skip_ws(P);
    char buf[32];
    int n = 0;
    if (P.p < P.end && (*P.p == '-' || *P.p == '+')) {
        if (n < 31) buf[n++] = *P.p;
        P.p++;
    }
    while (P.p < P.end && std::isdigit((unsigned char)*P.p)) {
        if (n < 31) buf[n++] = *P.p;
        P.p++;
    }
    buf[n] = '\0';
    if (n == 0) { P.ok = false; return false; }
    *out = std::atoi(buf);
    return true;
}

// Read whole file into a string. Returns false on missing/empty.
bool read_file(const char* path, std::string& out) {
    FILE* f = std::fopen(path, "rb");
    if (!f) return false;
    std::fseek(f, 0, SEEK_END);
    long sz = std::ftell(f);
    if (sz < 0) { std::fclose(f); return false; }
    std::fseek(f, 0, SEEK_SET);
    out.resize((size_t)sz);
    if (sz > 0) {
        size_t got = std::fread(&out[0], 1, (size_t)sz, f);
        out.resize(got);
    }
    std::fclose(f);
    return true;
}

// Skip a value of unknown shape (used to discard keys we don't care about).
void skip_value(Parser& P) {
    skip_ws(P);
    if (P.p >= P.end) return;
    if (*P.p == '"') {
        char dump[256];
        parse_string(P, dump, sizeof(dump));
        return;
    }
    if (*P.p == '{') {
        int depth = 0;
        while (P.p < P.end) {
            char c = *P.p++;
            if (c == '{') depth++;
            else if (c == '}') { depth--; if (depth == 0) return; }
            else if (c == '"') {
                while (P.p < P.end && *P.p != '"') {
                    if (*P.p == '\\' && P.p + 1 < P.end) P.p++;
                    P.p++;
                }
                if (P.p < P.end) P.p++;
            }
        }
        return;
    }
    if (*P.p == '[') {
        int depth = 0;
        while (P.p < P.end) {
            char c = *P.p++;
            if (c == '[') depth++;
            else if (c == ']') { depth--; if (depth == 0) return; }
            else if (c == '"') {
                while (P.p < P.end && *P.p != '"') {
                    if (*P.p == '\\' && P.p + 1 < P.end) P.p++;
                    P.p++;
                }
                if (P.p < P.end) P.p++;
            }
        }
        return;
    }
    // Number or bareword.
    while (P.p < P.end && *P.p != ',' && *P.p != '}' && *P.p != ']' &&
           *P.p != '\n' && *P.p != '\r') {
        P.p++;
    }
}

// Convert a string in-place to uppercase ASCII. Used so the player
// name stored on disk matches the bitmap font's caps-only glyph set.
void to_upper_ascii(char* s) {
    for (; *s; s++) {
        unsigned char c = (unsigned char)*s;
        if (c >= 'a' && c <= 'z') *s = (char)(c - 'a' + 'A');
    }
}

} // namespace

const char* profile_path() {
    static char buf[512];
    std::snprintf(buf, sizeof(buf), "%s/profile.json", profile_dir().c_str());
    return buf;
}

void profile_delete() {
    std::remove(profile_path());
    // Leave the directory in place; cheap.
}

void profile_save(const GameState& gs) {
    std::string dir = profile_dir();
    if (!ensure_dir(dir.c_str())) {
        std::fprintf(stderr, "profile_save: mkdir %s failed\n", dir.c_str());
        return;
    }
    FILE* f = std::fopen(profile_path(), "wb");
    if (!f) {
        std::fprintf(stderr, "profile_save: open %s failed\n", profile_path());
        return;
    }
    std::fprintf(f, "{\n");
    std::fprintf(f, "  \"name\": \"%s\",\n", gs.player_name);
    std::fprintf(f, "  \"character_index\": %d,\n", gs.character_index);
    std::fprintf(f, "  \"tasks\": [\n");
    for (int i = 0; i < gs.item_count; i++) {
        const ItemDef& d = item_def((ItemId)i);
        std::fprintf(f, "    { \"name\": \"%s\", \"category\": \"%s\", \"stat_gain\": %d }%s\n",
                     d.name ? d.name : "",
                     category_name(d.category),
                     d.stat_gain,
                     (i + 1 < gs.item_count) ? "," : "");
    }
    std::fprintf(f, "  ]\n");
    std::fprintf(f, "}\n");
    std::fclose(f);
}

bool profile_load(GameState& gs) {
    std::string body;
    if (!read_file(profile_path(), body)) return false;
    if (body.empty()) return false;

    Parser P{ body.data(), body.data() + body.size(), true };
    if (!match_char(P, '{')) return false;

    char name_buf[ONB_MAX_NAME + 1] = {0};
    int  character_index = 0;
    bool have_name = false;
    bool have_char = false;

    ItemDef tasks[MAX_ITEMS];
    char    task_name_storage[MAX_ITEMS][ONB_MAX_TASK_NAME + 1] = {{0}};
    int     n_tasks = 0;
    bool    have_tasks = false;

    while (P.p < P.end) {
        skip_ws(P);
        if (P.p < P.end && *P.p == '}') { P.p++; break; }

        char key[64];
        if (!parse_string(P, key, sizeof(key))) break;
        if (!match_char(P, ':')) break;
        skip_ws(P);

        if (std::strcmp(key, "name") == 0) {
            parse_string(P, name_buf, sizeof(name_buf));
            to_upper_ascii(name_buf);
            have_name = true;
        } else if (std::strcmp(key, "character_index") == 0) {
            int v = 0;
            if (parse_int(P, &v)) {
                character_index = v;
                have_char = true;
            }
        } else if (std::strcmp(key, "tasks") == 0) {
            if (!match_char(P, '[')) { skip_value(P); continue; }
            while (P.p < P.end) {
                skip_ws(P);
                if (P.p < P.end && *P.p == ']') { P.p++; break; }
                if (!match_char(P, '{')) { skip_value(P); continue; }

                ItemDef d{};
                d.category = CAT_LIFE;
                d.stat_gain = 1;
                char tname[ONB_MAX_TASK_NAME + 1] = {0};
                char tcat[16]  = {0};

                while (P.p < P.end) {
                    skip_ws(P);
                    if (P.p < P.end && *P.p == '}') { P.p++; break; }
                    char k2[32];
                    if (!parse_string(P, k2, sizeof(k2))) break;
                    if (!match_char(P, ':')) break;
                    if (std::strcmp(k2, "name") == 0) {
                        parse_string(P, tname, sizeof(tname));
                        to_upper_ascii(tname);
                    } else if (std::strcmp(k2, "category") == 0) {
                        parse_string(P, tcat, sizeof(tcat));
                        to_upper_ascii(tcat);
                        d.category = parse_category(tcat);
                    } else if (std::strcmp(k2, "stat_gain") == 0) {
                        int v = 1; parse_int(P, &v);
                        d.stat_gain = v;
                    } else {
                        skip_value(P);
                    }
                    skip_ws(P);
                    if (P.p < P.end && *P.p == ',') P.p++;
                }

                if (n_tasks < MAX_ITEMS && tname[0] != '\0') {
                    size_t tn = std::strlen(tname);
                    if (tn > (size_t)ONB_MAX_TASK_NAME) tn = ONB_MAX_TASK_NAME;
                    std::memcpy(task_name_storage[n_tasks], tname, tn);
                    task_name_storage[n_tasks][tn] = '\0';
                    d.name = task_name_storage[n_tasks];
                    tasks[n_tasks] = d;
                    n_tasks++;
                }
                skip_ws(P);
                if (P.p < P.end && *P.p == ',') P.p++;
            }
            have_tasks = true;
        } else {
            skip_value(P);
        }

        skip_ws(P);
        if (P.p < P.end && *P.p == ',') P.p++;
    }

    // Apply to GameState. Anything we didn't read keeps the state_init
    // default.
    if (have_name) {
        std::strncpy(gs.player_name, name_buf, ONB_MAX_NAME);
        gs.player_name[ONB_MAX_NAME] = '\0';
    }
    if (have_char) {
        if (character_index < 0) character_index = 0;
        if (character_index >= ONB_NUM_CHARACTERS) character_index = ONB_NUM_CHARACTERS - 1;
        gs.character_index = character_index;
    }
    if (have_tasks && n_tasks > 0) {
        state_set_items(tasks, n_tasks);
    } else {
        state_set_default_items();
    }
    gs.item_count = item_count();
    std::memset(gs.item_done, 0, sizeof(gs.item_done));
    gs.last_unlocked = -1;
    gs.unlock_anim = 1.0f;
    return true;
}
