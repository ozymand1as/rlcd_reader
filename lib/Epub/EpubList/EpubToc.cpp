#include "EpubToc.h"

static const char *TAG = "PUBINDEX";
#define PADDING 14
#define ITEMS_PER_PAGE 6

void EpubToc::next()
{
  if (!epub)
  {
    load();
  }
  state.selected_item = (state.selected_item + 1) % epub->get_toc_items_count();
}

void EpubToc::prev()
{
  if (!epub)
  {
    load();
  }
  state.selected_item = (state.selected_item - 1 + epub->get_toc_items_count()) % epub->get_toc_items_count();
}

bool EpubToc::load()
{
  ESP_LOGI(TAG, "load");

  if (!epub || epub->get_path() != selected_epub.path)
  {
    renderer->show_busy();
    delete epub;

    epub = new Epub(selected_epub.path);
    if (epub->load())
    {
      ESP_LOGI(TAG, "Epub index loaded");
      return false;
    }
  }
  return true;
}

void EpubToc::render()
{
  ESP_LOGD(TAG, "Rendering EPUB index");
  int current_page = state.selected_item / ITEMS_PER_PAGE;
  int cell_height = renderer->get_page_height() / ITEMS_PER_PAGE;
  int start_index = current_page * ITEMS_PER_PAGE;
  int ypos = 0;
  
  if (current_page != state.previous_rendered_page || m_needs_redraw)
  {
    m_needs_redraw = false;
    renderer->clear_screen();
    state.previous_selected_item = -1;
    state.previous_rendered_page = -1;
  }
  
  for (int i = start_index; i < start_index + ITEMS_PER_PAGE && i < epub->get_toc_items_count(); i++)
  {
    if (current_page != state.previous_rendered_page)
    {
      TextBlock *title_block = new TextBlock(LEFT_ALIGN);
      title_block->add_span(epub->get_toc_item(i).title.c_str(), false, false);
      title_block->layout(renderer, epub, renderer->get_page_width());
      
      int text_height = cell_height - PADDING;
      int title_height = title_block->line_breaks.size() * renderer->get_line_height();
      int y_offset = title_height < text_height ? (text_height - title_height) / 2 : 0;
      
      int height = 0;
      for (int i = 0; i < title_block->line_breaks.size() && height < text_height; i++)
      {
        title_block->render(renderer, i, 10, ypos + height + y_offset);
        height += renderer->get_line_height();
      }
      delete title_block;
    }
    
    if (state.previous_selected_item == i)
    {
      for (int line = 0; line < 3; line++)
      {
        renderer->draw_rect(line, ypos + PADDING / 2 + line, renderer->get_page_width() - 2 * line, cell_height - PADDING - 2 * line, 255);
      }
    }
    
    if (state.selected_item == i)
    {
      for (int line = 0; line < 3; line++)
      {
        renderer->draw_rect(line, ypos + PADDING / 2 + line, renderer->get_page_width() - 2 * line, cell_height - PADDING - 2 * line, 0);
      }
    }
    ypos += cell_height;
  }
  
  state.previous_selected_item = state.selected_item;
  state.previous_rendered_page = current_page;
}

uint16_t EpubToc::get_selected_toc()
{
  return epub->get_spine_index_for_toc_index(state.selected_item);
}
