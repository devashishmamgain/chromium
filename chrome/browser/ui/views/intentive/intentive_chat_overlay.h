#ifndef CHROME_BROWSER_UI_VIEWS_INTENTIVE_INTENTIVE_CHAT_OVERLAY_H_
#define CHROME_BROWSER_UI_VIEWS_INTENTIVE_INTENTIVE_CHAT_OVERLAY_H_

#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "ui/views/view.h"

namespace views {
class View;
class WebView;
class Widget;
}  // namespace views

namespace content {
class BrowserContext;
}  // namespace content

// Simple content view for the floating overlay. The Widget will own this view.
class IntentiveChatOverlay : public views::View {
 public:
  // Create or toggle the floating widget above |parent_view| and return its Widget.
  static views::Widget* ShowOrToggle(views::View* parent_view,
                                   content::BrowserContext* context);

  explicit IntentiveChatOverlay(content::BrowserContext* context);
  ~IntentiveChatOverlay() override;

 private:
  // Weak pointer to the widget that hosts this view.
  base::WeakPtr<views::Widget> widget_;
  raw_ptr<content::BrowserContext> browser_context_ = nullptr;
  raw_ptr<views::WebView> web_view_ = nullptr;
};

#endif  // CHROME_BROWSER_UI_VIEWS_INTENTIVE_INTENTIVE_CHAT_OVERLAY_H_
