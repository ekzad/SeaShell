
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>
#include <curl/curl.h>

static const char *my_strcasestr(const char *haystack, const char *needle) {
    if (!*needle) return haystack;
    for (; *haystack; haystack++) {
        const char *h = haystack, *n = needle;
        while (*h && *n && tolower((unsigned char)*h) == tolower((unsigned char)*n)) {
            h++; n++;
        }
        if (!*n) return haystack;
    }
    return NULL;
}
#define strcasestr my_strcasestr

#define CONNECT_TIMEOUT   8L
#define TOTAL_TIMEOUT     15L
#define USER_AGENT        "Spark Browser/2.0"
#define MAX_HISTORY       50
#define MAX_LINKS         100
#define URLBUF            2048

typedef struct {
    char url[URLBUF];
    char text[256];
} LinkEntry;

typedef struct {
    char *data;
    size_t len;
} MemBuf;

typedef struct {
    char url[URLBUF];
    long status;
    char content_type[256];
    char server[256];
    size_t size_bytes;
    char title[256];
    char *body_text;
    LinkEntry links[MAX_LINKS];
    int link_count;
} Page;

static char g_history[MAX_HISTORY][URLBUF];
static int g_history_count = 0;

static void history_push(const char *url) {
    if (g_history_count >= MAX_HISTORY) {
        memmove(g_history[0], g_history[1], (MAX_HISTORY - 1) * URLBUF);
        g_history_count--;
    }
    snprintf(g_history[g_history_count], URLBUF, "%s", url);
    g_history_count++;
}

static int history_pop(char *out, size_t outsz) {
    if (g_history_count == 0) return -1;
    g_history_count--;
    snprintf(out, outsz, "%s", g_history[g_history_count]);
    return 0;
}

static void trim(char *s) {
    size_t len = strlen(s);
    while (len > 0 && isspace((unsigned char)s[len - 1])) s[--len] = '\0';
    size_t start = 0;
    while (s[start] && isspace((unsigned char)s[start])) start++;
    if (start > 0) memmove(s, s + start, len - start + 1);
}

static void format_size(size_t bytes, char *out, size_t outsz) {
    if (bytes >= 1024 * 1024) {
        snprintf(out, outsz, "%.1f MB", bytes / (1024.0 * 1024.0));
    } else if (bytes >= 1024) {
        snprintf(out, outsz, "%.1f KB", bytes / 1024.0);
    } else {
        snprintf(out, outsz, "%zu bytes", bytes);
    }
}

static const char *status_phrase(long code) {
    switch (code) {
        case 200: return "OK";
        case 201: return "Created";
        case 204: return "No Content";
        case 301: return "Moved Permanently";
        case 302: return "Found";
        case 304: return "Not Modified";
        case 400: return "Bad Request";
        case 401: return "Unauthorized";
        case 403: return "Forbidden";
        case 404: return "Not Found";
        case 429: return "Too Many Requests";
        case 500: return "Internal Server Error";
        case 502: return "Bad Gateway";
        case 503: return "Service Unavailable";
        default:  return "";
    }
}

static void get_scheme_host(const char *url, char *out, size_t outsz) {
    const char *p = strstr(url, "://");
    if (!p) { snprintf(out, outsz, "%s", url); return; }
    const char *host_start = p + 3;
    const char *path = strchr(host_start, '/');
    size_t len = path ? (size_t)(path - url) : strlen(url);
    if (len >= outsz) len = outsz - 1;
    memcpy(out, url, len);
    out[len] = '\0';
}

static void get_base_dir(const char *url, char *out, size_t outsz) {
    const char *p = strstr(url, "://");
    const char *search_start = p ? p + 3 : url;
    const char *last_slash = strrchr(search_start, '/');
    if (!last_slash) {
        snprintf(out, outsz, "%s/", url);
        return;
    }
    size_t len = (size_t)(last_slash - url) + 1;
    if (len >= outsz) len = outsz - 1;
    memcpy(out, url, len);
    out[len] = '\0';
}

