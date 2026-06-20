#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <esp_sleep.h>
#include <esp_timer.h>
#include <esp_log.h>
#include "boards/Board.h"
#include "boards/battery/Battery.h"
#include "boards/controls/RLCDButtonControls.h"
#include "EpubList/Epub.h"
#include "EpubList/EpubList.h"
#include "EpubList/EpubReader.h"
#include "EpubList/EpubToc.h"
#include "EpubList/State.h"

#ifdef LOG_ENABLED
#define LOG_LEVEL ESP_LOG_INFO
#else
#define LOG_LEVEL ESP_LOG_NONE
#endif

extern "C"
{
  void app_main();
}

const char *TAG = "main";

typedef enum
{
  SELECTING_EPUB,
  SELECTING_TABLE_CONTENTS,
  READING_EPUB
} UIState;

RTC_NOINIT_ATTR UIState ui_state = SELECTING_EPUB;
RTC_DATA_ATTR EpubListState epub_list_state;
RTC_DATA_ATTR EpubTocState epub_index_state;

static EpubList *epub_list = nullptr;
static EpubReader *reader = nullptr;
static EpubToc *contents = nullptr;

void handleEpub(Renderer *renderer, UIAction action);
void handleEpubList(Renderer *renderer, UIAction action, bool needs_redraw);
void handleEpubTableContents(Renderer *renderer, UIAction action, bool needs_redraw);

void handleEpub(Renderer *renderer, UIAction action)
{
  if (!reader)
  {
    reader = new EpubReader(epub_list_state.epub_list[epub_list_state.selected_item], renderer);
    reader->load();
  }
  switch (action)
  {
  case UP:
    reader->prev();
    break;
  case DOWN:
    reader->next();
    break;
  case SELECT:
    ui_state = SELECTING_EPUB;
    renderer->clear_screen();
    delete reader;
    reader = nullptr;
    if (!epub_list)
    {
      epub_list = new EpubList(renderer, epub_list_state);
    }
    handleEpubList(renderer, NONE, true);
    return;
  case NONE:
  default:
    break;
  }
  reader->render();
}

void handleEpubTableContents(Renderer *renderer, UIAction action, bool needs_redraw)
{
  if (!contents)
  {
    contents = new EpubToc(epub_list_state.epub_list[epub_list_state.selected_item], epub_index_state, renderer);
    contents->set_needs_redraw();
    contents->load();
  }
  switch (action)
  {
  case UP:
    contents->prev();
    break;
  case DOWN:
    contents->next();
    break;
  case SELECT:
    ui_state = READING_EPUB;
    reader = new EpubReader(epub_list_state.epub_list[epub_list_state.selected_item], renderer);
    reader->set_state_section(contents->get_selected_toc());
    reader->load();
    delete contents;
    contents = nullptr;
    handleEpub(renderer, NONE);
    return;
  case NONE:
  default:
    break;
  }
  contents->render();
}

void handleEpubList(Renderer *renderer, UIAction action, bool needs_redraw)
{
  if (!epub_list)
  {
    ESP_LOGI(TAG, "Creating epub list");
    epub_list = new EpubList(renderer, epub_list_state);
    if (epub_list->load("/fs/"))
    {
      ESP_LOGI(TAG, "Epub files loaded");
    }
  }
  if (needs_redraw)
  {
    epub_list->set_needs_redraw();
  }
  switch (action)
  {
  case UP:
    epub_list->prev();
    break;
  case DOWN:
    epub_list->next();
    break;
  case SELECT:
    ui_state = SELECTING_TABLE_CONTENTS;
    contents = new EpubToc(epub_list_state.epub_list[epub_list_state.selected_item], epub_index_state, renderer);
    contents->load();
    contents->set_needs_redraw();
    handleEpubTableContents(renderer, NONE, true);
    return;
  case NONE:
  default:
    break;
  }
  epub_list->render();
}

void handleUserInteraction(Renderer *renderer, UIAction ui_action, bool needs_redraw)
{
  switch (ui_state)
  {
  case READING_EPUB:
    handleEpub(renderer, ui_action);
    break;
  case SELECTING_TABLE_CONTENTS:
    handleEpubTableContents(renderer, ui_action, needs_redraw);
    break;
  case SELECTING_EPUB:
  default:
    handleEpubList(renderer, ui_action, needs_redraw);
    break;
  }
}

