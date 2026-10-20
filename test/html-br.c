/* Callback-level regression tests. Also runnable against the unchanged parser. */
#include "enrmrkd.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef ENRMRKD_FLAG_HTMLBR
#define ENRMRKD_FLAG_HTMLBR 0x1000000
#endif

static char events[16384];
static size_t used;
static int failures;

static int block(ENRMRKD_BLOCKTYPE type, void* detail, void* data)
{
    (void) type; (void) detail; (void) data;
    return 0;
}

static int span(ENRMRKD_SPANTYPE type, void* detail, void* data)
{
    (void) detail; (void) data;
    used += (size_t) snprintf(events+used, sizeof(events)-used, "S%d;", type);
    return 0;
}

static int text(ENRMRKD_TEXTTYPE type, const ENRMRKD_CHAR* str, ENRMRKD_SIZE size, void* data)
{
    (void) data;
    used += (size_t) snprintf(events+used, sizeof(events)-used, "T%d[%.*s];", type, (int) size, str);
    return 0;
}

static const char* parse(const char* input, unsigned flags)
{
    ENRMRKD_PARSER parser = {0, 0, block, block, span, span, text, NULL, NULL};
    parser.flags = flags;
    used = 0;
    events[0] = '\0';
    if(enrmrkd_parse(input, (ENRMRKD_SIZE) strlen(input), &parser, NULL) != 0)
        exit(2);
    return events;
}

static void check(const char* input, unsigned flags, int breaks)
{
    char needle[32];
    const char* result = parse(input, flags);
    const char* pos = result;
    int count = 0;
    snprintf(needle, sizeof(needle), "T%d[\n];", ENRMRKD_TEXT_BR);
    while((pos = strstr(pos, needle)) != NULL) { count++; pos++; }
    if(count != breaks) {
        fprintf(stderr, "FAIL flags=%x input=%s expected=%d actual=%d events=%s\n",
                flags, input, breaks, count, result);
        failures++;
    }
}

static void unchanged(const char* input, unsigned flags)
{
    char before[sizeof(events)];
    strcpy(before, parse(input, flags));
    if(strcmp(before, parse(input, flags | ENRMRKD_FLAG_HTMLBR)) != 0) {
        fprintf(stderr, "FAIL changed literal/control flags=%x input=%s\nbefore=%s\nafter=%s\n",
                flags, input, before, events);
        failures++;
    }
}

int main(void)
{
    const char* tags[] = {"<br>", "<br/>", "<br />", "<BR>", "<Br/>", "<bR />"};
    const char* controls[] = {
        "one<br x>two", "one<br title=\"<br>\">two", "one<span title=\"<br/>\">two", "one<br class=\"x\">two", "one<br >two", "one<br\t>two", "one<br\n>two",
        "one<br  />two", "one<br\t/>two", "one<br/ >two", "one</br>two",
        "one<br", "one<br/", "one< br>two", "one<bravo>two", "one<br\n/>two",
        "one\\<br>two", "one&lt;br&gt;two", "`one<br>two`", "``one<BR/>two``",
        "```html\none<br>two\n```", "    one<br>two\n", "one<b>two</b>",
        "<https://example.org>", "<a@example.org>", "<br@example.org>",
        "[link](https://example.org)", "![alt](image.png)", "one  \ntwo", "one\ntwo"
    };
    unsigned policies[] = {0, ENRMRKD_FLAG_NOHTMLSPANS, ENRMRKD_FLAG_NOHTMLBLOCKS, ENRMRKD_FLAG_NOHTML};
    size_t i, j;
    char input[256];
    char hard_break_events[sizeof(events)];
    strcpy(hard_break_events, parse("one  \ntwo", ENRMRKD_FLAG_NOHTML));
    for(i = 0; i < sizeof(policies)/sizeof(policies[0]); i++) {
        for(j = 0; j < sizeof(tags)/sizeof(tags[0]); j++) {
            snprintf(input, sizeof(input), "one%stwo", tags[j]);
            check(input, policies[i] | ENRMRKD_FLAG_HTMLBR, 1);
            if(strcmp(events, hard_break_events) != 0) {
                fprintf(stderr, "FAIL event sequence differs from Markdown hard break: %s\n", events);
                failures++;
            }
            check(input, policies[i], 0);
            snprintf(input, sizeof(input), "| head |\n| --- |\n| one%stwo |", tags[j]);
            check(input, policies[i] | ENRMRKD_FLAG_HTMLBR | ENRMRKD_FLAG_TABLES, 1);
            check(input, policies[i] | ENRMRKD_FLAG_TABLES, 0);
        }
        for(j = 0; j < sizeof(controls)/sizeof(controls[0]); j++)
            unchanged(controls[j], policies[i]);
        unchanged("text <!-- <br> -->", policies[i]);
        unchanged("text <?test <br> ?>", policies[i]);
        unchanged("text <![CDATA[<br>]]>", policies[i]);
        if(!(policies[i] & ENRMRKD_FLAG_NOHTMLBLOCKS)) {
            unchanged("<div>\none<br>two\n</div>\n", policies[i]);
            unchanged("<video src=\"clip.mp4\">\none<br>two\n</video>", policies[i]);
        }
    }
    check("<br>", ENRMRKD_FLAG_HTMLBR | ENRMRKD_FLAG_NOHTMLBLOCKS, 1);
    unchanged("<br>\n", 0); /* Still an HTML block. */
    check("one<br><br/>two<BR />", ENRMRKD_FLAG_HTMLBR | ENRMRKD_FLAG_NOHTML, 3);
    printf("%s: exact br events, table cells, opt-in and HTML-policy controls\n", failures ? "FAIL" : "PASS");
    return failures ? 1 : 0;
}