static int resolve_url(const char *base_url, const char *href, char *out, size_t outsz) {
    if (href[0] == '\0' || href[0] == '#') return -1;
    if (strncasecmp(href, "javascript:", 11) == 0) return -1;
    if (strncasecmp(href, "mailto:", 7) == 0) return -1;
    if (strncasecmp(href, "tel:", 4) == 0) return -1;

    if (strncasecmp(href, "http://", 7) == 0 || strncasecmp(href, "https://", 8) == 0) {
        snprintf(out, outsz, "%s", href);
        return 0;
    }
    char scheme_host[URLBUF];
    get_scheme_host(base_url, scheme_host, sizeof(scheme_host));

    if (href[0] == '/' && href[1] == '/') {
        const char *colon = strchr(scheme_host, ':');
        char scheme[16] = "https";
        if (colon) { size_t sl = (size_t)(colon - scheme_host); if (sl < sizeof(scheme)) { memcpy(scheme, scheme_host, sl); scheme[sl] = '\0'; } }
        snprintf(out, outsz, "%s:%s", scheme, href);
        return 0;
    }
    if (href[0] == '/') {
        snprintf(out, outsz, "%s%s", scheme_host, href);
        return 0;
    }
    char base_dir[URLBUF];
    get_base_dir(base_url, base_dir, sizeof(base_dir));
    snprintf(out, outsz, "%s%s", base_dir, href);
    return 0;
}

static void html_decode_into(const char *s, size_t n, char *out, size_t *out_len, size_t outsz) {
    for (size_t i = 0; i < n && *out_len < outsz - 1; i++) {
        if (s[i] == '&') {
            if (strncmp(&s[i], "&amp;", 5) == 0)  { out[(*out_len)++] = '&'; i += 4; continue; }
            if (strncmp(&s[i], "&lt;", 4) == 0)   { out[(*out_len)++] = '<'; i += 3; continue; }
            if (strncmp(&s[i], "&gt;", 4) == 0)   { out[(*out_len)++] = '>'; i += 3; continue; }
            if (strncmp(&s[i], "&quot;", 6) == 0) { out[(*out_len)++] = '"'; i += 5; continue; }
            if (strncmp(&s[i], "&#39;", 5) == 0)  { out[(*out_len)++] = '\''; i += 4; continue; }
            if (strncmp(&s[i], "&nbsp;", 6) == 0) { out[(*out_len)++] = ' '; i += 5; continue; }
        }
        out[(*out_len)++] = s[i];
    }
}

static void extract_title(const char *html, char *out, size_t outsz) {
    out[0] = '\0';
    const char *start = strcasestr(html, "<title>");
    if (!start) return;
    start += 7;
    const char *end = strcasestr(start, "</title>");
    if (!end) return;
    size_t len = 0;
    html_decode_into(start, (size_t)(end - start), out, &len, outsz);
    out[len] = '\0';
    trim(out);
}

static char *extract_text(const char *html) {
    size_t len = strlen(html);
    char *out = malloc(len + 1);
    size_t out_len = 0;
    size_t i = 0, text_start = 0;
    int in_tag = 0, skip_content = 0;

    while (i < len) {
        if (html[i] == '<') {
            if (!in_tag && !skip_content && i > text_start) {
                html_decode_into(&html[text_start], i - text_start, out, &out_len, len + 1);
            }
            in_tag = 1;
            if (strncasecmp(&html[i], "<script", 7) == 0) skip_content = 1;
            if (strncasecmp(&html[i], "<style", 6) == 0)  skip_content = 1;
            if (strncasecmp(&html[i], "</script", 8) == 0) skip_content = 0;
            if (strncasecmp(&html[i], "</style", 7) == 0)  skip_content = 0;
        }
        if (html[i] == '>') { in_tag = 0; text_start = i + 1; }
        i++;
    }
    if (!in_tag && !skip_content && len > text_start) {
        html_decode_into(&html[text_start], len - text_start, out, &out_len, len + 1);
    }
    out[out_len] = '\0';
    return out;
}

