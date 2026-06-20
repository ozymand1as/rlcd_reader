#pragma once

#include "../../Renderer/Renderer.h"
#include <vector>
#include "Block.h"

typedef enum
{
  BOLD_SPAN = 1,
  ITALIC_SPAN = 2,
} SPAN_STYLE;

typedef enum
{
  JUSTIFIED = 0,
  LEFT_ALIGN = 1,
  CENTER_ALIGN = 2,
  RIGHT_ALIGN = 3,
} BLOCK_STYLE;

class TextBlock : public Block
{
private:
  std::vector<const char *> spans;
  std::vector<const char *> words;
  std::vector<uint16_t> word_widths;
  std::vector<uint16_t> word_xpos;
  std::vector<uint8_t> word_styles;
  BLOCK_STYLE style;

public:
  std::vector<uint16_t> line_breaks;

  void add_span(const char *span, bool is_bold, bool is_italic);
  TextBlock(BLOCK_STYLE style) : style(style) {}
  ~TextBlock()
  {
    for (auto span : spans)
    {
      delete[] span;
    }
  }
  void set_style(BLOCK_STYLE style) { this->style = style; }
  BLOCK_STYLE get_style() { return style; }
  bool isEmpty() { return spans.empty(); }
  void layout(Renderer *renderer, Epub *epub, int max_width = -1);
  void render(Renderer *renderer, int line_break_index, int x_pos, int y_pos);
  void dump();
  bool is_empty() { return words.empty(); }
  virtual BlockType getType() { return TEXT_BLOCK; }
};