void main_task(void *param)
{
  ESP_LOGI(TAG, "Powering up the board");
  
  Board *board = Board::factory();
  if (!board)
  {
    ESP_LOGE(TAG, "Failed to create board");
    return;
  }
  
  board->power_up();
  
  ESP_LOGI(TAG, "Creating renderer");
  Renderer *renderer = board->get_renderer();
  if (!renderer)
  {
    ESP_LOGE(TAG, "Failed to create renderer");
    return;
  }
  
  ESP_LOGI(TAG, "Starting file system");
  board->start_filesystem();
  
  ESP_LOGI(TAG, "Starting battery monitor");
  Battery *battery = board->get_battery();
  if (battery)
  {
    battery->setup();
  }
  
  renderer->set_margin_top(35);
  renderer->set_margin_left(10);
  renderer->set_margin_right(10);
  
  QueueHandle_t ui_queue = xQueueCreate(10, sizeof(UIAction));
  
  ESP_LOGI(TAG, "Setting up controls");
  RLCDButtonControls *button_controls = new RLCDButtonControls(
    BUTTON_UP_GPIO, BUTTON_DOWN_GPIO, BUTTON_SELECT_GPIO,
    BUTTON_ACTIVE_LEVEL, ui_queue
  );
  button_controls->setup();
  
  ESP_LOGI(TAG, "Controls configured");
  
  if (button_controls->did_wake_from_deep_sleep())
  {
    bool hydrate_success = renderer->hydrate();
    UIAction ui_action = button_controls->get_deep_sleep_action();
    handleUserInteraction(renderer, ui_action, !hydrate_success);
  }
  else
  {
    renderer->reset();
    handleUserInteraction(renderer, NONE, true);
  }
  
  if (battery)
  {
    ESP_LOGI(TAG, "Battery: %.2fV (%.0f%%)", 
             battery->get_voltage(), battery->get_percentage());
  }
  
  renderer->flush_display();
  
  int64_t last_user_interaction = esp_timer_get_time();
  while (esp_timer_get_time() - last_user_interaction < 120 * 1000 * 1000)
  {
    UIAction ui_action = NONE;
    
    if (xQueueReceive(ui_queue, &ui_action, pdMS_TO_TICKS(60000)) == pdTRUE)
    {
      if (ui_action != NONE)
      {
        last_user_interaction = esp_timer_get_time();
        handleUserInteraction(renderer, ui_action, false);
      }
    }
    
    if (battery)
    {
      ESP_LOGI(TAG, "Battery: %.2fV (%.0f%%)",
               battery->get_voltage(), battery->get_percentage());
    }
    
    renderer->flush_display();
  }
  
  ESP_LOGI(TAG, "Saving state");
  renderer->dehydrate();
  
  board->stop_filesystem();
  board->prepare_to_sleep();
  
  ESP_LOGI(TAG, "Entering deep sleep");
  button_controls->setup_deep_sleep();
  vTaskDelay(pdMS_TO_TICKS(500));
  esp_deep_sleep_start();
}

void app_main()
{
  esp_log_level_set("main", LOG_LEVEL);
  esp_log_level_set("RLCD_Board", LOG_LEVEL);
  esp_log_level_set("RLCDRenderer", LOG_LEVEL);
  esp_log_level_set("EPUB", LOG_LEVEL);
  esp_log_level_set("PUBLIST", LOG_LEVEL);
  esp_log_level_set("ZIP", LOG_LEVEL);
  esp_log_level_set("HTML", LOG_LEVEL);
  esp_log_level_set("EREADER", LOG_LEVEL);
  esp_log_level_set("SDC", LOG_LEVEL);
  
  ESP_LOGI(TAG, "Starting RLCD EPUB Reader");
  ESP_LOGI(TAG, "Free heap: %lu bytes", (unsigned long)esp_get_free_heap_size());
  
  xTaskCreatePinnedToCore(main_task, "main_task", 65536, NULL, 1, NULL, 1);
}