static int extract_links(const char *html, const char *base_url, LinkEntry *links, int max_links) {
    size_t len = strlen(html);
    size_t i = 0;
    int count = 0;

    while (i < len && count < max_links) {
        if (html[i] == '<' && (strncasecmp(&html[i], "<a ", 3) == 0 || strncasecmp(&html[i], "<a\t", 3) == 0)) {
            const char *tagend = strchr(&html[i], '>');
            if (!tagend) break;

            char raw_href[URLBUF] = {0};
            int have_href = 0;
            const char *scan = &html[i];
            while (scan < tagend) {
                if (strncasecmp(scan, "href=", 5) == 0) {
                    const char *v = scan + 5;
                    char quote = *v;
                    if (quote == '"' || quote == '\'') {
                        v++;
                        const char *end = strchr(v, quote);
                        if (end && end < tagend + 2) {
                            size_t hl = (size_t)(end - v);
                            if (hl >= sizeof(raw_href)) hl = sizeof(raw_href) - 1;
                            memcpy(raw_href, v, hl);
                            raw_href[hl] = '\0';
                            have_href = 1;
                        }
                    }
                    break;
                }
                scan++;
            }

            const char *close = strcasestr(tagend, "</a>");
            char text[256] = {0};
            if (close) {
                size_t tl = 0;
                const char *p = tagend + 1;
                int intag = 0;
                while (p < close && tl < sizeof(text) - 1) {
                    if (*p == '<') intag = 1;
                    else if (*p == '>') intag = 0;
                    else if (!intag) {
                        text[tl++] = (*p == '\n' || *p == '\t' || *p == '\r') ? ' ' : *p;
                    }
                    p++;
                }
                text[tl] = '\0';
                trim(text);
            }

            if (have_href) {
                char resolved[URLBUF];
                if (resolve_url(base_url, raw_href, resolved, sizeof(resolved)) == 0) {
                    snprintf(links[count].url, URLBUF, "%s", resolved);
                    snprintf(links[count].text, sizeof(links[count].text), "%s",
                             text[0] ? text : "(no text)");
                    count++;
                }
            }
            i = close ? (size_t)(close - html) + 4 : (size_t)(tagend - html) + 1;
            continue;
        }
        i++;
    }
    return count;
}

static size_t write_cb(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t realsize = size * nmemb;
    MemBuf *mb = (MemBuf *)userp;
    char *ptr = realloc(mb->data, mb->len + realsize + 1);
    if (!ptr) return 0;
    mb->data = ptr;
    memcpy(&mb->data[mb->len], contents, realsize);
    mb->len += realsize;
    mb->data[mb->len] = '\0';
    return realsize;
}

static size_t header_cb(char *buffer, size_t size, size_t nitems, void *userp) {
    size_t total = size * nitems;
    Page *pg = (Page *)userp;
    if (total > 14 && strncasecmp(buffer, "Content-Type:", 13) == 0) {
        const char *v = buffer + 13;
        while (*v == ' ') v++;
        size_t vlen = total - (size_t)(v - buffer);
        while (vlen > 0 && (v[vlen - 1] == '\r' || v[vlen - 1] == '\n')) vlen--;
        if (vlen >= sizeof(pg->content_type)) vlen = sizeof(pg->content_type) - 1;
        memcpy(pg->content_type, v, vlen);
        pg->content_type[vlen] = '\0';
    } else if (total > 7 && strncasecmp(buffer, "Server:", 7) == 0) {
        const char *v = buffer + 7;
        while (*v == ' ') v++;
        size_t vlen = total - (size_t)(v - buffer);
        while (vlen > 0 && (v[vlen - 1] == '\r' || v[vlen - 1] == '\n')) vlen--;
        if (vlen >= sizeof(pg->server)) vlen = sizeof(pg->server) - 1;
        memcpy(pg->server, v, vlen);
        pg->server[vlen] = '\0';
    }
    return total;
}

