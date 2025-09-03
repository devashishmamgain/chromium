#ifndef CHROME_BROWSER_UI_VIEWS_INTENTIVE_INTENTIVE_SIDEBAR_VIEW_H_
#define CHROME_BROWSER_UI_VIEWS_INTENTIVE_INTENTIVE_SIDEBAR_VIEW_H_

#include <vector>

#include "base/memory/raw_ptr.h"
#include "chrome/browser/ui/browser.h"
#include "ui/base/metadata/metadata_header_macros.h"
#include "ui/views/view.h"
#include "url/gurl.h"
#include <memory>

class PrefService;

namespace views {
class Textfield;
class LabelButton;
class ScrollView;
}

namespace base {
class CancelableTaskTracker;
}

struct IntentiveAppEntry {
  std::string name;
  GURL url;
  int icon_id = 0;  // placeholder
};

class IntentiveSidebarView : public views::View {
  public:
    // IMPORTANT: two-argument form (class, ancestor)
    METADATA_HEADER(IntentiveSidebarView, views::View)

  public:
    using NavigationCallback = base::RepeatingCallback<void(const GURL&)>;

    explicit IntentiveSidebarView(Browser* browser, int width_dip = 64);
    IntentiveSidebarView(NavigationCallback navigation_callback, int width_dip = 64);
    ~IntentiveSidebarView() override;

    void SetApps(std::vector<IntentiveAppEntry> apps);
    int dock_width_dip() const { return width_dip_; }

    // Optional: provide a Browser to enable prefs and tab selection tracking.
    void SetBrowser(Browser* browser);

    // views::View:
    gfx::Size CalculatePreferredSize(
        const views::SizeBounds& available_size) const override;

  private:
    void InitializeView();
    void Rebuild();
    void OnAppPressed(const IntentiveAppEntry& entry);
    void OnAddPressed();
    void OnAppAdded(std::string name, GURL url);
    void PersistAppsIfPossible();
    void LoadButtonIconForUrl(views::LabelButton* button, const GURL& url);

    raw_ptr<Browser> browser_ = nullptr;
    NavigationCallback navigation_callback_;
    int width_dip_ = 64;

    std::vector<IntentiveAppEntry> apps_;

    raw_ptr<views::View> rail_ = nullptr;
    raw_ptr<views::ScrollView> apps_scroll_ = nullptr;
    raw_ptr<views::View> apps_container_ = nullptr;
    raw_ptr<views::LabelButton> add_button_ = nullptr;
    raw_ptr<views::View> content_container_ = nullptr;

    int selected_index_ = -1;

    std::unique_ptr<base::CancelableTaskTracker> favicon_task_tracker_;
};

#endif  // CHROME_BROWSER_UI_VIEWS_INTENTIVE_INTENTIVE_SIDEBAR_VIEW_H_
