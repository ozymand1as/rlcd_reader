#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include "TextBlock.h"
#include <esp_log.h>

static bool is_whitespace(char c)
{
  return (c == ' ' || c == '\r' || c == '\n');
}

static int skip_word(const char *text, int index, int length)
{
  while (index < length && !is_whitespace(text[index]))
  {
    index++;
  }
  return index;
}

static int skip_whitespace(const char *html, int index, int length)
{
  while (index < length && is_whitespace(html[index]))
  {
    index++;
  }
  return index;
}

void TextBlock::add_span(const char *span, bool is_bold, bool is_italic)
{
  int length = strlen(span);
  char *text = new char[length + 1];
  strcpy(text, span);
  spans.push_back(text);
  
  int index = 0;
  while (index < length)
  {
    index = skip_whitespace(span, index, length);
    int word_start = index;
    index = skip_word(span, index, length);
    int word_length = index - word_start;
    if (word_length > 0)
    {
      text[word_start + word_length] = '\0';
      words.push_back(text + word_start);
      word_styles.push_back((is_bold ? BOLD_SPAN : 0) | (is_italic ? ITALIC_SPAN : 0));
    }
  }
}

void TextBlock::layout(Renderer *renderer, Epub *epub, int max_width)
{
  for (int i = 0; i < words.size(); i++)
  {
    int width = renderer->get_text_width(words[i], word_styles[i] & BOLD_SPAN, word_styles[i] & ITALIC_SPAN);
    word_widths.push_back(width);
  }

  int page_width = max_width != -1 ? max_width : renderer->get_page_width();
  int space_width = renderer->get_space_width();

  int n = word_widths.size();
  int dp[n];
  size_t ans[n];

  dp[n - 1] = 0;
  ans[n - 1] = n - 1;

  for (int i = n - 2; i >= 0; i--)
  {
    int currlen = -1;
    dp[i] = INT_MAX;
    int cost;

    for (int j = i; j < n; j++)
    {
      currlen += (word_widths[j] + space_width);
      if (currlen > page_width)
        break;

      if (j == n - 1)
        cost = 0;
      else
        cost = (page_width - currlen) * (page_width - currlen) + dp[j + 1];

      if (cost < dp[i])
      {
        dp[i] = cost;
        ans[i] = j;
      }
    }
  }

  size_t i = 0;
  while (i < n)
  {
    i = ans[i] + 1;
    if (i > n)
    {
      ESP_LOGI("TextBlock", "fallen off the end of the words");
      break;
    }
    line_breaks.push_back(i);
    if (line_breaks.size() > 1000)
    {
      ESP_LOGE("TextBlock", "too many line breaks");
      break;
    }
  }

  int start_word = 0;
  for (int i = 0; i < line_breaks.size(); i++)
  {
    int total_word_width = 0;
    for (int word_index = start_word; word_index < line_breaks[i]; word_index++)
    {
      total_word_width += word_widths[word_index];
    }
    float spare_space = page_width - total_word_width;
    float actual_spacing = space_width;
    int number_words = line_breaks[i] - start_word;
    
    if (i != line_breaks.size() - 1 && style == JUSTIFIED)
    {
      if (line_breaks[i] - start_word > 1)
      {
        actual_spacing = spare_space / float(number_words - 1);
      }
    }
    
    float xpos = 0;
    if (style == RIGHT_ALIGN)
    {
      xpos = spare_space - (number_words - 1) * space_width;
    }
    if (style == CENTER_ALIGN)
    {
      xpos = (spare_space - (number_words - 1) * space_width) / 2;
    }
    
    word_xpos.resize(words.size());
    for (int word_index = start_word; word_index < line_breaks[i]; word_index++)
    {
      word_xpos[word_index] = xpos;
      xpos += word_widths[word_index] + actual_spacing;
    }
    start_word = line_breaks[i];
  }
  
  spans.shrink_to_fit();
  words.shrink_to_fit();
  word_widths.shrink_to_fit();
  word_xpos.shrink_to_fit();
  word_styles.shrink_to_fit();
}

void TextBlock::render(Renderer *renderer, int line_break_index, int x_pos, int y_pos)
{
  int start = line_break_index == 0 ? 0 : line_breaks[line_break_index - 1];
  int end = line_breaks[line_break_index];
  for (int i = start; i < end; i++)
  {
    uint8_t style = word_styles[i];
    renderer->draw_text(x_pos + word_xpos[i], y_pos, words[i], style & BOLD_SPAN, style & ITALIC_SPAN);
  }
}

void TextBlock::dump()
{
  for (int i = 0; i < words.size(); i++)
  {
    printf("##%d#%s## ", word_widths[i], words[i]);
  }
}