static int fetch_page(const char *destination, Page *pg, char *errmsg, size_t errsz) {
    char url[URLBUF];
    if (strncmp(destination, "http://", 7) != 0 && strncmp(destination, "https://", 8) != 0) {
        snprintf(url, sizeof(url), "https://%s", destination);
    } else {
        snprintf(url, sizeof(url), "%s", destination);
    }

    CURL *curl = curl_easy_init();
    if (!curl) { snprintf(errmsg, errsz, "failed to init curl"); return -1; }

    memset(pg, 0, sizeof(*pg));
    strcpy(pg->content_type, "unknown");
    strcpy(pg->server, "unknown");

    MemBuf body = {0};
    body.data = malloc(1);
    body.data[0] = '\0';

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_cb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&body);
    curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, header_cb);
    curl_easy_setopt(curl, CURLOPT_HEADERDATA, (void *)pg);

    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);
    curl_easy_setopt(curl, CURLOPT_PROTOCOLS_STR, "http,https");
    curl_easy_setopt(curl, CURLOPT_REDIR_PROTOCOLS_STR, "http,https");
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 5L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, USER_AGENT);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, CONNECT_TIMEOUT);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, TOTAL_TIMEOUT);

    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        snprintf(errmsg, errsz, "%s", curl_easy_strerror(res));
        curl_easy_cleanup(curl);
        free(body.data);
        return -1;
    }

    char *eff_url = NULL;
    curl_easy_getinfo(curl, CURLINFO_EFFECTIVE_URL, &eff_url);
    if (eff_url) snprintf(pg->url, sizeof(pg->url), "%s", eff_url);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &pg->status);
    pg->size_bytes = body.len;

    curl_easy_cleanup(curl);

    extract_title(body.data, pg->title, sizeof(pg->title));
    pg->body_text = extract_text(body.data);
    pg->link_count = extract_links(body.data, pg->url, pg->links, MAX_LINKS);

    free(body.data);
    return 0;
}

static void print_rule(void) {
    printf("------------------------------------------------\n");
}

static void display_page(const Page *pg) {
    char sizebuf[32];
    format_size(pg->size_bytes, sizebuf, sizeof(sizebuf));

    print_rule();
    printf(" %s\n", pg->title[0] ? pg->title : "(untitled page)");
    print_rule();
    printf(" URL:    %s\n", pg->url);
    printf(" Status: %ld %s\n", pg->status, status_phrase(pg->status));
    printf(" Type:   %s\n", pg->content_type);
    printf(" Size:   %s\n", sizebuf);
    printf(" Server: %s\n", pg->server);
    print_rule();

    if (pg->link_count > 0) {
        printf("Links:\n");
        for (int i = 0; i < pg->link_count; i++) {
            printf("  [%2d] %-40.40s  %s\n", i + 1, pg->links[i].text, pg->links[i].url);
        }
    } else {
        printf("(no links found on this page)\n");
    }
    printf("\n");
}

static void display_help(void) {
    printf(
        "Commands:\n"
        "  <url>       go to a URL, e.g. github.com or https://example.com\n"
        "  <number>    follow that numbered link on the current page\n"
        "  back        go to the previous page\n"
        "  links       re-list links on the current page\n"
        "  text        show extracted page text\n"
        "  history     show pages you've visited\n"
        "  reload      re-fetch the current page\n"
        "  help        show this list\n"
        "  exit/quit   quit\n\n");
}

static void display_text(const Page *pg) {
    if (!pg->body_text || pg->body_text[0] == '\0') {
        printf("(no extractable text on this page)\n\n");
        return;
    }
    print_rule();
    printf("%s\n", pg->body_text);
    print_rule();
    printf("\n");
}

