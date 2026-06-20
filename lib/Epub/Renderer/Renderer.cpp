#include "Renderer.h"
#include <cstring>

Renderer::~Renderer()
{
}

void Renderer::draw_image(const std::string &filename, const uint8_t *data, 
                          size_t data_size, int x, int y, int width, int height)
{
  // Base implementation does nothing
  // Subclasses should override this
}

bool Renderer::get_image_size(const std::string &filename, const uint8_t *data, 
                              size_t data_size, int *width, int *height)
{
  // Base implementation returns false
  // Subclasses should override this
  return false;
}

void Renderer::draw_text_box(const std::string &text, int x, int y, 
                             int width, int height, bool bold, bool italic)
{
  // Simple text box rendering with word wrap
  int cursor_x = x;
  int cursor_y = y;
  int line_height = get_line_height();
  int space_width = get_space_width();
  
  const char *str = text.c_str();
  const char *word_start = str;
  bool in_word = false;
  
  while (*str)
  {
    if (*str == ' ' || *str == '\n')
    {
      if (in_word)
      {
        // Draw the word
        int word_len = str - word_start;
        char word[MAX_WORD_LENGTH + 1];
        if (word_len > MAX_WORD_LENGTH) word_len = MAX_WORD_LENGTH;
        strncpy(word, word_start, word_len);
        word[word_len] = '\0';
        
        int word_width = get_text_width(word, bold, italic);
        
        // Check if we need to wrap
        if (cursor_x + word_width > x + width)
        {
          cursor_x = x;
          cursor_y += line_height;
          
          // Check if we've exceeded the box height
          if (cursor_y + line_height > y + height)
          {
            break;
          }
        }
        
        draw_text(cursor_x, cursor_y, word, bold, italic);
        cursor_x += word_width + space_width;
        in_word = false;
      }
      
      if (*str == '\n')
      {
        cursor_x = x;
        cursor_y += line_height;
        
        if (cursor_y + line_height > y + height)
        {
          break;
        }
      }
    }
    else
    {
      if (!in_word)
      {
        word_start = str;
        in_word = true;
      }
    }
    str++;
  }
  
  // Draw the last word if there is one
  if (in_word)
  {
    int word_len = str - word_start;
    char word[MAX_WORD_LENGTH + 1];
    if (word_len > MAX_WORD_LENGTH) word_len = MAX_WORD_LENGTH;
    strncpy(word, word_start, word_len);
    word[word_len] = '\0';
    
    draw_text(cursor_x, cursor_y, word, bold, italic);
  }
}

ImageHelper *Renderer::get_image_helper(const std::string &filename, 
                                        const uint8_t *data, size_t data_size)
{
  // Determine image type from filename
  size_t len = filename.length();
  if (len >= 4)
  {
    const char *ext = filename.c_str() + len - 4;
    if (strcasecmp(ext, ".png") == 0)
    {
      // Return PNG helper
      // TODO: Implement PNG helper
    }
    else if (strcasecmp(ext, ".jpg") == 0 || strcasecmp(ext, ".jpeg") == 0)
    {
      // Return JPEG helper
      // TODO: Implement JPEG helper
    }
  }
  return nullptr;
}
