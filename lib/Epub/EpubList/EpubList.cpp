#include "EpubList.h"
#include <esp_log.h>
#include <dirent.h>

static const char *TAG = "EpubList";

EpubList::EpubList(Renderer *renderer, EpubListState &state)
  : m_renderer(renderer), m_state(state)
{
}

EpubList::~EpubList()
{
}

bool EpubList::load(const std::string &path)
{
  ESP_LOGI(TAG, "Loading EPUB list from %s", path.c_str());
  
  DIR *dir = opendir(path.c_str());
  if (!dir)
  {
    ESP_LOGE(TAG, "Failed to open directory: %s", path.c_str());
    return false;
  }
  
  m_state.epub_list.clear();
  
  struct dirent *entry;
  while ((entry = readdir(dir)) != nullptr)
  {
    std::string filename = entry->d_name;
    
    // Check if it's an EPUB file
    if (filename.length() > 5 && 
        filename.substr(filename.length() - 5) == ".epub")
    {
      m_state.epub_list.push_back(filename);
      ESP_LOGI(TAG, "Found EPUB: %s", filename.c_str());
    }
  }
  
  closedir(dir);
  
  m_state.num_epubs = m_state.epub_list.size();
  m_state.is_loaded = true;
  
  ESP_LOGI(TAG, "Found %d EPUB files", m_state.num_epubs);
  return true;
}

void EpubList::render()
{
  ESP_LOGI(TAG, "Rendering EPUB list");
  
  m_renderer->clear_screen();
  m_renderer->draw_text(10, 10, "EPUB Files", true);
  
  if (m_state.num_epubs == 0)
  {
    m_renderer->draw_text(10, 30, "No EPUB files found", false);
    m_renderer->draw_text(10, 50, "Insert SD card with", false);
    m_renderer->draw_text(10, 60, "EPUB files", false);
    return;
  }
  
  // Show list of EPUBs
  int y = 30;
  int max_visible = 10;
  int start = 0;
  
  if (m_state.selected_item >= max_visible)
  {
    start = m_state.selected_item - max_visible + 1;
  }
  
  for (int i = start; i < m_state.num_epubs && i < start + max_visible; i++)
  {
    std::string display_name = m_state.epub_list[i];
    
    // Remove .epub extension for display
    size_t ext_pos = display_name.rfind(".epub");
    if (ext_pos != std::string::npos)
    {
      display_name = display_name.substr(0, ext_pos);
    }
    
    // Truncate if too long
    if (display_name.length() > 35)
    {
      display_name = display_name.substr(0, 32) + "...";
    }
    
    // Highlight selected item
    bool is_selected = (i == m_state.selected_item);
    
    if (is_selected)
    {
      m_renderer->fill_rect(0, y - 2, m_renderer->get_page_width(), 12, 0);
      m_renderer->draw_text(10, y, display_name.c_str(), false);
    }
    else
    {
      m_renderer->draw_text(10, y, display_name.c_str(), false);
    }
    
    y += 14;
  }
  
  // Show scroll indicator
  if (m_state.num_epubs > max_visible)
  {
    int scroll_y = 30 + (m_state.selected_item * 140 / m_state.num_epubs);
    m_renderer->fill_rect(m_renderer->get_page_width() - 5, scroll_y, 3, 10, 0);
  }
}

void EpubList::prev()
{
  if (m_state.selected_item > 0)
  {
    m_state.selected_item--;
  }
}

void EpubList::next()
{
  if (m_state.selected_item < m_state.num_epubs - 1)
  {
    m_state.selected_item++;
  }
}

void EpubList::set_needs_redraw()
{
  // Force redraw on next render() call
}

std::string EpubList::get_selected_item()
{
  if (m_state.selected_item >= 0 && 
      m_state.selected_item < m_state.num_epubs)
  {
    return m_state.epub_list[m_state.selected_item];
  }
  return "";
}
