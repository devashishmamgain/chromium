#include "chrome/browser/ui/views/intentive/intentive_app_overlay.h"

#include <memory>

#include "base/strings/utf_string_conversions.h"
#include "content/public/browser/browser_context.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/gfx/geometry/size.h"
#include "ui/views/controls/webview/webview.h"
#include "ui/views/layout/fill_layout.h"

BEGIN_METADATA(IntentiveAppOverlay)
END_METADATA

IntentiveAppOverlay::IntentiveAppOverlay(content::BrowserContext* context)
    : context_(context) {
  SetLayoutManager(std::make_unique<views::FillLayout>());
  SetVisible(false);
}

IntentiveAppOverlay::~IntentiveAppOverlay() = default;

void IntentiveAppOverlay::ShowApp(const GURL& target_url) {
  const std::string key = target_url.spec();
  views::WebView* web_view = nullptr;
  auto it = webviews_by_key_.find(key);
  if (it == webviews_by_key_.end()) {
    auto owned = std::make_unique<views::WebView>(context_);
    web_view = owned.get();
    AddChildView(std::move(owned));
    webviews_by_key_[key] = web_view;
    // First-time navigation.
    web_view->LoadInitialURL(target_url);
  } else {
    web_view = it->second;
  }

  // Show only the selected app.
  for (size_t i = 0; i < children().size(); ++i) {
    views::View* child = children()[i];
    child->SetVisible(false);
  }
  if (web_view)
    web_view->SetVisible(true);
  SetVisible(true);
  InvalidateLayout();
  SchedulePaint();
}

void IntentiveAppOverlay::HideOverlay() {
  for (size_t i = 0; i < children().size(); ++i) {
    views::View* child = children()[i];
    child->SetVisible(false);
  }
  SetVisible(false);
}
