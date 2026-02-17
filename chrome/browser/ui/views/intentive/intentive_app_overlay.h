#ifndef CHROME_BROWSER_UI_VIEWS_INTENTIVE_INTENTIVE_APP_OVERLAY_H_
#define CHROME_BROWSER_UI_VIEWS_INTENTIVE_INTENTIVE_APP_OVERLAY_H_

#include <map>
#include <string>

#include "base/memory/raw_ptr.h"
#include "ui/views/view.h"
#include "ui/base/metadata/metadata_header_macros.h"
#include "url/gurl.h"

namespace content {
class BrowserContext;
}

namespace views {
class WebView;
}

// A simple overlay container that manages one WebView per app key and
// shows/hides them on demand. Intended to be added to an overlay layer above
// the normal contents container so these apps do not appear in the tab strip.
class IntentiveAppOverlay : public views::View {
 public:
  METADATA_HEADER(IntentiveAppOverlay, views::View)

  // Re-open public section after METADATA_HEADER which ends with `private:`.
  public:
  explicit IntentiveAppOverlay(content::BrowserContext* context);
  ~IntentiveAppOverlay() override;

  // Shows the app view for `target_url`. Uses the URL spec as a stable key.
  // Creates the WebView if missing and navigates it once. Hides all others.
  void ShowApp(const GURL& target_url);

  // Hides all app views and the overlay itself.
  void HideOverlay();

 private:
  raw_ptr<content::BrowserContext> context_ = nullptr;
  std::map<std::string, raw_ptr<views::WebView>> webviews_by_key_;
};

#endif  // CHROME_BROWSER_UI_VIEWS_INTENTIVE_INTENTIVE_APP_OVERLAY_H_
