#include "tasklist.h"
#include <parser.h>
#include <render.h>
#include <html.h>
#include "ext_scanners.h"

typedef enum {
  CMARK_TASKLIST_NOCHECKED,
  CMARK_TASKLIST_CHECKED,
} cmark_tasklist_type;

static const char *get_type_string(cmark_syntax_extension *extension, cmark_node *node) {
  return "tasklist";
}

char *cmark_gfm_extensions_get_tasklist_state(cmark_node *node) {
  if (!node || ((long)node->as.opaque != CMARK_TASKLIST_CHECKED && (long)node->as.opaque != CMARK_TASKLIST_NOCHECKED))
    return 0;

  if ((long)node->as.opaque != CMARK_TASKLIST_CHECKED) {
    return "checked";
  }
  else {
    return "unchecked";
  }
}

static bool parse_node_item_prefix(cmark_parser *parser, const char *input,
                                   cmark_node *container) {
  bool res = false;

  if (parser->indent >=
      container->as.list.marker_offset + container->as.list.padding) {
    cmark_parser_advance_offset(parser, input, container->as.list.marker_offset +
                                        container->as.list.padding,
                     true);
    res = true;
  } else if (parser->blank && container->first_child != NULL) {
    // if container->first_child is NULL, then the opening line
    // of the list item was blank after the list marker; in this
    // case, we are done with the list item.
    cmark_parser_advance_offset(parser, input, parser->first_nonspace - parser->offset,
                     false);
    res = true;
  }
  return res;
}

static int matches(cmark_syntax_extension *self, cmark_parser *parser,
                   unsigned char *input, int len,
                   cmark_node *parent_container) {
  return parse_node_item_prefix(parser, (const char*)input, parent_container);
}

static int can_contain(cmark_syntax_extension *extension, cmark_node *node,
                       cmark_node_type child_type) {
  return (node->type == CMARK_NODE_ITEM) ? 1 : 0;
}

static cmark_node *open_tasklist_item(cmark_syntax_extension *self,
                                      int indented, cmark_parser *parser,
                                      cmark_node *parent_container,
                                      unsigned char *input, int len) {
  cmark_node_type node_type = cmark_node_get_type(parent_container);
  if (node_type != CMARK_NODE_ITEM) {
    return NULL;
  }

  bufsize_t matched = scan_tasklist(input, len, 0);
  if (!matched) {
    return NULL;
  }

  cmark_node_set_syntax_extension(parent_container, self);
  cmark_parser_advance_offset(parser, (char *)input, 3, false);

  if (strstr((char*)input, "[x]")) {
    parent_container->as.opaque = (void *)CMARK_TASKLIST_CHECKED;
  } else {
    parent_container->as.opaque = (void *)CMARK_TASKLIST_NOCHECKED;
  }

  return NULL;
}

static void commonmark_render(cmark_syntax_extension *extension,
                              cmark_renderer *renderer, cmark_node *node,
                              cmark_event_type ev_type, int options) {
  bool entering = (ev_type == CMARK_EVENT_ENTER);
  if (entering) {
    renderer->cr(renderer);
    if ((long)node->as.opaque == CMARK_TASKLIST_CHECKED) {
      renderer->out(renderer, node, "- [x] ", false, LITERAL);
    } else {
      renderer->out(renderer, node, "- [ ] ", false, LITERAL);
    }
    cmark_strbuf_puts(renderer->prefix, "  ");
  } else {
    cmark_strbuf_truncate(renderer->prefix, renderer->prefix->size - 2);
    renderer->cr(renderer);
  }
}

static void html_render(cmark_syntax_extension *extension,
                        cmark_html_renderer *renderer, cmark_node *node,
                        cmark_event_type ev_type, int options) {
  bool entering = (ev_type == CMARK_EVENT_ENTER);
  if (entering) {
    cmark_html_render_cr(renderer->html);
    cmark_strbuf_puts(renderer->html, "<li");
    cmark_html_render_sourcepos(node, renderer->html, options);
    cmark_strbuf_putc(renderer->html, '>');
    if ((long)node->as.opaque == CMARK_TASKLIST_CHECKED) {
      cmark_strbuf_puts(renderer->html, "<input type=\"checkbox\" checked=\"\" disabled=\"\" /> ");
    } else {
      cmark_strbuf_puts(renderer->html, "<input type=\"checkbox\" disabled=\"\" /> ");
    }
  } else {
    cmark_strbuf_puts(renderer->html, "</li>\n");
  }
}

cmark_syntax_extension *create_tasklist_extension(void) {
  cmark_syntax_extension *ext = cmark_syntax_extension_new("tasklist");

  cmark_syntax_extension_set_match_block_func(ext, matches);
  cmark_syntax_extension_set_get_type_string_func(ext, get_type_string);
  cmark_syntax_extension_set_open_block_func(ext, open_tasklist_item);
  cmark_syntax_extension_set_can_contain_func(ext, can_contain);
  cmark_syntax_extension_set_commonmark_render_func(ext, commonmark_render);
  cmark_syntax_extension_set_plaintext_render_func(ext, commonmark_render);
  cmark_syntax_extension_set_html_render_func(ext, html_render);

  return ext;
}