static void display_history(void) {
    if (g_history_count == 0) { printf("(no history yet)\n\n"); return; }
    for (int i = 0; i < g_history_count; i++) {
        printf("  %d. %s\n", i + 1, g_history[i]);
    }
    printf("\n");
}

static int is_number(const char *s) {
    if (*s == '\0') return 0;
    for (const char *p = s; *p; p++) if (!isdigit((unsigned char)*p)) return 0;
    return 1;
}

int main(void) {
    curl_global_init(CURL_GLOBAL_DEFAULT);

    printf("===================\n");
    printf(" Spark Browser — type 'help' for commands\n");
    printf("===================\n\n");

    Page current;
    int have_page = 0;
    char line[1024];

    for (;;) {
        printf("SPB> ");
        fflush(stdout);
        if (!fgets(line, sizeof(line), stdin)) { printf("\nExiting.\n"); break; }
        trim(line);

        if (line[0] == '\0') continue;
        if (strcasecmp(line, "exit") == 0 || strcasecmp(line, "quit") == 0 || strcasecmp(line, "ext") == 0) {
            printf("Exiting.\n");
            break;
        }
        if (strcasecmp(line, "help") == 0) { display_help(); continue; }
        if (strcasecmp(line, "history") == 0) { display_history(); continue; }

        if (strcasecmp(line, "links") == 0) {
            if (!have_page) { printf("No page loaded yet.\n\n"); continue; }
            if (current.link_count == 0) printf("(no links on this page)\n\n");
            else for (int i = 0; i < current.link_count; i++)
                printf("  [%2d] %-40.40s  %s\n", i + 1, current.links[i].text, current.links[i].url);
            if (current.link_count) printf("\n");
            continue;
        }
        if (strcasecmp(line, "text") == 0) {
            if (!have_page) { printf("No page loaded yet.\n\n"); continue; }
            display_text(&current);
            continue;
        }
        if (strcasecmp(line, "reload") == 0) {
            if (!have_page) { printf("No page loaded yet.\n\n"); continue; }
            char errmsg[256];
            char target[URLBUF];
            snprintf(target, sizeof(target), "%s", current.url);
            free(current.body_text);
            if (fetch_page(target, &current, errmsg, sizeof(errmsg)) != 0) {
                printf("Error: %s\n\n", errmsg);
                have_page = 0;
                continue;
            }
            display_page(&current);
            continue;
        }
        if (strcasecmp(line, "back") == 0) {
            char prev[URLBUF];
            if (history_pop(prev, sizeof(prev)) != 0) { printf("No previous page.\n\n"); continue; }
            char errmsg[256];
            if (have_page) free(current.body_text);
            if (fetch_page(prev, &current, errmsg, sizeof(errmsg)) != 0) {
                printf("Error: %s\n\n", errmsg);
                have_page = 0;
                continue;
            }
            have_page = 1;
            display_page(&current);
            continue;
        }

        /* numbered link */
        if (is_number(line)) {
            if (!have_page) { printf("No page loaded yet.\n\n"); continue; }
            int n = atoi(line);
            if (n < 1 || n > current.link_count) { printf("No such link number.\n\n"); continue; }
            char target[URLBUF];
            snprintf(target, sizeof(target), "%s", current.links[n - 1].url);
            history_push(current.url);
            char errmsg[256];
            free(current.body_text);
            if (fetch_page(target, &current, errmsg, sizeof(errmsg)) != 0) {
                printf("Error: %s\n\n", errmsg);
                have_page = 0;
                continue;
            }
            display_page(&current);
            continue;
        }

        /* otherwise: treat as a URL to navigate to */
        {
            char errmsg[256];
            if (have_page) { history_push(current.url); free(current.body_text); }
            if (fetch_page(line, &current, errmsg, sizeof(errmsg)) != 0) {
                printf("Error: %s\n\n", errmsg);
                have_page = 0;
                continue;
            }
            have_page = 1;
            display_page(&current);
        }
    }

    if (have_page) free(current.body_text);
    curl_global_cleanup();
    return 0;
}